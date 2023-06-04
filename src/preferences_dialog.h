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
#ifndef PREFERENCES_DIALOG_H
#define PREFERENCES_DIALOG_H

#include <libadwaitamm.h>
#include <gtkmm-4.0/gtkmm.h>

class PreferencesDialog : private Adw::PreferencesWindow
{
    private:
        Gtk::Settings settings;

        Gtk::ComboBox text_dpi_combo;
        Gtk::ComboBox photo_dpi_combo;
        Gtk::ComboBox paper_size_combo;
        Gtk::Scale brightness_scale;
        Gtk::Scale contrast_scale;
        Gtk::Scale compression_scale;
        Gtk::ToggleButton page_delay_0s_button;
        Gtk::ToggleButton page_delay_3s_button;
        Gtk::ToggleButton page_delay_6s_button;
        Gtk::ToggleButton page_delay_10s_button;
        Gtk::ToggleButton page_delay_15s_button;
        Gtk::ListStore text_dpi_model;
        Gtk::ListStore photo_dpi_model;
        Gtk::ToggleButton front_side_button;
        Gtk::ToggleButton back_side_button;
        Gtk::ToggleButton both_side_button;
        Gtk::ListStore paper_size_model;
        Gtk::Adjustment brightness_adjustment;
        Gtk::Adjustment contrast_adjustment;
        Gtk::Adjustment compression_adjustment;
        Gtk::Switch postproc_enable_switch;
        Gtk::Entry postproc_script_entry;
        Gtk::Entry postproc_args_entry;
        Gtk::Switch postproc_keep_original_switch;

        void toggle_postproc_visibility(bool enabled);
        void set_page_side (ScanSide page_side);
        void set_dpi_combo (Gtk::ComboBox combo, int default_dpi, int current_dpi);

    public:
        void PreferencesDialog (Settings settings);
        ScanSide get_page_side (void);
        void set_paper_size (int width, int height);
        int get_text_dpi (void);
        int get_photo_dpi (void);
        bool get_paper_size (out int width, out int height);
        int get_brightness (void);
        void set_brightness (int brightness);
        int get_contrast (void);
        void set_contrast (int contrast);
        int get_page_delay (void);
        void set_page_delay (int page_delay);
};


#endif //PREFERENCES_DIALOG_H