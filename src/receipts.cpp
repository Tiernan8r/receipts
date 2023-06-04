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

#include "receipts.h"
#include <libadwaitamm.h>
#include <string>
#include <gusb-1/gusb.h>

Receipts::Receipts(ScanDevice *device = NULL) : register_session(true), default_device(device)
{
    /* The inhibit () method use this */
    Object(application_id
           : "me.tiernan8r.Receipts");
    register_session = true;

    default_device = device;
};

void Receipts::startup(void) override
{
    base.startup();

    app = new AppWindow();
    book = app.book;
    app.signal_start_scan().connect(scan_cb);
    app.signal_stop_scan().connect(cancel_cb);
    app.signal_redetect().connect(redetect_cb);

    scanner = Scanner.get_instance();
    scanner.signal_update_devices().connect(update_scan_devices_cb);
    scanner.signal_request_authorization().connect(authorize_cb);
    scanner.signal_expect_page().connect(scanner_new_page_cb);
    scanner.signal_got_page_info().connect(scanner_page_info_cb);
    scanner.signal_got_line().connect(scanner_line_cb);
    scanner.signal_page_done().connect(scanner_page_done_cb);
    scanner.signal_document_done().connect(scanner_document_done_cb);
    scanner.signal_scan_failed().connect(scanner_failed_cb);
    scanner.signal_scanning_changed().connect(scanner_scanning_changed_cb);

    try
    {
        usb_context = new GUsb::Context();
        usb_context.signal_device_added().connect(() = > { scanner.redetect(); });
        usb_context.signal_device_removed().connect(() = > { scanner.redetect(); });
    }
    catch (std::exception e)
    {
        spdlog::warn("Failed to create USB context: {}\n", e);
    }

    if (default_device != NULL)
    {
        std::list<ScanDevice> device_list = NULL;

        device_list.append(default_device);
        app.set_scan_devices(device_list);
        app.set_selected_device(default_device.name);
    }
}

void Receipts::activate(void) override
{
    base.activate();
    app.start();
    scanner.start();
};

void Receipts::shutdown(void) override
{
    base.shutdown();
    book = NULL;
    app = NULL;
    usb_context = NULL;
    scanner.free();
};

void Receipts::update_scan_devices_cb(Scanner scanner, std::list<ScanDevice> devices)
{
    // TODO
    auto devices_copy = devices.copy_deep((CopyFunc)Object.ref);

    /* If the default device is not detected add it to the list */
    if (default_device != NULL)
    {
        bool default_in_list = false;
        for (auto device : devices_copy)
        {
            if (device.name == default_device.name)
            {
                default_in_list = true;
                break;
            }
        }

        if (!default_in_list)
            devices_copy.prepend(default_device);
    }

    have_devices = devices_copy.length() > 0;

    /* If SANE doesn't see anything, see if we recognise any of the USB devices */
    std::string *missing_driver = NULL;
    if (!have_devices) {
        missing_driver = suggest_driver();
    }

    app.set_scan_devices(devices_copy, missing_driver);
};

void Receipts::add_devices(std::map<uint32, std::string> map, uint32 devices[], std::string driver)
{
    for (int i = 0; i < devices.length; i++) {
        map.insert(devices[i], driver);
    }
}

void Receipts::authorize_cb(Scanner scanner, std::string resource)
{
    app.authorize.begin(
        resource, (obj, res) = >
                               {
                                   auto data = app.authorize.end(res);
                                   if (data.success)
                                   {
                                       scanner.authorize(data.username, data.password);
                                   }
                               });
}

Page Receipts::append_page(int width = 100, int height = 100, int dpi = 100)
{
    /* Use current page if not used */
    Page page = book.get_page(-1);
    if (page != NULL && !page.has_data)
    {
        app.selected_page = page;
        page.start();
        return page;
    }

    /* Copy info from previous page */
    ScanDirection scan_direction = ScanDirection::TOP_TO_BOTTOM;
    bool do_crop = false;
    std::string named_crop = NULL;
    int cx = 0, cy = 0, cw = 0, ch = 0;
    if (page != NULL)
    {
        scan_direction = page.scan_direction;
        width = page.width;
        height = page.height;
        dpi = page.dpi;

        do_crop = page.has_crop;
        if (do_crop)
        {
            named_crop = page.crop_name;
            cx = page.crop_x;
            cy = page.crop_y;
            cw = page.crop_width;
            ch = page.crop_height;
        }
    }

    page = new Page(width, height, dpi, scan_direction);
    book.append_page(page);
    if (do_crop)
    {
        if (named_crop != NULL)
        {
            page.set_named_crop(named_crop);
        }
        else
            page.set_custom_crop(cw, ch);
        page.move_crop(cx, cy);
    }
    app.selected_page = page;
    page.start();

    return page;
}

