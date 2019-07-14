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

#include <map>
#include <string>

#include <Core/Iterator.h>

namespace spades {
	namespace gui {
		/**
		 * Represents a candidate in console command name/value autocompletion.
		 */
		struct ConsoleCommandCandidate {
			std::string name;
			std::string description;
		};

		/**
		 * A sequence of `ConsoleCommandCandidate`s. Must be sorted by
		 * `ConsoleCommandCandidate::name`.
		 */
		using ConsoleCommandCandidateIterator = Iterator<const ConsoleCommandCandidate &>;

		/**
		 * Construct a `ConsoleCommandCandidateIterator` from the specified
		 * `std::map`.
		 *
		 * The map should remain unmodified and outlive the returned iterator.
		 */
		Handle<ConsoleCommandCandidateIterator>
		MakeCandidates(const std::map<std::string, std::string> &, const std::string &query);

		Handle<ConsoleCommandCandidateIterator>
		MergeCandidates(Handle<ConsoleCommandCandidateIterator> first,
		                Handle<ConsoleCommandCandidateIterator> second);

		inline Handle<ConsoleCommandCandidateIterator>
		operator+(Handle<ConsoleCommandCandidateIterator> first,
		          Handle<ConsoleCommandCandidateIterator> second) {
			return MergeCandidates(first, second);
		}
	} // namespace gui
} // namespace spades
