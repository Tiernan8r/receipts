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
#ifndef SCREENSAVER_H
#define SCREENSAVER_H

#include <string>

// [DBus (name = "org.freedesktop.ScreenSaver")]
class IFreedesktopScreensaver
{
    public:
        virtual ~IFreedesktopScreensaver() {};
        static IFreedesktopScreensaver get_proxy (void); // throws IOError;

    // [DBus (name = "Inhibit")]
        virtual uint32_t inhibit (std::string application_name, std::string reason_for_inhibit); // throws Error;
    // [DBus (name = "UnInhibit")]
        virtual void uninhibit (uint32_t cookie); // throws Error;
};


#endif //SCREENSAVER_H