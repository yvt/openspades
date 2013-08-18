//
//  GLBasicShadowMapRenderer.h
//  OpenSpades
//
//  Created by yvt on 7/26/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

#include "IGLShadowMapRenderer.h"
#include "IGLDevice.h"

namespace spades {
	namespace draw {
		class GLRenderer;
		class GLShadowShader;
		class GLBasicShadowMapRenderer: public IGLShadowMapRenderer {
			friend class GLShadowMapShader;
			friend class GLShadowShader;
			
			enum {NumSlices = 3};
			
			IGLDevice *device;
			
			int textureSize;
			
			IGLDevice::UInteger framebuffer[NumSlices];
			IGLDevice::UInteger texture[NumSlices];
			
			// not used, but required
			IGLDevice::UInteger colorTexture;
			
			Matrix4 matrix;
			Matrix4 matrices[3];
			OBB3 obb;
			float vpWidth, vpHeight; // used for culling
			
			void BuildMatrix(float near, float far);
		public:
			
			GLBasicShadowMapRenderer(GLRenderer *);
			virtual ~GLBasicShadowMapRenderer();
			virtual void Render();
			
			virtual bool Cull(const AABB3&);
			virtual bool SphereCull(const Vector3& center, float rad);
		};
	}
}
