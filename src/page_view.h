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

#include <cairo.h>
#include <string>
#include <sigc++/sigc++.h>

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
        PageViewTexture page_texture;

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
        Page page { get; private set; }

    /* Border around image */
        bool selected_ = false;
        bool selected
    {
        get { return selected_; }
        set
        {
            if ((this.selected && selected) || (!this.selected && !selected))
                return;
            this.selected = selected;
            changed ();
        }
    };
    /* Location to place this page */
        int x_offset { get; set; };
        int y_offset { get; set; };

    /* Cursor over this page */
        std::string cursor { get; private set; default = "arrow"; };

        using type_signal_size_changed = sigc::signal<void(void)>;
        type_signal_size_changed signal_size_changed();
        using type_signal_changed = sigc::signal<void(void)>;
        type_signal_changed signal_changed();

        void PageView (Page page);
        void PageView (void) override;
        CropLocation get_crop_location (int x, int y);
        void button_press (int x, int y);
        void motion (int x, int y);
        void button_release (int x, int y);
    /* It is necessary to ask the ruler color since it is themed with the GTK */
    /* theme foreground color, and this class doesn't have any GTK widget     */
    /* available to lookup the color. */
        void render (Cairo::Context context, Gdk.RGBA ruler_color);
        int width
    {
        get { return width_; }
        set
        {
            // FIXME: Automatically update when get updated image
            var h = (int) ((double) value * page.height / page.width);
            if (width_ == value && height_ == h)
                return;

            width_ = value;
            height_ = h;

            /* Regenerate image */
            update_image = true;

            size_changed ();
            changed ();
        }
    }

        int height
    {
        get { return height_; }
        set
        {
            // FIXME: Automatically update when get updated image
            var w = (int) ((double) value * page.width / page.height);
            if (width_ == w && height_ == value)
                return;

            width_ = w;
            height_ = value;

            /* Regenerate image */
            update_image = true;

            size_changed ();
            changed ();
        }
    };
    protected:
        type_signal_size_changed m_signal_size_changed;
        type_signal_changed m_signal_changed;
}


#endif //PAGE_VIEW_H