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
#include <Client/IRenderer.h>
#include <Client/SceneDefinition.h>

namespace spades {
	namespace client{
		
		
		
		class RendererRegistrar: public ScriptObjectRegistrar {
			static IModel *RegisterModel(const std::string& str,
										 IRenderer *r) {
				try{
					IModel *m = r->RegisterModel(str.c_str());
					m->AddRef();
					return m;
				}catch(const std::exception& ex) {
					ScriptContextUtils().SetNativeException(ex);
					return NULL;
				}
			}
			static IImage *RegisterImage(const std::string& str,
										 IRenderer *r) {
				try{
					IImage *im = r->RegisterImage(str.c_str());
					im->AddRef();
					return im;
				}catch(const std::exception& ex) {
					ScriptContextUtils().SetNativeException(ex);
					return nullptr;
				}
			}
			static void AddDebugLine(const Vector3& a, const Vector3& b,
									 const Vector4& color,
									 IRenderer *r) {
				try{
					return r->AddDebugLine(a, b, color);
				}catch(const std::exception& ex) {
					ScriptContextUtils().SetNativeException(ex);
				}
			}
			static void AddSprite(IImage *img, const Vector3& center,
								  float rad, float rot,
								  IRenderer *r) {
				try{
					return r->AddSprite(img, center, rad, rot);
				}catch(const std::exception& ex) {
					ScriptContextUtils().SetNativeException(ex);
				}
			}
			static void AddLongSprite(IImage *img,
									  const Vector3& p1,
									  const Vector3& p2,
								  float rad, 
								  IRenderer *r) {
				try{
					return r->AddLongSprite(img, p1, p2, rad);
				}catch(const std::exception& ex) {
					ScriptContextUtils().SetNativeException(ex);
				}
			}
			static void MultiplyScreenColor(const Vector3& v,
											IRenderer *r){
				try{
					return r->MultiplyScreenColor(v);
				}catch(const std::exception& ex) {
					ScriptContextUtils().SetNativeException(ex);
				}
			}
			static void SetColor(const Vector4& v,
								 IRenderer *r){
				try{
					return r->SetColor(v);
				}catch(const std::exception& ex) {
					ScriptContextUtils().SetNativeException(ex);
				}
			}
			static void SetColorOpaque(const Vector3& v,
								 IRenderer *r){
				try{
					return r->SetColorAlphaPremultiplied(MakeVector4(v.x,v.y,v.z,1.f));
				}catch(const std::exception& ex) {
					ScriptContextUtils().SetNativeException(ex);
				}
			}
			static void SetColorAlphaPremultiplied(const Vector4& v,
								 IRenderer *r){
				try{
					return r->SetColorAlphaPremultiplied(v);
				}catch(const std::exception& ex) {
					ScriptContextUtils().SetNativeException(ex);
				}
			}
			static void SetColorAlphaNonPremultiplied(const Vector4& v,
												   IRenderer *r){
				Vector4 v2 = {v.x * v.w, v.y * v.w, v.z * v.w, v.w};
				try{
					return r->SetColorAlphaPremultiplied(v2);
				}catch(const std::exception& ex) {
					ScriptContextUtils().SetNativeException(ex);
				}
			}
			
