//
//  IFont.h
//  OpenSpades
//
//  Created by yvt on 7/18/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

#pragma once

#include "../Core/Math.h"

namespace spades {
	namespace client{
		class IFont {
		public:
			virtual ~IFont();
			
			virtual Vector2 Measure(const std::string&) = 0;
			virtual void Draw(const std::string&,
							  Vector2 offset,
							  float scale,
							  Vector4 color) = 0;
			
		};
	}
}