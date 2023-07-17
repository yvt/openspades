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
#include <memory>

#include "SWModel.h"
#include <Core/IStream.h>
#include <Core/VoxelModelLoader.h>

namespace spades {
	namespace draw {
		SWModel::SWModel(VoxelModel &m) : rawModel(m) {
			center.x = m.GetWidth();
			center.y = m.GetHeight();
			center.z = m.GetDepth();
			center *= 0.5f;
			radius = center.GetLength();

			int w = m.GetWidth();
			int h = m.GetHeight();
			int d = m.GetDepth();

			for (int y = 0; y < h; y++) {
				for (int x = 0; x < w; x++) {

					renderDataAddr.push_back(static_cast<uint32_t>(renderData.size()));

					uint64_t map = m.GetSolidBitsAt(x, y);
					uint64_t map1 = x > 0 ? m.GetSolidBitsAt(x - 1, y) : 0;
					uint64_t map2 = x < (w - 1) ? m.GetSolidBitsAt(x + 1, y) : 0;
					uint64_t map3 = y > 0 ? m.GetSolidBitsAt(x, y - 1) : 0;
					uint64_t map4 = y < (h - 1) ? m.GetSolidBitsAt(x, y + 1) : 0;
					map1 &= map2;
					map1 &= map3;
					map1 &= map4;

					for (int z = 0; z < d; z++) {
						if (!(map & (1ULL << z)))
							continue;
						if (z == 0 || z == (d - 1) || ((map >> (z - 1)) & 7ULL) != 7ULL ||
						    (map1 & (1ULL << z)) == 0) {
							uint32_t col = m.GetColor(x, y, z);

							uint32_t encodedColor;
							encodedColor =
							  (col & 0xff00) | ((col & 0xff) << 16) | ((col & 0xff0000) >> 16);
							encodedColor |= z << 24;
							renderData.push_back(encodedColor);

							auto material = static_cast<MaterialType>(col >> 24);

							// store normal
							uint32_t normal;

							if (material == MaterialType::Emissive) {
								normal = 27;
							} else {
								int nx = 0, ny = 0, nz = 0;
								for (int cx = -1; cx <= 1; cx++)
									for (int cy = -1; cy <= 1; cy++)
										for (int cz = -1; cz <= 1; cz++) {
											if (m.IsSolid(x + cx, y + cy, z + cz)) {
												nx -= cx;
												ny -= cy;
												nz -= cz;
											} else {
												nx += cx;
												ny += cy;
												nz += cz;
											}
										}
								nx = std::max(std::min(nx, 1), -1);
								ny = std::max(std::min(ny, 1), -1);
								nz = std::max(std::min(nz, 1), -1);
								nx++;
								ny++;
								nz++;
								normal = nx + ny * 3 + nz * 9;
							}

							renderData.push_back(normal);
						}
					}

					renderData.push_back(0xffffffffU);
				}
			}
		}

		SWModel::~SWModel() {}

		AABB3 SWModel::GetBoundingBox() {
			VoxelModel &m = *rawModel;
			Vector3 minPos = {0, 0, 0};
			Vector3 maxPos = {(float)m.GetWidth(), (float)m.GetHeight(), (float)m.GetDepth()};
			auto origin = rawModel->GetOrigin() - .5f;
			minPos += origin;
			maxPos += origin;
			Vector3 maxDiff = {std::max(fabsf(minPos.x), fabsf(maxPos.x)),
			                   std::max(fabsf(minPos.y), fabsf(maxPos.y)),
			                   std::max(fabsf(minPos.z), fabsf(maxPos.z))};
			radius = maxDiff.GetLength();

			AABB3 boundingBox;
			boundingBox.min = minPos;
			boundingBox.max = maxPos;
			return boundingBox;
		}

		SWModelManager::~SWModelManager() {}

		Handle<SWModel> SWModelManager::RegisterModel(const std::string &name) {
			auto it = models.find(name);
			if (it == models.end()) {
				auto vm = VoxelModelLoader::Load(name.c_str());

				Handle<SWModel> model = CreateModel(*vm);
				models.insert(std::make_pair(name, model));
				model->AddRef();

				return model;
			} else {
				return it->second;
			}
		}

		Handle<SWModel> SWModelManager::CreateModel(spades::VoxelModel &vm) {
			return Handle<SWModel>::New(vm);
		}

		void SWModelManager::ClearCache() { models.clear(); }
	} // namespace draw
} // namespace spades
