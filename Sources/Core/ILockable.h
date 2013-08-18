//
//  ILockable.h
//  OpenSpades
//
//  Created by yvt on 7/11/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

#pragma once

namespace spades {
	class ILockable {
	public:
		virtual void Lock() = 0;
		virtual void Unlock() = 0;
	};
}
