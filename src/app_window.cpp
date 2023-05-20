/*
    Copyright (C) 2023  Tiernan8r
    Fork of origin simple-scan at https://gitlab.gnome.org/GNOME/simple-scan

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "app_window.h"
#include "page.h"
#include "book_view.h"
#include <gtkmm.h>
#include <list>
#include <libadwaitamm.h>

// [GtkTemplate (ui = "/me/tiernan8r/Receipts/ui/app-window.ui")]
void AppWindow::update_scan_status(void)
{
    scan_button.sensitive = false;
    if (!have_devices)
    {
        status_page.set_title(/* Label shown when searching for scanners */
                              _("Searching for Scanners…"));
        status_secondary_label.visible = false;
        device_buttons_box.visible = false;
    }
    else if (get_selected_device() != NULL)
    {
        scan_button.sensitive = true;
        status_page.set_title(/* Label shown when detected a scanner */
                              _("Ready to Scan"));
        status_secondary_label.set_text(get_selected_device_label());
        status_secondary_label.visible = false;
        device_buttons_box.visible = true;
        device_buttons_box.sensitive = true;
        device_combo.sensitive = true;
    }
    else if (this.missing_driver != NULL)
    {
        status_page.set_title(/* Warning displayed when no drivers are installed but a compatible scanner is detected */
                              _("Additional Software Needed"));
        /* Instructions to install driver software */
        status_secondary_label.set_markup(_("You need to <a href=\"install-firmware\">install driver software</a> for your scanner."));
        status_secondary_label.visible = true;
        device_buttons_box.visible = false;
    }
    else
    {
        /* Warning displayed when no scanners are detected */
        status_page.set_title(_("No Scanners Detected"));
        /* Hint to user on why there are no scanners detected */
        status_secondary_label.set_text(_("Please check your scanner is connected and powered on."));
        status_secondary_label.visible = true;
        device_buttons_box.visible = true;
        device_buttons_box.sensitive = true;
        device_combo.sensitive = false; // We would like to be refresh button to be active
    }
};

async bool AppWindow::prompt_to_load_autosaved_book(void)
{
    Adw::MessageDialog dialog = new Adw::MessageDialog(this,
                                                       "",
                                                       /* Contents of dialog that shows if autosaved book should be loaded. */
                                                       _("An autosaved book exists. Do you want to open it?"));

    dialog.add_response("no", _("_No"));
    dialog.add_response("yes", _("_Yes"));

    dialog.set_response_appearance("no", Adw.ResponseAppearance.DESTRUCTIVE);
    dialog.set_response_appearance("yes", Adw.ResponseAppearance.SUGGESTED);

    dialog.set_default_response("yes");
    dialog.show();

    std::string response = "yes";

    SourceFunc callback = prompt_to_load_autosaved_book.callback;
    dialog.response.connect((res) = > {
        response = res;
        callback();
    });

    yield;

    return response == "yes";
}

std::string *AppWindow::get_selected_device(void)
{
    Gtk::TreeIter iter;

    if (device_combo.get_active_iter(&iter))
    {
        std::string device;
        device_model.get(iter, 0, &device, -1);
        return device;
    }

    return NULL;
}

std::string *AppWindow::get_selected_device_label(void)
{
    Gtk::TreeIter iter;

    if (device_combo.get_active_iter(&iter))
    {
        std::string label;
        device_model.get(iter, 1, &label, -1);
        return label;
    }

    return NULL;
}

bool AppWindow::find_scan_device(std::string device, Gtk::TreeIter *iter)
{
    bool have_iter = false;

    if (device_model.get_iter_first(&iter))
    {
        do
        {
            std::string d;
            device_model.get(iter, 0, &d, -1);
            if (d == device)
                have_iter = true;
        } while (!have_iter && device_model.iter_next(&iter));
    }

    return have_iter;
}

