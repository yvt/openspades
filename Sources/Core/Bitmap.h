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

#pragma once

#include <cstdint>
#include <string>

#include "Debug.h"
#include <Core/RefCountedObject.h>

namespace spades {
	class IStream;
	class Bitmap : public RefCountedObject {
		int width, height;
		uint32_t *pixels;
		bool autoDelete;

	protected:
		~Bitmap();

	public:
		/**
		 * Construct an owned bitmap image of the specified size.
		 *
		 * The initial content is undefined. Throws an exception if the size is
		 * invalid or outside the predetermined safe limit.
		 */
		Bitmap(int width, int height);

		/**
		 * Construct a borrowed bitmap image.
		 *
		 * "Borrowed" means the constructed `Bitmap` just points an existing
		 * memory location where the content is stored and does not own it by
		 * itself. Thus, the content is not released when `Bitmap` is released.
		 */
		Bitmap(uint32_t *pixels, int width, int height);

		/**
		 * Load a bitmap image from the specified OpenSpades filesystem path.
		 *
		 * Throws an exception if something goes wrong, e.g., a non-existent
		 * file or a corrupted image.
		 */
		static Handle<Bitmap> Load(const std::string &);

		/**
		 * Load a bitmap image from the specified stream. The stream must be
		 * readable and seekable.
		 *
		 * Throws an exception if something goes wrong, e.g., a corrupted image.
		 */
		static Handle<Bitmap> Load(IStream &);

		/**
		 * Save the bitmap image to the specified OpenSpades filesystem path.
		 * The image format is determined from the file extension.
		 *
		 * Throws an exception if something goes wrong, e.g., an unrecognized
		 * file extension or an I/O error.
		 */
		void Save(const std::string &);

		/**
		 * Get a pointer to the undering image data.
		 *
		 * - The image is stored in a row-major format, meaning when scanning
		 *   through the image data, the X coordinate changes fastest.
		 * - There are no padding between rows. Thus, the stride is calculated
		 *   as `GetWidth() * 4` bytes.
		 * - Each pixel is encoded in the 32-bit RGBA format.
		 */
		uint32_t *GetPixels() { return pixels; }

		/** Get the width of the image, measured in pixels. */
		int GetWidth() const { return width; }

		/** Get the height of the image, measured in pixels. */
		int GetHeight() const { return height; }

		/**
		 * Get the color value at the specified point.
		 *
		 * Throws an exception if the point is out of bounds.
		 */
		uint32_t GetPixel(int x, int y) const;

		/**
		 * Replace the color value at the specified point.
		 *
		 * Throws an exception if the point is out of bounds.
		 */
		void SetPixel(int x, int y, uint32_t);

		/**
		 * Construct a brand new owned bitmap image based on the content of
		 * an existing `Bitmap`.
		 */
		Handle<Bitmap> Clone();
	};
} // namespace spades
