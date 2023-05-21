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

#include <string>
#include <uchar.h>
#include <spdlog/spdlog.h>
#include <glibmm.h>
#include "page.h"

void Page::parse_line(ScanLine line, int n, bool *size_changed)
{
    int line_number = line.number + n;

    /* Extend image if necessary */
    bool t = false;
    size_changed = &t;
    while (line_number >= scan_height)
    {
        /* Extend image */
        int rows = scan_height;
        scan_height = rows + scan_width / 2;
        spdlog::debug("Extending image from %d lines to %d lines", rows, scan_height);
        pixels.resize(scan_height * rowstride);

        t = true;
        size_changed = &t;
    }

    /* Copy in new row */
    int offset = line_number * rowstride;
    int line_offset = n * line.data_length;
    for (int i = 0; i < line.data_length; i++)
        pixels[offset + i] = line.data[line_offset + i];

    scan_line = line_number;
};

// FIXME: Copied from page-view, should be shared code
char32_t Page::get_sample(char32_t pixels[], int offset, int x, int depth, int n_channels, int channel)
{
    // FIXME
    return 0xFF;
};

// FIXME: Copied from page-view, should be shared code
void Page::get_pixel(int x, int y, char32_t pixel[], int offset)
{
    switch (get_scan_direction())
    {
    case ScanDirection::TOP_TO_BOTTOM:
        break;
    case ScanDirection::BOTTOM_TO_TOP:
        x = scan_width - x - 1;
        y = scan_height - y - 1;
        break;
    case ScanDirection::LEFT_TO_RIGHT:
        int t = x;
        x = scan_width - y - 1;
        y = t;
        break;
    case ScanDirection::RIGHT_TO_LEFT:
        int t = x;
        x = y;
        y = scan_height - t - 1;
        break;
    }

    int line_offset = rowstride * y;

    /* Optimise for 8 bit images */
    if (depth == 8 && n_channels == 3)
    {
        int o = line_offset + x * n_channels;
        pixel[offset + 0] = pixels[o];
        pixel[offset + 1] = pixels[o + 1];
        pixel[offset + 2] = pixels[o + 2];
        return;
    }
    else if (depth == 8 && n_channels == 1)
    {
        int p = pixels[line_offset + x];
        pixel[offset + 0] = pixel[offset + 1] = pixel[offset + 2] = p;
        return;
    }

    /* Optimise for bitmaps */
    else if (depth == 1 && n_channels == 1)
    {
        int p = pixels[line_offset + (x / 8)];
        pixel[offset + 0] = pixel[offset + 1] = pixel[offset + 2] = (p & (0x80 >> (x % 8))) != 0 ? 0x00 : 0xFF;
        return;
    }

    /* Optimise for 2 bit images */
    else if (depth == 2 && n_channels == 1)
    {
        int block_shift[4] = {6, 4, 2, 0};

        char32_t p = pixels[line_offset + (x / 4)];
        int sample = (p >> block_shift[x % 4]) & 0x3;
        sample = sample * 255 / 3;

        pixel[offset + 0] = pixel[offset + 1] = pixel[offset + 2] = (char32_t)sample;
        return;
    }

    /* Use slow method */
    pixel[offset + 0] = get_sample(pixels, line_offset, x, depth, n_channels, 0);
    pixel[offset + 1] = get_sample(pixels, line_offset, x, depth, n_channels, 1);
    pixel[offset + 2] = get_sample(pixels, line_offset, x, depth, n_channels, 2);
};

/* Width of the page in pixels after rotation applied */
int Page::get_width(void)
{
    ScanDirection sd = get_scan_direction();
    if (sd == ScanDirection::TOP_TO_BOTTOM || sd == ScanDirection::BOTTOM_TO_TOP)
        return scan_width;
    else
        return scan_height;
}

/* Height of the page in pixels after rotation applied */
int Page::get_height(void)
{
    ScanDirection sd = get_scan_direction();
    if (sd == ScanDirection::TOP_TO_BOTTOM || sd == ScanDirection::BOTTOM_TO_TOP)
        return scan_height;
    else
        return scan_width;
}

/* true if the page is landscape (wider than the height) */
bool Page::is_landscape(void)
{
    return get_width() > get_height();
}

/* true if scan contains color information */
bool Page::is_color(void)
{
    return n_channels > 1;
}

/* Rotation of scanned data */
ScanDirection Page::get_scan_direction(void)
{
    return _scan_direction;
}

