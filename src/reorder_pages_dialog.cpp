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
#include <gtkmm-4.0/gtkmm.h>
#include <gtkmm-4.0/gtkmm/shortcut.h>

// [GtkTemplate (ui = "/me/tiernan8r/Receipts/ui/reorder-pages-item.ui")]
std::string ReorderPagesItem::get_label(void)
{
    return title.get_label();
}

void ReorderPagesItem::set_label(std::string l)
{
    title.set_label(l);
}

std::string ReorderPagesItem::get_before(void)
{
    return before_image.get_icon_name();
}

void ReorderPagesItem::set_before(std::string b)
{
    before_image.icon_name = b;
}

std::string ReorderPagesItem::get_after(void)
{
    return after_image.get_icon_name();
}

void ReorderPagesItem::set_after(std::string after)
{
    after_image.set_from_icon_name(after);
}

// [GtkTemplate (ui = "/me/tiernan8r/Receipts/ui/reorder-pages-dialog.ui")]
ReorderPagesDialog::ReorderPagesDialog()
{
    Gtk::Shortcut::create();
    Gtk::ShortcutController::add_shortcut(shortcut);
    // add_binding_action(Gdk::Key::Escape, 0, "window.close", NULL);
};