async std::string *AppWindow::choose_file_location(void)
{
    /* Get directory to save to */
    std::string *directory = NULL;
    directory = settings.get_string("save-directory");

    if (directory == NULL || directory == "")
        directory = GLib::Filename.to_uri(Environment.get_user_special_dir(UserDirectory.DOCUMENTS));

    Gtk::FileChooserNative save_dialog = new Gtk::FileChooserNative(/* Save dialog: Dialog title */
                                                                    _("Save As…"),
                                                                    this,
                                                                    Gtk::FileChooserAction.SAVE,
                                                                    _("_Save"),
                                                                    _("_Cancel"));

    save_dialog.set_modal(true);
    // TODO(gtk4)
    //  save_dialog.local_only = false;

    auto save_format = settings.get_string("save-format");
    if (book_uri != NULL)
    {
        GLib::File file = GLib::File::new_for_uri(book_uri);

        try
        {
            save_dialog.set_file(file);
        }
        catch (Error e)
        {
            spdlog::warn("Error file chooser set_file: %s", e.message);
        }
    }
    else
    {
        Glib::File file = GLib::File::new_for_uri(directory);

        try
        {
            save_dialog.set_current_folder(file);
        }
        catch (Error e)
        {
            spdlog::warn("Error file chooser set_current_folder: %s", e.message);
        }

        /* Default filename to use when saving document. */
        /* To that filename the extension will be added, eg. "Scanned Document.pdf" */
        save_dialog.set_current_name(_("Scanned Document") + "." + mime_type_to_extension(save_format));
    }

    Gtk::FileFilter pdf_filter = new Gtk::FileFilter();
    pdf_filter.set_filter_name(_("PDF (multi-page document)"));
    pdf_filter.add_pattern("*.pdf");
    pdf_filter.add_mime_type("application/pdf");
    save_dialog.add_filter(pdf_filter);

    Gtk::FileFilter jpeg_filter = new Gtk::FileFilter();
    jpeg_filter.set_filter_name(_("JPEG (compressed)"));
    jpeg_filter.add_pattern("*.jpg");
    jpeg_filter.add_pattern("*.jpeg");
    jpeg_filter.add_mime_type("image/jpeg");
    save_dialog.add_filter(jpeg_filter);

    Gtk::FileFilter png_filter = new Gtk::FileFilter();
    png_filter.set_filter_name(_("PNG (lossless)"));
    png_filter.add_pattern("*.png");
    png_filter.add_mime_type("image/png");
    save_dialog.add_filter(png_filter);

    Gtk::FileFilter webp_filter = new Gtk::FileFilter();
    webp_filter.set_filter_name(_("WebP (compressed)"));
    webp_filter.add_pattern("*.webp");
    webp_filter.add_mime_type("image/webp");
    save_dialog.add_filter(webp_filter);

    Gtk::FileFilter all_filter = new Gtk::FileFilter();
    all_filter.set_filter_name(_("All Files"));
    all_filter.add_pattern("*");
    save_dialog.add_filter(all_filter);

    switch (save_format)
    {
    case "application/pdf":
        save_dialog.set_filter(pdf_filter);
        break;
    case "image/jpeg":
        save_dialog.set_filter(jpeg_filter);
        break;
    case "image/png":
        save_dialog.set_filter(png_filter);
        break;
    case "image/webp":
        save_dialog.set_filter(webp_filter);
        break;
    }

    Gtk::Box box = new Gtk::Box(Gtk::Orientation.HORIZONTAL, 6);
    box.visible = true;
    box.spacing = 10;
    box.set_halign(Gtk::Align::CENTER);

    while (true)
    {
        save_dialog.show();

        Gtk::ResponseType response = Gtk::ResponseType.NONE;
        SourceFunc callback = choose_file_location.callback;

        save_dialog.response.connect((res) = > {
            response = (Gtk::ResponseType)res;
            callback();
        });

        yield;

        if (response != Gtk::ResponseType.ACCEPT)
        {
            save_dialog.destroy();
            return NULL;
        }

        GLib::File file = save_dialog.get_file();

        if (file == NULL)
        {
            return NULL;
        }

        auto uri = file.get_uri();

        auto extension = uri_extension(uri);

        auto mime_type = extension_to_mime_type(extension);
        mime_type = mime_type != NULL ? mime_type : "application/pdf";

        settings.set_string("save-format", mime_type);

        if (extension == NULL)
            uri += "." + mime_type_to_extension(mime_type);

        /* Check the file(s) don't already exist */
        auto files = new std::list<File>();
        if (mime_type == "image/jpeg" || mime_type == "image/png" || mime_type == "image/webp")
        {
            for (var j = 0; j < book.n_pages; j++)
                files.append(make_indexed_file(uri, j, book.n_pages));
        }
        else
            files.append(File.new_for_uri(uri));

        bool overwrite_check = true;

        // We assume that GTK or system file dialog asked about overwrite already so we reask only if there is more than one file or we changed the name
        // Ideally in flatpack era we should not modify file name after save dialog is done
        // but for the sake of keeping old functionality in tact we leave it as it
        if (files.length() > 1 || file.get_uri() != uri)
        {
            overwrite_check = yield check_overwrite(save_dialog.transient_for, files);
        }

        if (overwrite_check)
        {
            std::string directory_uri = uri.substring(0, uri.last_index_of("/") + 1);
            settings.set_string("save-directory", directory_uri);
            save_dialog.destroy();
            return uri;
        }
    }
}

async bool AppWindow::check_overwrite(Gtk::Window parent, std::list<File> files)
{
    foreach (File file in files)
    {
        if (!file.query_exists())
            continue;

        std::string title = _("A file named “%s” already exists.  Do you want to replace it?").printf(file.get_basename());

        Adw::MessageDialog dialog = new Adw::MessageDialog(parent,
                                                           /* Contents of dialog that shows if saving would overwrite and existing file. %s is replaced with the name of the file. */
                                                           title,
                                                           NULL);

        dialog.add_response("cancel", _("_Cancel"));
        dialog.add_response("replace", _("_Replace"));

        dialog.set_response_appearance("replace", Adw.ResponseAppearance.DESTRUCTIVE);

        SourceFunc callback = check_overwrite.callback;
        std::string response = "cancel";

        dialog.response.connect((res) = > {
            response = res;
            callback();
        });

        dialog.show();

        yield;

        if (response != "replace")
            return false;
    }

    return true;
}

std::string *AppWindow::mime_type_to_extension(std::string mime_type)
{
    if (mime_type == "application/pdf")
        return "pdf";
    else if (mime_type == "image/jpeg")
        return "jpg";
    else if (mime_type == "image/png")
        return "png";
    else if (mime_type == "image/webp")
        return "webp";
    else
        return NULL;
};

std::string *AppWindow::extension_to_mime_type(std::string extension)
{
    std::string extension_lower = extension.down();
    if (extension_lower == "pdf")
        return "application/pdf";
    else if (extension_lower == "jpg" || extension_lower == "jpeg")
        return "image/jpeg";
    else if (extension_lower == "png")
        return "image/png";
    else if (extension_lower == "webp")
        return "image/webp";
    else
        return NULL;
};

std::string *AppWindow::uri_extension(std::string uri)
{
    int extension_index = uri.last_index_of_char('.');
    if (extension_index < 0)
        return NULL;

    return uri.substring(extension_index + 1);
};

