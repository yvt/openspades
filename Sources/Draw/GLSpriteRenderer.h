//
//  GLSpriteRenderer.h
//  OpenSpades
//
//  Created by yvt on 7/20/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

#pragma once

#include "../Core/Math.h"
#include <vector>
#include <stdint.h>
#include "GLProgramAttribute.h"
#include "GLProgramUniform.h"
#include "IGLSpriteRenderer.h"

namespace spades {
	namespace draw {
		class GLRenderer;
		class IGLDevice;
		class GLImage;
		class GLSpriteRenderer: public IGLSpriteRenderer {
			struct Sprite {
				GLImage *image;
				Vector3 center;
				float radius;
				float angle;
				Vector4 color;
			};
			
			struct Vertex {
				// center position
				float x, y, z;
				float radius;
				
				// point coord
				float sx, sy;
				float angle;
				
				// color
				float r, g, b, a;
			};
			
			GLRenderer *renderer;
			IGLDevice *device;
			std::vector<Sprite> sprites;
			
			GLImage *lastImage;
			
			std::vector<Vertex> vertices;
			std::vector<uint32_t> indices;
			
			GLProgram *program;
			GLProgramUniform projectionViewMatrix;
			GLProgramUniform rightVector;
			GLProgramUniform upVector;
			GLProgramUniform texture;
			GLProgramUniform viewMatrix;
			GLProgramUniform fogDistance;
			GLProgramUniform fogColor;
			
			GLProgramAttribute positionAttribute;
			GLProgramAttribute spritePosAttribute;
			GLProgramAttribute colorAttribute;
			
			void Flush();
			
		public:
			GLSpriteRenderer(GLRenderer *);
			virtual ~GLSpriteRenderer();
			
			virtual void Add(GLImage *img, Vector3 center,
					 float rad, float ang, Vector4 color);
			virtual void Clear();
			virtual void Render();
		};
	}
}