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

			/** Equivalent to `std::string::starts_with` (since C++20) */
			bool StartsWith(const std::string &subject, const std::string &prefix) {
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

			class ConfigNameIterator : public ConsoleCommandCandidateIterator {
				std::vector<std::string> const names;
				std::string query;
				std::size_t i = 0;
				ConsoleCommandCandidate current;

				void SkipUnknowns() {
					while (i < names.size()) {
						Settings::ItemHandle cvarHandle{names[i], nullptr};
						if (!cvarHandle.IsUnknown()) {
							break;
						}
						++i;
					}
				}

			public:
				ConfigNameIterator(const std::string &query)
				    : names{Settings::GetInstance()->GetAllItemNames()}, query{query} {
					// Find the starting position
					i = std::lower_bound(names.begin(), names.end(), query) - names.begin();
					SkipUnknowns();
				}

				const ConsoleCommandCandidate &GetCurrent() override { return current; }

				bool MoveNext() override {
					if (i >= names.size() || !StartsWith(names[i], query)) {
						return false;
					}

					// Create `ConsoleCommandCandidate` for the current
					// config variable
					Settings::ItemHandle cvarHandle{names[i], nullptr};
					current.name = names[i];
					current.description = " = \"" + EscapeQuotes(cvarHandle) + "\"";

					i += 1;
					SkipUnknowns();

					return true;
				}
			};

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

		Handle<ConsoleCommandCandidateIterator>
		ConfigConsoleResponder::AutocompleteCommandName(const std::string &name) {
			return Handle<ConfigNameIterator>::New(name).Cast<ConsoleCommandCandidateIterator>();
		}
	} // namespace gui
} // namespace spades
