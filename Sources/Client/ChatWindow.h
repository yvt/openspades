//
//  ChatWindow.h
//  OpenSpades
//
//  Created by yvt on 7/18/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

#pragma once

#include <string>
#include <list>
#include "../Core/Math.h"

namespace spades {
	namespace client {
		class IRenderer;
		class IFont;
		class Client;
		
		static const char MsgColorTeam1 = 1;
		static const char MsgColorTeam2 = 2;
		static const char MsgColorTeam3 = 3;
		static const char MsgColorRed = 4;
		static const char MsgColorFriendlyFire = MsgColorRed;
		static const char MsgColorGreen = 5;
		static const char MsgColorSysInfo = MsgColorGreen;
		static const char MsgColorRestore = 6;
		static const char MsgColorMax = 9;
		
		class ChatWindow {
			Client *client;
			IRenderer *renderer;
			IFont *font;
			
			struct ChatEntry {
				std::string msg;
				float height;
				float fade;		// usual fade opacity
				float timeFade; // timeout fade opacity
			};
			
			std::list<ChatEntry> entries;
			float firstY;
			bool killfeed;
			
			float GetWidth();
			float GetHeight();
			float GetLineHeight();
			
			Vector4 GetColor(char);
			
		public:
			ChatWindow(Client *, IFont *font,
					   bool killfeed);
			~ChatWindow();
			
			void AddMessage(const std::string&);
			static std::string ColoredMessage(const std::string&, char);
			static std::string TeamColorMessage(const std::string&, int);
			
			void Update(float dt);
			void Draw();
		};
	}
}
