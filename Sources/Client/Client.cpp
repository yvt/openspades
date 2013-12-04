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

#include "Client.h"
#include "IRenderer.h"
#include "GameMap.h"
#include "SceneDefinition.h"
#include "../Core/FileManager.h"
#include "../Core/IStream.h"
#include "GameMapWrapper.h"
#include "World.h"
#include "Player.h"
#include "IAudioChunk.h"
#include "IAudioDevice.h"
#include "Weapon.h"
#include "Grenade.h"
#include "../Core/Debug.h"
#include "NetClient.h"
#include "FontData.h"
#include "Quake3Font.h"
#include "../Core/Exception.h"
#include "ChatWindow.h"
#include "Corpse.h"
#include "CenterMessageView.h"
#include "HurtRingView.h"
#include "CTFGameMode.h"
#include "MapView.h"
#include "ScoreboardView.h"
#include "LimboView.h"
#include "ILocalEntity.h"
#include "ParticleSpriteEntity.h"
#include "SmokeSpriteEntity.h"
#include "GameMapWrapper.h"
#include "../Core/Settings.h"
#include "../Core/Bitmap.h"
#include <string.h>
#include "FallingBlock.h"
#include "GunCasing.h"
#include "../Core/ConcurrentDispatch.h"
#include "PaletteView.h"
#include "TCGameMode.h"
#include "TCProgressView.h"
#include "../Core/IStream.h"
#include <stdarg.h>
#include <time.h>
#include "Tracer.h"
#include <stdlib.h>
#include "ClientPlayer.h"
#include "ClientUI.h"

static float nextRandom() {
	return (float)rand() / (float)RAND_MAX;
}

SPADES_SETTING(cg_ragdoll, "1");
SPADES_SETTING(cg_blood, "1");
SPADES_SETTING(cg_ejectBrass, "1");

SPADES_SETTING(cg_mouseSensitivity, "1");
SPADES_SETTING(cg_zoomedMouseSensScale, "0.6");

SPADES_SETTING(cg_holdAimDownSight, "0");

SPADES_SETTING(cg_keyAttack, "LeftMouseButton");
SPADES_SETTING(cg_keyAltAttack, "RightMouseButton");
SPADES_SETTING(cg_keyToolSpade, "1");
SPADES_SETTING(cg_keyToolBlock, "2");
SPADES_SETTING(cg_keyToolWeapon, "3");
SPADES_SETTING(cg_keyToolGrenade, "4");
SPADES_SETTING(cg_keyReloadWeapon, "r");
SPADES_SETTING(cg_keyFlashlight, "f");

SPADES_SETTING(cg_keyMoveLeft, "a");
SPADES_SETTING(cg_keyMoveRight, "d");
SPADES_SETTING(cg_keyMoveForward, "w");
SPADES_SETTING(cg_keyMoveBackward, "s");
SPADES_SETTING(cg_keyJump, "Space");
SPADES_SETTING(cg_keyCrouch, "Control");
SPADES_SETTING(cg_keySprint, "Shift");
SPADES_SETTING(cg_keySneak, "v");

SPADES_SETTING(cg_keyCaptureColor, "e");
SPADES_SETTING(cg_keyGlobalChat, "t");
SPADES_SETTING(cg_keyTeamChat, "y");
SPADES_SETTING(cg_keyChangeMapScale, "m");
SPADES_SETTING(cg_keyToggleMapZoom, "n");
SPADES_SETTING(cg_keyScoreboard, "Tab");
SPADES_SETTING(cg_keyLimbo, "l");

SPADES_SETTING(cg_keyScreenshot, "0");
SPADES_SETTING(cg_keySceneshot, "9");
SPADES_SETTING(cg_keySaveMap, "8");

SPADES_SETTING(cg_switchToolByWheel, "1");

SPADES_SETTING(cg_fov, "68");


SPADES_SETTING(cg_debugAim, "0");
SPADES_SETTING(cg_debugCorpse, "0");

namespace spades {
	namespace client {
		
		Client::Client(IRenderer *r, IAudioDevice *audioDev,
					   const ServerAddress& host, std::string playerName):
		renderer(r), audioDevice(audioDev), playerName(playerName) {
			SPADES_MARK_FUNCTION();
			SPLog("Initializing...");
			
			hostname = host;
			/*
			designFont = new Quake3Font(renderer,
										renderer->RegisterImage("Gfx/Fonts/Orbitron.tga"),
										(const int*)OrbitronMap,
										30,
										18);
			*/
			designFont = new Quake3Font(renderer,
										renderer->RegisterImage("Gfx/Fonts/UnsteadyOversteer.tga"),
										(const int *)UnsteadyOversteerMap,
										30,
										18);
			SPLog("Font 'Unsteady Oversteer' Loaded");
			/*
			textFont = new Quake3Font(renderer,
										renderer->RegisterImage("Gfx/Fonts/UbuntuCondensed.tga"),
										(const int*)UbuntuCondensedMap,
										24,
									  4);
			SPLog("Font 'Ubuntu Condensed' Loaded");*/
			textFont = new client::Quake3Font(renderer,
										 renderer->RegisterImage("Gfx/Fonts/SquareFontModified.png"),
										 (const int*)SquareFontMap,
										 24,
										 4);
			SPLog("Font 'SquareFont' Loaded");
			
			bigTextFont = new Quake3Font(renderer,
									  renderer->RegisterImage("Gfx/Fonts/UbuntuCondensedBig.tga"),
									  (const int*)UbuntuCondensedBigMap,
									  48,
										 8);
			SPLog("Font 'Ubuntu Condensed (Large)' Loaded");
			
			world = NULL;
			net = NULL;
			
			frameToRendererInit = 5;
			
			// preferences?
			corpseSoftTimeLimit = 30.f; // TODO: this is not used
			corpseSoftLimit = 6;
			corpseHardLimit = 16;
			
			lastMyCorpse = NULL;
			
			renderer->SetFogDistance(128.f);
			renderer->SetFogColor(MakeVector3(.8f, 1.f, 1.f));
			
			chatWindow = new ChatWindow(this, GetRenderer(), textFont, false);
			killfeedWindow = new ChatWindow(this, GetRenderer(), textFont, true);
			
			hurtRingView = new HurtRingView(this);
			centerMessageView = new CenterMessageView(this, bigTextFont);
			mapView = new MapView(this, false);
			largeMapView = new MapView(this, true);
			scoreboard = new ScoreboardView(this);
			limbo = new LimboView(this);
			paletteView = new PaletteView(this);
			tcView = new TCProgressView(this);
			scriptedUI.Set(new ClientUI(renderer, audioDev, textFont, this), false);
			
			time = 0.f;
			lastAliveTime = 0.f;
			readyToClose = false;
			scoreboardVisible = false;
			flashlightOn = false;
			lastKills = 0;
			
			renderer->SetGameMap(NULL);
			
			logStream = NULL;
			
			localFireVibrationTime = -1.f;
			
			lastPosSentTime = 0.f;
			worldSubFrame = 0.f;
			grenadeVibration = 0.f;
			inGameLimbo = false;
			
			nextScreenShotIndex = 0;
			nextMapShotIndex = 0;
						
			timeSinceInit = 0.f;
		}
		
		void Client::SetWorld(spades::client::World *w) {
			SPADES_MARK_FUNCTION();
			
			if(world == w){
				return;
			}
			
			scriptedUI->CloseUI();
			
			RemoveAllCorpses();
			lastHealth = 0;
			lastHurtTime = -100.f;
			hurtRingView->ClearAll();
			scoreboardVisible = false;
			flashlightOn = false;
			
			for(size_t i = 0; i < clientPlayers.size(); i++) {
				if(clientPlayers[i]) {
					clientPlayers[i]->Invalidate();
					clientPlayers[i]->Release();
				}
			}
			clientPlayers.clear();
			
			if(world){
				world->SetListener(NULL);
				renderer->SetGameMap(NULL);
				audioDevice->SetGameMap(NULL);
				delete world;
				world = NULL;
				map = NULL;
			}
			world = w;
			if(world){
				SPLog("World set");
				
				clientPlayers.resize(world->GetNumPlayerSlots());
				for(size_t i = 0; i < world->GetNumPlayerSlots(); i++) {
					Player *p = world->GetPlayer(i);
					if(p){
						clientPlayers[i] = new ClientPlayer(p, this);
					}else{
						clientPlayers[i] = NULL;
					}
				}
				
				world->SetListener(this);
				map = world->GetMap();
				renderer->SetGameMap(map);
				audioDevice->SetGameMap(map);
				NetLog("------ World Loaded ------");
			}else{
				
				SPLog("World removed");
				NetLog("------ World Unloaded ------");
			}
			
			limbo->SetSelectedTeam(2);
			limbo->SetSelectedWeapon(RIFLE_WEAPON);
			
			RemoveAllLocalEntities();
			
			worldSubFrame = 0.f;
			inGameLimbo = false;
			worldSetTime = time;
		}
		
		Client::~Client() {
			SPADES_MARK_FUNCTION();
			
			NetLog("Disconnecting");
			if(logStream) {
				SPLog("Closing netlog");
				delete logStream;
			}
			
			if(net){
				SPLog("Disconnecting");
				net->Disconnect();
				delete net;
			}
			
			SPLog("Disconnected");
			
			RemoveAllLocalEntities();
			RemoveAllCorpses();
			
			renderer->SetGameMap(NULL);
            audioDevice->SetGameMap(NULL);
			
			for(size_t i = 0; i < clientPlayers.size(); i++) {
				if(clientPlayers[i]) {
					clientPlayers[i]->Invalidate();
					clientPlayers[i]->Release();
				}
			}
			
			if(world)
				delete world;
			
			scriptedUI->ClientDestroyed();
			delete tcView;
			delete limbo;
			delete scoreboard;
			delete mapView;
			delete largeMapView;
			delete chatWindow;
			delete killfeedWindow;
			delete paletteView;
			delete centerMessageView;
			delete hurtRingView;
			designFont->Release();
			textFont->Release();
			bigTextFont->Release();
		}
		
		bool Client::WantsToBeClosed() {
			return readyToClose;
		}
		
		void Client::DoInit() {
			renderer->Init();
			// preload
			SmokeSpriteEntity(this, Vector4(), 20.f);
			
			renderer->RegisterImage("Textures/Fluid.png");
			renderer->RegisterImage("Textures/WaterExpl.png");
			renderer->RegisterImage("Gfx/White.tga");
			audioDevice->RegisterSound("Sounds/Weapons/Block/Build.wav");
			audioDevice->RegisterSound("Sounds/Weapons/Impacts/FleshLocal1.wav");
			audioDevice->RegisterSound("Sounds/Weapons/Impacts/FleshLocal2.wav");
			audioDevice->RegisterSound("Sounds/Weapons/Impacts/FleshLocal3.wav");
			audioDevice->RegisterSound("Sounds/Weapons/Impacts/FleshLocal4.wav");
			audioDevice->RegisterSound("Sounds/Misc/SwitchMapZoom.wav");
			audioDevice->RegisterSound("Sounds/Misc/OpenMap.wav");
			audioDevice->RegisterSound("Sounds/Misc/CloseMap.wav");
			audioDevice->RegisterSound("Sounds/Player/Flashlight.wav");
			audioDevice->RegisterSound("Sounds/Weapons/SwitchLocal.wav");
			renderer->RegisterImage("Gfx/Ball.png");
			renderer->RegisterModel("Models/Player/Dead.kv6");
			renderer->RegisterImage("Gfx/Spotlight.tga");
			renderer->RegisterImage("Gfx/Glare.tga");
			renderer->RegisterModel("Models/Weapons/Spade/Spade.kv6");
			renderer->RegisterModel("Models/Weapons/Block/Block2.kv6");
			renderer->RegisterModel("Models/Weapons/Grenade/Grenade.kv6");
			renderer->RegisterModel("Models/Weapons/SMG/Weapon.kv6");
			renderer->RegisterModel("Models/Weapons/SMG/WeaponNoMagazine.kv6");
			renderer->RegisterModel("Models/Weapons/SMG/Magazine.kv6");
			renderer->RegisterModel("Models/Weapons/Rifle/Weapon.kv6");
			renderer->RegisterModel("Models/Weapons/Rifle/WeaponNoMagazine.kv6");
			renderer->RegisterModel("Models/Weapons/Rifle/Magazine.kv6");
			renderer->RegisterModel("Models/Weapons/Shotgun/Weapon.kv6");
			renderer->RegisterModel("Models/Weapons/Shotgun/WeaponNoPump.kv6");
			renderer->RegisterModel("Models/Weapons/Shotgun/Pump.kv6");
			renderer->RegisterModel("Models/Player/Arm.kv6");
			renderer->RegisterModel("Models/Player/UpperArm.kv6");
			renderer->RegisterModel("Models/Player/LegCrouch.kv6");
			renderer->RegisterModel("Models/Player/TorsoCrouch.kv6");
			renderer->RegisterModel("Models/Player/Leg.kv6");
			renderer->RegisterModel("Models/Player/Torso.kv6");
			renderer->RegisterModel("Models/Player/Arms.kv6");
			renderer->RegisterModel("Models/Player/Head.kv6");
			renderer->RegisterModel("Models/MapObjects/Intel.kv6");
			renderer->RegisterModel("Models/MapObjects/CheckPoint.kv6");
			renderer->RegisterImage("Gfx/Sight.tga");
			renderer->RegisterImage("Gfx/Bullet/7.62mm.tga");
			renderer->RegisterImage("Gfx/Bullet/9mm.tga");
			renderer->RegisterImage("Gfx/Bullet/12gauge.tga");
			renderer->RegisterImage("Gfx/CircleGradient.png");
			renderer->RegisterImage("Gfx/LoadingWindow.png");
			renderer->RegisterImage("Gfx/LoadingWindowGlow.png");
			renderer->RegisterImage("Gfx/LoadingStripe.png");
			renderer->RegisterImage("Gfx/HurtSprite.png");
			renderer->RegisterImage("Gfx/HurtRing.tga");
			audioDevice->RegisterSound("Sounds/Feedback/Chat.wav");
			
			SPLog("Started connecting to '%s'", hostname.asString(true).c_str());
			net = new NetClient(this);
			net->Connect(hostname);
			
			// decide log file name
			std::string fn = hostname.asString(false);
			std::string fn2;
			{
				time_t t;
				struct tm tm;
				::time(&t);
				tm = *localtime(&t);
				char buf[256];
				sprintf(buf, "%04d%02d%02d%02d%02d%02d_",
						tm.tm_year + 1900,
						tm.tm_mon + 1,
						tm.tm_mday,
						tm.tm_hour,
						tm.tm_min,
						tm.tm_sec);
				fn2 = buf;
			}
			for(size_t i = 0; i < fn.size(); i++){
				char c = fn[i];
				if((c >= 'a' && c <= 'z') ||
				   (c >= 'A' && c <= 'Z') ||
				   (c >= '0' && c <= '9')) {
					fn2 += c;
				}else{
					fn2 += '_';
				}
			}
			fn2 = "NetLogs/" + fn2 + ".log";
			
			try{
				logStream = FileManager::OpenForWriting(fn2.c_str());
				SPLog("Netlog Started at '%s'", fn2.c_str());
			}catch(const std::exception& ex){
				SPLog("Failed to open netlog file '%s' (%s)", fn2.c_str(), ex.what());
			}
		}
		
