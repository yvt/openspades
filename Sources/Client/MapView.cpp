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

#include <utility>

#include "CTFGameMode.h"
#include "Client.h"
#include "GameMap.h"
#include "IImage.h"
#include "IRenderer.h"
#include "MapView.h"
#include "Player.h"
#include "TCGameMode.h"
#include "Weapon.h"
#include "World.h"
#include <Core/Settings.h>
#include <Core/TMPUtils.h>

DEFINE_SPADES_SETTING(cg_minimapSize, "128");
DEFINE_SPADES_SETTING(cg_minimapPlayerColor, "1");
DEFINE_SPADES_SETTING(cg_minimapPlayerIcon, "1");

using std::pair;
using stmp::optional;

namespace spades {
	namespace client {
		namespace {
			optional<pair<Vector2, Vector2>> ClipLineSegment(const pair<Vector2, Vector2> &inLine,
			                                                 const Plane2 &plane) {
				const float distance1 = plane.GetDistanceTo(inLine.first);
				const float distance2 = plane.GetDistanceTo(inLine.second);
				int bits = distance1 > 0 ? 1 : 0;
				bits |= distance2 > 0 ? 2 : 0;
				switch (bits) {
					case 0: return {};
					case 3: return inLine;
				}

				const float fraction = distance1 / (distance1 - distance2);
				Vector2 intersection = Mix(inLine.first, inLine.second, fraction);
				if (bits == 1) {
					return std::make_pair(inLine.first, intersection);
				} else {
					return std::make_pair(intersection, inLine.second);
				}
			}

			optional<pair<Vector2, Vector2>> ClipLineSegment(const pair<Vector2, Vector2> &inLine,
			                                                 const AABB2 &rect) {
				optional<pair<Vector2, Vector2>> line =
				  ClipLineSegment(inLine, Plane2{1.0f, 0.0f, -rect.GetMinX()});
				if (!line) {
					return line;
				}
				line = ClipLineSegment(*line, Plane2{-1.0f, 0.0f, rect.GetMaxX()});
				if (!line) {
					return line;
				}
				line = ClipLineSegment(*line, Plane2{0.0f, 1.0f, -rect.GetMinY()});
				if (!line) {
					return line;
				}
				line = ClipLineSegment(*line, Plane2{0.0f, -1.0f, rect.GetMaxY()});
				return line;
			}
		}

		MapView::MapView(Client *c, bool largeMap)
		    : client(c), renderer(c->GetRenderer()), largeMap(largeMap) {
			scaleMode = 2;
			actualScale = 1.f;
			lastScale = 1.f;
			zoomed = false;
			zoomState = 0.f;
		}

		MapView::~MapView() {}

		void MapView::Update(float dt) {
			float scale = 0.0f;
			switch (scaleMode) {
				case 0: // 400%
					scale = 1.f / 4.f;
					break;
				case 1: // 200%
					scale = 1.f / 2.f;
					break;
				case 2: // 100%
					scale = 1.f;
					break;
				case 3: // 50%
					scale = 2.f;
					break;
				default: SPAssert(false);
			}
			if (actualScale != scale) {
				float spd = fabsf(scale - lastScale) * 10.f;
				spd = std::max(spd, 0.2f);
				spd *= dt;
				if (scale > actualScale) {
					actualScale += spd;
					if (actualScale > scale)
						actualScale = scale;
				} else {
					actualScale -= spd;
					if (actualScale < scale)
						actualScale = scale;
				}
			}

			if (zoomed) {
				zoomState = 1.0f;
			} else {
				zoomState = 0.0f;
			}
		}

		Vector2 MapView::Project(const Vector2 &pos) const {
			Vector2 scrPos;
			scrPos.x = (pos.x - inRect.GetMinX()) / inRect.GetWidth();
			scrPos.x = (scrPos.x * outRect.GetWidth()) + outRect.GetMinX();
			scrPos.y = (pos.y - inRect.GetMinY()) / inRect.GetHeight();
			scrPos.y = (scrPos.y * outRect.GetHeight()) + outRect.GetMinY();
			return scrPos;
		}

