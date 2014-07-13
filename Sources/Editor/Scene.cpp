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

#include "Scene.h"
#include <Core/Debug.h>

namespace spades { namespace editor {
	
	Scene::Scene() {
		
	}
	
	Scene::~Scene() {
		
	}
	
	void Scene::AddListener(SceneListener *l) {
		SPAssert(l);
		listeners.insert(l);
	}
	
	void Scene::RemoveListener(SceneListener *l) {
		listeners.erase(l);
	}
	
	void Scene::AddRootFrame(RootFrame *r) {
		SPAssert(r);
		RemoveRootFrame(r);
		rootFrames.push_back(r);
		for (auto *l: listeners)
			l->RootFrameAdded(r);
	}
	
	void Scene::RemoveRootFrame(RootFrame *r) {
		auto it = std::find(rootFrames.begin(), rootFrames.end(), r);
		if (it != rootFrames.end()) {
			auto h = std::move(*it);
			rootFrames.erase(it);
			for (auto *l: listeners)
				l->RootFrameRemoved(h);
		}
	}
	
	void Scene::AddTimeline(TimelineItem *r) {
		SPAssert(r);
		RemoveTimeline(r);
		timelines.push_back(r);
		for (auto *l: listeners)
			l->TimelineAdded(r);
	}
	
	void Scene::RemoveTimeline(TimelineItem *r) {
		auto it = std::find(timelines.begin(), timelines.end(), r);
		if (it != timelines.end()) {
			auto h = std::move(*it);
			timelines.erase(it);
			for (auto *l: listeners)
				l->TimelineRemoved(h);
		}
	}
	
} }
