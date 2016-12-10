/*
 Copyright (c) 2016 yvt

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

#include <cassert>
#include <iostream>
#include <iterator>
#include <memory>
#include <type_traits>

namespace stmp {

	// creating our own version because boost is overweighted
	// (preproecssing optional.hpp emits 50000 lines of C++ code!)
	// the corresponding type in .NET Framework is System.Nullable<T>.
	template <class T> class optional {
		typename std::aligned_storage<sizeof(T), std::alignment_of<T>::value>::type storage;
		bool has_some;
		using Allocator = std::allocator<T>;

	public:
		optional() : has_some(false) {}
		optional(const T &v) : has_some(false) { reset(v); }
		optional(T &&v) : has_some(false) { reset(std::forward<T>(v)); }
		optional(optional &o) : has_some(o.has_some) {
			if (has_some) {
				Allocator().construct(get_pointer(), o.get());
			}
		}
		optional(const optional &o) : has_some(o.has_some) {
			if (has_some) {
				Allocator().construct(get_pointer(), o.get());
			}
		}
		optional(optional &&o) : has_some(o.has_some) {
			if (has_some) {
				Allocator().construct(get_pointer(), std::move(o.get()));
				o.has_some = false;
			}
		}
		~optional() { reset(); }
		void reset() {
			if (has_some) {
				Allocator().destroy(get_pointer());
				has_some = false;
			}
		}
		template <class... Args> void reset(Args &&... args) {
			reset();
			Allocator().construct(reinterpret_cast<T *>(&storage), std::forward<Args>(args)...);
			has_some = true;
		}
		void operator=(const T &o) { reset(o); }
		void operator=(T &&o) { reset(std::move(o)); }
		void operator=(const optional &o) {
			if (o)
				reset(*o);
			else
				reset();
		}

		T *get_pointer() { return has_some ? reinterpret_cast<T *>(&storage) : nullptr; }
		const T *get_pointer() const {
			return has_some ? reinterpret_cast<const T *>(&storage) : nullptr;
		}
		T &get() {
			assert(has_some);
			return *get_pointer();
		}
		const T &get() const {
			assert(has_some);
			return *get_pointer();
		}
		T &operator->() {
			assert(has_some);
			return get();
		}
		const T &operator->() const {
			assert(has_some);
			return get();
		}

		T &operator*() {
			assert(has_some);
			return get();
		}
		const T &operator*() const {
			assert(has_some);
			return get();
		}

		explicit operator bool() const { return has_some; }
	};
}