		void MapView::DrawIcon(spades::Vector3 pos, spades::client::IImage *img, float rotation) {
			if (pos.x < inRect.GetMinX() || pos.x > inRect.GetMaxX() || pos.y < inRect.GetMinY() ||
			    pos.y > inRect.GetMaxY())
				return;

			Vector2 scrPos = Project(Vector2{pos.x, pos.y});

			scrPos.x = floorf(scrPos.x);
			scrPos.y = floorf(scrPos.y);

			float c = rotation != 0.f ? cosf(rotation) : 1.f;
			float s = rotation != 0.f ? sinf(rotation) : 0.f;
			static const float coords[][2] = {{-1, -1}, {1, -1}, {-1, 1}};
			Vector2 u = MakeVector2(img->GetWidth() * .5f, 0.f);
			Vector2 v = MakeVector2(0.f, img->GetHeight() * .5f);

			Vector2 vt[3];
			for (int i = 0; i < 3; i++) {
				Vector2 ss = u * coords[i][0] + v * coords[i][1];
				vt[i].x = scrPos.x + ss.x * c - ss.y * s;
				vt[i].y = scrPos.y + ss.x * s + ss.y * c;
			}

			renderer->DrawImage(img, vt[0], vt[1], vt[2],
			                    AABB2(0, 0, img->GetWidth(), img->GetHeight()));
		}

		void MapView::SwitchScale() {
			scaleMode = (scaleMode + 1) % 4;

			lastScale = actualScale;
		}

