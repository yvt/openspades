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
#include <array>
#include <cmath>
#include <cstdlib>

#include "CTFGameMode.h"
#include "Client.h"
#include "ClientPlayer.h"
#include "GameMap.h"
#include "GunCasing.h"
#include "IAudioChunk.h"
#include "IAudioDevice.h"
#include "IImage.h"
#include "IModel.h"
#include "IRenderer.h"
#include "Player.h"
#include "Weapon.h"
#include "World.h"
#include <Core/Settings.h>
#include <ScriptBindings/IBlockSkin.h>
#include <ScriptBindings/IGrenadeSkin.h>
#include <ScriptBindings/ISpadeSkin.h>
#include <ScriptBindings/IThirdPersonToolSkin.h>
#include <ScriptBindings/IToolSkin.h>
#include <ScriptBindings/IViewToolSkin.h>
#include <ScriptBindings/IWeaponSkin.h>
#include <ScriptBindings/ScriptFunction.h>

SPADES_SETTING(cg_ragdoll);
SPADES_SETTING(cg_ejectBrass);
DEFINE_SPADES_SETTING(cg_animations, "1");
SPADES_SETTING(cg_shake);
DEFINE_SPADES_SETTING(cg_environmentalAudio, "1");
DEFINE_SPADES_SETTING(cg_viewWeaponX, "0");
DEFINE_SPADES_SETTING(cg_viewWeaponY, "0");
DEFINE_SPADES_SETTING(cg_viewWeaponZ, "0");

namespace spades {
	namespace client {

		class SandboxedRenderer : public IRenderer {
			Handle<IRenderer> base;
			AABB3 clipBox;
			bool allowDepthHack;

			void OnProhibitedAction() {}

			bool CheckVisibility(const AABB3 &box) {
				if (!clipBox.Contains(box) || !std::isfinite(box.min.x) ||
				    !std::isfinite(box.min.y) || !std::isfinite(box.min.z) ||
				    !std::isfinite(box.max.x) || !std::isfinite(box.max.y) ||
				    !std::isfinite(box.max.z)) {
					OnProhibitedAction();
					return false;
				}
				return true;
			}

		protected:
			~SandboxedRenderer() {}

		public:
			SandboxedRenderer(IRenderer *base) : base(base) {}

			void SetClipBox(const AABB3 &b) { clipBox = b; }
			void SetAllowDepthHack(bool h) { allowDepthHack = h; }

			void Init() { OnProhibitedAction(); }
			void Shutdown() { OnProhibitedAction(); }

			IImage *RegisterImage(const char *filename) { return base->RegisterImage(filename); }
			IModel *RegisterModel(const char *filename) { return base->RegisterModel(filename); }

			IImage *CreateImage(Bitmap *bmp) { return base->CreateImage(bmp); }
			IModel *CreateModel(VoxelModel *m) { return base->CreateModel(m); }

			void SetGameMap(GameMap *) { OnProhibitedAction(); }

			void SetFogDistance(float) { OnProhibitedAction(); }
			void SetFogColor(Vector3) { OnProhibitedAction(); }

			void StartScene(const SceneDefinition &) { OnProhibitedAction(); }

			void AddLight(const client::DynamicLightParam &light) {
				Vector3 rad(light.radius, light.radius, light.radius);
				if (CheckVisibility(AABB3(light.origin - rad, light.origin + rad))) {
					base->AddLight(light);
				}
			}

			void RenderModel(IModel *model, const ModelRenderParam &_p) {
				ModelRenderParam p = _p;

				if (!model) {
					SPInvalidArgument("model");
					return;
				}

				if (p.depthHack && !allowDepthHack) {
					OnProhibitedAction();
					return;
				}

				// Overbright surfaces bypass the fog
				p.customColor.x = std::max(std::min(p.customColor.x, 1.0f), 0.0f);
				p.customColor.y = std::max(std::min(p.customColor.y, 1.0f), 0.0f);
				p.customColor.z = std::max(std::min(p.customColor.z, 1.0f), 0.0f);

				// NaN values bypass the fog
				if (std::isnan(p.customColor.x) || std::isnan(p.customColor.y) ||
				    std::isnan(p.customColor.z)) {
					OnProhibitedAction();
					return;
				}

				auto bounds = (p.matrix * OBB3(model->GetBoundingBox())).GetBoundingAABB();
				if (CheckVisibility(bounds)) {
					base->RenderModel(model, p);
				}
			}
			void AddDebugLine(Vector3 a, Vector3 b, Vector4 color) { OnProhibitedAction(); }

			void AddSprite(IImage *image, Vector3 center, float radius, float rotation) {
				Vector3 rad(radius * 1.5f, radius * 1.5f, radius * 1.5f);
				if (CheckVisibility(AABB3(center - rad, center + rad))) {
					base->AddSprite(image, center, radius, rotation);
				}
			}
			void AddLongSprite(IImage *image, Vector3 p1, Vector3 p2, float radius) {
				Vector3 rad(radius * 1.5f, radius * 1.5f, radius * 1.5f);
				AABB3 bounds1(p1 - rad, p1 + rad);
				AABB3 bounds2(p2 - rad, p2 + rad);
				bounds1 += bounds2;
				if (CheckVisibility(bounds1)) {
					base->AddLongSprite(image, p1, p2, radius);
				}
			}

			void EndScene() { OnProhibitedAction(); }

			void MultiplyScreenColor(Vector3) { OnProhibitedAction(); }

			/** Sets color for image drawing. Deprecated because
			 * some methods treats this as an alpha premultiplied, while
			 * others treats this as an alpha non-premultiplied.
			 * @deprecated */
			void SetColor(Vector4 col) { base->SetColor(col); }

			/** Sets color for image drawing. Always alpha premultiplied. */
			void SetColorAlphaPremultiplied(Vector4 col) { base->SetColorAlphaPremultiplied(col); }

