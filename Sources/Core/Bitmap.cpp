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

#include "Bitmap.h"
#include "IStream.h"
#include <vector>
#include "Exception.h"
#include "../Core/Debug.h"
#include "Debug.h"
#include "IBitmapCodec.h"
#include "FileManager.h"
#include "ScriptManager.h"

namespace spades {
	Bitmap::Bitmap(int ww, int hh):
	w(ww), h(hh){
		SPADES_MARK_FUNCTION();
		
		if(w < 1 || h < 1 || w > 8192 || h > 8192) {
			SPRaise("Invalid dimension: %dx%d", w, h);
		}
		
		pixels = new uint32_t[w * h];
		SPAssert(pixels != NULL);
		
		refCount = 1;
	}
	
	Bitmap::~Bitmap() {
		SPADES_MARK_FUNCTION();
		
		delete[] pixels;
	}
	
	Bitmap *Bitmap::Load(const std::string& filename) {
		std::vector<IBitmapCodec *>codecs = IBitmapCodec::GetAllCodecs();
		std::string errMsg;
		for(size_t i = 0; i < codecs.size(); i++){
			IBitmapCodec *codec = codecs[i];
			if(codec->CanLoad() && codec->CheckExtension(filename)){
				// give it a try.
				// open error shouldn't be handled here
				StreamHandle str = FileManager::OpenForReading(filename.c_str());
				try{
					return codec->Load(str);
				}catch(const std::exception& ex){
					errMsg += codec->GetName();
					errMsg += ":\n";
					errMsg += ex.what();
					errMsg += "\n\n";
				}
			}
		}
		
		if(errMsg.empty()){
			SPRaise("Bitmap codec not found for filename: %s", filename.c_str());
		}else{
			SPRaise("No bitmap codec could load file successfully: %s\n%s\n",
					filename.c_str(), errMsg.c_str());
		}
	}
	
	void Bitmap::Save(const std::string &filename) {
		std::vector<IBitmapCodec *>codecs = IBitmapCodec::GetAllCodecs();
		for(size_t i = 0; i < codecs.size(); i++){
			IBitmapCodec *codec = codecs[i];
			if(codec->CanSave() && codec->CheckExtension(filename)){
				StreamHandle str = FileManager::OpenForWriting(filename.c_str());
				
				codec->Save(str, this);
				return;
			}
		}
		
		SPRaise("Bitmap codec not found for filename: %s", filename.c_str());
		
	}
	
	void Bitmap::AddRef() {
		asAtomicInc(refCount);
	}
	
	void Bitmap::Release(){
		if(asAtomicDec(refCount) <= 0) {
			delete this;
		}
	}
	
	uint32_t Bitmap::GetPixel(int x, int y) {
		SPAssert(x >= 0); SPAssert(y >= 0);
		SPAssert(x < w); SPAssert(y < h);
		return pixels[x + y * w];
	}
	
	void Bitmap::SetPixel(int x, int y, uint32_t p) {
		SPAssert(x >= 0); SPAssert(y >= 0);
		SPAssert(x < w); SPAssert(y < h);
		pixels[x + y * w] = p;
	}
	
	class BitmapRegistrar: public ScriptObjectRegistrar {
		static Bitmap *Factory(int w, int h){
			return new Bitmap(w, h);
		}
		
		static Bitmap *LoadFactory(const std::string& str){
			try{
				return Bitmap::Load(str);
			}catch(const std::exception& ex) {
				ScriptContextUtils().SetNativeException(ex);
				return NULL;
			}
		}
		
		static bool Save(const std::string& str,
						 Bitmap *bmp) {
			try{
				bmp->Save(str);
			}catch(const std::exception& ex) {
				// FIXME: returning error message?
				return false;
			}
			return true;
		}
		
		static uint32_t GetPixel(int x, int y,
								 Bitmap *bmp) {
			if(x < 0 || y < 0 || x >= bmp->GetWidth() || y >= bmp->GetHeight()){
				asGetActiveContext()->SetException("Attempted to fetch a pixel outside the valid range.");
				return 0;
			}
			return bmp->GetPixel(x, y);
		}
		
		static void SetPixel(int x, int y, uint32_t val,
							 Bitmap *bmp) {
			if(x < 0 || y < 0 || x >= bmp->GetWidth() || y >= bmp->GetHeight()){
				asGetActiveContext()->SetException("Attempted to write a pixel outside the valid range.");
				return;
			}
			bmp->SetPixel(x, y, val);
		}
		
	public:
		BitmapRegistrar():
		ScriptObjectRegistrar("Bitmap") {}
		
		virtual void Register(ScriptManager *manager, Phase phase) {
			asIScriptEngine *eng = manager->GetEngine();
			int r;
			eng->SetDefaultNamespace("spades");
			switch(phase){
				case PhaseObjectType:
					r = eng->RegisterObjectType("Bitmap",
												0, asOBJ_REF);
					manager->CheckError(r);
					break;
				case PhaseObjectMember:
					r = eng->RegisterObjectBehaviour("Bitmap",
													 asBEHAVE_ADDREF,
													 "void f()",
													 asMETHOD(Bitmap, AddRef),
													 asCALL_THISCALL);
					manager->CheckError(r);
					r = eng->RegisterObjectBehaviour("Bitmap",
													 asBEHAVE_RELEASE,
													 "void f()",
													 asMETHOD(Bitmap, Release),
													 asCALL_THISCALL);
					manager->CheckError(r);
					r = eng->RegisterObjectBehaviour("Bitmap",
													 asBEHAVE_FACTORY,
													 "Bitmap @f(int, int)",
													 asFUNCTION(Factory),
													 asCALL_CDECL);
					manager->CheckError(r);
					r = eng->RegisterObjectBehaviour("Bitmap",
													 asBEHAVE_FACTORY,
													 "Bitmap @f(const string& in)",
													 asFUNCTION(LoadFactory),
													 asCALL_CDECL);
					manager->CheckError(r);
					r = eng->RegisterObjectMethod("Bitmap",
												  "void Save(const string& in)",
												  asFUNCTION(Save),
												  asCALL_CDECL_OBJLAST);
					manager->CheckError(r);
					r = eng->RegisterObjectMethod("Bitmap",
												  "uint GetPixel(int, int)",
												  asFUNCTION(GetPixel),
												  asCALL_CDECL_OBJLAST);
					manager->CheckError(r);
					r = eng->RegisterObjectMethod("Bitmap",
												  "void SetPixel(int, int, uint)",
												  asFUNCTION(SetPixel),
												  asCALL_CDECL_OBJLAST);
					manager->CheckError(r);
					r = eng->RegisterObjectMethod("Bitmap",
												  "int get_Width()",
												  asMETHOD(Bitmap, GetWidth),
												  asCALL_THISCALL);
					manager->CheckError(r);
					r = eng->RegisterObjectMethod("Bitmap",
												  "int get_Height()",
												  asMETHOD(Bitmap, GetHeight),
												  asCALL_THISCALL);
					manager->CheckError(r);
					
					break;
				default:
					break;
			}
		}
	};
	
	static BitmapRegistrar registrar;
}