			static void SetFogColor(const Vector3& v,
								 IRenderer *r){
				try{
					return r->SetFogColor(v);
				}catch(const std::exception& ex) {
					ScriptContextUtils().SetNativeException(ex);
				}
			}
			static void ModelRenderParamFactory(ModelRenderParam *p) {
				new(p) ModelRenderParam();
			}
			static void DynamicLightParamFactory(DynamicLightParam *p) {
				new(p) DynamicLightParam();
			}
			static void DynamicLightParamFactory2(const DynamicLightParam& other, DynamicLightParam *p) {
				new(p) DynamicLightParam(other);
				if(p->image)
					p->image->AddRef();
			}
			static void DynamicLightParamAssign(const DynamicLightParam& other,
												DynamicLightParam *self) {
				IImage *old = self->image;
				*self = other;
				self->image->AddRef();
				old->Release();
			}
			static void DynamicLightParamDestructor(DynamicLightParam *p) {
				if(p->image)
					p->image->Release();
			}
			static void SceneDefinitionFactory(SceneDefinition *def) {
				new(def) SceneDefinition();
			}
		public:
			RendererRegistrar():
			ScriptObjectRegistrar("Renderer"){
				
			}
			virtual void Register(ScriptManager *manager, Phase phase) {
				asIScriptEngine *eng = manager->GetEngine();
				int r;
				eng->SetDefaultNamespace("spades");
				switch(phase){
					case PhaseObjectType:
						r = eng->RegisterObjectType("Renderer",
													0, asOBJ_REF);
						manager->CheckError(r);
						r = eng->RegisterObjectBehaviour("Renderer",
														 asBEHAVE_ADDREF, "void f()",
														 asMETHOD(IRenderer, AddRef),
														 asCALL_THISCALL);
						manager->CheckError(r);
						r = eng->RegisterObjectBehaviour("Renderer",
														 asBEHAVE_RELEASE, "void f()",
														 asMETHOD(IRenderer, Release),
														 asCALL_THISCALL);
						manager->CheckError(r);
						
						
						r = eng->RegisterObjectType("ModelRenderParam",
													sizeof(ModelRenderParam), asOBJ_VALUE|asOBJ_POD|asOBJ_APP_CLASS_CDAK);
						manager->CheckError(r);
						r = eng->RegisterObjectType("DynamicLightParam",
													sizeof(DynamicLightParam), asOBJ_VALUE|asOBJ_APP_CLASS_CDAK);
						manager->CheckError(r);
						r = eng->RegisterObjectType("SceneDefinition",
													sizeof(SceneDefinition), asOBJ_VALUE|asOBJ_POD|asOBJ_APP_CLASS_CDAK);
						manager->CheckError(r);
						r = eng->RegisterEnum("DynamicLightType");
						manager->CheckError(r);
						
						
						break;
					case PhaseObjectMember:
						r = eng->RegisterEnumValue("DynamicLightType", "Point",
												   DynamicLightTypePoint);
						manager->CheckError(r);
						r = eng->RegisterEnumValue("DynamicLightType", "Spotlight",
												   DynamicLightTypeSpotlight);
						manager->CheckError(r);
						
						r = eng->RegisterObjectBehaviour("ModelRenderParam",
														 asBEHAVE_CONSTRUCT,
														 "void f()",
														 asFUNCTION(ModelRenderParamFactory),
														 asCALL_CDECL_OBJLAST);
						manager->CheckError(r);
						r = eng->RegisterObjectProperty("ModelRenderParam",
														"Matrix4 matrix",
														asOFFSET(ModelRenderParam, matrix));
						manager->CheckError(r);
						r = eng->RegisterObjectProperty("ModelRenderParam",
														"Vector3 customColor",
														asOFFSET(ModelRenderParam, customColor));
						manager->CheckError(r);
						r = eng->RegisterObjectProperty("ModelRenderParam",
														"bool depthHack",
														asOFFSET(ModelRenderParam, depthHack));
						manager->CheckError(r);
						
						r = eng->RegisterObjectBehaviour("DynamicLightParam",
														 asBEHAVE_CONSTRUCT,
														 "void f()",
														 asFUNCTION(DynamicLightParamFactory),
														 asCALL_CDECL_OBJLAST);
						manager->CheckError(r);
						r = eng->RegisterObjectBehaviour("DynamicLightParam",
														 asBEHAVE_CONSTRUCT,
														 "void f(const DynamicLightParam& in)",
														 asFUNCTION(DynamicLightParamFactory2),
														 asCALL_CDECL_OBJLAST);
						manager->CheckError(r);
						r = eng->RegisterObjectBehaviour("DynamicLightParam",
														 asBEHAVE_DESTRUCT,
														 "void f()",
														 asFUNCTION(DynamicLightParamDestructor),
														 asCALL_CDECL_OBJLAST);
						manager->CheckError(r);
						r = eng->RegisterObjectMethod("DynamicLightParam",
													  "void opAssign(const DynamicLightParam& in)",
													  asFUNCTION(DynamicLightParamAssign),
													  asCALL_CDECL_OBJLAST);
						manager->CheckError(r);
						r = eng->RegisterObjectProperty("DynamicLightParam",
														"DynamicLightType type",
														asOFFSET(DynamicLightParam, type));
						manager->CheckError(r);
						r = eng->RegisterObjectProperty("DynamicLightParam",
														"Vector3 origin",
														asOFFSET(DynamicLightParam, origin));
						manager->CheckError(r);
						r = eng->RegisterObjectProperty("DynamicLightParam",
														"float radius",
														asOFFSET(DynamicLightParam, radius));
						manager->CheckError(r);
						r = eng->RegisterObjectProperty("DynamicLightParam",
														"Vector3 color",
														asOFFSET(DynamicLightParam, color));
						manager->CheckError(r);
						r = eng->RegisterObjectProperty("DynamicLightParam",
														"Vector3 spotAxisX",
														asOFFSET(DynamicLightParam, spotAxis[0]));
						manager->CheckError(r);
						r = eng->RegisterObjectProperty("DynamicLightParam",
														"Vector3 spotAxisY",
														asOFFSET(DynamicLightParam, spotAxis[1]));
						manager->CheckError(r);
						r = eng->RegisterObjectProperty("DynamicLightParam",
														"Vector3 spotAxisZ",
														asOFFSET(DynamicLightParam, spotAxis[2]));
						manager->CheckError(r);
						r = eng->RegisterObjectProperty("DynamicLightParam",
														"Image@ image",
														asOFFSET(DynamicLightParam, image));
						manager->CheckError(r);
						r = eng->RegisterObjectProperty("DynamicLightParam",
														"float spotAngle",
														asOFFSET(DynamicLightParam, spotAngle));
						manager->CheckError(r);
						r = eng->RegisterObjectProperty("DynamicLightParam",
														"bool useLensFlare",
														asOFFSET(DynamicLightParam, useLensFlare));
						manager->CheckError(r);
						
						r = eng->RegisterObjectBehaviour("SceneDefinition",
														 asBEHAVE_CONSTRUCT,
														 "void f()",
														 asFUNCTION(SceneDefinitionFactory),
														 asCALL_CDECL_OBJLAST);
						manager->CheckError(r);
						r = eng->RegisterObjectProperty("SceneDefinition",
														"int viewportLeft",
														asOFFSET(SceneDefinition, viewportLeft));
						manager->CheckError(r);
						r = eng->RegisterObjectProperty("SceneDefinition",
														"int viewportTop",
														asOFFSET(SceneDefinition, viewportTop));
						manager->CheckError(r);
						r = eng->RegisterObjectProperty("SceneDefinition",
														"int viewportWidth",
														asOFFSET(SceneDefinition, viewportWidth));
						manager->CheckError(r);
						r = eng->RegisterObjectProperty("SceneDefinition",
														"int viewportHeight",
														asOFFSET(SceneDefinition, viewportHeight));
						manager->CheckError(r);
						r = eng->RegisterObjectProperty("SceneDefinition",
														"float fovX",
														asOFFSET(SceneDefinition, fovX));
						manager->CheckError(r);
						r = eng->RegisterObjectProperty("SceneDefinition",
														"float fovY",
														asOFFSET(SceneDefinition, fovY));
						manager->CheckError(r);
						r = eng->RegisterObjectProperty("SceneDefinition",
														"Vector3 viewOrigin",
														asOFFSET(SceneDefinition, viewOrigin));
						manager->CheckError(r);
						r = eng->RegisterObjectProperty("SceneDefinition",
														"Vector3 viewAxisX",
														asOFFSET(SceneDefinition, viewAxis[0]));
						manager->CheckError(r);
						r = eng->RegisterObjectProperty("SceneDefinition",
														"Vector3 viewAxisY",
														asOFFSET(SceneDefinition, viewAxis[1]));
						manager->CheckError(r);
						r = eng->RegisterObjectProperty("SceneDefinition",
														"Vector3 viewAxisZ",
														asOFFSET(SceneDefinition, viewAxis[2]));
						manager->CheckError(r);
						r = eng->RegisterObjectProperty("SceneDefinition",
														"float zNear",
														asOFFSET(SceneDefinition, zNear));
						manager->CheckError(r);
						r = eng->RegisterObjectProperty("SceneDefinition",
														"float zFar",
														asOFFSET(SceneDefinition, zFar));
						manager->CheckError(r);
						r = eng->RegisterObjectProperty("SceneDefinition",
														"bool skipWorld",
														asOFFSET(SceneDefinition, skipWorld));
						manager->CheckError(r);
						r = eng->RegisterObjectProperty("SceneDefinition",
														"float depthOfFieldFocalLength",
														asOFFSET(SceneDefinition, depthOfFieldFocalLength));
						manager->CheckError(r);
						r = eng->RegisterObjectProperty("SceneDefinition",
														"uint time",
														asOFFSET(SceneDefinition, time));
						manager->CheckError(r);
						r = eng->RegisterObjectProperty("SceneDefinition",
														"bool denyCameraBlur",
														asOFFSET(SceneDefinition, denyCameraBlur));
						manager->CheckError(r);
						r = eng->RegisterObjectProperty("SceneDefinition",
														"float blurVignette",
														asOFFSET(SceneDefinition, blurVignette));
						manager->CheckError(r);
						r = eng->RegisterObjectProperty("SceneDefinition",
														"float radialBlur",
														asOFFSET(SceneDefinition, radialBlur));
						manager->CheckError(r);
						r = eng->RegisterObjectProperty("SceneDefinition",
														"float globalBlur",
														asOFFSET(SceneDefinition, globalBlur));
						manager->CheckError(r);
						
						r = eng->RegisterObjectMethod("Renderer",
													  "void Init()",
													  asMETHOD(IRenderer, Init),
													  asCALL_THISCALL);
						manager->CheckError(r);
						
						r = eng->RegisterObjectMethod("Renderer",
													  "void Shutdown()",
													  asMETHOD(IRenderer, Shutdown),
													  asCALL_THISCALL);
						manager->CheckError(r);
						
						r = eng->RegisterObjectMethod("Renderer",
													  "Image@ RegisterImage(const string& in)",
													  asFUNCTION(RegisterImage),
													  asCALL_CDECL_OBJLAST);
						manager->CheckError(r);
						r = eng->RegisterObjectMethod("Renderer",
													  "Model@ RegisterModel(const string& in)",
													  asFUNCTION(RegisterModel),
													  asCALL_CDECL_OBJLAST);
						manager->CheckError(r);
						// OpenSpades' C++ functions increase the reference count of a passed object
						// when storing it (just like the convention of Objective C), so we must
						// use "auto handles" (`@+`).  Otherwise, a memory leak would occur
						// (https://github.com/yvt/openspades/issues/687).
						r = eng->RegisterObjectMethod("Renderer",
													  "Image@ CreateImage(Bitmap@+)",
													  asMETHOD(IRenderer, CreateImage),
													  asCALL_THISCALL);
						manager->CheckError(r);
						r = eng->RegisterObjectMethod("Renderer",
													  "Model@ CreateModel(VoxelModel@+)",
													  asMETHOD(IRenderer, CreateModel),
													  asCALL_THISCALL);
						manager->CheckError(r);
						r = eng->RegisterObjectMethod("Renderer",
													  "void set_GameMap(GameMap@+)",
													  asMETHOD(IRenderer, SetGameMap),
													  asCALL_THISCALL);
						manager->CheckError(r);
						r = eng->RegisterObjectMethod("Renderer",
													  "void set_FogDistance(float)",
													  asMETHOD(IRenderer, SetFogDistance),
													  asCALL_THISCALL);
						manager->CheckError(r);
						r = eng->RegisterObjectMethod("Renderer",
													  "void set_FogColor(const Vector3& in)",
													  asFUNCTION(SetFogColor),
													  asCALL_CDECL_OBJLAST);
						manager->CheckError(r);
						r = eng->RegisterObjectMethod("Renderer",
													  "void StartScene(const SceneDefinition& in)",
													  asMETHOD(IRenderer, StartScene),
													  asCALL_THISCALL);
						manager->CheckError(r);
						r = eng->RegisterObjectMethod("Renderer",
													  "void AddLight(const DynamicLightParam& in)",
													  asMETHOD(IRenderer, AddLight),
													  asCALL_THISCALL);
						manager->CheckError(r);
						r = eng->RegisterObjectMethod("Renderer",
													  "void AddModel(Model@+, const ModelRenderParam& in)",
													  asMETHOD(IRenderer, RenderModel),
													  asCALL_THISCALL);
						manager->CheckError(r);
						r = eng->RegisterObjectMethod("Renderer",
													  "void AddDebugLine(const Vector3&in, const Vector3&in, const Vector4&in)",
													  asFUNCTION(AddDebugLine),
													  asCALL_CDECL_OBJLAST);
						manager->CheckError(r);
						r = eng->RegisterObjectMethod("Renderer",
													  "void AddSprite(Image@+, const Vector3&in, float, float)",
													  asFUNCTION(AddSprite),
													  asCALL_CDECL_OBJLAST);
						manager->CheckError(r);
						r = eng->RegisterObjectMethod("Renderer",
													  "void AddLongSprite(Image@+, const Vector3& in, const Vector3& in, float)",
													  asFUNCTION(AddLongSprite),
													  asCALL_CDECL_OBJLAST);
						manager->CheckError(r);
						r = eng->RegisterObjectMethod("Renderer",
													  "void EndScene()",
													  asMETHOD(IRenderer, EndScene),
													  asCALL_THISCALL);
						manager->CheckError(r);
						r = eng->RegisterObjectMethod("Renderer",
													  "void MultiplyScreenColor(const Vector3& in)",
													  asFUNCTION(MultiplyScreenColor),
													  asCALL_CDECL_OBJLAST);
						manager->CheckError(r);
						r = eng->RegisterObjectMethod("Renderer",
													  "void set_Color(const Vector4&in)",
													  asFUNCTION(SetColor),
													  asCALL_CDECL_OBJLAST);
						manager->CheckError(r);
						r = eng->RegisterObjectMethod("Renderer",
													  "void set_ColorOpaque(const Vector3&in)",
													  asFUNCTION(SetColorOpaque),
													  asCALL_CDECL_OBJLAST);
						manager->CheckError(r);
						r = eng->RegisterObjectMethod("Renderer",
													  "void set_ColorP(const Vector4&in)",
													  asFUNCTION(SetColorAlphaPremultiplied),
													  asCALL_CDECL_OBJLAST);
						manager->CheckError(r);
						r = eng->RegisterObjectMethod("Renderer",
													  "void set_ColorNP(const Vector4&in)",
													  asFUNCTION(SetColorAlphaNonPremultiplied),
													  asCALL_CDECL_OBJLAST);
						manager->CheckError(r);
						r = eng->RegisterObjectMethod("Renderer",
													  "void DrawImage(Image@+, const Vector2& in)",
													  asMETHODPR(IRenderer, DrawImage,
																 (IImage*, const Vector2&),
																 void),
													  asCALL_THISCALL);
						manager->CheckError(r);
						r = eng->RegisterObjectMethod("Renderer",
													  "void DrawImage(Image@+, const AABB2& in)",
													  asMETHODPR(IRenderer, DrawImage,
																 (IImage*, const AABB2&),
																 void),
													  asCALL_THISCALL);
						manager->CheckError(r);
						r = eng->RegisterObjectMethod("Renderer",
													  "void DrawImage(Image@+, const Vector2&in, const AABB2& in)",
													  asMETHODPR(IRenderer, DrawImage,
																 (IImage*, const Vector2&, const AABB2&),
																 void),
													  asCALL_THISCALL);
						manager->CheckError(r);
						r = eng->RegisterObjectMethod("Renderer",
													  "void DrawImage(Image@+, const AABB2&in, const AABB2& in)",
													  asMETHODPR(IRenderer, DrawImage,
																 (IImage*, const AABB2&, const AABB2&),
																 void),
													  asCALL_THISCALL);
						manager->CheckError(r);
						r = eng->RegisterObjectMethod("Renderer",
													  "void DrawImage(Image@+, const Vector2&in, const Vector2&in, const Vector2&in, const AABB2& in)",
													  asMETHODPR(IRenderer, DrawImage,
																 (IImage*, const Vector2&, const Vector2&, const Vector2&, const AABB2&),
																 void),
													  asCALL_THISCALL);
						manager->CheckError(r);
						r = eng->RegisterObjectMethod("Renderer",
													  "void DrawFlatGameMap(const AABB2&in, const AABB2& in)",
													  asMETHOD(IRenderer,DrawFlatGameMap),
													  asCALL_THISCALL);
						manager->CheckError(r);
						r = eng->RegisterObjectMethod("Renderer",
													  "void FrameDone()",
													  asMETHOD(IRenderer,FrameDone),
													  asCALL_THISCALL);
						manager->CheckError(r);
						r = eng->RegisterObjectMethod("Renderer",
													  "void Flip()",
													  asMETHOD(IRenderer,Flip),
													  asCALL_THISCALL);
						manager->CheckError(r);
						r = eng->RegisterObjectMethod("Renderer",
													  "Bitmap@ ReadBitmap()",
													  asMETHOD(IRenderer,ReadBitmap),
													  asCALL_THISCALL);
						manager->CheckError(r);
						r = eng->RegisterObjectMethod("Renderer",
													  "float get_ScreenWidth()",
													  asMETHOD(IRenderer,ScreenWidth),
													  asCALL_THISCALL);
						manager->CheckError(r);
						r = eng->RegisterObjectMethod("Renderer",
													  "float get_ScreenHeight()",
													  asMETHOD(IRenderer,ScreenHeight),
													  asCALL_THISCALL);
						manager->CheckError(r);
						break;
					default:
						break;
				}
			}
		};
		
		static RendererRegistrar registrar;
	}
}
