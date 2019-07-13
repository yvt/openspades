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

    /** Represents a 32-bit RGBA bitmap. */
    class Bitmap {

        /** Creates a new bitmap. */
        GameMap(int width, int height) {}

        /** Loads a bitmap from the specified file. */
        GameMap(const string @path) {}

        /** Gets the color of the specified pixel. */
        uint GetPixel(int x, int y) {}

        /** Sets the color of the specified pixel. */
        void SetPixel(int x, int y, uint color) {}

        /** Retrieves the width of the bitmap. */
        int Width {
            get {}
        }

        /** Retrieves the height of the bitmap. */
        int Height {
            get {}
        }
    }
}