			void DrawImage(IImage *img, const Vector2 &outTopLeft) {
				if (allowDepthHack)
					base->DrawImage(img, outTopLeft);
				else
					OnProhibitedAction();
			}
			void DrawImage(IImage *img, const AABB2 &outRect) {
				if (allowDepthHack)
					base->DrawImage(img, outRect);
				else
					OnProhibitedAction();
			}
			void DrawImage(IImage *img, const Vector2 &outTopLeft, const AABB2 &inRect) {
				if (allowDepthHack)
					base->DrawImage(img, outTopLeft, inRect);
				else
					OnProhibitedAction();
			}
			void DrawImage(IImage *img, const AABB2 &outRect, const AABB2 &inRect) {
				if (allowDepthHack)
					base->DrawImage(img, outRect, inRect);
				else
					OnProhibitedAction();
			}
			void DrawImage(IImage *img, const Vector2 &outTopLeft, const Vector2 &outTopRight,
			               const Vector2 &outBottomLeft, const AABB2 &inRect) {
				if (allowDepthHack)
					base->DrawImage(img, outTopLeft, outTopRight, outBottomLeft, inRect);
				else
					OnProhibitedAction();
			}

			void DrawFlatGameMap(const AABB2 &outRect, const AABB2 &inRect) {
				OnProhibitedAction();
			}

			void FrameDone() { OnProhibitedAction(); }

			void Flip() { OnProhibitedAction(); }

			Bitmap *ReadBitmap() {
				OnProhibitedAction();
				return nullptr;
			}

			float ScreenWidth() { return base->ScreenWidth(); }
			float ScreenHeight() { return base->ScreenHeight(); }
		};

		ClientPlayer::ClientPlayer(Player *p, Client *c) : client(c), player(p), hasValidOriginMatrix(false) {
			SPADES_MARK_FUNCTION();

			sprintState = 0.f;
			aimDownState = 0.f;
			toolRaiseState = 0.f;
			currentTool = p->GetTool();
			localFireVibrationTime = -100.f;
			time = 0.f;
			viewWeaponOffset = MakeVector3(0, 0, 0);
			lastFront = MakeVector3(0, 0, 0);
			flashlightOrientation = p->GetFront();

			ScriptContextHandle ctx;
			IRenderer *renderer = client->GetRenderer();
			IAudioDevice *audio = client->GetAudioDevice();

			sandboxedRenderer.Set(new SandboxedRenderer(renderer), false);
			renderer = sandboxedRenderer;

			static ScriptFunction spadeFactory(
			  "ISpadeSkin@ CreateThirdPersonSpadeSkin(Renderer@, AudioDevice@)");
			spadeSkin = initScriptFactory(spadeFactory, renderer, audio);

			static ScriptFunction spadeViewFactory(
			  "ISpadeSkin@ CreateViewSpadeSkin(Renderer@, AudioDevice@)");
			spadeViewSkin = initScriptFactory(spadeViewFactory, renderer, audio);

			static ScriptFunction blockFactory(
			  "IBlockSkin@ CreateThirdPersonBlockSkin(Renderer@, AudioDevice@)");
			blockSkin = initScriptFactory(blockFactory, renderer, audio);

			static ScriptFunction blockViewFactory(
			  "IBlockSkin@ CreateViewBlockSkin(Renderer@, AudioDevice@)");
			blockViewSkin = initScriptFactory(blockViewFactory, renderer, audio);

			static ScriptFunction grenadeFactory(
			  "IGrenadeSkin@ CreateThirdPersonGrenadeSkin(Renderer@, AudioDevice@)");
			grenadeSkin = initScriptFactory(grenadeFactory, renderer, audio);

			static ScriptFunction grenadeViewFactory(
			  "IGrenadeSkin@ CreateViewGrenadeSkin(Renderer@, AudioDevice@)");
			grenadeViewSkin = initScriptFactory(grenadeViewFactory, renderer, audio);

			static ScriptFunction rifleFactory(
			  "IWeaponSkin@ CreateThirdPersonRifleSkin(Renderer@, AudioDevice@)");
			static ScriptFunction smgFactory(
			  "IWeaponSkin@ CreateThirdPersonSMGSkin(Renderer@, AudioDevice@)");
			static ScriptFunction shotgunFactory(
			  "IWeaponSkin@ CreateThirdPersonShotgunSkin(Renderer@, AudioDevice@)");
			static ScriptFunction rifleViewFactory(
			  "IWeaponSkin@ CreateViewRifleSkin(Renderer@, AudioDevice@)");
			static ScriptFunction smgViewFactory(
			  "IWeaponSkin@ CreateViewSMGSkin(Renderer@, AudioDevice@)");
			static ScriptFunction shotgunViewFactory(
			  "IWeaponSkin@ CreateViewShotgunSkin(Renderer@, AudioDevice@)");
			switch (p->GetWeapon()->GetWeaponType()) {
				case RIFLE_WEAPON:
					weaponSkin = initScriptFactory(rifleFactory, renderer, audio);
					weaponViewSkin = initScriptFactory(rifleViewFactory, renderer, audio);
					break;
				case SMG_WEAPON:
					weaponSkin = initScriptFactory(smgFactory, renderer, audio);
					weaponViewSkin = initScriptFactory(smgViewFactory, renderer, audio);
					break;
				case SHOTGUN_WEAPON:
					weaponSkin = initScriptFactory(shotgunFactory, renderer, audio);
					weaponViewSkin = initScriptFactory(shotgunViewFactory, renderer, audio);
					break;
				default: SPAssert(false);
			}
		}
		ClientPlayer::~ClientPlayer() {
			spadeSkin->Release();
			blockSkin->Release();
			weaponSkin->Release();
			grenadeSkin->Release();

			spadeViewSkin->Release();
			blockViewSkin->Release();
			weaponViewSkin->Release();
			grenadeViewSkin->Release();
		}

		asIScriptObject *ClientPlayer::initScriptFactory(ScriptFunction &creator,
		                                                 IRenderer *renderer, IAudioDevice *audio) {
			ScriptContextHandle ctx = creator.Prepare();
			ctx->SetArgObject(0, reinterpret_cast<void *>(renderer));
			ctx->SetArgObject(1, reinterpret_cast<void *>(audio));
			ctx.ExecuteChecked();
			asIScriptObject *result = reinterpret_cast<asIScriptObject *>(ctx->GetReturnObject());
			result->AddRef();
			return result;
		}

		void ClientPlayer::Invalidate() { player = NULL; }

		bool ClientPlayer::IsChangingTool() {
			return currentTool != player->GetTool() || toolRaiseState < .999f;
		}

		float ClientPlayer::GetLocalFireVibration() {
			float localFireVibration = 0.f;
			localFireVibration = time - localFireVibrationTime;
			localFireVibration = 1.f - localFireVibration / 0.1f;
			if (localFireVibration < 0.f)
				localFireVibration = 0.f;
			return localFireVibration;
		}

