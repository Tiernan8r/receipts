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

#include <gtkmm.h>
#include <string>

class ReorderPagesItem : private Gtk::Button
{
    private:
        Gtk::Label title;
        Gtk::Image before_image;
        Gtk::Image after_image;

    public:
        std::string label
    {
        get { return title.label; }
        set { title.label = value; }
    };

        std::string before
    {
        get { return before_image.get_icon_name (); }
        set { before_image.icon_name = value; }
    };

        std::string after
    {
        get { return after_image.get_icon_name (); }
        set { after_image.icon_name = value; }
    };
};


class ReorderPagesDialog : private Gtk::Window
{
    public:
        ReorderPagesItem combine_sides;
        ReorderPagesItem combine_sides_rev;
        ReorderPagesItem flip_odd;
        ReorderPagesItem flip_even;
        ReorderPagesItem reverse;
    
        void ReorderPagesDialog (void);
};


#endif //REORDER_PAGES_DIALOG