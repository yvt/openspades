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

    /** Represents a small voxel model. */
    class VoxelModel {

        /**
         * Creates a new voxel model.
         * @param width Width of the voxel model.
         * @param height Height of the voxel model.
         * @param depth Depth of the voxel model, which must be <= 64.
         */
        GameMap(int width, int height, int depth) {}

        /** Loads a voxel model from the specified file. */
        GameMap(const string @path) {}

        /** Gets the color of the specified voxel. */
        uint GetColor(int x, int y, int z) {}

        /** Returns if the specified voxel is solid (or air). */
        bool IsSolid(int x, int y, int z) {}

        /** Makes the specified voxel non-solid. */
        void SetAir(int x, int y, int z) {}

        /** Makes the specified voxel solid, and sets its color. */
        void SetSolid(int x, int y, int z, uint color) {}

        /** Retrieves the width of the voxel model. */
        int Width {
            get {}
        }

        /** Retrieves the height of the voxel model. */
        int Height {
            get {}
        }

        /** Retrieves the depth of the voxel model. */
        int Depth {
            get {}
        }
    }

}
