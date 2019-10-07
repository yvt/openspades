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

#include <Core/Settings.h>
#include <Core/Strings.h>

#include "IAudioChunk.h"
#include "IAudioDevice.h"

#include "ClientUI.h"
#include "ChatWindow.h"
#include "Corpse.h"
#include "LimboView.h"
#include "MapView.h"
#include "PaletteView.h"

#include "Weapon.h"
#include "World.h"

#include "NetClient.h"

using namespace std;

DEFINE_SPADES_SETTING(cg_mouseSensitivity, "1");
DEFINE_SPADES_SETTING(cg_zoomedMouseSensScale, "1");
DEFINE_SPADES_SETTING(cg_mouseExpPower, "1");
DEFINE_SPADES_SETTING(cg_invertMouseY, "0");

DEFINE_SPADES_SETTING(cg_holdAimDownSight, "0");

DEFINE_SPADES_SETTING(cg_keyAttack, "LeftMouseButton");
DEFINE_SPADES_SETTING(cg_keyAltAttack, "RightMouseButton");
DEFINE_SPADES_SETTING(cg_keyToolSpade, "1");
DEFINE_SPADES_SETTING(cg_keyToolBlock, "2");
DEFINE_SPADES_SETTING(cg_keyToolWeapon, "3");
DEFINE_SPADES_SETTING(cg_keyToolGrenade, "4");
DEFINE_SPADES_SETTING(cg_keyReloadWeapon, "r");
DEFINE_SPADES_SETTING(cg_keyFlashlight, "f");
DEFINE_SPADES_SETTING(cg_keyLastTool, "");

DEFINE_SPADES_SETTING(cg_keyMoveLeft, "a");
DEFINE_SPADES_SETTING(cg_keyMoveRight, "d");
DEFINE_SPADES_SETTING(cg_keyMoveForward, "w");
DEFINE_SPADES_SETTING(cg_keyMoveBackward, "s");
DEFINE_SPADES_SETTING(cg_keyJump, "Space");
DEFINE_SPADES_SETTING(cg_keyCrouch, "Control");
DEFINE_SPADES_SETTING(cg_keySprint, "Shift");
DEFINE_SPADES_SETTING(cg_keySneak, "v");

DEFINE_SPADES_SETTING(cg_keyCaptureColor, "e");
DEFINE_SPADES_SETTING(cg_keyGlobalChat, "t");
DEFINE_SPADES_SETTING(cg_keyTeamChat, "y");
DEFINE_SPADES_SETTING(cg_keyZoomChatLog, "h");
DEFINE_SPADES_SETTING(cg_keyChangeMapScale, "m");
DEFINE_SPADES_SETTING(cg_keyToggleMapZoom, "n");
DEFINE_SPADES_SETTING(cg_keyScoreboard, "Tab");
DEFINE_SPADES_SETTING(cg_keyLimbo, "l");

DEFINE_SPADES_SETTING(cg_keyScreenshot, "0");
DEFINE_SPADES_SETTING(cg_keySceneshot, "9");
DEFINE_SPADES_SETTING(cg_keySaveMap, "8");

DEFINE_SPADES_SETTING(cg_switchToolByWheel, "1");
DEFINE_SPADES_SETTING(cg_debugCorpse, "0");
DEFINE_SPADES_SETTING(cg_alerts, "1");

SPADES_SETTING(cg_manualFocus);
DEFINE_SPADES_SETTING(cg_keyAutoFocus, "MiddleMouseButton");

namespace spades {
	namespace client {

		bool Client::WantsToBeClosed() { return readyToClose; }

		void Client::Closing() { SPADES_MARK_FUNCTION(); }

		bool Client::NeedsAbsoluteMouseCoordinate() {
			SPADES_MARK_FUNCTION();
			if (scriptedUI->NeedsInput()) {
				return true;
			}
			if (!world) {
				// now loading.
				return true;
			}
			if (IsLimboViewActive()) {
				return true;
			}
			return false;
		}

