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

#include "IImage.h"

namespace spades{
	namespace client {
		class LowLevelNativeImage {
			friend class IImage;
			IImage *img;
			
			int refCount;
			bool MakeSureAvailable() {
				if(!img) {
					asIScriptContext *ctx = asGetActiveContext();
					ctx->SetException("Image is unavailable.");
					return false;
				}
				return true;
			}
		public:
			LowLevelNativeImage(IImage *img):img(img),refCount(1){
				
			}
			~LowLevelNativeImage() {
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
			float GetWidth() {
				if(!MakeSureAvailable()) return 0.f;
				return img->GetWidth();
			}
			float GetHeight() {
				if(!MakeSureAvailable()) return 0.f;
				return img->GetHeight();
			}
		};
		
		IImage::IImage() {
			lowLevelNativeImage = NULL;
			scriptImage = NULL;
		}
		
		IImage::~IImage() {
			if(lowLevelNativeImage){
				lowLevelNativeImage->img = NULL;
				lowLevelNativeImage->Release();
			}
		}
		
		LowLevelNativeImage *IImage::GetLowLevelNativeImage(bool addRef) {
			if(!lowLevelNativeImage){
				lowLevelNativeImage = new LowLevelNativeImage(this);
			}
			if(addRef)
				lowLevelNativeImage->AddRef();
			return lowLevelNativeImage;
		}
		
		asIScriptObject *IImage::GetScriptImage() {
			if(!scriptImage){
				
				asIScriptEngine *eng = ScriptManager::GetInstance()->GetEngine();
				asIObjectType *typ = eng->GetObjectTypeByName("spades::NativeImage");
				asIScriptFunction *func = typ->GetFactoryByDecl("NativeImage @NativeImage(LowLevelNativeImage@)");
				
				ScriptContextHandle handle = ScriptManager::GetInstance()->GetContext();
				handle->Prepare(func);
				handle->SetArgObject(0, GetLowLevelNativeImage(false));
				handle.ExecuteChecked();
				
				scriptImage = static_cast<asIScriptObject *>(handle->GetReturnObject());
			}
			return scriptImage;
		}
		
		class LowLevelNativeImageRegistrar: public ScriptObjectRegistrar {
		public:
			LowLevelNativeImageRegistrar():
			ScriptObjectRegistrar("LowLevelNativeImage"){
				
			}
			virtual void Register(ScriptManager *manager, Phase phase) {
				asIScriptEngine *eng = manager->GetEngine();
				int r;
				eng->SetDefaultNamespace("spades");
				switch(phase){
					case PhaseObjectType:
						r = eng->RegisterObjectType("LowLevelNativeImage",
													0, asOBJ_REF);
						manager->CheckError(r);
						r = eng->RegisterObjectBehaviour("LowLevelNativeImage",
														 asBEHAVE_ADDREF, "void f()",
														 asMETHOD(LowLevelNativeImage, AddRef),
														 asCALL_THISCALL);
						manager->CheckError(r);
						r = eng->RegisterObjectBehaviour("LowLevelNativeImage",
														 asBEHAVE_RELEASE, "void f()",
														 asMETHOD(LowLevelNativeImage, Release),
														 asCALL_THISCALL);
						manager->CheckError(r);
						
						break;
					case PhaseObjectMember:
						r = eng->RegisterObjectMethod("LowLevelNativeImage",
													  "float get_Width()",
													  asMETHOD(LowLevelNativeImage, GetWidth),
													  asCALL_THISCALL);
						manager->CheckError(r);
						r = eng->RegisterObjectMethod("LowLevelNativeImage",
													  "float get_Height()",
													  asMETHOD(LowLevelNativeImage, GetHeight),
													  asCALL_THISCALL);
						manager->CheckError(r);
						break;
					default:
						
						break;
				}
			}
		};
		
		static LowLevelNativeImageRegistrar registrar;
	}
}

