# Copyright (C) 2022  Tiernan8r
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.
import os
import sys

# Required to guarantee that the 'qcp' module is accessible when
# this file is run directly.
if os.getcwd() not in sys.path:
    sys.path.append(os.getcwd())

import argparse

import output
from src.ocr import extract
from src.scanning import scan


def setup_cli():
    # construct the argument parser and parse the arguments
    ap = argparse.ArgumentParser()
    ap.add_argument("image_path",
                    help="Path to the image to be scanned")

    return ap


def main():
    ap = setup_cli()
    args = vars(ap.parse_args())

    img_path = args["image_path"]

    # Scan the image
    scanned_img = scan.scan(img_path)
    scan_save_path = output.get_save_path(img_path)

    success = output.save_img(scanned_img, scan_save_path)
    if not success:
        print(f"Could not save image to path '{scan_save_path}'")
        sys.exit(1)

    # OCR:
    text = extract.extract_text(scan_save_path)

    _, fname, _ = output.deconstruct_img_path(img_path)
    text_path = os.path.join(output.save_dir(), fname + ".txt")

    with open(text_path, "w") as f:
        f.write(text)

    print("Scan Text Result:")
    print(text)

    pdf_path = os.path.join(output.save_dir(), fname + ".pdf")
    extract.extract_to_pdf(scan_save_path, pdf_path)


if __name__ == "__main__":
    main()
