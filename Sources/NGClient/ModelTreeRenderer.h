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

#include <Core/ModelTree.h>
#include <Client/IRenderer.h>
#include <unordered_map>

namespace spades { namespace ngclient {
	
	struct ModelTreeRenderParam {
		Vector3 customColor { 0, 0, 0 };
		bool depthHack = false;
	};
	
	class ObjectRenderer: public RefCountedObject {
	protected:
		~ObjectRenderer() { };
	public:
		ObjectRenderer() { }
		virtual void AddToScene(const Matrix4&,
								const ModelTreeRenderParam&) = 0;
	};
	
	class ModelTreeRenderer: public RefCountedObject {
		Handle<osobj::Frame> root;
		Handle<client::IRenderer> renderer;
		std::unordered_map<osobj::Object *, Handle<ObjectRenderer>>
		objectRenderers;
		void AddToScene(osobj::Pose&,
						const ModelTreeRenderParam&,
						osobj::Frame&,
						const Matrix4&);
		void Init(osobj::Frame&);
	protected:
		~ModelTreeRenderer();
	public:
		ModelTreeRenderer(client::IRenderer *,
						  osobj::Frame *);
		void AddToScene(osobj::Pose&,
						const ModelTreeRenderParam&);
	};
	
} }

