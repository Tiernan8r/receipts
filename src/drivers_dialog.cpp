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

#include <gtkmm-4.0/gtkmm.h>
#include "drivers_dialog.h"

// [GtkTemplate (ui = "/me/tiernan8r/Receipts/ui/drivers-dialog.ui")]
DriversDialog::DriversDialog (Gtk::Window parent, std::string *missing_drivers)
    {
        missing_driver = missing_drivers;
        set_transient_for (parent);
    };
    
DriversDialog::~DriversDialog () {
            pulse_stop ();
        };
    
void DriversDialog::pulse_start (void)
    {
        pulse_stop ();
        pulse_timer = GLib::Timeout.add(100, () => {
            progress_bar.pulse ();
            return Source.CONTINUE;
        });
    };

void DriversDialog::pulse_stop (void)
    {
        Source.remove (pulse_timer);
    };
    
void DriversDialog::open_async (void)
    {
        std::string message = "", instructions = "";
        std::string packages_to_install[] = {};
        switch (missing_driver)
        {
        case "brscan":
        case "brscan2":
        case "brscan3":
        case "brscan4":
            /* Message to indicate a Brother scanner has been detected */
            message = _("You appear to have a Brother scanner.");
            /* Instructions on how to install Brother scanner drivers */
            instructions = _("Drivers for this are available on the <a href=\"http://support.brother.com\">Brother website</a>.");
            break;
        case "pixma":
            /* Message to indicate a Canon Pixma scanner has been detected */
            message = _("You appear to have a Canon scanner, which is supported by the <a href=\"http://www.sane-project.org/man/sane-pixma.5.html\">Pixma SANE backend</a>.");
            /* Instructions on how to resolve issue with SANE scanner drivers */
            instructions = _("Please check if your <a href=\"http://www.sane-project.org/sane-supported-devices.html\">scanner is supported by SANE</a>, otherwise report the issue to the <a href=\"https://alioth-lists.debian.net/cgi-bin/mailman/listinfo/sane-devel\">SANE mailing list</a>.");
            break;
        case "samsung":
            /* Message to indicate a Samsung scanner has been detected */
            message = _("You appear to have a Samsung scanner.");
            /* Instructions on how to install Samsung scanner drivers.
               Because HP acquired Samsung's global printing business in 2017, the support is made on HP site. */
            instructions = _("Drivers for this are available on the <a href=\"https://support.hp.com\">HP website</a> (HP acquired Samsung's printing business).");
            break;
        case "hpaio":
        case "smfp":
            /* Message to indicate a HP scanner has been detected */
            message = _("You appear to have an HP scanner.");
            if (missing_driver == "hpaio")
                packages_to_install = { "libsane-hpaio" };
            else
                /* Instructions on how to install HP scanner drivers.
                   smfp is rebranded and slightly modified Samsung devices,
                   for example: HP Laser MFP 135a is rebranded Samsung Xpress SL-M2070.
                   It require custom drivers, not available in hpaio package */
                instructions = _("Drivers for this are available on the <a href=\"https://support.hp.com\">HP website</a>.");
            break;
        case "epkowa":
            /* Message to indicate an Epson scanner has been detected */
            message = _("You appear to have an Epson scanner.");
            /* Instructions on how to install Epson scanner drivers */
            instructions = _("Drivers for this are available on the <a href=\"http://support.epson.com\">Epson website</a>.");
            break;
        case "lexmark_nscan":
            /* Message to indicate a Lexmark scanner has been detected */
            message = _("You appear to have a Lexmark scanner.");
            /* Instructions on how to install Lexmark scanner drivers */
            instructions = _("Drivers for this are available on the <a href=\"http://support.lexmark.com\">Lexmark website</a>.");
            break;
        }

        main_label.label = message;
        main_sublabel.label = instructions;

        if (packages_to_install.length > 0)
        {
#if HAVE_PACKAGEKIT
            this.progress_revealer.reveal_child = true;
            pulse_start();

            main_sublabel.set_text (/* Label shown while installing drivers */
                                         _("Installing drivers…"));

            present ();

            /* Label shown once drivers successfully installed */
            std::string result_text = _("Drivers installed successfully!");
            bool success = true;
            try
            {
                Pk::Results *results = yield install_packages(packages_to_install, () => {});

                if (results.get_error_code () != NULL)
                {
                    int e = results.get_error_code ();
                    /* Label shown if failed to install drivers */
                    result_text = _("Failed to install drivers (error code %d).").printf (e.code);
                    success = false;
                }
            }
            catch (Error e)
            {
                /* Label shown if failed to install drivers */
                result_text = _("Failed to install drivers.");
                success = false;
                spdlog::warn ("Failed to install drivers: %s", e.message);
            }

            result_label.label = result_text;

            if (success)
            {
                result_sublabel.label = _("Once installed you will need to restart this app.");
                result_icon.icon_name = "emblem-ok-symbolic";
            }
            else
            {
                result_sublabel.visible = false;
                result_icon.icon_name = "emblem-important-symbolic";
            }
                
            stack.set_visible_child_name ("result");
            header_revealer.reveal_child = false;
            progress_revealer.reveal_child = false;
            pulse_stop ();
#else
            main_sublabel.set_text (/* Label shown to prompt user to install packages (when PackageKit not available) */
                                         ngettext ("You need to install the %s package.", "You need to install the %s packages.", packages_to_install.length).printf (std::string.joinv (", ", packages_to_install)));
            present ();
#endif
        }
    }

#if HAVE_PACKAGEKIT
Pk::Results* DriversDialog::install_packages_async (std::string packages[], Pk::ProgressCallback progress_callback) // throws GLib::Error
    {
        Pk::Task task = new Pk::Task ();
        Pk::results results;
        results = yield task.resolve_async (Pk::Filter.NOT_INSTALLED, packages, NULL, progress_callback);
        if (results == NULL || results.get_error_code () != NULL)
            return results;

        auto package_array = results.get_package_array ();
        std::string package_ids[] = new std::string[package_array.length + 1];
        package_ids[package_array.length] = NULL;
        for (int i = 0; i < package_array.length; i++)
            package_ids[i] = package_array.data[i].get_id ();

        return yield task.install_packages_async (package_ids, NULL, progress_callback);
    }
#endif
