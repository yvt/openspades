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

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "CTFGameMode.h"
#include "Client.h"
#include "Fonts.h"
#include "IFont.h"
#include "IImage.h"
#include "IRenderer.h"
#include "MapView.h"
#include "NetClient.h"
#include "Player.h"
#include "ScoreboardView.h"
#include "TCGameMode.h"
#include "World.h"
#include <Core/Debug.h>
#include <Core/Settings.h>
#include <Core/Strings.h>

SPADES_SETTING(cg_minimapPlayerColor);

namespace spades {
	namespace client {

		static const Vector4 white = {1, 1, 1, 1};
		static const Vector4 spectatorIdColor = {210.f / 255, 210.f / 255, 210.f / 255, 1}; // Grey
		static const Vector4 spectatorTextColor = {220.f / 255, 220.f / 255, 0,
		                                           1}; // Goldish yellow
		static const auto spectatorTeamId = 255;       // Spectators have a team id of 255


		ScoreboardView::ScoreboardView(Client *client)
		    : client(client), renderer(client->GetRenderer()) {
			SPADES_MARK_FUNCTION();
			world = nullptr;
			tc = nullptr;
			ctf = nullptr;
			image = nullptr;

			// Use GUI font if spectator string has special chars
			auto spectatorString = _TrN("Client", "Spectator{1}", "Spectators{1}", "", "");
			auto has_special_char =
				std::find_if(spectatorString.begin(), spectatorString.end(),
				[](char ch) {
					return !(isalnum(static_cast<unsigned char>(ch)) || ch == '_');
			}) != spectatorString.end();

			spectatorFont = has_special_char ?
				client->fontManager->GetMediumFont() :
				client->fontManager->GetSquareDesignFont();
		}

		ScoreboardView::~ScoreboardView() {}

		int ScoreboardView::GetTeamScore(int team) const {
			if (ctf) {
				return ctf->GetTeam(team).score;
			} else if (tc) {
				int cnt = tc->GetNumTerritories();
				int num = 0;
				for (int i = 0; i < cnt; i++)
					if (tc->GetTerritory(i)->ownerTeamId == team)
						num++;
				return num;
			} else {
				return 0;
			}
		}

		Vector4 ScoreboardView::GetTeamColor(int team) {
			IntVector3 c = world->GetTeam(team).color;
			return MakeVector4(c.x / 255.f, c.y / 255.f, c.z / 255.f, 1.f);
		}

