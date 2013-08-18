//
//  IImage.h
//  OpenSpades
//
//  Created by yvt on 7/12/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

#pragma once

namespace spades{
	namespace client {
		class IImage {
		public:
			virtual ~IImage(){}
			
			virtual float GetWidth() = 0;
			virtual float GetHeight() = 0;
		};
	}
}