std::string AppWindow::uri_to_mime_type(std::string uri)
{
    std::string extension = uri_extension(uri);
    if (extension == NULL)
        return "image/jpeg";

    std::string mime_type = extension_to_mime_type(extension);
    if (mime_type == NULL)
        return "image/jpeg";

    return mime_type;
};

async bool AppWindow::save_document_async(void)
{
    std::future<std::string> fut = std::async(choose_file_location);
    std::string uri = fut.get();
    if (uri == NULL)
        return false;

    File file = File::new_for_uri(uri);

    spdlog::debug("Saving to '%s'", uri);

    std::string mime_type = uri_to_mime_type(uri);

    Cancellable cancellable = new Cancellable();
    CancellableProgressBar progress_bar = new CancellableProgressBar(_("Saving"), cancellable);
    action_bar.pack_end(progress_bar);
    progress_bar.visible = true;
    save_button.sensitive = false;
    try
    {
        std::future<> async_save = std::async(
            book.save_async, mime_type, settings.get_int("jpeg-quality"), file,
            settings.get_boolean("postproc-enabled"), settings.get_string("postproc-script"),
            settings.get_string("postproc-arguments"), settings.get_boolean("postproc-keep-original"),
            (fraction) = >
                         {
                             progress_bar.set_fraction(fraction);
                         },
            cancellable);
        async_save.wait();
        // yield book.save_async(
        //     mime_type, settings.get_int("jpeg-quality"), file,
        //     settings.get_boolean("postproc-enabled"), settings.get_string("postproc-script"),
        //     settings.get_string("postproc-arguments"), settings.get_boolean("postproc-keep-original"),
        //     (fraction) = >
        //                  {
        //                      progress_bar.set_fraction(fraction);
        //                  },
        //     cancellable);
    }
    catch (Error e)
    {
        save_button.sensitive = true;
        progress_bar.destroy();
        spdlog::warn("Error saving file: %s", e.message);
        show_error_dialog(/* Title of error dialog when save failed */
                          _("Failed to save file"),
                          e.message);
        return false;
    }
    save_button.sensitive = true;
    progress_bar.remove_with_delay(500, action_bar);

    book_needs_saving = false;
    book_uri = uri;
    return true;
}

async bool AppWindow::prompt_to_save_async(std::string title, std::string discard_label)
{
    if (!book_needs_saving || (book.n_pages == 0))
        return true;

    Adw::MessageDialog dialog = new Adw::MessageDialog(this,
                                                       title,
                                                       _("If you don’t save, changes will be permanently lost."));

    dialog.add_response("discard", discard_label);
    dialog.add_response("cancel", _("_Cancel"));
    dialog.add_response("save", _("_Save"));

    dialog.set_response_appearance("discard", Adw.ResponseAppearance.DESTRUCTIVE);
    dialog.set_response_appearance("save", Adw.ResponseAppearance.SUGGESTED);

    dialog.show();

    std::string response = "cancel";
    SourceFunc callback = prompt_to_save_async.callback;
    dialog.response.connect((res) = > {
        response = res;
        callback();
    });

    yield;

    switch (response)
    {
    case "save":
        if (yield save_document_async())
            return true;
        else
            return false;
    case "discard":
        return true;
    default:
        return false;
    }
};

void AppWindow::clear_document(void)
{
    book.clear();
    book_needs_saving = false;
    book_uri = NULL;
    save_button.sensitive = false;
    copy_to_clipboard_action.set_enabled(false);
    update_scan_status();
    stack.set_visible_child_name("startup");
};

void AppWindow::new_document(void)
{
    prompt_to_save_async.begin(/* Text in dialog warning when a document is about to be lost */
                               _("Save current document?"),
                               /* Button in dialog to create new document and discard unsaved document */
                               _("_Discard Changes"), (obj, res) = > {
                                   if (!prompt_to_save_async.end(res))
                                       return;

                                   if (scanning)
                                       stop_scan();

                                   clear_document();
                                   autosave_manager.cleanup();
                               });
};

// [GtkCallback]
bool AppWindow::status_label_activate_link_cb(Gtk::Label label, std::string uri)
{
    if (uri == "install-firmware")
    {
        DriversDialog dialog = new DriversDialog(this, missing_driver);
        dialog.open.begin(() = > {});
        return true;
    }

    return false;
}

// [GtkCallback]
void AppWindow::new_document_cb(void)
{
    new_document();
}

// [GtkCallback]
void AppWindow::crop_toggle_cb(Gtk::ToggleButton btn)
{
    if (updating_page_menu)
        return;

    Page page = book_view.selected_page;
    if (page == NULL)
    {
        spdlog::warn("Trying to set crop but no selected page");
        return;
    }

    if (btn.active)
    {
        // Avoid overwriting crop name if there is already different crop active
        if (!page.has_crop)
            set_crop("custom");
    }
    else
    {
        set_crop(NULL);
    }
};

// [GtkCallback]
void AppWindow::redetect_button_clicked_cb(Gtk::Button button)
{
    have_devices = false;
    update_scan_status();
    redetect();
};

void AppWindow::scan(ScanOptions options)
{
    status_page.set_title(/* Label shown when scan started */
                          _("Contacting Scanner…"));
    device_buttons_box.visible = true;
    device_buttons_box.sensitive = false;
    start_scan(get_selected_device(), options);
};

void AppWindow::scan_type_action_cb(SimpleAction action, Variant *value)
{
    std::string type = value.get_string();

    switch (type)
    {
    case "single":
        set_scan_type(ScanType.SINGLE);
        break;
    case "adf":
        set_scan_type(ScanType.ADF);
        break;
    case "batch":
        set_scan_type(ScanType.BATCH);
        break;
    default:
        return;
    }
}