void Receipts::scanner_new_page_cb(Scanner scanner)
{
    append_page();
};

std::string *Receipts::get_profile_for_device(std::string device_name)
{
#if HAVE_COLORD
    std::string device_id = "sane:%s".printf(device_name);
    spdlog::debug("Getting color profile for device %s", device_name);

    Cd::Client client = new Cd::Client();
    try
    {
        client.connect_sync();
    }
    catch (std::exception e)
    {
        spdlog::debug("Failed to connect to colord: {}", e);
        return NULL;
    }

    Cd::Device device;
    try
    {
        device = client.find_device_by_property_sync(Cd::DEVICE_PROPERTY_SERIAL, device_id);
    }
    catch (std::exception e)
    {
        spdlog::debug("Unable to find colord device {}: {}", device_name, e);
        return NULL;
    }

    try
    {
        device.connect_sync();
    }
    catch (std::exception e)
    {
        spdlog::debug("Failed to get properties from the device {}: {}", device_name, e.message);
        return NULL;
    }

    std::string *profile = device.get_default_profile();
    if (profile == NULL)
    {
        spdlog::debug("No default color profile for device: {}", device_name);
        return NULL;
    }

    try
    {
        profile.connect_sync();
    }
    catch (std::exception e)
    {
        spdlog::debug("Failed to get properties from the profile {}: {}", device_name, e);
        return NULL;
    }

    if (profile.filename == NULL)
    {
        spdlog::debug("No icc color profile for the device {}", device_name);
        return NULL;
    }

    spdlog::debug("Using color profile {} for device {}", profile.filename, device_name);
    return profile.filename;
#else
    return NULL;
#endif
}

void Receipts::scanner_page_info_cb(Scanner scanner, ScanPageInfo info)
{
    spdlog::debug("Page is {d} pixels wide, {d} pixels high, {d} bits per pixel",
                  info.width, info.height, info.depth);

    /* Add a new page */
    scanned_page = this.append_page();
    scanned_page.set_page_info(info);

    /* Get ICC color profile */
    /* FIXME: The ICC profile could change */
    /* FIXME: Don't do a D-bus call for each page, cache color profiles */
    scanned_page.color_profile = get_profile_for_device(info.device);
}

void Receipts::scanner_line_cb(Scanner scanner, ScanLine line)
{
    scanned_page.parse_scan_line(line);
}

void Receipts::scanner_page_done_cb(Scanner scanner)
{
    scanned_page.finish();
    scanned_page = NULL;
}

void Receipts::remove_empty_page(void)
{
    Page page = book.get_page((int)book.n_pages - 1);
    if (!page.has_data)
        book.delete_page(page);
}

void Receipts::scanner_document_done_cb(Scanner scanner)
{
    remove_empty_page();
}

void Receipts::scanner_failed_cb(Scanner scanner, int error_code, std::string error_string)
{
    remove_empty_page();
    scanned_page = NULL;
    if (error_code != Sane.Status.CANCELLED)
    {
        app.show_error_dialog(/* Title of error dialog when scan failed */
                              _("Failed to scan"),
                              error_string);
    }
}