		Vector4 ScoreboardView::AdjustColor(spades::Vector4 col, float bright, float saturation) const {
			col.x *= bright;
			col.y *= bright;
			col.z *= bright;
			float avg = (col.x + col.y + col.z) / 3.f;
			col.x = avg + (col.x - avg) * saturation;
			col.y = avg + (col.y - avg) * saturation;
			col.z = avg + (col.z - avg) * saturation;

			return col;
		}
		static Vector4 ModifyColor(IntVector3 v) {
			Vector4 fv;
			fv.x = static_cast<float>(v.x) / 255.f;
			fv.y = static_cast<float>(v.y) / 255.f;
			fv.z = static_cast<float>(v.z) / 255.f;
			float avg = (fv.x + fv.y + fv.z) * (1.f / 3.f);
			;
			fv.x = Mix(fv.x, avg, 0.5f);
			fv.y = Mix(fv.y, avg, 0.5f);
			fv.z = Mix(fv.z, avg, 0.5f);
			fv.w = 0.f; // suppress "operating on garbase value" static analyzer message
			fv = fv * 0.8f + 0.2f;
			fv.w = 1.f;
			return fv;
		}
		void ScoreboardView::Draw() {
			SPADES_MARK_FUNCTION();

			world = client->GetWorld();
			if (!world) {
				// no world
				return;
			}

			IGameMode *mode = world->GetMode();
			ctf = IGameMode::m_CTF == mode->ModeType() ? static_cast<CTFGameMode *>(mode) : NULL;
			tc = IGameMode::m_TC == mode->ModeType() ? static_cast<TCGameMode *>(mode) : NULL;

			Handle<IImage> image;
			IFont *font;
			Vector2 pos, size;
			std::string str;
			float scrWidth = renderer->ScreenWidth();
			// float scrHeight = renderer->ScreenHeight();
			const Vector4 whiteColor = {1, 1, 1, 1};
			Handle<IImage> whiteImage = renderer->RegisterImage("Gfx/White.tga");

			float teamBarTop = 120.f;
			float teamBarHeight = 60.f;
			float contentsLeft = scrWidth * .5f - 400.f;
			float contentsRight = scrWidth * .5f + 400.f;
			float playersHeight = 300.f;
			float spectatorsHeight = 78.f;
			float playersTop = teamBarTop + teamBarHeight;
			float playersBottom = playersTop + playersHeight;

			// draw shadow
			image = renderer->RegisterImage("Gfx/Scoreboard/TopShadow.tga");
			size.y = 32.f;
			renderer->SetColorAlphaPremultiplied(MakeVector4(0, 0, 0, 0.2f));
			renderer->DrawImage(image, AABB2(0, teamBarTop - size.y, scrWidth, size.y));
			renderer->SetColorAlphaPremultiplied(MakeVector4(0, 0, 0, 0.2f));
			renderer->DrawImage(image, AABB2(0, playersBottom + size.y, scrWidth, -size.y));

			// draw team bar
			image = whiteImage;
			renderer->SetColorAlphaPremultiplied(AdjustColor(GetTeamColor(0), 0.8f, 0.3f));
			renderer->DrawImage(image, AABB2(0, teamBarTop, scrWidth * .5f, teamBarHeight));
			renderer->SetColorAlphaPremultiplied(AdjustColor(GetTeamColor(1), 0.8f, 0.3f));
			renderer->DrawImage(image,
			                    AABB2(scrWidth * .5f, teamBarTop, scrWidth * .5f, teamBarHeight));

			image = renderer->RegisterImage("Gfx/Scoreboard/Grunt.png");
			size.x = 120.f;
			size.y = 60.f;
			renderer->DrawImage(
			  image, AABB2(contentsLeft, teamBarTop + teamBarHeight - size.y, size.x, size.y));
			renderer->DrawImage(
			  image, AABB2(contentsRight, teamBarTop + teamBarHeight - size.y, -size.x, size.y));

			font = client->fontManager->GetSquareDesignFont();
			str = world->GetTeam(0).name;
			pos.x = contentsLeft + 110.f;
			pos.y = teamBarTop + 5.f;
			font->Draw(str, pos + MakeVector2(0, 2), 1.f, MakeVector4(0, 0, 0, 0.5));
			font->Draw(str, pos, 1.f, whiteColor);

			str = world->GetTeam(1).name;
			size = font->Measure(str);
			pos.x = contentsRight - 110.f - size.x;
			pos.y = teamBarTop + 5.f;
			font->Draw(str, pos + MakeVector2(0, 2), 1.f, MakeVector4(0, 0, 0, 0.5));
			font->Draw(str, pos, 1.f, whiteColor);

			// draw scores
			int capLimit;
			if (ctf) {
				capLimit = ctf->GetCaptureLimit();
			} else if (tc) {
				capLimit = tc->GetNumTerritories();
			} else {
				capLimit = -1;
			}
			if (capLimit != -1) {
				str = Format("{0}-{1}", GetTeamScore(0), capLimit);
				pos.x = scrWidth * .5f - font->Measure(str).x - 15.f;
				pos.y = teamBarTop + 5.f;
				font->Draw(str, pos, 1.f, Vector4(1.f, 1.f, 1.f, 0.5f));

				str = Format("{0}-{1}", GetTeamScore(1), capLimit);
				pos.x = scrWidth * .5f + 15.f;
				pos.y = teamBarTop + 5.f;
				font->Draw(str, pos, 1.f, Vector4(1.f, 1.f, 1.f, 0.5f));
			}

			// players background
			auto areSpectatorsPr = areSpectatorsPresent();
			image = renderer->RegisterImage("Gfx/Scoreboard/PlayersBg.png");
			renderer->SetColorAlphaPremultiplied(MakeVector4(0, 0, 0, 1.f));
			renderer->DrawImage(image,
			                    AABB2(0, playersTop, scrWidth,
			                          playersHeight + (areSpectatorsPr ? spectatorsHeight : 0)));

			// draw players
			DrawPlayers(0, contentsLeft, playersTop, (contentsRight - contentsLeft) * .5f,
			            playersHeight);
			DrawPlayers(1, scrWidth * .5f, playersTop, (contentsRight - contentsLeft) * .5f,
			            playersHeight);
			if (areSpectatorsPr)
				DrawSpectators(playersBottom, scrWidth * .5f);
		}

		struct ScoreboardEntry {
			int id;
			int score;
			std::string name;
			bool alive;
			bool operator<(const ScoreboardEntry &ent) const { return score > ent.score; }
		};

