//
//  GLImageRenderer.h
//  OpenSpades
//
//  Created by yvt on 7/18/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

#pragma once

#include "GLProgram.h"
#include "GLProgramAttribute.h"
#include "GLProgramUniform.h"
#include <stdint.h>

namespace spades{
	namespace draw {
		class GLImage;
		class IGLDevice;
		class GLRenderer;
		class GLImageRenderer {
			GLRenderer *renderer;
			IGLDevice *device;
			GLImage *image;
			
			GLProgram *program;
			
			GLProgramAttribute *positionAttribute;
			GLProgramAttribute *colorAttribute;
			GLProgramAttribute *textureCoordAttribute;
			
			GLProgramUniform *screenSize;
			GLProgramUniform *textureSize;
			GLProgramUniform *texture;
			
			struct ImageVertex {
				float x, y, u, v;
				float r, g, b, a;
			};
		
			std::vector<ImageVertex> vertices;
			std::vector<uint32_t> indices;
		public:
			GLImageRenderer(GLRenderer *renderer);
			~GLImageRenderer();
			
			void Flush();
			
			void SetImage(GLImage *);
			
			void Add(float dx1, float dy1,
					 float dx2, float dy2,
					 float dx3, float dy3,
					 float dx4, float dy4,
					 float sx1, float sy1,
					 float sx2, float sy2,
					 float sx3, float sy3,
					 float sx4, float sy4,
					 float r, float g, float b, float a);
			
		};
	}
}