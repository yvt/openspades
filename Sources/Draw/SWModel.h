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

#include <unordered_map>
#include <vector>

#include <Client/IModel.h>
#include <Core/VoxelModel.h>

namespace spades {
	namespace draw {
		class SWModelRenderer;

		class SWModel : public client::IModel {
			friend class SWModelRenderer;

			Handle<VoxelModel> rawModel;
			float radius;
			Vector3 center;

			std::vector<uint32_t> renderData;
			std::vector<uint32_t> renderDataAddr;

		protected:
			~SWModel();

		public:
			SWModel(VoxelModel &model);

			float GetRadius() { return radius; }
			Vector3 GetCenter() { return center; }
			VoxelModel &GetRawModel() { return *rawModel; }

			AABB3 GetBoundingBox();
		};

		class SWModelManager {
			std::unordered_map<std::string, Handle<SWModel>> models;

		public:
			SWModelManager() {}
			~SWModelManager();

			Handle<SWModel> RegisterModel(const std::string &);
			Handle<SWModel> CreateModel(VoxelModel &);

			void ClearCache();
		};
	} // namespace draw
} // namespace spades
