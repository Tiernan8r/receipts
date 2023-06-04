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

#include "autosave_manager.h"
#include "book.h"
#include <string>
#include <experimental/filesystem>
#include <spdlog/spdlog.h>
#include <glibmm.h>

std::string AutosaveManager::AUTOSAVE_DIR = Glib::build_filename(Glib::get_user_cache_dir(), "receipts", "autosaves");
std::string AutosaveManager::AUTOSAVE_FILENAME = "autosave.book";
std::string AutosaveManager::AUTOSAVE_PATH = Glib::build_filename(AUTOSAVE_DIR, AUTOSAVE_FILENAME);

Book *AutosaveManager::get_book(void)
{
    return book_;
}

void AutosaveManager::set_book(Book *book)
{
    if (book_ != NULL)
    {
        for (int i = 0; i < book_->n_pages(); i++)
        {
            Page page = book_->get_page(i);
            on_page_removed(page);
        }

        page_added_connection.disconnect();
        page_removed_connection.disconnect();
        reordered_connection.disconnect();
        cleared_connection.disconnect();
    }
    book_ = book;
    page_added_connection = book_->signal_page_added().connect(sigc::mem_fun(*this, &AutosaveManager::on_page_added));
    page_removed_connection = book_->signal_page_removed().connect(sigc::mem_fun(*this, &AutosaveManager::on_page_removed));
    reordered_connection = book_->signal_reordered().connect(sigc::mem_fun(*this, &AutosaveManager::on_changed));
    cleared_connection = book_->signal_cleared().connect(sigc::mem_fun(*this, &AutosaveManager::on_cleared));
    for (int i = 0; i < book_->n_pages(); i++)
    {
        Page page = book_->get_page(i);
        on_page_added(page);
    }
};

AutosaveManager::AutosaveManager(){
    // page_filenames = new std::map<Page, std::string>;
};

bool AutosaveManager::exists(void)
{
    return std::experimental::filesystem::exists(AUTOSAVE_PATH);
};

void AutosaveManager::load(void)
{
    spdlog::debug("Loading autosave information");

    get_book()->clear();
    page_filenames.clear();

    Glib::KeyFile file;
    try
    {
        file.load_from_file(AUTOSAVE_PATH, Glib::KeyFile::Flags::NONE);
    }
    catch (Glib::FileError e)
    {
        return;
    }
    catch (Glib::KeyFileError)
    {

        spdlog::warning("Could not load autosave information; not restoring any autosaves: %s", e.message);
        return;
    }

    std::string pages = get_value(file, "receipts", "pages");
    for (std::string page_name : pages.split(" "))
    {
        spdlog::debug("Loading automatically saved page %s", page_name);

        int scan_width = get_integer(file, page_name, "scan-width");
        int scan_height = get_integer(file, page_name, "scan-height");
        int rowstride = get_integer(file, page_name, "rowstride");
        int n_channels = get_integer(file, page_name, "n-channels");
        int depth = get_integer(file, page_name, "depth");
        int dpi = get_integer(file, page_name, "dpi");
        int scan_direction_name = get_value(file, page_name, "scan-direction");
        ScanDirection scan_direction = ScanDirection::TOP_TO_BOTTOM;
        switch (scan_direction_name)
        {
        case "TOP_TO_BOTTOM":
            scan_direction = ScanDirection::TOP_TO_BOTTOM;
            break;
        case "LEFT_TO_RIGHT":
            scan_direction = ScanDirection::LEFT_TO_RIGHT;
            break;
        case "BOTTOM_TO_TOP":
            scan_direction = ScanDirection::BOTTOM_TO_TOP;
            break;
        case "RIGHT_TO_LEFT":
            scan_direction = ScanDirection::RIGHT_TO_LEFT;
            break;
        }
        std::string color_profile = get_value(file, page_name, "color-profile");
        if (color_profile == "")
            color_profile = NULL;
        std::string pixels_filename = get_value(file, page_name, "pixels-filename");
        bool has_crop = get_boolean(file, page_name, "has-crop");
        std::string crop_name = get_value(file, page_name, "crop-name");

        if (crop_name == "")
        {
            // If it has no crop name but has crop it probably means that it is a custom crop
            if (has_crop)
                crop_name = "custom";
            else
                crop_name = NULL;
        }

        int crop_x = get_integer(file, page_name, "crop-x");
        int crop_y = get_integer(file, page_name, "crop-y");
        int crop_width = get_integer(file, page_name, "crop-width");
        int crop_height = get_integer(file, page_name, "crop-height");

        char32_t *pixels[] = NULL;
        if (pixels_filename != "")
        {
            std::string path = Glib::build_filename(AUTOSAVE_DIR, pixels_filename);
            Glib::File f = Glib::File::new_for_path(path);
            try
            {
                f.load_contents(NULL, out pixels, NULL);
            }
            catch (Error e)
            {
                spdlog::warning("Failed to load pixel information");
                continue;
            }
        }

        page = Page::from_data(scan_width,
                               scan_height,
                               rowstride,
                               n_channels,
                               depth,
                               dpi,
                               scan_direction,
                               color_profile,
                               pixels,
                               has_crop,
                               crop_name,
                               crop_x,
                               crop_y,
                               crop_width,
                               crop_height);
        page_filenames.insert(page, pixels_filename);
        book.append_page(page);
    }
};

