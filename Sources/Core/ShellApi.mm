/*
 Copyright (c) 2016 yvt

 This file is part of OpenSpades.

 OpenSpades is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 OpenSpades is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with OpenSpades.  If not, see <http://www.gnu.org/licenses/>.

 */

// This file contains macOS specific code.

#import <Cocoa/Cocoa.h>

#include "ShellApi.h"

#include <Core/Debug.h>

namespace spades {
    bool ShowDirectoryInShell(const std::string &directoryPath) {
        NSString *path =
          [NSString stringWithCString:directoryPath.c_str() encoding:NSUTF8StringEncoding];
        BOOL result = [[NSWorkspace sharedWorkspace] openFile:path];
        return bool(result);
    }

    bool OpenURLInBrowser(const std::string &url) {
        NSString *urlString = [NSString stringWithCString:url.c_str() encoding:NSUTF8StringEncoding];
		NSURL *nsurl = [NSURL URLWithString:urlString];
        BOOL result = [[NSWorkspace sharedWorkspace] openURL:nsurl];
        return bool(result);
    }
}
