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
#include <Client/IRenderer.h>
#include <Client/IAudioDevice.h>
#include "Scene.h"
#include <Core/ModelTree.h>
#include <unordered_map>
#include <Core/VoxelModel.h>

namespace spades { namespace editor {
	
	class FrameRenderer;
	class ObjectRenderer;
	
	class SceneRenderer:
	public RefCountedObject,
	SceneListener {
		Handle<Scene> scene;
		Handle<client::IRenderer> renderer;
		std::unordered_map<RootFrame *, Handle<FrameRenderer>> frames;
		
		void RootFrameAdded(RootFrame *) override;
		void RootFrameRemoved(RootFrame *) override;
	protected:
		~SceneRenderer();
	public:
		SceneRenderer(Scene *, client::IRenderer *);
		void AddToScene(osobj::Pose *);
	};
	
	class FrameRenderer:
	public RefCountedObject,
	osobj::FrameListener {
		Handle<osobj::Frame> frame;
		Handle<client::IRenderer> renderer;
		
		std::unordered_map<osobj::Frame *, Handle<FrameRenderer>> children;
		std::unordered_map<osobj::Object *, Handle<ObjectRenderer>> objects;
		
		void ChildFrameAdded(osobj::Frame *, osobj::Frame *) override;
		void ChildFrameRemoved(osobj::Frame *, osobj::Frame *) override;
		void ObjectAdded(osobj::Frame *, osobj::Object *) override;
		void ObjectRemoved(osobj::Frame *, osobj::Object *) override;
	protected:
		~FrameRenderer();
	public:
		FrameRenderer(osobj::Frame *, client::IRenderer *);
		void AddToScene(const Matrix4&, osobj::Pose *,
						const Vector3& customColor);
	};
	
	class ObjectRenderer: public RefCountedObject {
	protected:
		~ObjectRenderer() { }
	public:
		virtual void AddToScene(const Matrix4&,
								const Vector3& customColor) = 0;
	};
	
	class VoxelModelObjectRenderer:
	public ObjectRenderer,
	VoxelModelListener {
		Handle<osobj::VoxelModelObject> obj;
		Handle<client::IRenderer> renderer;
		Handle<client::IModel> rendererModel;
		void VoxelModelUpdated(VoxelModel *) override;
	protected:
		~VoxelModelObjectRenderer();
	public:
		VoxelModelObjectRenderer(osobj::VoxelModelObject *, client::IRenderer *);
		void AddToScene(const Matrix4&,
						const Vector3& customColor) override;
	};
	
} }
