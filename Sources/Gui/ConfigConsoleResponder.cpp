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
#include "ConfigConsoleResponder.h"

#include <Core/Debug.h>
#include <Core/Settings.h>
#include <Core/Strings.h>

#include "ConsoleCommand.h"

namespace spades {
	namespace gui {
		namespace {
			std::string EscapeQuotes(std::string s) { return Replace(s, "\"", "\\\""); }
		} // namespace

		bool ConfigConsoleResponder::ExecCommand(const Handle<ConsoleCommand> &cmd) {
			SPADES_MARK_FUNCTION();

			Settings::ItemHandle cvarHandle{cmd->GetName(), nullptr};
			if (cvarHandle.IsUnknown()) {
				return false;
			}

			if (cmd->GetNumArguments() == 0) {
				// Display the current value
				SPLog("%s = \"%s\"", cmd->GetName().c_str(), EscapeQuotes(cvarHandle).c_str());
			} else if (cmd->GetNumArguments() == 1) {
				// Set a new value
				const std::string &newValue = cmd->GetArgument(0);
				SPLog("%s = \"%s\" (previously \"%s\")", cmd->GetName().c_str(),
				      EscapeQuotes(newValue).c_str(), EscapeQuotes(cvarHandle).c_str());
				cvarHandle = newValue;
			} else {
				SPLog("Invalid number of arguments (Maybe you meant something "
				      "like 'varname \"value1 value2\"'?)");
			}

			return true;
		}
	} // namespace gui
} // namespace spades
