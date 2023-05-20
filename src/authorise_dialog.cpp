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

#include "authorise_dialog.h"
#include <gtkmm.h>

// [GtkTemplate (ui = "/me/tiernan8r/Receipts/ui/authorize-dialog.ui")]
AuthoriseDialog::type_signal_authorised AuthoriseDialog::signal_authorised() {
    return m_signal_authorised;
}

AuthoriseDialog::AuthoriseDialog(Gtk::Window parent, std::string title)
{
    preferences_group.set_title(title);
    set_transient_for(parent);
};

std::string AuthorizeDialog::get_username(void)
{
    return username_entry.text;
};

std::string AuthorizeDialog::get_password(void)
{
    return password_entry.text;
};

async AuthorizeDialogResponse AuthorizeDialog::open(void)
{
    SourceFunc callback = open.callback;

    AuthorizeDialogResponse response = {};

    authorized.connect((res) = >
                               {
                                   response = res;
                                   callback();
                               });

    present();
    yield;
    close();

    return response;
};

// [GtkCallback]
void AuthorizeDialog::authorize_button_cb(void)
{
    authorized(AuthorizeDialogResponse.new_authorized(get_username(), get_password()));
};

// [GtkCallback]
void AuthorizeDialog::cancel_button_cb(void)
{
    authorized(AuthorizeDialogResponse.new_canceled());
};

AuthorizeDialogResponse AuthorizeDialogResponse::new_canceled(void)
{
    return AuthorizeDialogResponse(){
        success = false,
    };
};

AuthorizeDialogResponse AuthorizeDialogResponse::new_authorized(std::string username, std::string password)
{
    return AuthorizeDialogResponse(){
        username = username,
        password = password,
        success = true,
    };
};
