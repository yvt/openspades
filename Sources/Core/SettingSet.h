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

#include <functional>
#include <type_traits>
#include <vector>

#include <Core/Settings.h>

namespace spades {
	/**
	 * Defines a group of settings. Please see GLSettings.h for the usage.
	 *
	 * Warning: This class is not thread safe.
	 */
	class SettingSet {
	public:
		enum class ItemFlags {
			None = 0,

			/**
		     * Specifies that the value read held by an ItemHandle doesn't change once
		     * the ItemHandle was created. Call SettingSet::ReloadAll or ItemHandle::
		     * Reload to read the latest value.
		     */
			Latch = 1 << 0
		};

		/**
		 * Every instance of ItemHandle must be declared as a member variable of
		 * a class derived from SettingSet.
		 */
		class ItemHandle : ISettingItemListener {
		public:
			ItemHandle(SettingSet &set, const std::string &name, ItemFlags flags = ItemFlags::None);
			~ItemHandle();

			ItemHandle(const ItemHandle &) = delete;
			void operator=(const ItemHandle &) = delete;

			void Reload();

			void operator=(const std::string &);
			void operator=(int);
			void operator=(float);
			operator std::string();
			operator float();
			operator int();
			operator bool();
			const char *CString();

		private:
			void SettingChanged(const std::string &) override;

			SettingSet &set;
			Settings::ItemHandle handle;
			ItemFlags const flags;

			bool modified;
			std::string latchedStringValue;
			float latchedFloatValue;
			int latchedIntValue;
		};

		template <class T> class TypedItemHandle {
			ItemHandle handle;

		public:
			TypedItemHandle(SettingSet &set, const std::string &name,
			                ItemFlags flags = ItemFlags::None)
			    : handle(set, name, flags) {}
			void operator=(T value) { handle = value; }
			operator T() { return handle; }
			void Reload() { handle.Reload(); }
		};

		/**
		 * Updates every latched variable to the latest value.
		 */
		void ReloadAll();

	private:
		std::vector<std::reference_wrapper<ItemHandle>> handles;
		void Register(ItemHandle &);
	};

	inline SettingSet::ItemFlags operator|(SettingSet::ItemFlags lhs, SettingSet::ItemFlags rhs)

	{
		using T = std::underlying_type<SettingSet::ItemFlags>::type;
		return (SettingSet::ItemFlags)(static_cast<T>(lhs) | static_cast<T>(rhs));
	}

	inline SettingSet::ItemFlags &operator|=(SettingSet::ItemFlags &lhs,
	                                         SettingSet::ItemFlags rhs) {
		using T = std::underlying_type<SettingSet::ItemFlags>::type;
		lhs = (SettingSet::ItemFlags)(static_cast<T>(lhs) | static_cast<T>(rhs));
		return lhs;
	}

	inline SettingSet::ItemFlags operator&(SettingSet::ItemFlags lhs, SettingSet::ItemFlags rhs)

	{
		using T = std::underlying_type<SettingSet::ItemFlags>::type;
		return (SettingSet::ItemFlags)(static_cast<T>(lhs) & static_cast<T>(rhs));
	}
}