void Receipts::scanner_scanning_changed_cb(Scanner scanner)
{
    bool is_scanning = scanner.is_scanning();

    if (is_scanning)
    {
        /* Attempt to inhibit the screensaver when scanning */
        std::string reason = _("Scan in progress");

        /* This should work on Gnome, Budgie, Cinnamon, Mate, Unity, ...
         * but will not work on KDE, LXDE, XFCE, ... */
        inhibit_cookie = inhibit(app, Gtk::ApplicationInhibitFlags::IDLE, reason);

        if (inhibit_cookie == 0)
        {
            /* If the previous method didn't work, try the one
             * provided by Freedesktop. It should work with KDE,
             * LXDE, XFCE, and maybe others as well. */
            try
            {
                if ((fdss = FreedesktopScreensaver.get_proxy()) != NULL)
                {
                    inhibit_cookie = fdss.inhibit("receipts", reason);
                }
            }
            catch (Error error)
            {
            }
        }
    }
    else
    {
        /* When finished scanning, uninhibit if inhibit was working */
        if (inhibit_cookie != 0)
        {
            if (fdss == NULL)
                uninhibit(inhibit_cookie);
            else
            {
                try
                {
                    fdss.uninhibit(inhibit_cookie);
                }
                catch (Error error)
                {
                }
                fdss = NULL;
            }

            inhibit_cookie = 0;
        }
    }

    app.scanning = is_scanning;
}

void Receipts::scan_cb(AppWindow ui, std::string *device, ScanOptions options)
{
    spdlog::debug("Requesting scan at %d dpi from device '%s'", options.dpi, device);

    if (!scanner.is_scanning())
        // We need to add +1 to avoid visual glitches, fixes: #179
        append_page(options.paper_width + 1, options.paper_height + 1, options.dpi + 1);

    scanner.scan(device, options);
}

void Receipts::cancel_cb(AppWindow ui)
{
    scanner.cancel();
}

void Receipts::redetect_cb(AppWindow ui)
{
    scanner.redetect();
}

void Receipts::log_cb(std::string *log_domain, LogLevelFlags log_level, std::string message)
{
    std::string prefix;

    switch (log_level & LogLevelFlags.LEVEL_MASK)
    {
    case LogLevelFlags.LEVEL_ERROR:
        prefix = "ERROR:";
        break;
    case LogLevelFlags.LEVEL_CRITICAL:
        prefix = "CRITICAL:";
        break;
    case LogLevelFlags.LEVEL_WARNING:
        prefix = "WARNING:";
        break;
    case LogLevelFlags.LEVEL_MESSAGE:
        prefix = "MESSAGE:";
        break;
    case LogLevelFlags.LEVEL_INFO:
        prefix = "INFO:";
        break;
    case LogLevelFlags.LEVEL_DEBUG:
        prefix = "spdlog::debug:";
        break;
    default:
        prefix = "LOG:";
        break;
    }

    log_file.printf("[%+.2fs] %s %s\n", log_timer.elapsed(), prefix, message);
    if (debug_enabled)
        stderr.printf("[%+.2fs] %s %s\n", log_timer.elapsed(), prefix, message);
}

void Receipts::fix_pdf(std::string filename) // throws Error
{
    uint8[] data;
    FileUtils.get_data(filename, out data);

    var fixed_file = FileStream.open(filename + ".fixed", "w");

    int offset = 0;
    int line_number = 0;
    int xref_offset = 0;
    int xref_line = -1;
    int startxref_line = -1;
    int fixed_size = -1;
    StringBuilder line = new StringBuilder();
    while (offset < data.length)
    {
        int end_offset = offset;
        line.assign("");
        while (end_offset < data.length)
        {
            char c = data[end_offset];
            line.append_c((char)c);
            end_offset++;
            if (c == '\n')
                break;
        }

        if (line.str == "startxref\n")
            startxref_line = line_number;

        if (line.str == "xref\n")
            xref_line = line_number;

        /* Fix PDF header and binary comment */
        if (line_number < 2 && line.str.has_prefix("%%"))
        {
            xref_offset--;
            fixed_file.printf("%s", line.str.substring(1));
        }

        /* Fix xref subsection count */
        else if (line_number == xref_line + 1 && line.str.has_prefix("1 "))
        {
            fixed_size = int.parse(line.str.substring(2)) + 1;
            fixed_file.printf("0 %d\n", fixed_size);
            fixed_file.printf("0000000000 65535 f \n");
        }

        /* Fix xref format */
        else if (line_number > xref_line && line.str.has_suffix(" 0000 n\n"))
            fixed_file.printf("%010d 00000 n \n", int.parse(line.str) + xref_offset);

        /* Fix xref offset */
        else if (startxref_line > 0 && line_number == startxref_line + 1)
            fixed_file.printf("%d\n".printf(int.parse(line.str) + xref_offset));

        else if (fixed_size > 0 && line.str.has_prefix("/Size "))
            fixed_file.printf("/Size %d\n".printf(fixed_size));

        /* Fix EOF marker */
        else if (line_number == startxref_line + 2 && line.str.has_prefix("%%%%"))
            fixed_file.printf("%s", line.str.substring(2));

        else
            for (var i = offset; i < end_offset; i++)
                fixed_file.putc((char)data[i]);

        line_number++;
        offset = end_offset;
    }

    if (FileUtils.rename(filename, filename + "~") >= 0)
        FileUtils.rename(filename + ".fixed", filename);
}

