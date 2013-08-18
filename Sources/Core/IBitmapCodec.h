//
//  IBitmapCodec.h
//  OpenSpades
//
//  Created by yvt on 7/22/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

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
		static bool EndsWith(const std::string& filename,
							 const std::string& extension);

		virtual std::string GetName() = 0;
		
		virtual bool CanLoad() = 0;
		virtual bool CanSave() = 0;
		
		/** @return true if this codec supports the extension of the given filename.  */
		virtual bool CheckExtension(const std::string&) = 0;
		
		virtual Bitmap *Load(IStream *) = 0;
		virtual void Save(IStream *, Bitmap *) = 0;
	};
	
	
}