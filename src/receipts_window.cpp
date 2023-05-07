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

#include "receipts_window.h"
#include <stdexcept>

ReceiptsAppWindow::ReceiptsAppWindow(BaseObjectType* cobject,
  const Glib::RefPtr<Gtk::Builder>& refBuilder)
: Gtk::ApplicationWindow(cobject),
  m_refBuilder(refBuilder)
{
}

//static
ReceiptsAppWindow* ReceiptsAppWindow::create()
{
  // Load the Builder file and instantiate its widgets.
  auto refBuilder = Gtk::Builder::create_from_resource("/me/tiernan8r/receipts/ui/app_window.ui");

  auto window = Gtk::Builder::get_widget_derived<ReceiptsAppWindow>(refBuilder, "window1");
  if (!window)
    throw std::runtime_error("No \"window1\" object in app_window.ui");

  return window;
}

void ReceiptsAppWindow::open_file_view(const Glib::RefPtr<Gio::File>& /* file */)
{
}