void Page::set_scan_direction(ScanDirection value)
{
    if (_scan_direction == value)
        return;

    /* Work out how many times it has been rotated to the left */
    bool size_has_changed = false;
    int left_steps = (int)(value - _scan_direction);
    if (left_steps < 0)
        left_steps += 4;
    if (left_steps != 2)
        size_has_changed = true;

    /* Rotate crop */
    if (has_crop)
    {
        switch (left_steps)
        {
        /* 90 degrees counter-clockwise */
        case 1:
            int t = crop_x;
            crop_x = crop_y;
            crop_y = get_width() - (t + crop_width);
            t = crop_width;
            crop_width = crop_height;
            crop_height = t;
            break;
        /* 180 degrees */
        case 2:
            crop_x = get_width() - (crop_x + crop_width);
            crop_y = get_width() - (crop_y + crop_height);
            break;
        /* 90 degrees clockwise */
        case 3:
            int t = crop_y;
            crop_y = crop_x;
            crop_x = get_height() - (t + crop_height);
            t = crop_width;
            crop_width = crop_height;
            crop_height = t;
            break;
        }
    }

    _scan_direction = value;
    if (size_has_changed)
        signal_size_changed().emit();
    signal_scan_direction_changed().emit();
    if (has_crop)
        signal_crop_changed().emit();
}

Page::type_signal_pixels_changed Page::signal_pixels_changed()
{
    return m_signal_pixels_changed;
}

Page::type_signal_size_changed Page::signal_size_changed()
{
    return m_signal_size_changed;
}

Page::type_signal_scan_line_changed Page::signal_scan_line_changed()
{
    return m_signal_scan_line_changed;
}

Page::type_signal_scan_direction_changed Page::signal_scan_direction_changed()
{
    return m_signal_scan_direction_changed;
}

Page::type_signal_crop_changed Page::signal_crop_changed()
{
    return m_signal_crop_changed;
}

Page::type_signal_scan_finished Page::signal_scan_finished()
{
    return m_signal_scan_finished;
}

Page::Page(int width, int height, int dpi, ScanDirection scan_direction)
{
    if (scan_direction == ScanDirection::TOP_TO_BOTTOM || scan_direction == ScanDirection::BOTTOM_TO_TOP)
    {
        scan_width = width;
        scan_height = height;
    }
    else
    {
        scan_width = height;
        scan_height = width;
    }
    this->dpi = dpi;
    this->_scan_direction = scan_direction;
};

Page *Page::from_data(int scan_width,
                      int scan_height,
                      int rowstride,
                      int n_channels,
                      int depth,
                      int dpi,
                      ScanDirection scan_direction,
                      std::string *color_profile,
                      char32_t *pixels[],
                      bool has_crop,
                      std::string *crop_name,
                      int crop_x,
                      int crop_y,
                      int crop_width,
                      int crop_height)
{
    Page *pg = new Page(scan_width, scan_height, dpi, scan_direction);
    pg->expected_rows = scan_height;
    pg->rowstride = rowstride;
    pg->n_channels = n_channels;
    pg->depth = depth;
    pg->dpi = dpi;
    pg->color_profile = color_profile;
    pg->pixels = pixels;
    pg->has_data = pixels != NULL;
    pg->has_crop = has_crop;
    pg->crop_name = crop_name;
    pg->crop_x = crop_x;
    pg->crop_y = crop_y;
    pg->crop_width = (crop_x + crop_width > scan_width) ? scan_width : crop_width;
    pg->crop_height = (crop_y + crop_height > scan_height) ? scan_height : crop_height;

    return pg;
};

Page *Page::copy(void)
{
    Page *copy = from_data(
        scan_width,
        scan_height,
        rowstride,
        n_channels,
        depth,
        dpi,
        _scan_direction,
        color_profile,
        pixels,
        has_crop,
        crop_name,
        crop_x,
        crop_y,
        crop_width,
        crop_height);

    copy->scan_line = scan_line;

    return copy;
};

