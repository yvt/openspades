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

static float nextRandom() {
	return (float)rand() / (float)RAND_MAX;
}

SPADES_SETTING(cg_ragdoll, "1");
SPADES_SETTING(cg_blood, "1");
SPADES_SETTING(cg_ejectBrass, "1");

SPADES_SETTING(cg_mouseSensitivity, "1");
SPADES_SETTING(cg_zoomedMouseSensScale, "0.6");

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

SPADES_SETTING(cg_switchToolByWheel, "1");

namespace spades {
	namespace client {
		
		Client::Client(IRenderer *r, IAudioDevice *audioDev,
					   std::string host, std::string playerName):
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
			
			textFont = new Quake3Font(renderer,
										renderer->RegisterImage("Gfx/Fonts/UbuntuCondensed.tga"),
										(const int*)UbuntuCondensedMap,
										24,
									  4);
			SPLog("Font 'Ubuntu Condensed' Loaded");
			
			bigTextFont = new Quake3Font(renderer,
									  renderer->RegisterImage("Gfx/Fonts/UbuntuCondensedBig.tga"),
									  (const int*)UbuntuCondensedBigMap,
									  48,
										 8);
			SPLog("Font 'Ubuntu Condensed (Large)' Loaded");
			
			world = NULL;
			
			frameToRendererInit = 5;
			
			// preferences?
			corpseSoftTimeLimit = 30.f; // TODO: this is not used
			corpseSoftLimit = 6;
			corpseHardLimit = 16;
			
			lastMyCorpse = NULL;
			
			renderer->SetFogDistance(128.f);
			renderer->SetFogColor(MakeVector3(.8f, 1.f, 1.f));
			
			chatWindow = new ChatWindow(this, textFont, false);
			killfeedWindow = new ChatWindow(this, textFont, true);
			chatEditing = false;
			
			hurtRingView = new HurtRingView(this);
			centerMessageView = new CenterMessageView(this, bigTextFont);
			mapView = new MapView(this, false);
			largeMapView = new MapView(this, true);
			scoreboard = new ScoreboardView(this);
			limbo = new LimboView(this);
			paletteView = new PaletteView(this);
			tcView = new TCProgressView(this);
			
			time = 0.f;
			lastAliveTime = 0.f;
			readyToClose = false;
			scoreboardVisible = false;
			flashlightOn = false;
			lastKills = 0;
			
			logStream = NULL;
			
			localFireVibrationTime = -1.f;
			viewWeaponOffset = MakeVector3(0, 0, 0);
			
			lastPosSentTime = 0.f;
			worldSubFrame = 0.f;
			grenadeVibration = 0.f;
			inGameLimbo = false;
			
			nextScreenShotIndex = 0;
			
						
			timeSinceInit = 0.f;
		}
		
		void Client::SetWorld(spades::client::World *w) {
			SPADES_MARK_FUNCTION();
			
			if(world == w){
				return;
			}
			
			
			RemoveAllCorpses();
			lastHealth = 0;
			lastHurtTime = -100.f;
			hurtRingView->ClearAll();
			scoreboardVisible = false;
			flashlightOn = false;
			
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
			aimDownState = 0.f;
			sprintState = 0.f;
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
			
			// do before deleting world
			renderer->SetGameMap(NULL);
			
			if(world)
				delete world;
			
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
			delete designFont;
			delete textFont;
			delete bigTextFont;
		}
		
		bool Client::WantsToBeClosed() {
			return readyToClose;
		}
		