		void ClientPlayer::Update(float dt) {
			time += dt;

			PlayerInput actualInput = player->GetInput();
			WeaponInput actualWeapInput = player->GetWeaponInput();
			if (actualInput.sprint && player->IsAlive()) {
				sprintState += dt * 4.f;
				if (sprintState > 1.f)
					sprintState = 1.f;
			} else {
				sprintState -= dt * 3.f;
				if (sprintState < 0.f)
					sprintState = 0.f;
			}

			if (actualWeapInput.secondary && player->IsToolWeapon() && player->IsAlive()) {
				// This is the only animation that can be turned off
				// here; others affect the gameplay directly and
				// turning them off would be considered cheating
				if (cg_animations) {
					aimDownState += dt * 8.f;
					if (aimDownState > 1.f)
						aimDownState = 1.f;
				} else {
					aimDownState = 1.f;
				}
			} else {
				if (cg_animations) {
					aimDownState -= dt * 3.f;
					if (aimDownState < 0.f)
						aimDownState = 0.f;
				} else {
					aimDownState = 0.f;
				}
			}

			if (currentTool == player->GetTool()) {
				toolRaiseState += dt * 4.f;
				if (toolRaiseState > 1.f)
					toolRaiseState = 1.f;
				if (toolRaiseState < 0.f)
					toolRaiseState = 0.f;
			} else {
				toolRaiseState -= dt * 4.f;
				if (toolRaiseState < 0.f) {
					toolRaiseState = 0.f;
					currentTool = player->GetTool();

					// play tool change sound
					if (player->IsLocalPlayer()) {
						auto *audioDevice = client->GetAudioDevice();
						Handle<IAudioChunk> c;
						switch (player->GetTool()) {
							case Player::ToolSpade:
								c = audioDevice->RegisterSound(
								  "Sounds/Weapons/Spade/RaiseLocal.opus");
								break;
							case Player::ToolBlock:
								c = audioDevice->RegisterSound(
								  "Sounds/Weapons/Block/RaiseLocal.opus");
								break;
							case Player::ToolWeapon:
								switch (player->GetWeapon()->GetWeaponType()) {
									case RIFLE_WEAPON:
										c = audioDevice->RegisterSound(
										  "Sounds/Weapons/Rifle/RaiseLocal.opus");
										break;
									case SMG_WEAPON:
										c = audioDevice->RegisterSound(
										  "Sounds/Weapons/SMG/RaiseLocal.opus");
										break;
									case SHOTGUN_WEAPON:
										c = audioDevice->RegisterSound(
										  "Sounds/Weapons/Shotgun/RaiseLocal.opus");
										break;
								}

								break;
							case Player::ToolGrenade:
								c = audioDevice->RegisterSound(
								  "Sounds/Weapons/Grenade/RaiseLocal.opus");
								break;
						}
						audioDevice->PlayLocal(c, MakeVector3(.4f, -.3f, .5f), AudioParam());
					}
				} else if (toolRaiseState > 1.f) {
					toolRaiseState = 1.f;
				}
			}

			{
				float scale = dt;
				Vector3 vel = player->GetVelocity();
				Vector3 front = player->GetFront();
				Vector3 right = player->GetRight();
				Vector3 up = player->GetUp();

				// Offset the view weapon according to the player movement
				viewWeaponOffset.x += Vector3::Dot(vel, right) * scale;
				viewWeaponOffset.y -= Vector3::Dot(vel, front) * scale;
				viewWeaponOffset.z += Vector3::Dot(vel, up) * scale;

				// Offset the view weapon according to the camera movement
				Vector3 diff = front - lastFront;
				viewWeaponOffset.x += Vector3::Dot(diff, right) * 0.05f;
				viewWeaponOffset.z += Vector3::Dot(diff, up) * 0.05f;

				lastFront = front;

				if (dt > 0.f)
					viewWeaponOffset *= powf(.02f, dt);

				// Limit the movement
				auto softLimitFunc = [&](float &v, float minLimit, float maxLimit) {
					float transition = (maxLimit - minLimit) * 0.5f;
					if (v < minLimit) {
						float strength = std::min(1.f, (minLimit - v) / transition);
						v = Mix(v, minLimit, strength);
					}
					if (v > maxLimit) {
						float strength = std::min(1.f, (v - maxLimit) / transition);
						v = Mix(v, maxLimit, strength);
					}
				};
				softLimitFunc(viewWeaponOffset.x, -0.06f, 0.06f);
				softLimitFunc(viewWeaponOffset.y, -0.06f, 0.06f);
				softLimitFunc(viewWeaponOffset.z, -0.06f, 0.06f);

				// When the player is aiming down the sight, the weapon's movement
				// must be restricted so that other parts of the weapon don't
				// cover the ironsight.
				if (currentTool == Player::ToolWeapon && player->GetWeaponInput().secondary) {

					if (dt > 0.f)
						viewWeaponOffset *= powf(.01f, dt);

					const float limitX = .003f;
					const float limitY = .003f;
					softLimitFunc(viewWeaponOffset.x, -limitX, limitX);
					softLimitFunc(viewWeaponOffset.z, 0, limitY);
				}
			}

			{
				// Smooth the flashlight's movement
				Vector3 diff = player->GetFront() - flashlightOrientation;
				float sq = diff.GetLength();
				if (sq > 0.1) {
					flashlightOrientation += diff.Normalize() * (sq - 0.1);
				}

				flashlightOrientation =
				  Mix(flashlightOrientation, player->GetFront(), 1.0f - powf(1.0e-6, dt))
				    .Normalize();
			}

			// FIXME: should do for non-active skins?
			asIScriptObject *skin;
			if (ShouldRenderInThirdPersonView()) {
				if (currentTool == Player::ToolSpade) {
					skin = spadeSkin;
				} else if (currentTool == Player::ToolBlock) {
					skin = blockSkin;
				} else if (currentTool == Player::ToolGrenade) {
					skin = grenadeSkin;
				} else if (currentTool == Player::ToolWeapon) {
					skin = weaponSkin;
				} else {
					SPInvalidEnum("currentTool", currentTool);
				}
			} else {
				if (currentTool == Player::ToolSpade) {
					skin = spadeViewSkin;
				} else if (currentTool == Player::ToolBlock) {
					skin = blockViewSkin;
				} else if (currentTool == Player::ToolGrenade) {
					skin = grenadeViewSkin;
				} else if (currentTool == Player::ToolWeapon) {
					skin = weaponViewSkin;
				} else {
					SPInvalidEnum("currentTool", currentTool);
				}
			}
			{
				ScriptIToolSkin interface(skin);
				interface.Update(dt);
			}
		}