void AppWindow::document_hint_action_cb(SimpleAction action, Variant *value)
{
    std::string hint = value.get_string();
    set_document_hint(hint, true);
}

void AppWindow::scan_single_cb(void)
{
    ScanOptions options = make_scan_options();
    options.type = ScanType.SINGLE;
    scan(options);
}

void AppWindow::scan_adf_cb(void)
{
    ScanOptions options = make_scan_options();
    options.type = ScanType.ADF;
    scan(options);
}

void AppWindow::scan_batch_cb(void)
{
    ScanOptions options = make_scan_options();
    options.type = ScanType.BATCH;
    scan(options);
}

void AppWindow::scan_stop_cb(void)
{
    stop_scan();
}

void AppWindow::rotate_left_cb(void)
{
    if (updating_page_menu)
        return;
    Page page = book_view.selected_page;
    if (page != NULL)
        page.rotate_left();
};

void AppWindow::rotate_right_cb(void)
{
    if (updating_page_menu)
        return;
    Page page = book_view.selected_page;
    if (page != NULL)
        page.rotate_right();
};

void AppWindow::move_left_cb(void)
{
    Page page = book_view.selected_page;
    int index = book.get_page_index(page);
    if (index > 0)
        book.move_page(page, index - 1);
};

void AppWindow::move_right_cb(void)
{
    Page page = book_view.selected_page;
    int index = book.get_page_index(page);
    if (index < book.n_pages - 1)
        book.move_page(page, book.get_page_index(page) + 1);
};

void AppWindow::copy_page_cb(void)
{
    Page page = book_view.selected_page;
    if (page != NULL)
        page.copy_to_clipboard(this);
};

void AppWindow::delete_page_cb(void)
{
    book_view.book.delete_page(book_view.selected_page);
};

void AppWindow::set_scan_type(ScanType scan_type)
{
    this.scan_type = scan_type;

    switch (scan_type)
    {
    case ScanType.SINGLE:
        scan_type_action.set_state("single");
        scan_button_content.icon_name = "scanner-symbolic";
        scan_button.tooltip_text = _("Scan a single page from the scanner");
        break;
    case ScanType.ADF:
        scan_type_action.set_state("adf");
        scan_button_content.icon_name = "scan-type-adf-symbolic";
        scan_button.tooltip_text = _("Scan multiple pages from the scanner");
        break;
    case ScanType.BATCH:
        scan_type_action.set_state("batch");
        scan_button_content.icon_name = "scan-type-batch-symbolic";
        scan_button.tooltip_text = _("Scan multiple pages from the scanner");
        break;
    }
};

void AppWindow::set_document_hint(std::string document_hint, bool save = false)
{
    this.document_hint = document_hint;

    document_hint_action.set_state(document_hint);

    if (save)
        settings.set_string("document-type", document_hint);
}

ScanOptions AppWindow::make_scan_options(void)
{
    ScanOptions options = new ScanOptions();
    if (document_hint == "text")
    {
        options.scan_mode = ScanMode.GRAY;
        options.dpi = preferences_dialog.get_text_dpi();
        options.depth = 2;
    }
    else
    {
        options.scan_mode = ScanMode.COLOR;
        options.dpi = preferences_dialog.get_photo_dpi();
        options.depth = 8;
    }
    preferences_dialog.get_paper_size(&options.paper_width, &options.paper_height);
    options.brightness = brightness;
    options.contrast = contrast;
    options.page_delay = page_delay;
    options.side = preferences_dialog.get_page_side();

    return options;
}

// [GtkCallback]
void AppWindow::device_combo_changed_cb(Gtk::Widget widget)
{
    if (setting_devices)
        return;
    user_selected_device = true;
    if (get_selected_device() != NULL)
        settings.set_string("selected-device", get_selected_device());
}

// [GtkCallback]
void AppWindow::scan_button_clicked_cb(Gtk::Widget widget)
{
    scan_button.visible = false;
    stop_button.visible = true;
    var options = make_scan_options();
    options.type = scan_type;
    scan(options);
}

// [GtkCallback]
void AppWindow::stop_scan_button_clicked_cb(Gtk::Widget widget)
{
    scan_button.visible = true;
    stop_button.visible = false;
    stop_scan();
}

void AppWindow::preferences_cb(void)
{
    preferences_dialog.present();
};

void AppWindow::update_page_menu(void)
{
    Page page = book_view.selected_page;
    if (page == NULL)
    {
        page_move_left_action.set_enabled(false);
        page_move_right_action.set_enabled(false);
    }
    else
    {
        var index = book.get_page_index(page);
        page_move_left_action.set_enabled(index > 0);
        page_move_right_action.set_enabled(index < book.n_pages - 1);
    }
};

void AppWindow::page_selected_cb(BookView view, Page *page)
{
    if (page == NULL)
        return;

    updating_page_menu = true;

    update_page_menu();

    crop_actions.update_current_crop(page.crop_name);
    crop_button.active = page.has_crop;

    updating_page_menu = false;
};

void AppWindow::show_page_cb(BookView view, Page page)
{
    File file;
    try
    {
        auto dir = DirUtils.make_tmp("receipts-XXXXXX");
        file = File.new_for_path(Path.build_filename(dir, "scan.png"));
        page.save_png(file);
    }
    catch (Error e)
    {
        show_error_dialog(/* Error message display when unable to save image for preview */
                          _("Unable to save image for preview"),
                          e.message);
        return;
    }

    Gtk::show_uri(this, file.get_uri(), Gdk.CURRENT_TIME);
};

