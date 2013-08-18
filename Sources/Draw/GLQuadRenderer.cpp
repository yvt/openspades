//
//  GLQuadRenderer.cpp
//  OpenSpades
//
//  Created by yvt on 7/22/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

#include "GLQuadRenderer.h"
#include "GLProgramAttribute.h"
#include <stdint.h>

namespace spades {
	namespace draw {
		GLQuadRenderer::GLQuadRenderer(IGLDevice *device):
		device(device){
			
		}
		
		GLQuadRenderer::~GLQuadRenderer(){
			
		}
		
		void GLQuadRenderer::SetCoordAttributeIndex(IGLDevice::UInteger idx){
			attrIndex = idx;
			
		}
		
		void GLQuadRenderer::Draw(){
			static const uint8_t vertices[][4]={
				{0,0}, {1,0}, {1,1}, {0,1}
			};
			
			device->EnableVertexAttribArray(attrIndex, true);
			device->VertexAttribPointer(attrIndex, 2,
										IGLDevice::UnsignedByte, false,
										4, vertices);
			device->DrawArrays(IGLDevice::TriangleFan, 0, 4);
			device->EnableVertexAttribArray(attrIndex, false);
		}
	}
}

