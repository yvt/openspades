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
#include <memory>
#include <Client/GameMap.h>
#include "ScriptManager.h"
#include <Core/IStream.h>
#include <Core/FileManager.h>

namespace spades {
	namespace client {
		
		class GameMapRegistrar: public ScriptObjectRegistrar {
			static GameMap *Factory(int w, int h, int d){
				if(w != GameMap::DefaultWidth ||
				   h != GameMap::DefaultHeight ||
				   d != GameMap::DefaultDepth) {
					asGetActiveContext()->SetException("Currently, non-default GameMap dimensions aren't supported.");
					return nullptr;
				}
				try{
					return new GameMap();
				}catch(const std::exception& ex){
					ScriptContextUtils().SetNativeException(ex);
					return nullptr;
				}
			}
			static GameMap *LoadFactory(const std::string& fn){
				try{
					std::unique_ptr<spades::IStream> stream{FileManager::OpenForReading(fn.c_str())};
					GameMap *ret = GameMap::Load(stream.get());
					return ret;
				}catch(const std::exception& ex) {
					ScriptContextUtils().SetNativeException(ex);
					return nullptr;
				}
			}
			
			static uint32_t GetColor(int x, int y, int z,
									 GameMap *m) {
				if(x < 0 || y < 0 || x >= m->Width() || y >= m->Height() ||
				   z < 0 || z >= m->Depth()) {
					asGetActiveContext()->SetException("Attempted to access a voxel outside the valid range.");
					return 0;
				}
				return m->GetColor(x, y, z);
			}
			
			static bool IsSolid(int x, int y, int z,
								GameMap *m) {
				if(x < 0 || y < 0 || x >= m->Width() || y >= m->Height() ||
				   z < 0 || z >= m->Depth()) {
					asGetActiveContext()->SetException("Attempted to access a voxel outside the valid range.");
					return 0;
				}
				return m->IsSolid(x, y, z);
			}
			static uint32_t GetColorWrapped(int x, int y, int z,
											GameMap *m) {
				return m->GetColorWrapped(x, y, z);
			}
			
			static bool IsSolidWrapped(int x, int y, int z,
									   GameMap *m) {
				return m->IsSolidWrapped(x, y, z);
			}
			static void SetAir(int x, int y, int z,
							   GameMap *m) {
				if(x < 0 || y < 0 || x >= m->Width() || y >= m->Height() ||
				   z < 0 || z >= m->Depth()) {
					asGetActiveContext()->SetException("Attempted to access a voxel outside the valid range.");
					return;
				}
				m->Set(x, y, z, false, 0);
			}
			static void SetSolid(int x, int y, int z, uint32_t col,
								 GameMap *m) {
				if(x < 0 || y < 0 || x >= m->Width() || y >= m->Height() ||
				   z < 0 || z >= m->Depth()) {
					asGetActiveContext()->SetException("Attempted to access a voxel outside the valid range.");
					return;
				}
				m->Set(x, y, z, true, col);
			}
			static GameMap::RayCastResult CastRay(const Vector3& v0,
												  const Vector3& dir,
												  int maxSteps,
												  GameMap *m) {
				return m->CastRay2(v0, dir, maxSteps);
			}
			
		public:
			GameMapRegistrar():
			ScriptObjectRegistrar("GameMap") {}
			
