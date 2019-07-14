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

#include "ConsoleCommand.h"
#include "ConsoleHelper.h"
#include "ConsoleScreen.h"

namespace spades {
	namespace gui {
		ConsoleHelper::ConsoleHelper(ConsoleScreen *scr) {
			SPADES_MARK_FUNCTION();

			parentWeak = scr;
		}

		ConsoleHelper::~ConsoleHelper() {}

		void ConsoleHelper::ConsoleScreenDestroyed() {
			SPADES_MARK_FUNCTION();

			parentWeak = nullptr;
		}

		Handle<ConsoleScreen> ConsoleHelper::GetParent() { return {parentWeak, true}; }

		void ConsoleHelper::ExecCommand(const std::string &text) {
			SPADES_MARK_FUNCTION();

			auto parent = GetParent();
			if (!parent) {
				return;
			}

			SPLog("Command: %s", text.c_str());
			try {
				auto const command = ConsoleCommand::Parse(text);

				if (!parent->ExecCommand(command)) {
					SPLog("Unknown command: '%s'", command->GetName().c_str());
				}
			} catch (const std::exception &e) {
				SPLog("An exception was thrown while executing a console command: %s", e.what());
			}
		}

		ConsoleCommandCandidateIterator *
		ConsoleHelper::AutocompleteCommandName(const std::string &name) {
			SPADES_MARK_FUNCTION();

			auto parent = GetParent();
			if (!parent) {
				return new EmptyIterator<const ConsoleCommandCandidate &>();
			}

			return parent->AutocompleteCommandName(name).Unmanage();
		}
	} // namespace gui
} // namespace spades
