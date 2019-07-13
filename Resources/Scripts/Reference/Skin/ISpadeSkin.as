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

namespace spades {

    /** Action what a user doing with his/her spade. */
    enum SpadeActionType { Idle, Bash, Dig, DigStart }

    /**
     * A skin of spades. A class that implements this might also have to
     * implement either IThirdPersonToolSkin or IViewToolSkin.
     */
    interface ISpadeSkin {
        SpadeActionType ActionType { set; }

        /**
         * Receives an action progress. 0 = soon after swinging,
         * 1 = ready for the next swing.
         */
        float ActionProgress { set; }
    }

}
