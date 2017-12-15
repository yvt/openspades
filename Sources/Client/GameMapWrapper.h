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
#include <vector>
#include <memory>

namespace spades {
	namespace client {
		class GameMap;

		struct CellPos {
			short x, y, z;
			CellPos() = default;
			CellPos(int xx, int yy, int zz) : x(xx), y(yy), z(zz) {}

			bool operator<(const CellPos &p) const {
				if (x < p.x) {
					return true;
				} else if (x > p.x) {
					return false;
				}
				if (y < p.y) {
					return true;
				} else if (y > p.y) {
					return false;
				}
				return z < p.z;
			}
			bool operator==(const CellPos &p) const { return x == p.x && y == p.y && z == p.z; }
		};

		struct CellPosHash {
			inline std::size_t operator()(const CellPos &pos) const {
				std::size_t ret;
				if (sizeof(std::size_t) > 4) {
					ret = pos.x;
					ret <<= 16;
					ret |= pos.y;
					ret <<= 16;
					ret |= pos.z;
				} else {
					ret = pos.x;
					ret <<= 16;
					ret |= pos.y;
					ret ^= pos.z;
				}
				return ret;
			}
		};

		/** Wraps GameMap and provides floating-block detection.*/
		class GameMapWrapper {
			friend class Client; // FIXME: for debug
		public:
		private:
			GameMap &map;

			/** Each element represents where this cell is connected from. */
			std::unique_ptr<uint8_t[]> linkMap;

			enum LinkType {
				Invalid = 0,
				Root,
				NegativeX,
				PositiveX,
				NegativeY,
				PositiveY,
				NegativeZ,
				PositiveZ,

				Marked
			};

			int width, height, depth;

			inline LinkType GetLink(int x, int y, int z) {
				return (LinkType)linkMap[(x * height + y) * depth + z];
			}
			void SetLink(int x, int y, int z, LinkType l) {
				linkMap[(x * height + y) * depth + z] = l;
			}

		public:
			GameMapWrapper(GameMap &);
			~GameMapWrapper();

			/** Addes a new block. */
			void AddBlock(int x, int y, int z, uint32_t color);

			/** Removes the specified blocks, and returns floating blocks.
			 * This function, however, doesn't remove floating blocks. */
			std::vector<CellPos> RemoveBlocks(const std::vector<CellPos> &);

			void Rebuild();
		};
	}
}
