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

#include "SettingSet.h"

namespace spades {
	SettingSet::ItemHandle::ItemHandle(SettingSet &set, const std::string &name, ItemFlags flags)
	    : set{set}, handle{name, nullptr}, flags{flags}, modified{true} {
		set.Register(*this);
		Reload();

		handle.AddListener(this);
	}

	SettingSet::ItemHandle::~ItemHandle() { handle.RemoveListener(this); }

	void SettingSet::ItemHandle::SettingChanged(const std::string &) {
		modified = true;

		if ((flags & ItemFlags::Latch) == ItemFlags::None) {
			Reload();
		}
	}

	void SettingSet::ItemHandle::Reload() {
		if (!modified) {
			return;
		}

		modified = false;
		latchedStringValue = handle.operator std::string();
		latchedFloatValue = handle;
		latchedIntValue = handle;
	}

	void SettingSet::ItemHandle::operator=(const std::string &value) { handle = value; }
	void SettingSet::ItemHandle::operator=(int value) { handle = value; }
	void SettingSet::ItemHandle::operator=(float value) { handle = value; }
	SettingSet::ItemHandle::operator std::string() { return latchedStringValue; }
	SettingSet::ItemHandle::operator int() { return latchedIntValue; }
	SettingSet::ItemHandle::operator float() { return latchedFloatValue; }
	SettingSet::ItemHandle::operator bool() { return latchedIntValue != 0; }
	const char *SettingSet::ItemHandle::CString() { return latchedStringValue.c_str(); }

	void SettingSet::ReloadAll() {
		for (ItemHandle &handle : handles) {
			handle.Reload();
		}
	}

	void SettingSet::Register(ItemHandle &handle) { handles.push_back(handle); }
}
