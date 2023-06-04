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

#include "postprocessor.h"
#include <string>
#include <spdlog/spdlog.h>
#include <stdio.h>
#include <iostream>
#include <glibmm-2.68/glibmm.h>

extern char **environ;

Postprocessor::Postprocessor(){

};

int Postprocessor::process(std::string script, std::string mime_type, bool keep_original, std::string source_file, std::string arguments)
{ // throws Error {
    // Code copied and adapted from https://valadoc.org/glib-2.0/GLib.Process.spawn_sync.html
    auto spawn_args = new std::vector<std::string>{ script, mime_type, keep_original ? "true" : "false", source_file, arguments};
    auto spawn_env = new std::vector<std::string>(**environ);
    // std::string spawn_env[] = Environ::get();
    std::string process_stdout;
    std::string process_stderr;
    int process_status;

    std::cout << "Executing script" << script << "\n";
    Glib::spawn_sync(NULL, // inherit parent's working dir
                     spawn_args,
                     spawn_env,
                     Glib::SpawnFlags::SEARCH_PATH,
                     NULL,
                     &process_stdout,
                     &process_stderr,
                     &process_status);
    spdlog::debug("status: %d\n", process_status);
    spdlog::debug("STDOUT: \n");
    spdlog::debug("process_stdout");
    spdlog::debug("STDERR: \n");
    spdlog::debug("process_stderr");

    return process_status;
};
