//
//  ScoreboardView.h
//  OpenSpades
//
//  Created by yvt on 7/20/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

#pragma once

#include "../Core/Math.h"

namespace spades {
	namespace client {
		class Client;
		class IRenderer;
		class IImage;
		class World;
		class CTFGameMode;
		class TCGameMode;
		class ScoreboardView {
			Client *client;
			IRenderer *renderer;
			IImage *image;
			
			World *world;
			CTFGameMode *ctf;
			TCGameMode *tc;
			
			int GetTeamScore(int);
			Vector4 GetTeamColor(int);
			Vector4 AdjustColor(Vector4 col,
								float bright,
								float saturation);
			void DrawPlayers(int team,
							 float left, float top,
							 float width, float height);
		public:
			ScoreboardView(Client *);
			~ScoreboardView();
			
			void Draw();
		};
	}
}