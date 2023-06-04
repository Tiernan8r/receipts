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
#ifndef AUTOSAVE_MANAGER_H
#define AUTOSAVE_MANAGER_H

#include <string>
#include <map>
#include <glibmm-2.68/glibmm.h>
#include <sigc++-3.0/sigc++/sigc++.h>
#include "page.h"

class AutosaveManager
{
private:
    static std::string AUTOSAVE_DIR = Path.build_filename(Environment.get_user_cache_dir(), "receipts", "autosaves");
    static std::string AUTOSAVE_FILENAME = "autosave.book";
    static std::string AUTOSAVE_PATH = Path.build_filename(AUTOSAVE_DIR, AUTOSAVE_FILENAME);

    uint update_timeout = 0;

    std::map<Page, std::string> page_filenames;

    Book *book_ = NULL;
    sigc::connection page_added_connection;
    sigc::connection page_removed_connection;
    sigc::connection reordered_connection;
    sigc::connection cleared_connection;

    std::string get_value(Glib::KeyFile file, std::string group_name, std::string key, std::string& default);
    int get_integer(Glib::KeyFile file, std::string group_name, std::string key, int& default);
    bool get_boolean(Glib::KeyFile file, std::string group_name, std::string key, bool& default);
    void save(bool do_timeout = true);
    void real_save(void);
    void save_pixels(Page page);

public:
    Book* get_book(void);
    void set_book(Book *book);

    AutosaveManager(void);
    bool exists(void);
    void load(void);
    void cleanup(void);
    void on_page_added(Page page);
    void on_page_removed(Page page);
    void on_scan_finished(Page page);
    void on_changed(void);
    void on_cleared(void);
};

#endif // AUTOSAVE_MANAGER_H