		void Client::RunFrame(float dt) {
			SPADES_MARK_FUNCTION();
			
			if(frameToRendererInit > 0){
				// waiting for renderer initialization
				
				DrawStartupScreen();
				
				frameToRendererInit--;
				if(frameToRendererInit == 0){
					DoInit();

				}else{
					return;
				}
			}
			
			timeSinceInit += std::min(dt, .03f);
			
			try{
				if(net->GetStatus() == NetClientStatusConnected)
					net->DoEvents(0);
				else
					net->DoEvents(10);
			}catch(const std::exception& ex){
				if(net->GetStatus() == NetClientStatusNotConnected){
					SPLog("Disconnected because of error:\n%s", ex.what());
					NetLog("Disconnected because of error:\n%s", ex.what());
					throw;
				}else{
					SPLog("Exception while processing network packets (ignored):\n%s", ex.what());
				}
			}
			
			hurtRingView->Update(dt);
			centerMessageView->Update(dt);
			mapView->Update(dt);
			largeMapView->Update(dt);
			
			//puts(net->GetStatusString().c_str());
			if(world){
				Player* player = world->GetLocalPlayer();
				
				if(player && player->GetTeamId() >= 2){
					// spectating
					
					Vector3 lastPos = followPos;
					followVel *= powf(.3f, dt);
					followPos += followVel * dt;
					
					if(followPos.x < 0.f) {
						followVel.x = fabsf(followVel.x) * 0.2f;
						followPos = lastPos + followVel * dt;
					}
					if(followPos.y < 0.f) {
						followVel.y = fabsf(followVel.y) * 0.2f;
						followPos = lastPos + followVel * dt;
					}
					if(followPos.x > (float)GetWorld()->GetMap()->Width()) {
						followVel.x = fabsf(followVel.x) * -0.2f;
						followPos = lastPos + followVel * dt;
					}
					if(followPos.y > (float)GetWorld()->GetMap()->Height()) {
						followVel.y = fabsf(followVel.y) * -0.2f;
						followPos = lastPos + followVel * dt;
					}
					
					GameMap::RayCastResult minResult;
					float minDist = 1.e+10f;
					Vector3 minShift;
					
					if(followVel.GetLength() < .01){
						followPos = lastPos;
						followVel *= 0.f;
					}else{
						for(int sx = -1; sx <= 1; sx ++)
							for(int sy = -1; sy <= 1; sy++)
								for(int sz = -1; sz <= 1; sz++){
									GameMap::RayCastResult result;
									Vector3 shift = {sx*.1f, sy*.1f,sz*.1f};
									result = map->CastRay2(lastPos+shift, followPos - lastPos,
														   256);
									if(result.hit && !result.startSolid &&
									   Vector3::Dot(result.hitPos - followPos - shift,
													followPos - lastPos) < 0.f){
										   
										   float dist =  Vector3::Dot(result.hitPos - followPos - shift,
																	  (followPos - lastPos).Normalize());
										   if(dist < minDist){
											   minResult = result;
											   minDist = dist;
											   minShift = shift;
										   }
									   }
								}
						
					}
					if(minDist < 1.e+9f){
						GameMap::RayCastResult result = minResult;
						Vector3 shift = minShift;
						followPos = result.hitPos - shift;
						followPos.x += result.normal.x * .02f;
						followPos.y += result.normal.y * .02f;
						followPos.z += result.normal.z * .02f;
						
						// reflect
						Vector3 norm = {(float)result.normal.x,
							(float)result.normal.y,
							(float)result.normal.z};
						float dot = Vector3::Dot(followVel, norm);
						followVel -= norm * (dot * 1.2f);
						/*
						int link = world->mapWrapper->GetLink(result.hitBlock.x,
													   result.hitBlock.y,
													   result.hitBlock.z);
						
						printf("hit block: link = %d (", link);
						switch(link){
							case 0: puts("Invalid)"); break;
							case 1: puts("Root)"); break;
							case 2: puts("Neg X)"); break;
							case 3: puts("Pos X)"); break;
							case 4: puts("Neg Y)"); break;
							case 5: puts("Pos Y)"); break;
							case 6: puts("Neg Z)"); break;
							case 7: puts("Pos Z)"); break;
						}*/
						
					}
				
					
					Vector3 front;
					Vector3 up = {0, 0, -1};
					
					front.x = -cosf(followYaw) * cosf(followPitch);
					front.y = -sinf(followYaw) * cosf(followPitch);
					front.z = sinf(followPitch);
					
					Vector3 right = -Vector3::Cross(up, front).Normalize();
					Vector3 up2 = Vector3::Cross(right, front).Normalize();
					
					float scale = 10.f * dt;
					if(playerInput.sprint){
						scale *= 3.f;
					}
					front *= scale;
					right *= scale;
					up2 *= scale;
					
					if(playerInput.moveForward){
						followVel += front;
					}else if(playerInput.moveBackward){
						followVel -= front;
					}
					if(playerInput.moveLeft){
						followVel -= right;
					}else if(playerInput.moveRight){
						followVel += right;
					}
					if(playerInput.jump){
						followVel += up2;
					}else if(playerInput.crouch){
						followVel -= up2;
					}
					
					SPAssert(followVel.GetLength() < 100.f);
				}else if(player){
					// joined in a team
					PlayerInput inp = playerInput;
					WeaponInput winp = weapInput;
					
					if(inp.crouch == false){
						if(player->GetInput().crouch){
							if(!player->TryUncrouch(false)){
								inp.crouch = true;
							}
						}
					}
					if(inp.jump){
						if(!player->IsOnGroundOrWade())
							inp.jump = false;
					}

					if(clientPlayers[world->GetLocalPlayerIndex()]->IsChangingTool()) {
						winp.primary = false;
						winp.secondary = false;
					}
					
					if(player->GetTool() == Player::ToolWeapon &&
					   player->IsAwaitingReloadCompletion() &&
					   !player->GetWeapon()->IsReloadSlow()) {
						winp.primary = false;
					}
					
					player->SetInput(inp);
					player->SetWeaponInput(winp);
					
					//send player input
					// FIXME: send only there are any changed
					net->SendPlayerInput(inp);
					net->SendWeaponInput(weapInput);
					
					//PlayerInput actualInput = player->GetInput();
					WeaponInput actualWeapInput = player->GetWeaponInput();
					
					if(actualWeapInput.secondary && player->IsToolWeapon() &&
					   player->IsAlive()){
					}else{
						if(player->IsToolWeapon()){
							// there is a possibility that player has respawned or something.
							// stop aiming down
							weapInput.secondary = false;
						}
					}
					
					if(!player->IsToolSelectable(player->GetTool())) {
						// release mouse button before auto-switching tools
						winp.primary = false;
						winp.secondary = false;
						weapInput = winp;
						net->SendWeaponInput(weapInput);
						actualWeapInput = winp = player->GetWeaponInput();
						
						// select another tool
						Player::ToolType t = player->GetTool();
						do{
							switch(t){
								case Player::ToolSpade:
									t = Player::ToolGrenade;
									break;
								case Player::ToolBlock:
									t = Player::ToolSpade;
									break;
								case Player::ToolWeapon:
									t = Player::ToolBlock;
									break;
								case Player::ToolGrenade:
									t = Player::ToolWeapon;
									break;
							}
						}while(!world->GetLocalPlayer()->IsToolSelectable(t));
						SetSelectedTool(t);
					}
					
					Vector3 curFront = player->GetFront();
					if(curFront.x != lastFront.x ||
					   curFront.y != lastFront.y ||
					   curFront.z != lastFront.z) {
						lastFront = curFront;
						net->SendOrientation(curFront);
					}
					
					lastKills = world->GetPlayerPersistent(player->GetId()).kills;
					
					if(player->IsAlive())
						lastAliveTime = time;
					
					if(player->GetHealth() < lastHealth){
						// hurt!
						lastHealth = player->GetHealth();
						lastHurtTime = world->GetTime();
						
						Handle<IAudioChunk> c;
						switch((rand() >> 3) & 3){
							case 0:
								c = audioDevice->RegisterSound("Sounds/Weapons/Impacts/FleshLocal1.wav");
								break;
							case 1:
								c = audioDevice->RegisterSound("Sounds/Weapons/Impacts/FleshLocal2.wav");
								break;
							case 2:
								c = audioDevice->RegisterSound("Sounds/Weapons/Impacts/FleshLocal3.wav");
								break;
							case 3:
								c = audioDevice->RegisterSound("Sounds/Weapons/Impacts/FleshLocal4.wav");
								break;
						}
						
						float hpper = player->GetHealth() / 100.f;
						int cnt = 18 - (int)(player->GetHealth() / 100.f * 8.f);
						hurtSprites.resize(std::max(cnt, 6));
						for(size_t i = 0; i < hurtSprites.size(); i++) {
							HurtSprite& spr = hurtSprites[i];
							spr.angle = GetRandom() * (2.f * static_cast<float>(M_PI));
							spr.scale = .2f + GetRandom() * GetRandom() * .7f;
							spr.horzShift = GetRandom();
							spr.strength = .3f + GetRandom() * .7f;
							if(hpper > .5f) {
								spr.strength *= 1.5f - hpper;
							}
						}
						
						audioDevice->PlayLocal(c, AudioParam());
					}else{
						lastHealth = player->GetHealth();
					}
					
					inp.jump = false;
				}
				
#if 0
				world->Advance(dt);
#else
				// accurately resembles server's physics
				// but not smooth
				if(dt > 0.f)
					worldSubFrame += dt;
				
				float frameStep = 1.f / 60.f;
				while(worldSubFrame >= frameStep){
					world->Advance(frameStep);
					worldSubFrame -= frameStep;
				}
#endif
				
				for(size_t i = 0; i < clientPlayers.size(); i++){
					if(clientPlayers[i]){
						clientPlayers[i]->Update(dt);
					}
				}
				
				// corpse never accesses audio nor renderer
				class CorpseUpdateDispatch: public ConcurrentDispatch{
					Client *client;
					float dt;
				public:
					CorpseUpdateDispatch(Client *c, float dt):
					client(c), dt(dt){}
					virtual void Run(){
						std::list<Corpse *>::iterator it;
						for(it = client->corpses.begin(); it != client->corpses.end(); it++){
							for(int i = 0; i < 4; i++)
								(*it)->Update(dt / 4.f);
						}
					}
				};
				CorpseUpdateDispatch corpseDispatch(this, dt);
				corpseDispatch.Start();
				
				{
					std::list<ILocalEntity *>::iterator it;
					std::vector<std::list<ILocalEntity *>::iterator> its;
					for(it = localEntities.begin(); it != localEntities.end(); it++){
						if(!(*it)->Update(dt))
							its.push_back(it);
					}
					for(size_t i = 0; i < its.size(); i++){
						delete *(its[i]);
						localEntities.erase(its[i]);
					}
				}
				
				corpseDispatch.Join();
				
				if(grenadeVibration > 0.f){
					grenadeVibration -= dt;
					if(grenadeVibration < 0.f)
						grenadeVibration = 0.f;
				}
				
				if(time > lastPosSentTime + 1.f &&
				   world->GetLocalPlayer()){
					Player *p = world->GetLocalPlayer();
					if(p->IsAlive() && p->GetTeamId() < 2){
						net->SendPosition();
						lastPosSentTime = time;
					}
				}
				
			
			}else{
				
				renderer->SetFogColor(MakeVector3(0.f, 0.f, 0.f));
				
			}
			
			chatWindow->Update(dt);
			killfeedWindow->Update(dt);
			limbo->Update(dt);
			
			// SceneDef also can be used for sounds
			SceneDefinition sceneDef = SceneDef();
			lastSceneDef = sceneDef;
			
			// Update sounds
			try{
				audioDevice->Respatialize(sceneDef.viewOrigin,
										  sceneDef.viewAxis[2],
										  sceneDef.viewAxis[1]);
			}catch(const std::exception& ex){
				SPLog("Audio subsystem returned error (ignored):\n%s",
					  ex.what());
			}
					
			// render scene
			DrawScene();
			
			// draw 2d
			Draw2D();
			
			// draw scripted GUI
			scriptedUI->RunFrame(dt);
			if(scriptedUI->WantsClientToBeClosed())
				readyToClose = true;
			
			// Well done!
			renderer->FrameDone();
			renderer->Flip();
			
			time += dt;
		}
		
		void Client::Closing() {
			SPADES_MARK_FUNCTION();
			
		}
		
		void Client::MouseEvent(float x, float y) {
			SPADES_MARK_FUNCTION();
			
			if(scriptedUI->NeedsInput()) {
				scriptedUI->MouseEvent(x, y);
				return;
			}
			
			if(IsLimboViewActive()){
				limbo->MouseEvent(x, y);
				return;
			}
			
			if(IsFollowing()){
				SPAssert(world != NULL);
				if(world->GetLocalPlayer() &&
				   world->GetLocalPlayer()->GetTeamId() >= 2 &&
				   followingPlayerId == world->GetLocalPlayerIndex()){
					// invert dir
					x = -x; y = -y;
				}
				followYaw -= x * 0.003f;
				followPitch -= y * 0.003f;
				if(followPitch < -M_PI*.45f) followPitch = -static_cast<float>(M_PI)*.45f;
				if(followPitch > M_PI*.45f) followPitch = static_cast<float>(M_PI) * .45f;
				followYaw = fmodf(followYaw, static_cast<float>(M_PI)*2.f);
			}else if(world && world->GetLocalPlayer()){
				Player *p = world->GetLocalPlayer();
				float aimDownState = GetAimDownState();
				if(p->IsAlive()){
					x /= GetAimDownZoomScale();
					y /= GetAimDownZoomScale();
					
					if(aimDownState > 0.f) {
						float scale = cg_zoomedMouseSensScale;
						scale = powf(scale, aimDownState);
						x *= scale;
						y *= scale;
					}
					
					x *= (float)cg_mouseSensitivity;
					y *= (float)cg_mouseSensitivity;
					
					p->Turn(x * 0.003f, y * 0.003f);
				}
			}
		}
		
		void Client::CharEvent(const std::string &ch){
			SPADES_MARK_FUNCTION();
			
			if(scriptedUI->NeedsInput()) {
				scriptedUI->CharEvent(ch);
				return;
			}
			
			
			if(ch == "/") {
				scriptedUI->EnterCommandWindow();
			}
		}
		
		// TODO: this might not be a fast way
		//lm: ideally we should normalize the key when reading the config.
		static bool CheckKey(const std::string& cfg,
							 const std::string& input) {
			if(cfg.empty())
				return false;
			std::vector<std::string> keys = Split(cfg,",");
			static const std::string space1("space");
			static const std::string space2("spacebar");
			static const std::string space3("spacekey");
			for(size_t i = 0; i < keys.size(); i++) {
				std::string key = keys[i];
				if(EqualsIgnoringCase(key, space1) ||
				   EqualsIgnoringCase(key, space2) ||
				   EqualsIgnoringCase(key, space3)) {
					if(input == " ")
						return true;
				}else{
					if(EqualsIgnoringCase(key, input))
						return true;
				}
			}
			
			return false;
		}
		
