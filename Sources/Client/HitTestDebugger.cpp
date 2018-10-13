/*
 Copyright (c) 2013 yvt
 based on code of pysnip (c) Mathias Kaerlev 2011-2012.

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

#include <ctime> //windows needs this.

#include "HitTestDebugger.h"
#include "GameMap.h"
#include "Player.h"
#include "Weapon.h"
#include "World.h"
#include <Core/Debug.h>
#include <Core/FileManager.h>
#include <Core/Settings.h>
#include <Core/Strings.h>
#include <Draw/SWPort.h>
#include <Draw/SWRenderer.h>

SPADES_SETTING(cg_smp);

namespace spades {
	namespace client {
		class HitTestDebugger::Port : public draw::SWPort {
			Handle<Bitmap> bmp;

		public:
			Port() {
				SPADES_MARK_FUNCTION();
				bmp.Set(new Bitmap(512, 512), false);
			}
			Bitmap *GetFramebuffer() override { return bmp; }
			void Swap() override {
				// nothing to do here
			}
		};

		HitTestDebugger::HitTestDebugger(World *world) : world(world) {
			SPADES_MARK_FUNCTION();
			port.Set(new Port(), false);
			renderer.Set(new draw::SWRenderer(port), false);
			renderer->Init();
		}

		HitTestDebugger::~HitTestDebugger() {
			SPADES_MARK_FUNCTION();

			renderer->Shutdown();
		}

		void HitTestDebugger::SaveImage(const std::map<int, PlayerHit> &hits,
		                                const std::vector<Vector3> &bullets) {
			SPADES_MARK_FUNCTION();

			renderer->SetFogColor(MakeVector3(0.f, 0.f, 0.f));
			renderer->SetFogDistance(128.f);

			Player *localPlayer = world->GetLocalPlayer();

			if (localPlayer == nullptr) {
				SPLog("HitTestDebugger failure: Local player is null");
				return;
			}

			SceneDefinition def;

			Vector3 front = localPlayer->GetFront();
			Vector3 right = localPlayer->GetRight();
			Vector3 up = localPlayer->GetUp();

			def.viewOrigin = localPlayer->GetEye();
			def.viewAxis[0] = right;
			def.viewAxis[1] = up;
			def.viewAxis[2] = front;

			auto toViewCoord = [&](const Vector3 &targPos) {
				Vector3 targetViewPos;
				targetViewPos.x = Vector3::Dot(targPos - def.viewOrigin, def.viewAxis[0]);
				targetViewPos.y = Vector3::Dot(targPos - def.viewOrigin, def.viewAxis[1]);
				targetViewPos.z = Vector3::Dot(targPos - def.viewOrigin, def.viewAxis[2]);
				return targetViewPos;
			};

			// fit FoV to include all possibly hit players
			float range = 0.2f;
			for (std::size_t i = 0; i < world->GetNumPlayerSlots(); i++) {
				auto *p = world->GetPlayer(static_cast<unsigned int>(i));
				if (!p)
					continue;
				if (p == localPlayer)
					continue;
				if (p->GetTeamId() == localPlayer->GetTeamId())
					continue;
				if (!p->IsAlive())
					continue;
				Vector3 targPos = p->GetEye();
				Vector3 targetViewPos = toViewCoord(targPos);

				if (targetViewPos.GetPoweredLength() > 130.f * 130.f)
					continue;

				if (targetViewPos.z < -3.f) {
					continue;
				}
				targetViewPos.z = std::max(targetViewPos.z, 0.1f);

				const float bodySize = 3.5f;
				if (fabsf(targetViewPos.x) > bodySize + 2.5f ||
				    fabsf(targetViewPos.y) > bodySize + 2.5f)
					continue;

				float prange = std::max(fabsf(targetViewPos.x), fabsf(targetViewPos.y)) + bodySize;
				prange = atanf(prange / targetViewPos.z) * 2.f;
				range = std::max(range, prange);
			}

			// fit FoV to include all bullets
			for (auto v : bullets) {
				auto vc = toViewCoord(v + def.viewOrigin);
				vc /= vc.z;
				auto prange = atanf(std::max(fabsf(vc.x), fabsf(vc.y)) * 1.5f) * 2.f;
				;
				range = std::max(range, prange);
			}

			def.fovX = def.fovY = range;

			// we cannot change GameMap's listener in the client thread with SMP renderer
			def.skipWorld = ((int)cg_smp != 0);

			def.zNear = 0.05f;
			def.zFar = 200.f;

			// start rendering
			GameMap *map = world->GetMap();
			if (!def.skipWorld) {
				renderer->SetGameMap(map);
			}
			renderer->StartScene(def);

			auto numPlayers = world->GetNumPlayerSlots();

			auto drawBox = [&](const OBB3 &box, Vector4 color) {
				SPADES_MARK_FUNCTION();

				auto m = box.m;
				renderer->AddDebugLine((m * Vector3(0.f, 0.f, 0.f)).GetXYZ(),
				                       (m * Vector3(0.f, 0.f, 1.f)).GetXYZ(), color);
				renderer->AddDebugLine((m * Vector3(0.f, 1.f, 0.f)).GetXYZ(),
				                       (m * Vector3(0.f, 1.f, 1.f)).GetXYZ(), color);
				renderer->AddDebugLine((m * Vector3(1.f, 0.f, 0.f)).GetXYZ(),
				                       (m * Vector3(1.f, 0.f, 1.f)).GetXYZ(), color);
				renderer->AddDebugLine((m * Vector3(1.f, 1.f, 0.f)).GetXYZ(),
				                       (m * Vector3(1.f, 1.f, 1.f)).GetXYZ(), color);

				renderer->AddDebugLine((m * Vector3(0.f, 0.f, 0.f)).GetXYZ(),
				                       (m * Vector3(0.f, 1.f, 0.f)).GetXYZ(), color);
				renderer->AddDebugLine((m * Vector3(0.f, 1.f, 0.f)).GetXYZ(),
				                       (m * Vector3(1.f, 1.f, 0.f)).GetXYZ(), color);
				renderer->AddDebugLine((m * Vector3(1.f, 1.f, 0.f)).GetXYZ(),
				                       (m * Vector3(1.f, 0.f, 0.f)).GetXYZ(), color);
				renderer->AddDebugLine((m * Vector3(1.f, 0.f, 0.f)).GetXYZ(),
				                       (m * Vector3(0.f, 0.f, 0.f)).GetXYZ(), color);

				renderer->AddDebugLine((m * Vector3(0.f, 0.f, 1.f)).GetXYZ(),
				                       (m * Vector3(0.f, 1.f, 1.f)).GetXYZ(), color);
				renderer->AddDebugLine((m * Vector3(0.f, 1.f, 1.f)).GetXYZ(),
				                       (m * Vector3(1.f, 1.f, 1.f)).GetXYZ(), color);
				renderer->AddDebugLine((m * Vector3(1.f, 1.f, 1.f)).GetXYZ(),
				                       (m * Vector3(1.f, 0.f, 1.f)).GetXYZ(), color);
				renderer->AddDebugLine((m * Vector3(1.f, 0.f, 1.f)).GetXYZ(),
				                       (m * Vector3(0.f, 0.f, 1.f)).GetXYZ(), color);
			};

			auto getColor = [](int count) {
				SPADES_MARK_FUNCTION();

				switch (count) {
					case 0: return Vector4(0.5f, 0.5f, 0.5f, 1.f);
					case 1: return Vector4(1.f, 0.f, 0.f, 1.f);
					case 2: return Vector4(1.f, 1.f, 0.f, 1.f);
					case 3: return Vector4(0.f, 1.f, 0.f, 1.f);
					case 4: return Vector4(0.f, 1.f, 1.f, 1.f);
					case 5: return Vector4(0.f, 0.f, 1.f, 1.f);
					case 6: return Vector4(1.f, 0.f, 1.f, 1.f);
					default: return Vector4(1.f, 1.f, 1.f, 1.f);
				}
			};

			for (std::size_t i = 0; i < numPlayers; i++) {
				auto *p = world->GetPlayer(static_cast<unsigned int>(i));
				if (!p)
					continue;
				if (p == localPlayer)
					continue;
				if (!p->IsAlive())
					continue;

				if ((p->GetEye() - def.viewOrigin).GetPoweredLength() > 130.f * 130.f)
					continue;

				auto hitboxes = p->GetHitBoxes();
				PlayerHit hit;
				{
					auto it = hits.find(static_cast<int>(i));
					if (it != hits.end()) {
						hit = it->second;
					}
				}

				drawBox(hitboxes.head, getColor(hit.numHeadHits));
				drawBox(hitboxes.torso, getColor(hit.numTorsoHits));
				for (std::size_t i = 0; i < 3; i++)
					drawBox(hitboxes.limbs[i], getColor(hit.numLimbHits[i]));
			}

			renderer->EndScene();

			// draw crosshair
			IImage *img = renderer->RegisterImage("Gfx/White.tga");
			float size = renderer->ScreenWidth();

			renderer->SetColorAlphaPremultiplied(Vector4(1.f, 0.f, 0.f, 0.9f));
			renderer->DrawImage(img, AABB2(size * 0.5f - 1.f, 0.f, 2.f, size));
			renderer->DrawImage(img, AABB2(0.f, size * 0.5f - 1.f, size, 2.f));

			// draw bullet vectors
			float fov = tanf(def.fovY * .5f);
			for (auto v : bullets) {
				auto vc = toViewCoord(v + def.viewOrigin);
				vc /= vc.z * fov;
				float x = size * (0.5f + 0.5f * vc.x);
				float y = size * (0.5f - 0.5f * vc.y);
				x = floorf(x);
				y = floorf(y);
				renderer->SetColorAlphaPremultiplied(Vector4(1.f, 0.f, 0.f, 0.9f));
				renderer->DrawImage(img, AABB2(x - 1.f, y - 1.f, 3.f, 3.f));
				renderer->SetColorAlphaPremultiplied(Vector4(1.f, 1.f, 0.f, 0.9f));
				renderer->DrawImage(img, AABB2(x, y, 1.f, 1.f));
			}

			renderer->FrameDone();

			// create image filename
			std::string fileName;
			{
				time_t t;
				struct tm tm;
				time(&t);
				tm = *localtime(&t);
				char buf[256];

				sprintf(buf, "%04d%02d%02d-%02d%02d%02d", tm.tm_year + 1900, tm.tm_mon + 1,
				        tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
				fileName = buf;
			}

			switch (localPlayer->GetWeapon()->GetWeaponType()) {
				case SMG_WEAPON: fileName += "-SMG"; break;
				case RIFLE_WEAPON: fileName += "-Rifle"; break;
				case SHOTGUN_WEAPON: fileName += "-Shotgun"; break;
			}

			int numHits = 0;
			for (const auto &hit : hits) {
				numHits += hit.second.numHeadHits;
				numHits += hit.second.numTorsoHits;
				numHits += hit.second.numLimbHits[0];
				numHits += hit.second.numLimbHits[1];
				numHits += hit.second.numLimbHits[2];
			}

			fileName = "HitTestDebugger/" + fileName;

			std::string baseFileName = fileName;
			for (int i = 0;; i++) {
				fileName = Format("{0}-{1}-{2}hit.tga", baseFileName, i, numHits);
				if (!FileManager::FileExists(fileName.c_str()))
					break;
			}

			// save image
			try {
				Handle<Bitmap> b(renderer->ReadBitmap(), false);
				b->Save(fileName);
				SPLog("HitTestDebugger: saved to '%s'", fileName.c_str());
			} catch (const std::exception &ex) {
				SPLog("HitTestDebugger failure: failed to save '%s': %s", fileName.c_str(),
				      ex.what());
			}

			renderer->Flip();
		}
	}
}
