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
#include "page.h"

void Page::parse_line(ScanLine line, int n, bool *size_changed)
    {
        int line_number = line.number + n;

        /* Extend image if necessary */
        size_changed = false;
        while (line_number >= scan_height)
        {
            /* Extend image */
            int rows = scan_height;
            scan_height = rows + scan_width / 2;
            spdlog::spdlog::debug("Extending image from %d lines to %d lines", rows, scan_height);
            pixels.resize(scan_height * rowstride);

            size_changed = true;
        }

        /* Copy in new row */
        int offset = line_number * rowstride;
        int line_offset = n * line.data_length;
        for (int i = 0; i < line.data_length; i++)
            pixels[offset + i] = line.data[line_offset + i];

        scan_line = line_number;
    };

    // FIXME: Copied from page-view, should be shared code
uchar Page::get_sample(uchar pixels[], int offset, int x, int depth, int n_channels, int channel)
    {
        // FIXME
        return 0xFF;
    };

    // FIXME: Copied from page-view, should be shared code
void Page::get_pixel(int x, int y, uchar pixel[], int offset)
    {
        switch (scan_direction)
        {
        case ScanDirection::TOP_TO_BOTTOM:
            break;
        case ScanDirection::BOTTOM_TO_TOP:
            x = scan_width - x - 1;
            y = scan_height - y - 1;
            break;
        case ScanDirection::LEFT_TO_RIGHT:
            var t = x;
            x = scan_width - y - 1;
            y = t;
            break;
        case ScanDirection::RIGHT_TO_LEFT:
            var t = x;
            x = y;
            y = scan_height - t - 1;
            break;
        }

        int line_offset = rowstride * y;

        /* Optimise for 8 bit images */
        if (depth == 8 && n_channels == 3)
        {
            var o = line_offset + x * n_channels;
            pixel[offset + 0] = pixels[o];
            pixel[offset + 1] = pixels[o + 1];
            pixel[offset + 2] = pixels[o + 2];
            return;
        }
        else if (depth == 8 && n_channels == 1)
        {
            var p = pixels[line_offset + x];
            pixel[offset + 0] = pixel[offset + 1] = pixel[offset + 2] = p;
            return;
        }

        /* Optimise for bitmaps */
        else if (depth == 1 && n_channels == 1)
        {
            var p = pixels[line_offset + (x / 8)];
            pixel[offset + 0] = pixel[offset + 1] = pixel[offset + 2] = (p & (0x80 >> (x % 8))) != 0 ? 0x00 : 0xFF;
            return;
        }

        /* Optimise for 2 bit images */
        else if (depth == 2 && n_channels == 1)
        {
            int block_shift[4] = {6, 4, 2, 0};

            uchar p = pixels[line_offset + (x / 4)];
            int sample = (p >> block_shift[x % 4]) & 0x3;
            sample = sample * 255 / 3;

            pixel[offset + 0] = pixel[offset + 1] = pixel[offset + 2] = (uchar)sample;
            return;
        }

        /* Use slow method */
        pixel[offset + 0] = get_sample(pixels, line_offset, x, depth, n_channels, 0);
        pixel[offset + 1] = get_sample(pixels, line_offset, x, depth, n_channels, 1);
        pixel[offset + 2] = get_sample(pixels, line_offset, x, depth, n_channels, 2);
    };

    /* Width of the page in pixels after rotation applied */
int width
{
    get
    {
        if (scan_direction == ScanDirection::TOP_TO_BOTTOM || scan_direction == ScanDirection::BOTTOM_TO_TOP)
            return scan_width;
        else
            return scan_height;
    }
}

/* Height of the page in pixels after rotation applied */
int height
{
    get
    {
        if (scan_direction == ScanDirection::TOP_TO_BOTTOM || scan_direction == ScanDirection::BOTTOM_TO_TOP)
            return scan_height;
        else
            return scan_width;
    }
}

/* true if the page is landscape (wider than the height) */
bool is_landscape
{
    get { return width > height; }
}

/* true if scan contains color information */
bool is_color
{
    get { return n_channels > 1; }
}

