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

#include <string>
#include <vector>

namespace spades {
	class Bitmap;
	class IStream;
	class IBitmapCodec {
	public:
		IBitmapCodec();
		virtual ~IBitmapCodec();

		static std::vector<IBitmapCodec *> GetAllCodecs();
		static bool EndsWith(const std::string &filename, const std::string &extension);

		virtual std::string GetName() = 0;

		virtual bool CanLoad() = 0;
		virtual bool CanSave() = 0;

		/** @return true if this codec supports the extension of the given filename.  */
		virtual bool CheckExtension(const std::string &) = 0;

		virtual Bitmap *Load(IStream *) = 0;
		virtual void Save(IStream *, Bitmap *) = 0;
	};
}