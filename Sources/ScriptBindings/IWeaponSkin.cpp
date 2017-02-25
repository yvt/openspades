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

#include "ScriptManager.h"
#include "IWeaponSkin.h"
#include <Core/Debug.h>

namespace spades{
	namespace client {
		ScriptIWeaponSkin::ScriptIWeaponSkin(asIScriptObject *obj):
		obj(obj) {
			SPAssert(obj);
			SPAssert(obj->GetObjectType());
		}

		bool ScriptIWeaponSkin::ImplementsInterface()
		{
			return obj->GetObjectType()->Implements(obj->GetEngine()->GetTypeInfoByName("IWeaponSkin"));
		}
		
		void ScriptIWeaponSkin::SetReadyState(float v) {
			SPADES_MARK_FUNCTION_DEBUG();
			static ScriptFunction func("IWeaponSkin",
									   "void set_ReadyState(float)");
			ScriptContextHandle ctx = func.Prepare();
			int r;
			r = ctx->SetObject((void *)obj);
			ScriptManager::CheckError(r);
			r = ctx->SetArgFloat(0, v);
			ScriptManager::CheckError(r);
			ctx.ExecuteChecked();
		}
		
		void ScriptIWeaponSkin::SetAimDownSightState(float v) {
			SPADES_MARK_FUNCTION_DEBUG();
			static ScriptFunction func("IWeaponSkin",
									   "void set_AimDownSightState(float)");
			ScriptContextHandle ctx = func.Prepare();
			int r;
			r = ctx->SetObject((void *)obj);
			ScriptManager::CheckError(r);
			r = ctx->SetArgFloat(0, v);
			ScriptManager::CheckError(r);
			ctx.ExecuteChecked();
		}
		
		void ScriptIWeaponSkin::SetReloadProgress(float v) {
			SPADES_MARK_FUNCTION_DEBUG();
			static ScriptFunction func("IWeaponSkin",
									   "void set_ReloadProgress(float)");
			ScriptContextHandle ctx = func.Prepare();
			int r;
			r = ctx->SetObject((void *)obj);
			ScriptManager::CheckError(r);
			r = ctx->SetArgFloat(0, v);
			ScriptManager::CheckError(r);
			ctx.ExecuteChecked();
		}
		
		void ScriptIWeaponSkin::SetReloading(bool v) {
			SPADES_MARK_FUNCTION_DEBUG();
			static ScriptFunction func("IWeaponSkin",
									   "void set_IsReloading(bool)");
			ScriptContextHandle ctx = func.Prepare();
			int r;
			r = ctx->SetObject((void *)obj);
			ScriptManager::CheckError(r);
			r = ctx->SetArgByte(0, v?1:0);
			ScriptManager::CheckError(r);
			ctx.ExecuteChecked();
		}
		
		void ScriptIWeaponSkin::SetAmmo(int v) {
			SPADES_MARK_FUNCTION_DEBUG();
			static ScriptFunction func("IWeaponSkin",
									   "void set_Ammo(int)");
			ScriptContextHandle ctx = func.Prepare();
			int r;
			r = ctx->SetObject((void *)obj);
			ScriptManager::CheckError(r);
			r = ctx->SetArgDWord(0, (asDWORD)v);
			ScriptManager::CheckError(r);
			ctx.ExecuteChecked();
		}
		
		void ScriptIWeaponSkin::SetClipSize(int v) {
			SPADES_MARK_FUNCTION_DEBUG();
			static ScriptFunction func("IWeaponSkin",
									   "void set_ClipSize(int)");
			ScriptContextHandle ctx = func.Prepare();
			int r;
			r = ctx->SetObject((void *)obj);
			ScriptManager::CheckError(r);
			r = ctx->SetArgDWord(0, (asDWORD)v);
			ScriptManager::CheckError(r);
			ctx.ExecuteChecked();
		}
		
		void ScriptIWeaponSkin::WeaponFired() {
			SPADES_MARK_FUNCTION_DEBUG();
			static ScriptFunction func("IWeaponSkin",
									   "void WeaponFired()");
			ScriptContextHandle ctx = func.Prepare();
			int r;
			r = ctx->SetObject((void *)obj);
			ScriptManager::CheckError(r);
			ctx.ExecuteChecked();
		}
		
		void ScriptIWeaponSkin::ReloadingWeapon() {
			SPADES_MARK_FUNCTION_DEBUG();
			static ScriptFunction func("IWeaponSkin",
									   "void ReloadingWeapon()");
			ScriptContextHandle ctx = func.Prepare();
			int r;
			r = ctx->SetObject((void *)obj);
			ScriptManager::CheckError(r);
			ctx.ExecuteChecked();
		}
		