		Matrix4 ClientPlayer::GetEyeMatrix() {
			Vector3 eye = player->GetEye();

			if ((int)cg_shake >= 2) {
				float sp = SmoothStep(GetSprintState());
				float p =
				  cosf(player->GetWalkAnimationProgress() * static_cast<float>(M_PI) * 2.f - 0.8f);
				p = p * p;
				p *= p;
				p *= p;
				p *= p;
				eye.z -= p * 0.06f * sp;
			}

			return Matrix4::FromAxis(-player->GetRight(), player->GetFront(), -player->GetUp(),
			                         eye);
		}

		void ClientPlayer::SetSkinParameterForTool(Player::ToolType type, asIScriptObject *skin) {
			Player *p = player;
			if (currentTool == Player::ToolSpade) {

				ScriptISpadeSkin interface(skin);
				WeaponInput inp = p->GetWeaponInput();
				if (p->GetTool() != Player::ToolSpade) {
					interface.SetActionType(SpadeActionTypeIdle);
					interface.SetActionProgress(0.f);
				} else if (inp.primary) {
					interface.SetActionType(SpadeActionTypeBash);
					interface.SetActionProgress(p->GetSpadeAnimationProgress());
				} else if (inp.secondary) {
					interface.SetActionType(p->IsFirstDig() ? SpadeActionTypeDigStart
					                                        : SpadeActionTypeDig);
					interface.SetActionProgress(p->GetDigAnimationProgress());
				} else {
					interface.SetActionType(SpadeActionTypeIdle);
					interface.SetActionProgress(0.f);
				}
			} else if (currentTool == Player::ToolBlock) {

				// TODO: smooth ready state
				ScriptIBlockSkin interface(skin);
				if (p->GetTool() != Player::ToolBlock) {
					// FIXME: use block's IsReadyToUseTool
					// for smoother transition
					interface.SetReadyState(0.f);
				} else if (p->IsReadyToUseTool()) {
					interface.SetReadyState(1.f);
				} else {
					interface.SetReadyState(0.f);
				}

				interface.SetBlockColor(MakeVector3(p->GetBlockColor()) / 255.f);
			} else if (currentTool == Player::ToolGrenade) {

				ScriptIGrenadeSkin interface(skin);
				interface.SetReadyState(1.f - p->GetTimeToNextGrenade() / 0.5f);

				WeaponInput inp = p->GetWeaponInput();
				if (inp.primary) {
					interface.SetCookTime(p->GetGrenadeCookTime());
				} else {
					interface.SetCookTime(0.f);
				}
			} else if (currentTool == Player::ToolWeapon) {

				Weapon *w = p->GetWeapon();
				ScriptIWeaponSkin interface(skin);
				interface.SetReadyState(1.f - w->TimeToNextFire() / w->GetDelay());
				interface.SetAimDownSightState(aimDownState);
				interface.SetAmmo(w->GetAmmo());
				interface.SetClipSize(w->GetClipSize());
				interface.SetReloading(w->IsReloading());
				interface.SetReloadProgress(w->GetReloadProgress());
			} else {
				SPInvalidEnum("currentTool", currentTool);
			}
		}

		void ClientPlayer::SetCommonSkinParameter(asIScriptObject *skin) {
			asIScriptObject *curSkin;
			if (ShouldRenderInThirdPersonView()) {
				if (currentTool == Player::ToolSpade) {
					curSkin = spadeSkin;
				} else if (currentTool == Player::ToolBlock) {
					curSkin = blockSkin;
				} else if (currentTool == Player::ToolGrenade) {
					curSkin = grenadeSkin;
				} else if (currentTool == Player::ToolWeapon) {
					curSkin = weaponSkin;
				} else {
					SPInvalidEnum("currentTool", currentTool);
				}
			} else {
				if (currentTool == Player::ToolSpade) {
					curSkin = spadeViewSkin;
				} else if (currentTool == Player::ToolBlock) {
					curSkin = blockViewSkin;
				} else if (currentTool == Player::ToolGrenade) {
					curSkin = grenadeViewSkin;
				} else if (currentTool == Player::ToolWeapon) {
					curSkin = weaponViewSkin;
				} else {
					SPInvalidEnum("currentTool", currentTool);
				}
			}

			float sprint = SmoothStep(sprintState);
			float putdown = 1.f - toolRaiseState;
			putdown *= putdown;
			putdown = std::min(1.f, putdown * 1.5f);
			{
				ScriptIToolSkin interface(skin);
				interface.SetRaiseState((skin == curSkin) ? (1.f - putdown) : 0.f);
				interface.SetSprintState(sprint);
				interface.SetMuted(client->IsMuted());
			}
		}

		std::array<Vector3, 3> ClientPlayer::GetFlashlightAxes() {
			std::array<Vector3, 3> axes;

			axes[2] = flashlightOrientation;
			axes[0] = Vector3::Cross(flashlightOrientation, player->GetUp()).Normalize();
			axes[1] = Vector3::Cross(axes[0], axes[2]);

			return axes;
		}

