/*
 Copyright (c) 2013 yvt
 
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

#include "Arena.h"
#include <Core/Debug.h>
#include "NetworkClient.h"
#include "Client.h"
#include <Game/PlayerEntity.h>

namespace spades { namespace ngclient {
	
	void Arena::MouseEvent(float x, float y) {
		SPADES_MARK_FUNCTION();
		auto *lp = GetLocalPlayerEntity();
		// TODO: mouse sens, alive check, ...
		if (lp) {
			lp->GetTrajectory().eulerAngle.z += x * .005f;
			lp->GetTrajectory().eulerAngle.x =
			std::max<float>(std::min<float>(lp->GetTrajectory().eulerAngle.x + y * .005f,
							  M_PI * .499f), M_PI * -.499f);
		}
	}
	
	void Arena::KeyEvent(const std::string &key, bool down) {
		SPADES_MARK_FUNCTION();
		// TODO: key mapper
		// TODO: better movement
		if (key == "J" && down) {
			client->net->SendGenericCommand({"join"});
		} else if (key == "W") {
			auto *lp = GetLocalPlayerEntity();
			if (lp){
				auto pi = lp->GetPlayerInput();
				pi.ymove = down ? 127 : 0;
				lp->UpdatePlayerInput(pi);
			}
		} else if (key == "S") {
			auto *lp = GetLocalPlayerEntity();
			if (lp){
				auto pi = lp->GetPlayerInput();
				pi.ymove = down ? -127 : 0;
				lp->UpdatePlayerInput(pi);
			}
		} else if (key == "A") {
			auto *lp = GetLocalPlayerEntity();
			if (lp){
				auto pi = lp->GetPlayerInput();
				pi.xmove = down ? 127 : 0;
				lp->UpdatePlayerInput(pi);
			}
		} else if (key == "D") {
			auto *lp = GetLocalPlayerEntity();
			if (lp){
				auto pi = lp->GetPlayerInput();
				pi.xmove = down ? -127 : 0;
				lp->UpdatePlayerInput(pi);
			}
		}
	}
	
	void Arena::TextInputEvent(const std::string &key) {
		SPADES_MARK_FUNCTION();
		
	}
	
	void Arena::TextEditingEvent(const std::string &key,
								 int start, int len) {
		SPADES_MARK_FUNCTION();
		
	}
	
	bool Arena::AcceptsTextInput() {
		SPADES_MARK_FUNCTION();
		return false;
	}
	AABB2 Arena::GetTextInputRect() {
		SPADES_MARK_FUNCTION();
		return AABB2();
	}
	
	bool Arena::NeedsAbsoluteMouseCoordinate() {
		SPADES_MARK_FUNCTION();
		// TODO
		return GetLocalPlayerEntity() ? false :  true;
	}
	
	void Arena::WheelEvent(float x, float y) {
		SPADES_MARK_FUNCTION();
		
	}
	
} }