		void Client::KeyEvent(const std::string& name, bool down){
			SPADES_MARK_FUNCTION();
			
			if(scriptedUI->NeedsInput()) {
				scriptedUI->KeyEvent(name, down);
				return;
			}
			
			if(name == "Escape"){
				if(down){
					if(inGameLimbo){
						inGameLimbo = false;
					}else{
						if(GetWorld() == NULL){
							// no world = loading now.
							// in this case, quit the game immediately.
							readyToClose = true;
						}
						else {
							scriptedUI->EnterClientMenu();
						}
					}
				}
			}else if(world){
				if(IsLimboViewActive()){
					if(down){
						limbo->KeyEvent(name);
					}
					return;
				}
				if(IsFollowing()){
					if(CheckKey(cg_keyAttack, name)){
						if(down){
							if(world->GetLocalPlayer()->GetTeamId() >= 2 ||
							   time > lastAliveTime + 1.3f)
								FollowNextPlayer();
						}
						return;
					}else if(CheckKey(cg_keyAltAttack, name)){
						if(down){
							if(world->GetLocalPlayer()){
								followingPlayerId = world->GetLocalPlayerIndex();
							}
						}
						return;
					}
				}
				if(world->GetLocalPlayer()){
					Player *p = world->GetLocalPlayer();
					
					if(p->IsAlive() && p->GetTool() == Player::ToolBlock && down) {
						if(paletteView->KeyInput(name)){
							return;
						}
					}
					
					if(name == "h" && down && false) {
						// debug
						int h = p->GetHealth();
						h -= 10; if(h <= 0) h = 100;
						p->SetHP(h, HurtTypeWeapon, MakeVector3(0, 0, 0));
					}
					
					if(cg_debugCorpse){
						if(name == "p" && down){
							Corpse *corp;
							Player *victim = world->GetLocalPlayer();
							corp = new Corpse(renderer, map, victim);
							corp->AddImpulse(victim->GetFront() * 32.f);
							corpses.push_back(corp);
							
							if(corpses.size() > corpseHardLimit){
								corp = corpses.front();
								delete corp;
								corpses.pop_front();
							}else if(corpses.size() > corpseSoftLimit){
								RemoveInvisibleCorpses();
							}
						}
					}
					if(CheckKey(cg_keyMoveLeft, name)){
						playerInput.moveLeft = down;
						keypadInput.left = down;
						if(down) playerInput.moveRight = false;
						else playerInput.moveRight = keypadInput.right;
					}else if(CheckKey(cg_keyMoveRight, name)){
						playerInput.moveRight = down;
						keypadInput.right = down;
						if(down) playerInput.moveLeft = false;
						else playerInput.moveLeft = keypadInput.left;
					}else if(CheckKey(cg_keyMoveForward, name)){
						playerInput.moveForward = down;
						keypadInput.forward = down;
						if(down) playerInput.moveBackward = false;
						else playerInput.moveBackward = keypadInput.backward;
					}else if(CheckKey(cg_keyMoveBackward, name)){
						playerInput.moveBackward = down;
						keypadInput.backward = down;
						if(down) playerInput.moveForward = false;
						else playerInput.moveForward = keypadInput.forward;
					}else if(CheckKey(cg_keyCrouch, name)){
						playerInput.crouch = down;
					}else if(CheckKey(cg_keySprint, name)){
						playerInput.sprint = down;
						if(down){
							if(world->GetLocalPlayer()->IsToolWeapon()){
								weapInput.secondary = false;
							}
						}
					}else if(CheckKey(cg_keySneak, name)){
						playerInput.sneak = down;
					}else if(CheckKey(cg_keyJump, name)){
						playerInput.jump = down;
					}else if(CheckKey(cg_keyAttack, name)){
						weapInput.primary = down;
					}else if(CheckKey(cg_keyAltAttack, name)){
						if(world->GetLocalPlayer()->IsToolWeapon() && (!cg_holdAimDownSight)){
							if(down && !playerInput.sprint){
								weapInput.secondary = !weapInput.secondary;
							}
						}else{
							if(!playerInput.sprint){
								weapInput.secondary = down;
							}else{
								weapInput.secondary = down;
							}
						}
					}else if(CheckKey(cg_keyReloadWeapon, name) && down){
						Weapon *w = world->GetLocalPlayer()->GetWeapon();
						if(w->GetAmmo() < w->GetClipSize() &&
						   w->GetStock() > 0 &&
						   (!world->GetLocalPlayer()->IsAwaitingReloadCompletion()) &&
						   (!w->IsReloading()) &&
						   world->GetLocalPlayer()->GetTool() == Player::ToolWeapon){
							world->GetLocalPlayer()->Reload();
							if(world->GetLocalPlayer()->IsToolWeapon()){
								weapInput.secondary = false;
							}
							net->SendReload();
						}
					}else if(CheckKey(cg_keyToolSpade, name) && down){
						if(world->GetLocalPlayer()->GetTeamId() < 2 &&
						   world->GetLocalPlayer()->IsAlive() &&
						   world->GetLocalPlayer()->IsToolSelectable(Player::ToolSpade)){
							SetSelectedTool(Player::ToolSpade);
						}
					}else if(CheckKey(cg_keyToolBlock, name) && down){
						if(world->GetLocalPlayer()->GetTeamId() < 2 &&
						   world->GetLocalPlayer()->IsAlive() &&
						   world->GetLocalPlayer()->IsToolSelectable(Player::ToolBlock)){
							SetSelectedTool(Player::ToolBlock);
						}
					}else if(CheckKey(cg_keyToolWeapon, name) && down){
						if(world->GetLocalPlayer()->GetTeamId() < 2 &&
						   world->GetLocalPlayer()->IsAlive() &&
						   world->GetLocalPlayer()->IsToolSelectable(Player::ToolWeapon)){
							SetSelectedTool(Player::ToolWeapon);
						}
					}else if(CheckKey(cg_keyToolGrenade, name) && down){
						if(world->GetLocalPlayer()->GetTeamId() < 2 &&
						   world->GetLocalPlayer()->IsAlive() &&
						   world->GetLocalPlayer()->IsToolSelectable(Player::ToolGrenade)){
							SetSelectedTool(Player::ToolGrenade);
						}
					}else if(CheckKey(cg_keyGlobalChat, name) && down){
						// global chat
						scriptedUI->EnterGlobalChatWindow();
					}else if(CheckKey(cg_keyTeamChat, name) && down){
						// team chat
						scriptedUI->EnterTeamChatWindow();
					}else if(CheckKey(cg_keyCaptureColor, name) && down){
						CaptureColor();
					}else if(CheckKey(cg_keyChangeMapScale, name) && down){
						mapView->SwitchScale();
						Handle<IAudioChunk> chunk = audioDevice->RegisterSound("Sounds/Misc/SwitchMapZoom.wav");
						audioDevice->PlayLocal(chunk, AudioParam());
					}else if(CheckKey(cg_keyToggleMapZoom, name) && down){
						if(largeMapView->ToggleZoom()){
							Handle<IAudioChunk> chunk = audioDevice->RegisterSound("Sounds/Misc/OpenMap.wav");
							audioDevice->PlayLocal(chunk, AudioParam());
						}else{
							Handle<IAudioChunk> chunk = audioDevice->RegisterSound("Sounds/Misc/CloseMap.wav");
							audioDevice->PlayLocal(chunk, AudioParam());
						}
					}else if(CheckKey(cg_keyScoreboard, name)){
						scoreboardVisible = down;
					}else if(CheckKey(cg_keyLimbo, name) && down){
						limbo->SetSelectedTeam(world->GetLocalPlayer()->GetTeamId());
						limbo->SetSelectedWeapon(world->GetLocalPlayer()->GetWeapon()->GetWeaponType());
						inGameLimbo = true;
					}else if(CheckKey(cg_keySceneshot, name) && down){
						TakeScreenShot(true);
					}else if(CheckKey(cg_keyScreenshot, name) && down){
						TakeScreenShot(false);
					}else if(CheckKey(cg_keySaveMap, name) && down){
						TakeMapShot();
					}else if(CheckKey(cg_keyFlashlight, name) && down){
						flashlightOn = !flashlightOn;
						flashlightOnTime = time;
						Handle<IAudioChunk> chunk = audioDevice->RegisterSound("Sounds/Player/Flashlight.wav");
						audioDevice->PlayLocal(chunk, AudioParam());
					}else if(cg_switchToolByWheel && down) {
						bool rev = (int)cg_switchToolByWheel > 0;
						if(name == (rev ? "WheelDown":"WheelUp")) {
							if(world->GetLocalPlayer()->GetTeamId() < 2 &&
							   world->GetLocalPlayer()->IsAlive()){
								Player::ToolType t = world->GetLocalPlayer()->GetTool();
								do{
									switch(t){
										case Player::ToolSpade:
											t = Player::ToolGrenade;
											break;
										case Player::ToolBlock:
											t = Player::ToolSpade;
											break;
										case Player::ToolWeapon:
											t = Player::ToolBlock;
											break;
										case Player::ToolGrenade:
											t = Player::ToolWeapon;
											break;
									}
								}while(!world->GetLocalPlayer()->IsToolSelectable(t));
								SetSelectedTool(t);
							}
						}else if(name == (rev ? "WheelUp":"WheelDown")) {
							if(world->GetLocalPlayer()->GetTeamId() < 2 &&
							   world->GetLocalPlayer()->IsAlive()){
								Player::ToolType t = world->GetLocalPlayer()->GetTool();
								do{
									switch(t){
										case Player::ToolSpade:
											t = Player::ToolBlock;
											break;
										case Player::ToolBlock:
											t = Player::ToolWeapon;
											break;
										case Player::ToolWeapon:
											t = Player::ToolGrenade;
											break;
										case Player::ToolGrenade:
											t = Player::ToolSpade;
											break;
									}
								}while(!world->GetLocalPlayer()->IsToolSelectable(t));
								SetSelectedTool(t);
							}
						}
					}
				}else{
					// limbo
				}
			}
		}
		
		void Client::CaptureColor() {
			if(!world) return;
			Player *p = world->GetLocalPlayer();
			if(!p) return;
			if(!p->IsAlive()) return;
			
			IntVector3 outBlockCoord;
			if(!world->GetMap()->CastRay(p->GetEye(),
										 p->GetFront(),
										 256.f, outBlockCoord)){
				return;
			}
			
			uint32_t col = world->GetMap()->GetColorWrapped(outBlockCoord.x,
															outBlockCoord.y,
															outBlockCoord.z);
			
			IntVector3 colV;
			colV.x = (uint8_t)(col);
			colV.y = (uint8_t)(col >> 8);
			colV.z = (uint8_t)(col >> 16);
			
			p->SetHeldBlockColor(colV);
			net->SendHeldBlockColor();
		}
		
		bool Client::IsLimboViewActive(){
			if(world){
				if(!world->GetLocalPlayer()){
					return true;
				}else if(inGameLimbo){
					return true;
				}
			}
			// TODO: anytime limbo
			return false;
		}
		
		void Client::SpawnPressed() {
			WeaponType weap = limbo->GetSelectedWeapon();
			int team = limbo->GetSelectedTeam();
			inGameLimbo = false;
			if(team == 2)
				team = 255;
			
			if(!world->GetLocalPlayer()){
				// join
				net->SendJoin(team, weap,
							 playerName, lastKills);
			}else{
				Player *p = world->GetLocalPlayer();
				if(p->GetTeamId() != team){
					net->SendTeamChange(team);
				}
				if(team != 2 && p->GetWeapon()->GetWeaponType() != weap){
					net->SendWeaponChange(weap);
				}
			}
		}
		
		void Client::NetLog(const char *format, ...) {
			char buf[4096];
			va_list va;
			va_start(va, format);
			vsprintf(buf, format, va);
			va_end(va);
			std::string str = buf;
			
			time_t t;
			struct tm tm;
			::time(&t);
			tm = *localtime(&t);
			
			std::string timeStr = asctime(&tm);
			
			// remove '\n' in the end of the result of asctime().
			timeStr.resize(timeStr.size()-1);
			
			sprintf(buf, "%s %s\n",
					timeStr.c_str(), str.c_str());
			
			printf("%s", buf);
			
			if(logStream) {
				logStream->Write(buf);
				logStream->Flush();
			}
		}
		
		float Client::GetSprintState() {
			if(!world) return 0.f;
			if(!world->GetLocalPlayer())
				return 0.f;
			
			ClientPlayer *p = clientPlayers[(int)world->GetLocalPlayerIndex()];
			if(!p)
				return 0.f;
			return p->GetSprintState();
		}
		
		float Client::GetAimDownState() {
			if(!world) return 0.f;
			if(!world->GetLocalPlayer())
				return 0.f;
			
			ClientPlayer *p = clientPlayers[(int)world->GetLocalPlayerIndex()];
			if(!p)
				return 0.f;
			return p->GetAimDownState();
		}
		
		void Client::SetSelectedTool(Player::ToolType type, bool quiet) {
			if(type == world->GetLocalPlayer()->GetTool())
				return;
			world->GetLocalPlayer()->SetTool(type);
			net->SendTool();
			
			if(!quiet) {
				Handle<IAudioChunk> c = audioDevice->RegisterSound("Sounds/Weapons/SwitchLocal.wav");
				audioDevice->PlayLocal(c, MakeVector3(.4f, -.3f, .5f),
									   AudioParam());
			}
		}
		
#pragma mark - Drawing
		
		void Client::TakeMapShot(){
			
			try{
				std::string name = MapShotPath();
				IStream *stream = FileManager::OpenForWriting(name.c_str());
				try{
					GameMap *map = GetWorld()->GetMap();
					if(map == NULL){
						SPRaise("No map loaded");
					}
					map->Save(stream);
					delete stream;
				}catch(...){
					delete stream;
					throw;
				}
				
				std::string msg;
				msg = "Map saved: " + name;
				msg = ChatWindow::ColoredMessage(msg, MsgColorSysInfo);
				chatWindow->AddMessage(msg);
			}catch(const std::exception& ex){
				std::string msg;
				msg = "Saving map failed: ";
				std::vector<std::string> lines = SplitIntoLines(ex.what());
				msg += lines[0];
				msg = ChatWindow::ColoredMessage(msg, MsgColorRed);
				chatWindow->AddMessage(msg);
			}
		}
		
		std::string Client::MapShotPath() {
			char buf[256];
			for(int i = 0; i < 10000;i++){
				sprintf(buf, "Mapshots/shot%04d.vxl", nextScreenShotIndex);
				if(FileManager::FileExists(buf)){
					nextScreenShotIndex++;
					if(nextScreenShotIndex >= 10000)
						nextScreenShotIndex = 0;
					continue;
				}
				
				return buf;
			}
			
			SPRaise("No free file name");
		}
		
		void Client::TakeScreenShot(bool sceneOnly){
			SceneDefinition sceneDef = SceneDef();
			lastSceneDef = sceneDef;
			
			// render scene
			flashDlights = flashDlightsOld;
			DrawScene();
			
			// draw 2d
			if(!sceneOnly)
				Draw2D();
			
			// Well done!
			renderer->FrameDone();
			
			Handle<Bitmap> bmp(renderer->ReadBitmap(), false);
			// force 100% opacity
			
			uint32_t *pixels = bmp->GetPixels();
			for(size_t i = bmp->GetWidth() * bmp->GetHeight(); i > 0; i--) {
				*(pixels++) |= 0xff000000UL;
			}
			
			try{
				std::string name = ScreenShotPath();
				bmp->Save(name);
				
				std::string msg;
				if(sceneOnly)
					msg = "Sceneshot saved: ";
				else
					msg = "Screenshot saved: ";
				msg += name;
				msg = ChatWindow::ColoredMessage(msg, MsgColorSysInfo);
				chatWindow->AddMessage(msg);
			}catch(const std::exception& ex){
				std::string msg;
				msg = "Screenshot failed: ";
				std::vector<std::string> lines = SplitIntoLines(ex.what());
				msg += lines[0];
				msg = ChatWindow::ColoredMessage(msg, MsgColorRed);
				chatWindow->AddMessage(msg);
			}
		}
		
		std::string Client::ScreenShotPath() {
			char buf[256];
			for(int i = 0; i < 10000;i++){
				sprintf(buf, "Screenshots/shot%04d.tga", nextScreenShotIndex);
				if(FileManager::FileExists(buf)){
					nextScreenShotIndex++;
					if(nextScreenShotIndex >= 10000)
						nextScreenShotIndex = 0;
					continue;
				}
				
				return buf;
			}
			
			SPRaise("No free file name");
		}
		
		bool Client::ShouldRenderInThirdPersonView() {
			//^return true;
			
			if(world && world->GetLocalPlayer()){
				if(!world->GetLocalPlayer()->IsAlive())
					return true;
			}
			return false;
		}
		
		float Client::GetLocalFireVibration() {
			float localFireVibration = 0.f;
			localFireVibration = time - localFireVibrationTime;
			localFireVibration = 1.f - localFireVibration / 0.1f;
			if(localFireVibration < 0.f)
				localFireVibration = 0.f;
			return localFireVibration;
		}
		
		float Client::GetAimDownZoomScale(){
			if(world == NULL || world->GetLocalPlayer() == NULL ||
			   world->GetLocalPlayer()->IsToolWeapon() == false ||
			   world->GetLocalPlayer()->IsAlive() == false)
				return 1.f;
			float delta = .8f;
			switch(world->GetLocalPlayer()->GetWeapon()->GetWeaponType()) {
				case SMG_WEAPON:
					delta = .8f;
					break;
				case RIFLE_WEAPON:
					delta = 1.4f;
					break;
				case SHOTGUN_WEAPON:
					delta = .4f;
					break;
			}
			float aimDownState = GetAimDownState();
			return 1.f + powf(aimDownState, 5.f) * delta;
		}
		
