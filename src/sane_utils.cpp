
#include "sane_utils.h"

// This function scans the image using SANE
void scan_image(const char* device_name, const char* output_file) {
    SANE_Status status;
    SANE_Handle device;

    // initialise the scanner
    SANE_Int version_code;
    status = sane_init(&version_code, NULL);
    if (status != SANE_STATUS_GOOD) {
        std::cerr << "Failed to initialise device: " << status << " v" << version_code << std::endl;
        return;
    }

    // Open the device
    status = sane_open(device_name, &device);
    if (status != SANE_STATUS_GOOD) {
        std::cerr << "Failed to open device: " << device_name << std::endl;
        return;
    }

    // Get the device parameters
    SANE_Parameters parameters;
    status = sane_get_parameters(device, &parameters);
    if (status != SANE_STATUS_GOOD) {
        std::cerr << "Failed to get parameters for device: " << device_name << std::endl;
        sane_close(device);
        return;
    }

    // Set the scan options
//    SANE_Options options;
    SANE_Int options;
    status = sane_control_option(device, 0, SANE_ACTION_SET_VALUE, &options, NULL);
    if (status != SANE_STATUS_GOOD) {
        std::cerr << "Failed to set scan options for device: " << device_name << std::endl;
        sane_close(device);
        return;
    }

    // Start the scan
    SANE_Byte buffer[parameters.bytes_per_line * parameters.lines];
    SANE_Int read_bytes;
    SANE_Int total_bytes = 0;
    status = sane_start(device);
    if (status != SANE_STATUS_GOOD) {
        std::cerr << "Failed to start scan for device: " << device_name << std::endl;
        sane_close(device);
        return;
    }

    // Read the scan data
    do {
        status = sane_read(device, buffer + total_bytes, sizeof(buffer) - total_bytes, &read_bytes);
        if (status != SANE_STATUS_GOOD) {
            std::cerr << "Failed to read scan data for device: " << device_name << std::endl;
            sane_cancel(device);
            sane_close(device);
            return;
        }
        total_bytes += read_bytes;
    } while (total_bytes < sizeof(buffer));

    // Write the image to file
    FILE* file = fopen(output_file, "wb");
    fwrite(buffer, 1, sizeof(buffer), file);
    fclose(file);

    // End the scan
//    status = sane_cancel(device);
    sane_cancel(device);
    sane_close(device);
}