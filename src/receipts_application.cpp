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

#include "receipts_application.h"
#include "receipts_window.h"

ReceiptsApplication::ReceiptsApplication()
        : Gtk::Application("me.tiernan8r.receipts.application", Gio::Application::Flags::HANDLES_OPEN)
{
}

Glib::RefPtr<ReceiptsApplication> ReceiptsApplication::create()
{
    return Glib::make_refptr_for_instance<ReceiptsApplication>(new ReceiptsApplication());
}

ReceiptsAppWindow* ReceiptsApplication::create_appwindow()
{
    auto appwindow = ReceiptsAppWindow::create();

    // Make sure that the application runs for as long this window is still open.
    add_window(*appwindow);

    // A window can be added to an application with Gtk::Application::add_window()
    // or Gtk::Window::set_application(). When all added windows have been hidden
    // or removed, the application stops running (Gtk::Application::run() returns()),
    // unless Gio::Application::hold() has been called.

    // Delete the window when it is hidden.
    appwindow->signal_hide().connect(sigc::bind(sigc::mem_fun(*this,
                                                              &ReceiptsApplication::on_hide_window), appwindow));

    return appwindow;
}

void ReceiptsApplication::on_activate()
{
    // The application has been started, so let's show a window.
    auto appwindow = create_appwindow();
    appwindow->present();
}

void ReceiptsApplication::on_open(const Gio::Application::type_vec_files& files,
                                 const Glib::ustring& /* hint */)
{
    // The application has been asked to open some files,
    // so let's open a new view for each one.
    ReceiptsAppWindow* appwindow = nullptr;
    auto windows = get_windows();
    if (windows.size() > 0)
        appwindow = dynamic_cast<ReceiptsAppWindow*>(windows[0]);

    if (!appwindow)
        appwindow = create_appwindow();

    for (const auto& file : files)
        appwindow->open_file_view(file);

    appwindow->present();
}

void ReceiptsApplication::on_hide_window(Gtk::Window* window)
{
    delete window;
}
