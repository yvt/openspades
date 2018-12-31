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
#include "Mutex.h"

#define DEBUG_REFCOUNTED_OBJECT_LAST_RELEASE 0

namespace spades {

	class RefCountedObject {
		std::atomic<int> refCount;
#if DEBUG_REFCOUNTED_OBJECT_LAST_RELEASE
		reflection::BacktraceRecord lastRelease;
		reflection::BacktraceRecord secondLastRelease;
		Mutex releaseInfoMutex;
#endif
	protected:
		virtual ~RefCountedObject();

	public:
		RefCountedObject();

		void AddRef();
		void Release();
	};

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
		Handle(Handle<T> &&h) : ptr(h.MaybeUnmanage()) {}

		template <class S> Handle(Handle<S> &&h) : ptr(h.MaybeUnmanage()) {}

		template <class S> operator Handle<S>() && { return {std::move(*this)}; }

		~Handle() {
			if (ptr)
				ptr->Release();
		}

		template <class... Args> static Handle New(Args &&... args) {
			T *ptr = new T{std::forward<Args>(args)...};
			return {ptr, false};
		}

		T *operator->() {
			SPAssert(ptr != NULL);
			return ptr;
		}
		const T *operator->() const {
			SPAssert(ptr != NULL);
			return ptr;
		}
		T &operator*() {
			SPAssert(ptr != NULL);
			return *ptr;
		}
		const T &operator*() const {
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
		operator T *() { return ptr; }
		T *Unmanage() {
			SPAssert(ptr != NULL);
			T *p = ptr;
			ptr = NULL;
			return p;
		}
		T *MaybeUnmanage() {
			T *p = ptr;
			ptr = NULL;
			return p;
		}
		operator bool() { return ptr != NULL; }

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
		template <class S> Handle<S> Cast() const & {
			return Handle{*this}.Cast<S>();
		}
	};
}
