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
#include <Core/Bitmap.h>

namespace spades {
	class BitmapRegistrar: public ScriptObjectRegistrar {
		static Bitmap *Factory(int w, int h){
			return new Bitmap(w, h);
		}
		
		static Bitmap *LoadFactory(const std::string& str){
			try{
				return Bitmap::Load(str);
			}catch(const std::exception& ex) {
				ScriptContextUtils().SetNativeException(ex);
				return nullptr;
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

