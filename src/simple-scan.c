/*
 * Copyright (C) 2009 Canonical Ltd.
 * Author: Robert Ancell <robert.ancell@canonical.com>
 * 
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version. See http://www.gnu.org/copyleft/gpl.html the full text of the
 * license.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <cairo/cairo-pdf.h>
#include <cairo/cairo-ps.h>
#include <math.h>

#include "ui.h"
#include "scanner.h"


static const char *default_device = NULL;

static SimpleScan *ui;

static Scanner *scanner;

typedef struct
{
    gint dpi;
    GdkPixbuf *image;
} ScannedImage;

static ScannedImage *raw_image = NULL;

static gboolean scanning = FALSE;

static int current_line = 0, page_count = 0;


static void
scanner_ready_cb (Scanner *scanner)
{
    scanning = FALSE;
    ui_set_scanning (ui, FALSE);
    ui_redraw_preview (ui);
}


static void
update_scan_devices_cb (Scanner *scanner, GList *devices)
{
    GList *dev_iter;

    /* Mark existing values as undetected */
    ui_mark_devices_undetected (ui);

    /* Add/update detected devices */
    for (dev_iter = devices; dev_iter; dev_iter = dev_iter->next) {
        ScanDevice *device = dev_iter->data;       
        ui_add_scan_device (ui, device->name, device->label);
    }
}


static void
scanner_page_info_cb (Scanner *scanner, ScanPageInfo *info)
{
    gint height;

    g_debug ("Page is %d pixels wide, %d pixels high, %d bits per pixel",
             info->width, info->height, info->depth);

    /* Variable heigh, try 50% of the width for now */
    if (info->height < 0)
        height = info->width / 2;
    else
        height = info->height;

    raw_image->image = gdk_pixbuf_new (GDK_COLORSPACE_RGB, FALSE,
                                       8, // Pixbuf only supports 8 bit images
                                       //info->depth,
                                       info->width,
                                       height);
    memset (gdk_pixbuf_get_pixels (raw_image->image), 0xFF,
            gdk_pixbuf_get_height (raw_image->image) * gdk_pixbuf_get_rowstride (raw_image->image));

    current_line = 0;
    page_count++;
    ui_set_page_count (ui, page_count);
    ui_set_selected_page (ui, page_count);
}


static gint
get_sample (guchar *data, int depth, int index)
{
    int i, offset, value, n_bits;

    /* Optimise if using 8 bit samples */
    if (depth == 8)
        return data[index];

    /* Bit offset for this sample */
    offset = depth * index;

    /* Get the remaining bits in the octet this sample starts in */
    i = offset / 8;
    n_bits = 8 - offset % 8;
    value = data[i] & (0xFF >> (8 - n_bits));
    
    /* Add additional octets until get enough bits */
    while (n_bits < depth) {
        value = value << 8 | data[i++];
        n_bits += 8;
    }

    /* Trim remaining bits off */
    if (n_bits > depth)
        value >>= n_bits - depth;

    return value;
}


