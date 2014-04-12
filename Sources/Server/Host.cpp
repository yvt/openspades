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

#include "Host.h"
#include <Core/Debug.h>
#include <Core/Exception.h>

namespace spades { namespace server {
	
	Host::Host() {
		SPADES_MARK_FUNCTION();
		
		enet_initialize();
		SPLog("ENet initialized");
		
		host = enet_host_create(NULL,
								1, 1,
								100000, 100000);
		SPLog("ENet host created");
		if(!host){
			SPRaise("Failed to create ENet host");
		}
		
		if(enet_host_compress_with_range_coder(host) < 0)
			SPRaise("Failed to enable ENet Range coder.");
		
		SPLog("ENet Range Coder Enabled");
	}
	
	Host::~Host() {
		if(host) enet_host_destroy(host);
		SPLog("ENet host destroyed");
	}
	
} }

