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
#ifndef PAGE_H
#define PAGE_H

#include <gtkmm.h>

enum ScanDirection
{
    TOP_TO_BOTTOM,
    LEFT_TO_RIGHT,
    BOTTOM_TO_TOP,
    RIGHT_TO_LEFT
};

class Page
{
    private:

    /* Number of rows in this page or -1 if currently unknown */
        int expected_rows;

    /* Pixel data */
        uchar[] pixels;
    /* Rotation of scanned data */
        ScanDirection scan_direction_ = ScanDirection.TOP_TO_BOTTOM;

        void parse_line (ScanLine line, int n, bool *size_changed);

    // FIXME: Copied from page-view, should be shared code
        uchar get_sample (uchar[] pixels, int offset, int x, int depth, int n_channels, int channel);
    // FIXME: Copied from page-view, should be shared code
        void get_pixel (int x, int y, uchar[] pixel, int offset);

    public:
    /* Width of the page in pixels after rotation applied */
        int width
    {
        get
        {
            if (scan_direction == ScanDirection.TOP_TO_BOTTOM || scan_direction == ScanDirection.BOTTOM_TO_TOP)
                return scan_width;
            else
                return scan_height;
        }
    };

    /* Height of the page in pixels after rotation applied */
        int height
    {
        get
        {
            if (scan_direction == ScanDirection.TOP_TO_BOTTOM || scan_direction == ScanDirection.BOTTOM_TO_TOP)
                return scan_height;
            else
                return scan_width;
        }
    };

    /* true if the page is landscape (wider than the height) */
        bool is_landscape { get { return width > height; } };

    /* Resolution of page */
        int dpi { get; private set; };

    /* Bit depth */
        int depth { get; private set; };

    /* Color profile */
        std::string *color_profile { get; set; };

    /* Width of raw scan data in pixels */
        int scan_width { get; private set; };

    /* Height of raw scan data in pixels */
        int scan_height { get; private set; };

    /* Offset between rows in scan data */
        int rowstride { get; private set; };

    /* Number of color channels */
        int n_channels { get; private set; };

    /* Page is getting data */
        bool is_scanning { get; private set; };

    /* true if have some page data */
        bool has_data { get; private set; };

    /* Expected next scan row */
        int scan_line { get; private set; };

    /* true if scan contains color information */
        bool is_color { get { return n_channels > 1; } };

        ScanDirection scan_direction
    {
        get { return scan_direction_; }

        set
        {
            if (scan_direction_ == value)
                return;

            /* Work out how many times it has been rotated to the left */
            var size_has_changed = false;
            var left_steps = (int) (value - scan_direction_);
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
                size_changed (void);
            scan_direction_changed (void);
            if (has_crop)
                crop_changed (void);
        }
    };

    /* True if the page has a crop set */
        bool has_crop { get; private set; };

    /* Name of the crop if using a named crop */
        std::string? crop_name { get; private set; };

    /* X co-ordinate of top left crop corner */
        int crop_x { get; private set; };

    /* Y co-ordinate of top left crop corner */
        int crop_y { get; private set; };

    /* Width of crop in pixels */
        int crop_width { get; private set; };

    /* Height of crop in pixels*/
        int crop_height { get; private set; };

        signal void pixels_changed (void);
        signal void size_changed (void);
        signal void scan_line_changed (void);
        signal void scan_direction_changed (void);
        signal void crop_changed (void);
        signal void scan_finished (void);

        Page (int width, int height, int dpi, ScanDirection scan_direction);
        Page.from_data (int scan_width,
                           int scan_height,
                           int rowstride,
                           int n_channels,
                           int depth,
                           int dpi,
                           ScanDirection scan_direction,
                           std::string? color_profile,
                           uchar[]? pixels,
                           bool has_crop,
                           std::string? crop_name,
                           int crop_x,
                           int crop_y,
                           int crop_width,
                           int crop_height);
        Page copy(void);
        void set_page_info (ScanPageInfo info);
        void start (void);
        void parse_scan_line (ScanLine line);
        void finish (void);
        void rotate_left (void);
        void rotate_right (void);
        void set_no_crop (void);
        void set_custom_crop (int width, int height);
        void set_named_crop (std::string name);
        void move_crop (int x, int y);
        void rotate_crop (void);
        unowned uchar[] get_pixels (void);
        Gdk::Pixbuf get_image (bool apply_crop);
        std::string? get_icc_data_encoded (void);
        void copy_to_clipboard (Gtk.Window window);
        void save_png (File file) throws Error;
}


#endif //PAGE_H