void AppWindow::show_page_menu_cb(BookView view, Gtk::Widget from, double x, double y)
{
    double tx, ty;
    from.translate_coordinates(this, x, y, &tx, &ty);

    Gdk::Rectangle rect = {x : (int)tx, y : (int)ty, w : 1, h : 1};

    page_menu.set_pointing_to(rect);
    page_menu.popup();
};

void AppWindow::set_crop(std::string *crop_name)
{
    if (updating_page_menu)
        return;

    if (crop_name == "none")
        crop_name = NULL;

    Page page = book_view.selected_page;
    if (page == NULL)
    {
        spdlog::warn("Trying to set crop but no selected page");
        return;
    }

    if (crop_name == NULL)
        page.set_no_crop();
    else if (crop_name == "custom")
    {
        int width = page.width;
        int height = page.height;
        int crop_width = (int)(width * 0.8 + 0.5);
        int crop_height = (int)(height * 0.8 + 0.5);
        page.set_custom_crop(crop_width, crop_height);
        page.move_crop((width - crop_width) / 2, (height - crop_height) / 2);
    }
    else
        page.set_named_crop(crop_name);

    crop_actions.update_current_crop(crop_name);
    crop_button.active = page.has_crop;
};

void AppWindow::draw_page(Gtk::PrintOperation operation,
                          Gtk::PrintContext print_context,
                          int page_number)
{
    var context = print_context.get_cairo_context();
    Page page = book.get_page(page_number);

    /* Rotate to same aspect */
    bool is_landscape = false;
    if (print_context.get_width() > print_context.get_height())
        is_landscape = true;
    if (page.is_landscape != is_landscape)
    {
        context.translate(print_context.get_width(), 0);
        context.rotate(Math.PI_2);
    }

    context.scale(print_context.get_dpi_x() / page.dpi,
                  print_context.get_dpi_y() / page.dpi);

    var image = page.get_image(true);
    Gdk::cairo_set_source_pixbuf(context, image, 0, 0);
    context.paint();
};

void AppWindow::email_document_cb(void)
{
    email_document_async.begin();
};

async void AppWindow::email_document_async(void)
{
    try
    {
        // TODO: Fix
        var dir = DirUtils.make_tmp("receipts-XXXXXX");
        std::string mime_type, filename;
        if (document_hint == "text")
        {
            mime_type = "application/pdf";
            filename = "scan.pdf";
        }
        else
        {
            mime_type = "image/jpeg";
            filename = "scan.jpg";
        }
        File file = File.new_for_path(Path.build_filename(dir, filename));
        std::future<> fut = std::async(book.save_async, settings.get_int("jpeg-quality"), file,
                                       settings.get_boolean("postproc-enabled"), settings.get_string("postproc-script"),
                                       settings.get_string("postproc-arguments"), settings.get_boolean("postproc-keep-original"),
                                       NULL, NULL);
        fut.wait();
        // yield book.save_async(mime_type, settings.get_int("jpeg-quality"), file,
        //                       settings.get_boolean("postproc-enabled"), settings.get_string("postproc-script"),
        //                       settings.get_string("postproc-arguments"), settings.get_boolean("postproc-keep-original"),
        //                       NULL, NULL);
        std::string command_line = "xdg-email";
        if (mime_type == "application/pdf")
            command_line += " --attach %s".printf(file.get_path());
        else
        {
            for (int i = 0; i < book.n_pages; i++)
            {
                File indexed_file = make_indexed_file(file.get_uri(), i, book.n_pages);
                command_line += " --attach %s".printf(indexed_file.get_path());
            }
        }
        Process.spawn_command_line_async(command_line);
    }
    catch (Error e)
    {
        spdlog::warn("Unable to email document: %s", e.message);
    }
};

void AppWindow::print_document(void)
{
    Gtk::PrintOperation print = new Gtk::PrintOperation();
    print.n_pages = (int)book.n_pages;
    print.draw_page.connect(draw_page);

    try
    {
        print.run(Gtk::PrintOperationAction.PRINT_DIALOG, this);
    }
    catch (Error e)
    {
        spdlog::warn("Error printing: %s", e.message);
    }

    print.draw_page.disconnect(draw_page);
};

void AppWindow::print_document_cb(void)
{
    print_document();
};

void AppWindow::launch_help(void)
{
    Gtk::show_uri(this, "help:receipts", Gdk.CURRENT_TIME);
};

void AppWindow::help_cb(void)
{
    launch_help();
};

void AppWindow::show_about(void)
{
    std::string[] authors = {"Robert Ancell <robert.ancell@canonical.com>", "Tiernan8r <tiernan8r@pm.me>"};

    Adw::AboutWindow about = new Adw::AboutWindow(){
        transient_for = this,
        developers = authors,
        translator_credits = _("translator-credits"),
        copyright = "Copyright © 2009-2018 Canonical Ltd; 2023 Tiernan8r",
        license_type = Gtk::License.GPL_3_0,
        application_name = _("Receipts Scanner"),
        application_icon = "me.tiernan8r.Receipts",
        version = VERSION,
        website = "https://github.com/tiernan8r/receipts",
        issue_url = "https://github.com/Tiernan8r/receipts/issues/new",
    };

    about.present();
};

void AppWindow::about_cb(void)
{
    show_about();
};

void AppWindow::on_quit(void)
{
    // TODO: Fix
    prompt_to_save_async.begin(/* Text in dialog warning when a document is about to be lost */
                               _("Save document before quitting?"),
                               /* Text in dialog warning when a document is about to be lost */
                               _("_Quit without Saving"), (obj, res) = > {
                                   if (!prompt_to_save_async.end(res))
                                       return;

                                   destroy();

                                   if (save_state_timeout != 0)
                                       save_state(true);

                                   autosave_manager.cleanup();
                               });
}

