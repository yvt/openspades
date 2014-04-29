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

#include "GameMap.h"
#include <Core/DeflateStream.h>
#include <Core/IStream.h>
#include <memory>


namespace spades {
	namespace client {
	
		static const uint32_t signature = 0x1145148a;
		
		enum class ColumnFormat {
			Rle = 0,
			Bitmap = 1
		};;
		
		GameMap *GameMap::LoadNGMap(IStream *zstream,
									std::function<void(float)> progressListener) {
			DeflateStream stream(zstream, CompressModeDecompress);
			
			progressListener(0.f);
			
			auto sig = stream.ReadLittleInt();
			if(sig != signature) {
				SPRaise("Invalid signature: 0x%08x (expected 0x%08x)", sig, signature);
			}
			
			int w = stream.ReadLittleShort();
			int h = stream.ReadLittleShort();
			int d = stream.ReadLittleShort();
			if(w < 1 || h < 1 || d < 1 ||
			   w > 8192 || h > 8192 || d > 128) {
				SPRaise("Invalid map dimension: %dx%dx%d", w, h, d);
			}
			
			if(d > 64) {
				// due to bit width of solidMap
				SPRaise("Depth over 64 is not supported: %d", d);
			}
			
			Handle<GameMap> mp(new GameMap(w, h, d), false);
			
			// read geometry
			for(int y = 0; y < h; y++) {
				bool odd = y & 1;
				for(int i = 0, x = odd ? w - 1 : 0, s = odd ? -1 : 1; i < w; i++, x += s) {
					int fmt = stream.ReadByte();
					ColumnFormat fmtType = static_cast<ColumnFormat>(fmt & 1);
					uint64_t solidMap = 0;
					
					if(fmtType == ColumnFormat::Rle) {
						// RLE format
						int z = 0;
						bool fill = false;
						while(z < d) {
							auto inb = stream.ReadByte();
							if(inb & 0x80) {
								fill = true;
								inb &= 0x7f;
							}
							
							if(inb <= z || inb > d) {
								SPRaise("Data corrupted: invalid Z coordinate: %d", inb);
							}
							
							if(fill) {
								while(z < inb) {
									solidMap |= 1ULL << z;
									z++;
								}
							} else {
								z = inb;
							}
							
							fill = !fill;
						}
					} else if(fmtType == ColumnFormat::Bitmap) {
						for(int z = 0; z < d; z += 16) {
							if((fmt >> (1 + (z >> 4))) & 1) {
								auto b = stream.ReadLittleShort();
								solidMap |= static_cast<uint64_t>(b) << z;
							}
						}
					}
					
					mp->solidMap[x][y] = solidMap;
				}
				
				progressListener(0.5f * static_cast<float>(y) / static_cast<float>(h));
			}
			
			return mp.Unmanage();
		}
		GameMap *GameMap::LoadNGMap(IStream *stream) {
			return LoadNGMap(stream, [](float){});
		}
		
		void GameMap::SaveNGMap(IStream *zstream) {
			DeflateStream stream(zstream, CompressModeCompress);
			
			int w = Width();
			int h = Height();
			int d = Depth();
			
			stream.WriteLittleInt(signature);
			
			stream.WriteLittleShort(static_cast<uint16_t>(w));
			stream.WriteLittleShort(static_cast<uint16_t>(h));
			stream.WriteLittleShort(static_cast<uint16_t>(d));
			
			if(d > 64) {
				// due to bit width of solidMap
				SPRaise("Depth over 64 is not supported: %d", d);
			}
			
			// write geometry
			std::vector<uint8_t> columnbuf;
			for(int y = 0; y < h; y++) {
				bool odd = y & 1;
				for(int i = 0, x = odd ? w - 1 : 0, s = odd ? -1 : 1; i < w; i++, x += s) {
					
					// output column
					uint64_t solidMap = GetSolidMapWrapped(x, y);
					// first, try bitmap format
					{
						columnbuf.clear();
						int z = 1;
						bool last = (solidMap & 1) != 0;
						int step = 0;
						while(z < d) {
							bool b = ((solidMap >> z) & 1) != 0;
							if(b != last) {
								uint8_t outb = static_cast<uint8_t>(z);
								if(step == 0) {
									step = 1;
									if(last) outb |= 0x80;
								}
								columnbuf.push_back(outb);
								last = b;
							}
							z++;
						}
						
						{
							uint8_t outb = static_cast<uint8_t>(z);
							if(step == 0) {
								step = 1;
								if(last) outb |= 0x80;
							}
							columnbuf.push_back(outb);
						}
					}
					// second, estimate size of bitmap format
					std::size_t bmpfmtsize = 0;
					{
						for(int z = 0; z < d; z += 16) {
							if((solidMap >> z) & 0xffff) {
								bmpfmtsize+=2;
							}
						}
					}
					
					// compare and choose the smaller one
					if(bmpfmtsize < columnbuf.size() &&
					   bmpfmtsize != 0) {
						// bitmap format is smaller
						int fmt = static_cast<int>(ColumnFormat::Bitmap);
						for(int z = 0; z < d; z += 16) {
							if((solidMap >> z) & 0xffff) {
								fmt |= 1 << (1 + (z >> 4));
							}
						}
						stream.WriteByte(fmt);
						for(int z = 0; z < d; z += 16) {
							if((solidMap >> z) & 0xffff) {
								stream.WriteLittleShort(static_cast<uint16_t>(solidMap >> z));
							}
						}
					}else{
						// rle format is smaller
						stream.WriteByte(static_cast<int>(ColumnFormat::Rle));
						stream.Write(columnbuf.data(), columnbuf.size());
					}
					
					
					// one column done
				}
			}
			
			
			// TODO: colors
			
			stream.DeflateEnd();
			
		}
		
	}
}