static void
scanner_line_cb (Scanner *scanner, ScanLine *line)
{
    guchar *pixels;
    gint i, j;
    
    /* Extend image if necessary */
    while (line->number >= gdk_pixbuf_get_height (raw_image->image)) {
        GdkPixbuf *image;
        gint height, width, new_height;

        width = gdk_pixbuf_get_width (raw_image->image);
        height = gdk_pixbuf_get_height (raw_image->image);
        new_height = height + width / 2;
        g_debug("Resizing image height from %d pixels to %d pixels", height, new_height);

        image = gdk_pixbuf_new (GDK_COLORSPACE_RGB, FALSE,
                                gdk_pixbuf_get_bits_per_sample (raw_image->image),
                                width, new_height);
        memset (gdk_pixbuf_get_pixels (raw_image->image), 0xFF,
                gdk_pixbuf_get_height (raw_image->image) * gdk_pixbuf_get_rowstride (raw_image->image));
        memcpy (gdk_pixbuf_get_pixels (image),
                gdk_pixbuf_get_pixels (raw_image->image),
                height * gdk_pixbuf_get_rowstride (raw_image->image));
        g_object_unref (raw_image->image);
        raw_image->image = image;
    }

    pixels = gdk_pixbuf_get_pixels (raw_image->image) + line->number * gdk_pixbuf_get_rowstride (raw_image->image);
    switch (line->format) {
    case LINE_RGB:
        if (line->depth == 8) {
            memcpy (pixels, line->data, line->data_length);
        } else {
            for (i = 0, j = 0; i < line->width; i++) {
                pixels[j] = get_sample (line->data, line->depth, j) * 0xFF / (1 << (line->depth - 1));
                pixels[j+1] = get_sample (line->data, line->depth, j+1) * 0xFF / (1 << (line->depth - 1));
                pixels[j+2] = get_sample (line->data, line->depth, j+2) * 0xFF / (1 << (line->depth - 1));
                j += 3;
            }
        }
        break;
    case LINE_GRAY:
        for (i = 0, j = 0; i < line->width; i++) {
            int sample;

            /* Bitmap, 0 = white, 1 = black */
            sample = get_sample (line->data, line->depth, i) * 0xFF / (1 << (line->depth - 1));
            if (line->depth == 1)
                sample = sample ? 0x00 : 0xFF;

            pixels[j] = pixels[j+1] = pixels[j+2] = sample;
            j += 3;
        }
        break;
    case LINE_RED:
        for (i = 0, j = 0; i < line->width; i++) {
            pixels[j] = get_sample (line->data, line->depth, i) * 0xFF / (1 << (line->depth - 1));
            j += 3;
        }
        break;
    case LINE_GREEN:
        for (i = 0, j = 0; i < line->width; i++) {
            pixels[j+1] = get_sample (line->data, line->depth, i) * 0xFF / (1 << (line->depth - 1));
            j += 3;
        }
        break;
    case LINE_BLUE:
        for (i = 0, j = 0; i < line->width; i++) {
            pixels[j+2] = get_sample (line->data, line->depth, i) * 0xFF / (1 << (line->depth - 1));
            j += 3;
        }
        break;
    }

    current_line = line->number + 1;

    ui_redraw_preview (ui);
}


static void
scanner_image_done_cb (Scanner *scanner)
{
    /* Trim image */
    if (raw_image->image && current_line != gdk_pixbuf_get_height (raw_image->image)) {
        GdkPixbuf *image;

        gint height, width, new_height;

        width = gdk_pixbuf_get_width (raw_image->image);
        height = gdk_pixbuf_get_height (raw_image->image);
        new_height = current_line;
        g_debug("Trimming image height from %d pixels to %d pixels", height, new_height);

        image = gdk_pixbuf_new (GDK_COLORSPACE_RGB, FALSE,
                                gdk_pixbuf_get_bits_per_sample (raw_image->image),
                                width, new_height);
        memcpy (gdk_pixbuf_get_pixels (image),
                gdk_pixbuf_get_pixels (raw_image->image),
                new_height * gdk_pixbuf_get_rowstride (raw_image->image));
        g_object_unref (raw_image->image);
        raw_image->image = image;
    }

    ui_redraw_preview (ui);
    ui_set_have_scan (ui, TRUE);
}


static void
scanner_failed_cb (Scanner *scanner, GError *error)
{
    ui_show_error (ui,
                   /* Title of error dialog when scan failed */
                   _("Failed to scan"),
                   error->message);
}