			virtual void Register(ScriptManager *manager, Phase phase) {
				asIScriptEngine *eng = manager->GetEngine();
				int r;
				eng->SetDefaultNamespace("spades");
				switch(phase){
					case PhaseObjectType:
						r = eng->RegisterObjectType("GameMap",
													0, asOBJ_REF);
						manager->CheckError(r);
						r = eng->RegisterObjectType("GameMapRayCastResult",
													sizeof(GameMap::RayCastResult), asOBJ_VALUE|asOBJ_POD|asOBJ_APP_CLASS_CDAK);
						manager->CheckError(r);
						break;
					case PhaseObjectMember:
						r = eng->RegisterObjectBehaviour("GameMap",
														 asBEHAVE_ADDREF,
														 "void f()",
														 asMETHOD(GameMap, AddRef),
														 asCALL_THISCALL);
						manager->CheckError(r);
						r = eng->RegisterObjectBehaviour("GameMap",
														 asBEHAVE_RELEASE,
														 "void f()",
														 asMETHOD(GameMap, Release),
														 asCALL_THISCALL);
						manager->CheckError(r);
						r = eng->RegisterObjectBehaviour("GameMap",
														 asBEHAVE_FACTORY,
														 "GameMap @f(int, int, int)",
														 asFUNCTION(Factory),
														 asCALL_CDECL);
						manager->CheckError(r);
						r = eng->RegisterObjectBehaviour("GameMap",
														 asBEHAVE_FACTORY,
														 "GameMap @f(const string& in)",
														 asFUNCTION(LoadFactory),
														 asCALL_CDECL);
						manager->CheckError(r);
						r = eng->RegisterObjectMethod("GameMap",
													  "uint GetColor(int, int, int)",
													  asFUNCTION(GetColor),
													  asCALL_CDECL_OBJLAST);
						manager->CheckError(r);
						r = eng->RegisterObjectMethod("GameMap",
													  "bool IsSolid(int, int, int)",
													  asFUNCTION(IsSolid),
													  asCALL_CDECL_OBJLAST);
						manager->CheckError(r);
						r = eng->RegisterObjectMethod("GameMap",
													  "uint GetColorWrapped(int, int, int)",
													  asFUNCTION(GetColorWrapped),
													  asCALL_CDECL_OBJLAST);
						manager->CheckError(r);
						r = eng->RegisterObjectMethod("GameMap",
													  "bool IsSolidWrapped(int, int, int)",
													  asFUNCTION(IsSolidWrapped),
													  asCALL_CDECL_OBJLAST);
						manager->CheckError(r);
						r = eng->RegisterObjectMethod("GameMap",
													  "void SetAir(int, int, int)",
													  asFUNCTION(SetAir),
													  asCALL_CDECL_OBJLAST);
						manager->CheckError(r);
						r = eng->RegisterObjectMethod("GameMap",
													  "void SetSolid(int, int, int, uint)",
													  asFUNCTION(SetSolid),
													  asCALL_CDECL_OBJLAST);
						manager->CheckError(r);
						r = eng->RegisterObjectMethod("GameMap",
													  "int get_Width()",
													  asMETHOD(GameMap, Width),
													  asCALL_THISCALL);
						manager->CheckError(r);
						r = eng->RegisterObjectMethod("GameMap",
													  "int get_Height()",
													  asMETHOD(GameMap, Height),
													  asCALL_THISCALL);
						manager->CheckError(r);
						r = eng->RegisterObjectMethod("GameMap",
													  "int get_Depth()",
													  asMETHOD(GameMap, Depth),
													  asCALL_THISCALL);
						manager->CheckError(r);
						r = eng->RegisterObjectMethod("GameMap",
													  "bool ClipBox(int, int, int)",
													  asMETHODPR(GameMap, ClipBox, (int,int,int), bool),
													  asCALL_THISCALL);
						manager->CheckError(r);
						r = eng->RegisterObjectMethod("GameMap",
													  "bool ClipWorld(int, int, int)",
													  asMETHODPR(GameMap, ClipBox, (int,int,int), bool),
													  asCALL_THISCALL);
						manager->CheckError(r);
						r = eng->RegisterObjectMethod("GameMap",
													  "bool ClipBox(float, float, float)",
													  asMETHODPR(GameMap, ClipBox, (float,float,float), bool),
													  asCALL_THISCALL);
						manager->CheckError(r);
						r = eng->RegisterObjectMethod("GameMap",
													  "bool ClipWorld(float, float, float)",
													  asMETHODPR(GameMap, ClipBox, (float,float,float), bool),
													  asCALL_THISCALL);
						manager->CheckError(r);
						r = eng->RegisterObjectMethod("GameMap",
													  "GameMapRayCastResult CastRay(const Vector3& in, const Vector3& in, int)",
													  asFUNCTION(CastRay),
													  asCALL_CDECL_OBJLAST);
						manager->CheckError(r);
						
						r = eng->RegisterObjectProperty("GameMapRayCastResult",
														"bool hit",
														asOFFSET(GameMap::RayCastResult, hit));
						manager->CheckError(r);
						r = eng->RegisterObjectProperty("GameMapRayCastResult",
														"bool startSolid",
														asOFFSET(GameMap::RayCastResult, startSolid));
						manager->CheckError(r);
						r = eng->RegisterObjectProperty("GameMapRayCastResult",
														"Vector3 hitPos",
														asOFFSET(GameMap::RayCastResult, hitPos));
						manager->CheckError(r);
						r = eng->RegisterObjectProperty("GameMapRayCastResult",
														"IntVector3 hitBlock",
														asOFFSET(GameMap::RayCastResult, hitBlock));
						manager->CheckError(r);
						r = eng->RegisterObjectProperty("GameMapRayCastResult",
														"IntVector3 normal",
														asOFFSET(GameMap::RayCastResult, normal));
						manager->CheckError(r);
						
						break;
					default: break;
				}
			}
		};
		
		static GameMapRegistrar registrar;
	}
}

