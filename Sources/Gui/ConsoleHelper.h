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
#pragma once

#include <Core/RefCountedObject.h>

#include "ConsoleCommandCandidate.h"

namespace spades {
	namespace gui {
		class ConsoleScreen;

		/**
		 * Provides methods to be called by the AngelScript part of the system
		 * console.
		 */
		class ConsoleHelper : public RefCountedObject {
		public:
			ConsoleHelper(ConsoleScreen *scr);
			void ConsoleScreenDestroyed();

			/** Execute a console command. */
			void ExecCommand(const std::string &);

			/** Produce a sequence of candidates for command name autocompletion. */
			ConsoleCommandCandidateIterator *AutocompleteCommandName(const std::string &name);

		private:
			~ConsoleHelper();
			/** A weak reference to the owning `ConsoleScreen`. */
			ConsoleScreen *parentWeak;
			/** Get a strong reference to the owning `ConsoleScreen`. */
			Handle<ConsoleScreen> GetParent();
		};
	} // namespace gui
} // namespace spades
