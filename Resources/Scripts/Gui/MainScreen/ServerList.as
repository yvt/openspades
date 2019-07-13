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

#include "CountryFlags.as"

namespace spades {

    class ServerListItem : spades::ui::ButtonBase {
        MainScreenServerItem @item;
        FlagIconRenderer @flagIconRenderer;
        ServerListItem(spades::ui::UIManager @manager, MainScreenServerItem @item) {
            super(manager);
            @this.item = item;
            @flagIconRenderer = FlagIconRenderer(manager.Renderer);
        }
        void Render() {
            Renderer @renderer = Manager.Renderer;
            Vector2 pos = ScreenPosition;
            Vector2 size = Size;
            Image @img = renderer.RegisterImage("Gfx/White.tga");

            Vector4 bgcolor = Vector4(1.f, 1.f, 1.f, 0.0f);
            Vector4 fgcolor = Vector4(1.f, 1.f, 1.f, 1.f);
            if (item.Favorite) {
                bgcolor = Vector4(0.3f, 0.3f, 1.f, 0.1f);
                fgcolor = Vector4(220.f / 255.f, 220.f / 255.f, 0, 1);
            }
            if (Pressed && Hover) {
                bgcolor.w += 0.3;
            } else if (Hover) {
                bgcolor.w += 0.15;
            }
            renderer.ColorNP = bgcolor;
            renderer.DrawImage(img, AABB2(pos.x, pos.y, size.x, size.y));

            Font.Draw(item.Name, ScreenPosition + Vector2(4.f, 2.f), 1.f, fgcolor);
            string playersStr = ToString(item.NumPlayers) + "/" + ToString(item.MaxPlayers);
            Vector4 col(1, 1, 1, 1);
            if (item.NumPlayers >= item.MaxPlayers)
                col = Vector4(1, 0.7f, 0.7f, 1);
            else if (item.NumPlayers >= item.MaxPlayers * 3 / 4)
                col = Vector4(1, 1, 0.7f, 1);
            else if (item.NumPlayers == 0)
                col = Vector4(0.7f, 0.7f, 1, 1);
            Font.Draw(playersStr,
                      ScreenPosition + Vector2(340.f - Font.Measure(playersStr).x * 0.5f, 2.f), 1.f,
                      col);
            Font.Draw(item.MapName, ScreenPosition + Vector2(400.f, 2.f), 1.f, Vector4(1, 1, 1, 1));
            Font.Draw(item.GameMode, ScreenPosition + Vector2(550.f, 2.f), 1.f,
                      Vector4(1, 1, 1, 1));
            Font.Draw(item.Protocol, ScreenPosition + Vector2(630.f, 2.f), 1.f,
                      Vector4(1, 1, 1, 1));
            if (not flagIconRenderer.DrawIcon(item.Country,
                                              ScreenPosition + Vector2(700.f, size.y * 0.5f))) {
                Font.Draw(item.Country, ScreenPosition + Vector2(680.f, 2.f), 1.f,
                          Vector4(1, 1, 1, 1));
            }
        }
    }

    funcdef void ServerListItemEventHandler(ServerListModel @sender, MainScreenServerItem @item);

    class ServerListModel : spades::ui::ListViewModel {
        spades::ui::UIManager @manager;
        MainScreenServerItem @[] @list;

        ServerListItemEventHandler @ItemActivated;
        ServerListItemEventHandler @ItemDoubleClicked;
        ServerListItemEventHandler @ItemRightClicked;

        ServerListModel(spades::ui::UIManager @manager, MainScreenServerItem @[] @list) {
            @this.manager = manager;
            @this.list = list;
        }
        int NumRows {
            get { return int(list.length); }
        }
        private void OnItemClicked(spades::ui::UIElement @sender) {
            ServerListItem @item = cast<ServerListItem>(sender);
            if (ItemActivated !is null) {
                ItemActivated(this, item.item);
            }
        }
        private void OnItemDoubleClicked(spades::ui::UIElement @sender) {
            ServerListItem @item = cast<ServerListItem>(sender);
            if (ItemDoubleClicked !is null) {
                ItemDoubleClicked(this, item.item);
            }
        }
        private void OnItemRightClicked(spades::ui::UIElement @sender) {
            ServerListItem @item = cast<ServerListItem>(sender);
            if (ItemRightClicked !is null) {
                ItemRightClicked(this, item.item);
            }
        }
        spades::ui::UIElement @CreateElement(int row) {
            ServerListItem i(manager, list[row]);
            @i.Activated = spades::ui::EventHandler(this.OnItemClicked);
            @i.DoubleClicked = spades::ui::EventHandler(this.OnItemDoubleClicked);
            @i.RightClicked = spades::ui::EventHandler(this.OnItemRightClicked);
            return i;
        }
        void RecycleElement(spades::ui::UIElement @elem) {}
    }

    class ServerListHeader : spades::ui::ButtonBase {
        string Text;
        ServerListHeader(spades::ui::UIManager @manager) { super(manager); }
        void OnActivated() { ButtonBase::OnActivated(); }
        void Render() {
            Renderer @renderer = Manager.Renderer;
            Vector2 pos = ScreenPosition;
            Vector2 size = Size;
            Image @img = renderer.RegisterImage("Gfx/White.tga");
            if (Pressed && Hover) {
                renderer.ColorNP = Vector4(1.f, 1.f, 1.f, 0.3f);
            } else if (Hover) {
                renderer.ColorNP = Vector4(1.f, 1.f, 1.f, 0.15f);
            } else {
                renderer.ColorNP = Vector4(1.f, 1.f, 1.f, 0.0f);
            }
            renderer.DrawImage(img, AABB2(pos.x - 2.f, pos.y, size.x, size.y));

            Font.Draw(Text, ScreenPosition + Vector2(0.f, 2.f), 1.f, Vector4(1, 1, 1, 1));
        }
    }

    class MainScreenServerListLoadingView : spades::ui::UIElement {
        MainScreenServerListLoadingView(spades::ui::UIManager @manager) { super(manager); }
        void Render() {
            Renderer @renderer = Manager.Renderer;
            Vector2 pos = ScreenPosition;
            Vector2 size = Size;
            Font @font = this.Font;
            string text = _Tr("MainScreen", "Loading...");
            Vector2 txtSize = font.Measure(text);
            Vector2 txtPos;
            txtPos = pos + (size - txtSize) * 0.5f;

            font.Draw(text, txtPos, 1.f, Vector4(1, 1, 1, 0.8));
        }
    }

    class MainScreenServerListErrorView : spades::ui::UIElement {
        MainScreenServerListErrorView(spades::ui::UIManager @manager) { super(manager); }
        void Render() {
            Renderer @renderer = Manager.Renderer;
            Vector2 pos = ScreenPosition;
            Vector2 size = Size;
            Font @font = this.Font;
            string text = _Tr("MainScreen", "Failed to fetch the server list.");
            Vector2 txtSize = font.Measure(text);
            Vector2 txtPos;
            txtPos = pos + (size - txtSize) * 0.5f;

            font.Draw(text, txtPos, 1.f, Vector4(1, 1, 1, 0.8));
        }
    }

}