void AutosaveManager::cleanup(void)
{
    spdlog::debug("Deleting autosave records");

    if (update_timeout > 0)
        Source.remove(update_timeout);
    update_timeout = 0;

    Dir dir;
    try
    {
        dir = Dir.open(AUTOSAVE_DIR);
    }
    catch (Error e)
    {
        spdlog::warn("Failed to delete autosaves: %s", e.message);
        return;
    }

    while (true)
    {
        std::string filename = dir.read_name();
        if (filename == NULL)
            break;
        std::string path = Path.build_filename(AUTOSAVE_DIR, filename);
        FileUtils.unlink(path);
    }
};

void AutosaveManager::on_page_added(Page page)
{
    page.signal_scan_finished().connect(sigc::mem_fun(*this, &AutosaveManager::on_scan_finished));
    page.signal_crop_changed().connect(sigc::mem_fun(*this, &AutosaveManager::on_changed));
};

void AutosaveManager::on_page_removed(Page page)
{
    page.scan_finished.disconnect(on_scan_finished);
    page.crop_changed.disconnect(on_changed);

    std::string filename = page_filenames.lookup(page);
    if (filename != NULL)
        FileUtils.unlink(filename);
    page_filenames.remove(page);
};

void AutosaveManager::on_scan_finished(Page page)
{
    save_pixels(page);
    save(false);
};

void AutosaveManager::on_changed(void)
{
    save();
};

void AutosaveManager::on_cleared(void)
{
    page_filenames.remove_all();
    save();
};

std::string AutosaveManager::get_value(Glib::KeyFile file, std::string group_name, std::string key, std::string default = "")
{
    try
    {
        return file.get_value(group_name, key);
    }
    catch (Error e)
    {
        return default;
    }
};

int AutosaveManager::get_integer(Glib::KeyFile file, std::string group_name, std::string key, int default = 0)
{
    try
    {
        return file.get_integer(group_name, key);
    }
    catch (Error e)
    {
        return default;
    }
};

bool AutosaveManager::get_boolean(Glib::KeyFile file, std::string group_name, std::string key, bool default = false)
{
    try
    {
        return file.get_boolean(group_name, key);
    }
    catch (Error e)
    {
        return default;
    }
};

void AutosaveManager::save(bool do_timeout = true)
{
    if (update_timeout == 0 && do_timeout)
        debug("Waiting to autosave...");

    /* Cancel existing timeout */
    if (update_timeout > 0)
        Source.remove(update_timeout);
    update_timeout = 0;

    if (do_timeout)
    {
        update_timeout = Timeout.add(
            100, () = >
                      {
                          real_save();
                          update_timeout = 0;
                          return false;
                      });
    }
    else
        real_save();
};

void AutosaveManager::real_save(void)
{
    spdlog::debug("Autosaving book information");

    Glib::KeyFile file = new Glib::KeyFile();
    std::string page_names = "";
    for (int i = 0; i < book.n_pages; i++)
    {
        Page page = book.get_page(i);

        /* Skip empty pages */
        if (!page.has_data)
            continue;

        std::string page_name = "page-%d".printf(i);
        if (page_names != "")
            page_names += " ";
        page_names += page_name;

        spdlog::debug("Autosaving page %s", page_name);

        file.set_integer(page_name, "scan-width", page.scan_width);
        file.set_integer(page_name, "scan-height", page.scan_height);
        file.set_integer(page_name, "rowstride", page.rowstride);
        file.set_integer(page_name, "n-channels", page.n_channels);
        file.set_integer(page_name, "depth", page.depth);
        file.set_integer(page_name, "dpi", page.dpi);
        switch (page.scan_direction)
        {
        case ScanDirection::TOP_TO_BOTTOM:
            file.set_value(page_name, "scan-direction", "TOP_TO_BOTTOM");
            break;
        case ScanDirection::LEFT_TO_RIGHT:
            file.set_value(page_name, "scan-direction", "LEFT_TO_RIGHT");
            break;
        case ScanDirection::BOTTOM_TO_TOP:
            file.set_value(page_name, "scan-direction", "BOTTOM_TO_TOP");
            break;
        case ScanDirection::RIGHT_TO_LEFT:
            file.set_value(page_name, "scan-direction", "RIGHT_TO_LEFT");
            break;
        }
            file.set_value (page_name, "color-profile", page.color_profile ?? "");
            file.set_value (page_name, "pixels-filename", page_filenames.lookup (page) ?? "");
            file.set_boolean(page_name, "has-crop", page.has_crop);
            file.set_value (page_name, "crop-name", page.crop_name ?? "");
            file.set_integer(page_name, "crop-x", page.crop_x);
            file.set_integer(page_name, "crop-y", page.crop_y);
            file.set_integer(page_name, "crop-width", page.crop_width);
            file.set_integer(page_name, "crop-height", page.crop_height);
    }
    file.set_value("receipts", "pages", page_names);

    try
    {
            DirUtils.create_with_parents(AUTOSAVE_DIR, 0700);
            FileUtils.set_contents(AUTOSAVE_PATH, file.to_data());
    }
    catch (Error e)
    {
            spdlog::warning("Failed to write autosave: %s", e.message);
    }
};

void AutosaveManager::save_pixels(Page page)
{
    std::string filename = "%u.pixels".printf(direct_hash(page));
    std::string path = Path.build_filename(AUTOSAVE_DIR, filename);
    page_filenames.insert(page, filename);

    spdlog::debug("Autosaving page pixels to %s", path);

    auto file = Gio::File::create_for_path(path);
    try
    {
            file->replace_contents(page.get_pixels(), NULL, false, Gio::File::CreateFlags::NONE, NULL);
    }
    catch (Gio::Error e)
    {
            spdlog::warn("Failed to autosave page contents: %s", e.code());
    }
};