		void ClientPlayer::AddToSceneFirstPersonView() {
			Player *p = player;
			IRenderer *renderer = client->GetRenderer();
			World *world = client->GetWorld();
			Matrix4 eyeMatrix = GetEyeMatrix();

			sandboxedRenderer->SetClipBox(AABB3(eyeMatrix.GetOrigin() - Vector3(20.f, 20.f, 20.f),
			                                    eyeMatrix.GetOrigin() + Vector3(20.f, 20.f, 20.f)));
			sandboxedRenderer->SetAllowDepthHack(true);

			// no flashlight if spectating other players while dead
			if (client->flashlightOn && world->GetLocalPlayer()->IsAlive()) {
				float brightness;
				brightness = client->time - client->flashlightOnTime;
				brightness = 1.f - expf(-brightness * 5.f);

				// add flash light
				DynamicLightParam light;
				light.origin = (eyeMatrix * MakeVector3(0, -0.05f, -0.1f)).GetXYZ();
				light.color = MakeVector3(1, .7f, .5f) * 1.5f * brightness;
				light.radius = 40.f;
				light.type = DynamicLightTypeSpotlight;
				light.spotAngle = 30.f * M_PI / 180.f;
				light.spotAxis = GetFlashlightAxes();
				light.image = renderer->RegisterImage("Gfx/Spotlight.png");
				renderer->AddLight(light);

				light.color *= .3f;
				light.radius = 10.f;
				light.type = DynamicLightTypePoint;
				light.image = NULL;
				renderer->AddLight(light);

				// add glare
				renderer->SetColorAlphaPremultiplied(MakeVector4(1, .7f, .5f, 0) * brightness *
				                                     .3f);
				renderer->AddSprite(renderer->RegisterImage("Gfx/Glare.png"),
				                    (eyeMatrix * MakeVector3(0, 0.3f, -0.3f)).GetXYZ(), .8f, 0.f);
			}

			Vector3 leftHand, rightHand;
			leftHand = MakeVector3(0, 0, 0);
			rightHand = MakeVector3(0, 0, 0);

			// view weapon

			Vector3 viewWeaponOffset = this->viewWeaponOffset;

			// bobbing
			{
				float sp = 1.f - aimDownState;
				sp *= .3f;
				sp *= std::min(1.f, p->GetVelocity().GetLength() * 5.f);
				viewWeaponOffset.x +=
				  sinf(p->GetWalkAnimationProgress() * M_PI * 2.f) * 0.013f * sp;
				float vl = cosf(p->GetWalkAnimationProgress() * M_PI * 2.f);
				vl *= vl;
				viewWeaponOffset.z += vl * 0.018f * sp;
			}

			// slow pulse
			{
				float sp = 1.f - aimDownState;
				float vl = sinf(world->GetTime() * 1.f);

				viewWeaponOffset.x += vl * 0.001f * sp;
				viewWeaponOffset.y += vl * 0.0007f * sp;
				viewWeaponOffset.z += vl * 0.003f * sp;
			}

			// manual adjustment
			viewWeaponOffset += Vector3{cg_viewWeaponX, cg_viewWeaponY, cg_viewWeaponZ} * (1.f - aimDownState);

			asIScriptObject *skin;

			if (currentTool == Player::ToolSpade) {
				skin = spadeViewSkin;
			} else if (currentTool == Player::ToolBlock) {
				skin = blockViewSkin;
			} else if (currentTool == Player::ToolGrenade) {
				skin = grenadeViewSkin;
			} else if (currentTool == Player::ToolWeapon) {
				skin = weaponViewSkin;
			} else {
				SPInvalidEnum("currentTool", currentTool);
			}

			SetSkinParameterForTool(currentTool, skin);

			SetCommonSkinParameter(skin);

			// common process
			{
				ScriptIViewToolSkin interface(skin);
				interface.SetEyeMatrix(GetEyeMatrix());
				interface.SetSwing(viewWeaponOffset);
			}
			{
				ScriptIToolSkin interface(skin);
				interface.AddToScene();
			}
			{
				ScriptIViewToolSkin interface(skin);
				leftHand = interface.GetLeftHandPosition();
				rightHand = interface.GetRightHandPosition();
			}

			// view hands
			if (leftHand.GetPoweredLength() > 0.001f && rightHand.GetPoweredLength() > 0.001f) {

				ModelRenderParam param;
				param.depthHack = true;

				IModel *model = renderer->RegisterModel("Models/Player/Arm.kv6");
				IModel *model2 = renderer->RegisterModel("Models/Player/UpperArm.kv6");

				IntVector3 col = p->GetColor();
				param.customColor = MakeVector3(col.x / 255.f, col.y / 255.f, col.z / 255.f);

				const float armlen = 0.5f;

				Vector3 shoulders[] = {{0.4f, 0.0f, 0.25f}, {-0.4f, 0.0f, 0.25f}};
				Vector3 hands[] = {leftHand, rightHand};
				Vector3 benddirs[] = {{0.5f, 0.2f, 0.f}, {-0.5f, 0.2f, 0.f}};
				for (int i = 0; i < 2; i++) {
					Vector3 shoulder = shoulders[i];
					Vector3 hand = hands[i];
					Vector3 benddir = benddirs[i];

					float len2 = (hand - shoulder).GetPoweredLength();
					// len2/4 + x^2 = armlen^2
					float bendlen = sqrtf(std::max(armlen * armlen - len2 * .25f, 0.f));

					Vector3 bend = Vector3::Cross(benddir, hand - shoulder);
					bend = bend.Normalize();

					if (bend.z < 0.f)
						bend.z = -bend.z;

					Vector3 elbow = (hand + shoulder) * .5f;
					elbow += bend * bendlen;

					{
						Vector3 axises[3];
						axises[2] = (hand - elbow).Normalize();
						axises[0] = MakeVector3(0, 0, 1);
						axises[1] = Vector3::Cross(axises[2], axises[0]).Normalize();
						axises[0] = Vector3::Cross(axises[1], axises[2]).Normalize();

						Matrix4 mat = Matrix4::Scale(.05f);
						mat = Matrix4::FromAxis(axises[0], axises[1], axises[2], elbow) * mat;
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
						mat = Matrix4::FromAxis(axises[0], axises[1], axises[2], shoulder) * mat;
						mat = eyeMatrix * mat;

						param.matrix = mat;
						renderer->RenderModel(model2, param);
					}
				}
			}

			// --- local view ends
		}

