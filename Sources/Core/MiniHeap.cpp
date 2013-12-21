/*
 Copyright (c) 2013 yvt
 based on code of pysnip (c) Mathias Kaerlev 2011-2012.
 
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

#include "MiniHeap.h"

namespace spades {
	bool MiniHeap::Validate() {
		Ref fl = firstFreeRegion;
		Ref minPos = 0;
		int count = 100000;
		while(fl != NoFreeRegion) {
			if((--count) < 1) {
				SPRaise("Inconsistency detected: looped linked list");
			}
			auto *f = Dereference<FreeRegion>(fl);
			if(f->start < minPos) {
				if(f->start == minPos - 1) {
					SPRaise("Inconsistency detected: uncombined");
				}
				SPRaise("Inconsistency detected: unsorted list");
			}
			if(f->GetEnd() > buffer.size()) {
				SPRaise("Inconsistency detected: overflow");
			}
			minPos = f->GetEnd() + 1; // detect uncombineds
			
			if(f->next == fl){
				SPRaise("Inconsistency detected: self link");
			}
			
			fl = f->next;
		}
		return true;
	}
}
