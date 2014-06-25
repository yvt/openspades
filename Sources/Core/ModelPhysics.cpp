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

#include "ModelPhysics.h"

namespace spades { namespace osobj {
	
	PhysicsObject::PhysicsObject(Frame *f,
								 dWorldID world,
								 dSpaceID space,
								 dBodyID parent):
	world(world), space(space),
	root(f) {
		SPAssert(f);
		GenerateItem(f, Matrix4::Identity(), parent);
	}
	
	void PhysicsObject::GenerateItem(Frame *f,
									 const Matrix4& mat,
									 dBodyID parent) {
		SPAssert(f);
		
		auto m = mat * f->GetTransform();
		auto scale = m.GetAxis(0).GetLength();
		
		const auto& objs = f->GetObjects();
		
		Item item;
		
		// setup body
		item.body.reset(new dBody(world));
		
		Vector3 center(0, 0, 0);
		std::array<Vector3, 3> inertia {
			Vector3(1, 0, 0),
			Vector3(0, 1, 0),
			Vector3(0, 0, 1),
		};
		float mass = .1f;
		
		if (!objs.empty()) {
			// TODO: support more than two objects
			const auto& obj = objs.front();
			center = obj->GetCenterOfMass();
			mass = obj->GetMass();
			inertia = obj->GetInertiaTensor();
		}
		
		mass *= scale * scale * scale;
		inertia[0] *= scale * scale;
		inertia[1] *= scale * scale;
		inertia[2] *= scale * scale;
		
		dMass dmass;
		dMassSetParameters(&dmass, mass,
						   0, 0, 0, // center of mass must be at origin (ODE's restriction)
						   inertia[0].x, inertia[1].y, inertia[2].z,
						   inertia[0].y, inertia[0].z, inertia[1].z);
		
		item.body->setMass(&dmass);
		
		item.frameCenterOfMass = center;
		item.originLocalPos = -center * scale;
		item.scale = scale;
		
		// setup geom
		if (!objs.empty()) {
			// TODO: support more than two objects
			const auto& obj = objs.front();
			auto bounds = obj->GetBounds();
			item.geom.reset(new dBox(space,
									 bounds.GetWidth(),
									 bounds.GetHeight(),
									 bounds.GetDepth()));
		}
		
		// transform body
		auto pos = (m * center).GetXYZ();
		item.body->setPosition(pos.x, pos.y, pos.z);
		
		auto q = Quaternion::FromRotationMatrix(m);
		item.body->setQuaternion(std::array<dReal, 4>{q.v.w, q.v.x, q.v.y, q.v.z}.data());
		
		// bind geom to body
		if (item.geom) {
			item.geom->setBody(item.body->id());
			
			// TODO: support more than two objects
			const auto& obj = objs.front();
			auto bounds = obj->GetBounds();
			dGeomSetOffsetPosition(item.geom->id(),
								   bounds.GetMinX() + bounds.GetWidth() * .5f - center.x,
								   bounds.GetMinY() + bounds.GetHeight() * .5f - center.y,
								   bounds.GetMinZ() + bounds.GetDepth() * .5f - center.z);
		}
		
		// add joints
		const auto& constraints = f->GetConstraints();
		
		for (const auto& c: constraints) {
			class ConstraintBuilder: ConstConstraintVisitor {
				PhysicsObject& p;
				Item& item;
				dBodyID target;
				const Matrix4& m;
				
				void visit(const BallConstratint& c) override {
					auto j = std::make_shared<dBallJoint>(p.world);
					auto origin = (m * c.origin).GetXYZ();
					j->setAnchor(origin.x, origin.y, origin.z);
					j->attach(item.body->id(), target);
					p.joints.emplace_back(std::move(j));
				}
				void visit(const HingeConstratint& c) override {
					auto j = std::make_shared<dHingeJoint>(p.world);
					auto origin = (m * c.origin).GetXYZ();
					j->setAnchor(origin.x, origin.y, origin.z);
					
					auto lAxis = c.axis;
					auto axis = (m * Vector4(lAxis.x, lAxis.y, lAxis.z, 0.f)).GetXYZ().Normalize();
					j->setAxis(axis.x, axis.y, axis.z);
					
					j->setParam(dParamLoStop, c.minAngle);
					j->setParam(dParamHiStop, c.maxAngle);
					
					j->attach(item.body->id(), target);
					p.joints.emplace_back(std::move(j));
				}
				void visit(const UniversalJointConstratint& c) override {
					auto j = std::make_shared<dUniversalJoint>(p.world);
					auto origin = (m * c.origin).GetXYZ();
					j->setAnchor(origin.x, origin.y, origin.z);
					
					auto lAxis1 = c.axis1;
					auto axis1 = (m * Vector4(lAxis1.x, lAxis1.y, lAxis1.z, 0.f)).GetXYZ().Normalize();
					j->setAxis1(axis1.x, axis1.y, axis1.z);
					
					auto lAxis2 = c.axis2;
					auto axis2 = (m * Vector4(lAxis2.x, lAxis2.y, lAxis2.z, 0.f)).GetXYZ().Normalize();
					j->setAxis2(axis2.x, axis2.y, axis2.z);
					
					j->setParam(dParamLoStop, c.minAngle1);
					j->setParam(dParamHiStop, c.maxAngle1);
					j->setParam(dParamLoStop1, c.minAngle2);
					j->setParam(dParamHiStop1, c.maxAngle2);
					
					j->attach(item.body->id(), target);
					p.joints.emplace_back(std::move(j));
				}
			public:
				ConstraintBuilder(PhysicsObject& p, const Constraint& c,
								  Item& item, dBodyID target,
								  const Matrix4& m):
				p(p), item(item), target(target), m(m) {
					c.Accept(*this);
				}
			};
			if (parent == 0) continue;
			ConstraintBuilder(*this, *c, item, parent, m);
		}
		
		items[f] = item;
		
		for (const auto& e: f->GetChildren()) {
			GenerateItem(e, m, item.body->id());
		}
	}
	