void Page::set_page_info(ScanPageInfo info)
{
    expected_rows = info.height;
    dpi = (int)info.dpi;

    /* Create a white page */
    scan_width = info.width;
    scan_height = info.height;
    /* Variable height, try 50% of the width for now */
    if (scan_height < 0)
        scan_height = scan_width / 2;
    depth = info.depth;
    n_channels = info.n_channels;
    rowstride = (scan_width * depth * n_channels + 7) / 8;
    pixels.resize(scan_height * rowstride);

    if (pixels != NULL)
    {
        return;
    }

    /* Fill with white */
    if (depth == 1)
        memset(pixels, 0x00, scan_height * rowstride);
    else
        memset(pixels, 0xFF, scan_height * rowstride);

    signal_size_changed().emit();
    signal_pixels_changed().emit();
};

void Page::start(void)
{
    is_scanning = true;
    signal_scan_line_changed().emit();
};

void Page::parse_scan_line(ScanLine line)
{
    bool size_has_changed = false;
    for (int i = 0; i < line.n_lines; i++)
        parse_line(line, i, &size_has_changed);

    has_data = true;

    if (size_has_changed)
        signal_size_changed().emit();
    signal_scan_line_changed().emit();
    signal_pixels_changed().emit();
};

void Page::finish(void)
{
    bool size_has_changed = false;

    /* Trim page */
    if (expected_rows < 0 &&
        scan_line != scan_height)
    {
        int rows = scan_height;
        scan_height = scan_line;
        pixels.resize(scan_height * rowstride);
        spdlog::debug("Trimming page from %d lines to %d lines", rows, scan_height);

        size_has_changed = true;
    }
    is_scanning = false;

    if (size_has_changed)
        signal_size_changed().emit();
    signal_scan_line_changed().emit();
    signal_scan_finished().emit();
}

void Page::rotate_left(void)
{
    switch (get_scan_direction())
    {
    case ScanDirection::TOP_TO_BOTTOM:
        set_scan_direction(ScanDirection::LEFT_TO_RIGHT);
        break;
    case ScanDirection::LEFT_TO_RIGHT:
        set_scan_direction(ScanDirection::BOTTOM_TO_TOP);
        break;
    case ScanDirection::BOTTOM_TO_TOP:
        set_scan_direction(ScanDirection::RIGHT_TO_LEFT);
        break;
    case ScanDirection::RIGHT_TO_LEFT:
        set_scan_direction(ScanDirection::TOP_TO_BOTTOM);
        break;
    }
};

void Page::rotate_right(void)
{
    switch (get_scan_direction())
    {
    case ScanDirection::TOP_TO_BOTTOM:
        set_scan_direction(ScanDirection::RIGHT_TO_LEFT);
        break;
    case ScanDirection::LEFT_TO_RIGHT:
        set_scan_direction(ScanDirection::TOP_TO_BOTTOM);
        break;
    case ScanDirection::BOTTOM_TO_TOP:
        set_scan_direction(ScanDirection::LEFT_TO_RIGHT);
        break;
    case ScanDirection::RIGHT_TO_LEFT:
        set_scan_direction(ScanDirection::BOTTOM_TO_TOP);
        break;
    }
};

void Page::set_no_crop(void)
{
    if (!has_crop)
        return;
    has_crop = false;
    crop_name = NULL;
    crop_x = 0;
    crop_y = 0;
    crop_width = 0;
    crop_height = 0;
    signal_crop_changed().emit();
};

void Page::set_custom_crop(int width, int height)
{
    if (width >= 1)
    {
        return;
    }
    if (height >= 1)
    {
        return;
    }

    if (crop_name == NULL && has_crop && crop_width == width && crop_height == height)
        return;
    crop_name = NULL;
    has_crop = true;

    crop_width = width;
    crop_height = height;

    /*var pw = width;
    var ph = height;
    if (crop_width < pw)
        crop_x = (pw - crop_width) / 2;
    else
        crop_x = 0;
    if (crop_height < ph)
        crop_y = (ph - crop_height) / 2;
    else
        crop_y = 0;*/

    signal_crop_changed().emit();
};