static void
render_scan (cairo_t *context, ScannedImage *image, Orientation orientation, double canvas_width, double canvas_height, gboolean show_scan_line)
{
    double inner_width, inner_height;
    double orig_img_width, orig_img_height, img_width, img_height;
    double source_aspect, inner_aspect;
    double x_offset = 0.0, y_offset = 0.0, scale = 1.0, rotation = 0.0;
    double left_border = 1.0, right_border = 1.0, top_border = 1.0, bottom_border = 1.0;

    orig_img_width = img_width = gdk_pixbuf_get_width (image->image);
    orig_img_height = img_height = gdk_pixbuf_get_height (image->image);

    switch (orientation) {
    case TOP_TO_BOTTOM:
        rotation = 0.0;
        break;
    case BOTTOM_TO_TOP:
        rotation = M_PI;
        break;
    case LEFT_TO_RIGHT:
        img_width = orig_img_height;
        img_height = orig_img_width;
        rotation = -M_PI_2;
        break;
    case RIGHT_TO_LEFT:
        img_width = orig_img_height;
        img_height = orig_img_width;
        rotation = M_PI_2;
        break;
    }
    
    /* Area available for page inside canvas */
    inner_width = canvas_width - left_border - right_border;
    inner_height = canvas_height - top_border - bottom_border;

    /* Scale if cannot fit into canvas */
    if (img_width > inner_width || img_height > inner_height) {
        inner_aspect = inner_width / inner_height;
        source_aspect = img_width / img_height;

        /* Scale to canvas height */
        if (inner_aspect > source_aspect) {
            scale = inner_height / img_height;
            x_offset = (int) (inner_width - (img_width * scale)) / 2;
        }
        /* Otherwise scale to canvas width */
        else {
            scale = inner_width / img_width;
            y_offset = (int) (inner_height - (img_height * scale)) / 2;
        }
    /* Otherwise just center */
    } else {
        if (inner_width > img_width)
            x_offset = (int) (inner_width - img_width) / 2;
        if (inner_height > img_height)
            y_offset = (int) (inner_height - img_height) / 2;
    }
    x_offset += left_border;
    y_offset += top_border;
    
    cairo_save (context);

    cairo_translate (context, x_offset, y_offset);
    cairo_scale (context, scale, scale);
    cairo_translate (context, img_width / 2, img_height / 2);
    cairo_rotate (context, rotation);
    cairo_translate (context, -orig_img_width / 2, -orig_img_height / 2);

    /* Render the image */
    gdk_cairo_set_source_pixbuf (context, image->image, 0, 0);
    cairo_pattern_set_filter (cairo_get_source (context), CAIRO_FILTER_BEST);
    cairo_paint (context);

    cairo_restore (context);

    /* Overlay transform */
    switch (orientation) {
    case TOP_TO_BOTTOM:
        cairo_translate (context, x_offset, y_offset);
        break;
    case BOTTOM_TO_TOP:
        // FIXME: Doesn't work if right/bottom border not the same as left/right
        cairo_translate (context, canvas_width - x_offset, canvas_height - y_offset);
        break;
    case LEFT_TO_RIGHT:
        // FIXME: Doesn't work if right/bottom border not the same as left/right
        cairo_translate (context, x_offset, canvas_height - y_offset);
        break;
    case RIGHT_TO_LEFT:
        // FIXME: Doesn't work if right/bottom border not the same as left/right
        cairo_translate (context, canvas_width - x_offset, y_offset);
        break;
    }
    cairo_rotate (context, rotation);
    
    /* Render page border */
    /* NOTE: Border width and height is rounded up so border is sharp.  Background may not
     * extend to border, should fill with white (?) before drawing scanned image or extend
     * edges slightly */
    cairo_set_source_rgb (context, 0, 0, 0);
    cairo_set_line_width (context, 1);
    cairo_rectangle (context, -0.5, -0.5,
                     (int) (scale * orig_img_width + 1 + 0.5),
                     (int) (scale * orig_img_height + 1 + 0.5));
    cairo_stroke (context);

    /* Render scan line */
    if (show_scan_line && scanning) {
        double h = scale * (double)(current_line * orig_img_height) / (double)img_height;

        cairo_set_source_rgb (context, 1.0, 0.0, 0.0);
        cairo_move_to (context, 0, h);
        cairo_line_to (context, scale * orig_img_width, h);
        cairo_stroke (context);
    }
}


static void
render_cb (SimpleScan *ui, cairo_t *context, double width, double height)
{
    render_scan (context, raw_image, ui_get_orientation (ui),
                 width, height, TRUE);
}


