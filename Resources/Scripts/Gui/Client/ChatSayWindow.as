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
#include "FieldWithHistory.as"

namespace spades {
    // TODO: Remove cvar editing (superseded by the system console) after 0.1.4

    /** Shows cvar's current value when user types something like "/cg_foobar" */
    class CommandFieldConfigValueView : spades::ui::UIElement {
        string[] @configNames;
        string[] configValues;
        CommandFieldConfigValueView(spades::ui::UIManager @manager, string[] configNames) {
            super(manager);
            for (uint i = 0, len = configNames.length; i < len; i++) {
                configValues.insertLast(ConfigItem(configNames[i]).StringValue);
            }
            @this.configNames = configNames;
        }
        void Render() {
            float maxNameLen = 0.f;
            float maxValueLen = 20.f;
            Font @font = this.Font;
            Renderer @renderer = this.Manager.Renderer;
            float rowHeight = 25.f;

            for (uint i = 0, len = configNames.length; i < len; i++) {
                maxNameLen = Max(maxNameLen, font.Measure(configNames[i]).x);
                maxValueLen = Max(maxValueLen, font.Measure(configValues[i]).x);
            }
            Vector2 pos = this.ScreenPosition;
            pos.y -= float(configNames.length) * rowHeight + 10.f;

            renderer.ColorNP = Vector4(0.f, 0.f, 0.f, 0.5f);
            renderer.DrawImage(null, AABB2(pos.x, pos.y, maxNameLen + maxValueLen + 20.f,
                                           float(configNames.length) * rowHeight + 10.f));

            for (uint i = 0, len = configNames.length; i < len; i++) {
                font.DrawShadow(configNames[i], pos + Vector2(5.f, 8.f + float(i) * rowHeight), 1.f,
                                Vector4(1, 1, 1, 0.7), Vector4(0, 0, 0, 0.3f));
                font.DrawShadow(configValues[i],
                                pos + Vector2(15.f + maxNameLen, 8.f + float(i) * rowHeight), 1.f,
                                Vector4(1, 1, 1, 1), Vector4(0, 0, 0, 0.4f));
            }
        }
    }

    class CommandField : FieldWithHistory {
        CommandFieldConfigValueView @valueView;

        CommandField(spades::ui::UIManager @manager,
                     array<spades::ui::CommandHistoryItem @> @history) {
            super(manager, history);
        }

        void OnChanged() {
            FieldWithHistory::OnChanged();

            if (valueView !is null) {
                @valueView.Parent = null;
            }
            if (Text.substr(0, 1) == "/" && Text.substr(1, 1) != " ") {
                int whitespace = Text.findFirst(" ");
                if (whitespace < 0) {
                    whitespace = int(Text.length);
                }

                string input = Text.substr(1, whitespace - 1);
                if (input.length >= 2) {
                    string[] @names = GetAllConfigNames();
                    string[] filteredNames;
                    for (uint i = 0, len = names.length; i < len; i++) {
                        if (StringCommonPrefixLength(input, names[i]) == input.length &&
                            !ConfigItem(names[i]).IsUnknown) {
                            filteredNames.insertLast(names[i]);
                            if (filteredNames.length >= 8) {
                                // too many
                                break;
                            }
                        }
                    }
                    if (filteredNames.length > 0) {
                        @valueView = CommandFieldConfigValueView(this.Manager, filteredNames);
                        valueView.Bounds = AABB2(0.f, -15.f, 0.f, 0.f);
                        @valueView.Parent = this;
                    }
                }
            }
        }

        void KeyDown(string key) {
            if (key == "Tab") {
                if (SelectionLength == 0 && SelectionStart == int(Text.length) &&
                    Text.substr(0, 1) == "/" && Text.findFirst(" ") < 0) {
                    // config variable auto completion
                    string input = Text.substr(1);
                    string[] @names = GetAllConfigNames();
                    string commonPart;
                    bool foundOne = false;
                    for (uint i = 0, len = names.length; i < len; i++) {
                        if (StringCommonPrefixLength(input, names[i]) == input.length &&
                            !ConfigItem(names[i]).IsUnknown) {
                            if (!foundOne) {
                                commonPart = names[i];
                                foundOne = true;
                            }

                            uint commonLen = StringCommonPrefixLength(commonPart, names[i]);
                            commonPart = commonPart.substr(0, commonLen);
                        }
                    }

                    if (commonPart.length > input.length) {
                        Text = "/" + commonPart;
                        Select(Text.length, 0);
                    }
                }
            } else {
                FieldWithHistory::KeyDown(key);
            }
        }
    }

    class ClientChatWindow : spades::ui::UIElement {
        private ClientUI @ui;
        private ClientUIHelper @helper;

        CommandField @field;
        spades::ui::Button @sayButton;
        spades::ui::SimpleButton @teamButton;
        spades::ui::SimpleButton @globalButton;

        bool isTeamChat;