/* Rotation of scanned data */
ScanDirection Page::scan_direction
{
        get { return scan_direction_; }

        set
        {
            if (scan_direction_ == value)
                return;

            /* Work out how many times it has been rotated to the left */
            var size_has_changed = false;
            var left_steps = (int)(value - scan_direction_);
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
                    var t = crop_x;
                    crop_x = crop_y;
                    crop_y = width - (t + crop_width);
                    t = crop_width;
                    crop_width = crop_height;
                    crop_height = t;
                    break;
                /* 180 degrees */
                case 2:
                    crop_x = width - (crop_x + crop_width);
                    crop_y = width - (crop_y + crop_height);
                    break;
                /* 90 degrees clockwise */
                case 3:
                    var t = crop_y;
                    crop_y = crop_x;
                    crop_x = height - (t + crop_height);
                    t = crop_width;
                    crop_width = crop_height;
                    crop_height = t;
                    break;
                }
            }

            scan_direction_ = value;
            if (size_has_changed)
                size_changed();
            scan_direction_changed();
            if (has_crop)
                crop_changed();
        }
    }

Page::type_signal_pixels_changed Page::signal_pixels_changed() {
    return m_signal_pixels_changed;
}

Page::type_signal_size_changed Page::signal_size_changed() {
    return m_signal_size_changed;
}

Page::type_signal_scan_line_changed Page::signal_scan_line_changed() {
    return m_signal_scan_line_changed;
}

Page::type_signal_scan_direction_changed Page::signal_scan_direction_changed() {
    return m_signal_scan_direction_changed;
}

Page::type_signal_crop_changed Page::signal_crop_changed() {
    return m_signal_crop_changed;
}

Page::type_signal_scan_finished Page::signal_scan_finished() {
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
        this->scan_direction = scan_direction;
    };

Page::from_data(int scan_width,
              int scan_height,
              int rowstride,
              int n_channels,
              int depth,
              int dpi,
              ScanDirection scan_direction,
              std::string *color_profile,
              uchar *pixels[],
              bool has_crop,
              std::string *crop_name,
              int crop_x,
              int crop_y,
              int crop_width,
              int crop_height)
    {
        this.scan_width = scan_width;
        this.scan_height = scan_height;
        this.expected_rows = scan_height;
        this.rowstride = rowstride;
        this.n_channels = n_channels;
        this.depth = depth;
        this.dpi = dpi;
        this.scan_direction = scan_direction;
        this.color_profile = color_profile;
        this.pixels = pixels;
        has_data = pixels != NULL;
        this.has_crop = has_crop;
        this.crop_name = crop_name;
        this.crop_x = crop_x;
        this.crop_y = crop_y;
        this.crop_width = (crop_x + crop_width > scan_width) ? scan_width : crop_width;
        this.crop_height = (crop_y + crop_height > scan_height) ? scan_height : crop_height;
    };

Page Page::copy(void)
    {
        Page copy = new Page::from_data(
            scan_width,
            scan_height,
            rowstride,
            n_channels,
            depth,
            dpi,
            scan_direction,
            color_profile,
            pixels,
            has_crop,
            crop_name,
            crop_x,
            crop_y,
            crop_width,
            crop_height);

        copy.scan_line = scan_line;

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
        return_if_fail(pixels != NULL);

        /* Fill with white */
        if (depth == 1)
            Memory.set(pixels, 0x00, scan_height * rowstride);
        else
            Memory.set(pixels, 0xFF, scan_height * rowstride);

        size_changed();
        pixels_changed();
    };

void Page::start(void)
    {
        is_scanning = true;
        scan_line_changed();
    };

void Page::parse_scan_line(ScanLine line)
    {
        bool size_has_changed = false;
        for (int i = 0; i < line.n_lines; i++)
            parse_line(line, i, &size_has_changed);

        has_data = true;

        if (size_has_changed)
            size_changed();
        scan_line_changed();
        pixels_changed();
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
            size_changed();
        scan_line_changed();
        scan_finished();
    }

void Page::rotate_left(void)
    {
        switch (scan_direction)
        {
        case ScanDirection::TOP_TO_BOTTOM:
            scan_direction = ScanDirection::LEFT_TO_RIGHT;
            break;
        case ScanDirection::LEFT_TO_RIGHT:
            scan_direction = ScanDirection::BOTTOM_TO_TOP;
            break;
        case ScanDirection::BOTTOM_TO_TOP:
            scan_direction = ScanDirection::RIGHT_TO_LEFT;
            break;
        case ScanDirection::RIGHT_TO_LEFT:
            scan_direction = ScanDirection::TOP_TO_BOTTOM;
            break;
        }
    };

