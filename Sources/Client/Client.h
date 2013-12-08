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

#pragma once

#include <string>
#include "Player.h"
#include "IWorldListener.h"
#include <Core/Math.h>
#include <Core/ServerAddress.h>
#include "IRenderer.h"
#include <list>
#include <Gui/View.h>

namespace spades {
	class IStream;
	namespace client {
		class IRenderer;
		struct SceneDefinition;
		class GameMap;
		class GameMapWrapper;
		class World;
		struct PlayerInput;
		struct WeaponInput;
		class IAudioDevice;
		class IAudioChunk;
		class NetClient;
		class IFont;
		class ChatWindow;
		class CenterMessageView;
		class Corpse;
		class HurtRingView;
		class MapView;
		class ScoreboardView;
		class LimboView;
		class ILocalEntity;
		class Player;
		class PaletteView;
		class TCProgressView;
		class ClientPlayer;
		
		class ClientUI;
		
		class Client: public IWorldListener, public gui::View {
			friend class ScoreboardView;
			friend class LimboView;
			friend class MapView;
			friend class FallingBlock;
			friend class PaletteView;
			friend class TCProgressView;
			friend class ClientPlayer;
			friend class ClientUI;
			
			/** used to keep the input state of keypad so that
			 * after user user pressed left and right, and 
			 * released right, left is internally pressed. */
			struct KeypadInput {
				bool left, right, forward, backward;
				KeypadInput():
				left(false),right(false),
				forward(false),backward(false){
				}
			};
			
			NetClient *net;
			std::string playerName;
			IStream *logStream;
			
			Handle<ClientUI> scriptedUI;
			
			ServerAddress hostname;
			
			World *world;
			GameMap *map;
			GameMapWrapper *mapWrapper;
			IRenderer *renderer;
			IAudioDevice *audioDevice;
			float time;
			bool readyToClose;
			float worldSubFrame;
			
			int frameToRendererInit;
			float timeSinceInit;
			
			// view/drawing state for some world objects
			std::vector<ClientPlayer *> clientPlayers;
			
			// other windows
			CenterMessageView *centerMessageView;
			HurtRingView * hurtRingView;
			MapView *mapView;
			MapView *largeMapView;
			ScoreboardView *scoreboard;
			LimboView *limbo;
			PaletteView *paletteView;
			TCProgressView *tcView;
			
			// chat
			ChatWindow *chatWindow;
			ChatWindow *killfeedWindow;
			
			// player state
			PlayerInput playerInput;
			WeaponInput weapInput;
			KeypadInput keypadInput;
			Vector3 lastFront;
			float lastPosSentTime;
			int lastHealth;
			float lastHurtTime;
			float lastAliveTime;
			int lastKills;
			float worldSetTime;
            bool hasDelayedReload;
			struct HurtSprite {
				float angle;
				float horzShift;
				float scale;
				float strength;
			};
			std::vector<HurtSprite> hurtSprites;
			float GetAimDownState();
			float GetSprintState();
			
			float toolRaiseState;
			void SetSelectedTool(Player::ToolType, bool quiet = false);
			
			// view
			SceneDefinition lastSceneDef;
			float localFireVibrationTime;
			float grenadeVibration;
			bool scoreboardVisible;
			bool flashlightOn;
			float flashlightOnTime;
			
			// when dead...
			/** Following player ID, which may be local player itself */
			int followingPlayerId;
			float followYaw, followPitch;
			/** only for when spectating */
			Vector3 followPos;
			/** only for when spectating */
			Vector3 followVel;
			void FollowNextPlayer();
			/** @return true following is activated (and followingPlayerId should be used) */
			bool IsFollowing();
			
			bool inGameLimbo;

			float GetLocalFireVibration();
			void CaptureColor();
			bool IsLimboViewActive();
			void SpawnPressed();
			
			Player *HotTrackedPlayer( hitTag_t* hitFlag );
			
			// effects (local entity, etc)
			std::vector<DynamicLightParam> flashDlights;
			std::vector<DynamicLightParam> flashDlightsOld;
			void Bleed(Vector3);
			void EmitBlockFragments(Vector3, IntVector3 color);
			void EmitBlockDestroyFragments(IntVector3, IntVector3 color);
			void GrenadeExplosion(Vector3);
			void GrenadeExplosionUnderwater(Vector3);
			void MuzzleFire(Vector3, Vector3 dir, bool local);
			
			// drawings
			IFont *designFont;
			IFont *textFont;
			IFont *bigTextFont;
			
			std::list<ILocalEntity *> localEntities;
			std::list<Corpse *> corpses;
			Corpse *lastMyCorpse;
			float corpseSoftTimeLimit;
			unsigned int corpseSoftLimit;
			unsigned int corpseHardLimit;
			
