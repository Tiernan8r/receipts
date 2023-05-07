#include <gtkmm.h>
// #include <gtkmm-4.0/gtkmm.h>
#include <iostream>
#include <sane/sane.h>

// // Function to get the available scanners
// SANE_Status getSaneDevices(SANE_Device ***deviceList, SANE_Int *numDevices, SANE_Bool localOnly)
// {
//     return sane_get_devices(deviceList, localOnly);
// }

int main(int argc, char *argv[])
{
    // Initialize Gtkmm application
    // auto app = Gtk::Application::create(argc, argv);
    auto app = Gtk::Application::create("me.tiernan8r.chatgpt_example1");

    // Create Gtkmm window
    Gtk::Window window;
    window.set_default_size(400, 300);
    window.set_title("Scanning GUI");

    // Create Gtkmm button to start the scan
    Gtk::Button scanButton("Scan");

    // Create Gtkmm image to display the scanned image
    Gtk::Image scannedImage;

    // Create Gtkmm box to hold the scan button and scanned image
    Gtk::Box box(Gtk::Orientation::VERTICAL);
    box.append(scanButton);
    // box.pack_start(scanButton, Gtk::PACK_SHRINK);
    // box.pack_start(scannedImage, Gtk::PACK_EXPAND_WIDGET);
    box.append(scannedImage);

    // Connect scan button signal handler
    scanButton.signal_clicked().connect([&]()
                                        {
        // Get the available scanners
        const SANE_Device **deviceList;
        SANE_Int numDevices;
        bool localOnly = true;
        sane_get_devices(&deviceList, localOnly);
        // getSaneDevices(&deviceList, &numDevices, true);

        // Check if there are any scanners available
        if (numDevices == 0) {
            std::cout << "No scanners found.\n";
            return;
        }

        // Open the first scanner
        SANE_Handle handle;
        sane_open(deviceList[0]->name, &handle);

        // Get scanner parameters
        SANE_Parameters parameters;
        sane_get_parameters(handle, &parameters);

        // Set scan options
        // SANE_Option_Descriptor *desc;
        // SANE_Int numOptions;
        // sane_get_option_descriptor(handle, 0, &desc);

        // TODO: figure out how to fix??
        // const SANE_Option_Descriptor *desc;
        // desc = sane_get_option_descriptor(handle, 0);
        // // sane_control_option(handle, 0, SANE_ACTION_SET_VALUE, &desc->constraint->default_value, NULL);
        // sane_control_option(handle, 0, SANE_ACTION_SET_VALUE, &desc->constraint->range->quant, NULL);
        // // sane_get_option_descriptor(handle, 1, &desc);
        // desc = sane_get_option_descriptor(handle, 1);
        // sane_control_option(handle, 1, SANE_ACTION_SET_VALUE, &parameters.opt[1].constraint->range->max, NULL);
        // // sane_get_option_descriptor(handle, 2, &desc);
        // desc = sane_get_option_descriptor(handle, 2);
        // sane_control_option(handle, 2, SANE_ACTION_SET_VALUE, &parameters.opt[2].constraint->range->max, NULL);
        // // sane_get_option_descriptor(handle, 3, &desc);
        // desc = sane_get_option_descriptor(handle, 3);
        // sane_control_option(handle, 3, SANE_ACTION_SET_VALUE, &parameters.opt[3].constraint->default_value, NULL);

        // Start the scan
        SANE_Byte *buffer;
        SANE_Int readBytes;
        sane_start(handle);
        sane_get_parameters(handle, &parameters);
        buffer = (SANE_Byte *) malloc(parameters.bytes_per_line * parameters.lines);
        sane_read(handle, buffer, parameters.bytes_per_line * parameters.lines, &readBytes);
        sane_cancel(handle);

        // Convert the scanned image to GdkPixbuf
        Glib::RefPtr <Gdk::Pixbuf> pixbuf = Gdk::Pixbuf::create_from_data(
                buffer,
                Gdk::Colorspace::RGB,
                false,
                8,
                parameters.pixels_per_line,
                parameters.lines,
                parameters.bytes_per_line
        );

        // Set the scanned image to the Gtkmm image widget
        scannedImage.set(pixbuf);

        // Free the buffer and close the scanner
        free(buffer);
        sane_close(handle); });

    // Add the box to the window and show everything
    // window.add(box);
    window.set_child(box);
    // window.show_all();
    window.show();

    // Run Gtkmm application
    // return app->run(window);
    // return app->make_window_and_run(window)->run(argc, argv);
    app->run(argc, argv);
}