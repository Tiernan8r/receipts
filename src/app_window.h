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
#ifndef APP_WINDOW_H
#define APP_WINDOW_H

#include <gtkmm-4.0/gtkmm.h>
#include <glibmm-2.68/glibmm.h>
#include <libadwaitamm.h>
#include <sigc++-3.0/sigc++/sigc++.h>
#include "scanner.h"
#include "preferences_dialog.h"
#include "autosave_manager.h"
#include "book_view.h"

const int DEFAULT_TEXT_DPI = 150;
const int DEFAULT_PHOTO_DPI = 300;

class AppWindow : public Adw::ApplicationWindow
{
private:
    const GActionEntry action_entries[] =
        {
            {"new_document", new_document_cb},
            {"scan_type", scan_type_action_cb, "s", "'single'"},
            {"document_hint", document_hint_action_cb, "s", "'text'"},
            {"scan_single", scan_single_cb},
            {"scan_adf", scan_adf_cb},
            {"scan_batch", scan_batch_cb},
            {"scan_stop", scan_stop_cb},
            {"rotate_left", rotate_left_cb},
            {"rotate_right", rotate_right_cb},
            {"move_left", move_left_cb},
            {"move_right", move_right_cb},
            {"copy_page", copy_page_cb},
            {"delete_page", delete_page_cb},
            {"reorder", reorder_document_cb},
            {"save", save_document_cb},
            {"email", email_document_cb},
            {"print", print_document_cb},
            {"preferences", preferences_cb},
            {"help", help_cb},
            {"about", about_cb},
            {"quit", quit_cb}};

    Gio::SimpleAction scan_type_action;
    Gio::SimpleAction document_hint_action;

    Gio::SimpleAction delete_page_action;
    Gio::SimpleAction page_move_left_action;
    Gio::SimpleAction page_move_right_action;
    Gio::SimpleAction copy_to_clipboard_action;

    CropActions crop_actions;

    Gio::Settings settings;
    ScanType scan_type = ScanType.SINGLE;

    PreferencesDialog preferences_dialog;

    bool setting_devices;
    bool user_selected_device;

    Gtk::PopoverMenu page_menu;
    Gtk::Stack stack;
    Adw::StatusPage status_page;
    Gtk::Label status_secondary_label;
    Gtk::ListStore device_model;
    Gtk::Box device_buttons_box;
    Gtk::ComboBox device_combo;
    Gtk::Box main_vbox;
    Gtk::Button save_button;
    Gtk::Button stop_button;
    Gtk::Button scan_button;
    Gtk::ActionBar action_bar;
    Gtk::ToggleButton crop_button;

    Adw::ButtonContent scan_button_content;

    Gtk::MenuButton menu_button;

    bool have_devices = false;
    std::string *missing_driver = NULL;

    bool book_needs_saving;
    std::string book_uri = NULL;

    AutosaveManager autosave_manager;

    BookView book_view;
    bool updating_page_menu;

    std::string document_hint = "photo";

    bool _scanning = false;

    int window_width;
    int window_height;
    bool window_is_maximized;
    bool window_is_fullscreen;

    uint save_state_timeout;

    Book _book;

