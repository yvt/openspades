/*
 Copyright (c) 2013 yvt
 based on code of pysnip (c) Mathias Kaerlev 2011-2012.

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

#include "Debug.h"
#include "Exception.h"

namespace spades {
	// FIXME: namespace pollution...
	static const size_t NoFreeRegion = static_cast<size_t>(-1);
	class MiniHeap {
	public:
		typedef size_t Ref;
		template <typename T> struct Handle {
			MiniHeap *heap;
			Ref ref;
			inline Handle(MiniHeap *heap, Ref ref) : heap(heap), ref(ref) {}
			inline T *operator->() const { return heap->Dereference<T>(ref); }
			inline operator T *() const { return heap->Dereference<T>(ref); }
			inline operator Ref() const { return ref; }
			inline void Delete() {
				// FIXME: call destructor?
				heap->Free(ref, sizeof(T));
			}
		};

	private:
		std::vector<char> buffer;
		std::vector<Ref> freeRegionPool;

		Ref firstFreeRegion;
		Ref lastFreeRegion;

		struct FreeRegion {
			Ref start, len;
			Ref prev, next;
			Ref GetEnd() const { return start + len; }
		};

		Handle<FreeRegion> AllocFreeRegion() {
			if (freeRegionPool.empty())
				return Alloc<FreeRegion>();
			else {
				auto r = freeRegionPool.back();
				freeRegionPool.pop_back();
				return Handle<FreeRegion>(this, r);
			}
		}
		void ReleaseFreeRegion(Ref r) {
			freeRegionPool.push_back(r);
			auto *fr = Dereference<FreeRegion>(r);
			fr->start = 0xdeadbeef;
			fr->len = 0xdeadbeef;
			fr->prev = 0xdeadbeef;
			fr->next = 0xdeadbeef;
		}

		Ref FindFreeRegion(size_t bytes) {
			Ref fl = firstFreeRegion;
			while (fl != NoFreeRegion) {
				auto *f = Dereference<FreeRegion>(fl);
				if (f->len >= bytes)
					return fl;
				SPAssert(f->next != fl);
				fl = f->next;
			}
			return NoFreeRegion;
		}

		void DeleteFreeListNode(Ref freg) {
			auto *f = Dereference<FreeRegion>(freg);
			if (f->prev == NoFreeRegion) {
				if (f->next == NoFreeRegion) {
					firstFreeRegion = NoFreeRegion;
					lastFreeRegion = NoFreeRegion;
				} else {
					firstFreeRegion = f->next;
					Dereference<FreeRegion>(f->next)->prev = NoFreeRegion;
				}
			} else {
				if (f->next == NoFreeRegion) {
					lastFreeRegion = f->prev;
					Dereference<FreeRegion>(f->prev)->next = NoFreeRegion;
				} else {
					Dereference<FreeRegion>(f->next)->prev = f->prev;
					Dereference<FreeRegion>(f->prev)->next = f->next;
				}
			}
			ReleaseFreeRegion(freg);
			SPAssert(Validate());
		}

	public:
		MiniHeap(size_t initialSize) {
			if (initialSize < sizeof(FreeRegion))
				initialSize = sizeof(FreeRegion);
			buffer.resize(initialSize);

			// initialize first free region
			if (initialSize > sizeof(FreeRegion)) {
				firstFreeRegion = 0;
				lastFreeRegion = 0;
				auto *r = Dereference<FreeRegion>(firstFreeRegion);
				r->start = sizeof(FreeRegion);
				r->len = buffer.size() - r->start;
				r->prev = NoFreeRegion;
				r->next = NoFreeRegion;
			} else {
				firstFreeRegion = NoFreeRegion;
				lastFreeRegion = NoFreeRegion;
			}
			SPAssert(Validate());
		}
		bool Validate();
		void Reserve(size_t bytes) {
			size_t newSize = buffer.size();
			while (newSize < bytes)
				newSize <<= 1;
			if (newSize == buffer.size())
				return;
			size_t oldSize = buffer.size();
			buffer.resize(newSize);

			if (lastFreeRegion == NoFreeRegion) {
				firstFreeRegion = oldSize;
				lastFreeRegion = oldSize;
				auto *r = Dereference<FreeRegion>(firstFreeRegion);
				r->start = oldSize + sizeof(FreeRegion);
				r->len = buffer.size() - r->start;
				r->prev = NoFreeRegion;
				r->next = NoFreeRegion;
			} else {
				Ref oldLastFree = lastFreeRegion;
				auto *r = Dereference<FreeRegion>(lastFreeRegion);
				if (r->GetEnd() == oldSize) {
					r->len = buffer.size() - r->start;
				} else {
					lastFreeRegion = buffer.size() - sizeof(FreeRegion);
					auto *nr = Dereference<FreeRegion>(lastFreeRegion);
					nr->start = oldSize;
					nr->len = lastFreeRegion - nr->start;
					nr->prev = oldLastFree;
					nr->next = NoFreeRegion;
					r->next = lastFreeRegion;
				}
			}
			SPAssert(Validate());
		}
		Ref Alloc(size_t bytes) {

			auto freg = FindFreeRegion(bytes);
			if (freg == NoFreeRegion) {
				Reserve(buffer.size() + bytes);
				freg = FindFreeRegion(bytes);
			}

			SPAssert(freg != NoFreeRegion);

			auto *f = Dereference<FreeRegion>(freg);
			SPAssert(f->len >= bytes);
			auto pos = f->start;
			if (f->len == bytes) {
				// free region is removed
				DeleteFreeListNode(freg);
			} else {
				f->start += bytes;
				f->len -= bytes;
			}
			SPAssert(pos + bytes <= buffer.size());
			SPAssert(Validate());
			return pos;
		}
		template <typename T> Handle<T> Alloc() {
			Ref r = Alloc(sizeof(T));
			// FIXME: call constructor?
			return Handle<T>(this, r);
		}
		void Free(Ref offset, Ref len) {
			Ref fl = firstFreeRegion;
			Ref last = fl;
			Validate();
			auto extraFreeRegion = NoFreeRegion;
		TryAgain:
			fl = firstFreeRegion;
			while (fl != NoFreeRegion) {
				last = fl;
				auto *f = Dereference<FreeRegion>(fl);
				if (f->GetEnd() == offset) {
					f->len += len;

					// recombine?
					if (f->next != NoFreeRegion) {
						auto *fn = Dereference<FreeRegion>(f->next);
						if (fn->start == f->GetEnd()) {
							// recombine
							f->len += fn->len;
							DeleteFreeListNode(f->next);
						}
					}
					SPAssert(Validate());
					goto EoP;
				} else if (f->GetEnd() > offset && f->start < offset + len) {
					SPRaise("Internal inconsistency detected: double free");
				} else if (f->start == offset + len) {
					f->len += len;
					f->start -= len;

					// recombine?
					if (f->prev != NoFreeRegion) {
						auto *fn = Dereference<FreeRegion>(f->prev);
						if (fn->GetEnd() == f->start) {
							// recombine
							f->len += fn->len;
							f->start -= fn->len;
							DeleteFreeListNode(f->prev);
						}
					}
					SPAssert(Validate());
					goto EoP;
				} else if (f->start > offset + len) {
					// insert here.
					Ref prev = f->prev;
					auto reg = extraFreeRegion;
					if (reg == NoFreeRegion) {
						extraFreeRegion = AllocFreeRegion();
						goto TryAgain;
					}
					extraFreeRegion = NoFreeRegion;
					f = Dereference<FreeRegion>(fl);
					f->prev = reg;
					FreeRegion *r = Dereference<FreeRegion>(reg);
					r->next = fl;
					r->prev = prev;
					r->start = offset;
					r->len = len;
					if (prev == NoFreeRegion) {
						firstFreeRegion = reg;
					} else {
						Dereference<FreeRegion>(prev)->next = reg;
					}
					SPAssert(Validate());
					goto EoP;
				}
				SPAssert(f->start >= offset + len || f->GetEnd() <= offset);
				fl = f->next;
			}

			// push back.
			if (last == NoFreeRegion) {
				auto reg = AllocFreeRegion();
				FreeRegion *r = reg;
				r->next = NoFreeRegion;
				r->prev = NoFreeRegion;
				r->start = offset;
				r->len = len;
				firstFreeRegion = reg;
				lastFreeRegion = reg;
			} else {
				auto reg = AllocFreeRegion();
				FreeRegion *r = reg;
				r->next = NoFreeRegion;
				r->prev = last;
				r->start = offset;
				r->len = len;
				lastFreeRegion = reg;
				Dereference<FreeRegion>(last)->next = reg;
			}
		EoP:
			if (extraFreeRegion != NoFreeRegion) {
				ReleaseFreeRegion(extraFreeRegion);
			}
			SPAssert(Validate());
		}
		template <typename T> T *Dereference(Ref ref) {
			return reinterpret_cast<T *>(buffer.data() + ref);
		}
	};
}
