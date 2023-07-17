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

#include <atomic>
#include <type_traits>
#include <typeinfo>

#include "Debug.h"

#define DEBUG_REFCOUNTED_OBJECT_LAST_RELEASE 0

#if DEBUG_REFCOUNTED_OBJECT_LAST_RELEASE
#include <mutex>
#endif

namespace spades {

	/**
	 * A ref-counted object that internally holds a reference count.
	 *
	 * This type is used mainly for compatibility with AngelScript's GC
	 * behavior as it's impossible to implement with `std::shared_ptr`. In
	 * general cases, `std::unique_ptr` or `std::shared_ptr` is more suitable.
	 *
	 * # Conventions
	 *
	 *  - When storing a strong reference, use `Handle<T>`. Do not attempt to
	 *    manually update the reference count unless absolutely necessary.
	 *  - Under any circumstances, do not manually call its destructor.
	 *  - Use `Handle<T>::New(...)` to consturct an object.
	 *  - Methods receive `T` via a parameter of type `T&` or
	 *    `stmp::optional<T&>`. They may create and hold a strong reference
	 *    using `Handle::Handle(T&)`. Alternatively, they can use `Handle<T>`
	 *    if it is apparent that they store the object as `Handle<T>`.
	 *  - Methods return `T` via a return type `Handle<T>`.
	 *
	 * Note that `stmp::optional` nor `Handle` can be passed to/from AngelScript
	 * safely.
	 */
	class RefCountedObject {
		std::atomic<int> refCount;
#if DEBUG_REFCOUNTED_OBJECT_LAST_RELEASE
		reflection::BacktraceRecord lastRelease;
		reflection::BacktraceRecord secondLastRelease;
		std::mutex releaseInfoMutex;
#endif
	protected:
		virtual ~RefCountedObject();

	public:
		RefCountedObject();
		RefCountedObject(const RefCountedObject &) = delete;
		void operator=(const RefCountedObject &) = delete;

		void AddRef();
		void Release();
	};

	/**
	 * A nullable smart pointer for `T <: RefCountedObject`.
	 */
	template <typename T> class Handle {
		T *ptr;

	public:
		Handle(T *ptr, bool add = true) : ptr(ptr) {
			static_assert(std::is_base_of<RefCountedObject, T>::value,
			              "T is not based on RefCountedObject");

			if (ptr && add)
				ptr->AddRef();
		}
		Handle() : ptr(0) {}
		Handle(const Handle<T> &h) : ptr(h.ptr) {
			if (ptr)
				ptr->AddRef();
		}
		Handle(Handle<T> &&h) : ptr(std::move(h).MaybeUnmanage()) {}
		Handle(T &ref) : ptr{&ref} { ptr->AddRef(); }
		Handle(stmp::optional<T &> ref) : ptr{ref.get_pointer()} { if (ptr)
				ptr->AddRef();
		}

		template <class S> Handle(Handle<S> &&h) : ptr(h.MaybeUnmanage()) {}

		template <class S> operator Handle<S>() && { return {std::move(*this)}; }

		~Handle() {
			if (ptr)
				ptr->Release();
		}

		template <class... Args> static Handle New(Args &&... args) {
			T *ptr = new T(std::forward<Args>(args)...);
			return {ptr, false};
		}

		T *operator->() const {
			// TODO: Do not skip null check in release builds
			SPAssert(ptr != NULL);
			return ptr;
		}
		T &operator*() const {
			// TODO: Do not skip null check in release builds
			SPAssert(ptr != NULL);
			return *ptr;
		}
		void Set(T *p, bool add = true) {
			if (p == ptr) {
				if ((!add) && ptr != nullptr)
					ptr->Release();
				return;
			}
			T *old = ptr;
			ptr = p;
			if (add && ptr)
				ptr->AddRef();
			if (old)
				old->Release();
		}
		void operator=(T *p) { Set(p); }
		void operator=(const Handle<T> &h) { Set(h.ptr, true); }

		operator stmp::optional<T &>() const { return ptr; }

		/**
		 * Get a nullable raw pointer. After the operation, the original `Handle`
		 * still owns a reference to the referent (if any).
		 *
		 * This conversion have a danger of causing a pointer use-after-free if
		 * used incorrectly. For example, `IImage *image = CreateImage().GetPointer();`
		 * creates a dangling pointer because the temporary value `CreateImage()`
		 * is destroyed right after initializing the variable, invalidating the
		 * pointer returned by `GetPointer()`. This is why this conversion is
		 * no longer supported as implicit casting.
		 */
		T *GetPointerOrNull() const { return ptr; }
		/**
		 * Convert a `Handle` to a raw pointer, transfering the ownership.
		 * Throws an exception if the `Handle` is null.
		 */
		T *Unmanage() && {
			SPAssert(ptr != NULL);
			T *p = ptr;
			ptr = NULL;
			return p;
		}
		/**
		 * Convert a `Handle` to a raw pointer, transfering the ownership.
		 */
		T *MaybeUnmanage() && {
			T *p = ptr;
			ptr = NULL;
			return p;
		}
		operator bool() const { return ptr != NULL; }

		bool operator==(const Handle &rhs) const { return ptr == rhs.ptr; }

		/**
		 * Attempts to cast this `Handle<T>` to `Handle<S>` using `dynamic_cast`, consuming this
		 * `Handle<T>`. Throws an exception if the cast was unsuccessful.
		 */
		template <class S> Handle<S> Cast() && {
			static_assert(std::is_base_of<RefCountedObject, S>::value,
			              "S is not based on RefCountedObject");

			if (!ptr) {
				return {};
			}

			S *other = dynamic_cast<S *>(ptr);
			if (!other) {
				SPRaise("Invalid cast from %s to %s", typeid(ptr).name(), typeid(S).name());
			}

			ptr = nullptr;
			return {other, false};
		}

		/**
		 * Attempts to cast this `Handle<T>` to `Handle<S>` using `dynamic_cast`.
		 * Throws an exception if the cast was unsuccessful.
		 */
		template <class S> Handle<S> Cast() const & { return Handle{*this}.Cast<S>(); }
	};
} // namespace spades