		void Client::DoInit() {
			renderer->Init();
			// preload
			SmokeSpriteEntity(this, Vector4(), 20.f);
			
			renderer->RegisterModel("Models/Weapons/Grenade/Grenade.kv6");
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
			renderer->RegisterModel
			("Models/Player/Arm.kv6");
			renderer->RegisterModel
			("Models/Player/UpperArm.kv6");
			renderer->RegisterModel
			("Models/Player/LegCrouch.kv6");
			renderer->RegisterModel
			("Models/Player/TorsoCrouch.kv6");
			renderer->RegisterModel
			("Models/Player/Leg.kv6");
			renderer->RegisterModel
			("Models/Player/Torso.kv6");
			renderer->RegisterModel
			("Models/Player/Arms.kv6");
			renderer->RegisterModel
			("Models/Player/Head.kv6");
			renderer->RegisterModel
			("Models/MapObjects/Intel.kv6");
			renderer->RegisterModel("Models/MapObjects/CheckPoint.kv6");
			renderer->RegisterImage("Gfx/Sight.tga");
			renderer->RegisterImage("Gfx/Bullet/7.62mm.tga");
			renderer->RegisterImage("Gfx/Bullet/9mm.tga");
			renderer->RegisterImage("Gfx/Bullet/12gauge.tga");
			renderer->RegisterImage("Gfx/CircleGradient.png");
			renderer->RegisterImage("Gfx/LoadingWindow.png");
			renderer->RegisterImage("Gfx/LoadingWindowGlow.png");
			renderer->RegisterImage("Gfx/LoadingStripe.png");
			audioDevice->RegisterSound("Sounds/Feedback/Chat.wav");
			
			SPLog("Started connecting to '%s'", hostname.c_str());
			net = new NetClient(this);
			net->Connect(hostname);
			
			//net->Connect("192.168.24.24");
			//net->Connect("127.0.0.1");
			
			// decide log file name
			std::string fn = hostname;
			if(fn.find("aos:///") == 0)
				fn = fn.substr(7);
			if(fn.find("aos://") == 0)
				fn = fn.substr(6);
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
				SPLog("Failed to open netlog file '%s'", fn2.c_str());
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
					net->DoEvents(30);
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

					if(selectedTool != player->GetTool() ||
					   toolRaiseState < .999f) {
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
					
					PlayerInput actualInput = player->GetInput();
					WeaponInput actualWeapInput = player->GetWeaponInput();
					
					if(actualWeapInput.secondary && player->IsToolWeapon() &&
					   player->IsAlive()){
						aimDownState += dt * 6.f;
						if(aimDownState > 1.f)
							aimDownState = 1.f;
					}else{
						aimDownState -= dt * 3.f;
						if(aimDownState < 0.f)
							aimDownState = 0.f;
						
						if(player->IsToolWeapon()){
							// there is a possibility that player has respawned or something.
							// stop aiming down
							weapInput.secondary = false;
						}
					}
					
					Vector3 vel = player->GetVelocty();
					vel.z = 0.f;
					if(actualInput.sprint && player->IsAlive() &&
					   vel.GetLength() > .1f){
						sprintState += dt * 4.f;
						if(sprintState > 1.f)
							sprintState = 1.f;
					}else{
						sprintState -= dt * 3.f;
						if(sprintState < 0.f)
							sprintState = 0.f;
					}
					
					if(!player->IsToolSelectable(selectedTool)) {
						// select another tool
						Player::ToolType t = selectedTool;
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
					if(selectedTool == player->GetTool()) {
						toolRaiseState += dt * 4.f;
						if(toolRaiseState > 1.f)
							toolRaiseState = 1.f;
					}else{
						toolRaiseState -= dt * 4.f;
						if(toolRaiseState < 0.f){
							toolRaiseState = 0.f;
							player->SetTool(selectedTool);
							net->SendTool();
							
							// TODO: play tool raise sound
						}
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
						
						IAudioChunk *c;
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
			
			// Well done!
			renderer->FrameDone();
			renderer->Flip();
			
			if(world){
				Player* player = world->GetLocalPlayer();
				// view effects
				if(player){
					float scale = dt;
					Vector3 vel = player->GetVelocty();
					Vector3 front = player->GetFront();
					Vector3 right = player->GetRight();
					Vector3 up = player->GetUp();
					viewWeaponOffset.x += Vector3::Dot(vel, right) * scale;
					viewWeaponOffset.y -= Vector3::Dot(vel, front) * scale;
					viewWeaponOffset.z += Vector3::Dot(vel, up) * scale;
					if(dt > 0.f)
						viewWeaponOffset *= powf(.02f, dt);
					
					if(player->GetTool() == Player::ToolWeapon &&
					   player->GetWeaponInput().secondary) {
						
						if(dt > 0.f)
							viewWeaponOffset *= powf(.01f, dt);
						
						const float limitX = .01f;
						const float limitY = .01f;
						if(viewWeaponOffset.x < -limitX)
							viewWeaponOffset.x = Mix(viewWeaponOffset.x, -limitX, .5f);
						if(viewWeaponOffset.x > limitX)
							viewWeaponOffset.x = Mix(viewWeaponOffset.x, limitX, .5f);
						if(viewWeaponOffset.z < 0.f)
							viewWeaponOffset.z = Mix(viewWeaponOffset.z, 0.f, .5f);
						if(viewWeaponOffset.z > limitY)
							viewWeaponOffset.z = Mix(viewWeaponOffset.z, limitY, .5f);
					}
					
				}else{
					viewWeaponOffset = MakeVector3(0,0,0);
				}
				
			}
			
			time += dt;
		}
		
		void Client::Closing() {
			SPADES_MARK_FUNCTION();
			
		}
		
		void Client::MouseEvent(float x, float y) {
			SPADES_MARK_FUNCTION();
			
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
				if(followPitch < -M_PI*.45f) followPitch = -M_PI*.45f;
				if(followPitch > M_PI*.45f) followPitch = M_PI * .45f;
				followYaw = fmodf(followYaw, M_PI*2.f);
			}else if(world && world->GetLocalPlayer()){
				Player *p = world->GetLocalPlayer();
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
			
			if(chatEditing){
				ChatCharEvent(ch);
				return;
			}
			
			if(ch == "/"){
				ActivateChatTextEditor(false);
				ChatCharEvent("/");
			}
		}
		
		// TODO: this might not be a fast way
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
			
			if(chatEditing){
				if(down)
					ChatKeyEvent(name);
				return;
			}
			
			if(name == "Escape"){
				if(down){
					if(inGameLimbo){
						inGameLimbo = false;
					}else{
						readyToClose = true;
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
						if(world->GetLocalPlayer()->IsToolWeapon()){
							if(down && !playerInput.sprint){
								weapInput.secondary = !weapInput.secondary;
							}
						}else{
							weapInput.secondary = down;
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
						ActivateChatTextEditor(true);
					}else if(CheckKey(cg_keyTeamChat, name) && down){
						// team chat
						ActivateChatTextEditor(false);
					}else if(CheckKey(cg_keyCaptureColor, name) && down){
						CaptureColor();
					}else if(CheckKey(cg_keyChangeMapScale, name) && down){
						mapView->SwitchScale();
						IAudioChunk *chunk = audioDevice->RegisterSound("Sounds/Misc/SwitchMapZoom.wav");
						audioDevice->PlayLocal(chunk, AudioParam());
					}else if(CheckKey(cg_keyToggleMapZoom, name) && down){
						if(largeMapView->ToggleZoom()){
							IAudioChunk *chunk = audioDevice->RegisterSound("Sounds/Misc/OpenMap.wav");
							audioDevice->PlayLocal(chunk, AudioParam());
						}else{
							IAudioChunk *chunk = audioDevice->RegisterSound("Sounds/Misc/CloseMap.wav");
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
					}else if(CheckKey(cg_keyFlashlight, name) && down){
						flashlightOn = !flashlightOn;
						flashlightOnTime = time;
						IAudioChunk *chunk = audioDevice->RegisterSound("Sounds/Player/Flashlight.wav");
						audioDevice->PlayLocal(chunk, AudioParam());
					}else if(cg_switchToolByWheel && down) {
						bool rev = (int)cg_switchToolByWheel > 0;
						if(name == (rev ? "WheelDown":"WheelUp")) {
							if(world->GetLocalPlayer()->GetTeamId() < 2 &&
							   world->GetLocalPlayer()->IsAlive()){
								Player::ToolType t = selectedTool;
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
								Player::ToolType t = selectedTool;
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
		
		void Client::SetSelectedTool(Player::ToolType type, bool quiet) {
			if(type == selectedTool)
				return;
			selectedTool = type;
			
			if(!quiet) {
				IAudioChunk *c = audioDevice->RegisterSound("Sounds/Weapons/SwitchLocal.wav");
				audioDevice->PlayLocal(c, MakeVector3(.4f, -.3f, .5f),
									   AudioParam());
			}
		}
		
#pragma mark - Drawing
		
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
			
			Bitmap *bmp = renderer->ReadBitmap();
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
				delete bmp;
			}catch(const std::exception& ex){
				std::string msg;
				msg = "Screenshot failed: ";
				std::vector<std::string> lines = SplitIntoLines(ex.what());
				msg += lines[0];
				msg = ChatWindow::ColoredMessage(msg, MsgColorRed);
				chatWindow->AddMessage(msg);
				delete bmp;
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
						
						
						def.fovX = 90.f * M_PI /180.f;
						def.fovY = atanf(tanf(def.fovX * .5f) *
										 renderer->ScreenHeight() /
										 renderer->ScreenWidth()) * 2.f;
						
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
						
						def.fovY = 60.f * M_PI /180.f;
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
							float sp = SmoothStep(sprintState);
							vibYaw += sinf(player->GetWalkAnimationProgress() * M_PI * 2.f) * 0.01f * sp;
							roll -= sinf(player->GetWalkAnimationProgress() * M_PI * 2.f) * 0.005f * (sp);
							float p = cosf(player->GetWalkAnimationProgress() * M_PI * 2.f);
							p = p * p; p *= p; p *= p; p *= p; 
							vibPitch += p * 0.01f * sp;
						}
						
						
						
						scale /= GetAimDownZoomScale();
						
						def.viewOrigin = player->GetEye();
						def.viewAxis[0] = right;
						def.viewAxis[1] = up;
						def.viewAxis[2] = front;
						
						def.fovY = 60.f * M_PI /180.f;
						def.fovX = atanf(tanf(def.fovY * .5f) *
										 renderer->ScreenWidth() /
										 renderer->ScreenHeight()) * 2.f;
						
						// for 1st view, camera blur can be used
						def.denyCameraBlur = false;
						
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
						
						def.fovX *= scale;
						def.fovY *= scale;
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
					
					def.zNear = 0.05f;
					def.zFar = 130.f;
					
					def.skipWorld = false;
				}else{
					def.viewOrigin = MakeVector3(256, 256, 4);
					def.viewAxis[0] = MakeVector3(-1, 0, 0);
					def.viewAxis[1] = MakeVector3(0, 1, 0);
					def.viewAxis[2] = MakeVector3(0, 0, 1);
					
					def.fovY = 60.f * M_PI /180.f;
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
				
				def.fovY = 60.f * M_PI /180.f;
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
		
		void Client::AddPlayerToScene(spades::client::Player *p) {
			SPADES_MARK_FUNCTION();
			
			if(p->GetTeamId() >= 2){
				// spectator, or dummy player
				return;
				
			}
			// debug
			if(false){
				IImage *img = renderer->RegisterImage("Gfx/Ball.png");
				renderer->SetColor(MakeVector4(1, 0, 0, 0));
				renderer->AddLongSprite(img, lastSceneDef.viewOrigin +
										MakeVector3(0, 0, 1),
										p->GetOrigin(), 0.5f);
			}
			
			float distancePowered = (p->GetOrigin() - lastSceneDef.viewOrigin).GetPoweredLength();
			if(distancePowered > 140.f * 140.f){
				return;
			}
			
			
			if(!p->IsAlive()){
				if(!cg_ragdoll){
					ModelRenderParam param;
					param.matrix = Matrix4::Translate(p->GetOrigin()+
													  MakeVector3(0,0,1));
					param.matrix = param.matrix * Matrix4::Scale(.1f);
					IntVector3 col = p->GetColor();
					param.customColor = MakeVector3(col.x/255.f,
													col.y/255.f,
													col.z/255.f);
					
					IModel *model = renderer->RegisterModel("Models/Player/Dead.kv6");
					renderer->RenderModel(model, param);
				}
				return;
			}
			
			std::string weapPrefix = GetWeaponPrefix("Models", p->GetWeapon()->GetWeaponType());
			
			Matrix4 eyeMatrix = Matrix4::FromAxis(-p->GetRight(),
												  p->GetFront(),
												  -p->GetUp(),
												  p->GetEye());
			if(p == world->GetLocalPlayer() &&
			   !ShouldRenderInThirdPersonView()){
				
				if(flashlightOn){
					float brightness;
					brightness = time - flashlightOnTime;
					brightness = 1.f - expf(-brightness * 5.f);
					
					// add flash light
					DynamicLightParam light;
					light.origin = (eyeMatrix * MakeVector3(0, -0.05f, -0.1f)).GetXYZ();
					light.color = MakeVector3(1, .7f, .5f) * 1.5f * brightness;
					light.radius = 40.f;
					light.type = DynamicLightTypeSpotlight;
					light.spotAngle = 30.f * M_PI / 180.f;
					light.spotAxis[0] = p->GetRight();
					light.spotAxis[1] = p->GetUp();
					light.spotAxis[2] = p->GetFront();
					light.image = renderer->RegisterImage("Gfx/Spotlight.tga");
					renderer->AddLight(light);
					
					light.color *= .3f;
					light.radius = 10.f;
					light.type = DynamicLightTypePoint;
					light.image = NULL;
					renderer->AddLight(light);
					
					// add glare
					renderer->SetColor(MakeVector4(1, .7f, .5f, 0) * brightness * .3f);
					renderer->AddSprite(renderer->RegisterImage("Gfx/Glare.tga"),
										(eyeMatrix * MakeVector3(0, 0.3f, -0.3f)).GetXYZ(),
										.8f, 0.f);
				}
				
				Vector3 leftHand, rightHand;
				leftHand = MakeVector3(0, 0, 0);
				rightHand = MakeVector3(0, 0, 0);
				
				// view weapon
				float sprint = SmoothStep(sprintState);
				float putdown = 1.f - toolRaiseState;
				putdown *= putdown;
				putdown = std::min(1.f, putdown * 1.5f);
				
				Vector3 viewWeaponOffset = this->viewWeaponOffset;
				
				// bobbing
				{
					float sp = 1.f - aimDownState;
					sp *= .3f;
					sp *= std::min(1.f, p->GetVelocty().GetLength() * 5.f);
					viewWeaponOffset.x += sinf(p->GetWalkAnimationProgress() * M_PI * 2.f) * 0.01f * sp;
					float vl = cosf(p->GetWalkAnimationProgress() * M_PI * 2.f);
					vl *= vl;
					viewWeaponOffset.z += vl * 0.012f * sp;
				}
				
				// slow pulse
				{
					float sp = 1.f - aimDownState;
					float vl = sinf(world->GetTime() * 1.f);
					
					viewWeaponOffset.x += vl * 0.001f * sp;
					viewWeaponOffset.y += vl * 0.0007f * sp;
					viewWeaponOffset.z += vl * 0.003f * sp;
				}
				
				if(p->GetTool() == Player::ToolSpade){
					WeaponInput inp = p->GetWeaponInput();
					Matrix4 mat = Matrix4::Scale(0.033f);
					if(inp.primary) {
						float per = p->GetSpadeAnimationProgress();
						per = 1.f - per;
						mat = Matrix4::Rotate(MakeVector3(1, 0, 0),
											  per * 1.7f) * mat;
						mat = Matrix4::Translate(MakeVector3(0, per*0.3f, 0)) * mat;
					}else if(inp.secondary) {
						float per = p->GetDigAnimationProgress();
						bool first = p->IsFirstDig();
						float ang;
						const float readyFront = -0.8f;
						float front = readyFront;
						float side = 1.f;
						const float digAngle = .6f;
						const float readyAngle = 0.6f;
						if(per < .5f) {
							if(first) {
								// bringing to the position
								per *= 2.f;
								per *= per;
								ang = per * readyAngle;
								side = per;
								front = per * readyFront;
							}else{
								// digged!
								ang = readyAngle;
								per = (.5f - per) / .5f;
								per *= per;
								per *= per;
								ang += per * digAngle;
								front += per * 2.f;
							}
						}else{
							per = (per - .5f) / .5f;
							per = 1.f - (1.f-per)*(1.f-per);
							ang = readyAngle +
							per * digAngle;
							front += per * 2.f;
						}
						mat = Matrix4::Rotate(MakeVector3(1, 0, 0),
											  ang) * mat;
						mat = Matrix4::Rotate(MakeVector3(0, 0, 1),
											  front * .15f) * mat;
						
						side *= .3f;
						front *= .1f;
						mat = Matrix4::Translate(MakeVector3(side, front, front * .2f)) * mat;
					}
					
					if(sprint > 0.f || putdown > 0.f){
						
						float per = std::max(sprint, putdown);
						mat = Matrix4::Rotate(MakeVector3(0, 1, 0),
											  per * 1.3f) * mat;
						mat = Matrix4::Translate(MakeVector3(0.3f, -0.4f, -0.1f) * per) * mat;
						
					}
					
					mat = Matrix4::Translate(0.f, putdown * -.3f, 0.f) * mat;
					
					mat = Matrix4::Translate(-0.3f, .7f, 0.3f) * mat;
					mat = Matrix4::Translate(viewWeaponOffset) * mat;
					
					leftHand = (mat * MakeVector3(0.0f, 0.0f, 7.f)).GetXYZ();
					rightHand = (mat * MakeVector3(0.0f, 0.0f, -2.f)).GetXYZ();
					
					mat = eyeMatrix * mat;
					
					ModelRenderParam param;
					param.matrix = mat;
					param.depthHack = true;
					
					IModel *model = renderer->RegisterModel("Models/Weapons/Spade/Spade.kv6");
					renderer->RenderModel(model, param);
					
					
				}else if(p->GetTool() == Player::ToolBlock){
					if(p->IsReadyToUseTool()){
						Matrix4 mat = Matrix4::Scale(0.033f);
						if(sprint > 0.f){
							mat = Matrix4::Rotate(MakeVector3(0, 0, 1),
												  sprint * -0.3f) * mat;
							mat = Matrix4::Translate(MakeVector3(0.1f, -0.4f, -0.05f) * sprint) * mat;
						}
						mat = Matrix4::Translate(-0.3f, .7f, 0.3f) * mat;
						mat = Matrix4::Translate(viewWeaponOffset) * mat;
						
						
						mat = Matrix4::Translate(putdown * -.1f,
												 putdown * -.3f,
												 putdown * .2f) * mat;
						
						leftHand = (mat * MakeVector3(5.0f, -1.0f, 4.f)).GetXYZ();
						rightHand = (mat * MakeVector3(-5.5f, 3.0f, -5.f)).GetXYZ();
						
						mat = eyeMatrix * mat;
						
						ModelRenderParam param;
						param.matrix = mat;
						param.depthHack = true;
						param.customColor = MakeVector3(p->GetBlockColor()) / 255.f;
						
						IModel *model = renderer->RegisterModel("Models/Weapons/Block/Block2.kv6");
						renderer->RenderModel(model, param);
					}
				}else if(p->GetTool() == Player::ToolGrenade){
					float tim = p->GetTimeToNextGrenade();
					if(p->IsReadyToUseTool()){
						WeaponInput inp = p->GetWeaponInput();
						float bring = 0.f;
						float pin = 0.f;
						float side = 0.f;
						if(tim < 0.f) {
							bring = std::min(1.f, -tim * 5.f);
							bring = 1.f - bring;
							bring = 1.f - bring * bring;
						}
						
						if(inp.primary) {
							pin = p->GetGrenadeCookTime() * 8.f;
							if(pin > 2.f)pin = 2.f;
							
							if(pin > 1.f){
								side += pin - 1.f;
								bring -= (pin - 1.f) * 2.f;
							}
						}
						
						Matrix4 mat = Matrix4::Scale(0.033f);
						if(sprint > 0.f){
							mat = Matrix4::Rotate(MakeVector3(0, 0, 1),
												  sprint * -0.3f) * mat;
							mat = Matrix4::Translate(MakeVector3(0.1f, -0.4f, -0.05f) * sprint) * mat;
						}
						mat = Matrix4::Translate(-0.3f - side * .8f,
												 .8 - bring * .1f,
												 0.45f - bring * .15f) * mat;
						
						
						mat = Matrix4::Translate(putdown * -.1f,
												 putdown * -.3f,
												 putdown * .1f) * mat;
						
						mat = Matrix4::Translate(viewWeaponOffset) * mat;
						
						leftHand = (mat * MakeVector3(10.0f, -1.0f, 10.f)).GetXYZ();
						rightHand = (mat * MakeVector3(-3.f, 1.0f, 5.f)).GetXYZ();
						
						Vector3 leftHand2 = (mat * MakeVector3(2.f, 1.0f, -2.f)).GetXYZ();
						Vector3 leftHand3 = (mat * MakeVector3(8.0f, -1.0f, 10.f)).GetXYZ();
						if(pin < 1.f){
							leftHand = Mix(leftHand, leftHand2, pin);
						}else{
							leftHand = Mix(leftHand2, leftHand3, pin - 1.f);
						}
						
						mat = eyeMatrix * mat;
						
						ModelRenderParam param;
						param.matrix = mat;
						param.depthHack = true;
						
						IModel *model = renderer->RegisterModel("Models/Weapons/Grenade/Grenade.kv6");
						renderer->RenderModel(model, param);
					}else{
						// throwing
						float per = .5f - p->GetTimeToNextGrenade();
						per *= 6.f;
						if(per > 1.f) per = 1.f;
						
						leftHand = MakeVector3(0.5f, 0.5f, 0.6f);
						
						float p2 = per - .6f;
						p2 = .9f - p2 * p2 * 2.5f;
						rightHand = MakeVector3(-0.2f, p2,
												-.9f + per * 1.8f);
					}
				}else if(p->GetTool() == Player::ToolWeapon){
					Matrix4 mat = Matrix4::Scale(0.033f);
					float vib = GetLocalFireVibration();
					float aDown = SmoothStep(aimDownState);
					float motion = 1.f - aDown * 0.4f;
					float weapUp = aDown;
					switch(p->GetWeapon()->GetWeaponType()){
						case RIFLE_WEAPON: weapUp *= 0.05f; break;
						case SMG_WEAPON: weapUp *= 0.028f; break;
						case SHOTGUN_WEAPON: weapUp *= 0.05f; break;
						default: SPInvalidEnum("p->GetWeapon()->GetWeaponType()", p->GetWeapon()->GetWeaponType());
					}
					if(sprint > 0.f){
						
						mat = Matrix4::Rotate(MakeVector3(0, 0, 1),
											  sprint * -1.3f) * mat;
						mat = Matrix4::Rotate(MakeVector3(0, 1, 0),
											  sprint *
											  0.2f) * mat;
						mat = Matrix4::Translate(MakeVector3(0.2f, -0.2f, 0.05f) * sprint) * mat;
						
					}
					
					if(putdown > 0.f){
						mat = Matrix4::Rotate(MakeVector3(0, 0, 1),
											  putdown * -1.3f) * mat;
						mat = Matrix4::Rotate(MakeVector3(0, 1, 0),
											  putdown *
											  0.2f) * mat;
						mat = Matrix4::Translate(MakeVector3(0.1f, -0.3f, 0.1f) * putdown) * mat;
					}
					
					
					mat = Matrix4::Translate(-0.13f * (1.f - aDown),
											 .5f,
											 0.2f - weapUp) * mat;
					mat = Matrix4::Translate(viewWeaponOffset * motion) * mat;
					mat = Matrix4::Translate(sinf(vib*M_PI*2.f)*0.008f * motion,
											 vib*(vib-1.f)*0.14f * motion,
											 vib*(1.f-vib)*0.03f * motion) * mat;
					bool reloading = p->GetWeapon()->IsReloading();
					float reload = p->GetWeapon()->GetReloadProgress();
					
					switch(p->GetWeapon()->GetWeaponType()){
						case SMG_WEAPON:
						{
							leftHand = (mat * MakeVector3(1.0f, 6.0f, 1.f)).GetXYZ();
							rightHand = (mat * MakeVector3(0.f, -8.0f, 2.f)).GetXYZ();
							
							Vector3 leftHand2 = (mat * MakeVector3(5.0f, -10.0f, 4.f)).GetXYZ();
							Vector3 leftHand3 = (mat * MakeVector3(1.0f, 6.0f, -4.f)).GetXYZ();
							Vector3 leftHand4 = (mat * MakeVector3(1.0f, 9.0f, -6.f)).GetXYZ();
							
							ModelRenderParam param;
							param.depthHack = true;
							param.matrix = eyeMatrix * mat;
							
							{
								std::string path = weapPrefix + "/WeaponNoMagazine.kv6";
								IModel *model = renderer->RegisterModel(path.c_str());
								renderer->RenderModel(model, param);
							}
							
							if(false){ // just debug
								IImage *img = renderer->RegisterImage("Gfx/Ball.png");
								renderer->SetColor(MakeVector4(1, 0.3f, 0.1f, 0));
								renderer->AddLongSprite(img,
														(param.matrix * MakeVector3(0, 10, -2)).GetXYZ(),
														(param.matrix * MakeVector3(0, 1000, -2)).GetXYZ(),
														0.03f);
							}
							
							mat = mat * Matrix4::Translate(0.f, 3.f, 1.f);
							reload *= 2.5f;
							if(reloading) {
								if(reload < 0.7f){
									float per = reload / .7f;
									mat = mat * Matrix4::Translate(0.f, 0.f, per * per * 50.f);
									leftHand = Mix(leftHand, leftHand2, SmoothStep(per));
								}else if(reload < 1.4f){
									float per = (1.4f - reload) / .7f;
									if(per < .3f) {
										per *= 4.f;
										per -= .4f;
										per = std::max(std::min(per, .3f), 0.f);
									}
									mat = mat * Matrix4::Translate(0.f, 0.f, per * per * 10.f);
									
									leftHand = (mat * MakeVector3(0.f, 0.f, 4.f)).GetXYZ();
								}else if(reload < 1.9f){
									float per = (reload - 1.4f) / .5f;
									leftHand = (mat * MakeVector3(0.f, 0.f, 4.f)).GetXYZ();
									leftHand = Mix(leftHand, leftHand3, SmoothStep(per));
								}else if(reload < 2.2f){
									float per = (reload - 1.9f) / .3f;
									leftHand = Mix(leftHand3, leftHand4, SmoothStep(per));
								}else {
									float per = (reload - 2.2f) / .3f;
									leftHand = Mix(leftHand4, leftHand, SmoothStep(per));
								}
							}
							param.matrix = eyeMatrix * mat;
							{
								std::string path = weapPrefix + "/Magazine.kv6";
								IModel *model = renderer->RegisterModel(path.c_str());
								renderer->RenderModel(model, param);
							}
						}
							break;
							
						case RIFLE_WEAPON:
						{
							leftHand = (mat * MakeVector3(1.0f, 6.0f, 1.f)).GetXYZ();
							rightHand = (mat * MakeVector3(0.f, -8.0f, 2.f)).GetXYZ();
							
							Vector3 leftHand2 = (mat * MakeVector3(5.0f, -10.0f, 4.f)).GetXYZ();
							Vector3 rightHand3 = (mat * MakeVector3(-2.0f, -7.0f, -4.f)).GetXYZ();
							Vector3 rightHand4 = (mat * MakeVector3(-3.0f, -4.0f, -6.f)).GetXYZ();
							
							ModelRenderParam param;
							param.depthHack = true;
							param.matrix = eyeMatrix * mat;
							
							{
								std::string path = weapPrefix + "/WeaponNoMagazine.kv6";
								IModel *model = renderer->RegisterModel(path.c_str());
								renderer->RenderModel(model, param);
							}
							
							mat = mat * Matrix4::Translate(0.f, 1.f, 1.f);
							reload *= 2.5f;
							if(reloading) {
								if(reload < 0.1f){
									float per = reload / .1f;
									
									leftHand = Mix(leftHand,
												   (mat * MakeVector3(0.f, 0.f, 4.f)).GetXYZ(),
												   SmoothStep(per));
								}else if(reload < 0.7f){
									float per = (reload-.1f) / .6f;
									if(per < .2f) {
										per *= 4.f;
										per -= .4f;
										per = std::max(std::min(per, .2f), 0.f);
									}
									if(per > .5f){
										per += per - .5f;
									}
									mat = mat * Matrix4::Translate(0.f, 0.f, per * per * 10.f);
									
									leftHand = (mat * MakeVector3(0.f, 0.f, 4.f)).GetXYZ();
									if(per > .5f){
										per = (per - .5f);
										leftHand = Mix(leftHand, leftHand2, SmoothStep(per));
									}
								}else if(reload < 1.4f){
									float per = (1.4f - reload) / .7f;
									if(per < .3f) {
										per *= 4.f;
										per -= .4f;
										per = std::max(std::min(per, .3f), 0.f);
									}
									mat = mat * Matrix4::Translate(0.f, 0.f, per * per * 10.f);
									
									leftHand = (mat * MakeVector3(0.f, 0.f, 4.f)).GetXYZ();
								}else if(reload < 1.9f){
									float per = (reload - 1.4f) / .5f;
									Vector3 orig = leftHand;
									leftHand = (mat * MakeVector3(0.f, 0.f, 4.f)).GetXYZ();
									leftHand = Mix(leftHand, orig, SmoothStep(per));
									rightHand = Mix(rightHand, rightHand3, SmoothStep(per));
								}else if(reload < 2.2f){
									float per = (reload - 1.9f) / .3f;
									rightHand = Mix(rightHand3, rightHand4, SmoothStep(per));
								}else {
									float per = (reload - 2.2f) / .3f;
									rightHand = Mix(rightHand4, rightHand, SmoothStep(per));
								}
							}
							param.matrix = eyeMatrix * mat;
							{
								std::string path = weapPrefix + "/Magazine.kv6";
								IModel *model = renderer->RegisterModel(path.c_str());
								renderer->RenderModel(model, param);
							}
						}
							break;
							
						case SHOTGUN_WEAPON:
						{
							rightHand = (mat * MakeVector3(0.f, -8.0f, 2.f)).GetXYZ();
							
							Vector3 leftHand2 = (mat * MakeVector3(5.0f, -10.0f, 4.f)).GetXYZ();
							Vector3 leftHand3 = (mat * MakeVector3(1.0f, 1.0f, 2.f)).GetXYZ();
							
							ModelRenderParam param;
							param.depthHack = true;
							param.matrix = eyeMatrix * mat;
							
							{
								std::string path = weapPrefix + "/WeaponNoPump.kv6";
								IModel *model = renderer->RegisterModel(path.c_str());
								renderer->RenderModel(model, param);
							}
							
							//mat = mat * Matrix4::Translate(0.f, 0.f, 0.f);
							reload *= .5f;
							
							leftHand = (mat * MakeVector3(0.f, 4.f, 2.f)).GetXYZ();
							
							
							if(reloading) {
								if(reload < 0.2f){
									float per = reload / .2f;
									
									leftHand = Mix(leftHand,
												   leftHand2,
												   SmoothStep(per));
								}else if(reload < 0.35f){
									float per = (reload-.2f) / .15f;
									
									leftHand = Mix(leftHand2,
												   leftHand3,
												   SmoothStep(per));
								}else if(reload < .5f){
									float per = (reload-.35f) / .15f;
									
									leftHand = Mix(leftHand3,
												   leftHand,
												   SmoothStep(per));
									
								}
							}
							
							float cockFade = 1.f;
							if(reloading) {
								if(reload < .25f ||
								   p->GetWeapon()->GetAmmo() <
								   p->GetWeapon()->GetClipSize() - 1){
									cockFade = 0.f;
								}else{
									cockFade = (reload - .25f) * 10.f;
									if(cockFade > 1.f) cockFade = 1.f;
								}
							}
							if(cockFade > 0.f) {
								float cock = 0.f;
								float tim = p->GetWeapon()->TimeToNextFire();
								if(tim < 0.f){
									// might be right after reloading...
									if(p->GetWeapon()->GetAmmo() ==
									   p->GetWeapon()->GetClipSize() &&
									   reload > .5f && reload < 1.f) {
										tim = reload - .4f;
										if(tim < .05f) {
											cock = 0.f;
										}else if(tim < .12f){
											cock = (tim - .05f) / .07f;
										}else if(tim < .26f) {
											cock = 1.f;
										}else if(tim < .36f){
											cock = 1.f - (tim - .26f) / .1f;
										}
									}
								}if(tim < .2f) {
									cock = 0.f;
								}else if(tim < .3f) {
									cock = (tim - .2f) / .1f;
								}else if(tim < .42f) {
									cock = 1.f;
								}else if(tim < .52f) {
									cock = 1.f - (tim - .42f) / .1f;
								}else{
									cock = 0.f;
								}
								cock *= cockFade;
								mat = mat * Matrix4::Translate(0.f, -cock * 1.5f, 0.f);
								leftHand = Mix(leftHand,
											   (mat * MakeVector3(0.f, 4.f, 2.f)).GetXYZ(),
											   cockFade);
							}
							
							param.matrix = eyeMatrix * mat;
							{
								std::string path = weapPrefix + "/Pump.kv6";
								IModel *model = renderer->RegisterModel(path.c_str());
								renderer->RenderModel(model, param);
							}
						}
							break;
							
						default:
							abort();
					}
					
					
				}
				
				// view hands
				if(leftHand.GetPoweredLength() > 0.001f &&
				   rightHand.GetPoweredLength() > 0.001f){
					
					
					ModelRenderParam param;
					param.depthHack = true;
					
					IModel *model = renderer->RegisterModel
					("Models/Player/Arm.kv6");
					IModel *model2 = renderer->RegisterModel
					("Models/Player/UpperArm.kv6");
					
					IntVector3 col = p->GetColor();
					param.customColor = MakeVector3(col.x/255.f,
													col.y/255.f,
													col.z/255.f);
					
					const float armlen = 0.5f;
					
					Vector3 shoulders[] = {{0.4f, 0.0f, 0.25f},
										   {-0.4f, 0.0f, 0.25f}};
					Vector3 hands[] = {leftHand, rightHand};
					Vector3 benddirs[] = {0.5f, 0.2f, 0.f,
					                      -0.5f, 0.2f, 0.f};
					for(int i = 0; i < 2; i++){
						Vector3 shoulder = shoulders[i];
						Vector3 hand = hands[i];
						Vector3 benddir = benddirs[i];
						
						float len2 = (hand - shoulder).GetPoweredLength();
						// len2/4 + x^2 = armlen^2
						float bendlen = sqrtf(std::max(armlen*armlen - len2*.25f, 0.f));
						
						Vector3 bend = Vector3::Cross(benddir, hand-shoulder);
						bend = bend.Normalize();
						
						if(bend.z < 0.f) bend.z = -bend.z;
						
						Vector3 elbow = (hand + shoulder) * .5f;
						elbow += bend * bendlen;
						
						{
							Vector3 axises[3];
							axises[2] = (hand - elbow).Normalize();
							axises[0] = MakeVector3(0, 0, 1);
							axises[1] = Vector3::Cross(axises[2], axises[0]).Normalize();
							axises[0] = Vector3::Cross(axises[1], axises[2]).Normalize();
							
							Matrix4 mat = Matrix4::Scale(.05f);
							mat = Matrix4::FromAxis(axises[0],
													axises[1],
													axises[2],
													elbow) * mat;
							mat = eyeMatrix * mat;
							
							param.matrix = mat;
							renderer->RenderModel(model, param);
						}
						
						{
							Vector3 axises[3];
							axises[2] = (elbow - shoulder).Normalize();
							axises[0] = MakeVector3(0, 0, 1);
							axises[1] = Vector3::Cross(axises[2], axises[0]).Normalize();
							axises[0] = Vector3::Cross(axises[1], axises[2]).Normalize();
							
							Matrix4 mat = Matrix4::Scale(.05f);
							mat = Matrix4::FromAxis(axises[0],
													axises[1],
													axises[2],
													shoulder) * mat;
							mat = eyeMatrix * mat;
							
							param.matrix = mat;
							renderer->RenderModel(model2, param);
						}
					}
					
				}
				
				// --- local view ends
			} else {
				ModelRenderParam param;
				IModel *model;
				Vector3 front = p->GetFront();
				IntVector3 col = p->GetColor();
				param.customColor = MakeVector3(col.x/255.f,
												col.y/255.f,
												col.z/255.f);
				
				float yaw = atan2(front.y, front.x) + M_PI * .5f;
				float pitch = -atan2(front.z, sqrt(front.x * front.x + front.y * front.y));
				
				// lower axis
				Matrix4 lower = Matrix4::Translate(p->GetOrigin());
				lower = lower * Matrix4::Rotate(MakeVector3(0,0,1),
												yaw);
				
				Matrix4 scaler = Matrix4::Scale(0.1f);
				scaler  = scaler * Matrix4::Scale(-1,-1,1);
				
				PlayerInput inp = p->GetInput();
				
				// lower
				Matrix4 torso, head, arms;
				if(inp.crouch){
					Matrix4 leg1 = Matrix4::Translate(-0.25f, 0.2f, -0.1f);
					Matrix4 leg2 = Matrix4::Translate( 0.25f, 0.2f, -0.1f);
					
					float ang = sinf(p->GetWalkAnimationProgress() * M_PI * 2.f) * 0.6f;
					float walkVel = Vector3::Dot(p->GetVelocty(), p->GetFront2D()) * 4.f;
					leg1 = leg1 * Matrix4::Rotate(MakeVector3(1,0,0),
												  ang * walkVel);
					leg2 = leg2 * Matrix4::Rotate(MakeVector3(1,0,0),
												  -ang * walkVel);
					
					walkVel = Vector3::Dot(p->GetVelocty(), p->GetRight()) * 3.f;
					leg1 = leg1 * Matrix4::Rotate(MakeVector3(0,1,0),
												  ang * walkVel);
					leg2 = leg2 * Matrix4::Rotate(MakeVector3(0,1,0),
												  -ang * walkVel);
					
					leg1 = lower * leg1;
					leg2 = lower * leg2;
					
					model = renderer->RegisterModel
					("Models/Player/LegCrouch.kv6");
					param.matrix = leg1 * scaler;
					renderer->RenderModel(model, param);
					param.matrix = leg2 * scaler;
					renderer->RenderModel(model, param);
					
					torso = Matrix4::Translate(0.f,0.f,-0.55f);
					torso = lower * torso;
					
					model = renderer->RegisterModel
					("Models/Player/TorsoCrouch.kv6");
					param.matrix = torso * scaler;
					renderer->RenderModel(model, param);
					
					head = Matrix4::Translate(0.f,0.f,-0.0f);
					head = torso * head;
					
					arms = Matrix4::Translate(0.f,0.f,-0.0f);
					arms = torso * arms;
				}else{
					Matrix4 leg1 = Matrix4::Translate(-0.25f, 0.f, -0.1f);
					Matrix4 leg2 = Matrix4::Translate( 0.25f, 0.f, -0.1f);
					
					float ang = sinf(p->GetWalkAnimationProgress() * M_PI * 2.f) * 0.6f;
					float walkVel = Vector3::Dot(p->GetVelocty(), p->GetFront2D()) * 4.f;
					leg1 = leg1 * Matrix4::Rotate(MakeVector3(1,0,0),
												  ang * walkVel);
					leg2 = leg2 * Matrix4::Rotate(MakeVector3(1,0,0),
												  -ang * walkVel);
					
					walkVel = Vector3::Dot(p->GetVelocty(), p->GetRight()) * 3.f;
					leg1 = leg1 * Matrix4::Rotate(MakeVector3(0,1,0),
												  ang * walkVel);
					leg2 = leg2 * Matrix4::Rotate(MakeVector3(0,1,0),
												  -ang * walkVel);
										
					leg1 = lower * leg1;
					leg2 = lower * leg2;
					
					model = renderer->RegisterModel
					("Models/Player/Leg.kv6");
					param.matrix = leg1 * scaler;
					renderer->RenderModel(model, param);
					param.matrix = leg2 * scaler;
					renderer->RenderModel(model, param);
					
					torso = Matrix4::Translate(0.f,0.f,-1.0f);
					torso = lower * torso;
					
					model = renderer->RegisterModel
					("Models/Player/Torso.kv6");
					param.matrix = torso * scaler;
					renderer->RenderModel(model, param);
					
					head = Matrix4::Translate(0.f,0.f,-0.0f);
					head = torso * head;
					
					arms = Matrix4::Translate(0.f,0.f,0.1f);
					arms = torso * arms;
				}
				
				float armPitch = pitch;
				if(inp.sprint) {
					armPitch -= .5f;
				}
				if(armPitch < 0.f) {
					armPitch = std::max(armPitch, -(float)M_PI * .5f);
					armPitch *= .9f;
				}
				
				arms = arms * Matrix4::Rotate(MakeVector3(1,0,0),
											  armPitch);
				
				model = renderer->RegisterModel
				("Models/Player/Arms.kv6");
				param.matrix = arms * scaler;
				renderer->RenderModel(model, param);
				
				
				head = head * Matrix4::Rotate(MakeVector3(1,0,0),
											  pitch);
				
				model = renderer->RegisterModel
				("Models/Player/Head.kv6");
				param.matrix = head * scaler;
				renderer->RenderModel(model, param);
				
				// draw tool
				if(p->GetTool() == Player::ToolSpade){
					WeaponInput inp = p->GetWeaponInput();
					Matrix4 mat = Matrix4::Scale(0.05f);
					if(inp.primary) {
						float per = p->GetSpadeAnimationProgress();
						per = 1.f - per;
						mat = Matrix4::Rotate(MakeVector3(1, 0, 0),
											  -per * 0.7f) * mat;
					}
					
					mat = Matrix4::Translate(0.35f, -1.0f, 0.0f) * mat;
					mat = arms * mat;
					
					param.matrix = mat;
					
					model = renderer->RegisterModel("Models/Weapons/Spade/Spade.kv6");
					renderer->RenderModel(model, param);
				}else if(p->GetTool() == Player::ToolBlock){
					Matrix4 mat = Matrix4::Scale(0.05f);
					mat = Matrix4::Translate(0.35f, -1.0f, 0.0f) * mat;
					mat = arms * mat;
					
					param.matrix = mat;
					param.customColor = MakeVector3(p->GetBlockColor()) / 255.f;
					
					model = renderer->RegisterModel("Models/Weapons/Block/Block2.kv6");
					renderer->RenderModel(model, param);
				}else if(p->GetTool() == Player::ToolGrenade){
					Matrix4 mat = Matrix4::Scale(0.05f);
					mat = Matrix4::Translate(0.35f, -1.0f, 0.0f) * mat;
					mat = arms * mat;
					
					param.matrix = mat;
					
					model = renderer->RegisterModel("Models/Weapons/Grenade/Grenade.kv6");
					renderer->RenderModel(model, param);
				}else if(p->GetTool() == Player::ToolWeapon){
					Matrix4 mat = Matrix4::Scale(0.05f);
					mat = mat * Matrix4::Scale(-1, -1, 1);
					mat = Matrix4::Translate(0.35f, -1.0f, 0.0f) * mat;
					mat = arms * mat;
					
					ModelRenderParam param;
					param.matrix = mat;
					
					std::string path = weapPrefix + "/Weapon.kv6";
					model = renderer->RegisterModel(path.c_str());
					renderer->RenderModel(model, param);
				}
				
				// draw intel in ctf
				CTFGameMode *ctfMode = dynamic_cast<CTFGameMode *>(world->GetMode());
				if(ctfMode){
					int tId = p->GetTeamId();
					if(tId < 3){
						CTFGameMode::Team& team = ctfMode->GetTeam(p->GetTeamId());
						if(team.hasIntel && team.carrier == p->GetId()){
							
							IntVector3 col2 = world->GetTeam(1-p->GetTeamId()).color;
							param.customColor = MakeVector3(col2.x/255.f,
															col2.y/255.f,
															col2.z/255.f);
							Matrix4 mIntel = torso * Matrix4::Translate(0,0.6f,0.5f);
							
							model = renderer->RegisterModel
							("Models/MapObjects/Intel.kv6");
							param.matrix = mIntel * scaler;
							renderer->RenderModel(model, param);
							
							param.customColor = MakeVector3(col.x/255.f,
															col.y/255.f,
															col.z/255.f);
						}
					}
				}
				
				if(false){
					// draw hitbox
					AddDebugObjectToScene(p->GetHitBoxes().torso);
					AddDebugObjectToScene(p->GetHitBoxes().head);
					AddDebugObjectToScene(p->GetHitBoxes().limbs[0]);
					AddDebugObjectToScene(p->GetHitBoxes().limbs[1]);
					AddDebugObjectToScene(p->GetHitBoxes().limbs[2]);
				}
				
				// third person player rendering, done
			}
		}
		
		void Client::AddDebugObjectToScene(const spades::OBB3 &obb,
										   const Vector4& color) {
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
				
				for(int i = 0; i < world->GetNumPlayerSlots(); i++)
					if(world->GetPlayer(i))
						AddPlayerToScene(world->GetPlayer(i));
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
						if(blocks.size() > (int)p->GetNumBlocks())
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
				Player *hottracked = HotTrackedPlayer();
				if(hottracked){
					IntVector3 col = world->GetTeam(hottracked->GetTeamId()).color;
					Vector4 color;
					color.x = col.x / 255.f;
					color.y = col.y / 255.f;
					color.z = col.z / 255.f;
					color.w = 1.f;
					
					Player::HitBoxes hb = hottracked->GetHitBoxes();
					AddDebugObjectToScene(hb.head, color);
					AddDebugObjectToScene(hb.torso, color);
					AddDebugObjectToScene(hb.limbs[0], color);
					AddDebugObjectToScene(hb.limbs[1], color);
					AddDebugObjectToScene(hb.limbs[2], color);
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
			IImage *img;
			Vector2 siz;
			Vector2 scrSize = {renderer->ScreenWidth(),
				renderer->ScreenHeight()};
			renderer->SetColor(MakeVector4(1, 1, 1, 1.));
			img = renderer->RegisterImage("Gfx/Splash.jpg");
			
			siz = MakeVector2(img->GetWidth(), img->GetHeight());
			siz *= scrSize.x / siz.x;
			siz *= std::min(1.f, scrSize.y / siz.y);
			
			renderer->DrawImage(img, AABB2((scrSize.x - siz.x) * .5f,
										   (scrSize.y - siz.y) * .5f,
										   siz.x, siz.y));
			
			
		}
		
		void Client::DrawStartupScreen() {
			IImage *img;
			Vector2 scrSize = {renderer->ScreenWidth(),
				renderer->ScreenHeight()};
			
			renderer->SetColor(MakeVector4(0, 0, 0, 1.));
			img = renderer->RegisterImage("Gfx/White.tga");
			renderer->DrawImage(img, AABB2(0, 0,
										   scrSize.x, scrSize.y));
			
			DrawSplash();
			
			IFont *font = designFont;
			std::string str = "NOW LOADING";
			Vector2 size = font->Measure(str);
			Vector2 pos = MakeVector2(scrSize.x - 16.f,
									  scrSize.y - 16.f);
			pos -= size;
			font->Draw(str,
					   pos + MakeVector2(1,1),
					   1.f, MakeVector4(0,0,0,0.5));
			font->Draw(str,
					   pos,
					   1.f, MakeVector4(1,1,1,1));
			
			renderer->FrameDone();
			renderer->Flip();
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
					
					if(wTime < lastHurtTime + .2f){
						float per = (wTime - lastHurtTime) / .2f;
						Vector3 color = {1.f, per, per};
						renderer->MultiplyScreenColor(color);
					}
					
					Player *hottracked = HotTrackedPlayer();
					if(hottracked){
						Vector3 posxyz = Project(hottracked->GetEye());
						Vector2 pos = {posxyz.x, posxyz.y};
						float dist = (hottracked->GetEye() - p->GetEye()).GetLength();
						int idist = (int)floorf(dist + .5f);
						char buf[64];
						sprintf(buf, "%s [%d%s]", hottracked->GetName().c_str(),
								idist, (idist == 1) ? "block":"blocks");
						
						font = textFont;
						Vector2 size = font->Measure(buf);
						pos.x -= size.x * .5f;
						pos.y -= size.y;
						font->Draw(buf,
								   pos + MakeVector2(1,1),
								   1.f, MakeVector4(0,0,0,0.5));
						font->Draw(buf,
								   pos,
								   1.f, MakeVector4(1,1,1,1));
					}
					
					tcView->Draw();
					
					if(p->IsAlive() && p->GetTeamId() < 2){
						// draw damage ring
						hurtRingView->Draw();
						
						// draw sight
						IImage *sight = renderer->RegisterImage("Gfx/Sight.tga");
						renderer->SetColor(MakeVector4(1,1,1,1));
						renderer->DrawImage(sight,
											MakeVector2((scrWidth - sight->GetWidth()) * .5f,
														(scrHeight - sight->GetHeight()) * .5f));
						
						// draw ammo
						Weapon *weap = p->GetWeapon();
						IImage *ammoIcon;
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
						font->Draw(stockStr,
								   pos + MakeVector2(1,1),
								   1.f, MakeVector4(0,0,0,0.5));
						font->Draw(stockStr,
								   pos,
								   1.f, numberColor);
						
						
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
								font->Draw(msg,
										   pos + MakeVector2(1,1),
										   1.f, MakeVector4(0,0,0,0.5));
								font->Draw(msg,
										   pos,
										   1.f, MakeVector4(1,1,1,1));
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
								Vector2 pos = MakeVector2((scrWidth - size.x) * .5f,
														  scrHeight / 3.f);
								font->Draw(msg,
										   pos + MakeVector2(1,1),
										   1.f, MakeVector4(0,0,0,0.5));
								font->Draw(msg,
										   pos,
										   1.f, MakeVector4(1,1,1,1));
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
						font->Draw(stockStr,
								   pos + MakeVector2(1,1),
								   1.f, MakeVector4(0,0,0,0.5));
						font->Draw(stockStr,
								   pos,
								   1.f, numberColor);
					}
					
					if(IsFollowing()){
						if(followingPlayerId == p->GetId()){
							// just spectating
						}else{
							font = textFont;
							std::string msg;
							msg = "Following " + world->GetPlayerPersistent(followingPlayerId).name;
							Vector2 size = font->Measure(msg);
							Vector2 pos = MakeVector2(scrWidth - 8.f, 256.f + 32.f);
							pos.x -= size.x;
							font->Draw(msg,
									   pos + MakeVector2(1,1),
									   1.f, MakeVector4(0,0,0,0.5));
							font->Draw(msg,
									   pos,
									   1.f, MakeVector4(1, 1, 1, 1));
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
				
				// FIXME: when to draw chat editor?
				if(chatEditing){
					Vector2 pos;
					pos.x = 8.f;
					pos.y = scrHeight - 24.f;
					
					std::string str;
					if(chatGlobal)
						str = "Global Chat: ";
					else
						str = "Team Chat: ";
					str += chatText;
					str += "_";
					
					textFont->Draw(str, pos+MakeVector2(1, 1), 1.f,
								   MakeVector4(0,0,0,0.5));
					textFont->Draw(str, pos, 1.f, MakeVector4(1,1,1,1));
				}
				
			}else{
				// no world; loading?
				DrawSplash();
				
				float fade = std::min(1.f, timeSinceInit * 2.f);
				
				// background
				IImage *img;
				float bgSize = std::max(scrWidth, scrHeight);
				renderer->SetColor(MakeVector4(1, 1, 1, .4 * fade));
				img = renderer->RegisterImage("Gfx/CircleGradient.png");
				
				renderer->DrawImage(img, AABB2((scrWidth - bgSize) * .5f,
											   (scrHeight - bgSize) * .5f,
											   bgSize, bgSize));
				
				renderer->SetColor(MakeVector4(.1, .1, .1, .8 * fade));
				img = renderer->RegisterImage("Gfx/White.tga");
				renderer->DrawImage(img, AABB2(0,0,scrWidth,scrHeight));
				
				
				// loading window
				
				renderer->SetColor(MakeVector4(1, 1, 1, fade));
				
				float wndX, wndY;
				img = renderer->RegisterImage("Gfx/LoadingWindow.png");
				
				wndX = (scrWidth - img->GetWidth()) * .5f;
				wndY = (scrHeight - img->GetHeight()) * .5f;
				wndX = floorf(wndX); wndY = floorf(wndY);
				
				renderer->DrawImage(img, MakeVector2(wndX, wndY));
				
				
				renderer->SetColor(MakeVector4(1, 1, 1, .2 * fade));
				img = renderer->RegisterImage("Gfx/LoadingWindowGlow.png");
				
				renderer->DrawImage(img, AABB2((scrWidth - 512.f) * .5f,
											   (scrHeight - 256.f) * .5f,
											   512.f, 256.f));
				
				
				renderer->SetColor(MakeVector4(1, 1, 1, fade));
				img = renderer->RegisterImage("Gfx/LoadingStripe.png");
				float scrX = time * 32.f;
				scrX = fmodf(scrX, 16.f);
				
				renderer->DrawImage(img,
									MakeVector2(wndX+11, wndY+37),
									AABB2(-scrX, 0, 218,14));
				
				std::string msg = net->GetStatusString();
				font = textFont;
				font->Draw(msg,
						   MakeVector2(wndX + 8.f,
									   wndY + 8.f),
						   1.f,
						   MakeVector4(0,0,0,fade));
								
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
				IAudioChunk *chunk = audioDevice->RegisterSound("Sounds/Feedback/Chat.wav");
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
		
		void Client::ActivateChatTextEditor(bool global) {
			chatEditing = true;
			chatGlobal = global;
			
			playerInput = PlayerInput();
			weapInput = WeaponInput();
			keypadInput = KeypadInput();
		}
		
		void Client::CloseChatTextEditor() {
			chatEditing = false;
			chatText.clear();
		}
		
		void Client::ChatCharEvent(const std::string& ch){
			SPAssert(chatEditing);
			chatText += ch;
		}
		
		void Client::ChatKeyEvent(const std::string &key) {
			SPAssert(chatEditing);
			
			if(key == "BackSpace"){
				if(!chatText.empty())
					chatText = chatText.substr(0, chatText.size() - 1);
			}else if(key == "Enter"){
				if(!chatText.empty()){
					net->SendChat(chatText, chatGlobal);
				}
				CloseChatTextEditor();
			}else if(key == "Left"){
				// TODO: cursor
			}else if(key == "Right"){
				// TODO: cursor
			}else if(key == "Escape"){
				CloseChatTextEditor();
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
		
		Player *Client::HotTrackedPlayer(){
			if(!world)
				return NULL;
			Player *p = world->GetLocalPlayer();
			if(!p || !p->IsAlive())
				return NULL;
			if(ShouldRenderInThirdPersonView())
				return NULL;
			Vector3 origin = p->GetEye();
			Vector3 dir = p->GetFront();
			World::WeaponRayCastResult result = world->WeaponRayCast(origin,
																	 dir,
																	 p);
			
			if(result.hit == false || result.player == NULL)
				return NULL;
			
			// don't hot track enemies (non-spectator only)
			if(result.player->GetTeamId() != p->GetTeamId() &&
			   p->GetTeamId() < 2)
				return NULL;
			
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
			
			//IImage *img = renderer->RegisterImage("Textures/SoftBall.tga");
			IImage *img = renderer->RegisterImage("Gfx/White.tga");
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
			
			IImage *img = renderer->RegisterImage("Gfx/White.tga");
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
			
			IImage *img = renderer->RegisterImage("Gfx/White.tga");
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
			IImage *img = renderer->RegisterImage("Gfx/White.tga");
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
			IImage *img = renderer->RegisterImage("Textures/WaterExpl.png");
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
			
			selectedTool = world->GetLocalPlayer()->GetTool();
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
				IAudioChunk *c = audioDevice->RegisterSound("Sounds/Weapons/Block/Build.wav");
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
					IAudioChunk *chunk = audioDevice->RegisterSound("Sounds/Feedback/TC/YourTeamCaptured.wav");
					audioDevice->PlayLocal(chunk, AudioParam());
				}else{
					IAudioChunk *chunk = audioDevice->RegisterSound("Sounds/Feedback/TC/EnemyCaptured.wav");
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
					IAudioChunk *chunk = audioDevice->RegisterSound("Sounds/Feedback/CTF/YourTeamCaptured.wav");
					audioDevice->PlayLocal(chunk, AudioParam());
				}else{
					IAudioChunk *chunk = audioDevice->RegisterSound("Sounds/Feedback/CTF/EnemyCaptured.wav");
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
				IAudioChunk *chunk = audioDevice->RegisterSound("Sounds/Feedback/CTF/PickedUp.wav");
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
			
			IAudioChunk *c = audioDevice->RegisterSound("Sounds/Misc/BlockDestroy.wav");
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
			
			IAudioChunk *c = audioDevice->RegisterSound("Sounds/Misc/BlockDestroy.wav");
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
					IAudioChunk *chunk = audioDevice->RegisterSound("Sounds/Feedback/Win.wav");
					audioDevice->PlayLocal(chunk, AudioParam());
				}else{
					IAudioChunk *chunk = audioDevice->RegisterSound("Sounds/Feedback/Lose.wav");
					audioDevice->PlayLocal(chunk, AudioParam());
				}
			}
		}
		
#pragma mark - IWorldListener Handlers
		
		void Client::PlayerJumped(spades::client::Player *p){
			SPADES_MARK_FUNCTION();
			
			if(!IsMuted()){
			
				IAudioChunk *c = p->GetWade() ?
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
				IAudioChunk *c;
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
				IAudioChunk *c = p->GetWade() ?
				audioDevice->RegisterSound(wsnds[rand() % 4]):
				audioDevice->RegisterSound(snds[rand() % 4]);
				audioDevice->Play(c, p->GetOrigin(),
								  AudioParam());
			}
		}
		
		void Client::PlayerFiredWeapon(spades::client::Player *p) {
			SPADES_MARK_FUNCTION();
			
			Vector3 muzzle;
			// make dlight
			{
				Vector3 vec;
				Matrix4 eyeMatrix = Matrix4::FromAxis(-p->GetRight(),
													  p->GetFront(),
													  -p->GetUp(),
													  p->GetEye());
				Matrix4 mat;
				mat = Matrix4::Translate(-0.13f,
										 .5f,
										 0.2f);
				mat = eyeMatrix * mat;
				
				vec = (mat * MakeVector3(0, 1, 0)).GetXYZ();
				muzzle = vec;
				MuzzleFire(vec, p->GetFront(), p == world->GetLocalPlayer());
			}
			
			if(cg_ejectBrass){
				float dist = (p->GetOrigin() - lastSceneDef.viewOrigin).GetPoweredLength();
				if(dist < 130.f * 130.f) {
					IModel *model = NULL;
					IAudioChunk *snd = NULL;
					IAudioChunk *snd2 = NULL;
					switch(p->GetWeapon()->GetWeaponType()){
						case RIFLE_WEAPON:
							model = renderer->RegisterModel("Models/Weapons/Rifle/Casing.kv6");
							snd = (rand()&0x1000)?
							audioDevice->RegisterSound("Sounds/Weapons/Rifle/ShellDrop1.wav"):
							audioDevice->RegisterSound("Sounds/Weapons/Rifle/ShellDrop2.wav");
							snd2 =
							audioDevice->RegisterSound("Sounds/Weapons/Rifle/ShellWater.wav");
							break;
						case SHOTGUN_WEAPON:
							// FIXME: don't want to show shotgun't casing
							// because it isn't ejected when firing
							//model = renderer->RegisterModel("Models/Weapons/Shotgun/Casing.kv6");
							break;
						case SMG_WEAPON:
							model = renderer->RegisterModel("Models/Weapons/SMG/Casing.kv6");
							snd = (rand()&0x1000)?
							audioDevice->RegisterSound("Sounds/Weapons/SMG/ShellDrop1.wav"):
							audioDevice->RegisterSound("Sounds/Weapons/SMG/ShellDrop2.wav");
							snd2 =
							audioDevice->RegisterSound("Sounds/Weapons/SMG/ShellWater.wav");
							break;
					}
					if(model){
						Vector3 origin;
						origin = muzzle - p->GetFront() * 0.5f;
						
						Vector3 vel;
						vel = p->GetFront() * 0.5f + p->GetRight() +
						p->GetUp() * 0.2f;
						switch(p->GetWeapon()->GetWeaponType()){
							case SMG_WEAPON:
								vel -= p->GetFront() * 0.7f;
								break;
							case SHOTGUN_WEAPON:
								vel *= .5f;
								break;
							default:
								break;
						}
						
						ILocalEntity *ent;
						ent = new GunCasing(this, model, snd, snd2,
											origin, p->GetFront(),
											vel);
						AddLocalEntity(ent);
							
					}
				}
			}
			
			if(!IsMuted()){
				IAudioChunk *c;
				std::string weapPrefix = GetWeaponPrefix("Sounds", p->GetWeapon()->GetWeaponType());
				bool isLocal = p == world->GetLocalPlayer();
				c = isLocal ?
				audioDevice->RegisterSound((weapPrefix + "/FireLocal.wav").c_str()):
				audioDevice->RegisterSound((weapPrefix + "/Fire.wav").c_str());
				
				AudioParam param;
				param.volume = 8.f;
				
				if(isLocal)
					audioDevice->PlayLocal(c,MakeVector3(.4f, -.3f, .5f),
									  param);
				else
					audioDevice->Play(c, p->GetEye() + p->GetFront() * 0.5f
									  - p->GetUp() * .3f
									  + p->GetRight() * .4f,
									  param);
				if(isLocal)
					localFireVibrationTime = time;
				
				// play far sound
				c = audioDevice->RegisterSound((weapPrefix + "/FireFar.wav").c_str());
				param.volume = 1.0f;
				//if(p->GetWeapon()->GetWeaponType() == SMG_WEAPON)
				//	param.volume *= .3f;
				param.referenceDistance = 10.f;
				
				if(isLocal)
					audioDevice->PlayLocal(c,MakeVector3(.4f, -.3f, .5f),
										   param);
				else
					audioDevice->Play(c, p->GetEye() + p->GetFront() * 0.5f
									  - p->GetUp() * .3f
									  + p->GetRight() * .4f,
									  param);
				
				
				c = audioDevice->RegisterSound((weapPrefix + "/FireStereo.wav").c_str());
				if(isLocal)
					audioDevice->PlayLocal(c,MakeVector3(.4f, -.3f, .5f),
										   param);
				else
					audioDevice->Play(c, p->GetEye() + p->GetFront() * 0.5f
									  - p->GetUp() * .3f
									  + p->GetRight() * .4f,
									  param);
				
				if(p->GetWeapon()->GetWeaponType() == SMG_WEAPON){
					switch((rand() >> 8) & 3) {
						case 0:
							c = audioDevice->RegisterSound((weapPrefix + "/Mech1.wav").c_str());
							break;
						case 1:
							c = audioDevice->RegisterSound((weapPrefix + "/Mech2.wav").c_str());
							break;
						case 2:
							c = audioDevice->RegisterSound((weapPrefix + "/Mech3.wav").c_str());
							break;
						case 3:
							c = audioDevice->RegisterSound((weapPrefix + "/Mech4.wav").c_str());
							break;
					}
					param.volume = 1.6f;
					if(isLocal)
						audioDevice->PlayLocal(c,MakeVector3(.4f, -.3f, .5f),
											   param);
					else
						audioDevice->Play(c, p->GetEye() + p->GetFront() * 0.5f
										  - p->GetUp() * .3f
										  + p->GetRight() * .4f,
										  param);
				}
				
			}
		}
		void Client::PlayerDryFiredWeapon(spades::client::Player *p) {
			SPADES_MARK_FUNCTION();
			
			if(!IsMuted()){
				bool isLocal = p == world->GetLocalPlayer();
				IAudioChunk *c = audioDevice->RegisterSound("Sounds/Weapons/DryFire.wav");
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
			
			if(!IsMuted()){
				IAudioChunk *c;
				std::string weapPrefix = GetWeaponPrefix("Sounds", p->GetWeapon()->GetWeaponType());
				bool isLocal = p == world->GetLocalPlayer();
				c = isLocal ?
				audioDevice->RegisterSound((weapPrefix+"/ReloadLocal.wav").c_str()):
				audioDevice->RegisterSound((weapPrefix+"/Reload.wav").c_str());
				
				AudioParam param;
				param.volume = 0.2f;
				
				if(isLocal)
					audioDevice->PlayLocal(c, MakeVector3(.4f, -.3f, .5f),
									  param);
				else
					audioDevice->Play(c, p->GetEye() + p->GetFront() * 0.5f
									  - p->GetUp() * .3f
									  + p->GetRight() * .2f,
									  param);
			}
			
		}
		
		void Client::PlayerReloadedWeapon(spades::client::Player *p){
			SPADES_MARK_FUNCTION();
			
			bool plays = false;
			if(p->GetWeapon()->IsReloadSlow()){
				Weapon *w = p->GetWeapon();
				plays = true;
			}
			
			if(!plays)
				return;
			
			if(!IsMuted()){
				IAudioChunk *c;
				std::string weapPrefix = GetWeaponPrefix("Sounds", p->GetWeapon()->GetWeaponType());
				bool isLocal = p == world->GetLocalPlayer();
				c = isLocal ?
				audioDevice->RegisterSound((weapPrefix+"/CockLocal.wav").c_str()):
				audioDevice->RegisterSound((weapPrefix+"/Cock.wav").c_str());
				
				AudioParam param;
				param.volume = 0.2f;
				
				if(isLocal)
					audioDevice->PlayLocal(c, MakeVector3(.4f, -.3f, .5f),
										   param);
				else
					audioDevice->Play(c, p->GetEye() + p->GetFront() * 0.5f
									  - p->GetUp() * .3f
									  + p->GetRight() * .2f,
									  param);
			}
		}
		
		void Client::PlayerChangedTool(spades::client::Player *p){
			SPADES_MARK_FUNCTION();
			
			if(!IsMuted()){
				bool isLocal = p == world->GetLocalPlayer();
				IAudioChunk *c;
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
				IAudioChunk *c = isLocal ?
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
				IAudioChunk *c = audioDevice->RegisterSound("Sounds/Weapons/Grenade/Throw.wav");
				
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
				IAudioChunk *c = audioDevice->RegisterSound("Sounds/Weapons/Spade/Miss.wav");
				if(isLocal)
					audioDevice->PlayLocal(c, MakeVector3(.2f, -.1f, 0.7f),
										   AudioParam());
				else
					audioDevice->Play(c, p->GetOrigin() + p->GetFront() * 0.8f
									  - p->GetUp() * .2f,
									  AudioParam());
			}
		}
		
		void Client::PlayerHitPlayerWithSpade(spades::client::Player *p){
			SPADES_MARK_FUNCTION();
			SPRaise("Obsolete function called");
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
				IAudioChunk *c = audioDevice->RegisterSound("Sounds/Weapons/Spade/HitBlock.wav");
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
						IAudioChunk *c =
						audioDevice->RegisterSound("Sounds/Weapons/Impacts/Flesh.wav");
						audioDevice->Play(c, victim->GetEye(),
										  AudioParam());
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
				corp = new Corpse(renderer,
								  map,
								  victim);
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
				corp->AddImpulse(victim->GetVelocty());
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
			s = ChatWindow::TeamColorMessage(killer->GetName(),
							     killer->GetTeamId());
			
			std::string cause;
			bool ff = killer->GetTeamId() == victim->GetTeamId();
			if(killer == victim)
				ff = false;
			
			cause = " [";
			switch(kt){
				case KillTypeClassChange:
					cause += "Weapon Change";
					break;
				case KillTypeFall:
					cause += "Fall";
					break;
				case KillTypeGrenade:
					cause += "Grenade";
					break;
				case KillTypeHeadshot:
					cause += "Headshot";
					break;
				case KillTypeMelee:
					cause += "Spade";
					break;
				case KillTypeTeamChange:
					cause += "Team Change";
					break;
				case KillTypeWeapon:
					cause += killer->GetWeapon()->GetName();
					break;
			}
			cause += "] ";
			
			if(ff)
				s += ChatWindow::ColoredMessage(cause, MsgColorRed);
			else
				s += cause;
			
			if(killer != victim){
				s += ChatWindow::TeamColorMessage(victim->GetName(),
									 victim->GetTeamId());
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
				if(killer == world->GetLocalPlayer()){
					char buf[256];
					sprintf(buf, "You have killed %s", victim->GetName().c_str());
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
			   !ShouldRenderInThirdPersonView()){
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
					IAudioChunk *c =
					audioDevice->RegisterSound("Sounds/Weapons/Spade/HitPlayer.wav");
					audioDevice->Play(c, hitPos,
									  AudioParam());
				}else{
					IAudioChunk *c = 
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
				IAudioChunk *c = audioDevice->RegisterSound("Sounds/Weapons/Impacts/Hit.wav");
				audioDevice->Play(c, shiftedHitPos,
								  AudioParam());
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
				
				IAudioChunk *c = 
				audioDevice->RegisterSound("Sounds/Misc/BlockFall.wav");
				audioDevice->Play(c, o,
								  AudioParam());
			}
		}
		
		void Client::GrenadeBounced(spades::client::Grenade *g){
			SPADES_MARK_FUNCTION();
			
			if(g->GetPosition().z < 63.f){
				if(!IsMuted()){
					IAudioChunk *c = audioDevice->RegisterSound("Sounds/Weapons/Grenade/Bounce.wav");
					audioDevice->Play(c, g->GetPosition(),
									  AudioParam());
				}
			}
		}
		
		void Client::GrenadeDroppedIntoWater(spades::client::Grenade *g){
			SPADES_MARK_FUNCTION();
			
			if(!IsMuted()){
				IAudioChunk *c = audioDevice->RegisterSound("Sounds/Weapons/Grenade/DropWater.wav");
				audioDevice->Play(c, g->GetPosition(),
								  AudioParam());
			}
		}
		
		void Client::GrenadeExploded(spades::client::Grenade *g) {
			SPADES_MARK_FUNCTION();
			
			bool inWater = g->GetPosition().z > 63.f;
			
			if(inWater){
				if(!IsMuted()){
					IAudioChunk *c = audioDevice->RegisterSound("Sounds/Weapons/Grenade/WaterExplode.wav");
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
					IAudioChunk *c = audioDevice->RegisterSound("Sounds/Weapons/Grenade/Explode.wav");
					AudioParam param;
					param.volume = 10.f;
					audioDevice->Play(c, g->GetPosition(),
									  param);
					
					c = audioDevice->RegisterSound("Sounds/Weapons/Grenade/ExplodeFar.wav");
					param.volume = 40.f;
					audioDevice->Play(c, g->GetPosition(),
									  param);
					
					
					c = audioDevice->RegisterSound("Sounds/Weapons/Grenade/ExplodeStereo.wav");
					param.volume = 40.f;
					audioDevice->Play(c, g->GetPosition(),
									  param);
					
					// debri sound
					c = audioDevice->RegisterSound("Sounds/Weapons/Grenade/Debris.wav");
					param.volume = 5.f;
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
				IAudioChunk *c = audioDevice->RegisterSound("Sounds/Weapons/Grenade/Fire.wav");
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
