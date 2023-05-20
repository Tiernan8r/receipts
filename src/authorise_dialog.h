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

#include <string.h>
#include <adwaita.h>
#include <gtkmm.h>
#include <sigc++/sigc++.h>


class AuthorizeDialog : private Gtk::Window
{
    private:
        AdwPreferencesGroup preferences_group;
        AdwEntryRow username_entry;
        AdwPasswordEntryRow password_entry;
        void authorize_button_cb (void);
        void cancel_button_cb (void);

    public:
        using type_signal_authorised = sigc::signal<void(AuthorisedDialogResponse)>;
        type_signal_authorised signal_authorised();

        void AuthorizeDialog (Gtk::Window parent, std::string title);
        std::string get_username (void);
        std::string get_password (void);

    
        std::async AuthorizeDialogResponse open(void);
    protected:
        type_signal_authorised m_signal_authorised;
}

struct AuthorizeDialogResponse {
    std::string username;
    std::string password;
    bool success;
    
    static AuthorizeDialogResponse new_canceled (void);
    static AuthorizeDialogResponse new_authorized (std::string username, std::string password);
}

#endif //AUTHORISE_DIALOG_H