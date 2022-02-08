/*

 Copyright (c) 2022 non_performing

 This file is part of OpenSpades+.

 OpenSpades+ is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 OpenSpades+ is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with OpenSpades+. If not, see <http://www.gnu.org/licenses/>.

*/

#include "OpenSpadesPlus.h"
#include <iostream>
#include <Core/Settings.h>

DEFINE_SPADES_SETTING(p_viewmodel, "0");
DEFINE_SPADES_SETTING(p_showCustomClientMessage, "0");
DEFINE_SPADES_SETTING(p_customClientMessage, "");
DEFINE_SPADES_SETTING(p_showIP, "1");
DEFINE_SPADES_SETTING(p_showAccuracyInStats, "1");
DEFINE_SPADES_SETTING(p_showAccuracyUnderMap, "0");
DEFINE_SPADES_SETTING(p_streamer, "0");
DEFINE_SPADES_SETTING(p_corpse, "0");

DEFINE_SPADES_SETTING(p_hurtTint, "1");
DEFINE_SPADES_SETTING(p_hurtBlood, "0");

namespace spades {
	namespace plus {
    		const int revision = 9;
	} // namespace client
} // namespace spades