		SceneDefinition Client::SceneDef() {
			SPADES_MARK_FUNCTION();
			
			SceneDefinition def;
			def.time = (unsigned int)(time * 1000.f);
			def.denyCameraBlur = true;
			
			if(world){
				IntVector3 fogColor = world->GetFogColor();
				renderer->SetFogColor(MakeVector3(fogColor.x / 255.f,
												  fogColor.y / 255.f,
												  fogColor.z / 255.f));
				
				Player *player = world->GetLocalPlayer();
				
				def.blurVignette = .4f;
				
				if(IsFollowing()){
					int limit = 100;
					// if current following player has left,
					// or removed,
					// choose next player.
					while(!world->GetPlayer(followingPlayerId) ||
						  world->GetPlayer(followingPlayerId)->GetFront().GetPoweredLength() < .01f){
						FollowNextPlayer();
						if((limit--) <= 0){
							break;
						}
					}
					player = world->GetPlayer(followingPlayerId);
				}
				if(player){
					
					float roll = 0.f;
					float scale = 1.f;
					float vibPitch = 0.f;
					float vibYaw = 0.f;
					if(ShouldRenderInThirdPersonView() ||
					   (IsFollowing() && player != world->GetLocalPlayer())){
						Vector3 center = player->GetEye();
						Vector3 playerFront = player->GetFront2D();
						Vector3 up = MakeVector3(0,0,-1);
						
						if((!player->IsAlive()) && lastMyCorpse &&
						   player == world->GetLocalPlayer()){
							center = lastMyCorpse->GetCenter();
						}
						if(map->IsSolidWrapped((int)floorf(center.x),
										  (int)floorf(center.y),
										  (int)floorf(center.z))){
							float z = center.z;
							while(z > center.z - 5.f){
								if(!map->IsSolidWrapped((int)floorf(center.x),
														(int)floorf(center.y),
														(int)floorf(z))){
									center.z = z;
									break;
								}else{
									z -= 1.f;
								}
							}
						}
						
						float distance = 5.f;
						if(player == world->GetLocalPlayer() &&
						   world->GetLocalPlayer()->GetTeamId() < 2 &&
						   !world->GetLocalPlayer()->IsAlive()){
							// deathcam.
							float elapsedTime = time - lastAliveTime;
							distance -= 3.f * expf(-elapsedTime * 1.f);
						}
						
						Vector3 eye = center;
						//eye -= playerFront * 5.f;
						//eye += up * 2.0f;
						eye.x += cosf(followYaw) * cosf(followPitch) * distance;
						eye.y += sinf(followYaw) * cosf(followPitch) * distance;
						eye.z -= sinf(followPitch) * distance;
						
						if(false){
							// settings for making limbo stuff
							eye = center;
							eye += playerFront * 3.f;
							eye += up * -.1f;
							eye += player->GetRight() *2.f;
							scale *= .6f;
						}
						
						// try ray casting
						GameMap::RayCastResult result;
						result = map->CastRay2(center, (eye - center).Normalize(), 256);
						if(result.hit) {
							float dist = (result.hitPos - center).GetLength();
							float curDist = (eye - center).GetLength();
							dist -= 0.3f; // near clip plane
							if(curDist > dist){
								float diff = curDist - dist;
								eye += (center - eye).Normalize() * diff;
							}
						}
						
						Vector3 front = center - eye;
						front = front.Normalize();
						
						def.viewOrigin = eye;
						def.viewAxis[0] = -Vector3::Cross(up, front).Normalize();
						def.viewAxis[1] = -Vector3::Cross(front, def.viewAxis[0]).Normalize();
						def.viewAxis[2] = front;
						
						
						def.fovY = (float)cg_fov * static_cast<float>(M_PI) /180.f;
						def.fovX = atanf(tanf(def.fovY * .5f) *
										 renderer->ScreenWidth() /
										 renderer->ScreenHeight()) * 2.f;
						
						// update initial spectate pos
						// this is not used now, but if the local player is
						// is spectating, this is used when he/she's no
						// longer following
						followPos = def.viewOrigin;
						followVel = MakeVector3(0, 0, 0);
						
					}else if(player->GetTeamId() >= 2){
						// spectator view (noclip view)
						Vector3 center = followPos;
						Vector3 front;
						Vector3 up = {0, 0, -1};
						
						front.x = -cosf(followYaw) * cosf(followPitch);
						front.y = -sinf(followYaw) * cosf(followPitch);
						front.z = sinf(followPitch);
						
						def.viewOrigin = center;
						def.viewAxis[0] = -Vector3::Cross(up, front).Normalize();
						def.viewAxis[1] = -Vector3::Cross(front, def.viewAxis[0]).Normalize();
						def.viewAxis[2] = front;
						
						def.fovY = (float)cg_fov * static_cast<float>(M_PI) /180.f;
						def.fovX = atanf(tanf(def.fovY * .5f) *
										 renderer->ScreenWidth() /
										 renderer->ScreenHeight()) * 2.f;
						
						// for 1st view, camera blur can be used
						def.denyCameraBlur = false;

					}else{
						Vector3 front = player->GetFront();
						Vector3 right = player->GetRight();
						Vector3 up = player->GetUp();
						
						float localFireVibration = GetLocalFireVibration();
						localFireVibration *= localFireVibration;
						
						roll += (nextRandom() - nextRandom()) * 0.03f * localFireVibration;
						scale += nextRandom() * 0.04f * localFireVibration;
						
						vibPitch += localFireVibration * (1.f - localFireVibration) * 0.01f;
						vibYaw += sinf(localFireVibration * (float)M_PI * 2.f) * 0.001f;
						
						// sprint bob
						{
							float sp = SmoothStep(GetSprintState());
							vibYaw += sinf(player->GetWalkAnimationProgress() * static_cast<float>(M_PI) * 2.f) * 0.01f * sp;
							roll -= sinf(player->GetWalkAnimationProgress() * static_cast<float>(M_PI) * 2.f) * 0.005f * (sp);
							float p = cosf(player->GetWalkAnimationProgress() * static_cast<float>(M_PI) * 2.f);
							p = p * p; p *= p; p *= p; p *= p; 
							vibPitch += p * 0.01f * sp;
						}
						
						
						
						scale /= GetAimDownZoomScale();
						
						def.viewOrigin = player->GetEye();
						def.viewAxis[0] = right;
						def.viewAxis[1] = up;
						def.viewAxis[2] = front;
						
						def.fovY = (float)cg_fov * static_cast<float>(M_PI) /180.f;
						def.fovX = atanf(tanf(def.fovY * .5f) *
										 renderer->ScreenWidth() /
										 renderer->ScreenHeight()) * 2.f;
						
						// for 1st view, camera blur can be used
						def.denyCameraBlur = false;
						
						float aimDownState = GetAimDownState();
						float per = aimDownState;
						per *= per * per;
						def.depthOfFieldNearRange = per * 13.f + .054f;
					
						def.blurVignette = .4f;
						
						
						
					}
					
					// add vibration for both 1st/3rd view
					{
						// add grenade vibration
						float grenVib = grenadeVibration;
						if(grenVib > 0.f){
							grenVib *= 10.f;
							if(grenVib > 1.f)
								grenVib = 1.f;
							roll += (nextRandom() - nextRandom()) * 0.2f * grenVib;
							vibPitch += (nextRandom() - nextRandom()) * 0.1f * grenVib;
							vibYaw += (nextRandom() - nextRandom()) * 0.1f * grenVib;
							scale -= (nextRandom()-nextRandom()) * 0.1f * grenVib;
						}
					}
					
					// add roll / scale
					{
						Vector3 right = def.viewAxis[0];
						Vector3 up = def.viewAxis[1];
						
						def.viewAxis[0] = right * cosf(roll) - up * sinf(roll);
						def.viewAxis[1] = up * cosf(roll) + right * sinf(roll);
						
                        def.fovX = atanf(tanf(def.fovX * .5f) * scale) * 2.f;
                        def.fovY = atanf(tanf(def.fovY * .5f) * scale) * 2.f;
					}
					{
						Vector3 u = def.viewAxis[1];
						Vector3 v = def.viewAxis[2];
						
						def.viewAxis[1] = u * cosf(vibPitch) - v * sinf(vibPitch);
						def.viewAxis[2] = v * cosf(vibPitch) + u * sinf(vibPitch);
					}
					{
						Vector3 u = def.viewAxis[0];
						Vector3 v = def.viewAxis[2];
						
						def.viewAxis[0] = u * cosf(vibYaw) - v * sinf(vibYaw);
						def.viewAxis[2] = v * cosf(vibYaw) + u * sinf(vibYaw);
					}
					{
						float wTime = world->GetTime();
						if(wTime < lastHurtTime + .15f &&
                           wTime >= lastHurtTime){
							float per = 1.f - (wTime - lastHurtTime) / .15f;
							per *= .5f - player->GetHealth() / 100.f * .3f;
							def.blurVignette += per * 6.f;
						}
						if(wTime < lastHurtTime + .2f &&
                           wTime >= lastHurtTime){
							float per = 1.f - (wTime - lastHurtTime) / .2f;
							per *= .5f - player->GetHealth() / 100.f * .3f;
							def.saturation *= std::max(0.f, 1.f - per * 4.f);
						}
					}
					
					def.zNear = 0.05f;
					def.zFar = 130.f;
					
					def.skipWorld = false;
				}else{
					def.viewOrigin = MakeVector3(256, 256, 4);
					def.viewAxis[0] = MakeVector3(-1, 0, 0);
					def.viewAxis[1] = MakeVector3(0, 1, 0);
					def.viewAxis[2] = MakeVector3(0, 0, 1);
					
					def.fovY = (float)cg_fov * static_cast<float>(M_PI) /180.f;
					def.fovX = atanf(tanf(def.fovY * .5f) *
									 renderer->ScreenWidth() /
									 renderer->ScreenHeight()) * 2.f;
					
					def.zNear = 0.05f;
					def.zFar = 130.f;
					
					def.skipWorld = false;
				}
					
			}else{
				def.viewOrigin = MakeVector3(0, 0, 0);
				def.viewAxis[0] = MakeVector3(1, 0, 0);
				def.viewAxis[1] = MakeVector3(0, 0, -1);
				def.viewAxis[2] = MakeVector3(0, 0, 1);
				
				def.fovY = (float)cg_fov * static_cast<float>(M_PI) /180.f;
				def.fovX = atanf(tanf(def.fovY * .5f) *
								 renderer->ScreenWidth() /
								 renderer->ScreenHeight()) * 2.f;
				
				def.zNear = 0.05f;
				def.zFar = 130.f;
				
				def.skipWorld = true;
				
				renderer->SetFogColor(MakeVector3(0,0,0));
			}
			
			SPAssert(!isnan(def.viewOrigin.x));
			SPAssert(!isnan(def.viewOrigin.y));
			SPAssert(!isnan(def.viewOrigin.z));
			
			return def;
		}
		
		std::string Client::GetWeaponPrefix(std::string fold, WeaponType type){
			SPADES_MARK_FUNCTION_DEBUG();
			
			std::string weapPrefix;
			switch(type){
				case RIFLE_WEAPON:
					weapPrefix = fold + "/Weapons/Rifle";
					break;
				case SMG_WEAPON:
					weapPrefix = fold + "/Weapons/SMG";
					break;
				case SHOTGUN_WEAPON:
					weapPrefix = fold + "/Weapons/Shotgun";
					break;
			}
			return weapPrefix;
		}
		
		void Client::AddGrenadeToScene(spades::client::Grenade *g) {
			SPADES_MARK_FUNCTION();
			
			IModel *model;
			model = renderer->RegisterModel("Models/Weapons/Grenade/Grenade.kv6");
			
			if(g->GetPosition().z > 63.f) {
				// work-around for water refraction problem
				return;
			}
			
			ModelRenderParam param;
			Matrix4 mat = Matrix4::Scale(0.03f);
			mat = Matrix4::Translate(g->GetPosition()) * mat;
			param.matrix = mat;
			
			renderer->RenderModel(model, param);
		}
		
		void Client::AddDebugObjectToScene(const spades::OBB3 &obb, const Vector4& color) {
			const Matrix4& mat = obb.m;
			Vector3 v[2][2][2];
			v[0][0][0] = (mat * MakeVector3(0,0,0)).GetXYZ();
			v[0][0][1] = (mat * MakeVector3(0,0,1)).GetXYZ();
			v[0][1][0] = (mat * MakeVector3(0,1,0)).GetXYZ();
			v[0][1][1] = (mat * MakeVector3(0,1,1)).GetXYZ();
			v[1][0][0] = (mat * MakeVector3(1,0,0)).GetXYZ();
			v[1][0][1] = (mat * MakeVector3(1,0,1)).GetXYZ();
			v[1][1][0] = (mat * MakeVector3(1,1,0)).GetXYZ();
			v[1][1][1] = (mat * MakeVector3(1,1,1)).GetXYZ();
			
			renderer->AddDebugLine(v[0][0][0], v[1][0][0], color);
			renderer->AddDebugLine(v[0][0][1], v[1][0][1], color);
			renderer->AddDebugLine(v[0][1][0], v[1][1][0], color);
			renderer->AddDebugLine(v[0][1][1], v[1][1][1], color);
			
			renderer->AddDebugLine(v[0][0][0], v[0][1][0], color);
			renderer->AddDebugLine(v[0][0][1], v[0][1][1], color);
			renderer->AddDebugLine(v[1][0][0], v[1][1][0], color);
			renderer->AddDebugLine(v[1][0][1], v[1][1][1], color);
			
			renderer->AddDebugLine(v[0][0][0], v[0][0][1], color);
			renderer->AddDebugLine(v[0][1][0], v[0][1][1], color);
			renderer->AddDebugLine(v[1][0][0], v[1][0][1], color);
			renderer->AddDebugLine(v[1][1][0], v[1][1][1], color);
		}
		
		void Client::DrawCTFObjects() {
			SPADES_MARK_FUNCTION();
			CTFGameMode *mode = dynamic_cast<CTFGameMode *>(world->GetMode());
			int tId;
			IModel *base = renderer->RegisterModel("Models/MapObjects/CheckPoint.kv6");
			IModel *intel = renderer->RegisterModel("Models/MapObjects/Intel.kv6");
			for(tId = 0; tId < 2; tId++){
				CTFGameMode::Team& team = mode->GetTeam(tId);
				IntVector3 col = world->GetTeam(tId).color;
				Vector3 color = {
					col.x / 255.f, col.y / 255.f, col.z / 255.f
				};
				
				ModelRenderParam param;
				param.customColor = color;
				
				// draw base
				param.matrix = Matrix4::Translate(team.basePos);
				param.matrix = param.matrix * Matrix4::Scale(.3f);
				renderer->RenderModel(base, param);
				
				// draw flag
				if(!mode->GetTeam(1-tId).hasIntel){
					param.matrix = Matrix4::Translate(team.flagPos);
					param.matrix = param.matrix * Matrix4::Scale(.1f);
					renderer->RenderModel(intel, param);
				}
			}
		}
		
		void Client::DrawTCObjects() {
			SPADES_MARK_FUNCTION();
			TCGameMode *mode = dynamic_cast<TCGameMode *>(world->GetMode());
			int tId;
			IModel *base = renderer->RegisterModel("Models/MapObjects/CheckPoint.kv6");
			int cnt = mode->GetNumTerritories();
			for(tId = 0; tId < cnt; tId++){
				TCGameMode::Territory *t = mode->GetTerritory(tId);
				IntVector3 col;
				if(t->ownerTeamId == 2){
					col = IntVector3::Make(255, 255, 255);
				}else{
					col = world->GetTeam(t->ownerTeamId).color;
				}
				Vector3 color = {
					col.x / 255.f, col.y / 255.f, col.z / 255.f
				};
				
				ModelRenderParam param;
				param.customColor = color;
				
				// draw base
				param.matrix = Matrix4::Translate(t->pos);
				param.matrix = param.matrix * Matrix4::Scale(.3f);
				renderer->RenderModel(base, param);
				
			}
		}
		
		void Client::RemoveAllCorpses(){
			SPADES_MARK_FUNCTION();
			
			std::list<Corpse *>::iterator it;
			for(it = corpses.begin(); it != corpses.end(); it++)
				delete *it;
			corpses.clear();
			lastMyCorpse = NULL;
		}
		
		
		void Client::RemoveAllLocalEntities(){
			SPADES_MARK_FUNCTION();
			
			std::list<ILocalEntity *>::iterator it;
			for(it = localEntities.begin(); it != localEntities.end(); it++)
				delete *it;
			localEntities.clear();
		}
		
		void Client::RemoveInvisibleCorpses(){
			SPADES_MARK_FUNCTION();
			
			std::vector<std::list<Corpse *>::iterator> its;
			std::list<Corpse *>::iterator it;
			int cnt = (int)corpses.size() - corpseSoftLimit;
			for(it = corpses.begin(); it != corpses.end(); it++){
				if(cnt <= 0)
					break;
				Corpse *c = *it;
				if(!c->IsVisibleFrom(lastSceneDef.viewOrigin)){
					if(c == lastMyCorpse)
						lastMyCorpse = NULL;
					delete c;
					its.push_back(it);
				}
				cnt--;
			}
			
			for(size_t i = 0; i < its.size(); i++)
				corpses.erase(its[i]);
			
		}
		
		void Client::DrawScene(){
			SPADES_MARK_FUNCTION();
			
			renderer->StartScene(lastSceneDef);
			
			if(world){
				Player *p = world->GetLocalPlayer();
				
				for(size_t i = 0; i < world->GetNumPlayerSlots(); i++)
					if(world->GetPlayer(i)){
						SPAssert(clientPlayers[i]);
						clientPlayers[i]->AddToScene();
					}
				std::vector<Grenade *> nades = world->GetAllGrenades();
				for(size_t i = 0; i < nades.size(); i++){
					AddGrenadeToScene(nades[i]);
				}
				
				{
					std::list<Corpse *>::iterator it;
					for(it = corpses.begin(); it != corpses.end(); it++){
						Vector3 center = (*it)->GetCenter();
						if((center - lastSceneDef.viewOrigin).GetPoweredLength() > 150.f * 150.f)
							continue;
						(*it)->AddToScene();
					}
				}
				
				if(dynamic_cast<CTFGameMode *>(world->GetMode())){
					DrawCTFObjects();
				}
				if(dynamic_cast<TCGameMode *>(world->GetMode())){
					DrawTCObjects();
				}
				
				{
					std::list<ILocalEntity *>::iterator it;
					for(it = localEntities.begin(); it != localEntities.end(); it++){
						(*it)->Render3D();
					}
				}
				
				// draw block cursor
				// FIXME: don't use debug line
				if(p){
					if(p->IsBlockCursorActive() &&
					   p->IsReadyToUseTool()){
						std::vector<IntVector3> blocks;
						if(p->IsBlockCursorDragging()){
							blocks = world->CubeLine(p->GetBlockCursorDragPos(),
													 p->GetBlockCursorPos(),
													 256);
						}else{
							blocks.push_back(p->GetBlockCursorPos());
						}
						
						Vector4 color = {1,1,1,1};
						if((int)blocks.size() > p->GetNumBlocks())
							color = MakeVector4(1,0,0,1);
						
						for(size_t i = 0; i < blocks.size(); i++){
							IntVector3& v = blocks[i];
							
							renderer->AddDebugLine(MakeVector3(v.x, v.y, v.z),
												   MakeVector3(v.x+1, v.y, v.z),
												   color);
							renderer->AddDebugLine(MakeVector3(v.x, v.y+1, v.z),
												   MakeVector3(v.x+1, v.y+1, v.z),
												   color);
							renderer->AddDebugLine(MakeVector3(v.x, v.y, v.z+1),
												   MakeVector3(v.x+1, v.y, v.z+1),
												   color);
							renderer->AddDebugLine(MakeVector3(v.x, v.y+1, v.z+1),
												   MakeVector3(v.x+1, v.y+1, v.z+1),
												   color);
							renderer->AddDebugLine(MakeVector3(v.x, v.y, v.z),
												   MakeVector3(v.x+1, v.y, v.z),
												   color);
							
							renderer->AddDebugLine(MakeVector3(v.x, v.y, v.z),
												   MakeVector3(v.x, v.y+1, v.z),
												   color);
							renderer->AddDebugLine(MakeVector3(v.x, v.y, v.z+1),
												   MakeVector3(v.x, v.y+1, v.z+1),
												   color);
							renderer->AddDebugLine(MakeVector3(v.x+1, v.y, v.z),
												   MakeVector3(v.x+1, v.y+1, v.z),
												   color);
							renderer->AddDebugLine(MakeVector3(v.x+1, v.y, v.z+1),
												   MakeVector3(v.x+1, v.y+1, v.z+1),
												   color);
							
							renderer->AddDebugLine(MakeVector3(v.x, v.y, v.z),
												   MakeVector3(v.x, v.y, v.z+1),
												   color);
							renderer->AddDebugLine(MakeVector3(v.x, v.y+1, v.z),
												   MakeVector3(v.x, v.y+1, v.z+1),
												   color);
							renderer->AddDebugLine(MakeVector3(v.x+1, v.y, v.z),
												   MakeVector3(v.x+1, v.y, v.z+1),
												   color);
							renderer->AddDebugLine(MakeVector3(v.x+1, v.y+1, v.z),
												   MakeVector3(v.x+1, v.y+1, v.z+1),
												   color);
							
						}
					}
				}
			}
			
			for(size_t i = 0; i < flashDlights.size(); i++)
				renderer->AddLight(flashDlights[i]);
			flashDlightsOld.clear();
			flashDlightsOld.swap(flashDlights);
			
			// draw player hottrack
			// FIXME: don't use debug line
			{
				hitTag_t tag = hit_None;
				Player *hottracked = HotTrackedPlayer( &tag );
				if(hottracked){
					IntVector3 col = world->GetTeam(hottracked->GetTeamId()).color;
					Vector4 color = Vector4::Make( col.x / 255.f, col.y / 255.f, col.z / 255.f, 1.f );
					Vector4 color2 = Vector4::Make( 1, 1, 1, 1);
					
					Player::HitBoxes hb = hottracked->GetHitBoxes();
					AddDebugObjectToScene(hb.head, (tag & hit_Head) ? color2 : color );
					AddDebugObjectToScene(hb.torso, (tag & hit_Torso) ? color2 : color );
					AddDebugObjectToScene(hb.limbs[0], (tag & hit_Legs) ? color2 : color );
					AddDebugObjectToScene(hb.limbs[1], (tag & hit_Legs) ? color2 : color );
					AddDebugObjectToScene(hb.limbs[2], (tag & hit_Arms) ? color2 : color );
				}
			}
			
			renderer->EndScene();
		}
		
