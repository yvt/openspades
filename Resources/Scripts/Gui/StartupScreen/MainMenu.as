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

#include "UpdateCheckView.as"
#include "ConfigViewTabs.as"

namespace spades {

    class StartupScreenMainMenuState {
        int ActiveTabIndex;
    }

    class StartupScreenMainMenu: spades::ui::UIElement {

        StartupScreenUI@ ui;
        StartupScreenHelper@ helper;

        spades::ui::ListView@ serverList;
        spades::ui::CheckBox@ bypassStartupWindowCheck;

        StartupScreenGraphicsTab@ graphicsTab;
        StartupScreenAudioTab@ audioTab;
        StartupScreenGenericTab@ genericTab;
        StartupScreenSystemInfoTab@ systemInfoTab;
        StartupScreenAdvancedTab@ advancedTab;

        private ConfigItem cl_showStartupWindow("cl_showStartupWindow", "-1");
        private bool advancedTabVisible = false;

        StartupScreenMainMenu(StartupScreenUI@ ui) {
            super(ui.manager);
            @this.ui = ui;
            @this.helper = ui.helper;

            @this.Font = ui.fontManager.GuiFont;

            float width = Manager.Renderer.ScreenWidth;
            float height = Manager.Renderer.ScreenHeight;
            {
                spades::ui::Button button(Manager);
                button.Caption = _Tr("StartupScreen", "Start");
                button.Bounds = AABB2(width - 170.f, 20.f, 150.f, 30.f);
                @button.Activated = spades::ui::EventHandler(this.OnStartPressed);
                AddChild(button);
            }
            {
                spades::ui::CheckBox button(Manager);
                button.Caption = _Tr("StartupScreen", "Skip this screen next time");
                button.Bounds = AABB2(360.f, 62.f, width - 380.f, 20.f); // note: this is updated later soon
                AddChild(button);
                @bypassStartupWindowCheck = button;
                @button.Activated = spades::ui::EventHandler(this.OnBypassStartupWindowCheckChanged);
            }
            {
                UpdateCheckView view(Manager, ui.helper.PackageUpdateManager);
                view.Bounds = AABB2(0.f, height - 40.f, width, 40.f);
                @view.OpenUpdateInfoURL = spades::ui::EventHandler(this.OnShowUpdateDetailsPressed);
                AddChild(view);
            }

            AABB2 clientArea(10.f, 100.f, width - 20.f, height - 150.f);
            StartupScreenGraphicsTab graphicsTab(ui, clientArea.max - clientArea.min);
            StartupScreenAudioTab audioTab(ui, clientArea.max - clientArea.min);
            StartupScreenGenericTab genericTab(ui, clientArea.max - clientArea.min);
            StartupScreenSystemInfoTab profileTab(ui, clientArea.max - clientArea.min);
            StartupScreenAdvancedTab advancedTab(ui, clientArea.max - clientArea.min);
            graphicsTab.Bounds = clientArea;
            audioTab.Bounds = clientArea;
            genericTab.Bounds = clientArea;
            profileTab.Bounds = clientArea;
            advancedTab.Bounds = clientArea;
            AddChild(graphicsTab);
            AddChild(audioTab);
            AddChild(genericTab);
            AddChild(profileTab);
            AddChild(advancedTab);
            audioTab.Visible = false;
            profileTab.Visible = false;
            genericTab.Visible = false;
            advancedTab.Visible = false;

            @this.graphicsTab = graphicsTab;
            @this.audioTab = audioTab;
            @this.advancedTab = advancedTab;
            @this.systemInfoTab = profileTab;
            @this.genericTab = genericTab;

            {
                spades::ui::SimpleTabStrip tabStrip(Manager);
                AddChild(tabStrip);
                tabStrip.Bounds = AABB2(10.f, 70.f, width - 20.f, 24.f);
                tabStrip.AddItem(_Tr("StartupScreen", "Graphics"), graphicsTab);
                tabStrip.AddItem(_Tr("StartupScreen", "Audio"), audioTab);
                tabStrip.AddItem(_Tr("StartupScreen", "Generic"), genericTab);
                tabStrip.AddItem(_Tr("StartupScreen", "System Info"), profileTab);
                tabStrip.AddItem(_Tr("StartupScreen", "Advanced"), advancedTab);
                @tabStrip.Changed = spades::ui::EventHandler(this.OnTabChanged);

                // Reposition the "Skip this screen next time" check box
                spades::ui::UIElement@[]@ tabStripItems = tabStrip.GetChildren();
                float right = tabStripItems[tabStripItems.length - 1].Bounds.max.x +
                    tabStrip.Bounds.min.x + 10.f;
                bypassStartupWindowCheck.Bounds = AABB2(right, 62.f, width - right - 20.f, 20.f);
            }

            LoadConfig();
        }