static void
scan_cb (SimpleScan *ui, const gchar *device, const gchar *document_type)
{
    PageMode page_mode;

    g_debug ("Requesting scan of type %s from device '%s'", document_type, device);

    scanning = TRUE;
    ui_set_have_scan (ui, FALSE);
    ui_set_scanning (ui, TRUE);
    ui_redraw_preview (ui);
    
    // FIXME: Translate
    if (strcmp (document_type, "photo") == 0) {
        ui_set_default_file_name (ui,
                                  /* Default name for JPEG documents */
                                  _("Scanned Document.jpeg"));
        raw_image->dpi = 400;
    }
    else if (strcmp (document_type, "document") == 0) {
        ui_set_default_file_name (ui,
                                  /* Default name for PDF documents */
                                  _("Scanned Document.pdf"));
        raw_image->dpi = 200;
    }
    else if (strcmp (document_type, "raw") == 0) {
        ui_set_default_file_name (ui,
                                  /* Default name for PNG documents */
                                  _("Scanned Document.png"));
        raw_image->dpi = 400;
    }
    /* Draft or unknown */
    else
    {
        ui_set_default_file_name (ui, _("Scanned Document.jpeg"));
        raw_image->dpi = 75;
    }

    page_mode = ui_get_page_mode (ui);

    /* Start again if not using multiple scans */
    if (page_mode != PAGE_MULTIPLE) {
        // Clear existing pages
        page_count = 0;
        ui_set_page_count (ui, 1);
        ui_set_selected_page (ui, 1);
    }

    scanner_scan (scanner, device, NULL, raw_image->dpi, NULL, 8, page_mode == PAGE_AUTOMATIC);
    //scanner_scan (scanner, device, "Flatbed", 50, "Color", 8, page_mode == PAGE_AUTOMATIC);
    //scanner_scan (scanner, device, "Automatic Document Feeder", 50, "Color", 8, page_mode == PAGE_AUTOMATIC);
}


static void
cancel_cb (SimpleScan *ui)
{
    scanner_cancel (scanner);
}


static gboolean
write_pixbuf_data (const gchar *buf, gsize count, GError **error, GFileOutputStream *stream)
{
    return g_output_stream_write_all (G_OUTPUT_STREAM (stream), buf, count, NULL, NULL, error);
}


static gboolean
save_jpeg (ScannedImage *image, GFileOutputStream *stream, GError **error)
{
    return gdk_pixbuf_save_to_callback (image->image,
                                        (GdkPixbufSaveFunc) write_pixbuf_data, stream,
                                        "jpeg", error,
                                        "quality", "90",
                                        NULL);
}


static gboolean
save_png (ScannedImage *image, GFileOutputStream *stream, GError **error)
{
    return gdk_pixbuf_save_to_callback (image->image,
                                        (GdkPixbufSaveFunc) write_pixbuf_data, stream,
                                        "png", error,
                                        NULL);
}

    
static cairo_status_t
write_cairo_data (GFileOutputStream *stream, unsigned char *data, unsigned int length)
{
    gboolean result;
    GError *error = NULL;

    result = g_output_stream_write_all (G_OUTPUT_STREAM (stream), data, length, NULL, NULL, &error);
    
    if (error) {
        g_warning ("Error writing data: %s", error->message);
        g_error_free (error);
    }

    return result ? CAIRO_STATUS_SUCCESS : CAIRO_STATUS_WRITE_ERROR;
}


static void
save_ps_pdf_surface (cairo_surface_t *surface, ScannedImage *image)
{
    cairo_t *context;
    
    context = cairo_create (surface);

    cairo_scale (context, 72.0 / image->dpi, 72.0 / image->dpi);
    gdk_cairo_set_source_pixbuf (context, image->image, 0, 0);
    cairo_pattern_set_filter (cairo_get_source (context), CAIRO_FILTER_BEST);
    cairo_paint (context);

    cairo_destroy (context);
}


static gboolean
save_pdf (ScannedImage *image, GFileOutputStream *stream, GError **error)
{
    cairo_surface_t *surface;
    double width, height;
    
    width = gdk_pixbuf_get_width (image->image) * 72.0 / image->dpi;
    height = gdk_pixbuf_get_height (image->image) * 72.0 / image->dpi;
    surface = cairo_pdf_surface_create_for_stream ((cairo_write_func_t) write_cairo_data,
                                                   stream,
                                                   width, height);
    save_ps_pdf_surface (surface, image);
    cairo_surface_destroy (surface);
    
    return TRUE;
}


