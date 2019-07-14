/*
 Copyright (c) 2013 yvt

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
#include "../UIFramework/Field.as"

namespace spades {

    class CommandHistoryItem {
        string text;
        int selStart;
        int selEnd;
        CommandHistoryItem() {}
        CommandHistoryItem(string text, int selStart, int selEnd) {
            this.text = text;
            this.selStart = selStart;
            this.selEnd = selEnd;
        }
    }

    // Field with bash-like history support
    class FieldWithHistory : spades::ui::Field {
        array<spades::ui::CommandHistoryItem @> @cmdhistory;
        CommandHistoryItem @temporalLastHistory;
        uint currentHistoryIndex;

        FieldWithHistory(spades::ui::UIManager @manager,
                         array<spades::ui::CommandHistoryItem @> @history) {
            super(manager);

            @this.cmdhistory = history;
            currentHistoryIndex = history.length;
            @temporalLastHistory = this.CommandHistoryItemRep;
        }

        private CommandHistoryItem @CommandHistoryItemRep {
            get { return CommandHistoryItem(this.Text, this.SelectionStart, this.SelectionEnd); }
            set {
                this.Text = value.text;
                this.Select(value.selStart, value.selEnd - value.selStart);
            }
        }

        private void OverwriteItem() {
            if (currentHistoryIndex < cmdhistory.length) {
                @cmdhistory[currentHistoryIndex] = this.CommandHistoryItemRep;
            } else if (currentHistoryIndex == cmdhistory.length) {
                @temporalLastHistory = this.CommandHistoryItemRep;
            }
        }

        private void LoadItem() {
            if (currentHistoryIndex < cmdhistory.length) {
                @this.CommandHistoryItemRep = cmdhistory[currentHistoryIndex];
            } else if (currentHistoryIndex == cmdhistory.length) {
                @this.CommandHistoryItemRep = temporalLastHistory;
            }
        }

        void KeyDown(string key) {
            if (key == "Up") {
                if (currentHistoryIndex > 0) {
                    OverwriteItem();
                    currentHistoryIndex--;
                    LoadItem();
                }
            } else if (key == "Down") {
                if (currentHistoryIndex < cmdhistory.length) {
                    OverwriteItem();
                    currentHistoryIndex++;
                    LoadItem();
                }
            } else {
                Field::KeyDown(key);
            }
        }

        void CommandSent() {
            cmdhistory.insertLast(this.CommandHistoryItemRep);
            currentHistoryIndex = cmdhistory.length - 1;
        }

        void Clear() {
            currentHistoryIndex = cmdhistory.length;
            this.Text = "";
            OverwriteItem();
        }

        void Cancelled() { OverwriteItem(); }
    };

}
