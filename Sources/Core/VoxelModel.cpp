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

#include "VoxelModel.h"
#include "Exception.h"
#include <algorithm>
#include <vector>
#include "Debug.h"
#include "ScriptManager.h"
#include "FileManager.h"

namespace spades {
	VoxelModel::VoxelModel(int w, int h, int d) {
		SPADES_MARK_FUNCTION();
		
		if(w < 1 || h < 1 || d < 1 || w > 4096 || h > 4096)
			SPRaise("Invalid dimension: %dx%dx%d", w, h, d);
		
		width = w;
		height = h;
		depth = d;
		if(d > 64){
			SPRaise("Voxel model with depth > 64 is not supported.");
		}
		
		solidBits = new uint64_t[w * h];
		colors = new uint32_t[w * h * d];
		
		std::fill(solidBits, solidBits + w * h, 0);
		
		refCount = 1;
		
	}
	VoxelModel::~VoxelModel() {
		SPADES_MARK_FUNCTION();
		
		delete[] solidBits;
		delete[] colors;
	}
	
	struct KV6Block {
		uint32_t color;
		uint16_t zPos;
		uint8_t visFaces;
		uint8_t lighting;
	};
	
	struct KV6Header {
		uint32_t xsiz, ysiz, zsiz;
		float xpivot, ypivot, zpivot;
		uint32_t blklen;
	};
	static uint32_t swapColor(uint32_t col ){
		union {
			uint8_t bytes[4];
			uint32_t c;
		} u;
		u.c = col;
		std::swap(u.bytes[0], u.bytes[2]);
		return (u.c & 0xffffff);
	}
	
	
	VoxelModel *VoxelModel::LoadKV6(spades::IStream *stream) {
		SPADES_MARK_FUNCTION();
		
		if(stream->Read(4) != "Kvxl"){
			SPRaise("Invalid magic");
		}
		
		KV6Header header;
		if(stream->Read(&header, sizeof(header)) < sizeof(header)){
			SPRaise("File truncated: failed to read header");
		}
		
		std::vector<KV6Block> blkdata;
		blkdata.resize(header.blklen);
		
		if(stream->Read(blkdata.data(), sizeof(KV6Block) * header.blklen) <
		   sizeof(KV6Block) * header.blklen){
			SPRaise("File truncated: failed to read blocks");
		}
		
		std::vector<uint32_t> xoffset;
		xoffset.resize(header.xsiz);
		
		if(stream->Read(xoffset.data(), sizeof(uint32_t) * header.xsiz) <
		   sizeof(uint32_t) * header.xsiz){
			SPRaise("File truncated: failed to read xoffset");
		}
		
		std::vector<uint16_t> xyoffset;
		xyoffset.resize(header.xsiz * header.ysiz);
		
		if(stream->Read(xyoffset.data(), sizeof(uint16_t) * header.xsiz * header.ysiz) <
		   sizeof(uint16_t) * header.xsiz * header.ysiz){
			SPRaise("File truncated: failed to read xyoffset");
		}
		
		// validate: zpos < depth
		for(size_t i = 0; i < blkdata.size(); i++)
			if(blkdata[i].zPos >= header.zsiz)
				SPRaise("File corrupted: blkData[i].zPos >= header.zsiz");
		
		// validate sum(xyoffset) = blkLen
		{
			uint64_t ttl = 0;
			for(size_t i = 0; i < xyoffset.size(); i++){
				ttl += (uint32_t)xyoffset[i];
			}
			if(ttl != (uint64_t)blkdata.size())
				SPRaise("File corrupted: sum(xyoffset) != blkdata.size()");
		}
		
		VoxelModel *model = new VoxelModel(header.xsiz, header.ysiz, header.zsiz);
		model->SetOrigin(MakeVector3(-header.xpivot,
									 -header.ypivot,
									 -header.zpivot));
		try{
			int pos = 0;
			for(int x = 0; x < (int)header.xsiz; x++)
				for(int y = 0; y < (int)header.ysiz; y++){
					int spanBlocks = (int)xyoffset[x *header.ysiz + y];
					int lastZ = -1;
					//printf("pos: %d, (%d, %d): %d\n",
					//	   pos, x, y, spanBlocks);
					while(spanBlocks--){
						const KV6Block& b = blkdata[pos];
						//printf("%d, %d, %d: %d, %d\n", x, y, b.zPos,
						//	   b.visFaces, b.lighting);
						if(model->IsSolid(x, y, b.zPos)){
							SPRaise("Duplicate voxel (%d, %d, %d)",
									x, y, b.zPos);
						}
						if(b.zPos <= lastZ){
							SPRaise("Not Z-sorted");
						}
						lastZ = b.zPos;
						model->SetSolid(x, y, b.zPos,
										swapColor(b.color));
						pos++;
					}
				}
			
			SPAssert(pos == blkdata.size());
			
			return model;
		}catch(...){
			delete model;
			throw;
		}
	}
	
	void VoxelModel::AddRef() {
		asAtomicInc(refCount);
	}
	
	void VoxelModel::Release(){
		if(asAtomicDec(refCount) <= 0){
			delete this;
		}
	}
	
	class VoxelModelRegistrar: public ScriptObjectRegistrar {
		static VoxelModel *Factory(int w, int h, int d){
			try{
				return new VoxelModel(w, h, d);
			}catch(const std::exception& ex){
				ScriptContextUtils().SetNativeException(ex);
				return NULL;
			}
		}
		static VoxelModel *LoadFactory(const std::string& fn){
			spades::IStream *stream = NULL;
			try{
				stream = FileManager::OpenForReading(fn.c_str());
				VoxelModel *ret = VoxelModel::LoadKV6(stream);
				delete stream;
				return ret;
			}catch(const std::exception& ex) {
				ScriptContextUtils().SetNativeException(ex);
				if(stream) delete stream;
				return NULL;
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
			}
		}
	};
	
	static VoxelModelRegistrar registrar;
	
}
