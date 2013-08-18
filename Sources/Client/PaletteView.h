//
//  PaletteView.h
//  OpenSpades
//
//  Created by yvt on 8/5/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

#pragma once

#include <string>
#include "../Core/Math.h"

namespace spades {
	namespace client {
		class Client;
		class IRenderer;
		class PaletteView {
			Client *client;
			IRenderer *renderer;
			
			int defaultColor;
			std::vector<IntVector3> colors;
			int GetSelectedIndex();
			int GetSelectedOrDefaultIndex();
			
			void SetSelectedIndex(int);
		public:
			PaletteView(Client *);
			~PaletteView();
			
			bool KeyInput(std::string);
			
			void Update();
			void Draw();
		};
	}
}
