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
#include <memory>

#include "IStream.h"
#include "Math.h"
#include <Core/Debug.h>
#include <Core/RefCountedObject.h>

namespace spades {
	/**
	 * Material IDs used by `VoxelModel`.
	 */
	enum class MaterialType : uint8_t {
		Default = 0,
		Emissive = 1,
	};

	class VoxelModel : public RefCountedObject {
	public:
		/**
		 * Construct an empty voxel model of the specified size.
		 */
		VoxelModel(int width, int height, int depth);

		VoxelModel(const VoxelModel &) = delete;
		void operator=(const VoxelModel &) = delete;

		/**
		 * Fill hollow spaces (if any) in the model.
		 */
		void HollowFill();

		/**
		 * Load a `VoxelModel` from a stream in the KV6 format.
		 *
		 * The KV6 format does not include material information, so the material
		 * IDs of the loaded voxels are set to `0`.
		 */
		static Handle<VoxelModel> LoadKV6(IStream &);

		/**
		 * Replace the material ID with the specified value.
		 */
		void ForceMaterial(MaterialType newMaterialId);

		/** `GetSolidBits` without bounds checking. */
		const uint64_t &GetSolidBitsAtUnchecked(int x, int y) const {
			return solidBits[x + y * width];
		}

		/** `GetSolidBits` without bounds checking. */
		uint64_t &GetSolidBitsAtUnchecked(int x, int y) { return solidBits[x + y * width]; }

		/**
		 * Get a reference to a bitfield each of whose bit `z` indicates
		 * whether the voxel `(x, y, z)` is solid (occupied) or not.
		 */
		const uint64_t &GetSolidBitsAt(int x, int y) const {
			ValidateSpan(x, y);
			return GetSolidBitsAtUnchecked(x, y);
		}

		/** See `GetSolidBitsAt(int, int) const`. */
		uint64_t &GetSolidBitsAt(int x, int y) {
			ValidateSpan(x, y);
			return GetSolidBitsAtUnchecked(x, y);
		}

		/** `GetColor` without bounds checking. */
		const uint32_t &GetColorUnchecked(int x, int y, int z) const {
			return colors[(x + y * width) * depth + z];
		}

		/** `GetColor` without bounds checking. */
		uint32_t &GetColorUnchecked(int x, int y, int z) {
			return colors[(x + y * width) * depth + z];
		}

		/**
		 * Get a reference to the color value of a voxel.
		 *
		 * The color value is a 32-bit value where the lower 24 bits represent
		 * a color. The remaining 8 bits represent a material ID. See
		 * `MaterialType` for the predefined material IDs.
		 */
		const uint32_t &GetColor(int x, int y, int z) const {
			ValidatePoint(x, y, z);
			return GetColorUnchecked(x, y, z);
		}

		/**
		 * See `GetColor(int, int int) const`.
		 */
		uint32_t &GetColor(int x, int y, int z) {
			ValidatePoint(x, y, z);
			return GetColorUnchecked(x, y, z);
		}

		/**
		 * Return a flag whether indicating the specified voxel.
		 *
		 * This method returns `false` for out-of-bound voxels.
		 */
		bool IsSolid(int x, int y, int z) const {
			if (x < 0 || y < 0 || x >= width || y >= height || z < 0 || z >= depth) {
				return false;
			}
			uint64_t bits = GetSolidBitsAtUnchecked(x, y);
			return (bits >> z) & 1;
		}

		/**
		 * Remove the specified solid voxel (if any).
		 */
		void SetAir(int x, int y, int z) {
			ValidatePoint(x, y, z);

			uint64_t mask = 1ULL << z;
			GetSolidBitsAt(x, y) &= ~mask;
		}

		/**
		 * Replace the specified voxel with a solid voxel with the specified
		 * color value.
		 */
		void SetSolid(int x, int y, int z, uint32_t color) {
			GetColor(x, y, z) = color;

			// Let `GetColor` validate `z` so that the following shift operator
			// doesn't cause UB
			uint64_t mask = 1ULL << z;
			GetSolidBitsAtUnchecked(x, y) |= mask;
		}

		/** Get the origin point. */
		Vector3 GetOrigin() const { return origin; }

		/** Set the origin point. */
		void SetOrigin(Vector3 v) { origin = v; }

		/** Get the width (the dimention along the X axis). */
		int GetWidth() const { return width; }
		/** Get the height (the dimention along the Y axis). */
		int GetHeight() const { return height; }
		/** Get the depth (the dimention along the Z axis). */
		int GetDepth() const { return depth; }

	protected:
		~VoxelModel();

	private:
		Vector3 origin;
		int width, height, depth;
		std::unique_ptr<uint64_t[]> solidBits;
		std::unique_ptr<uint32_t[]> colors;

		void ValidateSpan(int x, int y) const {
			if (static_cast<unsigned int>(x) >= static_cast<unsigned int>(width) ||
			    static_cast<unsigned int>(y) >= static_cast<unsigned int>(height)) {
				ThrowInvalidSpan(x, y);
			}
		}

		void ValidatePoint(int x, int y, int z) const {
			if (static_cast<unsigned int>(x) >= static_cast<unsigned int>(width) ||
			    static_cast<unsigned int>(y) >= static_cast<unsigned int>(height) ||
			    static_cast<unsigned int>(z) >= static_cast<unsigned int>(depth)) {
				ThrowInvalidPoint(x, y, z);
			}
		}

		void ThrowInvalidSpan [[noreturn]] (int x, int y) const;
		void ThrowInvalidPoint [[noreturn]] (int x, int y, int z) const;
	};
} // namespace spades
