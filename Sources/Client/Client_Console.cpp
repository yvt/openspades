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
#include "Client.h"

#include <Gui/ConsoleCommand.h>

namespace spades {
	namespace client {
		namespace {
			constexpr const char *CMD_SAVEMAP = "savemap";

			std::map<std::string, std::string> const g_clientCommands{
			  {CMD_SAVEMAP, ": Save the current state of the map to the disk"},
			};
		} // namespace

		bool Client::ExecCommand(const Handle<gui::ConsoleCommand> &cmd) {
			if (cmd->GetName() == CMD_SAVEMAP) {
				if (cmd->GetNumArguments() != 0) {
					SPLog("Usage: %s (no arguments)", CMD_SAVEMAP);
					return true;
				}
				TakeMapShot();
				return true;
			} else {
				return false;
			}
		}

		Handle<gui::ConsoleCommandCandidateIterator>
		Client::AutocompleteCommandName(const std::string &name) {
			return gui::MakeCandidates(g_clientCommands, name);
		}
	} // namespace client
} // namespace spades