		void ScoreboardView::DrawPlayers(int team, float left, float top, float width,
		                                 float height) {
			IFont *font = client->fontManager->GetGuiFont();
			float rowHeight = 24.f;
			char buf[256];
			Vector2 size;
			Vector4 white = {1, 1, 1, 1};
			Vector4 gray = {0.5, 0.5, 0.5, 1};
			int maxRows = (int)floorf(height / rowHeight);
			int numPlayers = 0;
			int cols;
			std::vector<ScoreboardEntry> entries;

			for (int i = 0; i < world->GetNumPlayerSlots(); i++) {
				Player *p = world->GetPlayer(i);
				if (!p)
					continue;
				if (p->GetTeamId() != team)
					continue;

				ScoreboardEntry ent;
				ent.name = p->GetName();
				ent.score = world->GetPlayerPersistent(i).kills;
				ent.alive = p->IsAlive();
				ent.id = i;
				entries.push_back(ent);

				numPlayers++;
			}

			std::sort(entries.begin(), entries.end());

			cols = (numPlayers + maxRows - 1) / maxRows;
			if (cols == 0)
				cols = 1;
			maxRows = (numPlayers + cols - 1) / cols;

			int row = 0, col = 0;
			float colWidth = (float)width / (float)cols;
			extern int palette[32][3];
			std::string colormode = cg_minimapPlayerColor;
			for (int i = 0; i < numPlayers; i++) {
				ScoreboardEntry &ent = entries[i];

				float rowY = top + 6.f + row * rowHeight;
				float colX = left + width / (float)cols * (float)col;
				Vector4 color = white;
				if (ent.id == world->GetLocalPlayerIndex())
					color = GetTeamColor(team);

				sprintf(buf, "#%d", ent.id); // FIXME: 1-base?
				size = font->Measure(buf);

				if (colormode == "1") {
					IntVector3 Colorplayer =
					  IntVector3::Make(palette[ent.id][0], palette[ent.id][1], palette[ent.id][2]);
					Vector4 ColorplayerF = ModifyColor(Colorplayer);
					ColorplayerF *= 1.0f;
					font->Draw(buf, MakeVector2(colX + 35.f - size.x, rowY), 1.f, ColorplayerF);
				} else {
					font->Draw(buf, MakeVector2(colX + 35.f - size.x, rowY), 1.f, white);
				}

				color = ent.alive ? white : gray;

				font->Draw(ent.name, MakeVector2(colX + 45.f, rowY), 1.f, color);

				color = white;

				sprintf(buf, "%d", ent.score);
				size = font->Measure(buf);
				font->Draw(buf, MakeVector2(colX + colWidth - 10.f - size.x, rowY), 1.f, color);

				row++;
				if (row >= maxRows) {
					col++;
					row = 0;
				}
			}
		}

		void ScoreboardView::DrawSpectators(float top, float centerX) const {
			IFont *font = client->fontManager->GetGuiFont();
			char buf[256];
			std::vector<ScoreboardEntry> entries;

			static const auto xPixelSpectatorOffset = 20.f;

			int numSpectators = 0;
			float totalPixelWidth = 0;
			for (int i = 0; i < world->GetNumPlayerSlots(); i++) {
				Player *p = world->GetPlayer(i);
				if (!p)
					continue;
				if (p->GetTeamId() != spectatorTeamId)
					continue;

				ScoreboardEntry ent;
				ent.name = p->GetName();
				ent.id = i;
				entries.push_back(ent);

				numSpectators++;

				// Measure total width in pixels so that we can center align all the spectators
				sprintf(buf, "#%d", ent.id);
				totalPixelWidth +=
				  font->Measure(buf).x + font->Measure(ent.name).x + xPixelSpectatorOffset;
			}
			if (numSpectators == 0) {
				return;
			}

			strcpy(buf,
			       _TrN("Client", "Spectator{1}", "Spectators{1}", numSpectators, ":").c_str());

			auto isSquareFont = spectatorFont == client->fontManager->GetSquareDesignFont();
			auto sizeSpecString = spectatorFont->Measure(buf);
			spectatorFont->Draw(buf,
				MakeVector2(centerX - sizeSpecString.x / 2, top + (isSquareFont ? 0 : 10)),
				1.f,
				spectatorTextColor);

			auto yOffset = top + sizeSpecString.y;
			auto halfTotalX = totalPixelWidth / 2;
			auto currentXoffset = centerX - halfTotalX;

			for (int i = 0; i < numSpectators; i++) {
				ScoreboardEntry &ent = entries[i];

				sprintf(buf, "#%d", ent.id);
				font->Draw(buf, MakeVector2(currentXoffset, yOffset), 1.f, spectatorIdColor);

				auto sizeName = font->Measure(ent.name);
				auto sizeID = font->Measure(buf);
				font->Draw(ent.name, MakeVector2(currentXoffset + sizeID.x + 5.f, yOffset), 1.f,
				           white);

				currentXoffset += sizeID.x + sizeName.x + xPixelSpectatorOffset;
			}
		}

		bool ScoreboardView::areSpectatorsPresent() const {
			for (auto i = 0; i < client->GetWorld()->GetNumPlayerSlots(); i++) {
				auto *p = world->GetPlayer(i);
				if (p && p->GetTeamId() == spectatorTeamId)
					return true;
			}
			return false;
		}
	}
}
