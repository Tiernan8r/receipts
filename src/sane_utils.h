
#ifndef RECEIPTS_SANE_UTILS__H
#define RECEIPTS_SANE_UTILS__H

#include <sane/sane.h>
#include <iostream>

void scan_image(const char* device_name, const char* output_file);

#endif /* RECEIPTS_SANE_UTILS__H */