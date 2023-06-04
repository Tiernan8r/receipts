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
#ifndef REORDER_PAGES_DIALOG
#define REORDER_PAGES_DIALOG

#include <gtkmm-4.0/gtkmm.h>
#include <string>

class ReorderPagesItem : private Gtk::Button
{
    private:
        Gtk::Label title;
        Gtk::Image before_image;
        Gtk::Image after_image;

    public:
        std::string get_label(void);
        void set_label(std::string);

        std::string get_before(void);
        void set_before(std::string);

        std::string get_after(void);
        void set_after(std::string);
};


class ReorderPagesDialog : private Gtk::Window
{
    public:
        ReorderPagesItem combine_sides;
        ReorderPagesItem combine_sides_rev;
        ReorderPagesItem flip_odd;
        ReorderPagesItem flip_even;
        ReorderPagesItem reverse;
    
        ReorderPagesDialog (void);
};


#endif //REORDER_PAGES_DIALOG