void AppWindow::quit_cb(void)
{
    on_quit();
};

void AppWindow::reorder_document_cb(void)
{
    ReorderPagesDialog dialog = new ReorderPagesDialog();
    dialog.set_transient_for(this);

    /* Button for combining sides in reordering dialog */
    dialog.combine_sides.clicked.connect(() = >
                                              {
                                                  book.combine_sides();
                                                  dialog.close();
                                              });

    /* Button for combining sides in reverse order in reordering dialog */
    dialog.combine_sides_rev.clicked.connect(() = >
                                                  {
                                                      book.combine_sides_reverse();
                                                      dialog.close();
                                                  });

    /* Button for reversing in reordering dialog */
    dialog.reverse.clicked.connect(() = >
                                        {
                                            book.reverse();
                                            dialog.close();
                                        });

    /* Button for keeping the ordering, but flip every second upside down */
    dialog.flip_odd.clicked.connect(() = >
                                         {
                                             book.flip_every_second(FlipEverySecond.Odd);
                                             dialog.close();
                                         });

    /* Button for keeping the ordering, but flip every second upside down */
    dialog.flip_even.clicked.connect(() = >
                                          {
                                              dialog.close();
                                              book.flip_every_second(FlipEverySecond.Even);
                                          });

    dialog.present();
};

// [GtkCallback]
bool AppWindow::window_close_request_cb(Gtk::Window window)
{
    on_quit();
    return true; /* Let us quit on our own terms */
}

void AppWindow::page_added_cb(Book book, Page page)
{
    update_page_menu();
};

void AppWindow::reordered_cb(Book book)
{
    update_page_menu();
};

void AppWindow::page_removed_cb(Book book, Page page)
{
    update_page_menu();
};

void AppWindow::book_changed_cb(Book book)
{
    save_button.sensitive = true;
    book_needs_saving = true;
    copy_to_clipboard_action.set_enabled(true);
};

void AppWindow::load(void)
{
    preferences_dialog = new PreferencesDialog(settings);
    preferences_dialog.close_request.connect(() = > {
        preferences_dialog.visible = false;
        return true;
    });
    preferences_dialog.transient_for = this;
    preferences_dialog.modal = true;

    Gtk::Window.set_default_icon_name("me.tiernan8r.Receipts");

    Gtk::Application app = (Gtk::Application)Application.get_default();

    crop_actions = new CropActions(this);

    app.add_action_entries(action_entries, this);

    scan_type_action = (GLib::SimpleAction)app.lookup_action("scan_type");
    document_hint_action = (GLib::SimpleAction)app.lookup_action("document_hint");

    delete_page_action = (GLib::SimpleAction)app.lookup_action("delete_page");
    page_move_left_action = (GLib::SimpleAction)app.lookup_action("move_left");
    page_move_right_action = (GLib::SimpleAction)app.lookup_action("move_right");
    copy_to_clipboard_action = (GLib::SimpleAction)app.lookup_action("copy_page");

    app.set_accels_for_action("app.new_document", {"<Ctrl>N"});
    app.set_accels_for_action("app.scan_single", {"<Ctrl>1"});
    app.set_accels_for_action("app.scan_adf", {"<Ctrl>F"});
    app.set_accels_for_action("app.scan_batch", {"<Ctrl>M"});
    app.set_accels_for_action("app.scan_stop", {"Escape"});
    app.set_accels_for_action("app.rotate_left", {"bracketleft"});
    app.set_accels_for_action("app.rotate_right", {"bracketright"});
    app.set_accels_for_action("app.move_left", {"less"});
    app.set_accels_for_action("app.move_right", {"greater"});
    app.set_accels_for_action("app.copy_page", {"<Ctrl>C"});
    app.set_accels_for_action("app.delete_page", {"Delete"});
    app.set_accels_for_action("app.save", {"<Ctrl>S"});
    app.set_accels_for_action("app.email", {"<Ctrl>E"});
    app.set_accels_for_action("app.print", {"<Ctrl>P"});
    app.set_accels_for_action("app.help", {"F1"});
    app.set_accels_for_action("app.quit", {"<Ctrl>Q"});
    app.set_accels_for_action("app.preferences", {"<Ctrl>comma"});
    app.set_accels_for_action("win.show-help-overlay", {"<Ctrl>question"});

    Menu gear_menu = new Menu();
    Menu section = new Menu();
    gear_menu.append_section(NULL, section);
    section.append(_("_Email"), "app.email");
    section.append(_("Pri_nt"), "app.print");
    section.append(C_("menu", "_Reorder Pages"), "app.reorder");
    section = new Menu();
    gear_menu.append_section(NULL, section);
    section.append(_("_Preferences"), "app.preferences");
    section.append(_("_Keyboard Shortcuts"), "win.show-help-overlay");
    section.append(_("_Help"), "app.help");
    section.append(_("_About Document Scanner"), "app.about");
    menu_button.set_menu_model(gear_menu);

    app.add_window(this);

    std::string document_type = settings.get_string("document-type");
    if (document_type != NULL)
        set_document_hint(document_type);

    book_view = new BookView(book);
    book_view.vexpand = true;

    main_vbox.prepend(book_view);

    book_view.page_selected.connect(page_selected_cb);
    book_view.show_page.connect(show_page_cb);
    book_view.show_menu.connect(show_page_menu_cb);
    book_view.visible = true;

    preferences_dialog.transient_for = this;

    /* Load previous state */
    load_state();

    /* Restore window size */
    debug("Restoring window to %dx%d pixels", window_width, window_height);
    set_default_size(window_width, window_height);
    if (window_is_maximized)
    {
        debug("Restoring window to maximized");
        maximize();
    }
    if (window_is_fullscreen)
    {
        debug("Restoring window to fullscreen");
        fullscreen();
    }
};

