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


#include "ClientPlayer.h"
#include "Player.h"
#include <ScriptBindings/ScriptFunction.h>
#include "IImage.h"
#include "IModel.h"
#include "IRenderer.h"
#include "Client.h"
#include "World.h"
#include <Core/Settings.h>
#include "CTFGameMode.h"
#include "Weapon.h"
#include "GunCasing.h"
#include <stdlib.h>
#include <ScriptBindings/IToolSkin.h>
#include <ScriptBindings/IViewToolSkin.h>
#include <ScriptBindings/IThirdPersonToolSkin.h>
#include <ScriptBindings/ISpadeSkin.h>
#include <ScriptBindings/IBlockSkin.h>
#include <ScriptBindings/IGrenadeSkin.h>
#include <ScriptBindings/IWeaponSkin.h>
#include "IAudioDevice.h"
#include "GunCasing.h"
#include "IAudioChunk.h"

SPADES_SETTING(cg_ragdoll, "");
SPADES_SETTING(cg_ejectBrass, "");

namespace spades {
	namespace client {
		ClientPlayer::ClientPlayer(Player *p,
								   Client *c):
		player(p), client(c){
			SPADES_MARK_FUNCTION();
			
			sprintState = 0.f;
			aimDownState = 0.f;
			toolRaiseState = 0.f;
			currentTool = p->GetTool();
			localFireVibrationTime = -100.f;
			time = 0.f;
			viewWeaponOffset = MakeVector3(0, 0, 0);
			
			ScriptContextHandle ctx;
			IRenderer *renderer = client->GetRenderer();
			IAudioDevice *audio = client->GetAudioDevice();
			
			static ScriptFunction spadeFactory("ISpadeSkin@ CreateThirdPersonSpadeSkin(Renderer@, AudioDevice@)");
			spadeSkin = initScriptFactory( spadeFactory, renderer, audio );
			
			static ScriptFunction spadeViewFactory("ISpadeSkin@ CreateViewSpadeSkin(Renderer@, AudioDevice@)");
			spadeViewSkin = initScriptFactory( spadeViewFactory, renderer, audio );
			
			static ScriptFunction blockFactory("IBlockSkin@ CreateThirdPersonBlockSkin(Renderer@, AudioDevice@)");
			blockSkin = initScriptFactory( blockFactory, renderer, audio );
			
			static ScriptFunction blockViewFactory("IBlockSkin@ CreateViewBlockSkin(Renderer@, AudioDevice@)");
			blockViewSkin = initScriptFactory( blockViewFactory, renderer, audio );
			
			static ScriptFunction grenadeFactory("IGrenadeSkin@ CreateThirdPersonGrenadeSkin(Renderer@, AudioDevice@)");
			grenadeSkin = initScriptFactory( grenadeFactory, renderer, audio );
			
			static ScriptFunction grenadeViewFactory("IGrenadeSkin@ CreateViewGrenadeSkin(Renderer@, AudioDevice@)");
			grenadeViewSkin = initScriptFactory( grenadeViewFactory, renderer, audio );
			
			static ScriptFunction rifleFactory("IWeaponSkin@ CreateThirdPersonRifleSkin(Renderer@, AudioDevice@)");
			static ScriptFunction smgFactory("IWeaponSkin@ CreateThirdPersonSMGSkin(Renderer@, AudioDevice@)");
			static ScriptFunction shotgunFactory("IWeaponSkin@ CreateThirdPersonShotgunSkin(Renderer@, AudioDevice@)");
			static ScriptFunction rifleViewFactory("IWeaponSkin@ CreateViewRifleSkin(Renderer@, AudioDevice@)");
			static ScriptFunction smgViewFactory("IWeaponSkin@ CreateViewSMGSkin(Renderer@, AudioDevice@)");
			static ScriptFunction shotgunViewFactory("IWeaponSkin@ CreateViewShotgunSkin(Renderer@, AudioDevice@)");
			switch(p->GetWeapon()->GetWeaponType()){
				case RIFLE_WEAPON:
					weaponSkin = initScriptFactory( rifleFactory, renderer, audio );
					weaponViewSkin = initScriptFactory( rifleViewFactory, renderer, audio );
					break;
				case SMG_WEAPON:
					weaponSkin = initScriptFactory( smgFactory, renderer, audio );
					weaponViewSkin = initScriptFactory( smgViewFactory, renderer, audio );
					break;
				case SHOTGUN_WEAPON:
					weaponSkin = initScriptFactory( shotgunFactory, renderer, audio );
					weaponViewSkin = initScriptFactory( shotgunViewFactory, renderer, audio );
					break;
				default:
					SPAssert(false);
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

		asIScriptObject* ClientPlayer::initScriptFactory( ScriptFunction& creator, IRenderer* renderer, IAudioDevice* audio )
		{
			ScriptContextHandle ctx = creator.Prepare();
			ctx->SetArgObject(0, reinterpret_cast<void*>(renderer));
			ctx->SetArgObject(1, reinterpret_cast<void*>(audio));
			ctx.ExecuteChecked();
			asIScriptObject* result = reinterpret_cast<asIScriptObject *>(ctx->GetReturnObject());
			result->AddRef();
			return result;
		}

		void ClientPlayer::Invalidate() {
			player = NULL;
		}
		
		bool ClientPlayer::IsChangingTool() {
			return currentTool != player->GetTool() ||
			toolRaiseState < .999f;
		}
		
		float ClientPlayer::GetLocalFireVibration() {
			float localFireVibration = 0.f;
			localFireVibration = time - localFireVibrationTime;
			localFireVibration = 1.f - localFireVibration / 0.1f;
			if(localFireVibration < 0.f)
				localFireVibration = 0.f;
			return localFireVibration;
		}
		
		void ClientPlayer::Update(float dt) {
			time += dt;
			
			PlayerInput actualInput = player->GetInput();
			WeaponInput actualWeapInput = player->GetWeaponInput();
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
			
			if(actualWeapInput.secondary && player->IsToolWeapon() &&
			   player->IsAlive()){
				aimDownState += dt * 6.f;
				if(aimDownState > 1.f)
					aimDownState = 1.f;
			}else{
				aimDownState -= dt * 3.f;
				if(aimDownState < 0.f)
					aimDownState = 0.f;
			}
			
			if(currentTool == player->GetTool()) {
				toolRaiseState += dt * 4.f;
				if(toolRaiseState > 1.f)
					toolRaiseState = 1.f;
				if(toolRaiseState < 0.f)
					toolRaiseState = 0.f;
			}else{
				toolRaiseState -= dt * 4.f;
				if(toolRaiseState < 0.f){
					toolRaiseState = 0.f;
					currentTool = player->GetTool();
					
					// play tool change sound
					if(player->IsLocalPlayer()) {
						auto *audioDevice = client->GetAudioDevice();
						Handle<IAudioChunk> c;
						switch(player->GetTool()) {
							case Player::ToolSpade:
								c = audioDevice->RegisterSound("Sounds/Weapons/Spade/RaiseLocal.wav");
								break;
							case Player::ToolBlock:
								c = audioDevice->RegisterSound("Sounds/Weapons/Block/RaiseLocal.wav");
								break;
							case Player::ToolWeapon:
								switch(player->GetWeapon()->GetWeaponType()){
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
						audioDevice->PlayLocal(c, MakeVector3(.4f, -.3f, .5f),
											   AudioParam());
					}
				}else if(toolRaiseState > 1.f){
					toolRaiseState = 1.f;
				}
			}
			
			{
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
				
				if(currentTool == Player::ToolWeapon &&
				   player->GetWeaponInput().secondary) {
					
					if(dt > 0.f)
						viewWeaponOffset *= powf(.01f, dt);
					
					const float limitX = .003f;
					const float limitY = .003f;
					if(viewWeaponOffset.x < -limitX)
						viewWeaponOffset.x = Mix(viewWeaponOffset.x, -limitX, .5f);
					if(viewWeaponOffset.x > limitX)
						viewWeaponOffset.x = Mix(viewWeaponOffset.x, limitX, .5f);
					if(viewWeaponOffset.z < 0.f)
						viewWeaponOffset.z = Mix(viewWeaponOffset.z, 0.f, .5f);
					if(viewWeaponOffset.z > limitY)
						viewWeaponOffset.z = Mix(viewWeaponOffset.z, limitY, .5f);
				}
			}
			
			// FIXME: should do for non-active skins?
			asIScriptObject *skin;
			if(ShouldRenderInThirdPersonView()){
				if(currentTool == Player::ToolSpade) {
					skin = spadeSkin;
				}else if(currentTool == Player::ToolBlock) {
					skin = blockSkin;
				}else if(currentTool == Player::ToolGrenade) {
					skin = grenadeSkin;
				}else if(currentTool == Player::ToolWeapon) {
					skin = weaponSkin;
				}else{
					SPInvalidEnum("currentTool", currentTool);
				}
			}else{
				if(currentTool == Player::ToolSpade) {
					skin = spadeViewSkin;
				}else if(currentTool == Player::ToolBlock) {
					skin = blockViewSkin;
				}else if(currentTool == Player::ToolGrenade) {
					skin = grenadeViewSkin;
				}else if(currentTool == Player::ToolWeapon) {
					skin = weaponViewSkin;
				}else{
					SPInvalidEnum("currentTool", currentTool);
				}
			}
			{
				ScriptIToolSkin interface(skin);
				interface.Update(dt);
			}
		}
		
		Matrix4 ClientPlayer::GetEyeMatrix() {
			Player *p = player;
			return Matrix4::FromAxis(-p->GetRight(), p->GetFront(), -p->GetUp(), p->GetEye());
		}
		
		void ClientPlayer::SetSkinParameterForTool(Player::ToolType type,
												   asIScriptObject *skin) {
			Player *p = player;
			if(currentTool == Player::ToolSpade) {
				
				ScriptISpadeSkin interface(skin);
				WeaponInput inp = p->GetWeaponInput();
				if(p->GetTool() != Player::ToolSpade){
					interface.SetActionType(SpadeActionTypeIdle);
					interface.SetActionProgress(0.f);
				}else if(inp.primary) {
					interface.SetActionType(SpadeActionTypeBash);
					interface.SetActionProgress(p->GetSpadeAnimationProgress());
				}else if(inp.secondary) {
					interface.SetActionType(p->IsFirstDig() ?
											SpadeActionTypeDigStart:
											SpadeActionTypeDig);
					interface.SetActionProgress(p->GetDigAnimationProgress());
				}else{
					interface.SetActionType(SpadeActionTypeIdle);
					interface.SetActionProgress(0.f);
				}
			}else if(currentTool == Player::ToolBlock) {
				
				// TODO: smooth ready state
				ScriptIBlockSkin interface(skin);
				if(p->GetTool() != Player::ToolBlock){
					// FIXME: use block's IsReadyToUseTool
					// for smoother transition
					interface.SetReadyState(0.f);
				}else if(p->IsReadyToUseTool()) {
					interface.SetReadyState(1.f);
				}else{
					interface.SetReadyState(0.f);
				}
				
				interface.SetBlockColor(MakeVector3(p->GetBlockColor()) / 255.f);
			}else if(currentTool == Player::ToolGrenade) {
				
				ScriptIGrenadeSkin interface(skin);
				interface.SetReadyState(1.f - p->GetTimeToNextGrenade() / 0.5f);
				
				WeaponInput inp = p->GetWeaponInput();
				if(inp.primary) {
					interface.SetCookTime(p->GetGrenadeCookTime());
				}else{
					interface.SetCookTime(0.f);
				}
			}else if(currentTool == Player::ToolWeapon) {
				
				Weapon *w = p->GetWeapon();
				ScriptIWeaponSkin interface(skin);
				interface.SetReadyState(1.f - w->TimeToNextFire() / w->GetDelay());
				interface.SetAimDownSightState(aimDownState);
				interface.SetAmmo(w->GetAmmo());
				interface.SetClipSize(w->GetClipSize());
				interface.SetReloading(w->IsReloading());
				interface.SetReloadProgress(w->GetReloadProgress());
			}else{
				SPInvalidEnum("currentTool", currentTool);
			}
		}
		
		void ClientPlayer::SetCommonSkinParameter(asIScriptObject *skin){
			asIScriptObject *curSkin;
			if(ShouldRenderInThirdPersonView()){
				if(currentTool == Player::ToolSpade) {
					curSkin = spadeSkin;
				}else if(currentTool == Player::ToolBlock) {
					curSkin = blockSkin;
				}else if(currentTool == Player::ToolGrenade) {
					curSkin = grenadeSkin;
				}else if(currentTool == Player::ToolWeapon) {
					curSkin = weaponSkin;
				}else{
					SPInvalidEnum("currentTool", currentTool);
				}
			}else{
				if(currentTool == Player::ToolSpade) {
					curSkin = spadeViewSkin;
				}else if(currentTool == Player::ToolBlock) {
					curSkin = blockViewSkin;
				}else if(currentTool == Player::ToolGrenade) {
					curSkin = grenadeViewSkin;
				}else if(currentTool == Player::ToolWeapon) {
					curSkin = weaponViewSkin;
				}else{
					SPInvalidEnum("currentTool", currentTool);
				}
			}
			
			float sprint = SmoothStep(sprintState);
			float putdown = 1.f - toolRaiseState;
			putdown *= putdown;
			putdown = std::min(1.f, putdown * 1.5f);
			{
				ScriptIToolSkin interface(skin);
				interface.SetRaiseState((skin == curSkin)?
										(1.f - putdown):
										0.f);
				interface.SetSprintState(sprint);
				interface.SetMuted(client->IsMuted());
			}
		}
		
		void ClientPlayer::AddToSceneFirstPersonView() {
			Player *p = player;
			IRenderer *renderer = client->GetRenderer();
			World *world = client->GetWorld();
			Matrix4 eyeMatrix = GetEyeMatrix();
			
			if(client->flashlightOn){
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
				renderer->SetColorAlphaPremultiplied(MakeVector4(1, .7f, .5f, 0) * brightness * .3f);
				renderer->AddSprite(renderer->RegisterImage("Gfx/Glare.tga"), (eyeMatrix * MakeVector3(0, 0.3f, -0.3f)).GetXYZ(), .8f, 0.f);
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
			
			asIScriptObject *skin;
			
			if(currentTool == Player::ToolSpade) {
				skin = spadeViewSkin;
			}else if(currentTool == Player::ToolBlock) {
				skin = blockViewSkin;
			}else if(currentTool == Player::ToolGrenade) {
				skin = grenadeViewSkin;
			}else if(currentTool == Player::ToolWeapon) {
				skin = weaponViewSkin;
			}else{
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
			if(leftHand.GetPoweredLength() > 0.001f &&
			   rightHand.GetPoweredLength() > 0.001f){
				
				
				ModelRenderParam param;
				param.depthHack = true;
				
				IModel *model = renderer->RegisterModel("Models/Player/Arm.kv6");
				IModel *model2 = renderer->RegisterModel("Models/Player/UpperArm.kv6");
				
				IntVector3 col = p->GetColor();
				param.customColor = MakeVector3(col.x/255.f, col.y/255.f, col.z/255.f);
				
				const float armlen = 0.5f;
				
				Vector3 shoulders[] = {{0.4f, 0.0f, 0.25f},
					{-0.4f, 0.0f, 0.25f}};
				Vector3 hands[] = {leftHand, rightHand};
				Vector3 benddirs[] = {{0.5f, 0.2f, 0.f},
					{-0.5f, 0.2f, 0.f}};
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
			
			
			
			if(!p->IsAlive()){
				if(!cg_ragdoll){
					ModelRenderParam param;
					param.matrix = Matrix4::Translate(p->GetOrigin()+ MakeVector3(0,0,1));
					param.matrix = param.matrix * Matrix4::Scale(.1f);
					IntVector3 col = p->GetColor();
					param.customColor = MakeVector3(col.x/255.f, col.y/255.f, col.z/255.f);
					
					IModel *model = renderer->RegisterModel("Models/Player/Dead.kv6");
					renderer->RenderModel(model, param);
				}
				return;
			}
			
			// ready for tool rendering
			asIScriptObject *skin;
			
			if(currentTool == Player::ToolSpade) {
				skin = spadeSkin;
			}else if(currentTool == Player::ToolBlock) {
				skin = blockSkin;
			}else if(currentTool == Player::ToolGrenade) {
				skin = grenadeSkin;
			}else if(currentTool == Player::ToolWeapon) {
				skin = weaponSkin;
			}else{
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
			param.customColor = MakeVector3(col.x/255.f, col.y/255.f, col.z/255.f);
			
			float yaw = atan2(front.y, front.x) + M_PI * .5f;
			float pitch = -atan2(front.z, sqrt(front.x * front.x + front.y * front.y));
			
			// lower axis
			Matrix4 lower = Matrix4::Translate(p->GetOrigin());
			lower = lower * Matrix4::Rotate(MakeVector3(0,0,1), yaw);
			
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
				leg1 = leg1 * Matrix4::Rotate(MakeVector3(1,0,0), ang * walkVel);
				leg2 = leg2 * Matrix4::Rotate(MakeVector3(1,0,0), -ang * walkVel);
				
				walkVel = Vector3::Dot(p->GetVelocty(), p->GetRight()) * 3.f;
				leg1 = leg1 * Matrix4::Rotate(MakeVector3(0,1,0), ang * walkVel);
				leg2 = leg2 * Matrix4::Rotate(MakeVector3(0,1,0), -ang * walkVel);
				
				leg1 = lower * leg1;
				leg2 = lower * leg2;
				
				model = renderer->RegisterModel("Models/Player/LegCrouch.kv6");
				param.matrix = leg1 * scaler;
				renderer->RenderModel(model, param);
				param.matrix = leg2 * scaler;
				renderer->RenderModel(model, param);
				
				torso = Matrix4::Translate(0.f,0.f,-0.55f);
				torso = lower * torso;
				
				model = renderer->RegisterModel("Models/Player/TorsoCrouch.kv6");
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
				leg1 = leg1 * Matrix4::Rotate(MakeVector3(1,0,0), ang * walkVel);
				leg2 = leg2 * Matrix4::Rotate(MakeVector3(1,0,0), -ang * walkVel);
				
				walkVel = Vector3::Dot(p->GetVelocty(), p->GetRight()) * 3.f;
				leg1 = leg1 * Matrix4::Rotate(MakeVector3(0,1,0), ang * walkVel);
				leg2 = leg2 * Matrix4::Rotate(MakeVector3(0,1,0), -ang * walkVel);
				
				leg1 = lower * leg1;
				leg2 = lower * leg2;
				
				model = renderer->RegisterModel("Models/Player/Leg.kv6");
				param.matrix = leg1 * scaler;
				renderer->RenderModel(model, param);
				param.matrix = leg2 * scaler;
				renderer->RenderModel(model, param);
				
				torso = Matrix4::Translate(0.f,0.f,-1.0f);
				torso = lower * torso;
				
				model = renderer->RegisterModel("Models/Player/Torso.kv6");
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
			armPitch += pitchBias;
			if(armPitch < 0.f) {
				armPitch = std::max(armPitch, -(float)M_PI * .5f);
				armPitch *= .9f;
			}
			
			arms = arms * Matrix4::Rotate(MakeVector3(1,0,0), armPitch);
			
			model = renderer->RegisterModel("Models/Player/Arms.kv6");
			param.matrix = arms * scaler;
			renderer->RenderModel(model, param);
			
			
			head = head * Matrix4::Rotate(MakeVector3(1,0,0), pitch);
			
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
			
			
			// draw intel in ctf
			IGameMode* mode = world->GetMode();
			if( mode && IGameMode::m_CTF == mode->ModeType() ){
				CTFGameMode *ctfMode = static_cast<CTFGameMode *>(world->GetMode());
				int tId = p->GetTeamId();
				if(tId < 3){
					CTFGameMode::Team& team = ctfMode->GetTeam(p->GetTeamId());
					if(team.hasIntel && team.carrier == p->GetId()){
						
						IntVector3 col2 = world->GetTeam(1-p->GetTeamId()).color;
						param.customColor = MakeVector3(col2.x/255.f, col2.y/255.f, col2.z/255.f);
						Matrix4 mIntel = torso * Matrix4::Translate(0,0.6f,0.5f);
						
						model = renderer->RegisterModel("Models/MapObjects/Intel.kv6");
						param.matrix = mIntel * scaler;
						renderer->RenderModel(model, param);
						
						param.customColor = MakeVector3(col.x/255.f, col.y/255.f, col.z/255.f);
					}
				}
			}
			
			// third person player rendering, done

		}
		
		void ClientPlayer::AddToScene() {
			SPADES_MARK_FUNCTION();
			
			Player *p = player;
			IRenderer *renderer = client->GetRenderer();
			const SceneDefinition& lastSceneDef = client->GetLastSceneDef();
			
			if(p->GetTeamId() >= 2){
				// spectator, or dummy player
				return;
				
			}
			// debug
			if(false){
				Handle<IImage> img = renderer->RegisterImage("Gfx/Ball.png");
				renderer->SetColorAlphaPremultiplied(MakeVector4(1, 0, 0, 0));
				renderer->AddLongSprite(img, lastSceneDef.viewOrigin + MakeVector3(0, 0, 1), p->GetOrigin(), 0.5f);
			}
			
			float distancePowered = (p->GetOrigin() - lastSceneDef.viewOrigin).GetPoweredLength();
			if(distancePowered > 140.f * 140.f){
				return;
			}
			
			if(!ShouldRenderInThirdPersonView()){
				AddToSceneFirstPersonView();
			} else {
				AddToSceneThirdPersonView();
			}
		}
		
		void ClientPlayer::Draw2D() {
			if(!ShouldRenderInThirdPersonView()) {
				asIScriptObject *skin;
				
				if(currentTool == Player::ToolSpade) {
					skin = spadeViewSkin;
				}else if(currentTool == Player::ToolBlock) {
					skin = blockViewSkin;
				}else if(currentTool == Player::ToolGrenade) {
					skin = grenadeViewSkin;
				}else if(currentTool == Player::ToolWeapon) {
					skin = weaponViewSkin;
				}else{
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
			if(player != player->GetWorld()->GetLocalPlayer())
				return true;
			return client->ShouldRenderInThirdPersonView();
		}
		
		void ClientPlayer::FiredWeapon() {
			World *world = player->GetWorld();
			Vector3 muzzle;
			const SceneDefinition& lastSceneDef = client->GetLastSceneDef();
			IRenderer *renderer = client->GetRenderer();
			IAudioDevice *audioDevice = client->GetAudioDevice();
			Player *p = player;
			
			// make dlight
			{
				Vector3 vec;
				Matrix4 eyeMatrix = GetEyeMatrix();
				Matrix4 mat;
				mat = Matrix4::Translate(-0.13f,
										 .5f,
										 0.2f);
				mat = eyeMatrix * mat;
				
				vec = (mat * MakeVector3(0, 1, 0)).GetXYZ();
				muzzle = vec;
				client->MuzzleFire(vec, player->GetFront(), player == world->GetLocalPlayer());
			}
			
			if(cg_ejectBrass){
				float dist = (player->GetOrigin() - lastSceneDef.viewOrigin).GetPoweredLength();
				if(dist < 130.f * 130.f) {
					IModel *model = NULL;
					Handle<IAudioChunk> snd = NULL;
					Handle<IAudioChunk> snd2 = NULL;
					switch(player->GetWeapon()->GetWeaponType()){
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
						ent = new GunCasing(client, model, snd, snd2,
											origin, p->GetFront(),
											vel);
						client->AddLocalEntity(ent);
						
					}
				}
			}
			
			asIScriptObject *skin;
			// FIXME: what if current tool isn't weapon?
			if(ShouldRenderInThirdPersonView()){
				skin = weaponSkin;
			}else{
				skin = weaponViewSkin;
			}
			
			{
				ScriptIWeaponSkin interface(skin);
				interface.WeaponFired();
			}
	
		}
		
		void ClientPlayer::ReloadingWeapon() {
			asIScriptObject *skin;
			// FIXME: what if current tool isn't weapon?
			if(ShouldRenderInThirdPersonView()){
				skin = weaponSkin;
			}else{
				skin = weaponViewSkin;
			}
			
			{
				ScriptIWeaponSkin interface(skin);
				interface.ReloadingWeapon();
			}
		}
		
		void ClientPlayer::ReloadedWeapon() {
			asIScriptObject *skin;
			// FIXME: what if current tool isn't weapon?
			if(ShouldRenderInThirdPersonView()){
				skin = weaponSkin;
			}else{
				skin = weaponViewSkin;
			}

			{
				ScriptIWeaponSkin interface(skin);
				interface.ReloadedWeapon();
			}
		}
	}
}
