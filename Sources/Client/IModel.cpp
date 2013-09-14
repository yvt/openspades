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

#include "IModel.h"

namespace spades{
	namespace client {
		class LowLevelNativeModel {
			friend class IModel;
			IModel *img;
			
			int refCount;
			bool MakeSureAvailable() {
				if(!img) {
					asIScriptContext *ctx = asGetActiveContext();
					ctx->SetException("Model is unavailable.");
					return false;
				}
				return true;
			}
		public:
			LowLevelNativeModel(IModel *img):img(img),refCount(1){
				
			}
			~LowLevelNativeModel() {
				if(img){
					delete img;
				}
			}
			void AddRef() {
				asAtomicInc(refCount);
			}
			void Release() {
				if(asAtomicDec(refCount) <= 0) {
					delete this;
				}
			}
		};
		
		IModel::IModel() {
			lowLevelNativeModel = NULL;
			scriptModel = NULL;
		}
		
		IModel::~IModel() {
			if(lowLevelNativeModel){
				lowLevelNativeModel->img = NULL;
				lowLevelNativeModel->Release();
			}
		}
		
		LowLevelNativeModel *IModel::GetLowLevelNativeModel(bool addRef) {
			if(!lowLevelNativeModel){
				lowLevelNativeModel = new LowLevelNativeModel(this);
			}
			if(addRef)
				lowLevelNativeModel->AddRef();
			return lowLevelNativeModel;
		}
		
		asIScriptObject *IModel::GetScriptModel() {
			if(!scriptModel){
				
				asIScriptEngine *eng = ScriptManager::GetInstance()->GetEngine();
				asIObjectType *typ = eng->GetObjectTypeByName("spades::NativeModel");
				asIScriptFunction *func = typ->GetFactoryByDecl("NativeModel @NativeModel(LowLevelNativeModel@)");
				
				ScriptContextHandle handle = ScriptManager::GetInstance()->GetContext();
				handle->Prepare(func);
				handle->SetArgObject(0, GetLowLevelNativeModel(false));
				handle.ExecuteChecked();
				
				scriptModel = static_cast<asIScriptObject *>(handle->GetReturnObject());
			}
			return scriptModel;
		}
		
		class LowLevelNativeModelRegistrar: public ScriptObjectRegistrar {
		public:
			LowLevelNativeModelRegistrar():
			ScriptObjectRegistrar("LowLevelNativeModel"){
				
			}
			virtual void Register(ScriptManager *manager, Phase phase) {
				asIScriptEngine *eng = manager->GetEngine();
				int r;
				eng->SetDefaultNamespace("spades");
				switch(phase){
					case PhaseObjectType:
						r = eng->RegisterObjectType("LowLevelNativeModel",
													0, asOBJ_REF);
						manager->CheckError(r);
						r = eng->RegisterObjectBehaviour("LowLevelNativeModel",
														 asBEHAVE_ADDREF, "void f()",
														 asMETHOD(LowLevelNativeModel, AddRef),
														 asCALL_THISCALL);
						manager->CheckError(r);
						r = eng->RegisterObjectBehaviour("LowLevelNativeModel",
														 asBEHAVE_RELEASE, "void f()",
														 asMETHOD(LowLevelNativeModel, Release),
														 asCALL_THISCALL);
						manager->CheckError(r);
						
						break;
					case PhaseObjectMember:
						break;
					default:
						
						break;
				}
			}
		};
		
		static LowLevelNativeModelRegistrar registrar;
	}
}
