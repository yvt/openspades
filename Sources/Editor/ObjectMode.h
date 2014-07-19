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

#include "Editor.h"
#include <unordered_map>

namespace spades { namespace editor {

	class ObjectSelectionMode: public EditorMode {
	protected:
		~ObjectSelectionMode();
	public:
		ObjectSelectionMode();
		void Leave(Editor *) override;
		void Enter(Editor *) override;
		UIElement *CreateView(UIManager *, Editor *) override;
	};
	
	class FrameTransformAbstractMode: public EditorMode {
		Editor *editor = nullptr;
		bool done = false;
		std::vector<osobj::Frame *> targetFrames;
		std::vector<osobj::Frame *> affectedFrames;
		std::unordered_map<osobj::Frame *, Matrix4> originalTransform;
		void ForceCancel();
	protected:
		~FrameTransformAbstractMode();
		void ActiveObjectChanged(osobj::Object *);
		void ActiveTimelineChanged(TimelineItem *);
		void SelectedFramesChanged();
		
		const decltype(targetFrames)& GetTargetFrames() const
		{ return targetFrames; }
		Matrix4 GetOriginalTransform(osobj::Frame *);
		Matrix4 GetGlobalOriginalTransform(osobj::Frame *);
		virtual std::vector<osobj::Frame *> ComputeAffectedFrames()
		{ return targetFrames; }
	public:
		FrameTransformAbstractMode();
		void Leave(Editor *) override;
		void Enter(Editor *) override;
		
		void Apply();
		void Cancel();
		
		bool IsActive() const { return !done && editor; }
	};
	
	class TranslateFrameMode: public FrameTransformAbstractMode {
		class View;
	protected:
		~TranslateFrameMode();
	public:
		TranslateFrameMode();
		UIElement *CreateView(UIManager *, Editor *) override;
	};
	
} }
