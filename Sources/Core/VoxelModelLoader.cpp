/*
 Copyright (c) 2019 yvt

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
#include "VoxelModelLoader.h"

#include <json/json.h>

#include <Core/Debug.h>
#include <Core/Exception.h>
#include <Core/FileManager.h>
#include <Core/IStream.h>
#include <Core/Math.h>
#include <Core/TMPUtils.h>
#include <Core/VoxelModel.h>

namespace spades {
	namespace {
		// Copied from `ngspades`, a cancelled branch of OpenSpades
		Vector3 ReadVector3(const Json::Value &json, const char *name) {
			if (json.isArray() && json.size() == 3) {
				auto e1 = json.get((Json::UInt)0, Json::nullValue);
				auto e2 = json.get((Json::UInt)1, Json::nullValue);
				auto e3 = json.get((Json::UInt)2, Json::nullValue);
				if (e1.isConvertibleTo(Json::ValueType::realValue) &&
				    e2.isConvertibleTo(Json::ValueType::realValue) &&
				    e3.isConvertibleTo(Json::ValueType::realValue)) {
					return Vector3(e1.asDouble(), e2.asDouble(), e3.asDouble());
				}
			}
			SPRaise("%s must be vector consisting of three real values", name);
		}

		struct Metadata {
			stmp::optional<Vector3> origin;
			stmp::optional<MaterialType> forceMaterial;

			void Parse(const Json::Value &root) {
				if (root.type() == Json::objectValue) {
					auto jsonOrigin = root.get("Origin", Json::nullValue);
					if (!jsonOrigin.isNull()) {
						origin = ReadVector3(jsonOrigin, "Origin");
					}

					auto jsonForceMaterial = root.get("ForceMaterial", Json::nullValue);
					if (!jsonForceMaterial.isNull()) {
						if (!jsonForceMaterial.isString()) {
							SPRaise("ForceMaterial must be a string");
						}

						auto str = jsonForceMaterial.asString();
						if (str == "Default") {
							forceMaterial = MaterialType::Default;
						} else if (str == "Emissive") {
							forceMaterial = MaterialType::Emissive;
						} else {
							SPRaise("ForceMaterial: Unrecognized value '%s'", str.c_str());
						}
					}
				}
			}
		};
	} // namespace

	Handle<VoxelModel> VoxelModelLoader::Load(const char *path) {
		// Load the metadata file
		std::string metadataPath = path;
		{
			auto i = metadataPath.rfind('.');
			if (i != std::string::npos) {
				metadataPath.resize(i);
			}
			metadataPath += ".meta.json";
		}

		// Load the metadata
		Metadata meta;
		if (FileManager::FileExists(metadataPath.c_str())) {
			SPLog("Found a metadata file '%s', loading it...", metadataPath.c_str());

			std::string metadataJson = FileManager::ReadAllBytes(metadataPath.c_str());

			Json::Reader reader;
			Json::Value root;

			if (reader.parse(metadataJson, root, false)) {
				meta.Parse(root);
			} else {
				SPRaise("The voxel model metadata file is not a valid JSON file.");
			}
		} else {
			SPLog("Ignoring a non-existend metadata file '%s'.", metadataPath.c_str());
		}

		// Load the base KV6 model
		Handle<VoxelModel> voxelModel;
		{
			SPLog("Loading '%s' as a KV6 voxel model.", path);
			std::unique_ptr<IStream> stream{FileManager::OpenForReading(path)};
			voxelModel = VoxelModel::LoadKV6(*stream);
		}

		// Apply transformation requested by the metadata
		if (meta.origin) {
			voxelModel->SetOrigin(*meta.origin);
		}

		if (meta.forceMaterial) {
			voxelModel->ForceMaterial(*meta.forceMaterial);
		}

		return voxelModel;
	}
} // namespace spades
