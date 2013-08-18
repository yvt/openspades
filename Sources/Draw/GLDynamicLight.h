//
//  GLDynamicLight.h
//  OpenSpades
//
//  Created by yvt on 8/1/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

#pragma once

#include "../Client/IRenderer.h"

namespace spades {
	namespace draw {
		class GLDynamicLight {
			client::DynamicLightParam param;
			Matrix4 projMatrix;
		public:
			GLDynamicLight(const client::DynamicLightParam& param);
			const client::DynamicLightParam& GetParam() const { return param; }
			
			const Matrix4& GetProjectionMatrix() const { return projMatrix; }
		};
	}
}