		Vector3 Client::Project(spades::Vector3 v){
			v -= lastSceneDef.viewOrigin;
			
			// transform to NDC
			Vector3 v2;
			v2.x = Vector3::Dot(v, lastSceneDef.viewAxis[0]);
			v2.y = Vector3::Dot(v, lastSceneDef.viewAxis[1]);
			v2.z = Vector3::Dot(v, lastSceneDef.viewAxis[2]);
			
			float tanX = tanf(lastSceneDef.fovX * .5f);
			float tanY = tanf(lastSceneDef.fovY * .5f);
			
			v2.x /= tanX * v2.z;
			v2.y /= tanY * v2.z;
			
			// transform to IRenderer 2D coord
			v2.x = (v2.x + 1.f) / 2.f * renderer->ScreenWidth();
			v2.y = (-v2.y + 1.f) / 2.f * renderer->ScreenHeight();
			
			return v2;
		}
		
		void Client::DrawSplash() {
			Handle<IImage> img;
			Vector2 siz;
			Vector2 scrSize = {renderer->ScreenWidth(),
				renderer->ScreenHeight()};
			
			
			renderer->SetColor(MakeVector4(0, 0, 0, 1));
			img = renderer->RegisterImage("Gfx/White.tga");
			renderer->DrawImage(img, AABB2(0, 0, siz.x, siz.y));
			
			renderer->SetColor(MakeVector4(1, 1, 1, 1.));
			img = renderer->RegisterImage("Gfx/Title/Logo.png");
			
			siz = MakeVector2(img->GetWidth(), img->GetHeight());
			siz *= std::min(1.f, scrSize.x / siz.x * 0.5f);
			siz *= std::min(1.f, scrSize.y / siz.y);
			
			renderer->DrawImage(img, AABB2((scrSize.x - siz.x) * .5f,
										   (scrSize.y - siz.y) * .5f,
										   siz.x, siz.y));
			
			
		}
		
		void Client::DrawStartupScreen() {
			Handle<IImage> img;
			Vector2 scrSize = {renderer->ScreenWidth(),
				renderer->ScreenHeight()};
			
			renderer->SetColor(MakeVector4(0, 0, 0, 1.));
			img = renderer->RegisterImage("Gfx/White.tga");
			renderer->DrawImage(img, AABB2(0, 0,
										   scrSize.x, scrSize.y));
			
			DrawSplash();
			
			IFont *font = textFont;
			std::string str = "NOW LOADING";
			Vector2 size = font->Measure(str);
			Vector2 pos = MakeVector2(scrSize.x - 16.f, scrSize.y - 16.f);
			pos -= size;
			font->DrawShadow(str, pos, 1.f, MakeVector4(1,1,1,1), MakeVector4(0,0,0,0.5));
			
			renderer->FrameDone();
			renderer->Flip();
		}
		
		void Client::DrawHurtSprites() {
			float per = (world->GetTime() - lastHurtTime) / 1.5f;
			if(per > 1.f) return;
            if(per < 0.f) return;
			Handle<IImage> img = renderer->RegisterImage("Gfx/HurtSprite.png");
			
			Vector2 scrSize = {renderer->ScreenWidth(), renderer->ScreenHeight()};
			Vector2 scrCenter = scrSize * .5f;
			float radius = scrSize.GetLength() * .5f;
			
			for(size_t i = 0 ; i < hurtSprites.size(); i++) {
				HurtSprite& spr = hurtSprites[i];
				float alpha = spr.strength - per;
				if(alpha < 0.f) continue;
				if(alpha > 1.f) alpha = 1.f;
				
				Vector2 radDir = {
					cosf(spr.angle), sinf(spr.angle)
				};
				Vector2 angDir = {
					-sinf(spr.angle), cosf(spr.angle)
				};
				float siz = spr.scale * radius;
				Vector2 base = radDir * radius + scrCenter;
				Vector2 centVect = radDir * (-siz);
				Vector2 sideVect1 = angDir * (siz * 4.f * (spr.horzShift));
				Vector2 sideVect2 = angDir * (siz * 4.f * (spr.horzShift - 1.f));
				
				Vector2 v1 = base + centVect + sideVect1;
				Vector2 v2 = base + centVect + sideVect2;
				Vector2 v3 = base + sideVect1;
				
				renderer->SetColor(MakeVector4(0.f, 0.f, 0.f, alpha));
				renderer->DrawImage(img,
									v1, v2, v3,
									AABB2(0, 8.f, img->GetWidth(), img->GetHeight()));
			}
		}
		
		void Client::Draw2D(){
			SPADES_MARK_FUNCTION();
			
			float scrWidth = renderer->ScreenWidth();
			float scrHeight = renderer->ScreenHeight();
			IFont *font;
			
			if(GetWorld()){
				float wTime = world->GetTime();
				
				{
					std::list<ILocalEntity *>::iterator it;
					for(it = localEntities.begin(); it != localEntities.end(); it++){
						(*it)->Render2D();
					}
				}
				
				Player *p = GetWorld()->GetLocalPlayer();
				if(p){
					
					DrawHurtSprites();
					
					if(wTime < lastHurtTime + .35f &&
                       wTime >= lastHurtTime){
						float per = (wTime - lastHurtTime) / .35f;
						per = 1.f - per;
						per *= .3f + (1.f - p->GetHealth() / 100.f) * .7f;
						per = std::min(per, 0.9f);
						per = 1.f - per;
						Vector3 color = {1.f, per, per};
						renderer->MultiplyScreenColor(color);
						renderer->SetColor(MakeVector4(1,0,0,(1.f - per) * .1f));
                        renderer->DrawImage(renderer->RegisterImage("Gfx/White.tga"),
                                            AABB2(0, 0, scrWidth, scrHeight));
					}
					
					hitTag_t tag = hit_None;
					Player *hottracked = HotTrackedPlayer( &tag );
					if(hottracked){
						Vector3 posxyz = Project(hottracked->GetEye());
						Vector2 pos = {posxyz.x, posxyz.y};
						float dist = (hottracked->GetEye() - p->GetEye()).GetLength();
						int idist = (int)floorf(dist + .5f);
						char buf[64];
						sprintf(buf, "%s [%d%s]", hottracked->GetName().c_str(), idist, (idist == 1) ? "block":"blocks");
						
						font = textFont;
						Vector2 size = font->Measure(buf);
						pos.x -= size.x * .5f;
						pos.y -= size.y;
						font->DrawShadow(buf, pos, 1.f, MakeVector4(1,1,1,1), MakeVector4(0,0,0,0.5));
					}
					
					tcView->Draw();
					
					if(p->IsAlive() && p->GetTeamId() < 2){
						// draw damage ring
						hurtRingView->Draw();
						
						// draw local weapon's 2d things
						clientPlayers[p->GetId()]->Draw2D();
						
						if(cg_debugAim && p->GetTool() == Player::ToolWeapon) {
							Weapon *w = p->GetWeapon();
							float spread = w->GetSpread();
							
							AABB2 boundary(0,0,0,0);
							for(int i = 0; i < 8; i++){
								Vector3 vec = p->GetFront();
								if(i & 1) vec.x += spread;
								else vec.x -= spread;
								if(i & 2) vec.y += spread;
								else vec.y -= spread;
								if(i & 4) vec.z += spread;
								else vec.z -= spread;
								
								Vector3 viewPos;
								viewPos.x = Vector3::Dot(vec, p->GetRight());
								viewPos.y = Vector3::Dot(vec, p->GetUp());
								viewPos.z = Vector3::Dot(vec, p->GetFront());
								
								Vector2 p;
								p.x = viewPos.x / viewPos.z;
								p.y = viewPos.y / viewPos.z;
								boundary.min.x = std::min(boundary.min.x, p.x);
								boundary.min.y = std::min(boundary.min.y, p.y);
								boundary.max.x = std::max(boundary.max.x, p.x);
								boundary.max.y = std::max(boundary.max.y, p.y);
							}
							
							
							Handle<IImage> img = renderer->RegisterImage("Gfx/White.tga");
							boundary.min *= renderer->ScreenHeight() * .5f;
							boundary.max *= renderer->ScreenHeight() * .5f;
							boundary.min /= tanf(lastSceneDef.fovY * .5f);
							boundary.max /= tanf(lastSceneDef.fovY * .5f);
							IntVector3 cent;
							cent.x = (int)(renderer->ScreenWidth() * .5f);
							cent.y = (int)(renderer->ScreenHeight() * .5f);
							
							
							IntVector3 p1 = cent;
							IntVector3 p2 = cent;
							
							p1.x += (int)floorf(boundary.min.x);
							p1.y += (int)floorf(boundary.min.y);
							p2.x += (int)ceilf(boundary.max.x);
							p2.y += (int)ceilf(boundary.max.y);
							
							renderer->SetColor(MakeVector4(0,0,0,1));
							renderer->DrawImage(img, AABB2(p1.x - 2, p1.y - 2,
														   p2.x - p1.x + 4, 1));
							renderer->DrawImage(img, AABB2(p1.x - 2, p1.y - 2,
														   1, p2.y - p1.y + 4));
							renderer->DrawImage(img, AABB2(p1.x - 2, p2.y + 1,
														   p2.x - p1.x + 4, 1));
							renderer->DrawImage(img, AABB2(p2.x + 1, p1.y - 2,
														   1, p2.y - p1.y + 4));
							
							renderer->SetColor(MakeVector4(1,1,1,1));
							renderer->DrawImage(img, AABB2(p1.x - 1, p1.y - 1,
														   p2.x - p1.x + 2, 1));
							renderer->DrawImage(img, AABB2(p1.x - 1, p1.y - 1,
														   1, p2.y - p1.y + 2));
							renderer->DrawImage(img, AABB2(p1.x - 1, p2.y,
														   p2.x - p1.x + 2, 1));
							renderer->DrawImage(img, AABB2(p2.x, p1.y - 1,
														   1, p2.y - p1.y + 2));
						}
						
						// draw ammo
						Weapon *weap = p->GetWeapon();
						Handle<IImage> ammoIcon;
						float iconWidth, iconHeight;
						float spacing = 2.f;
						int stockNum;
						int warnLevel;
						
						if(p->IsToolWeapon()){
							switch(weap->GetWeaponType()){
								case RIFLE_WEAPON:
									ammoIcon = renderer->RegisterImage("Gfx/Bullet/7.62mm.tga");
									iconWidth = 6.f;
									iconHeight = iconWidth * 4.f;
									break;
								case SMG_WEAPON:
									ammoIcon = renderer->RegisterImage("Gfx/Bullet/9mm.tga");
									iconWidth = 4.f;
									iconHeight = iconWidth * 4.f;
									break;
								case SHOTGUN_WEAPON:
									ammoIcon = renderer->RegisterImage("Gfx/Bullet/12gauge.tga");
									iconWidth = 30.f;
									iconHeight = iconWidth / 4.f;
									spacing = -6.f;
									break;
								default:
									SPInvalidEnum("weap->GetWeaponType()", weap->GetWeaponType());
							}
							
							int clipSize = weap->GetClipSize();
							int clip = weap->GetAmmo();
							
							clipSize = std::max(clipSize, clip);
							
							for(int i = 0; i < clipSize; i++){
								float x = scrWidth - 16.f - (float)(i+1) *
								(iconWidth + spacing);
								float y = scrHeight - 16.f - iconHeight;
								
								if(clip >= i + 1){
									renderer->SetColor(MakeVector4(1,1,1,1));
								}else{
									renderer->SetColor(MakeVector4(0.4,0.4,0.4,1));
								}
								
								renderer->DrawImage(ammoIcon,
													AABB2(x,y,iconWidth,iconHeight));
							}
							
							stockNum = weap->GetStock();
							warnLevel = weap->GetMaxStock() / 3;
						}else{
							iconHeight = 0.f;
							warnLevel = 0;
							
							switch(p->GetTool()){
								case Player::ToolSpade:
								case Player::ToolBlock:
									stockNum = p->GetNumBlocks();
									break;
								case Player::ToolGrenade:
									stockNum = p->GetNumGrenades();
									break;
								default:
									SPInvalidEnum("p->GetTool()", p->GetTool());
							}
							
						}
						
						Vector4 numberColor = {1, 1, 1, 1};
						
						if(stockNum == 0){
							numberColor.y = 0.3f;
							numberColor.z = 0.3f;
						}else if(stockNum <= warnLevel){
							numberColor.z = 0.3f;
						}
						
						char buf[64];
						sprintf(buf, "%d", stockNum);
						font = designFont;
						std::string stockStr = buf;
						Vector2 size = font->Measure(stockStr);
						Vector2 pos = MakeVector2(scrWidth - 16.f, scrHeight - 16.f - iconHeight);
						pos -= size;
						font->DrawShadow(stockStr, pos, 1.f, numberColor, MakeVector4(0,0,0,0.5));
						
						
						// draw "press ... to reload"
						{
							std::string msg = "";
							
							switch(p->GetTool()){
								case Player::ToolBlock:
									if(p->GetNumBlocks() == 0){
										msg = "Out of Block";
									}
									break;
								case Player::ToolGrenade:
									if(p->GetNumGrenades() == 0){
										msg = "Out of Grenade";
									}
									break;
								case Player::ToolWeapon:
								{
									Weapon *weap = p->GetWeapon();
									if(weap->IsReloading() ||
									   p->IsAwaitingReloadCompletion()){
										msg = "Reloading";
									}else if(weap->GetAmmo() == 0 &&
											 weap->GetStock() == 0){
										msg = "Out of Ammo";
									}else if(weap->GetStock() > 0 &&
											 weap->GetAmmo() < weap->GetClipSize() / 4){
										msg = "Press [" + (std::string)cg_keyReloadWeapon + "] to Reload";
									}
								}
									break;
								default:;
									// no message
							}
							
							if(!msg.empty()){
								font = textFont;
								Vector2 size = font->Measure(msg);
								Vector2 pos = MakeVector2((scrWidth - size.x) * .5f,
														  scrHeight * 2.f / 3.f);
								font->DrawShadow(msg, pos, 1.f, MakeVector4(1,1,1,1), MakeVector4(0,0,0,0.5));
							}
						}
						
						if(p->GetTool() == Player::ToolBlock) {
							paletteView->Draw();
						}
						
						// draw map
						mapView->Draw();
						
						// --- end of "player is alive" drawing
					}else {
						
						// draw respawn tme
						if(!p->IsAlive()){
							std::string msg;
							
							float secs = p->GetRespawnTime() - world->GetTime();
							char buf[64];
							if(secs > 0.f)
								sprintf(buf, "You will respawn in: %d", (int)ceilf(secs));
							else
								strcpy(buf, "Waiting for respawn");
							
							msg = buf;
							
							if(!msg.empty()){
								font = textFont;
								Vector2 size = font->Measure(msg);
								Vector2 pos = MakeVector2((scrWidth - size.x) * .5f, scrHeight / 3.f);

								font->DrawShadow(msg, pos, 1.f, MakeVector4(1,1,1,1), MakeVector4(0,0,0,0.5));
							}
						}
						
						// draw map
						mapView->Draw();
						
					}
					
					// draw health
					if(p->GetTeamId() < 2){
						char buf[64];
						sprintf(buf, "%d", p->GetHealth());
						
						Vector4 numberColor = {1, 1, 1, 1};
						
						if(p->GetHealth() == 0){
							numberColor.y = 0.3f;
							numberColor.z = 0.3f;
						}else if(p->GetHealth() <= 50){
							numberColor.z = 0.3f;
						}
						
						font = designFont;
						std::string stockStr = buf;
						Vector2 size = font->Measure(stockStr);
						Vector2 pos = MakeVector2(16.f, scrHeight - 16.f);
						pos.y -= size.y;
						font->DrawShadow(stockStr, pos, 1.f, numberColor, MakeVector4(0,0,0,0.5));
					}
					
					if(IsFollowing()){
						if(followingPlayerId == p->GetId()){
							// just spectating
						}else{
							font = textFont;
							std::string msg = "Following " + world->GetPlayerPersistent(followingPlayerId).name;
							Vector2 size = font->Measure(msg);
							Vector2 pos = MakeVector2(scrWidth - 8.f, 256.f + 32.f);
							pos.x -= size.x;
							font->DrawShadow(msg, pos, 1.f, MakeVector4(1, 1, 1, 1), MakeVector4(0,0,0,0.5));
						}
					}
					
					chatWindow->Draw();
					killfeedWindow->Draw();
					
					if(p->IsAlive()) {
						
						// large map view should come in front
						largeMapView->Draw();
						
					}
					
					if(scoreboardVisible)
						scoreboard->Draw();
					
					
					// --- end "player is there" render
				}else{
					// world exists, but no local player: not joined
					
					scoreboard->Draw();
				}
				
				if(IsLimboViewActive())
					limbo->Draw();
				
				
			}else{
				// no world; loading?
				DrawSplash();
				
				Handle<IImage> img;
				
				std::string msg = net->GetStatusString();
				font = textFont;
				Vector2 textSize = font->Measure(msg);
				font->Draw(msg, MakeVector2(scrWidth - 16.f, scrHeight - 24.f) - textSize, 1.f, MakeVector4(1,1,1,0.95f));
				
				img = renderer->RegisterImage("Gfx/White.tga");
				float pos = timeSinceInit / 3.6f;
				pos -= floorf(pos);
				pos = 1.f - pos * 2.0f;
				for(float v = 0; v < 0.6f; v += 0.14f) {
					float p = pos + v;
					if(p < 0.01f || p > .99f) continue;
					p = asin(p * 2.f - 1.f);
					p = p / (float)M_PI + 0.5f;
					
					float op = p * (1.f - p) * 4.f;
					renderer->SetColor(MakeVector4(1.f, 1.f, 1.f, op));
					renderer->DrawImage(img, AABB2(scrWidth - 236.f + p * 234.f, scrHeight - 18.f, 4.f, 4.f));
				}
				
			}
			
			centerMessageView->Draw();
		}
		
#pragma mark - Chat Messages
		
