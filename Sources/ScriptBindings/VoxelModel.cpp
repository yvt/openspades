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
#include <Core/VoxelModel.h>
#include "ScriptManager.h"
#include <Core/FileManager.h>

namespace spades {
	
	
	class VoxelModelRegistrar: public ScriptObjectRegistrar {
		static VoxelModel *Factory(int w, int h, int d){
			try{
				return new VoxelModel(w, h, d);
			}catch(const std::exception& ex){
				ScriptContextUtils().SetNativeException(ex);
				return nullptr;
			}
		}
		static VoxelModel *LoadFactory(const std::string& fn){
			try{
				std::unique_ptr<spades::IStream> stream{FileManager::OpenForReading(fn.c_str())};
				VoxelModel *ret = VoxelModel::LoadKV6(stream.get());
				return ret;
			}catch(const std::exception& ex) {
				ScriptContextUtils().SetNativeException(ex);
				return nullptr;
			}
		}
		static uint64_t GetSolidBits(int x, int y,
									 VoxelModel *m) {
			if(x < 0 || y < 0 || x >= m->GetWidth() || y >= m->GetHeight()) {
				asGetActiveContext()->SetException("Attempted to access a voxel outside the valid range.");
				return 0;
			}
			return m->GetSolidBitsAt(x, y);
		}
		
		static uint32_t GetColor(int x, int y, int z,
								 VoxelModel *m) {
			if(x < 0 || y < 0 || x >= m->GetWidth() || y >= m->GetHeight() ||
			   z < 0 || z >= m->GetDepth()) {
				asGetActiveContext()->SetException("Attempted to access a voxel outside the valid range.");
				return 0;
			}
			return m->GetColor(x, y, z);
		}
		
		static bool IsSolid(int x, int y, int z,
							VoxelModel *m) {
			if(x < 0 || y < 0 || x >= m->GetWidth() || y >= m->GetHeight() ||
			   z < 0 || z >= m->GetDepth()) {
				asGetActiveContext()->SetException("Attempted to access a voxel outside the valid range.");
				return 0;
			}
			return m->IsSolid(x, y, z);
		}
		static void SetAir(int x, int y, int z,
						   VoxelModel *m) {
			if(x < 0 || y < 0 || x >= m->GetWidth() || y >= m->GetHeight() ||
			   z < 0 || z >= m->GetDepth()) {
				asGetActiveContext()->SetException("Attempted to access a voxel outside the valid range.");
				return;
			}
			m->SetAir(x, y, z);
		}
		static void SetSolid(int x, int y, int z, uint32_t col,
							 VoxelModel *m) {
			if(x < 0 || y < 0 || x >= m->GetWidth() || y >= m->GetHeight() ||
			   z < 0 || z >= m->GetDepth()) {
				asGetActiveContext()->SetException("Attempted to access a voxel outside the valid range.");
				return;
			}
			m->SetSolid(x, y, z, col);
		}
		
		
	public:
		VoxelModelRegistrar():
		ScriptObjectRegistrar("VoxelModel") {}
		
		virtual void Register(ScriptManager *manager, Phase phase) {
			asIScriptEngine *eng = manager->GetEngine();
			int r;
			eng->SetDefaultNamespace("spades");
			switch(phase){
				case PhaseObjectType:
					r = eng->RegisterObjectType("VoxelModel",
												0, asOBJ_REF);
					manager->CheckError(r);
					break;
				case PhaseObjectMember:
					r = eng->RegisterObjectBehaviour("VoxelModel",
													 asBEHAVE_ADDREF,
													 "void f()",
													 asMETHOD(VoxelModel, AddRef),
													 asCALL_THISCALL);
					manager->CheckError(r);
					r = eng->RegisterObjectBehaviour("VoxelModel",
													 asBEHAVE_RELEASE,
													 "void f()",
													 asMETHOD(VoxelModel, Release),
													 asCALL_THISCALL);
					manager->CheckError(r);
					r = eng->RegisterObjectBehaviour("VoxelModel",
													 asBEHAVE_FACTORY,
													 "VoxelModel @f(int, int, int)",
													 asFUNCTION(Factory),
													 asCALL_CDECL);
					manager->CheckError(r);
					r = eng->RegisterObjectBehaviour("VoxelModel",
													 asBEHAVE_FACTORY,
													 "VoxelModel @f(const string& in)",
													 asFUNCTION(LoadFactory),
													 asCALL_CDECL);
					manager->CheckError(r);
					r = eng->RegisterObjectMethod("VoxelModel",
												  "uint GetSolidBits(int, int)",
												  asFUNCTION(GetSolidBits),
												  asCALL_CDECL_OBJLAST);
					manager->CheckError(r);
					r = eng->RegisterObjectMethod("VoxelModel",
												  "uint GetColor(int, int, int)",
												  asFUNCTION(GetColor),
												  asCALL_CDECL_OBJLAST);
					manager->CheckError(r);
					r = eng->RegisterObjectMethod("VoxelModel",
												  "bool IsSolid(int, int, int)",
												  asFUNCTION(IsSolid),
												  asCALL_CDECL_OBJLAST);
					manager->CheckError(r);
					r = eng->RegisterObjectMethod("VoxelModel",
												  "void SetAir(int, int, int)",
												  asFUNCTION(SetAir),
												  asCALL_CDECL_OBJLAST);
					manager->CheckError(r);
					r = eng->RegisterObjectMethod("VoxelModel",
												  "void SetSolid(int, int, int, uint)",
												  asFUNCTION(SetSolid),
												  asCALL_CDECL_OBJLAST);
					manager->CheckError(r);
					r = eng->RegisterObjectMethod("VoxelModel",
												  "int get_Width()",
												  asMETHOD(VoxelModel, GetWidth),
												  asCALL_THISCALL);
					manager->CheckError(r);
					r = eng->RegisterObjectMethod("VoxelModel",
												  "int get_Height()",
												  asMETHOD(VoxelModel, GetHeight),
												  asCALL_THISCALL);
					manager->CheckError(r);
					r = eng->RegisterObjectMethod("VoxelModel",
												  "int get_Depth()",
												  asMETHOD(VoxelModel, GetDepth),
												  asCALL_THISCALL);
					manager->CheckError(r);
					break;
				default:
					break;
			}
		}
	};
	
	static VoxelModelRegistrar registrar;
	
}
