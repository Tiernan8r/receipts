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

#include "preferences_dialog.h"
#include <libadwaitamm.h>

// [GtkTemplate (ui = "/me/tiernan8r/Receipts/ui/preferences-dialog.ui")]
void PreferencesDialog::toggle_postproc_visibility(bool enabled)
    {
        postproc_script_entry.get_parent().get_parent().get_parent().get_parent().set_visible(enabled);
        postproc_args_entry.get_parent().get_parent().get_parent().get_parent().set_visible(enabled);
        postproc_keep_original_switch.get_parent().get_parent().get_parent().get_parent().set_visible(enabled);
    };

void PreferencesDialog::set_page_side(ScanSide page_side)
    {
        switch (page_side)
        {
        case ScanSide.FRONT:
            front_side_button.active = true;
            break;
        case ScanSide.BACK:
            back_side_button.active = true;
            break;
        default:
        case ScanSide.BOTH:
            both_side_button.active = true;
            break;
        }
    };

void PreferencesDialog::set_dpi_combo(Gtk::ComboBox combo, int default_dpi, int current_dpi)
    {
        Gtk::CellRendererText renderer = new Gtk::CellRendererText();
        combo.pack_start(renderer, true);
        combo.add_attribute(renderer, "text", 1);

        Gtk::ListStore model = (Gtk::ListStore)combo.model;
        int scan_resolutions[] = {75, 150, 200, 300, 600, 1200, 2400};
        foreach (int dpi in scan_resolutions)
        {
            std::string label;
            if (dpi == default_dpi)
                /* Preferences dialog: Label for default resolution in resolution list */
                label = _("%d dpi (default)").printf(dpi);
            else if (dpi == 75)
                /* Preferences dialog: Label for minimum resolution in resolution list */
                label = _("%d dpi (draft)").printf(dpi);
            else if (dpi == 1200)
                /* Preferences dialog: Label for maximum resolution in resolution list */
                label = _("%d dpi (high resolution)").printf(dpi);
            else
                /* Preferences dialog: Label for resolution value in resolution list (dpi = dots per inch) */
                label = _("%d dpi").printf(dpi);

            Gtk::TreeIter iter;
            model.append(&iter);
            model.set(iter, 0, dpi, 1, label, -1);

            if (dpi == current_dpi)
                combo.set_active_iter(iter);
        }
    };