		void ClientPlayer::AddToSceneThirdPersonView() {

			Player *p = player;
			IRenderer *renderer = client->GetRenderer();
			World *world = client->GetWorld();

			if (!p->IsAlive()) {
				if (!cg_ragdoll) {
					ModelRenderParam param;
					param.matrix = Matrix4::Translate(p->GetOrigin() + MakeVector3(0, 0, 1));
					param.matrix = param.matrix * Matrix4::Scale(.1f);
					IntVector3 col = p->GetColor();
					param.customColor = MakeVector3(col.x / 255.f, col.y / 255.f, col.z / 255.f);

					IModel *model = renderer->RegisterModel("Models/Player/Dead.kv6");
					renderer->RenderModel(model, param);
				}
				return;
			}

			auto origin = p->GetOrigin();
			sandboxedRenderer->SetClipBox(
			  AABB3(origin - Vector3(2.f, 2.f, 4.f), origin + Vector3(2.f, 2.f, 2.f)));
			sandboxedRenderer->SetAllowDepthHack(false);

			// ready for tool rendering
			asIScriptObject *skin;

			if (currentTool == Player::ToolSpade) {
				skin = spadeSkin;
			} else if (currentTool == Player::ToolBlock) {
				skin = blockSkin;
			} else if (currentTool == Player::ToolGrenade) {
				skin = grenadeSkin;
			} else if (currentTool == Player::ToolWeapon) {
				skin = weaponSkin;
			} else {
				SPInvalidEnum("currentTool", currentTool);
			}

			SetSkinParameterForTool(currentTool, skin);

			SetCommonSkinParameter(skin);

			float pitchBias;
			{
				ScriptIThirdPersonToolSkin interface(skin);
				pitchBias = interface.GetPitchBias();
			}

			ModelRenderParam param;
			IModel *model;
			Vector3 front = p->GetFront();
			IntVector3 col = p->GetColor();
			param.customColor = MakeVector3(col.x / 255.f, col.y / 255.f, col.z / 255.f);

			float yaw = atan2(front.y, front.x) + M_PI * .5f;
			float pitch = -atan2(front.z, sqrt(front.x * front.x + front.y * front.y));

			// lower axis
			Matrix4 lower = Matrix4::Translate(p->GetOrigin());
			lower = lower * Matrix4::Rotate(MakeVector3(0, 0, 1), yaw);

			Matrix4 scaler = Matrix4::Scale(0.1f);
			scaler = scaler * Matrix4::Scale(-1, -1, 1);

			PlayerInput inp = p->GetInput();

			// lower
			Matrix4 torso, head, arms;
			if (inp.crouch) {
				Matrix4 leg1 = Matrix4::Translate(-0.25f, 0.2f, -0.1f);
				Matrix4 leg2 = Matrix4::Translate(0.25f, 0.2f, -0.1f);

				float ang = sinf(p->GetWalkAnimationProgress() * M_PI * 2.f) * 0.6f;
				float walkVel = Vector3::Dot(p->GetVelocity(), p->GetFront2D()) * 4.f;
				leg1 = leg1 * Matrix4::Rotate(MakeVector3(1, 0, 0), ang * walkVel);
				leg2 = leg2 * Matrix4::Rotate(MakeVector3(1, 0, 0), -ang * walkVel);

				walkVel = Vector3::Dot(p->GetVelocity(), p->GetRight()) * 3.f;
				leg1 = leg1 * Matrix4::Rotate(MakeVector3(0, 1, 0), ang * walkVel);
				leg2 = leg2 * Matrix4::Rotate(MakeVector3(0, 1, 0), -ang * walkVel);

				leg1 = lower * leg1;
				leg2 = lower * leg2;

				model = renderer->RegisterModel("Models/Player/LegCrouch.kv6");
				param.matrix = leg1 * scaler;
				renderer->RenderModel(model, param);
				param.matrix = leg2 * scaler;
				renderer->RenderModel(model, param);

				torso = Matrix4::Translate(0.f, 0.f, -0.55f);
				torso = lower * torso;

				model = renderer->RegisterModel("Models/Player/TorsoCrouch.kv6");
				param.matrix = torso * scaler;
				renderer->RenderModel(model, param);

				head = Matrix4::Translate(0.f, 0.f, -0.0f);
				head = torso * head;

				arms = Matrix4::Translate(0.f, 0.f, -0.0f);
				arms = torso * arms;
			} else {
				Matrix4 leg1 = Matrix4::Translate(-0.25f, 0.f, -0.1f);
				Matrix4 leg2 = Matrix4::Translate(0.25f, 0.f, -0.1f);

				float ang = sinf(p->GetWalkAnimationProgress() * M_PI * 2.f) * 0.6f;
				float walkVel = Vector3::Dot(p->GetVelocity(), p->GetFront2D()) * 4.f;
				leg1 = leg1 * Matrix4::Rotate(MakeVector3(1, 0, 0), ang * walkVel);
				leg2 = leg2 * Matrix4::Rotate(MakeVector3(1, 0, 0), -ang * walkVel);

				walkVel = Vector3::Dot(p->GetVelocity(), p->GetRight()) * 3.f;
				leg1 = leg1 * Matrix4::Rotate(MakeVector3(0, 1, 0), ang * walkVel);
				leg2 = leg2 * Matrix4::Rotate(MakeVector3(0, 1, 0), -ang * walkVel);

				leg1 = lower * leg1;
				leg2 = lower * leg2;

				model = renderer->RegisterModel("Models/Player/Leg.kv6");
				param.matrix = leg1 * scaler;
				renderer->RenderModel(model, param);
				param.matrix = leg2 * scaler;
				renderer->RenderModel(model, param);

				torso = Matrix4::Translate(0.f, 0.f, -1.0f);
				torso = lower * torso;

				model = renderer->RegisterModel("Models/Player/Torso.kv6");
				param.matrix = torso * scaler;
				renderer->RenderModel(model, param);

				head = Matrix4::Translate(0.f, 0.f, -0.0f);
				head = torso * head;

				arms = Matrix4::Translate(0.f, 0.f, 0.1f);
				arms = torso * arms;
			}

			float armPitch = pitch;
			if (inp.sprint) {
				armPitch -= .5f;
			}
			armPitch += pitchBias;
			if (armPitch < 0.f) {
				armPitch = std::max(armPitch, -(float)M_PI * .5f);
				armPitch *= .9f;
			}

			arms = arms * Matrix4::Rotate(MakeVector3(1, 0, 0), armPitch);

			model = renderer->RegisterModel("Models/Player/Arms.kv6");
			param.matrix = arms * scaler;
			renderer->RenderModel(model, param);

			head = head * Matrix4::Rotate(MakeVector3(1, 0, 0), pitch);

			model = renderer->RegisterModel("Models/Player/Head.kv6");
			param.matrix = head * scaler;
			renderer->RenderModel(model, param);

			// draw tool
			{
				ScriptIThirdPersonToolSkin interface(skin);
				interface.SetOriginMatrix(arms);
			}
			{
				ScriptIToolSkin interface(skin);
				interface.AddToScene();
			}

			hasValidOriginMatrix = true;

			// draw intel in ctf
			IGameMode *mode = world->GetMode();
			if (mode && IGameMode::m_CTF == mode->ModeType()) {
				CTFGameMode *ctfMode = static_cast<CTFGameMode *>(world->GetMode());
				int tId = p->GetTeamId();
				if (tId < 3) {
					CTFGameMode::Team &team = ctfMode->GetTeam(p->GetTeamId());
					if (team.hasIntel && team.carrier == p->GetId()) {

						IntVector3 col2 = world->GetTeam(1 - p->GetTeamId()).color;
						param.customColor =
						  MakeVector3(col2.x / 255.f, col2.y / 255.f, col2.z / 255.f);
						Matrix4 mIntel = torso * Matrix4::Translate(0, 0.6f, 0.5f);

						model = renderer->RegisterModel("Models/MapObjects/Intel.kv6");
						param.matrix = mIntel * scaler;
						renderer->RenderModel(model, param);

						param.customColor =
						  MakeVector3(col.x / 255.f, col.y / 255.f, col.z / 255.f);
					}
				}
			}

			// third person player rendering, done
		}