static gboolean
save_ps (ScannedImage *image, GFileOutputStream *stream, GError **error)
{
    cairo_surface_t *surface;
    double width, height;
    
    width = gdk_pixbuf_get_width (image->image) * 72.0 / image->dpi;
    height = gdk_pixbuf_get_height (image->image) * 72.0 / image->dpi;
    surface = cairo_ps_surface_create_for_stream ((cairo_write_func_t) write_cairo_data,
                                                  stream,
                                                  width, height);
    save_ps_pdf_surface (surface, image);
    cairo_surface_destroy (surface);

    return TRUE;
}


static ScannedImage *get_rotated_image (Orientation orientation)
{
    ScannedImage *image;

    image = g_malloc0 (sizeof (ScannedImage));
    image->dpi = raw_image->dpi;
    switch (orientation) {
    default:
    case TOP_TO_BOTTOM:
        image->image = gdk_pixbuf_ref (raw_image->image);
        break;
    case BOTTOM_TO_TOP:
        image->image = gdk_pixbuf_rotate_simple (raw_image->image, GDK_PIXBUF_ROTATE_UPSIDEDOWN);
        break;
    case LEFT_TO_RIGHT:
        image->image = gdk_pixbuf_rotate_simple (raw_image->image, GDK_PIXBUF_ROTATE_COUNTERCLOCKWISE);
        break;
    case RIGHT_TO_LEFT:
        image->image = gdk_pixbuf_rotate_simple (raw_image->image, GDK_PIXBUF_ROTATE_CLOCKWISE);
        break;
    }
    return image;
}


static void
save_cb (SimpleScan *ui, gchar *uri)
{
    GFile *file;
    GError *error = NULL;
    GFileOutputStream *stream;

    file = g_file_new_for_uri (uri);

    stream = g_file_replace (file, NULL, FALSE, G_FILE_CREATE_NONE, NULL, &error);
    if (!stream) {
        g_warning ("Error saving file: %s", error->message);
        g_error_free (error);
    }
    else {
        gboolean result;
        gchar *uri_lower;
        ScannedImage *image;

        image = get_rotated_image (ui_get_orientation (ui));

        uri_lower = g_utf8_strdown (uri, -1);
        if (g_str_has_suffix (uri_lower, ".pdf"))
            result = save_pdf (image, stream, &error);
        else if (g_str_has_suffix (uri_lower, ".ps"))
            result = save_ps (image, stream, &error);
        else if (g_str_has_suffix (uri_lower, ".png"))
            result = save_png (image, stream, &error);
        else
            result = save_jpeg (image, stream, &error);

        g_free (uri_lower);           
        g_object_unref (image->image);
        g_free (image);

        if (error) {
            g_warning ("Error saving file: %s", error->message);
            ui_show_error (ui,
                           /* Title of error dialog when save failed */
                           _("Failed to save file"),
                           error->message);
            g_error_free (error);
        }

        g_output_stream_close (G_OUTPUT_STREAM (stream), NULL, NULL);
    }
}


static void
print_cb (SimpleScan *ui, cairo_t *context)
{
    ScannedImage *image;

    image = get_rotated_image (ui_get_orientation (ui));

    gdk_cairo_set_source_pixbuf (context, image->image, 0, 0);
    cairo_pattern_set_filter (cairo_get_source (context), CAIRO_FILTER_BEST);
    cairo_paint (context);

    g_object_unref (image->image);
    g_free (image);
}


static void
quit_cb (SimpleScan *ui)
{
    scanner_free (scanner);
    gtk_main_quit ();
}


static void
version()
{
    /* NOTE: Is not translated so can be easily parsed */
    fprintf(stderr, "%1$s %2$s\n", SIMPLE_SCAN_BINARY, VERSION);
}


