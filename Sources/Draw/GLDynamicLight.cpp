//
//  GLDynamicLight.cpp
//  OpenSpades
//
//  Created by yvt on 8/1/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

#include "GLDynamicLight.h"

namespace spades {
	namespace draw {
		GLDynamicLight::GLDynamicLight(const client::DynamicLightParam& param):
		param(param){
		
			if(param.type == client::DynamicLightTypeSpotlight){
				float t = tanf(param.spotAngle * .5f) * 2.f;
				Matrix4 mat;
				mat = Matrix4::FromAxis(param.spotAxis[0],
										param.spotAxis[1],
										param.spotAxis[2],
										param.origin);
				mat = mat * Matrix4::Scale(t, t, 1.f);
				
				projMatrix = mat.InversedFast();
				
				Matrix4 m = Matrix4::Identity();
				m.m[15] = 0.f;
				m.m[11] = 1.f;
				
				m.m[8] += .5f;
				m.m[9] += .5f;
				projMatrix = m * projMatrix;
			}
			
		}
		
	}
}

