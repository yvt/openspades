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

#include <cstring>
#include <deque>
#include <set>
#include <vector>

#include "GameMap.h"
#include "GameMapWrapper.h"
#include <Core/Debug.h>
#include <Core/Debug.h>
#include <Core/Deque.h>
#include <Core/Stopwatch.h>

namespace spades {
	namespace client {

		GameMapWrapper::GameMapWrapper(GameMap &mp) : map(mp) {
			SPADES_MARK_FUNCTION();

			width = mp.Width();
			height = mp.Height();
			depth = mp.Depth();
			linkMap.reset(new uint8_t[width * height * depth]);
			memset(linkMap.get(), 0, width * height * depth);
		}

		GameMapWrapper::~GameMapWrapper() { SPADES_MARK_FUNCTION(); }

		void GameMapWrapper::Rebuild() {
			SPADES_MARK_FUNCTION();

			Stopwatch stopwatch;

			GameMap &m = map;
			memset(linkMap.get(), 0, width * height * depth);

			for (int x = 0; x < width; x++)
				for (int y = 0; y < height; y++)
					SetLink(x, y, depth - 1, Root);

			Deque<CellPos> queue(width * height * 2);

			for (int x = 0; x < width; x++)
				for (int y = 0; y < height; y++)
					if (m.IsSolid(x, y, depth - 2)) {
						SetLink(x, y, depth - 2, PositiveZ);
						queue.Push(CellPos(x, y, depth - 2));
					}

			while (!queue.IsEmpty()) {
				CellPos p = queue.Front();
				queue.Shift();

				int x = p.x, y = p.y, z = p.z;

				if (p.x > 0 && m.IsSolid(x - 1, y, z) && GetLink(x - 1, y, z) == Invalid) {
					SetLink(x - 1, y, z, PositiveX);
					queue.Push(CellPos(x - 1, y, z));
				}
				if (p.x < width - 1 && m.IsSolid(x + 1, y, z) && GetLink(x + 1, y, z) == Invalid) {
					SetLink(x + 1, y, z, NegativeX);
					queue.Push(CellPos(x + 1, y, z));
				}
				if (p.y > 0 && m.IsSolid(x, y - 1, z) && GetLink(x, y - 1, z) == Invalid) {
					SetLink(x, y - 1, z, PositiveY);
					queue.Push(CellPos(x, y - 1, z));
				}
				if (p.y < height - 1 && m.IsSolid(x, y + 1, z) && GetLink(x, y + 1, z) == Invalid) {
					SetLink(x, y + 1, z, NegativeY);
					queue.Push(CellPos(x, y + 1, z));
				}
				if (p.z > 0 && m.IsSolid(x, y, z - 1) && GetLink(x, y, z - 1) == Invalid) {
					SetLink(x, y, z - 1, PositiveZ);
					queue.Push(CellPos(x, y, z - 1));
				}
				if (p.z < depth - 1 && m.IsSolid(x, y, z + 1) && GetLink(x, y, z + 1) == Invalid) {
					SetLink(x, y, z + 1, NegativeZ);
					queue.Push(CellPos(x, y, z + 1));
				}
			}

			SPLog("%.3f msecs to rebuild", stopwatch.GetTime() * 1000.);
		}

