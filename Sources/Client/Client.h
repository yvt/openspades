//
//  Client.h
//  OpenSpades
//
//  Created by yvt on 7/11/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

#pragma once

#include <string>
#include "Player.h"
#include "IWorldListener.h"
#include "../Core/Math.h"
#include "IRenderer.h"
#include <list>

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
		
		class Client: public IWorldListener {
			friend class ScoreboardView;
			friend class LimboView;
			friend class MapView;
			friend class FallingBlock;
			friend class PaletteView;
			friend class TCProgressView;
			
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
			
			World *world;
			GameMap *map;
			GameMapWrapper *mapWrapper;
			IRenderer *renderer;
			IAudioDevice *audioDevice;
			float time;
			bool readyToClose;
			float worldSubFrame;
			
			// other windows
			CenterMessageView *centerMessageView;
			HurtRingView * hurtRingView;
			MapView *mapView;
			ScoreboardView *scoreboard;
			LimboView *limbo;
			PaletteView *paletteView;
			TCProgressView *tcView;
			
			// chat
			ChatWindow *chatWindow;
			ChatWindow *killfeedWindow;
			bool chatEditing;
			bool chatGlobal;
			std::string chatText;
			void ActivateChatTextEditor(bool global);
			void ChatKeyEvent(const std::string& key);
			void ChatCharEvent(const std::string& key);
			void CloseChatTextEditor();
			
			// player state
			PlayerInput playerInput;
			WeaponInput weapInput;
			KeypadInput keypadInput;
			Vector3 lastFront;
			float lastPosSentTime;
			int lastHealth;
			float lastHurtTime;
			float aimDownState;
			float sprintState;
			float lastAliveTime;
			int lastKills;
			float worldSetTime;
			
			// view
			SceneDefinition lastSceneDef;
			Vector3 viewWeaponOffset;
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
			
			Player *HotTrackedPlayer();
			
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
			int corpseSoftLimit;
			int corpseHardLimit;
			
			int nextScreenShotIndex;
			
			std::string GetWeaponPrefix(std::string fold, WeaponType);
			void AddPlayerToScene(Player *);
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
			
			void DrawScene();
			void Draw2D();
			
			std::string ScreenShotPath();
			void TakeScreenShot(bool sceneOnly);
			
			void NetLog(const char *format, ...);
			
		public:
			Client(IRenderer *, IAudioDevice *,
				   std::string host, std::string playerName);
			~Client();
			
			void RunFrame(float dt);
			
			void Closing();
			void MouseEvent(float x, float y);
			void KeyEvent(const std::string&,
						  bool down);
			void CharEvent(const std::string&);
			
			void SetWorld(World *);
			World *GetWorld() const { return world; }
			void AddLocalEntity(ILocalEntity *ent){
				localEntities.push_back(ent);
			}
			
			IRenderer *GetRenderer() const {return renderer;}
			SceneDefinition GetLastSceneDef() const { return lastSceneDef; }
			IAudioDevice *GetAudioDevice() const {return audioDevice; }
			
			bool WantsToBeClosed();
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
			virtual void PlayerHitPlayerWithSpade(Player *);
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
