//
//  Stopwatch.h
//  OpenSpades
//
//  Created by yvt on 7/14/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

#pragma once

#include <stdint.h>

namespace spades {
	class Stopwatch {
		double start;
	public:
		Stopwatch();
		void Reset();
		
		/** @return elapsed time in seconds */
		double GetTime();
	};
}