std::string AppWindow::state_filename
{
    owned get { return Path.build_filename(Environment.get_user_config_dir(), "receipts", "state"); }
}

void AppWindow::load_state(void)
{
    spdlog::debug("Loading state from %s", state_filename);

    KeyFile f = new KeyFile();
    try
    {
        f.load_from_file(state_filename, KeyFileFlags.NONE);
    }
    catch (Error e)
    {
        if (!(e is FileError.NOENT))
            spdlog::warn("Failed to load state: %s", e.message);
    }
    window_width = state_get_integer(f, "window", "width", 600);
    if (window_width <= 0)
        window_width = 600;
    window_height = state_get_integer(f, "window", "height", 400);
    if (window_height <= 0)
        window_height = 400;
    window_is_maximized = state_get_boolean(f, "window", "is-maximized");
    window_is_fullscreen = state_get_boolean(f, "window", "is-fullscreen");
    scan_type = Scanner.type_from_string(state_get_string(f, "scanner", "scan-type"));
    set_scan_type(scan_type);
};

std::string AppWindow::state_get_string(KeyFile f, std::string group_name, std::string key, std::string default = "")
{
    try
    {
        return f.get_string(group_name, key);
    }
    catch
    {
        return default;
    }
};

int AppWindow::state_get_integer(KeyFile f, std::string group_name, std::string key, int default = 0)
{
    try
    {
        return f.get_integer(group_name, key);
    }
    catch
    {
        return default;
    }
};

bool AppWindow::state_get_boolean(KeyFile f, std::string group_name, std::string key, bool default = false)
{
    try
    {
        return f.get_boolean(group_name, key);
    }
    catch
    {
        return default;
    }
};

// TODO: belongs in .h?
//  std::string AppWindow::STATE_DIR = Path.build_filename(Environment.get_user_config_dir(), "receipts", NULL);
void AppWindow::save_state(bool force = false)
{
    if (!force)
    {
        if (save_state_timeout != 0)
            Source.remove(save_state_timeout);
        save_state_timeout = Timeout.add(
            100, () = >
                      {
                          save_state(true);
                          save_state_timeout = 0;
                          return false;
                      });
        return;
    }

    spdlog::debug("Saving state to %s", state_filename);

    KeyFile f = new KeyFile();
    f.set_integer("window", "width", window_width);
    f.set_integer("window", "height", window_height);
    f.set_boolean("window", "is-maximized", window_is_maximized);
    f.set_boolean("window", "is-fullscreen", window_is_fullscreen);
    f.set_string("scanner", "scan-type", Scanner.type_to_string(scan_type));
    try
    {
        DirUtils.create_with_parents(STATE_DIR, 0700);
        FileUtils.set_contents(state_filename, f.to_data());
    }
    catch (Error e)
    {
        spdlog::warn("Failed to write state: %s", e.message);
    }
};

Page AppWindow::selected_page
{
    get
    {
        return book_view.selected_page;
    }
    set
    {
        book_view.selected_page = value;
    }
}

bool AppWindow::scanning
{
    get { return scanning_; }
    set
    {
        scanning_ = value;
        stack.set_visible_child_name("document");

        delete_page_action.set_enabled(!value);
        scan_button.visible = !value;
        stop_button.visible = value;
    }
}

int AppWindow::brightness
{
    get { return preferences_dialog.get_brightness(); }
    set { preferences_dialog.set_brightness(value); }
}

int AppWindow::contrast
{
    get { return preferences_dialog.get_contrast(); }
    set { preferences_dialog.set_contrast(value); }
}

int AppWindow::page_delay
{
    get { return preferences_dialog.get_page_delay(); }
    set { preferences_dialog.set_page_delay(value); }
}

AppWindow::type_signal_start_scan AppWindow::signal_start_scan() {
    return m_signal_start_scan;
}

AppWindow::type_signal_stop_scan AppWindow::signal_stop_scan() {
    return m_signal_stop_scan;
}

AppWindow::type_signal_redect AppWindow::signal_redect() {
    return m_signal_redect;
}

AppWindow::AppWindow(void)
{
    settings = new Settings("me.tiernan8r.Receipts");

    Gtk::CellRendererText renderer = new Gtk::CellRendererText();
    renderer.set_property("xalign", 0.5);
    device_combo.pack_start(renderer, true);
    device_combo.add_attribute(renderer, "text", 1);

    book = new Book();
    book.page_added.connect(page_added_cb);
    book.reordered.connect(reordered_cb);
    book.page_removed.connect(page_removed_cb);
    book.changed.connect(book_changed_cb);

    load();

    clear_document();
};

AppWindow::~AppWindow()
{
    book.page_added.disconnect(page_added_cb);
    book.reordered.disconnect(reordered_cb);
    book.page_removed.disconnect(page_removed_cb);
}

void AppWindow::show_error_dialog(std::string error_title, std::string error_text)
{
    Adw::MessageDialog dialog = new Adw::MessageDialog(this,
                                                       error_title,
                                                       error_text);
    dialog.add_response("close", _("_Close"));
    dialog.set_response_appearance("close", Adw.ResponseAppearance.SUGGESTED);
    dialog.show();
}

