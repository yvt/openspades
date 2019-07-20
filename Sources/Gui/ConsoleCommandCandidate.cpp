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

			return Handle<MergeConsoleCommandCandidates>::New(std::move(first), std::move(second))
			  .Cast<ConsoleCommandCandidateIterator>();
		}

		namespace {
			/** Equivalent to `std::string::starts_with` (since C++20) */
			bool StartsWith(const std::string &subject, const std::string &prefix) {
				// FIXME: Code duplicate (see `ConfigConsoleResponder.cpp`)
				if (subject.size() < prefix.size()) {
					return false;
				}
				for (std::size_t i = 0; i < prefix.size(); ++i) {
					if (subject[i] != prefix[i]) {
						return false;
					}
				}
				return true;
			}

			class MapIterator : public ConsoleCommandCandidateIterator {
				const std::map<std::string, std::string> &items;
				std::string query;
				std::map<std::string, std::string>::const_iterator it;
				ConsoleCommandCandidate current;

			public:
				MapIterator(const std::map<std::string, std::string> &items,
				            const std::string &query)
				    : items{items}, query{query} {
					// Find the starting position
					it = items.lower_bound(query);
				}

				const ConsoleCommandCandidate &GetCurrent() override { return current; }

				bool MoveNext() override {
					if (it == items.end() || !StartsWith(it->first, query)) {
						return false;
					}

					// Create `ConsoleCommandCandidate` for the current item
					current.name = it->first;
					current.description = it->second;

					++it;
					return true;
				}
			};
		} // namespace

		Handle<ConsoleCommandCandidateIterator>
		MakeCandidates(const std::map<std::string, std::string> &items, const std::string &query) {
			return Handle<MapIterator>::New(items, query).Cast<ConsoleCommandCandidateIterator>();
		}
	} // namespace gui
} // namespace spades
