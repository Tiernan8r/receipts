#include <iostream>
#include <sane/sane.h>
#include <gtkmm.h>

// This function scans the image using SANE
void scan_image(const char *device_name, const char *output_file)
{
    SANE_Status status;
    SANE_Handle device;

    // Open the device
    status = sane_open(device_name, &device);
    if (status != SANE_STATUS_GOOD)
    {
        std::cerr << "Failed to open device: " << device_name << std::endl;
        return;
    }

    // Get the device parameters
    SANE_Parameters parameters;
    status = sane_get_parameters(device, &parameters);
    if (status != SANE_STATUS_GOOD)
    {
        std::cerr << "Failed to get parameters for device: " << device_name << std::endl;
        sane_close(device);
        return;
    }

    // Set the scan options
    //    SANE_Options options;
    SANE_Int options;
    status = sane_control_option(device, 0, SANE_ACTION_SET_VALUE, &options, NULL);
    if (status != SANE_STATUS_GOOD)
    {
        std::cerr << "Failed to set scan options for device: " << device_name << std::endl;
        sane_close(device);
        return;
    }

    // Start the scan
    SANE_Byte buffer[parameters.bytes_per_line * parameters.lines];
    SANE_Int read_bytes;
    SANE_Int total_bytes = 0;
    status = sane_start(device);
    if (status != SANE_STATUS_GOOD)
    {
        std::cerr << "Failed to start scan for device: " << device_name << std::endl;
        sane_close(device);
        return;
    }

    // Read the scan data
    do
    {
        status = sane_read(device, buffer + total_bytes, sizeof(buffer) - total_bytes, &read_bytes);
        if (status != SANE_STATUS_GOOD)
        {
            std::cerr << "Failed to read scan data for device: " << device_name << std::endl;
            sane_cancel(device);
            sane_close(device);
            return;
        }
        total_bytes += read_bytes;
    } while (total_bytes < sizeof(buffer));

    // Write the image to file
    FILE *file = fopen(output_file, "wb");
    fwrite(buffer, 1, sizeof(buffer), file);
    fclose(file);

    // End the scan
    //    status = sane_cancel(device);
    sane_cancel(device);
    sane_close(device);
}

class ScannerWindow : public Gtk::Window
{
public:
    ScannerWindow()
    {
        // Set the window title
        set_title("Scanner");

        // Set the window size
        set_default_size(400, 300);

        // Create a button to initiate the scan
        m_button.set_label("Scan");
        m_button.signal_clicked().connect(sigc::mem_fun(*this, &ScannerWindow::on_scan_button_clicked));

        // Add the button to the window
        // add(m_button);
        set_child(m_button);

        // Show all the widgets
        // show_all_children();
        show();
    }

    void on_scan_button_clicked()
    {
        // Open a file dialog to select the output file
        Gtk::FileChooserDialog dialog(*this, "Save file as", Gtk::FileChooser::Action::SAVE);
        // dialog.set_current_folder(".");
        dialog.add_button("_Cancel", Gtk::ResponseType::CANCEL);
        dialog.add_button("_Save", Gtk::ResponseType::OK);
        // int result = dialog.run();
        dialog.show();
        // int result = dialog.response();
        int result;
        if (result == Gtk::ResponseType::OK)
        {
            std::string filename = dialog.get_file()->get_path();

            // Scan the image and save
            scan_image("scanner", filename.c_str());
        }
    }

private:
    Gtk::Button m_button;
};

int main(int argc, char *argv[])
{
    // Initialize GTK
    auto app = Gtk::Application::create("me.tiernan8r.chatgpt_example2");

    return app->make_window_and_run<ScannerWindow>(argc, argv);
}