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
#ifndef BOOK_VIEW_H
#define BOOK_VIEW_H

#include <gtkmm.h>
#include <string>
#include <map>
#include <cairo.h>
#include "book.h"
#include "page.h"
#include "page_view.h"

class BookView : public Gtk::Box
{
    private:
        std::map<Page, PageView> page_data;

    /* True if the view needs to be laid out again */
        bool need_layout;
        bool laying_out;
        bool show_selected_page;
    
    /* Currently selected page */
        PageView *selected_page_view = NULL;

        /* Widget being rendered to */
        Gtk::DrawingArea drawing_area;

        /* Horizontal scrollbar */
        Gtk::Scrollbar scroll;
        Gtk::Adjustment adjustment;

        std::string cursor;
        
        Gtk::EventControllerMotion motion_controller;
        Gtk::EventControllerKey key_controller; 
        Gtk::GestureClick primary_click_gesture; 
        Gtk::GestureClick secondary_click_gesture; 
        Gtk::EventControllerFocus focus_controller;


        PageView get_nth_page (int n);
        PageView get_next_page (PageView page);
        PageView get_prev_page (PageView page);
        void page_view_changed_cb (PageView page);
        void page_view_size_changed_cb (PageView page);
        void add_cb (Book book, Page page);
        void set_selected_page_view (PageView? page);
        void show_page_view (PageView *page);
        void select_page_view (PageView *page);
        void remove_cb (Book book, Page page);
        void reorder_cb (Book book);
        void clear_cb (Book book);
        void layout_into (int width, int height, out int book_width, out int book_height);
        void layout (void);
        PageView *get_page_at (int x, int y, int *x_, int *y_);
        void primary_pressed_cb (Gtk::GestureClick controler, int n_press, double x, double y);
        void primary_released_cb (Gtk::GestureClick controler, int n_press, double x, double y);
        void secondary_pressed_cb (Gtk::GestureClick controler, int n_press, double x, double y);
        void secondary_released_cb (Gtk::GestureClick controler, int n_press, double x, double y);
        void button_cb (Gtk::GestureClick controler, int button, bool press, int n_press, double dx, double dy);
        void set_cursor (std::string cursor);
        void motion_cb (Gtk::EventControllerMotion controler, double dx, double dy);
        bool key_cb (Gtk::EventControllerKey controler, uint keyval, uint keycode, Gdk::ModifierType state);
        void focus_cb (Gtk::EventControllerFocus controler);
        void scroll_cb (Gtk::Adjustment adjustment);

    public:
    /* Book being rendered */
        Book book { get;     set; }
        Page *selected_page
    {
        get
        {
            if (selected_page_view != null)
                return selected_page_view.page;
            else
                return null;
        }
        set
        {
            if (selected_page == value)
                return;

            if (value != null)
                select_page_view (page_data.lookup (value));
            else
                select_page_view (null);
        }
    }


    signal void page_selected (Page *page);
    signal void show_page (Page page);
    signal void show_menu (Gtk::Widget from, double x, double y);

    int x_offset
    {
        get
        {
            return (int) adjustment.get_value ();
        }
        set
        {
            adjustment.value = value;
        }
    }

    BookView (Book book);
    BookView (void) override;
    void drawing_area_resize_cb (void);
    void draw_cb (Gtk::DrawingArea drawing_area, Cairo::Context context, int width, int height);
    void redraw (void);
    void select_next_page (void);
    void select_prev_page (void);
}



#endif //BOOK_VIEW_H