std::string *Receipts::suggest_driver(void)
{
    if (usb_context == NULL)
        return NULL;

    auto driver_map = new std::map<uint32, std::string>(direct_hash, direct_equal);
    add_devices(driver_map, brscan_devices, "brscan");
    add_devices(driver_map, brscan2_devices, "brscan2");
    add_devices(driver_map, brscan3_devices, "brscan3");
    add_devices(driver_map, brscan4_devices, "brscan4");
    add_devices(driver_map, pixma_devices, "pixma");
    add_devices(driver_map, samsung_devices, "samsung");
    add_devices(driver_map, smfp_devices, "smfp");
    add_devices(driver_map, hpaio_devices, "hpaio");
    add_devices(driver_map, epkowa_devices, "epkowa");
    add_devices(driver_map, lexmark_nscan_devices, "lexmark_nscan");
    var devices = usb_context.get_devices();
    for (int i = 0; i < devices.length; i++)
    {
        char device = devices.data[i];
        std::string driver = driver_map.lookup(device.get_vid() << 16 | device.get_pid());
        if (driver != NULL)
            return driver;
    }

    return NULL;
}

int Receipts::main(std::string[] args)
{
    Intl.setlocale(LocaleCategory.ALL, "");
    Intl.bindtextdomain(GETTEXT_PACKAGE, LOCALE_DIR);
    Intl.bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
    Intl.textdomain(GETTEXT_PACKAGE);

    OptionContext c = new OptionContext(/* Arguments and description for --help text */
                                        _("[DEVICE…] — Scanning utility"));
    c.add_main_entries(options, GETTEXT_PACKAGE);
    try
    {
        c.parse(ref args);
    }
    catch (Error e)
    {
        stderr.printf("%s\n", e.message);
        stderr.printf(/* Text printed out when an unknown command-line argument provided */
                      _("Run “%s --help” to see a full list of available command line options."), args[0]);
        stderr.printf("\n");
        return Posix.EXIT_FAILURE;
    }
    if (show_version)
    {
        /* Note, not translated so can be easily parsed */
        stderr.printf("receipts %s\n", VERSION);
        return Posix.EXIT_SUCCESS;
    }
    if (fix_pdf_filename != NULL)
    {
        try
        {
            fix_pdf(fix_pdf_filename);
            for (var i = 1; i < args.length; i++)
                fix_pdf(args[i]);
        }
        catch (Error e)
        {
            stderr.printf("Error fixing PDF file: %s\n", e.message);
            return Posix.EXIT_FAILURE;
        }
        return Posix.EXIT_SUCCESS;
    }

    ScanDevice *device = NULL;
    if (args.length > 1)
    {
        device = new ScanDevice();
        device.name = args[1];
        device.label = args[1];
    }

    /* Log to a file */
    log_timer = new Timer();
    var path = Path.build_filename(Environment.get_user_cache_dir(), "receipts", NULL);
    DirUtils.create_with_parents(path, 0700);
    path = Path.build_filename(Environment.get_user_cache_dir(), "receipts", "receipts.log", NULL);
    log_file = FileStream.open(path, "w");
    if (log_file == NULL)
    {
        stderr.printf("Error: Unable to open %s file for writing\n", path);
        return Posix.EXIT_FAILURE;
    }
    Log.set_default_handler(log_cb);

    spdlog::debug("Starting %s %s, PID=%i", args[0], VERSION, Posix.getpid());

    Gtk::init();

    Receipts app = new Receipts(device);
    return app.run();
};