static void
usage(int show_gtk)
{
    fprintf(stderr,
            /* Description on how to use simple-scan displayed on command-line */
            _("Usage:\n"
              "  %s [DEVICE...] - Scanning utility"), SIMPLE_SCAN_BINARY);

    fprintf(stderr,
            "\n\n");

    fprintf(stderr,
            /* Description on how to use simple-scan displayed on command-line */    
            _("Help Options:\n"
              "  -v, --version                   Show release version\n"
              "  -h, --help                      Show help options\n"
              "  --help-all                      Show all help options\n"
              "  --help-gtk                      Show GTK+ options"));
    fprintf(stderr,
            "\n\n");

    if (show_gtk) {
        fprintf(stderr,
                /* Description on simple-scan command-line GTK+ options displayed on command-line */
                _("GTK+ Options:\n"
                  "  --class=CLASS                   Program class as used by the window manager\n"
                  "  --name=NAME                     Program name as used by the window manager\n"
                  "  --screen=SCREEN                 X screen to use\n"
                  "  --sync                          Make X calls synchronous\n"
                  "  --gtk-module=MODULES            Load additional GTK+ modules\n"
                  "  --g-fatal-warnings              Make all warnings fatal"));
        fprintf(stderr,
                "\n\n");
    }
}


static void
get_options (int argc, char **argv)
{
    int i;

    for (i = 1; i < argc; i++) {
        char *arg = argv[i];

        if (strcmp (arg, "-v") == 0 ||
            strcmp (arg, "--version") == 0) {
            version ();
            exit (0);
        }
        else if (strcmp (arg, "-h") == 0 ||
                 strcmp (arg, "--help") == 0) {
            usage (FALSE);
            exit (0);
        }
        else if (strcmp (arg, "--help-all") == 0) {
            usage (TRUE);
            exit (0);
        }
        else {
            if (default_device) {
                fprintf (stderr, "Unknown argument: '%s'\n", arg);
                exit (1);
            }
            default_device = arg;
        }
    }
}


int
main(int argc, char **argv)
{
    g_thread_init (NULL);
    gtk_init (&argc, &argv);
    
    get_options (argc, argv);

    /* Start with A4 white image at 72dpi */
    /* TODO: Should be like the last scanned image for the selected scanner */
    raw_image = g_malloc0(sizeof(ScannedImage));
    raw_image->dpi = 72;
    raw_image->image = gdk_pixbuf_new (GDK_COLORSPACE_RGB, FALSE,
                                       8, 595, 842);
    memset (gdk_pixbuf_get_pixels (raw_image->image), 0xFF,
            gdk_pixbuf_get_height (raw_image->image) * gdk_pixbuf_get_rowstride (raw_image->image));

    ui = ui_new ();
    g_signal_connect (ui, "render-preview", G_CALLBACK (render_cb), NULL);
    g_signal_connect (ui, "start-scan", G_CALLBACK (scan_cb), NULL);
    g_signal_connect (ui, "stop-scan", G_CALLBACK (cancel_cb), NULL);
    g_signal_connect (ui, "save", G_CALLBACK (save_cb), NULL);
    g_signal_connect (ui, "print", G_CALLBACK (print_cb), NULL);
    g_signal_connect (ui, "quit", G_CALLBACK (quit_cb), NULL);
    
    ui_set_page_count (ui, 1);
    ui_set_selected_page (ui, 1);

    scanner = scanner_new ();
    g_signal_connect (G_OBJECT (scanner), "ready", G_CALLBACK (scanner_ready_cb), NULL);
    g_signal_connect (G_OBJECT (scanner), "update-devices", G_CALLBACK (update_scan_devices_cb), NULL);
    g_signal_connect (G_OBJECT (scanner), "got-page-info", G_CALLBACK (scanner_page_info_cb), NULL);
    g_signal_connect (G_OBJECT (scanner), "got-line", G_CALLBACK (scanner_line_cb), NULL);
    g_signal_connect (G_OBJECT (scanner), "image-done", G_CALLBACK (scanner_image_done_cb), NULL);
    g_signal_connect (G_OBJECT (scanner), "scan-failed", G_CALLBACK (scanner_failed_cb), NULL);

    if (default_device)
        ui_set_selected_device (ui, default_device);

    ui_start (ui);
    scanner_start (scanner);

    gtk_main ();

    return 0;
}
