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

#include <sigc++-3.0/sigc++/sigc++.h>
#include <gtkmm-4.0/gtkmm.h>
#include <uchar.h>
#include "scanner.h"

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
        std::vector<unsigned char> pixels;
    /* Rotation of scanned data */
        ScanDirection _scan_direction = ScanDirection::TOP_TO_BOTTOM;

        void parse_line (ScanLine line, int n, bool *size_changed);

    // FIXME: Copied from page-view, should be shared code
        unsigned char get_sample (std::vector<unsigned char> pixels, int offset, int x, int depth, int n_channels, int channel);
    // FIXME: Copied from page-view, should be shared code
        void get_pixel (int x, int y, guint8* pixel, int offset);

    public:
    /* Width of the page in pixels after rotation applied */
        int get_width(void);

    /* Height of the page in pixels after rotation applied */
        int get_height(void);

    /* true if the page is landscape (wider than the height) */
        bool is_landscape(void);

    /* Resolution of page */
        int dpi;

    /* Bit depth */
        int depth;

    /* Color profile */
        std::string *color_profile;

    /* Width of raw scan data in pixels */
        int scan_width ;

    /* Height of raw scan data in pixels */
        int scan_height;

    /* Offset between rows in scan data */
        int rowstride;

    /* Number of color channels */
        int n_channels;

    /* Page is getting data */
        bool is_scanning;

    /* true if have some page data */
        bool has_data;

    /* Expected next scan row */
        int scan_line;

    /* true if scan contains color information */
        bool is_color(void);

        ScanDirection get_scan_direction(void);
        void set_scan_direction(ScanDirection);

    /* True if the page has a crop set */
        bool has_crop;

    /* Name of the crop if using a named crop */
        std::string *crop_name;

    /* X co-ordinate of top left crop corner */
        int crop_x;

    /* Y co-ordinate of top left crop corner */
        int crop_y;

    /* Width of crop in pixels */
        int crop_width;

    /* Height of crop in pixels*/
        int crop_height;

        using type_signal_pixels_changed = sigc::signal<void(void)>;
        type_signal_pixels_changed signal_pixels_changed();
        using type_signal_size_changed = sigc::signal<void(void)>;
        type_signal_size_changed signal_size_changed();
        using type_signal_scan_line_changed = sigc::signal<void(void)>;
        type_signal_scan_line_changed signal_scan_line_changed();
        using type_signal_scan_direction_changed = sigc::signal<void(void)>;
        type_signal_scan_direction_changed signal_scan_direction_changed();
        using type_signal_crop_changed = sigc::signal<void(void)>;
        type_signal_crop_changed signal_crop_changed();
        using type_signal_scan_finished = sigc::signal<void(void)>;
        type_signal_scan_finished signal_scan_finished();

        Page (int width, int height, int dpi, ScanDirection scan_direction);
        static Page *from_data (int scan_width,
                           int scan_height,
                           int rowstride,
                           int n_channels,
                           int depth,
                           int dpi,
                           ScanDirection scan_direction,
                           std::string *color_profile,
                           std::vector<unsigned char> *pixels,
                           bool has_crop,
                           std::string *crop_name,
                           int crop_x,
                           int crop_y,
                           int crop_width,
                           int crop_height);
        Page *copy(void);
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
        std::vector<unsigned char> get_pixels (void);
        Glib::RefPtr<Gdk::Pixbuf> get_image (bool apply_crop);
        std::string *get_icc_data_encoded (void);
        void copy_to_clipboard (Gtk::Window window);
        void save_png (Glib::RefPtr<Gio::File> file); // throws Error;

    protected:
        type_signal_pixels_changed m_signal_pixels_changed;
        type_signal_size_changed m_signal_size_changed;
        type_signal_scan_line_changed m_signal_scan_line_changed;
        type_signal_scan_direction_changed m_signal_scan_direction_changed;
        type_signal_crop_changed m_signal_crop_changed;
        type_signal_scan_finished m_signal_scan_finished;
};

#endif //PAGE_H