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

#include <list>
#include <memory>
#include <string>

#include "ClientCameraMode.h"
#include "ILocalEntity.h"
#include "IRenderer.h"
#include "IWorldListener.h"
#include "MumbleLink.h"
#include "NoiseSampler.h"
#include "Player.h"
#include <Core/Math.h>
#include <Core/ServerAddress.h>
#include <Core/Stopwatch.h>
#include <Gui/View.h>

namespace spades {
	class IStream;
	class Stopwatch;
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
		class FontManager;
		class ChatWindow;
		class CenterMessageView;
		class Corpse;
		class HurtRingView;
		class MapView;
		class ScoreboardView;
		class LimboView;
		class Player;
		class PaletteView;
		class TCProgressView;
		class ClientPlayer;

		class ClientUI;

		class Client : public IWorldListener, public gui::View {
			friend class ScoreboardView;
			friend class LimboView;
			friend class MapView;
			friend class FallingBlock;
			friend class PaletteView;
			friend class TCProgressView;
			friend class ClientPlayer;
			friend class ClientUI;

			/** used to keep the input state of keypad so that
			 * after user pressed left and right, and then
			 * released right, left is internally pressed. */
			struct KeypadInput {
				bool left, right, forward, backward;
				KeypadInput() : left(false), right(false), forward(false), backward(false) {}
			};

			class FPSCounter {
				Stopwatch sw;
				int numFrames;
				double lastFps;

			public:
				FPSCounter();
				void MarkFrame();
				double GetFps() { return lastFps; }
			};

			FPSCounter fpsCounter;
			FPSCounter upsCounter;

			std::unique_ptr<NetClient> net;
			std::string playerName;
			std::unique_ptr<IStream> logStream;

			Handle<ClientUI> scriptedUI;

			ServerAddress hostname;

			std::unique_ptr<World> world;
			Handle<GameMap> map;
			std::unique_ptr<GameMapWrapper> mapWrapper;
			Handle<IRenderer> renderer;
			Handle<IAudioDevice> audioDevice;
			float time;
			bool readyToClose;
			float worldSubFrame;

			int frameToRendererInit;
			float timeSinceInit;

			MumbleLink mumbleLink;

			// view/drawing state for some world objects
			std::vector<Handle<ClientPlayer>> clientPlayers;

			// other windows
			std::unique_ptr<CenterMessageView> centerMessageView;
			std::unique_ptr<HurtRingView> hurtRingView;
			std::unique_ptr<MapView> mapView;
			std::unique_ptr<MapView> largeMapView;
			std::unique_ptr<ScoreboardView> scoreboard;
			std::unique_ptr<LimboView> limbo;
			std::unique_ptr<PaletteView> paletteView;
			std::unique_ptr<TCProgressView> tcView;

			// chat
			std::unique_ptr<ChatWindow> chatWindow;
			std::unique_ptr<ChatWindow> killfeedWindow;

			// player state
			PlayerInput playerInput;
			WeaponInput weapInput;
			KeypadInput keypadInput;
			Player::ToolType lastTool;
			bool hasLastTool;
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

			/**
			 * Queries whether the local player is allowed to use a tool in this state.
			 *
			 * The following factors are considered by this function:
			 *
			 *  - The player cannot use a tool while / soon after sprinting.
			 *  - The player cannot use a tool while switching tools.
			 *  - The player must exist and be alive to use a tool.
			 *
			 * The following factors also affect whether a tool can actually be used, but they
			 * do not affect the result of this function:
			 *
			 *  - Tool-specific status — e.g., out of ammo, out of block, "cannot build there"
			 *  - Firing rate limit imposed by the tool
			 */
			bool CanLocalPlayerUseToolNow();

			/** Retrieves `ClientPlayer` for the local player, or `nullptr` if it does not exist. */
			ClientPlayer *GetLocalClientPlayer();

			float toolRaiseState;
			void SetSelectedTool(Player::ToolType, bool quiet = false);

			// view
			SceneDefinition lastSceneDef;
			float localFireVibrationTime;
			float grenadeVibration;
			float grenadeVibrationSlow;
			bool scoreboardVisible;
			bool flashlightOn;
			float flashlightOnTime;
			CoherentNoiseSampler1D coherentNoiseSamplers[3];
			void KickCamera(float strength);

			float hitFeedbackIconState;
			bool hitFeedbackFriendly;

			// manual focus
			float focalLength;
			float targetFocalLength;
			bool autoFocusEnabled;

			// Spectator camera control
			/** The state of the following camera used for spectating. */
			struct {
				/**
				 * Indicates whether the current camera mode is first-person or not.
				 * Ignored and locked to third-person when the target player
				 * (`followedPlayerId`) is dead.
				 */
				bool firstPerson = true;