void Page::set_named_crop(std::string name)
{
    double w, h;
    if (name == "A3")
    {
        w = 11.692;
        h = 16.535;
    }
    else if (name == "A4")
    {
        w = 8.267;
        h = 11.692;
    }
    else if (name == "A5")
    {
        w = 5.846;
        h = 8.267;
    }
    else if (name == "A6")
    {
        w = 4.1335;
        h = 5.846;
    }
    else if (name == "letter")
    {
        w = 8.5;
        h = 11;
    }
    else if (name == "legal")
    {
        w = 8.5;
        h = 14;
    }
    else if (name == "4x6")
    {
        w = 4;
        h = 6;
    }
    else
    {
        spdlog::warn("Unknown paper size '%s'", name);
        return;
    }

    crop_name = &name;
    has_crop = true;

    int pw = get_width();
    int ph = get_height();

    /* Rotate to match original aspect */
    if (pw > ph)
    {
        int t = w;
        w = h;
        h = t;
    }

    /* Custom crop, make slightly smaller than original */
    crop_width = (int)(w * dpi + 0.5);
    crop_height = (int)(h * dpi + 0.5);

    if (crop_width < pw)
        crop_x = (pw - crop_width) / 2;
    else
        crop_x = 0;
    if (crop_height < ph)
        crop_y = (ph - crop_height) / 2;
    else
        crop_y = 0;
    signal_crop_changed().emit();
};

void Page::move_crop(int x, int y)
{
    if (x >= 0) {
        return;
    }
    if (y >= 0) {
        return;
    }
    if ( x < get_width()) {
        return;
    }
    if (y < get_height()) {
        return;
    }

    crop_x = x;
    crop_y = y;
    signal_crop_changed().emit();
};

void Page::rotate_crop(void)
{
    if (!has_crop)
        return;

    int t = crop_width;
    crop_width = crop_height;
    crop_height = t;

    /* Clip custom crops */
    if (crop_name == NULL)
    {
        int w = get_width();
        int h = get_height();

        if (crop_x + crop_width > w)
            crop_x = w - crop_width;
        if (crop_x < 0)
        {
            crop_x = 0;
            crop_width = w;
        }
        if (crop_y + crop_height > h)
            crop_y = h - crop_height;
        if (crop_y < 0)
        {
            crop_y = 0;
            crop_height = h;
        }
    }

    signal_crop_changed().emit();
};

char32_t[] Page::get_pixels(void)
{
    return pixels;
};

Glib::RefPtr<Gdk::Pixbuf> Page::get_image(bool apply_crop)
{
    int l, r, t, b;
    if (apply_crop && has_crop)
    {
        l = crop_x;
        r = l + crop_width;
        t = crop_y;
        b = t + crop_height;

        if (l < 0)
            l = 0;
        if (r > get_width())
            r = get_width();
        if (t < 0)
            t = 0;
        if (b > get_height())
            b = get_height();
    }
    else
    {
        l = 0;
        r = get_width();
        t = 0;
        b = get_height();
    }
    auto image = Gdk::Pixbuf::create(Gdk::Colorspace::RGB, false, 8, r - l, b - t);
    guint8 image_pixels[] = image->get_pixels();
    for (int y = t; y < b; y++)
    {
        int offset = image->get_rowstride() * (y - t);
        for (int x = l; x < r; x++)
            get_pixel(x, y, image_pixels, offset + (x - l) * 3);
    }

    return image;
};

// std::string Page::get_icc_data_encoded(void)
// {
//     if (color_profile == NULL)
//         return NULL;

//     /* Get binary data */
//     std::string contents;
//     try
//     {
//         FileUtils.get_contents(color_profile, &contents);
//     }
//     catch (Error e)
//     {
//         spdlog::warn("failed to get icc profile data: %s", e.message);
//         return NULL;
//     }

//     /* Encode into base64 */
//     return Base64.encode((char32_t[])contents.to_utf8());
// };

void Page::copy_to_clipboard(Gtk::Window window)
{
    auto clipboard = window.get_clipboard();
    auto image = get_image(true);
    auto texture = Gdk::Texture::create_for_pixbuf(image);
    clipboard->set_texture(texture);
};

void Page::save_png(Glib::RefPtr<Gio::File> file) // throws Error
{
    auto stream = file->replace(NULL, NULL, false, Gio::File::CreateFlags::NONE);
    auto image = get_image(true);

    std::string *icc_profile_data = NULL;
    if (color_profile != NULL) {
        icc_profile_data = get_icc_data_encoded();
    }

    std::string keys[] = {"x-dpi", "y-dpi", "icc-profile", NULL};
    std::string dpi_str = std::to_string(dpi);
    std::string values[] = {dpi_str, dpi_str, *icc_profile_data, NULL};
    if (icc_profile_data == NULL) {
        keys[2] = NULL;
    }

    image->save_to_callbackv((buf) = >
                                    {
                                        stream.write_all(buf, NULL, NULL);
                                        return true;
                                    },
                            "png", keys, values);
};