	PhysicsObject::~PhysicsObject() {
		
	}
	
	void PhysicsObject::CopyPose(Frame *frame, Pose& pose,
								 const Matrix4& m) {
		SPAssert(frame);
		
		auto t = pose.GetTransform(frame);
		
		const auto& item = items[frame];
		auto pos = m * item.frameCenterOfMass;
		item.body->setPosition(pos.x, pos.y, pos.z);
		
		auto q = Quaternion::FromRotationMatrix(t);
		item.body->setQuaternion(std::array<dReal, 4>{q.v.w, q.v.x, q.v.y, q.v.z}.data());
		
		auto tt = m * t;
		
		for (const auto& e: frame->GetChildren()) {
			CopyPose(e, pose, tt);
		}
	}
	
	void PhysicsObject::CopyPose(Pose& pose) {
		CopyPose(root, pose, Matrix4::Identity());
	}
	
	void PhysicsObject::UpdatePose(Frame *frame, Pose& pose,
								   const Matrix4& m) {
		SPAssert(frame);
		
		const auto& item = items[frame];
		
		const auto *pos = item.body->getPosition();
		const auto *rot = item.body->getQuaternion();
		
		auto mat = Matrix4::Translate(pos[0], pos[1], pos[2]);
		mat *= Matrix4::Scale(item.scale);
		mat *= Quaternion(rot[1], rot[2], rot[3], rot[0]).ToRotationMatrix();
		mat *= Matrix4::Translate(item.originLocalPos);
				
		auto mInv = m.InversedFast();
		pose.SetTransform(frame, mInv * mat);
		
		auto nextMat = mat;
		
		for (const auto& e: frame->GetChildren()) {
			UpdatePose(e, pose, nextMat);
		}
	}
	
	void PhysicsObject::UpdatePose(Pose& pose) {
		UpdatePose(root, pose, Matrix4::Identity());
	}
	
	std::shared_ptr<dBody> PhysicsObject::GetBody(Frame *f) {
		auto it = items.find(f);
		if (it != items.end()) return it->second.body;
		return nullptr;
	}
	std::shared_ptr<dGeom> PhysicsObject::GetGeom(Frame *f) {
		auto it = items.find(f);
		if (it != items.end()) return it->second.geom;
		return nullptr;
	}
	
	std::list<std::shared_ptr<dBody>> PhysicsObject::GetAllBodies() {
		std::list<std::shared_ptr<dBody>> lst;
		for (const auto& pair: items) {
			if (pair.second.body) lst.emplace_back(pair.second.body);
		}
		return lst;
	}
	std::list<std::shared_ptr<dGeom>> PhysicsObject::GetAllGeoms() {
		std::list<std::shared_ptr<dGeom>> lst;
		for (const auto& pair: items) {
			if (pair.second.geom) lst.emplace_back(pair.second.geom);
		}
		return lst;
	}
	
} }