        private void OnTabChanged(spades::ui::UIElement@) {
            bool v = advancedTab.Visible;
            if(advancedTabVisible and not v) {
                LoadConfig();
            }
            advancedTabVisible = v;
        }

        void LoadConfig() {
            switch(cl_showStartupWindow.IntValue) {
            case -1:
                bypassStartupWindowCheck.Toggled = false;
                break;
            case 0:
                bypassStartupWindowCheck.Toggled = true;
                break;
            default:
                bypassStartupWindowCheck.Toggled = false;
                break;
            }

            this.graphicsTab.LoadConfig();
            this.audioTab.LoadConfig();
            this.genericTab.LoadConfig();
            this.advancedTab.LoadConfig();
        }

        StartupScreenMainMenuState@ GetState() {
            StartupScreenMainMenuState state;
            if (this.graphicsTab.Visible) {
                state.ActiveTabIndex = 0;
            } else if (this.audioTab.Visible) {
                state.ActiveTabIndex = 1;
            } else if (this.genericTab.Visible) {
                state.ActiveTabIndex = 2;
            } else if (this.systemInfoTab.Visible) {
                state.ActiveTabIndex = 3;
            } else if (this.advancedTab.Visible) {
                state.ActiveTabIndex = 4;
            }
            return state;
        }

        void SetState(StartupScreenMainMenuState@ state) {
            this.graphicsTab.Visible = state.ActiveTabIndex == 0;
            this.audioTab.Visible = state.ActiveTabIndex == 1;
            this.genericTab.Visible = state.ActiveTabIndex == 2;
            this.systemInfoTab.Visible = state.ActiveTabIndex == 3;
            this.advancedTab.Visible = state.ActiveTabIndex == 4;
        }

        private void OnBypassStartupWindowCheckChanged(spades::ui::UIElement@ sender) {
            cl_showStartupWindow.IntValue = (bypassStartupWindowCheck.Toggled ? 0 : 1);
        }

        private void OnShowUpdateDetailsPressed(spades::ui::UIElement@ sender) {
            if (ui.helper.OpenUpdateInfoURL()) {
                return;
            }

            string msg = _Tr("StartupScreen", "An unknown error has occurred while opening the update info website.");
            msg += "\n\n" + ui.helper.PackageUpdateManager.LatestVersionInfoPageURL;
            AlertScreen al(Parent, msg, 100.f);
            al.Run();
        }

        private void OnQuitPressed(spades::ui::UIElement@ sender) {
            ui.shouldExit = true;
        }

        private void OnSetupPressed(spades::ui::UIElement@ sender) {
            PreferenceView al(this, PreferenceViewOptions(), ui.fontManager);
            al.Run();
        }

        private void Start() {
            helper.Start();
            ui.shouldExit = true; // we have to exit from the startup screen to start the game
        }

        private void OnStartPressed(spades::ui::UIElement@ sender) {
            Start();
        }

        void HotKey(string key) {
            if(IsEnabled and key == "Enter") {
                Start();
            } else if(IsEnabled and key == "Escape") {
                ui.shouldExit = true;
            } else {
                UIElement::HotKey(key);
            }
        }

        void Render() {
            UIElement::Render();

        }
    }

}
