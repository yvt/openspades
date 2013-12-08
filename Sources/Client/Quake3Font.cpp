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

#include "Quake3Font.h"
#include "IRenderer.h"
#include <Core/Debug.h>

namespace spades {
	namespace client {
		Quake3Font::Quake3Font(IRenderer *r, IImage *tex, const int *mp, int gh, float sw)
			:renderer(r), tex(tex),
			glyphHeight(gh), spaceWidth(sw)
		{
			SPADES_MARK_FUNCTION();
			
			tex->AddRef();
			for(int i = 0; i < 128; i++){
				int x = *(mp++);
				int y = *(mp++);
				int w = *(mp++);
				
				GlyphInfo info;
				if(w == -1){
					info.type = Invalid;
					glyphs.push_back(info);
					continue;
				}
				if(w == PROP_SPACE_WIDTH){
					info.type = Space;
					glyphs.push_back(info);
					continue;
				}
				
				info.type = Image;
				info.imageRect = AABB2(x, y, w, gh);
				
				glyphs.push_back(info);
			}
		}
		
		Quake3Font::~Quake3Font(){
			SPADES_MARK_FUNCTION();
			tex->Release();
		}
		
		Vector2 Quake3Font::Measure(const std::string &txt) {
			SPADES_MARK_FUNCTION();
			
			float x = 0.f, w = 0.f, h = (float)glyphHeight;
			for(size_t i = 0; i < txt.size(); i++){
				int ch = ((int)txt[i]) & 127;
				
				if(ch == 13 || ch == 10){
					// new line
					x = 0.f;
					h += (float)glyphHeight;
					continue;
				}
				
				SPAssert(ch >= 0); SPAssert(ch < 128);
				const GlyphInfo& info = glyphs[ch];
				
				if(info.type == Invalid)
					continue;
				else if(info.type == Space){
					x += spaceWidth;
				}else if(info.type == Image ) {
					x += info.imageRect.GetWidth();
				}
				
				if(x > w){
					w = x;
				}
			}
			return MakeVector2(w, h);
		}
		
		void Quake3Font::Draw(const std::string &txt, spades::Vector2 offset, float scale, spades::Vector4 color) {
			float x = 0.f, y = 0;
			
			if(scale == 1.f){
				offset.x = floorf(offset.x);
				offset.y = floorf(offset.y);
			}
			
			float a = color.w;
			color.w = 1.f;
			color *= a;
			renderer->SetColorAlphaPremultiplied(color);
			
			for(size_t i = 0; i < txt.size(); i++){
				int ch = ((int)txt[i]) & 127;
				
				if(ch == 13 || ch == 10){
					// new line
					x = 0.f;
					y += (float)glyphHeight;
					continue;
				}
				
				SPAssert(ch >= 0); SPAssert(ch < 128);
				const GlyphInfo& info = glyphs[ch];
				
				if(info.type == Invalid)
					continue;
				else if(info.type == Space){
					x += spaceWidth;
				}else if(info.type == Image ) {
					AABB2 rt(x * scale + offset.x, y * scale + offset.y, info.imageRect.GetWidth() * scale, info.imageRect.GetHeight() * scale);
					renderer->DrawImage(tex, rt, info.imageRect);
					x += info.imageRect.GetWidth();
				}
				
			}
		}
	}
}