				/** Controls whether the follow camera is enabled. */
				bool enabled = false;
			} followCameraState;

			/** The state of the free floating camera used for spectating. */
			struct {
				/** The temporally smoothed position (I guess). */
				Vector3 position{0.0f, 0.0f, 0.0f};

				/** The temporally smoothed velocity (I guess). */
				Vector3 velocity{0.0f, 0.0f, 0.0f};
			} freeCameraState;

			/**
			 * The state shared by both of the third-person and free-floating cameras.
			 *
			 * Note: These values are not used in the `cg_thirdperson` mode.
			 */
			struct {
				/** The yaw angle. */
				float yaw = 0.0f;

				/** The pitch angle. */
				float pitch = 0.0f;
			} followAndFreeCameraState;

			/**
			 * The ID of the player being followed (in a spectator mode, or when the local player is
			 * dead). Must be valid as long as the follow cam is enabled.
			 *
			 * Must *not* specify a local player.
			 */
			int followedPlayerId;

			/**
			 * Chooses the next player to follow and assigns it to `this->followingPlayerId`.
			 * Enables the follow cam by assigning `true` to `followCameraState.enabled`.
			 * If the next player is the local player, disables the follow cam.
			 */
			void FollowNextPlayer(bool reverse);

			/**
			 * Retrieves the current camera mode.
			 */
			ClientCameraMode GetCameraMode();

			/**
			 * Retrieves the target player ID of the current camera mode (as returned by
			 * `GetCameraMode`).
			 *
			 * Throws an exception if the current camera mode does not have a player in concern.
			 */
			int GetCameraTargetPlayerId();

			/**
			 * Retrieves the target player of the current camera mode (as returned by
			 * `GetCameraMode`).
			 *
			 * Throws an exception if the current camera mode does not have a player in concern.
			 */
			Player &GetCameraTargetPlayer();

			/**
			 * Calculate the zoom value incorporating the effect of ADS for a first-person view.
			 *
			 * The camera mode must be first-person.
			 */
			float GetAimDownZoomScale();

			bool inGameLimbo;

			float GetLocalFireVibration();
			void CaptureColor();
			bool IsLimboViewActive();
			void SpawnPressed();

			Player *HotTrackedPlayer(hitTag_t *hitFlag);

			// effects (local entity, etc)
			std::vector<DynamicLightParam> flashDlights;
			std::vector<DynamicLightParam> flashDlightsOld;
			void Bleed(Vector3);
			void EmitBlockFragments(Vector3, IntVector3 color);
			void EmitBlockDestroyFragments(IntVector3, IntVector3 color);
			void GrenadeExplosion(Vector3);
			void GrenadeExplosionUnderwater(Vector3);
			void MuzzleFire(Vector3, Vector3 dir, bool local);
			void BulletHitWaterSurface(Vector3);

			// drawings
			Handle<FontManager> fontManager;

			enum class AlertType { Notice, Warning, Error };
			AlertType alertType;
			std::string alertContents;
			float alertDisappearTime;
			float alertAppearTime;

			// Loading screen
			float mapReceivingProgressSmoothed = 0.0;

			std::list<std::unique_ptr<ILocalEntity>> localEntities;
			std::list<std::unique_ptr<Corpse>> corpses;
			Corpse *lastMyCorpse;
			float corpseSoftTimeLimit;
			unsigned int corpseSoftLimit;
			unsigned int corpseHardLimit;
			void RemoveAllCorpses();
			void RemoveInvisibleCorpses();
			void RemoveAllLocalEntities();
			void RemoveCorpseForPlayer(int playerId);

			int nextScreenShotIndex;
			int nextMapShotIndex;

			Vector3 Project(Vector3);

			void DrawSplash();
			void DrawStartupScreen();
			void DrawDisconnectScreen();
			void DoInit();

			void ShowAlert(const std::string &contents, AlertType type);
			void ShowAlert(const std::string &contents, AlertType type, float timeout,
			               bool quiet = false);
			void PlayAlertSound();

			void UpdateWorld(float dt);
			void UpdateLocalSpectator(float dt);
			void UpdateLocalPlayer(float dt);
			void UpdateAutoFocus(float dt);
			float RayCastForAutoFocus(const Vector3 &origin, const Vector3 &direction);

			void Draw2D();

			void Draw2DWithoutWorld();
			void Draw2DWithWorld();

			/** Called when the local plyaer is alive. */
			void DrawJoinedAlivePlayerHUD();
			/** Called when the local plyaer is dead. */
			void DrawDeadPlayerHUD();