		bool MapView::ToggleZoom() {
			zoomed = !zoomed;
			return zoomed;
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

		// definite a palette of 32 color in RGB code
		int palette[32][3] = {
		  {0, 0, 0},       // 0 black
		  {255, 255, 255}, // 1 white
		  {128, 128, 128}, // 2 grey
		  {255, 255, 0},   // 3 yellow
		  {0, 255, 255},   // 4 cyan-acqua
		  {255, 0, 255},   // 5 fuchsia
		  {255, 0, 0},     // 6 red
		  {0, 255, 0},     // 7 lime
		  {0, 0, 255},     // 8 blue

		  {128, 0, 0},   // 9 maroon
		  {0, 128, 0},   // 10 green
		  {0, 0, 128},   // 11 navy
		  {128, 128, 0}, // 12 olive
		  {128, 0, 128}, // 13 purple
		  {0, 128, 128}, // 14 teal

		  {255, 128, 0}, // 15 orange
		  {255, 0, 128}, // 16 deep pink
		  {128, 0, 255}, // 17 violet
		  {0, 128, 255}, // 18 bluette
		  {128, 255, 0}, // 19 lime 2
		  {0, 255, 128}, // 20 spring green

		  {255, 128, 128}, // 21 light pink
		  {128, 255, 128}, // 22 light spring
		  {128, 128, 255}, // 23 light blue
		  {128, 255, 255}, // 24 light azure
		  {255, 255, 128}, // 25 light yellow
		  {255, 128, 255}, // 26 light pink2

		  {165, 42, 42},  // 27 brown
		  {255, 69, 0},   // 28 orange	red
		  {255, 165, 0},  // 29  orange
		  {139, 69, 19},  // 30 maroon medium
		  {210, 105, 30}, // 31 choccolate
		};

		void MapView::Draw() {
			World *world = client->GetWorld();
			if (!world)
				return;

			// The player to focus on
			Player *focusPlayerPtr = nullptr;
			Vector3 focusPlayerPos;
			float focusPlayerAngle;

			if (HasTargetPlayer(client->GetCameraMode())) {
				Player &player = client->GetCameraTargetPlayer();
				Vector3 front = player.GetFront2D();

				focusPlayerPos = player.GetPosition();
				focusPlayerAngle = atan2(front.x, -front.y);

				focusPlayerPtr = &player;
			} else if (client->GetCameraMode() == ClientCameraMode::Free) {
				focusPlayerPos = client->freeCameraState.position;
				focusPlayerAngle = client->followAndFreeCameraState.yaw - static_cast<float>(M_PI) * .5f;
				focusPlayerPtr = world->GetLocalPlayer();
			} else {
				return;
			}

			// The local player (this is important for access control)
			if (!world->GetLocalPlayer()) {
				return;
			}
			Player &localPlayer = *world->GetLocalPlayer();

			SPAssert(focusPlayerPtr);
			Player &focusPlayer = *focusPlayerPtr;

			if (largeMap)
				if (zoomState < .0001f)
					return;

			GameMap *map = world->GetMap();
			Vector2 mapSize = MakeVector2(map->Width(), map->Height());

			Vector2 center = {focusPlayerPos.x, focusPlayerPos.y};
			float cfgMapSize = cg_minimapSize;
			if (cfgMapSize < 32)
				cfgMapSize = 32;
			if (cfgMapSize > 256)
				cfgMapSize = 256;
			Vector2 mapWndSize = {cfgMapSize, cfgMapSize};
			float scale = actualScale;

			center = Mix(center, mapSize * .5f, zoomState);

			Vector2 zoomedSize = {512, 512};
			if (renderer->ScreenWidth() < 512.f || renderer->ScreenHeight() < 512.f)
				zoomedSize = MakeVector2(256, 256);
			if (largeMap) {
				float per = zoomState;
				per = 1.f - per;
				per *= per;
				per = 1.f - per;
				per = Mix(.7f, 1.f, per);
				zoomedSize = Mix(MakeVector2(0, 0), zoomedSize, per);
				mapWndSize = zoomedSize;
			}

			Vector2 inRange = mapWndSize * .5f * scale;
			AABB2 inRect(center - inRange, center + inRange);
			if (largeMap) {
				inRect.min = MakeVector2(0, 0);
				inRect.max = mapSize;
			} else {
				if (inRect.GetMinX() < 0.f)
					inRect = inRect.Translated(-inRect.GetMinX(), 0.f);
				if (inRect.GetMinY() < 0.f)
					inRect = inRect.Translated(0, -inRect.GetMinY());
				if (inRect.GetMaxX() > mapSize.x)
					inRect = inRect.Translated(mapSize.x - inRect.GetMaxX(), 0.f);
				if (inRect.GetMaxY() > mapSize.y)
					inRect = inRect.Translated(0, mapSize.y - inRect.GetMaxY());
			}

			AABB2 outRect(renderer->ScreenWidth() - mapWndSize.x - 16.f, 16.f, mapWndSize.x,
			              mapWndSize.y);
			if (largeMap) {
				outRect.min = MakeVector2((renderer->ScreenWidth() - zoomedSize.x) * .5f,
				                          (renderer->ScreenHeight() - zoomedSize.y) * .5f);
				outRect.max = MakeVector2((renderer->ScreenWidth() + zoomedSize.x) * .5f,
				                          (renderer->ScreenHeight() + zoomedSize.y) * .5f);
			}

			float alpha = 1.f;
			if (largeMap) {
				alpha = zoomState;
			}

			// fades bg
			if (largeMap) {
				Handle<IImage> bg = renderer->RegisterImage("Gfx/MapBg.png");
				Vector2 scrSize = {renderer->ScreenWidth(), renderer->ScreenHeight()};
				float size = std::max(scrSize.x, scrSize.y);
				renderer->SetColorAlphaPremultiplied(MakeVector4(0, 0, 0, alpha * .5f));
				renderer->DrawImage(
				  bg, AABB2((scrSize.x - size) * .5f, (scrSize.y - size) * .5f, size, size));
			}

			// draw border
			Handle<IImage> border;
			float borderWidth;
			AABB2 borderRect = outRect;
			if (largeMap) {
				border = renderer->RegisterImage("Gfx/MapBorder.png");
				borderWidth = 3.f * outRect.GetHeight() / zoomedSize.y;
			} else {
				border = renderer->RegisterImage("Gfx/MinimapBorder.png");
				borderWidth = 2.f;
			}
			borderRect = borderRect.Inflate(borderWidth - 8.f);

			renderer->SetColorAlphaPremultiplied(MakeVector4(alpha, alpha, alpha, alpha));
			renderer->DrawImage(border,
			                    AABB2(borderRect.GetMinX() - 16, borderRect.GetMinY() - 16, 16, 16),
			                    AABB2(0, 0, 16, 16));
			renderer->DrawImage(border,
			                    AABB2(borderRect.GetMaxX(), borderRect.GetMinY() - 16, 16, 16),
			                    AABB2(16, 0, 16, 16));
			renderer->DrawImage(border,
			                    AABB2(borderRect.GetMinX() - 16, borderRect.GetMaxY(), 16, 16),
			                    AABB2(0, 16, 16, 16));
			renderer->DrawImage(border, AABB2(borderRect.GetMaxX(), borderRect.GetMaxY(), 16, 16),
			                    AABB2(16, 16, 16, 16));
			renderer->DrawImage(border, AABB2(borderRect.GetMinX(), borderRect.GetMinY() - 16,
			                                  borderRect.GetWidth(), 16),
			                    AABB2(16, 0, 0, 16));
			renderer->DrawImage(
			  border, AABB2(borderRect.GetMinX(), borderRect.GetMaxY(), borderRect.GetWidth(), 16),
			  AABB2(16, 16, 0, 16));
			renderer->DrawImage(border, AABB2(borderRect.GetMinX() - 16, borderRect.GetMinY(), 16,
			                                  borderRect.GetHeight()),
			                    AABB2(0, 16, 16, 0));
			renderer->DrawImage(
			  border, AABB2(borderRect.GetMaxX(), borderRect.GetMinY(), 16, borderRect.GetHeight()),
			  AABB2(16, 16, 16, 0));

			// draw map
			renderer->SetColorAlphaPremultiplied(MakeVector4(alpha, alpha, alpha, alpha));
			renderer->DrawFlatGameMap(outRect, inRect);

			this->inRect = inRect;
			this->outRect = outRect;

			// draw grid

			renderer->SetColorAlphaPremultiplied(MakeVector4(0, 0, 0, 0.8f * alpha));
			Handle<IImage> dashLine = renderer->RegisterImage("Gfx/DashLine.tga");
			for (float x = 64.f; x < map->Width(); x += 64.f) {
				float wx = (x - inRect.GetMinX()) / inRect.GetWidth();
				if (wx < 0.f || wx >= 1.f)
					continue;
				wx = (wx * outRect.GetWidth()) + outRect.GetMinX();
				wx = roundf(wx);
				renderer->DrawImage(dashLine, MakeVector2(wx, outRect.GetMinY()),
				                    AABB2(0, 0, 1.f, outRect.GetHeight()));
			}
			for (float y = 64.f; y < map->Height(); y += 64.f) {
				float wy = (y - inRect.GetMinY()) / inRect.GetHeight();
				if (wy < 0.f || wy >= 1.f)
					continue;
				wy = (wy * outRect.GetHeight()) + outRect.GetMinY();
				wy = roundf(wy);
				renderer->DrawImage(dashLine, MakeVector2(outRect.GetMinX(), wy),
				                    AABB2(0, 0, outRect.GetWidth(), 1.f));
			}

			// draw grid label
			renderer->SetColorAlphaPremultiplied(MakeVector4(1, 1, 1, 1) * (0.8f * alpha));
			Handle<IImage> mapFont = renderer->RegisterImage("Gfx/Fonts/MapFont.tga");
			for (int i = 0; i < 8; i++) {
				float startX = (float)i * 64.f;
				float endX = startX + 64.f;
				if (startX > inRect.GetMaxX() || endX < inRect.GetMinX())
					continue;
				float fade =
				  std::min((std::min(endX, inRect.GetMaxX()) - std::max(startX, inRect.GetMinX())) /
				             (endX - startX) * 2.f,
				           1.f);
				renderer->SetColorAlphaPremultiplied(MakeVector4(1, 1, 1, 1) *
				                                     (fade * .8f * alpha));

				float center = std::max(startX, inRect.GetMinX());
				center = .5f * (center + std::min(endX, inRect.GetMaxX()));

				float wx = (center - inRect.GetMinX()) / inRect.GetWidth();
				wx = (wx * outRect.GetWidth()) + outRect.GetMinX();
				wx = roundf(wx);

				float fntX = static_cast<float>((i & 3) * 8);
				float fntY = static_cast<float>((i >> 2) * 8);
				renderer->DrawImage(mapFont, MakeVector2(wx - 4.f, outRect.GetMinY() + 4),
				                    AABB2(fntX, fntY, 8, 8));
			}
			for (int i = 0; i < 8; i++) {
				float startY = (float)i * 64.f;
				float endY = startY + 64.f;
				if (startY > inRect.GetMaxY() || endY < inRect.GetMinY())
					continue;
				float fade =
				  std::min((std::min(endY, inRect.GetMaxY()) - std::max(startY, inRect.GetMinY())) /
				             (endY - startY) * 2.f,
				           1.f);
				renderer->SetColorAlphaPremultiplied(MakeVector4(1, 1, 1, 1) *
				                                     (fade * .8f * alpha));

				float center = std::max(startY, inRect.GetMinY());
				center = .5f * (center + std::min(endY, inRect.GetMaxY()));

				float wy = (center - inRect.GetMinY()) / inRect.GetHeight();
				wy = (wy * outRect.GetHeight()) + outRect.GetMinY();
				wy = roundf(wy);

				int fntX = (i & 3) * 8;
				int fntY = (i >> 2) * 8 + 16;
				renderer->DrawImage(mapFont, MakeVector2(outRect.GetMinX() + 4, wy - 4.f),
				                    AABB2(fntX, fntY, 8, 8));
			}
			// draw objects

			const int iconMode = cg_minimapPlayerIcon;
			const int colorMode = cg_minimapPlayerColor;

			Handle<IImage> playerSMG = renderer->RegisterImage("Gfx/Map/SMG.png");
			Handle<IImage> playerRifle = renderer->RegisterImage("Gfx/Map/Rifle.png");
			Handle<IImage> playerShotgun = renderer->RegisterImage("Gfx/Map/Shotgun.png");
			Handle<IImage> playerIcon = renderer->RegisterImage("Gfx/Map/Player.png");

			{

				IntVector3 teamColor =
				  world->GetLocalPlayer()->GetTeamId() >= 2
				    ? IntVector3::Make(200, 200, 200)
				    : world->GetTeam(world->GetLocalPlayer()->GetTeamId()).color;
				Vector4 teamColorF = ModifyColor(teamColor);
				teamColorF *= alpha;

				// Draw the focused player's view
				{
					Handle<IImage> viewIcon = renderer->RegisterImage("Gfx/Map/View.png");
					if (focusPlayer.IsAlive()) {
						renderer->SetColorAlphaPremultiplied(teamColorF * 0.9f);
						DrawIcon(focusPlayerPos, viewIcon, focusPlayerAngle);
					}
				}

				// draw player's icon
				for (int i = 0; i < world->GetNumPlayerSlots(); i++) {
					Player *p = world->GetPlayer(i);
					if (!p || !p->IsAlive()) {
						// The player is non-existent or dead
						continue;
					}
					if (!localPlayer.IsSpectator() && localPlayer.GetTeamId() != p->GetTeamId()) {
						// Duh
						continue;
					}

					Vector3 front = p->GetFront2D();
					float ang = atan2(front.x, -front.y);
					if (p == &focusPlayer && p->IsSpectator()) {
						ang = focusPlayerAngle;
					}

					// use a spec color for each player
					if (colorMode) {
						IntVector3 Colorplayer =
						  IntVector3::Make(palette[i][0], palette[i][1], palette[i][2]);
						Vector4 ColorplayerF = ModifyColor(Colorplayer);
						ColorplayerF *= 1.0f;
						renderer->SetColorAlphaPremultiplied(ColorplayerF);
					} else {
						renderer->SetColorAlphaPremultiplied(teamColorF);
					}

					// use a different icon in minimap according to weapon of player
					if (iconMode) {
						WeaponType weapon = world->GetPlayer(i)->GetWeaponType();
						if (weapon == WeaponType::SMG_WEAPON) {
							DrawIcon(p->IsSpectator() ? client->freeCameraState.position
							                                  : p->GetPosition(),
							         playerSMG, ang);
						}

						else if (weapon == WeaponType::RIFLE_WEAPON) {
							DrawIcon(p->IsSpectator() ? client->freeCameraState.position
							                                  : p->GetPosition(),
							         playerRifle, ang);
						}

						else if (weapon == WeaponType::SHOTGUN_WEAPON) {
							DrawIcon(p->IsSpectator() ? client->freeCameraState.position
							                                  : p->GetPosition(),
							         playerShotgun, ang);
						}
					} else { // draw normal color
						DrawIcon(p == &focusPlayer ? focusPlayerPos : p->GetPosition(),
						         playerIcon, ang);
					}
				}
			}

			IGameMode *mode = world->GetMode();
			if (mode && IGameMode::m_CTF == mode->ModeType()) {
				CTFGameMode *ctf = static_cast<CTFGameMode *>(mode);
				Handle<IImage> intelIcon = renderer->RegisterImage("Gfx/Map/Intel.png");
				Handle<IImage> baseIcon = renderer->RegisterImage("Gfx/Map/CommandPost.png");
				for (int tId = 0; tId < 2; tId++) {
					CTFGameMode::Team &team = ctf->GetTeam(tId);
					IntVector3 teamColor = world->GetTeam(tId).color;
					Vector4 teamColorF = ModifyColor(teamColor);
					teamColorF *= alpha;

					// draw base
					renderer->SetColorAlphaPremultiplied(teamColorF);
					DrawIcon(team.basePos, baseIcon, 0.f);

					// draw flag
					if (!ctf->GetTeam(1 - tId).hasIntel) {
						renderer->SetColorAlphaPremultiplied(teamColorF);
						DrawIcon(team.flagPos, intelIcon, 0.f);
					} else if (world->GetLocalPlayer()->GetTeamId() == 1 - tId) {
						// local player's team is carrying
						int cId = ctf->GetTeam(1 - tId).carrier;

						// in some game modes, carrier becomes invalid
						if (cId < world->GetNumPlayerSlots()) {
							Player *carrier = world->GetPlayer(cId);
							if (carrier &&
							    carrier->GetTeamId() == world->GetLocalPlayer()->GetTeamId()) {

								Vector4 col = teamColorF;
								col *= fabsf(sinf(world->GetTime() * 4.f));
								renderer->SetColorAlphaPremultiplied(col);
								DrawIcon(carrier->GetPosition(), intelIcon, 0.f);
							}
						}
					}
				}
			} else if (mode && IGameMode::m_TC == mode->ModeType()) {
				TCGameMode *tc = static_cast<TCGameMode *>(mode);
				Handle<IImage> icon = renderer->RegisterImage("Gfx/Map/CommandPost.png");
				int cnt = tc->GetNumTerritories();
				for (int i = 0; i < cnt; i++) {
					TCGameMode::Territory *t = tc->GetTerritory(i);
					IntVector3 teamColor = {128, 128, 128};
					if (t->ownerTeamId < 2) {
						teamColor = world->GetTeam(t->ownerTeamId).color;
					}
					Vector4 teamColorF = ModifyColor(teamColor);
					teamColorF *= alpha;

					// draw base
					renderer->SetColorAlphaPremultiplied(teamColorF);
					DrawIcon(t->pos, icon, 0.f);
				}
			}

			// draw tracers
			Handle<IImage> tracerImage = renderer->RegisterImage("Gfx/Ball.png");
			const float tracerWidth = 2.0f;
			const AABB2 tracerInRect{0.0f, 0.0f, tracerImage->GetWidth(), tracerImage->GetHeight()};

			for (const auto &localEntity : client->localEntities) {
				auto *const tracer = dynamic_cast<MapViewTracer *>(localEntity.get());
				if (!tracer) {
					continue;
				}

				const auto line1 = tracer->GetLineSegment();
				if (!line1) {
					continue;
				}

				auto line2 =
				  ClipLineSegment(std::make_pair(Vector2{(*line1).first.x, (*line1).first.y},
				                                 Vector2{(*line1).second.x, (*line1).second.y}),
				                  inRect);
				if (!line2) {
					continue;
				}

				auto &line3 = *line2;
				line3.first = Project(line3.first);
				line3.second = Project(line3.second);

				if (line3.first == line3.second) {
					continue;
				}

				Vector2 normal = (line3.second - line3.first).Normalize();
				normal = {-normal.y, normal.x};

				{
					const Vector2 vertices[] = {line3.first - normal * tracerWidth,
					                            line3.first + normal * tracerWidth,
					                            line3.second - normal * tracerWidth};

					renderer->SetColorAlphaPremultiplied(Vector4{1.0f, 0.8f, 0.6f, 1.0f} * alpha);
					renderer->DrawImage(tracerImage, vertices[0], vertices[1], vertices[2],
					                    tracerInRect);
				}
			}
		}

		MapViewTracer::MapViewTracer(Vector3 p1, Vector3 p2, float bulletVel)
		    : startPos(p1), velocity(bulletVel) {
			// Z coordinate doesn't matter in MapView
			p1.z = 0.0f;
			p2.z = 0.0f;

			dir = (p2 - p1).Normalize();
			length = (p2 - p1).GetLength();

			// in MapView it looks slower than it is actually, so compensate for that
			bulletVel *= 4.0f;

			const float maxTimeSpread = 1.0f / 10.f;
			const float shutterTime = 1.0f / 10.f;

			visibleLength = shutterTime * velocity;
			curDistance = -visibleLength;
			curDistance += maxTimeSpread * SampleRandomFloat();

			firstUpdate = true;
		}

		bool MapViewTracer::Update(float dt) {
			if (!firstUpdate) {
				curDistance += dt * velocity;
				if (curDistance > length) {
					return false;
				}
			}
			firstUpdate = false;
			return true;
		}

		stmp::optional<std::pair<Vector3, Vector3>> MapViewTracer::GetLineSegment() {
			float startDist = curDistance;
			float endDist = curDistance + visibleLength;
			startDist = std::max(startDist, 0.f);
			endDist = std::min(endDist, length);
			if (startDist >= endDist) {
				return {};
			}
			Vector3 pos1 = startPos + dir * startDist;
			Vector3 pos2 = startPos + dir * endDist;
			return std::make_pair(pos1, pos2);
		}

		MapViewTracer::~MapViewTracer() {}
	}
}
