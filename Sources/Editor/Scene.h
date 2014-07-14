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

#include <Core/RefCountedObject.h>
#include <Core/ModelTree.h>
#include <vector>
#include <set>

namespace spades { namespace editor {
	
	class SceneListener;
	
	struct RootFrame: public RefCountedObject {
		Handle<osobj::Frame> frame;
		std::string name;
		bool isForeignObject;
		Matrix4 matrix = Matrix4::Identity();
	protected:
		~RootFrame() { }
	};
	
	struct TimelineItem: public RefCountedObject {
		Handle<osobj::Timeline> timeline;
		std::string name;
	protected:
		~TimelineItem() { }
	};
	
	class Scene: public RefCountedObject {
		std::set<SceneListener *> listeners;
		std::vector<Handle<RootFrame>> rootFrames;
		std::vector<Handle<TimelineItem>> timelines;
	protected:
		~Scene();
	public:
		Scene();
		
		void AddListener(SceneListener *);
		void RemoveListener(SceneListener *);
		
		const decltype(rootFrames)& GetRootFrames() const
		{ return rootFrames; }
		void AddRootFrame(RootFrame *);
		void RemoveRootFrame(RootFrame *);
		
		const decltype(timelines)& GetTimelines() const
		{ return timelines; }
		void AddTimeline(TimelineItem *);
		void RemoveTimeline(TimelineItem *);
	};
	
	class SceneListener {
	public:
		virtual ~SceneListener() { }
		
		virtual void RootFrameAdded(RootFrame *) { }
		virtual void RootFrameRemoved(RootFrame *) { }
		
		virtual void TimelineAdded(TimelineItem *) { }
		virtual void TimelineRemoved(TimelineItem *) { }
	};
	
} }

