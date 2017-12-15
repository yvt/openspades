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
#include <Core/FileManager.h>
#include <Core/IStream.h>

namespace spades {
	namespace draw {
		SWModel::SWModel(VoxelModel *m) : rawModel(m) {
			center.x = m->GetWidth();
			center.y = m->GetHeight();
			center.z = m->GetDepth();
			center *= 0.5f;
			radius = center.GetLength();

			int w = m->GetWidth();
			int h = m->GetHeight();
			int d = m->GetDepth();

			for (int y = 0; y < h; y++) {
				for (int x = 0; x < w; x++) {

					renderDataAddr.push_back(static_cast<uint32_t>(renderData.size()));

					uint64_t map = m->GetSolidBitsAt(x, y);
					uint64_t map1 = x > 0 ? m->GetSolidBitsAt(x - 1, y) : 0;
					uint64_t map2 = x < (w - 1) ? m->GetSolidBitsAt(x + 1, y) : 0;
					uint64_t map3 = y > 0 ? m->GetSolidBitsAt(x, y - 1) : 0;
					uint64_t map4 = y < (h - 1) ? m->GetSolidBitsAt(x, y + 1) : 0;
					map1 &= map2;
					map1 &= map3;
					map1 &= map4;

					for (int z = 0; z < d; z++) {
						if (!(map & (1ULL << z)))
							continue;
						if (z == 0 || z == (d - 1) || ((map >> (z - 1)) & 7ULL) != 7ULL ||
						    (map1 & (1ULL << z)) == 0) {
							uint32_t col = m->GetColor(x, y, z);
							SPAssert(col != 0xddbeef);
							col = (col & 0xff00) | ((col & 0xff) << 16) | ((col & 0xff0000) >> 16);
							col |= z << 24;
							renderData.push_back(col);

							// store normal
							int nx = 0, ny = 0, nz = 0;
							for (int cx = -1; cx <= 1; cx++)
								for (int cy = -1; cy <= 1; cy++)
									for (int cz = -1; cz <= 1; cz++) {
										if (m->IsSolid(x + cx, y + cy, z + cz)) {
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
							renderData.push_back(nx + ny * 3 + nz * 9);
						}
					}

					renderData.push_back(0xffffffffU);
				}
			}
		}

		SWModel::~SWModel() {}

		AABB3 SWModel::GetBoundingBox() {
			VoxelModel *m = rawModel;
			Vector3 minPos = {0, 0, 0};
			Vector3 maxPos = {(float)m->GetWidth(), (float)m->GetHeight(), (float)m->GetDepth()};
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

		SWModelManager::~SWModelManager() {
			for (auto it = models.begin(); it != models.end(); it++)
				it->second->Release();
		}

		SWModel *SWModelManager::RegisterModel(const std::string &name) {
			auto it = models.find(name);
			if (it == models.end()) {
				std::unique_ptr<IStream> stream{FileManager::OpenForReading(name.c_str())};

				Handle<VoxelModel> vm;
				vm.Set(VoxelModel::LoadKV6(stream.get()), false);

				SWModel *model = CreateModel(vm);
				models.insert(std::make_pair(name, model));
				model->AddRef();

				return model;
			} else {
				SWModel *model = it->second;
				model->AddRef();
				return model;
			}
		}

		SWModel *SWModelManager::CreateModel(spades::VoxelModel *vm) { return new SWModel(vm); }
	}
}
