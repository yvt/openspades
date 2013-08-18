//
//  TCProgressView.h
//  OpenSpades
//
//  Created by yvt on 8/6/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

#pragma once

namespace spades {
	namespace client {
		class Client;
		class IRenderer;
		class TCProgressView {
			Client *client;
			IRenderer *renderer;
			
			int lastTerritoryId;
			float lastTerritoryTime;
		public:
			TCProgressView(Client *);
			~TCProgressView();
			
			void Draw();
		};
	}
}