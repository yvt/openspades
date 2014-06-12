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

#include "Client.h"
#include "Arena.h"

namespace spades { namespace ngclient {
	
	void Client::MouseEvent(float x, float y) {
		switch (GetInputRoute()) {
			case InputRoute::Arena:
				arena->MouseEvent(x, y);
				break;
			default:
				SPAssert(false);
		}
	}
	
	void Client::KeyEvent(const std::string &key, bool down) {
		switch (GetInputRoute()) {
			case InputRoute::Arena:
				arena->KeyEvent(key, down);
				break;
			default:
				SPAssert(false);
		}
	}
	
	void Client::TextInputEvent(const std::string &key) {
		switch (GetInputRoute()) {
			case InputRoute::Arena:
				arena->TextInputEvent(key);
				break;
			default:
				SPAssert(false);
		}
	}
	
	void Client::TextEditingEvent(const std::string &key,
								  int start, int len) {
		switch (GetInputRoute()) {
			case InputRoute::Arena:
				arena->TextEditingEvent(key, start, len);
				break;
			default:
				SPAssert(false);
		}
	}
	
	bool Client::AcceptsTextInput() {
		switch (GetInputRoute()) {
			case InputRoute::Arena:
				return arena->AcceptsTextInput();
			default:
				SPAssert(false);
		}
		return false;
	}
	AABB2 Client::GetTextInputRect() {
		switch (GetInputRoute()) {
			case InputRoute::Arena:
				return arena->GetTextInputRect();
			default:
				SPAssert(false);
		}
		return AABB2();
	}
	
	bool Client::NeedsAbsoluteMouseCoordinate() {
		switch (GetInputRoute()) {
			case InputRoute::Arena:
				return arena->NeedsAbsoluteMouseCoordinate();
			default:
				SPAssert(false);
		}
		return false;
	}
	
	void Client::WheelEvent(float x, float y) {
		switch (GetInputRoute()) {
			case InputRoute::Arena:
				arena->WheelEvent(x, y);
				break;
			default:
				SPAssert(false);
		}
	}
	
} }

