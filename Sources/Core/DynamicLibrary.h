//
//  DynamicLibrary.h
//  OpenSpades
//
//  Created by yvt on 7/14/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

#pragma once

#include <string>

namespace spades {
	class DynamicLibrary {
		std::string name;
		void *handle;
	public:
		DynamicLibrary(const char *fn);
		~DynamicLibrary();

		void *GetSymbol(const char *);
		void *GetSymbolOrNull(const char *);
	};
}