		void GameMapWrapper::AddBlock(int x, int y, int z, uint32_t color) {
			SPADES_MARK_FUNCTION();

			GameMap &m = map;

			if (GetLink(x, y, z) != Invalid) {
				SPAssert(m.IsSolid(x, y, z));
				return;
			}

			m.Set(x, y, z, true, color);

			if (GetLink(x, y, z) != Invalid) {
				return;
			}

			LinkType l = Invalid;
			if (x > 0 && m.IsSolid(x - 1, y, z) && GetLink(x - 1, y, z) != Invalid) {
				l = NegativeX;
				SPAssert(GetLink(x - 1, y, z) != PositiveX);
			}
			if (x < width - 1 && m.IsSolid(x + 1, y, z) && GetLink(x + 1, y, z) != Invalid) {
				l = PositiveX;
				SPAssert(GetLink(x + 1, y, z) != NegativeX);
			}
			if (y > 0 && m.IsSolid(x, y - 1, z) && GetLink(x, y - 1, z) != Invalid) {
				l = NegativeY;
				SPAssert(GetLink(x, y - 1, z) != PositiveY);
			}
			if (y < height - 1 && m.IsSolid(x, y + 1, z) && GetLink(x, y + 1, z) != Invalid) {
				l = PositiveY;
				SPAssert(GetLink(x, y + 1, z) != NegativeY);
			}
			if (z > 0 && m.IsSolid(x, y, z - 1) && GetLink(x, y, z - 1) != Invalid) {
				l = NegativeZ;
				SPAssert(GetLink(x, y, z - 1) != PositiveZ);
			}
			if (z < depth - 1 && m.IsSolid(x, y, z + 1) && GetLink(x, y, z + 1) != Invalid) {
				l = PositiveZ;
				SPAssert(GetLink(x, y, z + 1) != NegativeZ);
			}
			SetLink(x, y, z, l);

			if (l == Invalid)
				return;
			// if there's invalid block around this block,
			// rebuild tree
			Deque<CellPos> queue(1024);
			queue.Push(CellPos(x, y, z));
			while (!queue.IsEmpty()) {
				CellPos p = queue.Front();
				queue.Shift();

				int x = p.x, y = p.y, z = p.z;
				SPAssert(m.IsSolid(x, y, z));

				LinkType thisLink = GetLink(x, y, z);

				if (p.x > 0 && m.IsSolid(x - 1, y, z) && GetLink(x - 1, y, z) == Invalid &&
				    thisLink != NegativeX) {
					SetLink(x - 1, y, z, PositiveX);
					queue.Push(CellPos(x - 1, y, z));
				}
				if (p.x < width - 1 && m.IsSolid(x + 1, y, z) && GetLink(x + 1, y, z) == Invalid &&
				    thisLink != PositiveX) {
					SetLink(x + 1, y, z, NegativeX);
					queue.Push(CellPos(x + 1, y, z));
				}
				if (p.y > 0 && m.IsSolid(x, y - 1, z) && GetLink(x, y - 1, z) == Invalid &&
				    thisLink != NegativeY) {
					SetLink(x, y - 1, z, PositiveY);
					queue.Push(CellPos(x, y - 1, z));
				}
				if (p.y < height - 1 && m.IsSolid(x, y + 1, z) && GetLink(x, y + 1, z) == Invalid &&
				    thisLink != PositiveY) {
					SetLink(x, y + 1, z, NegativeY);
					queue.Push(CellPos(x, y + 1, z));
				}
				if (p.z > 0 && m.IsSolid(x, y, z - 1) && GetLink(x, y, z - 1) == Invalid &&
				    thisLink != NegativeZ) {
					SetLink(x, y, z - 1, PositiveZ);
					queue.Push(CellPos(x, y, z - 1));
				}
				if (p.z < depth - 1 && m.IsSolid(x, y, z + 1) && GetLink(x, y, z + 1) == Invalid &&
				    thisLink != PositiveZ) {
					SetLink(x, y, z + 1, NegativeZ);
					queue.Push(CellPos(x, y, z + 1));
				}
			}
		}

		template <typename T> static inline bool EqualTwoCond(T a, T b, T c, bool cond) {
			return a == b || (cond && a == c);
		}