void Page::rotate_right(void)
    {
        switch (scan_direction)
        {
        case ScanDirection::TOP_TO_BOTTOM:
            scan_direction = ScanDirection::RIGHT_TO_LEFT;
            break;
        case ScanDirection::LEFT_TO_RIGHT:
            scan_direction = ScanDirection::TOP_TO_BOTTOM;
            break;
        case ScanDirection::BOTTOM_TO_TOP:
            scan_direction = ScanDirection::LEFT_TO_RIGHT;
            break;
        case ScanDirection::RIGHT_TO_LEFT:
            scan_direction = ScanDirection::BOTTOM_TO_TOP;
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
        crop_changed();
    };

void Page::set_custom_crop(int width, int height)
    {
        return_if_fail(width >= 1);
        return_if_fail(height >= 1);

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

        crop_changed();
    };

void Page::set_named_crop(std::string name)
    {
        double w, h;
        switch (name)
        {
        case "A3":
            w = 11.692;
            h = 16.535;
            break;
        case "A4":
            w = 8.267;
            h = 11.692;
            break;
        case "A5":
            w = 5.846;
            h = 8.267;
            break;
        case "A6":
            w = 4.1335;
            h = 5.846;
            break;
        case "letter":
            w = 8.5;
            h = 11;
            break;
        case "legal":
            w = 8.5;
            h = 14;
            break;
        case "4x6":
            w = 4;
            h = 6;
            break;
        default:
            spdlog::warning("Unknown paper size '%s'", name);
            return;
        }

        crop_name = name;
        has_crop = true;

        var pw = width;
        var ph = height;

        /* Rotate to match original aspect */
        if (pw > ph)
        {
            var t = w;
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
        crop_changed();
    };

void Page::move_crop(int x, int y)
    {
        return_if_fail(x >= 0);
        return_if_fail(y >= 0);
        return_if_fail(x < width);
        return_if_fail(y < height);

        crop_x = x;
        crop_y = y;
        crop_changed();
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
            int w = width;
            int h = height;

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

        crop_changed();
    };

uchar[] Page::get_pixels(void)
    {
        return pixels;
    };

Gdk::Pixbuf Page::get_image(bool apply_crop)
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
            if (r > width)
                r = width;
            if (t < 0)
                t = 0;
            if (b > height)
                b = height;
        }
        else
        {
            l = 0;
            r = width;
            t = 0;
            b = height;
        }

        Gdk::Pixbuf image = new Gdk::Pixbuf(Gdk::Colorspace::RGB, false, 8, r - l, b - t);
        uint8 image_pixels[] = image.get_pixels();
        for (int y = t; y < b; y++)
        {
            int offset = image.get_rowstride() * (y - t);
            for (int x = l; x < r; x++)
                get_pixel(x, y, image_pixels, offset + (x - l) * 3);
        }

        return image;
    };

std::string Page::get_icc_data_encoded(void)
    {
        if (color_profile == NULL)
            return NULL;

        /* Get binary data */
        std::string contents;
        try
        {
            FileUtils.get_contents(color_profile, &contents);
        }
        catch (Error e)
        {
            spdlog::warning("failed to get icc profile data: %s", e.message);
            return NULL;
        }

        /* Encode into base64 */
        return Base64.encode((uchar[])contents.to_utf8());
    };

void Page::copy_to_clipboard(Gtk::Window window)
    {
        Glib::RefPtr<Gdk::Clipboard> clipboard = window.get_clipboard();
        Gdk::Pixbuf image = get_image(true);
        clipboard.set_value(image);
    };

void Page::save_png(File file) // throws Error
    {
        var stream = file.replace(NULL, false, FileCreateFlags::NONE, NULL);
        var image = get_image(true);

        std::string *icc_profile_data = NULL;
        if (color_profile != NULL)
            icc_profile_data = get_icc_data_encoded();

        std::string keys[] = {"x-dpi", "y-dpi", "icc-profile", NULL};
        std::string values[] = {"%d".printf(dpi), "%d".printf(dpi), icc_profile_data, NULL};
        if (icc_profile_data == NULL)
            keys[2] = NULL;

        image.save_to_callbackv((buf) = >
                                        {
                                            stream.write_all(buf, NULL, NULL);
                                            return true;
                                        },
                                "png", keys, values);
    };
