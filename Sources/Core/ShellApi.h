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

#pragma once

#include <string>

namespace spades {

    /**
     * Shows the specified directory with a native file manager like Explorer
     * or Finder.
     *
     * @param directoryPath The path of the directory to show.
     * @return true if it succeeds. false otherwise.
     */
    bool ShowDirectoryInShell(const std::string &directoryPath);

	/**
	 * Opens the specified URL with the preferred web browser.
	 *
	 * @param url The URL to open.
	 * @return true if it succeeds. false otherwise.
	 */
	bool OpenURLInBrowser(const std::string &url);
}
