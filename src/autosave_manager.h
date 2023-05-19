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
#include "page.h"

class AutosaveManager
{
    private:
        static std::string AUTOSAVE_DIR = Path.build_filename (Environment.get_user_cache_dir (), "receipts", "autosaves");
        static std::string AUTOSAVE_FILENAME = "autosave.book";
        static std::string AUTOSAVE_PATH = Path.build_filename (AUTOSAVE_DIR, AUTOSAVE_FILENAME);

        uint update_timeout = 0;

        std::map<Page, std::string> page_filenames;

        Book *book_ = NULL;

        std::string get_value (KeyFile file, std::string group_name, std::string key, std::string default = "");
        int get_integer (KeyFile file, std::string group_name, std::string key, int default = 0);
        bool get_boolean (KeyFile file, std::string group_name, std::string key, bool default = false);
        void save (bool do_timeout = true);
        void real_save (void);
        void save_pixels (Page page);

    public:
        Book book {
        get
        {
            return book_;
        }
        set
        {
            if (book_ != null)
            {
                for (var i = 0; i < book_.n_pages; i++)
                {
                    var page = book_.get_page (i);
                    on_page_removed (page);
                }
                book_.page_added.disconnect (on_page_added);
                book_.page_removed.disconnect (on_page_removed);
                book_.reordered.disconnect (on_changed);
                book_.cleared.disconnect (on_cleared);
            }
            book_ = value;
            book_.page_added.connect (on_page_added);
            book_.page_removed.connect (on_page_removed);
            book_.reordered.connect (on_changed);
            book_.cleared.connect (on_cleared);
            for (var i = 0; i < book_.n_pages; i++)
            {
                var page = book_.get_page (i);
                on_page_added (page);
            }
        }
    };

    AutosaveManager (void);
    bool exists (void);
    void load (void);
    void cleanup (void);
    void on_page_added (Page page);
    void on_page_removed (Page page);
    void on_scan_finished (Page page);
    void on_changed (void);
    void on_cleared (void);
};


#endif //AUTOSAVE_MANAGER_H