//
//  CenterMessageView.h
//  OpenSpades
//
//  Created by yvt on 7/19/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

#pragma once

#include <string>
#include <vector>
#include <list>

namespace spades{
	namespace client {
		class Client;
		class IFont;
		class IRenderer;
		
		class CenterMessageView {
			struct Entry {
				std::string msg;
				float fade;
				int line;
			};
			
			Client *client;
			IRenderer *renderer;
			IFont *font;
			std::vector<bool> lineUsing;
			std::list<Entry> entries;
			
			int GetFreeLine();
			
		public:
			CenterMessageView(Client *,
							  IFont *);
			~CenterMessageView();
			
			void AddMessage(const std::string&);
			void Update(float dt);
			void Draw();
		};
	}
}