/* gtkmm example Copyright (C) 2016 gtkmm development team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#ifndef GTKMM_RECEIPTS_APPLICATION_H
#define GTKMM_RECEIPTS_APPLICATION_H

#include <gtkmm.h>

class ReceiptsAppWindow;

class ReceiptsApplication: public Gtk::Application
{
protected:
    ReceiptsApplication();

public:
    static Glib::RefPtr<ReceiptsApplication> create();

protected:
    // Override default signal handlers:
    void on_activate() override;
    void on_open(const Gio::Application::type_vec_files& files,
                 const Glib::ustring& hint) override;

private:
    ReceiptsAppWindow* create_appwindow();
    void on_hide_window(Gtk::Window* window);
};

#endif /* GTKMM_RECEIPTS_APPLICATION_H */