PreferencesDialog::PreferencesDialog(Settings settings)
    {
        this.settings = settings;

        Gtk::TreeIter iter;
        paper_size_model.append(&iter);
        paper_size_model.set(iter, 0, 0, 1, 0, 2,
                             /* Combo box value for automatic paper size */
                             _("Automatic"), -1);
        paper_size_model.append(&iter);
        paper_size_model.set(iter, 0, 1050, 1, 1480, 2, "A6", -1);
        paper_size_model.append(&iter);
        paper_size_model.set(iter, 0, 1480, 1, 2100, 2, "A5", -1);
        paper_size_model.append(&iter);
        paper_size_model.set(iter, 0, 2100, 1, 2970, 2, "A4", -1);
        paper_size_model.append(&iter);
        paper_size_model.set(iter, 0, 2970, 1, 4200, 2, "A3", -1);
        paper_size_model.append(&iter);
        paper_size_model.set(iter, 0, 2159, 1, 2794, 2, "Letter", -1);
        paper_size_model.append(&iter);
        paper_size_model.set(iter, 0, 2159, 1, 3556, 2, "Legal", -1);
        paper_size_model.append(&iter);
        paper_size_model.set(iter, 0, 1016, 1, 1524, 2, "4×6", -1);

        int dpi = settings.get_int("text-dpi");
        if (dpi <= 0)
            dpi = DEFAULT_TEXT_DPI;
        set_dpi_combo(text_dpi_combo, DEFAULT_TEXT_DPI, dpi);
        text_dpi_combo.changed.connect(() = > { settings.set_int("text-dpi", get_text_dpi()); });
        dpi = settings.get_int("photo-dpi");
        if (dpi <= 0)
            dpi = DEFAULT_PHOTO_DPI;
        set_dpi_combo(photo_dpi_combo, DEFAULT_PHOTO_DPI, dpi);
        photo_dpi_combo.changed.connect(() = > { settings.set_int("photo-dpi", get_photo_dpi()); });

        set_page_side((ScanSide)settings.get_enum("page-side"));
        front_side_button.toggled.connect((button) = > { if (button.active) settings.set_enum ("page-side", ScanSide.FRONT); });
        back_side_button.toggled.connect((button) = > { if (button.active) settings.set_enum ("page-side", ScanSide.BACK); });
        both_side_button.toggled.connect((button) = > { if (button.active) settings.set_enum ("page-side", ScanSide.BOTH); });

        Gtk::CellRendererText renderer = new Gtk::CellRendererText();
        paper_size_combo.pack_start(renderer, true);
        paper_size_combo.add_attribute(renderer, "text", 2);

        int lower = brightness_adjustment.lower;
        std::string darker_label = "<small>%s</small>".printf(_("Darker"));
        int upper = brightness_adjustment.upper;
        std::string lighter_label = "<small>%s</small>".printf(_("Lighter"));
        brightness_scale.add_mark(lower, Gtk::PositionType.BOTTOM, darker_label);
        brightness_scale.add_mark(0, Gtk::PositionType.BOTTOM, NULL);
        brightness_scale.add_mark(upper, Gtk::PositionType.BOTTOM, lighter_label);
        brightness_adjustment.value = settings.get_int("brightness");
        brightness_adjustment.value_changed.connect(() = > { settings.set_int("brightness", get_brightness()); });

        lower = contrast_adjustment.lower;
        std::string less_label = "<small>%s</small>".printf(_("Less"));
        upper = contrast_adjustment.upper;
        std::string more_label = "<small>%s</small>".printf(_("More"));
        contrast_scale.add_mark(lower, Gtk::PositionType.BOTTOM, less_label);
        contrast_scale.add_mark(0, Gtk::PositionType.BOTTOM, NULL);
        contrast_scale.add_mark(upper, Gtk::PositionType.BOTTOM, more_label);
        contrast_adjustment.value = settings.get_int("contrast");
        contrast_adjustment.value_changed.connect(() = > { settings.set_int("contrast", get_contrast()); });

        std::string minimum_size_label = "<small>%s</small>".printf(_("Minimum size"));
        compression_scale.add_mark(compression_adjustment.lower, Gtk::PositionType.BOTTOM, minimum_size_label);
        compression_scale.add_mark(75, Gtk::PositionType.BOTTOM, NULL);
        compression_scale.add_mark(90, Gtk::PositionType.BOTTOM, NULL);
        std::string full_detail_label = "<small>%s</small>".printf(_("Full detail"));
        compression_scale.add_mark(compression_adjustment.upper, Gtk::PositionType.BOTTOM, full_detail_label);
        compression_adjustment.value = settings.get_int("jpeg-quality");
        compression_adjustment.value_changed.connect(() = > { settings.set_int("jpeg-quality", (int)compression_adjustment.value); });

        int paper_width = settings.get_int("paper-width");
        int paper_height = settings.get_int("paper-height");
        set_paper_size(paper_width, paper_height);
        paper_size_combo.changed.connect(() = >
                                              {
                                                  int w, h;
                                                  get_paper_size(&w, &h);
                                                  settings.set_int("paper-width", w);
                                                  settings.set_int("paper-height", h);
                                              });

        set_page_delay(settings.get_int("page-delay"));
        page_delay_0s_button.toggled.connect((button) = > { if (button.active) settings.set_int ("page-delay", 0); });
        page_delay_3s_button.toggled.connect((button) = > { if (button.active) settings.set_int ("page-delay", 3000); });
        page_delay_6s_button.toggled.connect((button) = > { if (button.active) settings.set_int ("page-delay", 6000); });
        page_delay_10s_button.toggled.connect((button) = > { if (button.active) settings.set_int ("page-delay", 10000); });
        page_delay_15s_button.toggled.connect((button) = > { if (button.active) settings.set_int ("page-delay", 15000); });

        // Postprocessing settings
        bool postproc_enabled = settings.get_boolean("postproc-enabled");
        postproc_enable_switch.set_state(postproc_enabled);
        toggle_postproc_visibility(postproc_enabled);
        postproc_enable_switch.state_set.connect((is_active) = > {  toggle_postproc_visibility (is_active);
                                                                    settings.set_boolean("postproc-enabled", is_active);
                                                                    return true; });

        std::string postproc_script = settings.get_string("postproc-script");
        postproc_script_entry.set_text(postproc_script);
        postproc_script_entry.changed.connect(() = > { settings.set_string("postproc-script", postproc_script_entry.get_text()); });

        std::string postproc_arguments = settings.get_string("postproc-arguments");
        postproc_args_entry.set_text(postproc_arguments);
        postproc_args_entry.changed.connect(() = > { settings.set_string("postproc-arguments", postproc_args_entry.get_text()); });

        bool postproc_keep_original = settings.get_boolean("postproc-keep-original");
        postproc_keep_original_switch.set_state(postproc_keep_original);
        postproc_keep_original_switch.state_set.connect((is_active) = > {   settings.set_boolean("postproc-keep-original", is_active);
                                                                            return true; });
    };

