/*
 Copyright (c) 2019 yvt

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
#include <Core/Debug.h>
#include <Core/Exception.h>
#include <Core/Math.h>

#include "ConsoleCommand.h"

namespace spades {
	namespace gui {
		Handle<ConsoleCommand> ConsoleCommand::Parse(const std::string &textRaw) {
			SPADES_MARK_FUNCTION();

			Handle<ConsoleCommand> cmd{new ConsoleCommand(), false};

			std::string const text = TrimSpaces(textRaw);
			std::vector<std::string> parts;
			std::size_t i = 0;
			bool quoted = false;

			parts.emplace_back();

			while (i < text.size()) {
				// Search for the next special characters. Whitespaces are
				// treated as a parameter separator, except in a quotation.
				std::size_t next = text.find_first_of(quoted ? "\"\\" : " \"\\", i);
				if (next == std::string::npos) {
					parts.back() += text.substr(i);
					break;
				}

				parts.back() += text.substr(i, next - i);

				switch (text[next]) {
					case ' ':
						parts.emplace_back();
						i = next + 1;
						break;
					case '"':
						quoted = !quoted;
						i = next + 1;
						break;
					case '\\':
						if (next + 1 >= text.size()) {
							// No character to escape
							SPRaise("Backslashes must be followed by an ASCII character.");
						}
						if (text[next + 1] < 0) {
							// Non-ASCII UTF-8 character
							SPRaise("Backslashes must be followed by an ASCII character.");
						}
						parts.back().push_back(text[next + 1]);
						i = next + 2;
						break;
					default: SPUnreachable();
				}
			}

			cmd->name = std::move(parts[0]);
			parts.erase(parts.begin(), parts.begin() + 1);
			cmd->args = std::move(parts);

			return cmd;
		}

		ConsoleCommand::~ConsoleCommand() {}
	} // namespace gui
} // namespace spades