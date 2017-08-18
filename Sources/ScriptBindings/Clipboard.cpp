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
#include <Client/IModel.h>
#include <Imports/SDL.h>

namespace spades{
	static bool hasClipboardData = false;
	static std::string clipboardData;
	class ClipboardRegistrar: public ScriptObjectRegistrar {
		
		
		static bool GotClipboardData() {
			if(!hasClipboardData) {
				auto *txt = SDL_GetClipboardText();
				if(txt == nullptr) {
					return false;
				}
				clipboardData = txt;
				SDL_free(txt);
				hasClipboardData = true;
			}
			return hasClipboardData;
		}
		static std::string GetClipboardData() {
			hasClipboardData = false;
			return clipboardData;
		}
		
		static void RequestClipboardData() {
		}
		
		static void SetClipboardData(const std::string& s) {
			SDL_SetClipboardText(s.c_str());
		}
		
	public:
		ClipboardRegistrar():
		ScriptObjectRegistrar("Clipboard"){
		}
		virtual void Register(ScriptManager *manager, Phase phase) {
			asIScriptEngine *eng = manager->GetEngine();
			int r;
			eng->SetDefaultNamespace("spades");
			switch(phase){
				case PhaseObjectType:
					break;
				case PhaseObjectMember:
					r = eng->RegisterGlobalFunction("bool GotClipboardData()", asFUNCTION(GotClipboardData), asCALL_CDECL);
					manager->CheckError(r);
					r = eng->RegisterGlobalFunction("string GetClipboardData()", asFUNCTION(GetClipboardData), asCALL_CDECL);
					manager->CheckError(r);
					r = eng->RegisterGlobalFunction("void SetClipboardData(const string&in)", asFUNCTION(SetClipboardData), asCALL_CDECL);
					manager->CheckError(r);
					r = eng->RegisterGlobalFunction("void RequestClipboardData()", asFUNCTION(RequestClipboardData), asCALL_CDECL);
					manager->CheckError(r);
					break;
				default:
					
					break;
			}
		}
	};
	
	static ClipboardRegistrar registrar;
	
}
