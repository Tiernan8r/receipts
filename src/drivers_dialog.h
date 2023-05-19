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
#ifndef DRIVERS_DIALOG_H
#define DRIVERS_DIALOG_H

#include <gtkmm.h>

class DriversDialog : private Gtk::Window
{
    private:
        Gtk::Revealer header_revealer;
        Gtk::Label main_label;
        Gtk::Label main_sublabel;
        Gtk::Revealer progress_revealer;
        Gtk::ProgressBar progress_bar;
        Gtk::Label result_label;
        Gtk::Label result_sublabel;
        Gtk::Image result_icon;
        Gtk::Stack stack;
    
        uint pulse_timer;
        std::string? missing_driver;
    
        void pulse_start ();
        void pulse_stop ();
#if HAVE_PACKAGEKIT
        std::async Pk.Results? install_packages (std::string[] packages, Pk.ProgressCallback progress_callback); throws GLib.Error
#endif

    public:
        DriversDialog (Gtk::Window parent, std::string *missing_driver);
        DriversDialog () override;
        std::async void open ();
};

#endif //DRIVERS_DIALOG_H