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

#include <string>
#include <vector>

#include <Core/RefCountedObject.h>

namespace spades {
	namespace gui {
		/**
		 * Represents a console command to be executed.
		 */
		class ConsoleCommand : public RefCountedObject {
		public:
			/**
			 * Constructa a `ConsoleCommand` by parsing the given command
			 * string.
			 *
			 * Throws an exception if parsing fails.
			 *
			 * @return A `ConsoleCommand` object representing the parsed command.
			 */
			static Handle<ConsoleCommand> Parse(const std::string &);

			/**
			 * Get the command name.
			 */
			const std::string &GetName() const { return name; }

			/**
			 * Get the number of arguments.
			 */
			std::size_t GetNumArguments() const { return args.size(); }

			/**
			 * Get the argument at the specified index `i`. `i` must be in
			 * range `[0, GetNumArguments() - 1]`.
			 */
			const std::string &GetArgument(std::size_t i) const { return args.at(i); }

		private:
			ConsoleCommand() {}
			~ConsoleCommand();

			std::string name;
			std::vector<std::string> args;
		};
	} // namespace gui
} // namespace spades