			int nextScreenShotIndex;
			int nextMapShotIndex;
			
			std::string GetWeaponPrefix(std::string fold, WeaponType);
			void AddGrenadeToScene(Grenade *);
			void AddDebugObjectToScene(const OBB3&,
									   const Vector4& col = MakeVector4(1,1,1,1));
			void DrawCTFObjects();
			void DrawTCObjects();
			SceneDefinition SceneDef();
			bool ShouldRenderInThirdPersonView();
			void RemoveAllCorpses();
			void RemoveInvisibleCorpses();
			float GetAimDownZoomScale();
			void RemoveAllLocalEntities();
			Vector3 Project(Vector3);
			
			void DrawSplash();
			void DrawStartupScreen();
			void DoInit();
			
			void DrawScene();
			void Draw2D();
			void DrawHurtSprites();
			
			std::string ScreenShotPath();
			void TakeScreenShot(bool sceneOnly);
			
			std::string MapShotPath();
			void TakeMapShot();
			
			void NetLog(const char *format, ...);
		protected:
			virtual ~Client();
			
		public:
			Client(IRenderer *, IAudioDevice *,
				   const ServerAddress& host, std::string playerName);
			
			virtual void RunFrame(float dt);
			
			virtual void Closing();
			virtual void MouseEvent(float x, float y);
			virtual void KeyEvent(const std::string&,
						  bool down);
			virtual void CharEvent(const std::string&);
			
			void SetWorld(World *);
			World *GetWorld() const { return world; }
			void AddLocalEntity(ILocalEntity *ent){
				localEntities.push_back(ent);
			}
			
			IRenderer *GetRenderer() const {return renderer;}
			SceneDefinition GetLastSceneDef() const { return lastSceneDef; }
			IAudioDevice *GetAudioDevice() const {return audioDevice; }
			
			virtual bool WantsToBeClosed();
			bool IsMuted();
			
			void PlayerSentChatMessage(Player *, bool global,
									   const std::string&);
			void ServerSentMessage(const std::string&);
			
			void PlayerCapturedIntel(Player *);
			void PlayerCreatedBlock(Player *);
			void PlayerPickedIntel(Player *);
			void PlayerDropIntel(Player *);
			void TeamCapturedTerritory(int teamId, int territoryId);
			void TeamWon(int);
			void JoinedGame();
			void LocalPlayerCreated();
			void PlayerDestroyedBlockWithWeaponOrTool(IntVector3);
			void PlayerDiggedBlock(IntVector3);
			void GrenadeDestroyedBlock(IntVector3);
			void PlayerLeaving(Player *);
			void PlayerJoinedTeam(Player *);
			
			virtual void PlayerObjectSet(int);
			virtual void PlayerMadeFootstep(Player *);
			virtual void PlayerJumped(Player *);
			virtual void PlayerLanded(Player *, bool hurt);
			virtual void PlayerFiredWeapon(Player *);
			virtual void PlayerDryFiredWeapon(Player *);
			virtual void PlayerReloadingWeapon(Player *);
			virtual void PlayerReloadedWeapon(Player *);
			virtual void PlayerChangedTool(Player *);
			virtual void PlayerThrownGrenade(Player *, Grenade *);
			virtual void PlayerMissedSpade(Player *);
			virtual void PlayerRestocked(Player *);
			
			/** @deprecated use BulletHitPlayer */
			virtual void PlayerHitBlockWithSpade(Player *,
												 Vector3 hitPos,
												 IntVector3 blockPos,
												 IntVector3 normal);
			virtual void PlayerKilledPlayer(Player *killer,
											Player *victim,
											KillType);
			
			virtual void BulletHitPlayer(Player *hurtPlayer, HitType,
										 Vector3 hitPos,
										 Player *by);
			virtual void BulletHitBlock(Vector3,
										IntVector3 blockPos,
										IntVector3 normal);
			virtual void AddBulletTracer(Player *player,
										 Vector3 muzzlePos,
										 Vector3 hitPos);
			virtual void GrenadeExploded(Grenade *);
			virtual void GrenadeBounced(Grenade *);
			virtual void GrenadeDroppedIntoWater(Grenade *);
			
			virtual void BlocksFell(std::vector<IntVector3>);
			
			virtual void LocalPlayerPulledGrenadePin();
			virtual void LocalPlayerBlockAction(IntVector3, BlockActionType type);
			virtual void LocalPlayerCreatedLineBlock(IntVector3, IntVector3);
			virtual void LocalPlayerHurt(HurtType type, bool sourceGiven,
										 Vector3 source);
		};
	}
}