		void Client::PlayerSentChatMessage(spades::client::Player *p,
										   bool global,
										   const std::string &msg){
			std::string s;
			if(global)
				s = "[Global] ";
			s += ChatWindow::TeamColorMessage(p->GetName(), p->GetTeamId());
			s += ": ";
			s += msg;
			chatWindow->AddMessage(s);
			if(global)
				NetLog("[Global] %s (%s): %s",
					   p->GetName().c_str(),
					   world->GetTeam(p->GetTeamId()).name.c_str(),
					   msg.c_str());
			else
				NetLog("[Team] %s (%s): %s",
					   p->GetName().c_str(),
					   world->GetTeam(p->GetTeamId()).name.c_str(),
					   msg.c_str());
			
			if(!IsMuted()) {
				Handle<IAudioChunk> chunk = audioDevice->RegisterSound("Sounds/Feedback/Chat.wav");
				audioDevice->PlayLocal(chunk, AudioParam());
			}
		}
		
		void Client::ServerSentMessage(const std::string &msg) {
			chatWindow->AddMessage(msg);
			NetLog("%s", msg.c_str());
			if(msg.find(playerName) != std::string::npos){
				printf("Mention: %s\n", msg.c_str());
			}
		}
		
#pragma mark - Follow / Spectate
		
		void Client::FollowNextPlayer() {
			int myTeam = 2;
			if(world->GetLocalPlayer()){
				myTeam = world->GetLocalPlayer()->GetTeamId();
			}
			
			int nextId = followingPlayerId;
			do{
				nextId++;
				if(nextId >= world->GetNumPlayerSlots())
					nextId = 0;
				
				Player *p = world->GetPlayer(nextId);
				if(p == NULL)
					continue;
				if(myTeam < 2 && p->GetTeamId() != myTeam)
					continue;
				if(p->GetFront().GetPoweredLength() < .01f)
					continue;
				
				break;
			}while(nextId != followingPlayerId);
			
			followingPlayerId = nextId;
		}
		
		bool Client::IsFollowing() {
			if(!world) return false;
			if(!world->GetLocalPlayer())
				return false;
			Player *p = world->GetLocalPlayer();
			if(p->GetTeamId() >= 2)
				return true;
			if(p->IsAlive())
				return false;
			else return true;
		}
		
#pragma mark - Effects
		
		Player *Client::HotTrackedPlayer( hitTag_t* hitFlag ){
			if(!world)
				return NULL;
			Player *p = world->GetLocalPlayer();
			if(!p || !p->IsAlive())
				return NULL;
			if(ShouldRenderInThirdPersonView())
				return NULL;
			Vector3 origin = p->GetEye();
			Vector3 dir = p->GetFront();
			World::WeaponRayCastResult result = world->WeaponRayCast(origin, dir, p);
			
			if(result.hit == false || result.player == NULL)
				return NULL;
			
			// don't hot track enemies (non-spectator only)
			if(result.player->GetTeamId() != p->GetTeamId() &&
			   p->GetTeamId() < 2)
				return NULL;
			if( hitFlag ) {
				*hitFlag = result.hitFlag;
			}
			return result.player;
		}
		
		bool Client::IsMuted() {
			// prevent to play loud sound at connection
			return time < worldSetTime + .05f;
		}
		
		void Client::Bleed(spades::Vector3 v){
			SPADES_MARK_FUNCTION();
			
			if(!cg_blood)
				return;
			
			// distance cull
			if((v - lastSceneDef.viewOrigin).GetPoweredLength() >
			   150.f * 150.f)
				return;
			
			//Handle<IImage> img = renderer->RegisterImage("Textures/SoftBall.tga");
			Handle<IImage> img = renderer->RegisterImage("Gfx/White.tga");
			Vector4 color = {0.5f, 0.02f, 0.04f, 1.f};
			for(int i = 0; i < 10; i++){
				ParticleSpriteEntity *ent =
				new ParticleSpriteEntity(this, img, color);
				ent->SetTrajectory(v,
								   MakeVector3(GetRandom()-GetRandom(),
											   GetRandom()-GetRandom(),
											   GetRandom()-GetRandom()) * 10.f,
								   1.f, 0.7f);
				ent->SetRotation(GetRandom() * (float)M_PI * 2.f);
				ent->SetRadius(0.1f + GetRandom()*GetRandom()*0.2f);
				ent->SetLifeTime(3.f, 0.f, 1.f);
				localEntities.push_back(ent);
			}
			
			color = MakeVector4(.7f, .35f, .37f, .6f);
			for(int i = 0; i < 2; i++){
				ParticleSpriteEntity *ent =
				new SmokeSpriteEntity(this, color, 100.f);
				ent->SetTrajectory(v,
								   MakeVector3(GetRandom()-GetRandom(),
											   GetRandom()-GetRandom(),
											   GetRandom()-GetRandom()) * .7f,
								   .8f, 0.f);
				ent->SetRotation(GetRandom() * (float)M_PI * 2.f);
				ent->SetRadius(.5f + GetRandom()*GetRandom()*0.2f,
							   2.f);
				ent->SetBlockHitAction(ParticleSpriteEntity::Ignore);
				ent->SetLifeTime(.20f + GetRandom() * .2f, 0.06f, .20f);
				localEntities.push_back(ent);
			}
		}
		
		void Client::EmitBlockFragments(Vector3 origin,
										IntVector3 c){
			SPADES_MARK_FUNCTION();
			
			// distance cull
			float distPowered = (origin - lastSceneDef.viewOrigin).GetPoweredLength();
			if(distPowered >
			   150.f * 150.f)
				return;
			
			Handle<IImage> img = renderer->RegisterImage("Gfx/White.tga");
			Vector4 color = {c.x / 255.f,
			c.y / 255.f, c.z / 255.f, 1.f};
			for(int i = 0; i < 7; i++){
				ParticleSpriteEntity *ent =
				new ParticleSpriteEntity(this, img, color);
				ent->SetTrajectory(origin,
								   MakeVector3(GetRandom()-GetRandom(),
											   GetRandom()-GetRandom(),
											   GetRandom()-GetRandom()) * 7.f,
								   1.f, .9f);
				ent->SetRotation(GetRandom() * (float)M_PI * 2.f);
				ent->SetRadius(0.2f + GetRandom()*GetRandom()*0.1f);
				ent->SetLifeTime(2.f, 0.f, 1.f);
				if(distPowered < 16.f * 16.f)
					ent->SetBlockHitAction(ParticleSpriteEntity::BounceWeak);
				localEntities.push_back(ent);
			}
			
			if(distPowered <
			   32.f * 32.f){
				for(int i = 0; i < 16; i++){
					ParticleSpriteEntity *ent =
					new ParticleSpriteEntity(this, img, color);
					ent->SetTrajectory(origin,
									   MakeVector3(GetRandom()-GetRandom(),
												   GetRandom()-GetRandom(),
												   GetRandom()-GetRandom()) * 12.f,
									   1.f, .9f);
					ent->SetRotation(GetRandom() * (float)M_PI * 2.f);
					ent->SetRadius(0.1f + GetRandom()*GetRandom()*0.14f);
					ent->SetLifeTime(2.f, 0.f, 1.f);
					if(distPowered < 16.f * 16.f)
						ent->SetBlockHitAction(ParticleSpriteEntity::BounceWeak);
					localEntities.push_back(ent);
				}
			}
			
			color += (MakeVector4(1, 1, 1, 1) - color) * .2f;
			color.w *= .2f;
			for(int i = 0; i < 2; i++){
				ParticleSpriteEntity *ent =
				new SmokeSpriteEntity(this, color, 100.f);
				ent->SetTrajectory(origin,
								   MakeVector3(GetRandom()-GetRandom(),
											   GetRandom()-GetRandom(),
											   GetRandom()-GetRandom()) * .7f,
								   1.f, 0.f);
				ent->SetRotation(GetRandom() * (float)M_PI * 2.f);
				ent->SetRadius(.6f + GetRandom()*GetRandom()*0.2f,
							   0.8f);
				ent->SetLifeTime(.3f + GetRandom() * .3f, 0.06f, .4f);
				ent->SetBlockHitAction(ParticleSpriteEntity::Ignore);
				localEntities.push_back(ent);
			}
			
		}
		
		void Client::EmitBlockDestroyFragments(IntVector3 blk,
										IntVector3 c){
			SPADES_MARK_FUNCTION();
			
			Vector3 origin = {blk.x + .5f, blk.y + .5f, blk.z + .5f};
			
			// distance cull
			if((origin - lastSceneDef.viewOrigin).GetPoweredLength() >
			   150.f * 150.f)
				return;
			
			Handle<IImage> img = renderer->RegisterImage("Gfx/White.tga");
			Vector4 color = {c.x / 255.f,
				c.y / 255.f, c.z / 255.f, 1.f};
			for(int i = 0; i < 8; i++){
				ParticleSpriteEntity *ent =
				new ParticleSpriteEntity(this, img, color);
				ent->SetTrajectory(origin,
								   MakeVector3(GetRandom()-GetRandom(),
											   GetRandom()-GetRandom(),
											   GetRandom()-GetRandom()) * 7.f,
								   1.f, 1.f);
				ent->SetRotation(GetRandom() * (float)M_PI * 2.f);
				ent->SetRadius(0.3f + GetRandom()*GetRandom()*0.2f);
				ent->SetLifeTime(2.f, 0.f, 1.f);
				ent->SetBlockHitAction(ParticleSpriteEntity::BounceWeak);
				localEntities.push_back(ent);
			}
		}
		
		void Client::MuzzleFire(spades::Vector3 origin,
								spades::Vector3 dir,
								bool local) {
			DynamicLightParam l;
			l.origin = origin;
			l.radius = 5.f;
			l.type = DynamicLightTypePoint;
			l.color = MakeVector3(3.f, 1.6f, 0.5f);
			flashDlights.push_back(l);
			
			Vector4 color;
			Vector3 velBias = {0, 0, -0.5f};
			color = MakeVector4( .8f, .8f, .8f, .3f);
			
			// rapid smoke
			for(int i = 0; i < 2; i++){
				ParticleSpriteEntity *ent =
				new SmokeSpriteEntity(this, color, 120.f);
				ent->SetTrajectory(origin,
								   (MakeVector3(GetRandom()-GetRandom(),
												GetRandom()-GetRandom(),
												GetRandom()-GetRandom())+velBias*.5f) * 0.3f,
								   1.f, 0.f);
				ent->SetRotation(GetRandom() * (float)M_PI * 2.f);
				ent->SetRadius(.2f,
							   7.f, 0.0000005f);
				ent->SetBlockHitAction(ParticleSpriteEntity::Ignore);
				ent->SetLifeTime(0.2f + GetRandom()*0.1f, 0.f, .30f);
				localEntities.push_back(ent);
			}
		}
		
		void Client::GrenadeExplosion(spades::Vector3 origin){
			float dist = (origin - lastSceneDef.viewOrigin).GetLength();
			if(dist > 170.f)
				return;
			grenadeVibration += 2.f / (dist + 5.f);
			if(grenadeVibration > 1.f)
				grenadeVibration = 1.f;
			
			DynamicLightParam l;
			l.origin = origin;
			l.radius = 16.f;
			l.type = DynamicLightTypePoint;
			l.color = MakeVector3(3.f, 1.6f, 0.5f);
			l.useLensFlare = true;
			flashDlights.push_back(l);
			
			Vector3 velBias = {0,0,0};
			if(!map->ClipBox(origin.x, origin.y, origin.z)){
				if(map->ClipBox(origin.x + 1.f, origin.y, origin.z)){
					velBias.x -= 1.f;
				}
				if(map->ClipBox(origin.x - 1.f, origin.y, origin.z)){
					velBias.x += 1.f;
				}
				if(map->ClipBox(origin.x, origin.y + 1.f, origin.z)){
					velBias.y -= 1.f;
				}
				if(map->ClipBox(origin.x, origin.y - 1.f, origin.z)){
					velBias.y += 1.f;
				}
				if(map->ClipBox(origin.x, origin.y , origin.z + 1.f)){
					velBias.z -= 1.f;
				}
				if(map->ClipBox(origin.x, origin.y , origin.z - 1.f)){
					velBias.z += 1.f;
				}
			}
			
			Vector4 color;
			color = MakeVector4( .8f, .8f, .8f, .6f);
			// rapid smoke
			for(int i = 0; i < 4; i++){
				ParticleSpriteEntity *ent =
				new SmokeSpriteEntity(this, color, 60.f);
				ent->SetTrajectory(origin,
								   (MakeVector3(GetRandom()-GetRandom(),
											   GetRandom()-GetRandom(),
											   GetRandom()-GetRandom())+velBias*.5f) * 4.f,
								   1.f, 0.f);
				ent->SetRotation(GetRandom() * (float)M_PI * 2.f);
				ent->SetRadius(1.f + GetRandom()*GetRandom()*0.4f,
							   10.f);
				ent->SetBlockHitAction(ParticleSpriteEntity::Ignore);
				ent->SetLifeTime(.1f + GetRandom()*0.02f, 0.f, .10f);
				localEntities.push_back(ent);
			}
			
			// slow smoke
			color.w = .25f;
			for(int i = 0; i < 8; i++){
				ParticleSpriteEntity *ent =
				new SmokeSpriteEntity(this, color, 20.f);
				ent->SetTrajectory(origin,
								   (MakeVector3(GetRandom()-GetRandom(),
											   GetRandom()-GetRandom(),
											   (GetRandom()-GetRandom()) * .2f)) * 2.f,
								   1.f, 0.f);
				ent->SetRotation(GetRandom() * (float)M_PI * 2.f);
				ent->SetRadius(1.4f + GetRandom()*GetRandom()*0.8f,
							   0.2f);
				ent->SetBlockHitAction(ParticleSpriteEntity::Ignore);
				ent->SetLifeTime(6.f + GetRandom() * 5.f, 0.1f, 8.f);
				localEntities.push_back(ent);
			}
			
			// fragments
			Handle<IImage> img = renderer->RegisterImage("Gfx/White.tga");
			color = MakeVector4(0.01, 0.03, 0, 1.f);
			for(int i = 0; i < 42; i++){
				ParticleSpriteEntity *ent =
				new ParticleSpriteEntity(this, img, color);
				Vector3 dir = MakeVector3(GetRandom()-GetRandom(),
										  GetRandom()-GetRandom(),
										  GetRandom()-GetRandom());
				dir += velBias * .5f;
				float radius = 0.1f + GetRandom()*GetRandom()*0.2f;
				ent->SetTrajectory(origin + dir * .2f,
								   dir * 20.f,
								   .1f + radius * 3.f, 1.f);
				ent->SetRotation(GetRandom() * (float)M_PI * 2.f);
				ent->SetRadius(radius);
				ent->SetLifeTime(3.5f + GetRandom() * 2.f, 0.f, 1.f);
				ent->SetBlockHitAction(ParticleSpriteEntity::BounceWeak);
				localEntities.push_back(ent);
			}
			
			// fire smoke
			color= MakeVector4(1.f, .6f, .2f, 1.f);
			for(int i = 0; i < 4; i++){
				ParticleSpriteEntity *ent =
				new SmokeSpriteEntity(this, color, 60.f);
				ent->SetTrajectory(origin,
								   (MakeVector3(GetRandom()-GetRandom(),
											   GetRandom()-GetRandom(),
											   GetRandom()-GetRandom())+velBias) * 12.f,
								   1.f, 0.f);
				ent->SetRotation(GetRandom() * (float)M_PI * 2.f);
				ent->SetRadius(1.f + GetRandom()*GetRandom()*0.4f,
							   6.f);
				ent->SetBlockHitAction(ParticleSpriteEntity::Ignore);
				ent->SetLifeTime(.08f + GetRandom()*0.03f, 0.f, .10f);
				ent->SetAdditive(true);
				localEntities.push_back(ent);
			}
		}
		
