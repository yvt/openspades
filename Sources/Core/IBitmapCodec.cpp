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

#include <cctype>

#include "IBitmapCodec.h"
#include "Debug.h"
#include "Exception.h"

namespace spades {

	static std::vector<IBitmapCodec *> *allCodecs = NULL;

	static void InitCodecList() {
		if (allCodecs)
			return;
		allCodecs = new std::vector<IBitmapCodec *>();
	}

	std::vector<IBitmapCodec *> IBitmapCodec::GetAllCodecs() { return *allCodecs; }

	bool IBitmapCodec::EndsWith(const std::string &filename, const std::string &extension) {
		SPADES_MARK_FUNCTION_DEBUG();

		if (filename.size() < extension.size())
			return false;
		for (size_t i = 0; i < extension.size(); i++) {
			int a = tolower(extension[i]);
			int b = tolower(filename[i + filename.size() - extension.size()]);
			if (a != b)
				return false;
		}
		return true;
	}

	IBitmapCodec::IBitmapCodec() {
		SPADES_MARK_FUNCTION();

		InitCodecList();
		allCodecs->push_back(this);
	}

	IBitmapCodec::~IBitmapCodec() {
		SPADES_MARK_FUNCTION();
		// FIXME: uninstall bitmap codec?
	}
}
