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
import PIL.Image
import pytesseract


def extract_text(image_path):
    image = PIL.Image.open(image_path)

    output = pytesseract.image_to_string(image, lang='eng')

    return output


def extract_to_pdf(image_path, pdf_save_path):
    image = PIL.Image.open(image_path)

    PDF = pytesseract.image_to_pdf_or_hocr(
        image, lang='eng', config='', nice=0, extension='pdf')

    with open(pdf_save_path, "w+b") as f:
        f.write(bytearray(PDF))
