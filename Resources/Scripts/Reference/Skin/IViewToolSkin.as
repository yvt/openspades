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

    /**
     * A skin of all tools for first-person view. A class that implements
     * this might also have to implement IToolSkin.
     */
    interface IViewToolSkin {

        /** Receives a transform matrix from view coordinate to world one. */
        Matrix4 EyeMatrix { set; }

        /** Receives a swing offset that varies with a player movement. */
        Vector3 Swing { set; }

        /** Returns positions for player hands in view coordinate. */
        Vector3 LeftHandPosition { get; }
        Vector3 RightHandPosition { get; }

        /** Issues draw commands to draw 2d images, such as HUD and crosshair. */
        void Draw2D();
    }

}