		void Client::MouseEvent(float x, float y) {
			SPADES_MARK_FUNCTION();

			if (scriptedUI->NeedsInput()) {
				scriptedUI->MouseEvent(x, y);
				return;
			}

			if (IsLimboViewActive()) {
				limbo->MouseEvent(x, y);
				return;
			}

			auto cameraMode = GetCameraMode();

			switch (cameraMode) {
				case ClientCameraMode::None:
				case ClientCameraMode::NotJoined:
				case ClientCameraMode::FirstPersonFollow:
					// No-op
					break;

				case ClientCameraMode::Free:
				case ClientCameraMode::ThirdPersonLocal:
				case ClientCameraMode::ThirdPersonFollow: {
					// Move the third-person or free-floating camera
					x = -x;
					if (!cg_invertMouseY)
						y = -y;

					auto &state = followAndFreeCameraState;

					state.yaw -= x * 0.003f;
					state.pitch -= y * 0.003f;
					if (state.pitch < -M_PI * .45f)
						state.pitch = -static_cast<float>(M_PI) * .45f;
					if (state.pitch > M_PI * .45f)
						state.pitch = static_cast<float>(M_PI) * .45f;
					state.yaw = fmodf(state.yaw, static_cast<float>(M_PI) * 2.f);
					break;
				}

				case ClientCameraMode::FirstPersonLocal: {
					SPAssert(world);
					SPAssert(world->GetLocalPlayer());

					Player *p = world->GetLocalPlayer();
					if (p->IsAlive()) {
						float aimDownState = GetAimDownState();
						x /= GetAimDownZoomScale();
						y /= GetAimDownZoomScale();

						float rad = x * x + y * y;
						if (rad > 0.f) {
							if ((float)cg_mouseExpPower < 0.001f ||
							    isnan((float)cg_mouseExpPower)) {
								SPLog("Invalid cg_mouseExpPower value, resetting to 1.0");
								cg_mouseExpPower = 1.f;
							}
							float factor = renderer->ScreenWidth() * .1f;
							factor *= factor;
							rad /= factor;
							rad = powf(rad, (float)cg_mouseExpPower * 0.5f - 0.5f);

							// shouldn't happen...
							if (isnan(rad))
								rad = 1.f;

							x *= rad;
							y *= rad;
						}

						if (aimDownState > 0.f) {
							float scale = cg_zoomedMouseSensScale;
							scale = powf(scale, aimDownState);
							x *= scale;
							y *= scale;
						}

						x *= (float)cg_mouseSensitivity;
						y *= (float)cg_mouseSensitivity;

						if (cg_invertMouseY)
							y = -y;

						p->Turn(x * 0.003f, y * 0.003f);
					}
					break;
				}
			}
		}

		void Client::WheelEvent(float x, float y) {
			SPADES_MARK_FUNCTION();

			if (scriptedUI->NeedsInput()) {
				scriptedUI->WheelEvent(x, y);
				return;
			}

			if (y > .5f) {
				KeyEvent("WheelDown", true);
				KeyEvent("WheelDown", false);
			} else if (y < -.5f) {
				KeyEvent("WheelUp", true);
				KeyEvent("WheelUp", false);
			}
		}

		void Client::TextInputEvent(const std::string &ch) {
			SPADES_MARK_FUNCTION();

			if (scriptedUI->NeedsInput() && !scriptedUI->isIgnored(ch)) {
				scriptedUI->TextInputEvent(ch);
				return;
			}

			// we don't get "/" here anymore
		}

		void Client::TextEditingEvent(const std::string &ch, int start, int len) {
			SPADES_MARK_FUNCTION();

			if (scriptedUI->NeedsInput() && !scriptedUI->isIgnored(ch)) {
				scriptedUI->TextEditingEvent(ch, start, len);
				return;
			}
		}