async AuthorizeDialogResponse AppWindow::authorize(std::string resource)
{
    /* Label in authorization dialog.  “%s” is replaced with the name of the resource requesting authorization */
    std::string description = _("Username and password required to access “%s”").printf(resource);
    AuthoriseDialog authorize_dialog = new AuthoriseDialog(this, description);
    authorize_dialog.transient_for = this;

    return yield authorize_dialog.open();
}

void AppWindow::set_scan_devices(std::list<ScanDevice> devices, std::string *missing_driver = NULL)
{
    have_devices = true;
    this.missing_driver = missing_driver;

    setting_devices = true;

    /* If the user hasn't chosen a scanner choose the best available one */
    bool have_selection = false;
    if (user_selected_device)
        have_selection = device_combo.active >= 0;

    /* Add new devices */
    int index = 0;
    Gtk::TreeIter iter;
    foreach (auto device in devices)
    {
        int n_delete = -1;

        /* Find if already exists */
        if (device_model.iter_nth_child(&iter, NULL, index))
        {
            int i = 0;
            do
            {
                std::string name;
                bool matched;

                device_model.get(iter, 0, &name, -1);
                matched = name == device.name;

                if (matched)
                {
                    n_delete = i;
                    break;
                }
                i++;
            } while (device_model.iter_next(&iter));
        }

        /* If exists, remove elements up to this one */
        if (n_delete >= 0)
        {
            int i;

            /* Update label */
            device_model.set(iter, 1, device.label, -1);

            for (i = 0; i < n_delete; i++)
            {
                device_model.iter_nth_child(&iter, NULL, index);
#if VALA_0_36
                device_model.remove(&iter);
#else
                device_model.remove(iter);
#endif
            }
        }
        else
        {
            device_model.insert(&iter, index);
            device_model.set(iter, 0, device.name, 1, device.label, -1);
        }
        index++;
    }

    /* Remove any remaining devices */
    while (device_model.iter_nth_child(&iter, NULL, index))
#if VALA_0_36
        device_model.remove(&iter);
#else
        device_model.remove(iter);
#endif

    /* Select the previously selected device or the first available device */
    if (!have_selection)
    {
        std::string device = settings.get_string("selected-device");
        if (device != NULL && find_scan_device(device, &iter))
            device_combo.set_active_iter(iter);
        else
            device_combo.set_active(0);
    }

    setting_devices = false;

    update_scan_status();
}

void AppWindow::set_selected_device(std::string device)
{
    user_selected_device = true;

    Gtk::TreeIter iter;
    if (!find_scan_device(device, &iter))
        return;

    device_combo.set_active_iter(iter);
}

void AppWindow::crop_set_action_cb(SimpleAction action, Variant *value)
{
    set_crop(value.get_string());
};

void AppWindow::crop_rotate_action_cb(void)
{
    Page page = book_view.selected_page;
    if (page == NULL)
        return;
    page.rotate_crop();
};

void AppWindow::save_document_cb(void)
{
    save_document_async.begin();
};

void AppWindow::size_allocate(int width, int height, int baseline) override
{
    base.size_allocate(width, height, baseline);

    if (!window_is_maximized && !window_is_fullscreen)
    {
        window_width = this.get_width();
        window_height = this.get_height();
        save_state();
    }
};

void AppWindow::unmap(void)
{
    window_is_maximized = is_maximized();
    window_is_fullscreen = is_fullscreen();
    save_state();

    base.unmap();
};

void AppWindow::start(void)
{
    visible = true;
    autosave_manager = new AutosaveManager();
    autosave_manager.book = book;

    if (autosave_manager.exists())
    {
        prompt_to_load_autosaved_book.begin((obj, res) = > {
            bool restore = prompt_to_load_autosaved_book.end(res);

            if (restore)
            {
                autosave_manager.load();
            }

            if (book.n_pages == 0)
                book_needs_saving = false;
            else
            {
                stack.set_visible_child_name("document");
                book_view.selected_page = book.get_page(0);
                book_needs_saving = true;
                book_changed_cb(book);
            }
        });
    }
};

CancellableProgressBar::CancellableProgressBar(std::string *text, Cancellable *cancellable)
{
    this.orientation = Gtk::Orientation.HORIZONTAL;

    bar = new Gtk::ProgressBar();
    bar.visible = true;
    bar.set_text(text);
    bar.set_show_text(true);
    prepend(bar);

    if (cancellable != NULL)
    {
        button = new Gtk::Button.with_label(/* Text of button for cancelling save */
                                            _("Cancel"));
        button.visible = true;
        button.clicked.connect(() = >
                                    {
                                        set_visible(false);
                                        cancellable.cancel();
                                    });
        prepend(button);
    }
};

void CancellableProgressBar::set_fraction(double fraction)
{
    bar.set_fraction(fraction);
};

void CancellableProgressBar::remove_with_delay(uint delay, Gtk::ActionBar parent)
{
    button.set_sensitive(false);

    Timeout.add(
        delay, () = >
                    {
                        parent.remove(this);
                        return false;
                    });
};

CropActions::CropActions(AppWindow window)
{
    group = new GLib::SimpleActionGroup();
    group.add_action_entries(crop_entries, window);

    crop_set = (GLib::SimpleAction)group.lookup_action("set");
    crop_rotate = (GLib::SimpleAction)group.lookup_action("rotate");

    window.insert_action_group("crop", group);
};

void CropActions::update_current_crop(std::string *crop_name)
{
    crop_rotate.set_enabled(crop_name != NULL);

    if (crop_name == NULL)
        crop_set.set_state("none");
    else
        crop_set.set_state(crop_name);
};