		void Client::GrenadeExplosionUnderwater(spades::Vector3 origin){
			float dist = (origin - lastSceneDef.viewOrigin).GetLength();
			if(dist > 170.f)
				return;
			grenadeVibration += 1.5f / (dist + 5.f);
			if(grenadeVibration > 1.f)
				grenadeVibration = 1.f;
			
			Vector3 velBias = {0,0,0};
			
			Vector4 color;
			color = MakeVector4( .95f, .95f, .95f, .6f);
			// water1
			Handle<IImage> img = renderer->RegisterImage("Textures/WaterExpl.png");
			for(int i = 0; i < 7; i++){
				ParticleSpriteEntity *ent =
				new ParticleSpriteEntity(this, img, color);
				ent->SetTrajectory(origin,
								   (MakeVector3(GetRandom()-GetRandom(),
												GetRandom()-GetRandom(),
												-GetRandom()*7.f)) * 2.5f,
								   .3f, .6f);
				ent->SetRotation(0.f);
				ent->SetRadius(1.5f + GetRandom()*GetRandom()*0.4f,
							   1.3f);
				ent->SetBlockHitAction(ParticleSpriteEntity::Ignore);
				ent->SetLifeTime(3.f + GetRandom()*0.3f, 0.f, .60f);
				localEntities.push_back(ent);
			}
			
			// water2
			img = renderer->RegisterImage("Textures/Fluid.png");
			color.w = .9f;
			for(int i = 0; i < 16; i++){
				ParticleSpriteEntity *ent =
				new ParticleSpriteEntity(this, img, color);
				ent->SetTrajectory(origin,
								   (MakeVector3(GetRandom()-GetRandom(),
												GetRandom()-GetRandom(),
												-GetRandom()*10.f)) * 3.5f,
								   1.f, 1.f);
				ent->SetRotation(GetRandom() * (float)M_PI * 2.f);
				ent->SetRadius(0.9f + GetRandom()*GetRandom()*0.4f,
							   0.7f);
				ent->SetBlockHitAction(ParticleSpriteEntity::Ignore);
				ent->SetLifeTime(3.f + GetRandom()*0.3f, .7f, .60f);
				localEntities.push_back(ent);
			}
			
			// slow smoke
			color.w = .4f;
			for(int i = 0; i < 8; i++){
				ParticleSpriteEntity *ent =
				new SmokeSpriteEntity(this, color, 20.f);
				ent->SetTrajectory(origin,
								   (MakeVector3(GetRandom()-GetRandom(),
												GetRandom()-GetRandom(),
												(GetRandom()-GetRandom()) * .2f)) * 2.f,
								   1.f, 0.f);
				ent->SetRotation(GetRandom() * (float)M_PI * 2.f);
				ent->SetRadius(1.4f + GetRandom()*GetRandom()*0.8f,
							   0.2f);
				ent->SetBlockHitAction(ParticleSpriteEntity::Ignore);
				ent->SetLifeTime(6.f + GetRandom() * 5.f, 0.1f, 8.f);
				localEntities.push_back(ent);
			}
			
			// fragments
			img = renderer->RegisterImage("Gfx/White.tga");
			color = MakeVector4(1,1,1, 0.7f);
			for(int i = 0; i < 42; i++){
				ParticleSpriteEntity *ent =
				new ParticleSpriteEntity(this, img, color);
				Vector3 dir = MakeVector3(GetRandom()-GetRandom(),
										  GetRandom()-GetRandom(),
										  -GetRandom() * 3.f);
				dir += velBias * .5f;
				float radius = 0.1f + GetRandom()*GetRandom()*0.2f;
				ent->SetTrajectory(origin + dir * .2f +
								   MakeVector3(0, 0, -1.2f),
								   dir * 13.f,
								   .1f + radius * 3.f, 1.f);
				ent->SetRotation(GetRandom() * (float)M_PI * 2.f);
				ent->SetRadius(radius);
				ent->SetLifeTime(3.5f + GetRandom() * 2.f, 0.f, 1.f);
				ent->SetBlockHitAction(ParticleSpriteEntity::Delete);
				localEntities.push_back(ent);
			}
			
			
			// TODO: wave?
		}

		
#pragma mark - Server Packet Handlers
		
		void Client::LocalPlayerCreated(){
			followPos = world->GetLocalPlayer()->GetEye();
			weapInput = WeaponInput();
			playerInput = PlayerInput();
			keypadInput = KeypadInput();
			
			toolRaiseState = .0f;
		}
		
		void Client::JoinedGame() {
			// note: local player doesn't exist yet now
			
			// tune for spectate mode
			followingPlayerId = world->GetLocalPlayerIndex();
			followPos = MakeVector3(256, 256, 30);
			followVel = MakeVector3(0, 0, 0);
		}
		
		void Client::PlayerCreatedBlock(spades::client::Player *p){
			SPADES_MARK_FUNCTION();
			
			if(!IsMuted()) {
				Handle<IAudioChunk> c = audioDevice->RegisterSound("Sounds/Weapons/Block/Build.wav");
				audioDevice->Play(c, p->GetEye() + p->GetFront(),
								  AudioParam());
			}
		}
		
		void Client::TeamCapturedTerritory(int teamId,
											 int terId) {
			TCGameMode::Territory *ter = static_cast<TCGameMode *>(world->GetMode())->GetTerritory(terId);
			int old = ter->ownerTeamId;
			std::string msg;
			msg = chatWindow->TeamColorMessage(world->GetTeam(teamId).name,
											   teamId);
			msg += " captured ";
			if(old < 2){
				msg += chatWindow->TeamColorMessage(world->GetTeam(old).name,
													old);
				msg += "'s territory";
			}else{
				msg += "an neutral territory";
			}
			chatWindow->AddMessage(msg);
			
			msg = world->GetTeam(teamId).name;
			msg += " captured ";
			if(old < 2){
				msg += world->GetTeam(old).name;
				msg += "'s Territory";
			}else{
				msg += "an Neutral Territory";
			}
			NetLog("%s", msg.c_str());
			centerMessageView->AddMessage(msg);
			
			if(world->GetLocalPlayer() && !IsMuted()){
				if(teamId == world->GetLocalPlayer()->GetTeamId()){
					Handle<IAudioChunk> chunk = audioDevice->RegisterSound("Sounds/Feedback/TC/YourTeamCaptured.wav");
					audioDevice->PlayLocal(chunk, AudioParam());
				}else{
					Handle<IAudioChunk> chunk = audioDevice->RegisterSound("Sounds/Feedback/TC/EnemyCaptured.wav");
					audioDevice->PlayLocal(chunk, AudioParam());
				}
			}
			
		}
		
		void Client::PlayerCapturedIntel(spades::client::Player *p){
			std::string msg;
			msg = chatWindow->TeamColorMessage(p->GetName(), p->GetTeamId());
			msg += " captured ";
			msg += chatWindow->TeamColorMessage(world->GetTeam(1 - p->GetTeamId()).name,
												1 - p->GetTeamId());
			msg += "'s intel";
			chatWindow->AddMessage(msg);
			
			msg = p->GetName();
			msg += " captured ";
			msg += world->GetTeam(1 - p->GetTeamId()).name;
			msg += "'s Intel.";
			NetLog("%s", msg.c_str());
			centerMessageView->AddMessage(msg);
			
			if(world->GetLocalPlayer() && !IsMuted()){
				if(p->GetTeamId() == world->GetLocalPlayer()->GetTeamId()){
					Handle<IAudioChunk> chunk = audioDevice->RegisterSound("Sounds/Feedback/CTF/YourTeamCaptured.wav");
					audioDevice->PlayLocal(chunk, AudioParam());
				}else{
					Handle<IAudioChunk> chunk = audioDevice->RegisterSound("Sounds/Feedback/CTF/EnemyCaptured.wav");
					audioDevice->PlayLocal(chunk, AudioParam());
				}
			}
		}
		
		void Client::PlayerPickedIntel(spades::client::Player *p) {
			std::string msg;
			msg = chatWindow->TeamColorMessage(p->GetName(), p->GetTeamId());
			msg += " picked up ";
			msg += chatWindow->TeamColorMessage(world->GetTeam(1 - p->GetTeamId()).name,
												1 - p->GetTeamId());
			msg += "'s intel";
			chatWindow->AddMessage(msg);
			
			msg = p->GetName();
			msg += " picked up ";
			msg += world->GetTeam(1 - p->GetTeamId()).name;
			msg += "'s intel.";
			NetLog("%s", msg.c_str());
			centerMessageView->AddMessage(msg);
			
			if(!IsMuted()) {
				Handle<IAudioChunk> chunk = audioDevice->RegisterSound("Sounds/Feedback/CTF/PickedUp.wav");
				audioDevice->PlayLocal(chunk, AudioParam());
			}
		}
		
		void Client::PlayerDropIntel(spades::client::Player *p) {
			std::string msg;
			msg = chatWindow->TeamColorMessage(p->GetName(), p->GetTeamId());
			msg += " dropped ";
			msg += chatWindow->TeamColorMessage(world->GetTeam(1 - p->GetTeamId()).name,
												1 - p->GetTeamId());
			msg += "'s intel";
			chatWindow->AddMessage(msg);
			
			msg = p->GetName();
			msg += " dropped ";
			msg += world->GetTeam(1 - p->GetTeamId()).name;
			msg += "'s intel.";
			NetLog("%s", msg.c_str());
			centerMessageView->AddMessage(msg);
		}
		
		void Client::PlayerDestroyedBlockWithWeaponOrTool(spades::IntVector3 blk){
			Vector3 origin = {blk.x + .5f, blk.y + .5f, blk.z + .5f};
			
			if(!map->IsSolid(blk.x, blk.y, blk.z))
				return;;
			
			Handle<IAudioChunk> c = audioDevice->RegisterSound("Sounds/Misc/BlockDestroy.wav");
			if(!IsMuted()){
				audioDevice->Play(c, origin,
									   AudioParam());
			}
				
			uint32_t col = map->GetColor(blk.x, blk.y, blk.z);
			IntVector3 colV = {(uint8_t)col,
				(uint8_t)(col >> 8), (uint8_t)(col >> 16)};
			
			EmitBlockDestroyFragments(blk, colV);
		}
		
		void Client::PlayerDiggedBlock(spades::IntVector3 blk){
			Vector3 origin = {blk.x + .5f, blk.y + .5f, blk.z + .5f};
			
			Handle<IAudioChunk> c = audioDevice->RegisterSound("Sounds/Misc/BlockDestroy.wav");
			if(!IsMuted()){
				audioDevice->Play(c, origin,
								  AudioParam());
			}
				
			for(int z = blk.z - 1 ; z <= blk.z + 1; z++){
				if(z < 0 || z > 61)
					continue;
				if(!map->IsSolid(blk.x, blk.y, z))
					continue;
				uint32_t col = map->GetColor(blk.x, blk.y, z);
				IntVector3 colV = {(uint8_t)col,
					(uint8_t)(col >> 8), (uint8_t)(col >> 16)};
				
				EmitBlockDestroyFragments(IntVector3::Make(blk.x, blk.y, z), colV);
			}
		}
		
		void Client::PlayerLeaving(spades::client::Player *p) {
			std::string msg;
			msg = "Player " + chatWindow->TeamColorMessage(p->GetName(), p->GetTeamId());
			msg += " has left";
			chatWindow->AddMessage(msg);
		}
		
		void Client::PlayerJoinedTeam(spades::client::Player *p) {
			std::string msg;
			msg = p->GetName();
			msg += " joined ";
			msg += chatWindow->TeamColorMessage(world->GetTeam(p->GetTeamId()).name,
												p->GetTeamId());
			msg += " team";
			chatWindow->AddMessage(msg);
		}
		
		void Client::GrenadeDestroyedBlock(spades::IntVector3 blk){
			for(int x = blk.x - 1; x <= blk.x + 1; x++)
				for(int y = blk.y - 1; y <= blk.y + 1; y++)
					for(int z = blk.z - 1 ; z <= blk.z + 1; z++){
						if(z < 0 || z > 61 || x < 0 || x >= 512 ||
						   y < 0 || y >= 512)
							continue;
						if(!map->IsSolid(x, y, z))
							continue;
						uint32_t col = map->GetColor(x, y, z);
						IntVector3 colV = {(uint8_t)col,
							(uint8_t)(col >> 8), (uint8_t)(col >> 16)};
						
						EmitBlockDestroyFragments(IntVector3::Make(x, y, z), colV);
					}
		}

		
		void Client::TeamWon(int teamId){
			std::string msg;
			msg = chatWindow->TeamColorMessage(world->GetTeam(teamId).name,
												teamId);
			msg += " wins!";
			chatWindow->AddMessage(msg);
			
			msg = world->GetTeam(teamId).name;
			msg += " wins!";
			NetLog("%s", msg.c_str());
			centerMessageView->AddMessage(msg);
			
			if(world->GetLocalPlayer()){
				if(teamId == world->GetLocalPlayer()->GetTeamId()){
					Handle<IAudioChunk> chunk = audioDevice->RegisterSound("Sounds/Feedback/Win.wav");
					audioDevice->PlayLocal(chunk, AudioParam());
				}else{
					Handle<IAudioChunk> chunk = audioDevice->RegisterSound("Sounds/Feedback/Lose.wav");
					audioDevice->PlayLocal(chunk, AudioParam());
				}
			}
		}
		
#pragma mark - IWorldListener Handlers
		
		void Client::PlayerObjectSet(int id) {
			if(clientPlayers[id]){
				clientPlayers[id]->Invalidate();
				clientPlayers[id]->Release();
				clientPlayers[id] = NULL;
			}
			
			Player *p = world->GetPlayer(id);
			if(p)
				clientPlayers[id] = new ClientPlayer(p, this);
		}
		
		void Client::PlayerJumped(spades::client::Player *p){
			SPADES_MARK_FUNCTION();
			
			if(!IsMuted()){
			
				Handle<IAudioChunk> c = p->GetWade() ?
				audioDevice->RegisterSound("Sounds/Player/WaterJump.wav"):
				audioDevice->RegisterSound("Sounds/Player/Jump.wav");
				audioDevice->Play(c, p->GetOrigin(),
								  AudioParam());
			}
		}
		
		void Client::PlayerLanded(spades::client::Player *p,
								  bool hurt) {
			SPADES_MARK_FUNCTION();
			
			if(!IsMuted()){
				Handle<IAudioChunk> c;
				if(hurt)
					c = audioDevice->RegisterSound("Sounds/Player/FallHurt.wav");
				else if(p->GetWade())
					c = audioDevice->RegisterSound("Sounds/Player/WaterLand.wav");
				else
					c = audioDevice->RegisterSound("Sounds/Player/Land.wav");
				audioDevice->Play(c, p->GetOrigin(),
								  AudioParam());
			}
		}
		
		void Client::PlayerMadeFootstep(spades::client::Player *p){
			SPADES_MARK_FUNCTION();
			
			if(!IsMuted()){
				const char *snds[] = {
					"Sounds/Player/Footstep1.wav",
					"Sounds/Player/Footstep2.wav",
					"Sounds/Player/Footstep3.wav",
					"Sounds/Player/Footstep4.wav"
				};
				const char *wsnds[] = {
					"Sounds/Player/Wade1.wav",
					"Sounds/Player/Wade2.wav",
					"Sounds/Player/Wade3.wav",
					"Sounds/Player/Wade4.wav"
				};
				Handle<IAudioChunk> c = p->GetWade() ?
				audioDevice->RegisterSound(wsnds[rand() % 4]):
				audioDevice->RegisterSound(snds[rand() % 4]);
				audioDevice->Play(c, p->GetOrigin(),
								  AudioParam());
			}
		}
		
		void Client::PlayerFiredWeapon(spades::client::Player *p) {
			SPADES_MARK_FUNCTION();
			
			if(p == world->GetLocalPlayer()){
				localFireVibrationTime = time;
			}
			
			clientPlayers[p->GetId()]->FiredWeapon();
		}
		void Client::PlayerDryFiredWeapon(spades::client::Player *p) {
			SPADES_MARK_FUNCTION();
			
			if(!IsMuted()){
				bool isLocal = p == world->GetLocalPlayer();
				Handle<IAudioChunk> c = audioDevice->RegisterSound("Sounds/Weapons/DryFire.wav");
				if(isLocal)
					audioDevice->PlayLocal(c, MakeVector3(.4f, -.3f, .5f),
									  AudioParam());
				else
					audioDevice->Play(c, p->GetEye() + p->GetFront() * 0.5f
										   - p->GetUp() * .3f
										   + p->GetRight() * .4f,
										   AudioParam());
			}
		}
		
		void Client::PlayerReloadingWeapon(spades::client::Player *p) {
			SPADES_MARK_FUNCTION();
			
			clientPlayers[p->GetId()]->ReloadingWeapon();
		}
		
		void Client::PlayerReloadedWeapon(spades::client::Player *p){
			SPADES_MARK_FUNCTION();
			
			
			clientPlayers[p->GetId()]->ReloadedWeapon();
		}
		
		void Client::PlayerChangedTool(spades::client::Player *p){
			SPADES_MARK_FUNCTION();
			
			if(!IsMuted()){
				bool isLocal = p == world->GetLocalPlayer();
				Handle<IAudioChunk> c;
				if(isLocal){
					switch(p->GetTool()) {
						case Player::ToolSpade:
							c = audioDevice->RegisterSound("Sounds/Weapons/Spade/RaiseLocal.wav");
							break;
						case Player::ToolBlock:
							c = audioDevice->RegisterSound("Sounds/Weapons/Block/RaiseLocal.wav");
							break;
						case Player::ToolWeapon:
							switch(p->GetWeapon()->GetWeaponType()){
								case RIFLE_WEAPON:
									c = audioDevice->RegisterSound("Sounds/Weapons/Rifle/RaiseLocal.wav");
									break;
								case SMG_WEAPON:
									c = audioDevice->RegisterSound("Sounds/Weapons/SMG/RaiseLocal.wav");
									break;
								case SHOTGUN_WEAPON:
									c = audioDevice->RegisterSound("Sounds/Weapons/Shotgun/RaiseLocal.wav");
									break;
							}
							
							break;
						case Player::ToolGrenade:
							c = audioDevice->RegisterSound("Sounds/Weapons/Grenade/RaiseLocal.wav");
							break;
					}
				}else{
					c = audioDevice->RegisterSound("Sounds/Weapons/Switch.wav");
				}
				if(isLocal)
					audioDevice->PlayLocal(c, MakeVector3(.4f, -.3f, .5f),
										   AudioParam());
				else
					audioDevice->Play(c, p->GetEye() + p->GetFront() * 0.5f
									  - p->GetUp() * .3f
									  + p->GetRight() * .4f,
									  AudioParam());
			}
		}
		
