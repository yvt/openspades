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

#include <Core/Strings.h>
#include <stdlib.h>
#include "ScriptManager.h"
#include <new>

namespace spades {
	
	class StringsObjectRegistrar: public ScriptObjectRegistrar {
		static std::string DefaultTranslate(const std::string& c, const std::string& s) {
			return _Tr(c, s);
		}
		static std::string DefaultTranslate(const std::string& c,
											const std::string& s,
									 const std::string& p1) {
			return _Tr(c, s, p1);
		}
		static std::string DefaultTranslate(const std::string& c,
											const std::string& s,
									 const std::string& p1,
									 const std::string& p2) {
			return _Tr(c, s, p1, p2);
		}
		static std::string DefaultTranslate(const std::string& c,
											const std::string& s,
									 const std::string& p1,
									 const std::string& p2,
									 const std::string& p3) {
			return _Tr(c, s, p1, p2, p3);
		}
		static std::string DefaultTranslate(const std::string& c,
											const std::string& s,
									 const std::string& p1,
									 const std::string& p2,
									 const std::string& p3,
									 const std::string& p4) {
			return _Tr(c, s, p1, p2, p3, p4);
		}
		static std::string DefaultTranslateN(const std::string& c,
											 const std::string& sg,
									  const std::string& pl,
									  int n) {
			return _TrN(c, sg, pl, n);
		}
		static std::string DefaultTranslateN(const std::string& c,
											 const std::string& sg,
									  const std::string& pl,
									  int n,
									  const std::string& p1) {
			return _TrN(c, sg, pl, n, p1);
		}
		static std::string DefaultTranslateN(const std::string& c,
											 const std::string& sg,
									  const std::string& pl,
									  int n,
									  const std::string& p1,
									  const std::string& p2) {
			return _TrN(c, sg, pl, n, p1, p2);
		}
		static std::string DefaultTranslateN(const std::string& c,
											 const std::string& sg,
									  const std::string& pl,
									  int n,
									  const std::string& p1,
									  const std::string& p2,
									  const std::string& p3) {
			return _TrN(c, sg, pl, n, p1, p2, p3);
		}
	public:
		typedef const std::string& RCStr; // Reference Const String
		StringsObjectRegistrar():
		ScriptObjectRegistrar("Strings"){}
		virtual void Register(ScriptManager *manager, Phase phase) {
			asIScriptEngine *eng = manager->GetEngine();
			int r;
			eng->SetDefaultNamespace("spades");
			switch(phase){
				case PhaseObjectMember:
					
					r = eng->RegisterGlobalFunction("string _Tr(const string&in,const string&in)",
													asFUNCTIONPR(DefaultTranslate, (RCStr,RCStr), std::string),
													asCALL_CDECL);
					manager->CheckError(r);
					r = eng->RegisterGlobalFunction("string _Tr(const string&in,const string&in, const string&in)",
													asFUNCTIONPR(DefaultTranslate, (RCStr,RCStr,RCStr), std::string),
													asCALL_CDECL);
					manager->CheckError(r);
					r = eng->RegisterGlobalFunction("string _Tr(const string&in,const string&in, const string&in, const string&in)",
													asFUNCTIONPR(DefaultTranslate, (RCStr,RCStr,RCStr,RCStr), std::string),
													asCALL_CDECL);
					manager->CheckError(r);
					r = eng->RegisterGlobalFunction("string _Tr(const string&in,const string&in, const string&in, const string&in, const string&in)",
													asFUNCTIONPR(DefaultTranslate, (RCStr,RCStr,RCStr,RCStr,RCStr), std::string),
													asCALL_CDECL);
					manager->CheckError(r);
					r = eng->RegisterGlobalFunction("string _Tr(const string&in,const string&in, const string&in, const string&in, const string&in, const string&in)",
													asFUNCTIONPR(DefaultTranslate, (RCStr,RCStr,RCStr,RCStr,RCStr,RCStr), std::string),
													asCALL_CDECL);
					manager->CheckError(r);
					r = eng->RegisterGlobalFunction("string _TrN(const string&in,const string&in, const string&in, int)",
													asFUNCTIONPR(DefaultTranslateN, (RCStr,RCStr, RCStr, int), std::string),
													asCALL_CDECL);
					manager->CheckError(r);
					r = eng->RegisterGlobalFunction("string _TrN(const string&in,const string&in, const string&in, int, const string& in)",
													asFUNCTIONPR(DefaultTranslateN, (RCStr,RCStr, RCStr, int, RCStr), std::string),
													asCALL_CDECL);
					manager->CheckError(r);
					r = eng->RegisterGlobalFunction("string _TrN(const string&in,const string&in, const string&in, int, const string& in, const string& in)",
													asFUNCTIONPR(DefaultTranslateN, (RCStr,RCStr, RCStr, int, RCStr, RCStr), std::string),
													asCALL_CDECL);
					manager->CheckError(r);
					r = eng->RegisterGlobalFunction("string _TrN(const string&in,const string&in, const string&in, int, const string& in, const string& in, const string& in)",
													asFUNCTIONPR(DefaultTranslateN, (RCStr,RCStr, RCStr, int, RCStr, RCStr, RCStr), std::string),
													asCALL_CDECL);
					manager->CheckError(r);
					
					
					break;
				default: break;
			}
			
			
			
		}
	};
	
	static StringsObjectRegistrar registrar;
}