        ClientChatWindow(ClientUI @ui, bool isTeamChat) {
            super(ui.manager);
            @this.ui = ui;
            @this.helper = ui.helper;
            this.isTeamChat = isTeamChat;

            float winW = Manager.Renderer.ScreenWidth * 0.7f, winH = 66.f;
            float winX = (Manager.Renderer.ScreenWidth - winW) * 0.5f;
            float winY = (Manager.Renderer.ScreenHeight - winH) - 20.f;
            /*
            {
                spades::ui::Label label(Manager);
                label.BackgroundColor = Vector4(0, 0, 0, 0.5f);
                label.Bounds = Bounds;
                AddChild(label);
            }*/

            {
                spades::ui::Label label(Manager);
                label.BackgroundColor = Vector4(0, 0, 0, 0.5f);
                label.Bounds = AABB2(winX - 8.f, winY - 8.f, winW + 16.f, winH + 16.f);
                AddChild(label);
            }
            {
                spades::ui::Button button(Manager);
                button.Caption = _Tr("Client", "Say");
                button.Bounds = AABB2(winX + winW - 244.f, winY + 36.f, 120.f, 30.f);
                @button.Activated = spades::ui::EventHandler(this.OnSay);
                AddChild(button);
                @sayButton = button;
            }
            {
                spades::ui::Button button(Manager);
                button.Caption = _Tr("Client", "Cancel");
                button.Bounds = AABB2(winX + winW - 120.f, winY + 36.f, 120.f, 30.f);
                @button.Activated = spades::ui::EventHandler(this.OnCancel);
                AddChild(button);
            }
            {
                @field = CommandField(Manager, ui.chatHistory);
                field.Bounds = AABB2(winX, winY, winW, 30.f);
                field.Placeholder = _Tr("Client", "Chat Text");
                field.MaxLength = 93;
                @field.Changed = spades::ui::EventHandler(this.OnFieldChanged);
                AddChild(field);
            }
            {
                @globalButton = spades::ui::SimpleButton(Manager);
                globalButton.Toggle = true;
                globalButton.Toggled = isTeamChat == false;
                globalButton.Caption = _Tr("Client", "Global");
                globalButton.Bounds = AABB2(winX, winY + 36.f, 70.f, 30.f);
                @globalButton.Activated = spades::ui::EventHandler(this.OnSetGlobal);
                AddChild(globalButton);
            }
            {
                @teamButton = spades::ui::SimpleButton(Manager);
                teamButton.Toggle = true;
                teamButton.Toggled = isTeamChat == true;
                teamButton.Caption = _Tr("Client", "Team");
                teamButton.Bounds = AABB2(winX + 70.f, winY + 36.f, 70.f, 30.f);
                @teamButton.Activated = spades::ui::EventHandler(this.OnSetTeam);
                AddChild(teamButton);
            }
        }

        void UpdateState() { sayButton.Enable = field.Text.length > 0; }

        bool IsTeamChat {
            get final { return isTeamChat; }
            set {
                if (isTeamChat == value)
                    return;
                isTeamChat = value;
                teamButton.Toggled = isTeamChat;
                globalButton.Toggled = not isTeamChat;
                UpdateState();
            }
        }

        private void OnSetGlobal(spades::ui::UIElement @sender) { IsTeamChat = false; }
        private void OnSetTeam(spades::ui::UIElement @sender) { IsTeamChat = true; }

        private void OnFieldChanged(spades::ui::UIElement @sender) { UpdateState(); }

        private void Close() { @ui.ActiveUI = null; }

        private void OnCancel(spades::ui::UIElement @sender) {
            field.Cancelled();
            Close();
        }

        private bool CheckAndSetConfigVariable() {
            string text = field.Text;
            if (text.substr(0, 1) != "/")
                return false;
            int idx = text.findFirst(" ");
            if (idx < 2)
                return false;

            // find variable
            string varname = text.substr(1, idx - 1);
            string[] vars = GetAllConfigNames();

            for (uint i = 0, len = vars.length; i < len; i++) {
                if (vars[i].length == varname.length &&
                    StringCommonPrefixLength(vars[i], varname) == vars[i].length) {
                    // match
                    string val = text.substr(idx + 1);
                    ConfigItem item(vars[i]);
                    item.StringValue = val;
                    return true;
                }
            }

            return false;
        }

        private void OnSay(spades::ui::UIElement @sender) {
            field.CommandSent();
            if (!CheckAndSetConfigVariable()) {
                if (isTeamChat)
                    ui.helper.SayTeam(field.Text);
                else
                    ui.helper.SayGlobal(field.Text);
            } else {
                ui.helper.AlertWarning(_Tr(
                    "Client", "cvar editing via chat window is being phased out (see issue #842)"));
            }
            Close();
        }

        void HotKey(string key) {
            if (IsEnabled and key == "Escape") {
                OnCancel(this);
            } else if (IsEnabled and key == "Enter") {
                if (field.Text.length == 0) {
                    OnCancel(this);
                } else {
                    OnSay(this);
                }
            } else {
                UIElement::HotKey(key);
            }
        }
    }

}
