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

#include "IRenderer.h"
#include <Core/ScriptManager.h>

namespace spades {
	namespace client{
	
		class LowLevelNativeRenderer {
			friend class IRenderer;
			IRenderer *renderer;
			int refCount;
			
			bool MakeSureAvailable() {
				if(!renderer) {
					asIScriptContext *ctx = asGetActiveContext();
					ctx->SetException("Renderer is unavailable.");
					return false;
				}
				return true;
			}
		public:
			LowLevelNativeRenderer(IRenderer *r):
			renderer(r){
				refCount = 1;
			}
			
			void AddRef() {
				asAtomicInc(refCount);
			}
			
			void Release() {
				if(asAtomicDec(refCount) <= 0) {
					delete this;
				}
			}
			
			IRenderer *GetRenderer() { return renderer; }
			
			LowLevelNativeImage *RegisterImage(const std::string& fn) {
				if(!MakeSureAvailable()){
					return NULL;
				}
				try{
					IImage *img = renderer->RegisterImage(fn.c_str());
					LowLevelNativeImage *obj = img->GetLowLevelNativeImage(true);
					return obj;
				}catch(const std::exception& ex){
					ScriptContextUtils().SetNativeException(ex);
					return NULL;
				}
			}
			LowLevelNativeModel *RegisterModel(const std::string& fn) {
				if(!MakeSureAvailable()){
					return NULL;
				}
				try{
					IModel *img = renderer->RegisterModel(fn.c_str());
					LowLevelNativeModel *obj = img->GetLowLevelNativeModel(true);
					return obj;
				}catch(const std::exception& ex){
					ScriptContextUtils().SetNativeException(ex);
					return NULL;
				}
			}
			LowLevelNativeImage *CreateImage(Bitmap *bmp) {
				if(!MakeSureAvailable()){
					return NULL;
				}
				try{
					IImage *img = renderer->CreateImage(bmp);
					LowLevelNativeImage *obj = img->GetLowLevelNativeImage(false);
					return obj;
				}catch(const std::exception& ex){
					ScriptContextUtils().SetNativeException(ex);
					return NULL;
				}
			}
			LowLevelNativeModel *CreateModel(VoxelModel *model) {
				if(!MakeSureAvailable()){
					return NULL;
				}
				try{
					IModel *img = renderer->CreateModel(model);
					LowLevelNativeModel *obj = img->GetLowLevelNativeModel(false);
					return obj;
				}catch(const std::exception& ex){
					ScriptContextUtils().SetNativeException(ex);
					return NULL;
				}
			}
		};
		
		IRenderer::IRenderer() {
			scriptLLRenderer = NULL;
			scriptRenderer = NULL;
		}
		
		IRenderer::~IRenderer() {
			if(scriptLLRenderer){
				scriptLLRenderer->renderer = NULL;
				scriptLLRenderer->Release();
			}
		}
		
		LowLevelNativeRenderer *IRenderer::GetLowLevelNativeRenderer() {
			if(!scriptLLRenderer){
				scriptLLRenderer = new LowLevelNativeRenderer(this);
			}
		}
		
		asIScriptObject *IRenderer::GetScriptRenderer() {
			if(!scriptRenderer){
				
				asIScriptEngine *eng = ScriptManager::GetInstance()->GetEngine();
				asIObjectType *typ = eng->GetObjectTypeByName("spades::NativeRenderer");
				asIScriptFunction *func = typ->GetFactoryByDecl("NativeRenderer @NativeRenderer(LowLevelNativeRenderer@)");
				
				ScriptContextHandle handle = ScriptManager::GetInstance()->GetContext();
				handle->Prepare(func);
				handle->SetArgObject(0, GetLowLevelNativeRenderer());
				handle.ExecuteChecked();
				
				scriptRenderer = static_cast<asIScriptObject *>(handle->GetReturnObject());
			}
			return scriptRenderer;
		}
		
		class LowLevelNativeRendererRegistrar: public ScriptObjectRegistrar {
		public:
			LowLevelNativeRendererRegistrar():
			ScriptObjectRegistrar("LowLevelNativeRenderer"){
				
			}
			virtual void Register(ScriptManager *manager, Phase phase) {
				asIScriptEngine *eng = manager->GetEngine();
				int r;
				eng->SetDefaultNamespace("spades");
				switch(phase){
					case PhaseObjectType:
						r = eng->RegisterObjectType("LowLevelNativeRenderer",
													0, asOBJ_REF);
						manager->CheckError(r);
						r = eng->RegisterObjectBehaviour("LowLevelNativeRenderer",
														 asBEHAVE_ADDREF, "void f()",
														 asMETHOD(LowLevelNativeRenderer, AddRef),
														 asCALL_THISCALL);
						manager->CheckError(r);
						r = eng->RegisterObjectBehaviour("LowLevelNativeRenderer",
														 asBEHAVE_RELEASE, "void f()",
														 asMETHOD(LowLevelNativeRenderer, Release),
														 asCALL_THISCALL);
						manager->CheckError(r);
						
						break;
					case PhaseObjectMember:
						r = eng->RegisterObjectMethod("LowLevelNativeRenderer",
													  "LowLevelNativeImage@ RegisterImage(const string& in)",
													  asMETHOD(LowLevelNativeRenderer, RegisterImage),
													  asCALL_THISCALL);
						manager->CheckError(r);
						r = eng->RegisterObjectMethod("LowLevelNativeRenderer",
													  "LowLevelNativeModel@ RegisterModel(const string& in)",
													  asMETHOD(LowLevelNativeRenderer, RegisterModel),
													  asCALL_THISCALL);
						manager->CheckError(r);
						r = eng->RegisterObjectMethod("LowLevelNativeRenderer",
													  "LowLevelNativeImage@ CreateImage(Bitmap@)",
													  asMETHOD(LowLevelNativeRenderer, CreateImage),
													  asCALL_THISCALL);
						manager->CheckError(r);
						r = eng->RegisterObjectMethod("LowLevelNativeRenderer",
													  "LowLevelNativeModel@ CreateModel(VoxelModel@)",
													  asMETHOD(LowLevelNativeRenderer, CreateModel),
													  asCALL_THISCALL);
						manager->CheckError(r);
						break;
					default:
						break;
				}
			}
		};
		
		static LowLevelNativeRendererRegistrar registrar;
	}
}
