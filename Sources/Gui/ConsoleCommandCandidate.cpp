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
#include <utility>

#include <Core/Debug.h>

#include "ConsoleCommandCandidate.h"

namespace spades {
	namespace gui {
		namespace {
			class MergeConsoleCommandCandidates final : public ConsoleCommandCandidateIterator {
				Handle<ConsoleCommandCandidateIterator> first;
				Handle<ConsoleCommandCandidateIterator> second;
				bool hasFirst : 1;
				bool hasSecond : 1;
				bool chooseSecond : 1;
				bool initial : 1;

			public:
				MergeConsoleCommandCandidates(Handle<ConsoleCommandCandidateIterator> first,
				                              Handle<ConsoleCommandCandidateIterator> second)
				    : first{std::move(first)},
				      second{std::move(second)},
				      hasFirst{true},
				      hasSecond{true},
				      chooseSecond{false},
				      initial{true} {}

				const ConsoleCommandCandidate &GetCurrent() override {
					SPADES_MARK_FUNCTION_DEBUG();

					return chooseSecond ? second->GetCurrent() : first->GetCurrent();
				}

				bool MoveNext() override {
					SPADES_MARK_FUNCTION_DEBUG();

					if (initial) {
						hasFirst = first->MoveNext();
						hasSecond = second->MoveNext();
						initial = false;
					} else {
						if (chooseSecond) {
							hasSecond = second->MoveNext();
						} else {
							hasFirst = first->MoveNext();
						}
					}

					if (!hasSecond && !hasFirst) {
						return false;
					} else if (!hasFirst) {
						chooseSecond = true;
					} else if (!hasSecond) {
						chooseSecond = false;
					} else {
						chooseSecond = second->GetCurrent().name < first->GetCurrent().name;
					}

					return true;
				}
			};
		} // namespace

		Handle<ConsoleCommandCandidateIterator>
		MergeCandidates(Handle<ConsoleCommandCandidateIterator> first,
		                Handle<ConsoleCommandCandidateIterator> second) {
			SPADES_MARK_FUNCTION();

			return {new MergeConsoleCommandCandidates{std::move(first), std::move(second)}, false};
		}
	} // namespace gui
} // namespace spades
