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
import cv2
import imutils
import os


def save_dir():
    return os.path.join(os.getcwd(), "scans")


def deconstruct_img_path(img_path):
    dir, basename = os.path.split(img_path)
    fname, extension = os.path.splitext(basename)

    return dir, fname, extension


def get_save_path(img_path):
    _, fname, extension = deconstruct_img_path(img_path)

    save_fname = fname + "-scan"

    return os.path.join(save_dir(), save_fname + extension)


def _ensure_path_exists(save_path):
    dirs, fname = os.path.split(save_path)
    if not os.path.exists(dirs):
        os.makedirs(dirs)


def save_img(img, save_path):
    # Resize the image
    resized = imutils.resize(img, height=650)
    _ensure_path_exists(save_path)

    return cv2.imwrite(save_path, resized)
