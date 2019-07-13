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

    /** Represents a game world map. */
    class GameMap {

        /**
         * Creates a new map.
         * @param width You must specify 512.
         * @param height You must specify 512.
         * @param depth You must specify 64.
         */
        GameMap(int width, int height, int depth) {}

        /** Loads a map from the specified file. */
        GameMap(const string @path) {}

        /** Gets the color of the specified voxel. */
        uint GetColor(int x, int y, int z) {}

        /** Returns if the specified voxel is solid (or air). */
        bool IsSolid(int x, int y, int z) {}

        /** Gets the color of the specified voxel. */
        uint GetColorWrapped(int x, int y, int z) {}

        /** Returns if the specified voxel is solid (or air). */
        bool IsSolidWrapped(int x, int y, int z) {}

        /** Makes the specified voxel non-solid. */
        void SetAir(int x, int y, int z) {}

        /** Makes the specified voxel solid, and sets its color. */
        void SetSolid(int x, int y, int z, uint color) {}

        /** Retrieves the width of the map. */
        int Width {
            get {}
        }

        /** Retrieves the height of the map. */
        int Height {
            get {}
        }

        /** Retrieves the depth of the map. */
        int Depth {
            get {}
        }

        bool ClipBox(int x, int y, int z) {}
        bool ClipWorld(int x, int y, int z) {}

        bool ClipBox(float x, float y, float z) {}
        bool ClipWorld(float x, float y, float z) {}

        /** Casts a ray. */
        GameMapRayCastResult CastRay(Vector3 start, Vector3 direction, int maxScanSteps);
    }

    /** GameMapRayCastResult contains the result of the ray-cast. */
    class GameMapRayCastResult {
        /** true when the ray hit a solid voxel. */
        bool hit;

        /** true when the start position is in a solid voxel. */
        bool startSolid;

        /** Position where the ray hit a solid voxel. */
        Vector3 hitPos;

        /** Coordinate of the voxel that the ray hit. */
        IntVector3 hitBlock;

        /** Normal of the face that the ray hit. */
        IntVector3 normal;
    }

}
