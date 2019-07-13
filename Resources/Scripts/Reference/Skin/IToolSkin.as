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

    /** A skin of all tools for third-person view. */
    interface IToolSkin {

        /**
         * Receives a value that indicates whether the owner of the tool is
         * sprinting. 0 = not sprinting, 1 = sprinting.
         */
        float SprintState { set; }

        /**
         * Receives a value that indicates whether this tool is raised.
         * 0 = brought down, 1 = raised.
         */
        float RaiseState { set; }

        /** Receives the team color. */
        Vector3 TeamColor { set; }

        /** Receives whether this skin should play a sound. */
        bool IsMuted { set; }

        /** Advances the animation by the specified time span. */
        void Update(float dt);

        /** Issues draw commands to add models of this tool to the scene. */
        void AddToScene();
    }

}