		void ClientPlayer::AddToScene() {
			SPADES_MARK_FUNCTION();

			Player *p = player;
			const SceneDefinition &lastSceneDef = client->GetLastSceneDef();

			hasValidOriginMatrix = false;

			if (p->GetTeamId() >= 2) {
				// spectator, or dummy player
				return;
			}

			float distancePowered = (p->GetOrigin() - lastSceneDef.viewOrigin).GetPoweredLength();
			if (distancePowered > 140.f * 140.f) {
				return;
			}

			if (!ShouldRenderInThirdPersonView()) {
				AddToSceneFirstPersonView();
			} else {
				AddToSceneThirdPersonView();
			}
		}

		void ClientPlayer::Draw2D() {
			if (!ShouldRenderInThirdPersonView() && player->IsAlive()) {
				asIScriptObject *skin;

				if (currentTool == Player::ToolSpade) {
					skin = spadeViewSkin;
				} else if (currentTool == Player::ToolBlock) {
					skin = blockViewSkin;
				} else if (currentTool == Player::ToolGrenade) {
					skin = grenadeViewSkin;
				} else if (currentTool == Player::ToolWeapon) {
					skin = weaponViewSkin;
				} else {
					SPInvalidEnum("currentTool", currentTool);
				}

				SetSkinParameterForTool(currentTool, skin);

				SetCommonSkinParameter(skin);

				// common process
				{
					ScriptIViewToolSkin interface(skin);
					interface.SetEyeMatrix(GetEyeMatrix());
					interface.SetSwing(viewWeaponOffset);
					interface.Draw2D();
				}
			}
		}

		bool ClientPlayer::ShouldRenderInThirdPersonView() {
			// The player from whom's perspective the game is
			return !IsFirstPerson(client->GetCameraMode()) || player != &client->GetCameraTargetPlayer();
		}

		struct ClientPlayer::AmbienceInfo {
			float room;
			float size;
			float distance;
		};

		ClientPlayer::AmbienceInfo ClientPlayer::ComputeAmbience() {
			const SceneDefinition &lastSceneDef = client->GetLastSceneDef();

			if (!cg_environmentalAudio) {
				AmbienceInfo result;
				result.room = 0.0f;
				result.distance = (lastSceneDef.viewOrigin - player->GetEye()).GetLength();
				result.size = 0.0f;
				return result;
			}

			float maxDistance = 40.f;
			GameMap *map = client->map;
			SPAssert(map);

			Vector3 rayFrom = player->GetEye();
			// uniformly distributed random unit vectors
			const Vector3 directions[24] = {
			  {-0.4806003057749437f, -0.42909622618705534f, 0.7647874049440525f},
			  {-0.32231294555647927f, 0.6282069816346844f, 0.7081457147735524f},
			  {0.048740582496498826f, -0.6670915238644523f, 0.7433796166200044f},
			  {0.4507022412112344f, 0.2196054264547812f, 0.8652403980621708f},
			  {-0.42721511627413183f, -0.587164590982542f, -0.6875499891085622f},
			  {-0.5570464880797501f, 0.3832470400156089f, -0.7367638131974799f},
			  {0.4379032819319448f, -0.5217172826725083f, -0.732155579528044f},
			  {0.5505793235065188f, 0.5884516130938041f, -0.5921039668625805f},
			  {0.681714179159347f, -0.6289005125058891f, -0.3738314102679548f},
			  {0.882424317058847f, 0.4680895178240496f, -0.047111866514457174f},
			  {0.8175844570742612f, -0.5123280060684333f, 0.26282250616819125f},
			  {0.7326555076593512f, 0.16938649523355995f, 0.6591844372623717f},
			  {-0.8833847855718798f, -0.46859333747646814f, -0.007183640636104698f},
			  {-0.6478926243769724f, 0.5325399055055595f, -0.5446433661783178f},
			  {-0.7011236289377749f, -0.4179353735633245f, 0.5777159167528706f},
			  {-0.8834742898471629f, 0.3226030059694268f, 0.3397064611080296f},
			  {-0.701272268659947f, 0.7126868112640804f, -0.017167243773185584f},
			  {-0.4048459451282839f, 0.8049148135357349f, 0.4338339586338529f},
			  {0.10511344475950758f, 0.7400485819463978f, -0.664288536774432f},
			  {0.4228172536676786f, 0.7759558485735245f, 0.46810051384874957f},
			  {-0.641642302739998f, -0.7293326298605313f, -0.23742171416118207f},
			  {-0.269582155924164f, -0.957885171758109f, 0.09890125850168793f},
			  {0.09274966874325204f, -0.9126579244190587f, -0.39806156803076687f},
			  {0.49359438685568013f, -0.721891173178783f, 0.48501310843226225f}};
			std::array<float, 24> distances;
			std::array<float, 24> feedbacknesses;

			std::fill(feedbacknesses.begin(), feedbacknesses.end(), 0.0f);

			for (std::size_t i = 0; i < distances.size(); ++i) {
				float &distance = distances[i];
				float &feedbackness = feedbacknesses[i];

				const Vector3 &rayTo = directions[i];

				IntVector3 hitPos;
				bool hit = map->CastRay(rayFrom, rayTo, maxDistance, hitPos);
				if (hit) {
					Vector3 hitPosf = {(float)hitPos.x, (float)hitPos.y, (float)hitPos.z};
					distance = (hitPosf - rayFrom).GetLength();
				} else {
					distance = maxDistance * 2.f;
				}

				if (hit) {
					bool hit2 = map->CastRay(rayFrom, -rayTo, maxDistance, hitPos);
					if (hit2)
						feedbackness = 1.f;
					else
						feedbackness = 0.f;
				}
			}

			// monte-carlo integration
			unsigned int rayHitCount = 0;
			float roomSize = 0.f;
			float feedbackness = 0.f;

			for (float dist : distances) {
				if (dist < maxDistance) {
					rayHitCount++;
					roomSize += dist;
				}
			}
			for (float fb : feedbacknesses) {
				feedbackness += fb;
			}

			float reflections;
			if (rayHitCount > distances.size() / 4) {
				roomSize /= (float)rayHitCount;
				reflections = (float)rayHitCount / (float)distances.size();
			} else {
				reflections = 0.1f;
				roomSize = 100.f;
			}

			feedbackness /= (float)distances.size();
			feedbackness = std::min(std::max(0.0f, feedbackness - 0.35f) / 0.5f, 1.0f);

			AmbienceInfo result;
			result.room = reflections * feedbackness;
			result.distance = (lastSceneDef.viewOrigin - player->GetEye()).GetLength();
			result.size = std::max(std::min(roomSize / 15.0f, 1.0f), 0.0f);
			result.room *= std::max(0.0f, std::min((result.size - 0.1f) * 4.0f, 1.0f));
			result.room *= 1.0f - result.size * 0.3f;

			return result;
		}