		std::vector<CellPos> GameMapWrapper::RemoveBlocks(const std::vector<CellPos> &cells) {
			SPADES_MARK_FUNCTION();

			if (cells.empty())
				return std::vector<CellPos>();

			GameMap &m = map;

			// solid, but unlinked cells
			std::vector<CellPos> unlinkedCells;
			Deque<CellPos> queue(1024);

			// unlink children
			for (size_t i = 0; i < cells.size(); i++) {
				CellPos pos = cells[i];
				m.Set(pos.x, pos.y, pos.z, false, 0);
				// if(GetLink(pos.x, pos.y, pos.z) == Invalid){
				// this block is already disconnected.
				// }

				if (GetLink(pos.x, pos.y, pos.z) == Marked) {
					continue;
				}
				SPAssert(GetLink(pos.x, pos.y, pos.z) != Root);

				SetLink(pos.x, pos.y, pos.z, Invalid);
				queue.Push(pos);

				while (!queue.IsEmpty()) {
					pos = queue.Front();
					queue.Shift();

					if (m.IsSolid(pos.x, pos.y, pos.z))
						unlinkedCells.push_back(pos);
					// don't "continue;" when non-solid

					int x = pos.x, y = pos.y, z = pos.z;
					if (x > 0 && EqualTwoCond(GetLink(x - 1, y, z), PositiveX, Invalid,
					                          m.IsSolid(x - 1, y, z))) {
						SetLink(x - 1, y, z, Marked);
						queue.Push(CellPos(x - 1, y, z));
					}
					if (x < width - 1 && EqualTwoCond(GetLink(x + 1, y, z), NegativeX, Invalid,
					                                  m.IsSolid(x + 1, y, z))) {
						SetLink(x + 1, y, z, Marked);
						queue.Push(CellPos(x + 1, y, z));
					}
					if (y > 0 && EqualTwoCond(GetLink(x, y - 1, z), PositiveY, Invalid,
					                          m.IsSolid(x, y - 1, z))) {
						SetLink(x, y - 1, z, Marked);
						queue.Push(CellPos(x, y - 1, z));
					}
					if (y < height - 1 && EqualTwoCond(GetLink(x, y + 1, z), NegativeY, Invalid,
					                                   m.IsSolid(x, y + 1, z))) {
						SetLink(x, y + 1, z, Marked);
						queue.Push(CellPos(x, y + 1, z));
					}
					if (z > 0 && EqualTwoCond(GetLink(x, y, z - 1), PositiveZ, Invalid,
					                          m.IsSolid(x, y, z - 1))) {
						SetLink(x, y, z - 1, Marked);
						queue.Push(CellPos(x, y, z - 1));
					}
					if (z < depth - 1 && EqualTwoCond(GetLink(x, y, z + 1), NegativeZ, Invalid,
					                                  m.IsSolid(x, y, z + 1))) {
						SetLink(x, y, z + 1, Marked);
						queue.Push(CellPos(x, y, z + 1));
					}
				}
			}

			// remove "visited" mark
			for (size_t i = 0; i < unlinkedCells.size(); i++) {
				const CellPos &pos = unlinkedCells[i];
				if (GetLink(pos.x, pos.y, pos.z) == Marked)
					SetLink(pos.x, pos.y, pos.z, Invalid);
			}

			SPAssert(queue.IsEmpty());

			// start relinking
			for (size_t i = 0; i < unlinkedCells.size(); i++) {
				const CellPos &pos = unlinkedCells[i];
				int x = pos.x, y = pos.y, z = pos.z;
				if (!m.IsSolid(x, y, z)) {
					// notice: (x,y,z) may be air, so
					// don't use SPAssert()
					continue;
				}

				LinkType newLink = Invalid;
				if (z < depth - 1 && GetLink(x, y, z + 1) != Invalid) {
					newLink = PositiveZ;
				} else if (x > 0 && GetLink(x - 1, y, z) != Invalid) {
					newLink = NegativeX;
				} else if (x < width - 1 && GetLink(x + 1, y, z) != Invalid) {
					newLink = PositiveX;
				} else if (y > 0 && GetLink(x, y - 1, z) != Invalid) {
					newLink = NegativeY;
				} else if (y < height - 1 && GetLink(x, y + 1, z) != Invalid) {
					newLink = PositiveY;
				} else if (z > 0 && GetLink(x, y, z - 1) != Invalid) {
					newLink = NegativeZ;
				}

				if (newLink != Invalid) {
					SetLink(x, y, z, newLink);
					queue.Push(pos);
				}
			}

			while (!queue.IsEmpty()) {
				CellPos p = queue.Front();
				queue.Shift();

				int x = p.x, y = p.y, z = p.z;
				LinkType thisLink = GetLink(x, y, z);

				if (p.x > 0 && m.IsSolid(x - 1, y, z) && GetLink(x - 1, y, z) == Invalid &&
				    thisLink != NegativeX) {
					SetLink(x - 1, y, z, PositiveX);
					queue.Push(CellPos(x - 1, y, z));
				}
				if (p.x < width - 1 && m.IsSolid(x + 1, y, z) && GetLink(x + 1, y, z) == Invalid &&
				    thisLink != PositiveX) {
					SetLink(x + 1, y, z, NegativeX);
					queue.Push(CellPos(x + 1, y, z));
				}
				if (p.y > 0 && m.IsSolid(x, y - 1, z) && GetLink(x, y - 1, z) == Invalid &&
				    thisLink != NegativeY) {
					SetLink(x, y - 1, z, PositiveY);
					queue.Push(CellPos(x, y - 1, z));
				}
				if (p.y < height - 1 && m.IsSolid(x, y + 1, z) && GetLink(x, y + 1, z) == Invalid &&
				    thisLink != PositiveY) {
					SetLink(x, y + 1, z, NegativeY);
					queue.Push(CellPos(x, y + 1, z));
				}
				if (p.z > 0 && m.IsSolid(x, y, z - 1) && GetLink(x, y, z - 1) == Invalid &&
				    thisLink != NegativeZ) {
					SetLink(x, y, z - 1, PositiveZ);
					queue.Push(CellPos(x, y, z - 1));
				}
				if (p.z < depth - 1 && m.IsSolid(x, y, z + 1) && GetLink(x, y, z + 1) == Invalid &&
				    thisLink != PositiveZ) {
					SetLink(x, y, z + 1, NegativeZ);
					queue.Push(CellPos(x, y, z + 1));
				}
			}

			std::vector<CellPos> floatingBlocks;
			floatingBlocks.reserve(unlinkedCells.size());

			for (size_t i = 0; i < unlinkedCells.size(); i++) {
				const CellPos &p = unlinkedCells[i];
				if (!m.IsSolid(p.x, p.y, p.z))
					continue;
				if (GetLink(p.x, p.y, p.z) == Invalid) {
					floatingBlocks.push_back(p);
				}
			}

			return floatingBlocks;
		}
	}
}