ScanSide PreferencesDialog::get_page_side(void)
    {
        if (front_side_button.active)
            return ScanSide.FRONT;
        else if (back_side_button.active)
            return ScanSide.BACK;
        else
            return ScanSide.BOTH;
    };

void PreferencesDialog::set_paper_size(int width, int height)
    {
        Gtk::TreeIter iter;
        bool have_iter;

        for (have_iter = paper_size_model.get_iter_first(&iter);
             have_iter;
             have_iter = paper_size_model.iter_next(ref iter))
        {
            int w, h;
            paper_size_model.get(iter, 0, &w, 1, &h, -1);
            if (w == width && h == height)
                break;
        }

        if (!have_iter)
            have_iter = paper_size_model.get_iter_first(&iter);
        if (have_iter)
            paper_size_combo.set_active_iter(iter);
    };

int PreferencesDialog::get_text_dpi(void)
    {
        Gtk::TreeIter iter;
        int dpi = DEFAULT_TEXT_DPI;

        if (text_dpi_combo.get_active_iter(&iter))
            text_dpi_model.get(iter, 0, &dpi, -1);

        return dpi;
    };

int PreferencesDialog::get_photo_dpi(void)
    {
        Gtk::TreeIter iter;
        int dpi = DEFAULT_PHOTO_DPI;

        if (photo_dpi_combo.get_active_iter(&iter))
            photo_dpi_model.get(iter, 0, &dpi, -1);

        return dpi;
    };

bool PreferencesDialog::get_paper_size(int *width, int *height)
    {
        Gtk::TreeIter iter;

        width = height = 0;
        if (paper_size_combo.get_active_iter(&iter))
        {
            paper_size_model.get(iter, 0, ref width, 1, ref height, -1);
            return true;
        }

        return false;
    };

int PreferencesDialog::get_brightness(void)
    {
        return (int)brightness_adjustment.value;
    }

void PreferencesDialog::set_brightness(int brightness)
    {
        brightness_adjustment.value = brightness;
    };

int PreferencesDialog::get_contrast(void)
    {
        return (int)contrast_adjustment.value;
    };

void PreferencesDialog::set_contrast(int contrast)
    {
        contrast_adjustment.value = contrast;
    };

int PreferencesDialog::get_page_delay(void)
    {
        if (page_delay_15s_button.active)
            return 15000;
        else if (page_delay_10s_button.active)
            return 10000;
        else if (page_delay_6s_button.active)
            return 6000;
        else if (page_delay_3s_button.active)
            return 3000;
        else
            return 0;
    };

void PreferencesDialog::set_page_delay(int page_delay)
    {
        if (page_delay >= 15000)
            page_delay_15s_button.active = true;
        else if (page_delay >= 10000)
            page_delay_10s_button.active = true;
        else if (page_delay >= 6000)
            page_delay_6s_button.active = true;
        else if (page_delay >= 3000)
            page_delay_3s_button.active = true;
        else
            page_delay_0s_button.active = true;
    };
