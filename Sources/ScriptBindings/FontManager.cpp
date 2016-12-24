/*
 Copyright (c) 2016 yvt

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
#include <Client/Fonts.h>

namespace spades{
	namespace client {

		class FontManagerRegistrar : public ScriptObjectRegistrar {
			static IFont *GetGuiFont(FontManager *self) {
				IFont *font = self->GetGuiFont();
				font->AddRef();
				return font;
			}
			static IFont *GetHeadingFont(FontManager *self) {
				IFont *font = self->GetHeadingFont();
				font->AddRef();
				return font;
			}
			static IFont *GetLargeFont(FontManager *self) {
				IFont *font = self->GetLargeFont();
				font->AddRef();
				return font;
			}
			static IFont *GetSquareDesignFont(FontManager *self) {
				IFont *font = self->GetSquareDesignFont();
				font->AddRef();
				return font;
			}

		public:
			FontManagerRegistrar() : ScriptObjectRegistrar("FontManager") {}
			virtual void Register(ScriptManager *manager, Phase phase) {
				asIScriptEngine *eng = manager->GetEngine();
				int r;
				eng->SetDefaultNamespace("spades");
				switch(phase){
					case PhaseObjectType:
						r = eng->RegisterObjectType("FontManager",
													0, asOBJ_REF);
						manager->CheckError(r);
						r = eng->RegisterObjectBehaviour("FontManager",
														 asBEHAVE_ADDREF, "void f()",
														 asMETHOD(FontManager, AddRef),
														 asCALL_THISCALL);
						manager->CheckError(r);
						r = eng->RegisterObjectBehaviour("FontManager",
														 asBEHAVE_RELEASE, "void f()",
														 asMETHOD(FontManager, Release),
														 asCALL_THISCALL);
						manager->CheckError(r);
						r = eng->RegisterObjectMethod("FontManager",
													  "Font@ get_GuiFont()",
													  asFUNCTION(GetGuiFont),
													  asCALL_CDECL_OBJLAST);
						manager->CheckError(r);
						r = eng->RegisterObjectMethod("FontManager",
													  "Font@ get_HeadingFont()",
													  asFUNCTION(GetHeadingFont),
													  asCALL_CDECL_OBJLAST);
						manager->CheckError(r);
						r = eng->RegisterObjectMethod("FontManager",
													  "Font@ get_LargeFont()",
													  asFUNCTION(GetLargeFont),
													  asCALL_CDECL_OBJLAST);
						manager->CheckError(r);
						r = eng->RegisterObjectMethod("FontManager",
													  "Font@ get_SquareDesignFont()",
													  asFUNCTION(GetSquareDesignFont),
													  asCALL_CDECL_OBJLAST);
						manager->CheckError(r);

						break;
					case PhaseObjectMember:
						break;
					default:

						break;
				}
			}
		};

		static FontManagerRegistrar registrar;
	}
}
