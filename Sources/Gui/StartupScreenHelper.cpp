/*
 Copyright (c) 2013 yvt
 Portion of the code is based on Serverbrowser.cpp.
 
 This file is part of OpenSpades.
 
 OpenSpades is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 OpenSpades is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with OpenSpades.  If not, see <http://www.gnu.org/licenses/>.
 
 */
#include <OpenSpades.h>
#include "StartupScreenHelper.h"
#include "StartupScreen.h"
#include <Core/Settings.h>
#include <algorithm>
#include <cctype>

namespace spades {
	namespace gui {
		
		StartupScreenHelper::StartupScreenHelper(StartupScreen *scr):
		scr(scr){
			SPADES_MARK_FUNCTION();
		}
		
		StartupScreenHelper::~StartupScreenHelper() {
			SPADES_MARK_FUNCTION();
		}
		
		void StartupScreenHelper::StartupScreenDestroyed() {
			SPADES_MARK_FUNCTION();
			scr = nullptr;
		}
		
		void StartupScreenHelper::Start() {
			if(scr == nullptr){
				return;
			}
			scr->Start();
		}
				
	}
}

