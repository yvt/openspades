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

#include <cctype>

#include "ChatWindow.h"
#include "Client.h"
#include "IFont.h"
#include "IRenderer.h"
#include "World.h"
#include <Core/Debug.h>
#include <Core/Exception.h>
#include <Core/Math.h>
#include <Core/Settings.h>

DEFINE_SPADES_SETTING(cg_chatHeight, "30");
DEFINE_SPADES_SETTING(cg_killfeedHeight, "26");

namespace spades {
	namespace client {

		ChatWindow::ChatWindow(Client *cli, IRenderer *rend, IFont *fnt, bool killfeed)
		    : client(cli), renderer(rend), font(fnt), killfeed(killfeed) {
			firstY = 0.f;
		}
		ChatWindow::~ChatWindow() {}

		float ChatWindow::GetWidth() { return renderer->ScreenWidth() / 2; }

		float ChatWindow::GetNormalHeight() {
			float prop = killfeed ? (float)cg_killfeedHeight : (float)cg_chatHeight;

			return renderer->ScreenHeight() * prop * 0.01f;
		}

		float ChatWindow::GetBufferHeight() {
			if (killfeed) {
				return GetNormalHeight();
			} else {
				// Take up the remaining height
				float prop = 100.0f - (float)cg_killfeedHeight;

				return renderer->ScreenHeight() * prop * 0.01f - 100.0f;
			}
		}

		float ChatWindow::GetLineHeight() { return 20.f; }

		static bool isWordChar(char c) { return isalnum(c) || c == '\''; }

		std::string ChatWindow::killImage(int type, int weapon) {
			std::string tmp = "xx";
			tmp[0] = 7;
			switch (type) {
				case KillTypeWeapon:
					switch (weapon) {
						case 0:
						case 1:
						case 2: tmp[1] = 'a' + weapon; break;
						default: return "";
					}
					break;
				case KillTypeHeadshot:
				case KillTypeMelee:
				case KillTypeGrenade:
				case KillTypeFall:
				case KillTypeTeamChange:
				case KillTypeClassChange: tmp[1] = 'a' + 2 + type; break;
				default: return "";
			}
			return tmp;
		}

		void ChatWindow::AddMessage(const std::string &msg) {
			SPADES_MARK_FUNCTION();

			// get visible message string
			std::string str;
			float x = 0.f, maxW = GetWidth();
			float lh = GetLineHeight(), h = lh;
			size_t wordStart = std::string::npos;
			size_t wordStartOutPos = 0;

			for (size_t i = 0; i < msg.size(); i++) {
				if (msg[i] > MsgColorMax && msg[i] != 13 && msg[i] != 10) {
					if (isWordChar(msg[i])) {
						if (wordStart == std::string::npos) {
							wordStart = msg.size();
							wordStartOutPos = str.size();
						}
					} else {
						wordStart = std::string::npos;
					}

					float w = font->Measure(std::string(&msg[i], 1)).x;
					if (x + w > maxW) {
						if (wordStart != std::string::npos && wordStart != str.size()) {
							// adding a part of word.
							// do word wrapping
							std::string s = msg.substr(wordStart, i - wordStart + 1);
							float nw = font->Measure(s).x;
							if (nw <= maxW) {
								// word wrap succeeds
								w = nw;
								x = w;
								h += lh;
								str.insert(wordStartOutPos, "\n");

								goto didWordWrap;
							}
						}
						x = 0;
						h += lh;
						str += 13;
					}
					x += w;
					str += msg[i];
				didWordWrap:;
				} else if (msg[i] == 13 || msg[i] == 10) {
					x = 0;
					h += lh;
					str += 13;
				} else {
					str += msg[i];
				}
			}

			entries.push_front(ChatEntry(msg, h, 15.f));

			firstY -= h;
		}

		std::string ChatWindow::ColoredMessage(const std::string &msg, char c) {
			SPADES_MARK_FUNCTION_DEBUG();
			std::string s;
			s += c;
			s += msg;
			s += MsgColorRestore;
			return s;
		}

		std::string ChatWindow::TeamColorMessage(const std::string &msg, int team) {
			SPADES_MARK_FUNCTION_DEBUG();
			switch (team) {
				case 0: return ColoredMessage(msg, MsgColorTeam1);
				case 1: return ColoredMessage(msg, MsgColorTeam2);
				case 2: return ColoredMessage(msg, MsgColorTeam3);
				default: return msg;
			}
		}

		static Vector4 ConvertColor(IntVector3 v) {
			return MakeVector4((float)v.x / 255.f, (float)v.y / 255.f, (float)v.z / 255.f, 1.f);
		}

