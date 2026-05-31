/* 
 * Copyright (C) 2026 Bruce MacKinnon
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#include <dirent.h>
#include <stdio.h>
#include <cstring>

#include "kc1fsz-tools/linux/FileUtils.h"

using namespace std;

namespace kc1fsz {

void visitDir(const string& base, std::function<void(const string& name)> cb) {
    DIR *d;
    struct dirent *dir;
    d = opendir(base.c_str());
    if (d) {
        // Read each entry in the directory
        while ((dir = readdir(d)) != NULL) {
            // Skip the special entries for current ('.') and parent ('..') directories
            if (strcmp(dir->d_name, ".") == 0 || strcmp(dir->d_name, "..") == 0) 
                continue;
            cb(string(dir->d_name));
        }
        // Close the directory stream
        closedir(d);
    }
}

}