		void ClientPlayer::FiredWeapon() {
			World *world = player->GetWorld();
			Vector3 muzzle;
			const SceneDefinition &lastSceneDef = client->GetLastSceneDef();
			IRenderer *renderer = client->GetRenderer();
			IAudioDevice *audioDevice = client->GetAudioDevice();
			Player *p = player;

			// make dlight
			{
				Vector3 vec;
				Matrix4 eyeMatrix = GetEyeMatrix();
				Matrix4 mat;
				mat = Matrix4::Translate(-0.13f, .5f, 0.2f);
				mat = eyeMatrix * mat;

				vec = (mat * MakeVector3(0, 1, 0)).GetXYZ();
				muzzle = vec;
				client->MuzzleFire(vec, player->GetFront(), player == world->GetLocalPlayer());
			}

			if (cg_ejectBrass) {
				float dist = (player->GetOrigin() - lastSceneDef.viewOrigin).GetPoweredLength();
				if (dist < 130.f * 130.f) {
					IModel *model = NULL;
					Handle<IAudioChunk> snd = NULL;
					Handle<IAudioChunk> snd2 = NULL;
					switch (player->GetWeapon()->GetWeaponType()) {
						case RIFLE_WEAPON:
							model = renderer->RegisterModel("Models/Weapons/Rifle/Casing.kv6");
							snd =
							  SampleRandomBool()
							    ? audioDevice->RegisterSound("Sounds/Weapons/Rifle/ShellDrop1.opus")
							    : audioDevice->RegisterSound(
							        "Sounds/Weapons/Rifle/ShellDrop2.opus");
							snd2 =
							  audioDevice->RegisterSound("Sounds/Weapons/Rifle/ShellWater.opus");
							break;
						case SHOTGUN_WEAPON:
							// FIXME: don't want to show shotgun't casing
							// because it isn't ejected when firing
							// model = renderer->RegisterModel("Models/Weapons/Shotgun/Casing.kv6");
							break;
						case SMG_WEAPON:
							model = renderer->RegisterModel("Models/Weapons/SMG/Casing.kv6");
							snd =
							  SampleRandomBool()
							    ? audioDevice->RegisterSound("Sounds/Weapons/SMG/ShellDrop1.opus")
							    : audioDevice->RegisterSound("Sounds/Weapons/SMG/ShellDrop2.opus");
							snd2 = audioDevice->RegisterSound("Sounds/Weapons/SMG/ShellWater.opus");
							break;
					}
					if (model) {
						Vector3 origin;
						origin = muzzle - p->GetFront() * 0.5f;

						Vector3 vel;
						vel = p->GetFront() * 0.5f + p->GetRight() + p->GetUp() * 0.2f;
						switch (p->GetWeapon()->GetWeaponType()) {
							case SMG_WEAPON: vel -= p->GetFront() * 0.7f; break;
							case SHOTGUN_WEAPON: vel *= .5f; break;
							default: break;
						}

						ILocalEntity *ent;
						ent = new GunCasing(client, model, snd, snd2, origin, p->GetFront(), vel);
						client->AddLocalEntity(ent);
					}
				}
			}

			// sound ambience estimation
			auto ambience = ComputeAmbience();

			asIScriptObject *skin;
			// FIXME: what if current tool isn't weapon?
			if (ShouldRenderInThirdPersonView()) {
				skin = weaponSkin;
			} else {
				skin = weaponViewSkin;
			}

			{
				ScriptIWeaponSkin2 interface(skin);
				if (interface.ImplementsInterface()) {
					interface.SetSoundEnvironment(ambience.room, ambience.size, ambience.distance);
					interface.SetSoundOrigin(player->GetEye());
				} else if (ShouldRenderInThirdPersonView() && !hasValidOriginMatrix) {
					// Legacy skin scripts rely on OriginMatrix which is only updated when
					// the player's location is within the fog range.
					return;
				}
			}

			{
				ScriptIWeaponSkin interface(skin);
				interface.WeaponFired();
			}
		}

		void ClientPlayer::ReloadingWeapon() {
			asIScriptObject *skin;
			// FIXME: what if current tool isn't weapon?
			if (ShouldRenderInThirdPersonView()) {
				skin = weaponSkin;
			} else {
				skin = weaponViewSkin;
			}

			// sound ambience estimation
			auto ambience = ComputeAmbience();

			{
				ScriptIWeaponSkin2 interface(skin);
				if (interface.ImplementsInterface()) {
					interface.SetSoundEnvironment(ambience.room, ambience.size, ambience.distance);
					interface.SetSoundOrigin(player->GetEye());
				} else if (ShouldRenderInThirdPersonView() && !hasValidOriginMatrix) {
					// Legacy skin scripts rely on OriginMatrix which is only updated when
					// the player's location is within the fog range.
					return;
				}
			}

			{
				ScriptIWeaponSkin interface(skin);
				interface.ReloadingWeapon();
			}
		}

		void ClientPlayer::ReloadedWeapon() {
			asIScriptObject *skin;
			// FIXME: what if current tool isn't weapon?
			if (ShouldRenderInThirdPersonView()) {
				skin = weaponSkin;
			} else {
				skin = weaponViewSkin;
			}

			{
				ScriptIWeaponSkin interface(skin);
				interface.ReloadedWeapon();
			}
		}
	}
}