    void update_scan_status(void);
    bool prompt_to_load_autosaved_book_async(void);
    std::string get_selected_device(void);
    std::string get_selected_device_label(void);
    bool find_scan_device(std::string device, GtkTreeIter *iter);
    std::string choose_file_location_async(void);
    bool check_overwrite_async(Gtk::Window parent, std::list<File> files);
    std::string mime_type_to_extension(std::string mime_type);
    std::string extension_to_mime_type(std::string extension);
    std::string uri_extension(std::string uri);
    std::string uri_to_mime_type(std::string uri);
    bool save_document_async(void);
    bool prompt_to_save_async(std::string title, std::string discard_label);
    void clear_document(void);
    void new_document(void);
    bool status_label_activate_link_cb(Gtk::Label label, std::string uri);
    void new_document_cb(void);
    void crop_toggle_cb(Gtk::ToggleButton btn);
    void redetect_button_clicked_cb(Gtk::Button button);
    void scan(ScanOptions options);
    void scan_type_action_cb(Gio::SimpleAction action, Variant *value);
    void document_hint_action_cb(Gio::SimpleAction action, Variant *value);
    void scan_single_cb(void);
    void scan_adf_cb(void);
    void scan_batch_cb(void);
    void scan_stop_cb(void);
    void rotate_left_cb(void);
    void rotate_right_cb(void);
    void move_left_cb(void);
    void move_right_cb(void);
    void copy_page_cb(void);
    void delete_page_cb(void);
    void set_scan_type(ScanType scan_type);
    void set_document_hint(std::string document_hint, bool save = false);
    ScanOptions make_scan_options(void);
    void device_combo_changed_cb(Gtk::Widget widget);
    void scan_button_clicked_cb(Gtk::Widget widget);
    void stop_scan_button_clicked_cb(Gtk::Widget widget);
    void preferences_cb(void);
    void update_page_menu(void);
    void page_selected_cb(BookView view, Page *page);
    void show_page_cb(BookView view, Page page);
    void show_page_menu_cb(BookView view, Gtk::Widget from, double x, double y);
    void set_crop(std::string *crop_name);
    void reorder_document_cb(void);
    void draw_page(Gtk::PrintOperation operation, Gtk::PrintContext print_context, int page_number);
    void email_document_cb(void);
    void email_document_async(void);
    void print_document(void);
    void print_document_cb(void);
    void launch_help(void);
    void help_cb(void);
    void show_about(void);
    void about_cb(void);
    void on_quit(void);
    void quit_cb(void);
    bool window_close_request_cb(Gtk::Window window);
    void page_added_cb(Book book, Page page);
    void reordered_cb(Book book);
    void page_removed_cb(Book book, Page page);
    void book_changed_cb(Book book);
    void load(void);
    std::string _state_filename
    std::string state_filename(void);
    void load_state(void);
    std::string state_get_string(KeyFile f, std::string group_name, std::string key, std::string default = "");
    int state_get_integer(KeyFile f, std::string group_name, std::string key, int default = 0);
    bool state_get_boolean(KeyFile f, std::string group_name, std::string key, bool default = false);
    static std::string STATE_DIR = Path.build_filename(Environment.get_user_config_dir(), "receipts", NULL);
    void save_state(bool force = false);

public:
    Book book(void);
    Page get_selected_page(void);
    void set_selected_page(Page);

    bool get_scanning(void);
    void set_scanning(bool);

    int get_brightness(void);
    void set_brightness(int);

    int get_contrast(void);
    void set_contrast(int);

    int get_page_delay(void);
    void set_page_delay(int);

    using type_signal_start_scan = sigc::signal<void(std::string *, ScanOptions)>;
    type_signal_start_scan signal_start_scan();
    using type_signal_stop_scan = sigc::signal<void(void)>;
    type_signal_stop_scan signal_stop_scan();
    using type_signal_redect = sigc::signal<void(void)>;
    type_signal_redect signal_redetect();
    void AppWindow(void) void AppWindow(void) override;
    void show_error_dialog(std::string error_title, std::string error_text);
    AuthoriseDialogResponse authorise_async(std::string resource);
    void set_scan_devices(std::list<ScanDevice> devices, std::string *missing_driver);
    void set_selected_device(std::string device);
    void crop_set_action_cb(SimpleAction action, Variant *value);
    void crop_rotate_action_cb(void);
    void save_document_cb(void);
    void size_allocate(int width, int height, int baseline) override;
    void unmap(void) override;
    void start(void);

protected:
    type_signal_start_scan m_signal_start_scan;
    type_signal_stop_scan m_signal_stop_scan;
    type_signal_redect m_signal_redect;
};

class CancellableProgressBar : private Gtk::Box
{
private:
    Gtk::ProgressBar bar;
    Gtk::Button *button;

public:
    CancellableProgressBar(std::string *text, Cancellable *cancellable);
    void set_fraction(double fraction);
    void remove_with_delay(uint delay, Gtk::ActionBar parent);
};

class CropActions
{
private:
    Gio::SimpleActionGroup group;
    Gio::SimpleAction crop_set;
    Gio::SimpleAction crop_rotate;
    Gio::ActionEntry crop_entries[] =
        {
            {"set", AppWindow::crop_set_action_cb, "s", "'none'"},
            {"rotate", AppWindow::crop_rotate_action_cb},
    };

public:
    CropActions(AppWindow window);
    void update_current_crop(std::string *crop_name);
};

#endif // APP_WINDOW_H