		void Client::PlayerRestocked(spades::client::Player *p){
			if(!IsMuted()){
				bool isLocal = p == world->GetLocalPlayer();
				Handle<IAudioChunk> c = isLocal ?
				audioDevice->RegisterSound("Sounds/Weapons/SwitchLocal.wav"):
				audioDevice->RegisterSound("Sounds/Weapons/Switch.wav");
				if(isLocal)
					audioDevice->PlayLocal(c, MakeVector3(.4f, -.3f, .5f),
										   AudioParam());
				else
					audioDevice->Play(c, p->GetEye() + p->GetFront() * 0.5f
									  - p->GetUp() * .3f
									  + p->GetRight() * .4f,
									  AudioParam());
			}
		}
		
		void Client::PlayerThrownGrenade(spades::client::Player *p, Grenade *g){
			SPADES_MARK_FUNCTION();
			
			if(!IsMuted()){
				bool isLocal = p == world->GetLocalPlayer();
				Handle<IAudioChunk> c = audioDevice->RegisterSound("Sounds/Weapons/Grenade/Throw.wav");
				
				if(g && isLocal){
					net->SendGrenade(g);
				}
				
				if(isLocal)
					audioDevice->PlayLocal(c, MakeVector3(.4f, 0.1f, .3f),
										   AudioParam());
				else
					audioDevice->Play(c, p->GetEye() + p->GetFront() * 0.5f
									  - p->GetUp() * .2f
									  + p->GetRight() * .3f,
									  AudioParam());
			}
		}
		
		void Client::PlayerMissedSpade(spades::client::Player *p){
			SPADES_MARK_FUNCTION();
			
			if(!IsMuted()){
				bool isLocal = p == world->GetLocalPlayer();
				Handle<IAudioChunk> c = audioDevice->RegisterSound("Sounds/Weapons/Spade/Miss.wav");
				if(isLocal)
					audioDevice->PlayLocal(c, MakeVector3(.2f, -.1f, 0.7f),
										   AudioParam());
				else
					audioDevice->Play(c, p->GetOrigin() + p->GetFront() * 0.8f
									  - p->GetUp() * .2f,
									  AudioParam());
			}
		}
		
		void Client::PlayerHitBlockWithSpade(spades::client::Player *p,
											 Vector3 hitPos,
											 IntVector3 blockPos,
											 IntVector3 normal){
			SPADES_MARK_FUNCTION();
			
			uint32_t col = map->GetColor(blockPos.x, blockPos.y, blockPos.z);
			IntVector3 colV = {(uint8_t)col,
				(uint8_t)(col >> 8), (uint8_t)(col >> 16)};
			Vector3 shiftedHitPos = hitPos;
			shiftedHitPos.x += normal.x * .05f;
			shiftedHitPos.y += normal.y * .05f;
			shiftedHitPos.z += normal.z * .05f;
			
			EmitBlockFragments(shiftedHitPos, colV);
			
			if(!IsMuted()){
				bool isLocal = p == world->GetLocalPlayer();
				Handle<IAudioChunk> c = audioDevice->RegisterSound("Sounds/Weapons/Spade/HitBlock.wav");
				if(isLocal)
					audioDevice->PlayLocal(c, MakeVector3(.1f, -.1f, 1.2f),
										   AudioParam());
				else
					audioDevice->Play(c, p->GetOrigin() + p->GetFront() * 0.5f
									  - p->GetUp() * .2f,
									  AudioParam());
			}
		}
		
		void Client::PlayerKilledPlayer(spades::client::Player *killer,
										spades::client::Player *victim,
										KillType kt) {
			// play hit sound
			if(kt == KillTypeWeapon ||
			   kt == KillTypeHeadshot) {
				// don't play on local: see BullethitPlayer
				if(victim != world->GetLocalPlayer()) {
					if(!IsMuted()){
						Handle<IAudioChunk> c =
						audioDevice->RegisterSound("Sounds/Weapons/Impacts/Flesh.wav");
						audioDevice->Play(c, victim->GetEye(), AudioParam());
					}
				}
			}
			
			// begin following
			if(victim == world->GetLocalPlayer()){
				followingPlayerId = victim->GetId();
				
				Vector3 v = -victim->GetFront();
				followYaw = atan2(v.y, v.x);
				followPitch = 30.f * M_PI /180.f;
			}
			
			// emit blood (also for local player)
			// FIXME: emiting blood for either
			// client-side or server-side hit?
			switch(kt){
				case KillTypeGrenade:
				case KillTypeHeadshot:
				case KillTypeMelee:
				case KillTypeWeapon:
					Bleed(victim->GetEye());
					break;
				default:
					break;
			}
			
			// create ragdoll corpse
			if(cg_ragdoll && victim->GetTeamId() < 2){
				Corpse *corp;
				corp = new Corpse(renderer, map, victim);
				if(victim == world->GetLocalPlayer())
					lastMyCorpse = corp;
				if(killer != victim && kt != KillTypeGrenade){
					Vector3 dir = victim->GetPosition() - killer->GetPosition();
					dir = dir.Normalize();
					if(kt == KillTypeMelee){
						dir *= 6.f;
					}else{
						if(killer->GetWeapon()->GetWeaponType() == SMG_WEAPON){
							dir *= 2.8f;
						}else if(killer->GetWeapon()->GetWeaponType() == SHOTGUN_WEAPON){
							dir *= 4.5f;
						}else{
							dir *= 3.5f;
						}
					}
					corp->AddImpulse(dir);
				}else if(kt == KillTypeGrenade){
					corp->AddImpulse(MakeVector3(0, 0, -4.f - GetRandom() * 4.f));
				}
				corp->AddImpulse(victim->GetVelocty() * 32.f);
				corpses.push_back(corp);
				
				if(corpses.size() > corpseHardLimit){
					corp = corpses.front();
					delete corp;
					corpses.pop_front();
				}else if(corpses.size() > corpseSoftLimit){
					RemoveInvisibleCorpses();
				}
			}
			
			// add chat message
			std::string s;
			s = ChatWindow::TeamColorMessage(killer->GetName(), killer->GetTeamId());
			
			std::string cause;
			bool ff = killer->GetTeamId() == victim->GetTeamId();
			if(killer == victim)
				ff = false;
			
			cause = " [";
			Weapon* w = killer ? killer->GetWeapon() : NULL;	//only used in case of KillTypeWeapon
			cause += ChatWindow::killImage( kt, w ? w->GetWeaponType() : RIFLE_WEAPON );
			cause += "] ";
			
			if(ff)
				s += ChatWindow::ColoredMessage(cause, MsgColorRed);
			else
				s += cause;
			
			if(killer != victim){
				s += ChatWindow::TeamColorMessage(victim->GetName(), victim->GetTeamId());
			}
			
			killfeedWindow->AddMessage(s);
			
			// log to netlog
			if(killer != victim) {
				NetLog("%s (%s)%s%s (%s)",
					   killer->GetName().c_str(),
					   world->GetTeam(killer->GetTeamId()).name.c_str(),
					   cause.c_str(),
					   victim->GetName().c_str(),
					   world->GetTeam(victim->GetTeamId()).name.c_str());
			}else{
				NetLog("%s (%s)%s",
					   killer->GetName().c_str(),
					   world->GetTeam(killer->GetTeamId()).name.c_str(),
					   cause.c_str());
			}
			
			// show big message if player is involved
			if(victim != killer){
				Player* local = world->GetLocalPlayer();
				if(killer == local || victim == local){
					char buf[256];
					if( killer == local ) {
						sprintf(buf, "You have killed %s", victim->GetName().c_str());
					} else {
						sprintf(buf, "You were killed by %s", killer->GetName().c_str());
					}
					centerMessageView->AddMessage(buf);
				}
			}
		}
		
		void Client::BulletHitPlayer(spades::client::Player *hurtPlayer,
									 HitType type,
									 spades::Vector3 hitPos,
									 spades::client::Player *by) {
			SPADES_MARK_FUNCTION();
			
			SPAssert(type != HitTypeBlock);
			
			// don't bleed local player
			if(hurtPlayer != world->GetLocalPlayer() ||
			   ShouldRenderInThirdPersonView()){
				Bleed(hitPos);
			}
			
			if(hurtPlayer == world->GetLocalPlayer()){
				// don't player hit sound now;
				// local bullet impact sound is
				// played by checking the decrease of HP
				return;
			}
			
			if(!IsMuted()){
				if(type == HitTypeMelee){
					Handle<IAudioChunk> c =
					audioDevice->RegisterSound("Sounds/Weapons/Spade/HitPlayer.wav");
					audioDevice->Play(c, hitPos,
									  AudioParam());
				}else{
					Handle<IAudioChunk> c = 
					audioDevice->RegisterSound("Sounds/Weapons/Impacts/Flesh.wav");
					audioDevice->Play(c, hitPos,
									  AudioParam());
				}
			}
			
			if(by == world->GetLocalPlayer() &&
			   hurtPlayer){
				net->SendHit(hurtPlayer->GetId(), type);
			}
			
			
			
		}
		
		void Client::BulletHitBlock(Vector3 hitPos,
									IntVector3 blockPos,
									IntVector3 normal){
			SPADES_MARK_FUNCTION();
			
			uint32_t col = map->GetColor(blockPos.x, blockPos.y, blockPos.z);
			IntVector3 colV = {(uint8_t)col,
			(uint8_t)(col >> 8), (uint8_t)(col >> 16)};
			Vector3 shiftedHitPos = hitPos;
			shiftedHitPos.x += normal.x * .05f;
			shiftedHitPos.y += normal.y * .05f;
			shiftedHitPos.z += normal.z * .05f;
			
			EmitBlockFragments(shiftedHitPos, colV);
			
			if(!IsMuted()){
				AudioParam param;
				param.volume = 4.f;
				
				Handle<IAudioChunk> c = audioDevice->RegisterSound("Sounds/Weapons/Impacts/Block.wav");
				audioDevice->Play(c, shiftedHitPos,
								  param);
				
				param.pitch = .9f + GetRandom() * 0.2f;
				switch((rand() >> 6) & 3){
					case 0:
						c = audioDevice->RegisterSound("Sounds/Weapons/Impacts/Ricochet1.wav");
						break;
					case 1:
						c = audioDevice->RegisterSound("Sounds/Weapons/Impacts/Ricochet2.wav");
						break;
					case 2:
						c = audioDevice->RegisterSound("Sounds/Weapons/Impacts/Ricochet3.wav");
						break;
					case 3:
						c = audioDevice->RegisterSound("Sounds/Weapons/Impacts/Ricochet4.wav");
						break;
				}
				audioDevice->Play(c, shiftedHitPos,
								  param);
			}
		}
		
		void Client::AddBulletTracer(spades::client::Player *player,
									 spades::Vector3 muzzlePos,
									 spades::Vector3 hitPos) {
			Tracer *t;
			float vel;
			switch(player->GetWeapon()->GetWeaponType()) {
				case RIFLE_WEAPON:
					vel = 700.f;
					break;
				case SMG_WEAPON:
					vel = 360.f;
					break;
				case SHOTGUN_WEAPON:
					return;
			}
			t = new Tracer(this, muzzlePos, hitPos, vel);
			AddLocalEntity(t);
		}
		
		void Client::BlocksFell(std::vector<IntVector3> blocks) {
			if(blocks.empty())
				return;
			FallingBlock *b = new FallingBlock(this, blocks);
			AddLocalEntity(b);
			
			if(!IsMuted()){
				
				IntVector3 v = blocks[0];
				Vector3 o;
				o.x = v.x; o.y = v.y; o.z = v.z;
				o += .5f;
				
				Handle<IAudioChunk> c =
				audioDevice->RegisterSound("Sounds/Misc/BlockFall.wav");
				audioDevice->Play(c, o,
								  AudioParam());
			}
		}
		
		void Client::GrenadeBounced(spades::client::Grenade *g){
			SPADES_MARK_FUNCTION();
			
			if(g->GetPosition().z < 63.f){
				if(!IsMuted()){
					Handle<IAudioChunk> c = audioDevice->RegisterSound("Sounds/Weapons/Grenade/Bounce.wav");
					audioDevice->Play(c, g->GetPosition(),
									  AudioParam());
				}
			}
		}
		
		void Client::GrenadeDroppedIntoWater(spades::client::Grenade *g){
			SPADES_MARK_FUNCTION();
			
			if(!IsMuted()){
				Handle<IAudioChunk> c = audioDevice->RegisterSound("Sounds/Weapons/Grenade/DropWater.wav");
				audioDevice->Play(c, g->GetPosition(),
								  AudioParam());
			}
		}
		
		void Client::GrenadeExploded(spades::client::Grenade *g) {
			SPADES_MARK_FUNCTION();
			
			bool inWater = g->GetPosition().z > 63.f;
			
			if(inWater){
				if(!IsMuted()){
					Handle<IAudioChunk> c = audioDevice->RegisterSound("Sounds/Weapons/Grenade/WaterExplode.wav");
					AudioParam param;
					param.volume = 10.f;
					audioDevice->Play(c, g->GetPosition(),
									  param);
					
					c = audioDevice->RegisterSound("Sounds/Weapons/Grenade/WaterExplodeFar.wav");
					param.volume = 40.f;
					audioDevice->Play(c, g->GetPosition(),
									  param);
					
					
					c = audioDevice->RegisterSound("Sounds/Weapons/Grenade/WaterExplodeStereo.wav");
					param.volume = 40.f;
					audioDevice->Play(c, g->GetPosition(),
									  param);
				}
				
				GrenadeExplosionUnderwater(g->GetPosition());
			}else{
				
				GrenadeExplosion(g->GetPosition());
				
				if(!IsMuted()){
					Handle<IAudioChunk> c;
					
					switch((rand() >> 8) & 3){
					case 0:
						c = audioDevice->RegisterSound("Sounds/Weapons/Grenade/Explode1.wav");
						break;
					case 1:
						c = audioDevice->RegisterSound("Sounds/Weapons/Grenade/Explode2.wav");
						break;
					case 2:
						c = audioDevice->RegisterSound("Sounds/Weapons/Grenade/Explode3.wav");
						break;
					case 3:
						c = audioDevice->RegisterSound("Sounds/Weapons/Grenade/Explode4.wav");
						break;
					}
					
					AudioParam param;
					param.volume = 10.f;
					param.referenceDistance = 5.f;
					audioDevice->Play(c, g->GetPosition(),
									  param);
					
					c = audioDevice->RegisterSound("Sounds/Weapons/Grenade/ExplodeStereo.wav");
					audioDevice->Play(c, g->GetPosition(),
									  param);
					
					c = audioDevice->RegisterSound("Sounds/Weapons/Grenade/ExplodeFar.wav");
					param.volume = .3f;
					param.referenceDistance = 50.f;
					audioDevice->Play(c, g->GetPosition(),
									  param);
					
					
					c = audioDevice->RegisterSound("Sounds/Weapons/Grenade/ExplodeFarStereo.wav");
					param.volume = .3f;
					param.referenceDistance = 50.f;
					audioDevice->Play(c, g->GetPosition(),
									  param);
					
					// debri sound
					c = audioDevice->RegisterSound("Sounds/Weapons/Grenade/Debris.wav");
					param.volume = 5.f;
					param.referenceDistance = 1.f;
					IntVector3 outPos;
					Vector3 soundPos = g->GetPosition();
					if(world->GetMap()->CastRay(soundPos,
												MakeVector3(0,0,1),
												8.f, outPos)){
						soundPos.z = (float)outPos.z - .2f;
					}
					audioDevice->Play(c, soundPos,
									  param);
				}
				
			}
		}
		
		void Client::LocalPlayerPulledGrenadePin() {
			SPADES_MARK_FUNCTION();
			
			if(!IsMuted()){
				Handle<IAudioChunk> c = audioDevice->RegisterSound("Sounds/Weapons/Grenade/Fire.wav");
				audioDevice->PlayLocal(c, MakeVector3(.4f, -.3f, .5f),
									   AudioParam());
			}
		}
		
		void Client::LocalPlayerBlockAction(spades::IntVector3 v, BlockActionType type){
			net->SendBlockAction(v, type);
		}
		void Client::LocalPlayerCreatedLineBlock(spades::IntVector3 v1, spades::IntVector3 v2) {
			net->SendBlockLine(v1, v2);
		}
		
		void Client::LocalPlayerHurt(HurtType type,
									 bool sourceGiven,
									 spades::Vector3 source) {
			SPADES_MARK_FUNCTION();
			if(sourceGiven){
				Player * p = world->GetLocalPlayer();
				if(!p)
					return;
				Vector3 rel = source - p->GetEye();
				rel.z = 0.f; rel = rel.Normalize();
				hurtRingView->Add(rel);
			}
		}
	}
}