		void ScriptIWeaponSkin::ReloadedWeapon() {
			SPADES_MARK_FUNCTION_DEBUG();
			static ScriptFunction func("IWeaponSkin",
									   "void ReloadedWeapon()");
			ScriptContextHandle ctx = func.Prepare();
			int r;
			r = ctx->SetObject((void *)obj);
			ScriptManager::CheckError(r);
			ctx.ExecuteChecked();
		}

		ScriptIWeaponSkin2::ScriptIWeaponSkin2(asIScriptObject *obj):
		obj(obj) {
			SPAssert(obj);
			SPAssert(obj->GetObjectType());
		}

		bool ScriptIWeaponSkin2::ImplementsInterface()
		{
			return obj->GetObjectType()->Implements(obj->GetEngine()->GetTypeInfoByName("IWeaponSkin2"));
		}

		void ScriptIWeaponSkin2::SetSoundEnvironment(float room, float size, float distance) {
			SPADES_MARK_FUNCTION_DEBUG();
			static ScriptFunction func("IWeaponSkin2",
									   "void SetSoundEnvironment(float, float, float)");
			ScriptContextHandle ctx = func.Prepare();
			int r;
			r = ctx->SetObject((void *)obj);
			ScriptManager::CheckError(r);
			r = ctx->SetArgFloat(0, room);
			ScriptManager::CheckError(r);
			r = ctx->SetArgFloat(1, size);
			ScriptManager::CheckError(r);
			r = ctx->SetArgFloat(2, distance);
			ScriptManager::CheckError(r);
			ctx.ExecuteChecked();
		}

		void ScriptIWeaponSkin2::SetSoundOrigin(Vector3 origin) {
			SPADES_MARK_FUNCTION_DEBUG();
			static ScriptFunction func("IWeaponSkin2",
									   "void set_SoundOrigin(Vector3)");
			ScriptContextHandle ctx = func.Prepare();
			int r;
			r = ctx->SetObject((void *)obj);
			ScriptManager::CheckError(r);
			r = ctx->SetArgObject(0, &origin);
			ScriptManager::CheckError(r);
			ctx.ExecuteChecked();
		}

		class IWeaponSkinRegistrar: public ScriptObjectRegistrar {
		public:
			IWeaponSkinRegistrar():
			ScriptObjectRegistrar("IWeaponSkin"){
				
			}
			virtual void Register(ScriptManager *manager, Phase phase) {
				asIScriptEngine *eng = manager->GetEngine();
				int r;
				eng->SetDefaultNamespace("spades");
				switch(phase){
					case PhaseObjectType:
						r = eng->RegisterInterface("IWeaponSkin");
						manager->CheckError(r);
						r = eng->RegisterInterface("IWeaponSkin2");
						manager->CheckError(r);
						break;
					case PhaseObjectMember:
						r = eng->RegisterInterfaceMethod("IWeaponSkin",
														 "void set_ReadyState(float)");
						manager->CheckError(r);
						r = eng->RegisterInterfaceMethod("IWeaponSkin",
														 "void set_AimDownSightState(float)");
						manager->CheckError(r);
						r = eng->RegisterInterfaceMethod("IWeaponSkin",
														 "void set_IsReloading(bool)");
						manager->CheckError(r);
						r = eng->RegisterInterfaceMethod("IWeaponSkin",
														 "void set_ReloadProgress(float)");
						manager->CheckError(r);
						r = eng->RegisterInterfaceMethod("IWeaponSkin",
														 "void set_Ammo(int)");
						manager->CheckError(r);
						r = eng->RegisterInterfaceMethod("IWeaponSkin",
														 "void set_ClipSize(int)");
						manager->CheckError(r);
						r = eng->RegisterInterfaceMethod("IWeaponSkin",
														 "void WeaponFired()");
						manager->CheckError(r);
						r = eng->RegisterInterfaceMethod("IWeaponSkin",
														 "void ReloadingWeapon()");
						manager->CheckError(r);
						r = eng->RegisterInterfaceMethod("IWeaponSkin",
														 "void ReloadedWeapon()");
						manager->CheckError(r);

						r = eng->RegisterInterfaceMethod("IWeaponSkin2",
														 "void SetSoundEnvironment(float, float, float)");
						r = eng->RegisterInterfaceMethod("IWeaponSkin2",
														 "void set_SoundOrigin(Vector3)");
						manager->CheckError(r);
						break;
					default:
						break;
				}
			}
		};
		
		static IWeaponSkinRegistrar registrar;
	}
}

