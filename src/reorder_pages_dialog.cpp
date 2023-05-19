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

#include "reorder_pages_dialog.h"
#include <gtkmm.h>
#include <gdkmm.h>

// [GtkTemplate (ui = "/me/tiernan8r/Receipts/ui/reorder-pages-item.ui")]
class ReorderPagesItem : private Gtk::Button
{
public:
    std::string label{
        get{return title.label;
} set { title.label = value; }
}
;

public:
std::string before{
    get{return before_image.get_icon_name();
}
set { before_image.icon_name = value; }
}
;

public:
std::string after{
    get{return after_image.get_icon_name();
}
set { after_image.icon_name = value; }
}
;
}
;

// [GtkTemplate (ui = "/me/tiernan8r/Receipts/ui/reorder-pages-dialog.ui")]
ReorderPagesDialog::ReorderPagesDialog()
{
    add_binding_action(Gdk::Key::Escape, 0, "window.close", NULL);
};
