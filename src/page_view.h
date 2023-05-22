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
#ifndef PAGE_VIEW_H
#define PAGE_VIEW_H

#include <string>
#include <sigc++-3.0/sigc++/sigc++.h>
#include "page_texture.h"

enum CropLocation
{
    NONE = 0,
    MIDDLE,
    TOP,
    BOTTOM,
    LEFT,
    RIGHT,
    TOP_LEFT,
    TOP_RIGHT,
    BOTTOM_LEFT,
    BOTTOM_RIGHT
};

class PageView
{

    /* Image to render at current resolution */
    private:
        PageViewTexture *page_texture;

        int ruler_width = 8;

        int border_width = 2;

    /* True if image needs to be regenerated */
        bool update_image = true;

    /* Dimensions of image to generate */
        int width_;
        int height_;

        CropLocation crop_location;
        double selected_crop_px;
        double selected_crop_py;
        int selected_crop_x;
        int selected_crop_y;
        int selected_crop_w;
        int selected_crop_h;

        int animate_n_segments = 7;
        int animate_segment;
        uint animate_timeout;

        void new_buffer_cb();
        int get_preview_width ();
        int get_preview_height ();
        int page_to_screen_x (int x);
        int page_to_screen_y (int y);
        int screen_to_page_x (int x);
        int screen_to_page_y (int y);
        bool animation_cb ();
        void update_animation ();

        void page_pixels_changed_cb (Page p);
        void page_size_changed_cb (Page p);
        void page_overlay_changed_cb (Page p);
        void scan_direction_changed_cb (Page p);

    public:
    /* Page being rendered */
        Page page;

    /* Border around image */
        bool _selected = false;
        bool get_selected(void);
        void set_selected(bool);
    /* Location to place this page */
        int x_offset;
        int y_offset;

    /* Cursor over this page */
        std::string cursor = "arrow";

        using type_signal_size_changed = sigc::signal<void(void)>;
        type_signal_size_changed signal_size_changed();
        using type_signal_changed = sigc::signal<void(void)>;
        type_signal_changed signal_changed();

        PageView (Page page);
        ~PageView();
        CropLocation get_crop_location (int x, int y);
        void button_press (int x, int y);
        void motion (int x, int y);
        void button_release (int x, int y);
    /* It is necessary to ask the ruler color since it is themed with the GTK */
    /* theme foreground color, and this class doesn't have any GTK widget     */
    /* available to lookup the color. */
        void render (Cairo::RefPtr<Cairo::Context> context, Gdk::RGBA ruler_color);

        int get_width(void);
        void set_width(int);

        int get_height(void);
        void set_height(int);

    protected:
        type_signal_size_changed m_signal_size_changed;
        type_signal_changed m_signal_changed;

        sigc::connection connection_signal_page_pixels_changed;
        sigc::connection connection_signal_page_size_changed;
        sigc::connection connection_signal_page_crop_changed;
        sigc::connection connection_signal_page_line_changed;
        sigc::connection connection_signal_page_scan_direction_changed;
        sigc::connection connection_signal_page_new_buffer;

        sigc::connection animation_timeout_connection;
};


#endif //PAGE_VIEW_H