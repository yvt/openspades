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

#include <atomic>
#include <cassert>
#include <iostream>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <type_traits>

namespace stmp {
	struct bad_optional_access : public std::logic_error {
		bad_optional_access() : std::logic_error{"bad optional access"} {};
	};

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

		T &get() & {
			assert(has_some);
			return *get_pointer();
		}
		const T &get() const & {
			assert(has_some);
			return *get_pointer();
		}
		T &&get() && {
			assert(has_some);
			return *get_pointer();
		}
		const T &&get() const && {
			assert(has_some);
			return *get_pointer();
		}

		T &value() & {
			if (!has_some) {
				throw bad_optional_access{};
			}
			return *get_pointer();
		}
		const T &value() const & {
			if (!has_some) {
				throw bad_optional_access{};
			}
			return *get_pointer();
		}
		T &&value() && {
			if (!has_some) {
				throw bad_optional_access{};
			}
			return *get_pointer();
		}
		const T &&value() const && {
			if (!has_some) {
				throw bad_optional_access{};
			}
			return *get_pointer();
		}

		template <class U> T value_or(U &&default_value) const & {
			return *this ? get() : static_cast<T>(std::forward<U>(default_value));
		}
		template <class U> T value_or(U &&default_value) && {
			return *this ? std::move(get()) : static_cast<T>(std::forward<U>(default_value));
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

	/** Safe atomic smart pointer. */
	template <class T> class atomic_unique_ptr {
		std::atomic<T *> inner;

	public:
		inline atomic_unique_ptr() : inner{nullptr} {}
		inline atomic_unique_ptr(std::unique_ptr<T> &&x) : inner{x.release()} {}
		atomic_unique_ptr(const atomic_unique_ptr &) = delete;
		inline atomic_unique_ptr(atomic_unique_ptr &&x) : inner{x.release()} {}
		inline ~atomic_unique_ptr() { take(); }

		void operator=(const atomic_unique_ptr &) = delete;
		void operator=(atomic_unique_ptr &&x) { exchange(x.take()); }

		inline std::unique_ptr<T>
		unsafe_exchange(std::unique_ptr<T> &&desired,
		                std::memory_order order = std::memory_order_seq_cst) {
			return std::unique_ptr<T>{inner.exchange(desired.release(), order)};
		}

		inline std::unique_ptr<T> exchange(std::unique_ptr<T> &&desired) {
			return unsafe_exchange(std::move(desired));
		}

		inline std::unique_ptr<T> take() {
			auto p = unsafe_exchange(std::unique_ptr<T>{}, std::memory_order_relaxed);
			if (p) {
				std::atomic_thread_fence(std::memory_order_acquire);
			}
			return p;
		}

		inline void store(std::unique_ptr<T> &&desired) { exchange(std::move(desired)); }

		inline T *release() { return take().release(); }
	};
}
