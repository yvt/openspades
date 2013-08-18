//
//  Quake3Font.h
//  OpenSpades
//
//  Created by yvt on 7/18/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

#pragma once

#include "IFont.h"
#define PROP_SPACE_WIDTH -2

namespace spades {
	namespace client {
		class IRenderer;
		class IImage;
		class Quake3Font: public IFont {
			
			enum GlyphType {
				Invalid = 0,
				Image,
				Space
			};
			struct GlyphInfo {
				GlyphType type;
				AABB2 imageRect;
				
			};
			
			IRenderer *renderer;
			IImage *tex;
			int glyphHeight;
			std::vector<GlyphInfo> glyphs;
			float spaceWidth;
		public:
			Quake3Font(IRenderer *,
					   IImage *texture,
					   const int *map,
					   int glyphHeight,
					   float spaceWidth);
			virtual ~Quake3Font();
			
			virtual Vector2 Measure(const std::string&);
			virtual void Draw(const std::string&,
							  Vector2 offset,
							  float scale,
							  Vector4 color);
		};
	}
}