		Vector4 ChatWindow::GetColor(char c) {
			World *w = client ? client->GetWorld() : NULL;
			switch (c) {
				case MsgColorTeam1:
					return w ? ConvertColor(w->GetTeam(0).color) : MakeVector4(0, 1, 0, 1);
				case MsgColorTeam2:
					return w ? ConvertColor(w->GetTeam(1).color) : MakeVector4(0, 0, 1, 1);
				case MsgColorTeam3:
					return w ? ConvertColor(w->GetTeam(2).color) : MakeVector4(1, 1, 0, 1);
				case MsgColorRed: return MakeVector4(1, 0, 0, 1);
				case MsgColorGray: return MakeVector4(0.5f, 0.5f, 0.5f, 1);
				case MsgColorSysInfo: return MakeVector4(0, 1, 0, 1);
				default: return MakeVector4(1, 1, 1, 1);
			}
		}

		void ChatWindow::Update(float dt) {

			if (firstY < 0.f) {
				firstY += dt * std::max(100.f, -firstY);
				if (firstY > 0.f)
					firstY = 0.f;
			}

			float normalHeight = GetNormalHeight();
			float bufferHeight = GetBufferHeight();
			float y = firstY;

			for (std::list<ChatEntry>::iterator it = entries.begin(); it != entries.end();) {
				ChatEntry &ent = *it;
				if (y + ent.height > bufferHeight) {
					ent.bufferFade -= dt * 4.f;
					if (ent.bufferFade < 0.f) {
						// evict from the buffer
						std::list<ChatEntry>::iterator er = it++;
						entries.erase(er);
						continue;
					}
				}

				if (y + ent.height > normalHeight) {
					ent.fade = std::max(ent.fade - dt * 4.f, 0.0f);
				} else if (y + ent.height > 0.f) {
					ent.fade = std::min(ent.fade + dt * 4.f, 1.0f);
					ent.bufferFade = std::min(ent.bufferFade + dt * 4.f, 1.0f);
				}

				ent.timeFade -= dt;
				if (ent.timeFade < 0.f) {
					ent.timeFade = 0.f;
				}

				y += ent.height;
				++it;
			}
		}

		void ChatWindow::Draw() {
			SPADES_MARK_FUNCTION();

			float winH = expanded ? GetBufferHeight() : GetNormalHeight();

			float winX = 4.f;
			float winY = killfeed ? 8.f : renderer->ScreenHeight() - winH - 60.f;
			std::list<ChatEntry>::iterator it;

			float lHeight = GetLineHeight();

			float y = firstY;

			Vector4 shadowColor = {0, 0, 0, 0.8f};
			Vector4 brightShadowColor = {1, 1, 1, 0.8f};

			std::string ch = "aaaaaa"; // let's not make a new object for each character.
			// note: UTF-8's longest character is 6 bytes

			if (expanded) {
				// Draw a box behind text when expanded
				Handle<IImage> whiteImage = renderer->RegisterImage("Gfx/White.tga");
				renderer->SetColorAlphaPremultiplied(MakeVector4(0, 0, 0, 0.2f));
				renderer->DrawImage(whiteImage, AABB2(0, winY, GetWidth(), winH));
			}

			for (it = entries.begin(); it != entries.end(); ++it) {
				ChatEntry &ent = *it;

				const std::string &msg = ent.msg;
				Vector4 color = GetColor(MsgColorRestore);

				float tx = 0.f, ty = y;

				float fade = ent.fade;

				if (expanded) {
					// Display out-dated messages when expanded
					fade = ent.bufferFade;
				} else {
					if (ent.timeFade < 1.f) {
						fade *= ent.timeFade;
					}
				}

				if (fade < 0.01f) {
					// Skip rendering invisible messages
					goto endDrawLine;
				}

				brightShadowColor.w = shadowColor.w = .8f * fade;

				color.w = fade;
				for (size_t i = 0; i < msg.size(); i++) {
					if (msg[i] == 13 || msg[i] == 10) {
						tx = 0.f;
						ty += lHeight;
					} else if (msg[i] <= MsgColorMax && msg[i] >= 1) {
						color = GetColor(msg[i]);
						color.w = fade;
					} else {
						size_t ln = 0;
						GetCodePointFromUTF8String(msg, i, &ln);
						ch.resize(ln);
						for (size_t k = 0; k < ln; k++)
							ch[k] = msg[i + k];
						i += ln - 1;

						float luminosity = color.x + color.y + color.z;

						font->DrawShadow(ch, MakeVector2(tx + winX, ty + winY), 1.f, color,
						                 luminosity > 0.9f ? shadowColor : brightShadowColor);
						tx += font->Measure(ch).x;
					}
				}

			endDrawLine:
				y += ent.height;
			}
		}
	}
}
