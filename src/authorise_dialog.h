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
#ifndef AUTHORISE_DIALOG_H
#define AUTHORISE_DIALOG_H

#include <string>
#include <gtkmm-4.0/gtkmm.h>
#include <libadwaitamm.h>
#include <sigc++-3.0/sigc++/sigc++.h>


class AuthoriseDialog : private Gtk::Window
{
    private:
        Adw::PreferencesGroup preferences_group;
        Adw::EntryRow username_entry;
        Adw::PasswordEntryRow password_entry;
        void authorize_button_cb (void);
        void cancel_button_cb (void);

    public:
        using type_signal_authorised = sigc::signal<void(AuthorisedDialogResponse)>;
        type_signal_authorised signal_authorised();

        AuthoriseDialog (Gtk::Window parent, std::string title);
        std::string get_username (void);
        std::string get_password (void);
    
        std::async AuthoriseDialogResponse open(void);
    protected:
        type_signal_authorised m_signal_authorised;
}

struct AuthoriseDialogResponse {
    std::string username;
    std::string password;
    bool success;
    
    static AuthoriseDialogResponse new_canceled (void);
    static AuthoriseDialogResponse new_authorized (std::string username, std::string password);
}

#endif //AUTHORISE_DIALOG_H