		bool Client::AcceptsTextInput() {
			SPADES_MARK_FUNCTION();

			if (scriptedUI->NeedsInput()) {
				return scriptedUI->AcceptsTextInput();
			}
			return false;
		}

		AABB2 Client::GetTextInputRect() {
			SPADES_MARK_FUNCTION();
			if (scriptedUI->NeedsInput()) {
				return scriptedUI->GetTextInputRect();
			}
			return AABB2();
		}

		static bool CheckKey(const std::string &cfg, const std::string &input) {
			if (cfg.empty())
				return false;

			static const std::string space1("space");
			static const std::string space2("spacebar");
			static const std::string space3("spacekey");

			if (EqualsIgnoringCase(cfg, space1) || EqualsIgnoringCase(cfg, space2) ||
			    EqualsIgnoringCase(cfg, space3)) {

				if (input == " ")
					return true;
			} else {
				if (EqualsIgnoringCase(cfg, input))
					return true;
			}
			return false;
		}

		void Client::KeyEvent(const std::string &name, bool down) {
			SPADES_MARK_FUNCTION();

			if (scriptedUI->NeedsInput()) {
				if (!scriptedUI->isIgnored(name)) {
					scriptedUI->KeyEvent(name, down);
				} else {
					if (!down) {
						scriptedUI->setIgnored("");
					}
				}
				return;
			}

			if (name == "Escape") {
				if (down) {
					if (inGameLimbo) {
						inGameLimbo = false;
					} else {
						if (GetWorld() == nullptr) {
							// no world = loading now.
							// in this case, abort download, and quit the game immediately.
							readyToClose = true;
						} else {
							scriptedUI->EnterClientMenu();
						}
					}
				}
			} else if (world) {
				if (IsLimboViewActive()) {
					if (down) {
						limbo->KeyEvent(name);
					}
					return;
				}

				auto cameraMode = GetCameraMode();

				switch (cameraMode) {
					case ClientCameraMode::None:
					case ClientCameraMode::NotJoined:
					case ClientCameraMode::FirstPersonLocal: break;
					case ClientCameraMode::ThirdPersonLocal:
						if (world->GetLocalPlayer()->IsAlive()) {
							break;
						}
					case ClientCameraMode::FirstPersonFollow:
					case ClientCameraMode::ThirdPersonFollow:
					case ClientCameraMode::Free:
						if (CheckKey(cg_keyAttack, name)) {
							if (down) {
								if (cameraMode == ClientCameraMode::Free ||
								    cameraMode == ClientCameraMode::ThirdPersonLocal) {
									// Start with the local player
									followedPlayerId = world->GetLocalPlayerIndex();
								}
								if (world->GetLocalPlayer()->IsSpectator() ||
								    time > lastAliveTime + 1.3f) {
									FollowNextPlayer(false);
								}
							}
							return;
						} else if (CheckKey(cg_keyAltAttack, name)) {
							if (down) {
								if (cameraMode == ClientCameraMode::Free ||
								    cameraMode == ClientCameraMode::ThirdPersonLocal) {
									// Start with the local player
									followedPlayerId = world->GetLocalPlayerIndex();
								}
								if (world->GetLocalPlayer()->IsSpectator() ||
								    time > lastAliveTime + 1.3f) {
									FollowNextPlayer(true);
								}
							}
							return;
						} else if (CheckKey(cg_keyJump, name) &&
						           cameraMode != ClientCameraMode::Free) {
							if (down && GetCameraTargetPlayer().IsAlive()) {
								followCameraState.firstPerson = !followCameraState.firstPerson;
							}
							return;
						} else if (CheckKey(cg_keyReloadWeapon, name) &&
						           world->GetLocalPlayer()->IsSpectator() &&
						           followCameraState.enabled) {
							if (down) {
								// Unfollow
								followCameraState.enabled = false;
							}
							return;
						}
						break;
				}

				if (world->GetLocalPlayer()) {
					Player *p = world->GetLocalPlayer();

					if (p->IsAlive() && p->GetTool() == Player::ToolBlock && down) {
						if (paletteView->KeyInput(name)) {
							return;
						}
					}

					if (cg_debugCorpse) {
						if (name == "p" && down) {
							Corpse *corp;
							Player *victim = world->GetLocalPlayer();
							corp = new Corpse(renderer, map, victim);
							corp->AddImpulse(victim->GetFront() * 32.f);
							corpses.emplace_back(corp);

							if (corpses.size() > corpseHardLimit) {
								corpses.pop_front();
							} else if (corpses.size() > corpseSoftLimit) {
								RemoveInvisibleCorpses();
							}
						}
					}
					if (CheckKey(cg_keyMoveLeft, name)) {
						playerInput.moveLeft = down;
						keypadInput.left = down;
						if (down)
							playerInput.moveRight = false;
						else
							playerInput.moveRight = keypadInput.right;
					} else if (CheckKey(cg_keyMoveRight, name)) {
						playerInput.moveRight = down;
						keypadInput.right = down;
						if (down)
							playerInput.moveLeft = false;
						else
							playerInput.moveLeft = keypadInput.left;
					} else if (CheckKey(cg_keyMoveForward, name)) {
						playerInput.moveForward = down;
						keypadInput.forward = down;
						if (down)
							playerInput.moveBackward = false;
						else
							playerInput.moveBackward = keypadInput.backward;
					} else if (CheckKey(cg_keyMoveBackward, name)) {
						playerInput.moveBackward = down;
						keypadInput.backward = down;
						if (down)
							playerInput.moveForward = false;
						else
							playerInput.moveForward = keypadInput.forward;
					} else if (CheckKey(cg_keyCrouch, name)) {
						playerInput.crouch = down;
					} else if (CheckKey(cg_keySprint, name)) {
						playerInput.sprint = down;
					} else if (CheckKey(cg_keySneak, name)) {
						playerInput.sneak = down;
					} else if (CheckKey(cg_keyJump, name)) {
						playerInput.jump = down;
					} else if (CheckKey(cg_keyAttack, name)) {
						weapInput.primary = down;
					} else if (CheckKey(cg_keyAltAttack, name)) {
						auto lastVal = weapInput.secondary;
						if (world->GetLocalPlayer()->IsToolWeapon() && (!cg_holdAimDownSight)) {
							if (down && !world->GetLocalPlayer()->GetWeapon()->IsReloading()) {
								weapInput.secondary = !weapInput.secondary;
							}
						} else {
							weapInput.secondary = down;
						}
						if (world->GetLocalPlayer()->IsToolWeapon() && weapInput.secondary &&
						    !lastVal && world->GetLocalPlayer()->GetWeapon()->TimeToNextFire() <= 0 &&
						    !world->GetLocalPlayer()->GetWeapon()->IsReloading() &&
						    GetSprintState() == 0.0f) {
							AudioParam params;
							params.volume = 0.08f;
							Handle<IAudioChunk> chunk =
							  audioDevice->RegisterSound("Sounds/Weapons/AimDownSightLocal.opus");
							audioDevice->PlayLocal(chunk, MakeVector3(.4f, -.3f, .5f), params);
						}
					} else if (CheckKey(cg_keyReloadWeapon, name) && down) {
						Weapon *w = world->GetLocalPlayer()->GetWeapon();
						if (w->GetAmmo() < w->GetClipSize() && w->GetStock() > 0 &&
						    (!world->GetLocalPlayer()->IsAwaitingReloadCompletion()) &&
						    (!w->IsReloading()) &&
						    world->GetLocalPlayer()->GetTool() == Player::ToolWeapon) {
							if (world->GetLocalPlayer()->IsToolWeapon()) {
								if (weapInput.secondary) {
									// if we send WeaponInput after sending Reload,
									// server might cancel the reload.
									// https://github.com/infogulch/pyspades/blob/895879ed14ddee47bb278a77be86d62c7580f8b7/pyspades/server.py#343
									hasDelayedReload = true;
									weapInput.secondary = false;
									return;
								}
							}
							world->GetLocalPlayer()->Reload();
							net->SendReload();
						}
					} else if (CheckKey(cg_keyToolSpade, name) && down) {
						if (world->GetLocalPlayer()->GetTeamId() < 2 &&
						    world->GetLocalPlayer()->IsAlive() &&
						    world->GetLocalPlayer()->IsToolSelectable(Player::ToolSpade)) {
							SetSelectedTool(Player::ToolSpade);
						}
					} else if (CheckKey(cg_keyToolBlock, name) && down) {
						if (world->GetLocalPlayer()->GetTeamId() < 2 &&
						    world->GetLocalPlayer()->IsAlive()) {
							if (world->GetLocalPlayer()->IsToolSelectable(Player::ToolBlock)) {
								SetSelectedTool(Player::ToolBlock);
							} else {
								if (cg_alerts)
									ShowAlert(_Tr("Client", "Out of Blocks"), AlertType::Error);
								else
									PlayAlertSound();
							}
						}
					} else if (CheckKey(cg_keyToolWeapon, name) && down) {
						if (world->GetLocalPlayer()->GetTeamId() < 2 &&
						    world->GetLocalPlayer()->IsAlive()) {
							if (world->GetLocalPlayer()->IsToolSelectable(Player::ToolWeapon)) {
								SetSelectedTool(Player::ToolWeapon);
							} else {
								if (cg_alerts)
									ShowAlert(_Tr("Client", "Out of Ammo"), AlertType::Error);
								else
									PlayAlertSound();
							}
						}
					} else if (CheckKey(cg_keyToolGrenade, name) && down) {
						if (world->GetLocalPlayer()->GetTeamId() < 2 &&
						    world->GetLocalPlayer()->IsAlive()) {
							if (world->GetLocalPlayer()->IsToolSelectable(Player::ToolGrenade)) {
								SetSelectedTool(Player::ToolGrenade);
							} else {
								if (cg_alerts)
									ShowAlert(_Tr("Client", "Out of Grenades"), AlertType::Error);
								else
									PlayAlertSound();
							}
						}
					} else if (CheckKey(cg_keyLastTool, name) && down) {
						if (hasLastTool && world->GetLocalPlayer()->GetTeamId() < 2 &&
						    world->GetLocalPlayer()->IsAlive() &&
						    world->GetLocalPlayer()->IsToolSelectable(lastTool)) {
							hasLastTool = false;
							SetSelectedTool(lastTool);
						}
					} else if (CheckKey(cg_keyGlobalChat, name) && down) {
						// global chat
						scriptedUI->EnterGlobalChatWindow();
						scriptedUI->setIgnored(name);
					} else if (CheckKey(cg_keyTeamChat, name) && down) {
						// team chat
						scriptedUI->EnterTeamChatWindow();
						scriptedUI->setIgnored(name);
					} else if (CheckKey(cg_keyZoomChatLog, name)) {
						chatWindow->SetExpanded(down);
					} else if (name == "/" && down) {
						// command
						scriptedUI->EnterCommandWindow();
						scriptedUI->setIgnored(name);
					} else if (CheckKey(cg_keyCaptureColor, name) && down) {
						CaptureColor();
					} else if (CheckKey(cg_keyChangeMapScale, name) && down) {
						mapView->SwitchScale();
						Handle<IAudioChunk> chunk =
						  audioDevice->RegisterSound("Sounds/Misc/SwitchMapZoom.opus");
						audioDevice->PlayLocal(chunk, AudioParam());
					} else if (CheckKey(cg_keyToggleMapZoom, name) && down) {
						if (largeMapView->ToggleZoom()) {
							Handle<IAudioChunk> chunk =
							  audioDevice->RegisterSound("Sounds/Misc/OpenMap.opus");
							audioDevice->PlayLocal(chunk, AudioParam());
						} else {
							Handle<IAudioChunk> chunk =
							  audioDevice->RegisterSound("Sounds/Misc/CloseMap.opus");
							audioDevice->PlayLocal(chunk, AudioParam());
						}
					} else if (CheckKey(cg_keyScoreboard, name)) {
						scoreboardVisible = down;
					} else if (CheckKey(cg_keyLimbo, name) && down) {
						limbo->SetSelectedTeam(world->GetLocalPlayer()->GetTeamId());
						limbo->SetSelectedWeapon(
						  world->GetLocalPlayer()->GetWeapon()->GetWeaponType());
						inGameLimbo = true;
					} else if (CheckKey(cg_keySceneshot, name) && down) {
						TakeScreenShot(true);
					} else if (CheckKey(cg_keyScreenshot, name) && down) {
						TakeScreenShot(false);
					} else if (CheckKey(cg_keySaveMap, name) && down) {
						TakeMapShot();
					} else if (CheckKey(cg_keyFlashlight, name) && down) {
						// spectators and dead players should not be able to toggle the flashlight
						if (world->GetLocalPlayer()->IsSpectator() || !world->GetLocalPlayer()->IsAlive())
							return;
						flashlightOn = !flashlightOn;
						flashlightOnTime = time;
						Handle<IAudioChunk> chunk =
						  audioDevice->RegisterSound("Sounds/Player/Flashlight.opus");
						audioDevice->PlayLocal(chunk, AudioParam());
					} else if (CheckKey(cg_keyAutoFocus, name) && down && (int)cg_manualFocus) {
						autoFocusEnabled = true;
					} else if (down) {
						bool rev = (int)cg_switchToolByWheel > 0;
						if (name == (rev ? "WheelDown" : "WheelUp")) {
							if ((int)cg_manualFocus) {
								// When DoF control is enabled,
								// tool switch is overrided by focal length control.
								float dist = 1.f / targetFocalLength;
								dist = std::min(dist + 0.01f, 1.f);
								targetFocalLength = 1.f / dist;
								autoFocusEnabled = false;
							} else if (cg_switchToolByWheel &&
							           world->GetLocalPlayer()->GetTeamId() < 2 &&
							           world->GetLocalPlayer()->IsAlive()) {
								Player::ToolType t = world->GetLocalPlayer()->GetTool();
								do {
									switch (t) {
										case Player::ToolSpade: t = Player::ToolGrenade; break;
										case Player::ToolBlock: t = Player::ToolSpade; break;
										case Player::ToolWeapon: t = Player::ToolBlock; break;
										case Player::ToolGrenade: t = Player::ToolWeapon; break;
									}
								} while (!world->GetLocalPlayer()->IsToolSelectable(t));
								SetSelectedTool(t);
							}
						} else if (name == (rev ? "WheelUp" : "WheelDown")) {
							if ((int)cg_manualFocus) {
								// When DoF control is enabled,
								// tool switch is overrided by focal length control.
								float dist = 1.f / targetFocalLength;
								dist =
								  std::max(dist - 0.01f, 1.f / 128.f); // limit to fog max distance
								targetFocalLength = 1.f / dist;
								autoFocusEnabled = false;
							} else if (cg_switchToolByWheel &&
							           world->GetLocalPlayer()->GetTeamId() < 2 &&
							           world->GetLocalPlayer()->IsAlive()) {
								Player::ToolType t = world->GetLocalPlayer()->GetTool();
								do {
									switch (t) {
										case Player::ToolSpade: t = Player::ToolBlock; break;
										case Player::ToolBlock: t = Player::ToolWeapon; break;
										case Player::ToolWeapon: t = Player::ToolGrenade; break;
										case Player::ToolGrenade: t = Player::ToolSpade; break;
									}
								} while (!world->GetLocalPlayer()->IsToolSelectable(t));
								SetSelectedTool(t);
							}
						}
					}
				} else {
					// limbo
				}
			}
		}
	}
}