			/**
			 * Called when `IsFirstPerson(GetCameraMode()).` Renders the follwing element:
			 *  - The center reticule
			 */
			void DrawFirstPersonHUD();

			/**
			 * Called when the local player is dead or a spectator.
			 */
			void DrawSpectateHUD();

			void DrawHottrackedPlayerName();
			void DrawHurtScreenEffect();
			void DrawHurtSprites();
			void DrawHealth();
			void DrawAlert();
			void DrawDebugAim();
			void DrawStats();

			void DrawScene();
			void AddGrenadeToScene(Grenade *);
			void AddDebugObjectToScene(const OBB3 &, const Vector4 &col = MakeVector4(1, 1, 1, 1));
			void DrawCTFObjects();
			void DrawTCObjects();

			SceneDefinition CreateSceneDefinition();

			std::string ScreenShotPath();
			void TakeScreenShot(bool sceneOnly);

			std::string MapShotPath();
			void TakeMapShot();

			void NetLog(const char *format, ...);

			static Client *globalInstance;

		protected:
			~Client();

		public:
			Client(Handle<IRenderer>, Handle<IAudioDevice>, const ServerAddress &host,
			       Handle<FontManager>);

			void RunFrame(float dt) override;
			void RunFrameLate(float dt) override;

			void Closing() override;
			void MouseEvent(float x, float y) override;
			void WheelEvent(float x, float y) override;
			void KeyEvent(const std::string &, bool down) override;
			void TextInputEvent(const std::string &) override;
			void TextEditingEvent(const std::string &, int start, int len) override;
			bool AcceptsTextInput() override;
			AABB2 GetTextInputRect() override;
			bool NeedsAbsoluteMouseCoordinate() override;
			bool ExecCommand(const Handle<gui::ConsoleCommand> &) override;
			Handle<gui::ConsoleCommandCandidateIterator>
			AutocompleteCommandName(const std::string &name) override;

			void SetWorld(World *);
			World *GetWorld() const { return world.get(); }
			void AddLocalEntity(ILocalEntity *ent) { localEntities.emplace_back(ent); }

			void MarkWorldUpdate();

			IRenderer *GetRenderer() { return renderer; }
			SceneDefinition GetLastSceneDef() { return lastSceneDef; }
			IAudioDevice *GetAudioDevice() { return audioDevice; }

			bool WantsToBeClosed() override;
			bool IsMuted();

			void PlayerSentChatMessage(Player *, bool global, const std::string &);
			void ServerSentMessage(const std::string &);

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
			void PlayerSpawned(Player *);

			void PlayerObjectSet(int) override;
			void PlayerMadeFootstep(Player *) override;
			void PlayerJumped(Player *) override;
			void PlayerLanded(Player *, bool hurt) override;
			void PlayerFiredWeapon(Player *) override;
			void PlayerDryFiredWeapon(Player *) override;
			void PlayerReloadingWeapon(Player *) override;
			void PlayerReloadedWeapon(Player *) override;
			void PlayerChangedTool(Player *) override;
			void PlayerThrownGrenade(Player *, Grenade *) override;
			void PlayerMissedSpade(Player *) override;
			void PlayerRestocked(Player *) override;

			/** @deprecated use BulletHitPlayer */
			void PlayerHitBlockWithSpade(Player *, Vector3 hitPos, IntVector3 blockPos,
			                             IntVector3 normal) override;
			void PlayerKilledPlayer(Player *killer, Player *victim, KillType) override;

			void BulletHitPlayer(Player *hurtPlayer, HitType, Vector3 hitPos, Player *by) override;
			void BulletHitBlock(Vector3, IntVector3 blockPos, IntVector3 normal) override;
			void AddBulletTracer(Player *player, Vector3 muzzlePos, Vector3 hitPos) override;
			void GrenadeExploded(Grenade *) override;
			void GrenadeBounced(Grenade *) override;
			void GrenadeDroppedIntoWater(Grenade *) override;

			void BlocksFell(std::vector<IntVector3>) override;

			void LocalPlayerPulledGrenadePin() override;
			void LocalPlayerBlockAction(IntVector3, BlockActionType type) override;
			void LocalPlayerCreatedLineBlock(IntVector3, IntVector3) override;
			void LocalPlayerHurt(HurtType type, bool sourceGiven, Vector3 source) override;
			void LocalPlayerBuildError(BuildFailureReason reason) override;

			static bool AreCheatsEnabled(); // 'cheats', i.e. spectator wallhack or player names
			static bool WallhackActive();
			static Vector3 TeamCol(unsigned int teamId);
		};
	} // namespace client
} // namespace spades
