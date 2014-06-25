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

#include <ode/ode.h>
#include <ode/odecpp.h>
#include <ode/odecpp_collision.h>
#include "ModelTree.h"
#include <unordered_map>
#include <memory>

namespace spades { namespace osobj {
	
	class PhysicsObject: public RefCountedObject {
		Handle<Frame> root;
		dWorldID world;
		dSpaceID space;
		
		struct Item {
			/** local position in the body that corresponds to
			 * the origin of the original frame */
			Vector3 originLocalPos;
			Vector3 frameCenterOfMass;
			float scale;
			std::shared_ptr<dBody> body;
			std::shared_ptr<dGeom> geom;
		};
		
		std::unordered_map<Handle<Frame>, Item> items;
		
		std::list<std::shared_ptr<dJoint>> joints;
		
		void GenerateItem(Frame *,
						  const Matrix4&,
						  dBodyID parent);
		void CopyPose(Frame *, Pose&, const Matrix4&);
		void UpdatePose(Frame *, Pose&, const Matrix4&);
		
	protected:
		~PhysicsObject();
	public:
		PhysicsObject(Frame *,
					  dWorldID world,
					  dSpaceID space = 0,
					  dBodyID parent = 0);
		void CopyPose(Pose&);
		void UpdatePose(Pose&);
		
		std::shared_ptr<dBody> GetBody(Frame *);
		std::shared_ptr<dGeom> GetGeom(Frame *);
		
		std::list<std::shared_ptr<dBody>> GetAllBodies();
		std::list<std::shared_ptr<dGeom>> GetAllGeoms();
	};
	
} }
