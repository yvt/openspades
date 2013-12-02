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
#include <FL/Fl_Widget.H>
#include <FL/Fl.H>

namespace spades{
		
	class ClipboardReceiver: public Fl_Widget {
	public:
		std::string clipboardData;
		bool hasClipboardData;
		
		ClipboardReceiver():
		Fl_Widget(0,0,2,2,NULL),
		hasClipboardData(false){
			
		}
		
		virtual int handle(int event) {
			if(event == FL_PASTE) {
				hasClipboardData = true;
				clipboardData = Fl::event_text();
				return 1;
			}
			return Fl_Widget::handle(event);
		}
		virtual void draw(){}
	};
	
	static ClipboardReceiver *receiver = NULL;
	class ClipboardRegistrar: public ScriptObjectRegistrar {
		static void EnsureReceiver() {
			if(!receiver) {
				receiver = new ClipboardReceiver();
			}
		}
		
		static bool GotClipboardData() {
			EnsureReceiver();
			return receiver->hasClipboardData;
		}
		static std::string GetClipboardData() {
			EnsureReceiver();
			std::string s = receiver->clipboardData;
			receiver->hasClipboardData = false;
			return s;
		}
		
		static void RequestClipboardData() {
			EnsureReceiver();
			Fl::paste(*receiver, 1);
		}
		
		static void SetClipboardData(const std::string& s) {
			Fl::copy(s.data(), s.size(), 1);
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
