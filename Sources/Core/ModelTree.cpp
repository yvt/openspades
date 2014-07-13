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


#include "ModelTree.h"
#include <Core/Debug.h>
#include <algorithm>
#include "VoxelModel.h"
#include <Core/TMPUtils.h>
#include <Core/FileManager.h>
#include <json/json.h>
#include <Core/Debug.h>
#include <memory>
#include <Core/IStream.h>
#include <Core/TMPUtils.h>
#include "YSpline.h"

namespace spades { namespace osobj {
	
#pragma mark - Object
	
	Object::Object() { }
	Object::~Object() { }
	
	namespace {
		
		Vector3 ComputeCenterOfMass(VoxelModel& model, float& mass) {
			SPADES_MARK_FUNCTION();
			
			uint64_t xx = 0, yy = 0, zz = 0;
			unsigned int numSolids = 0;
			// FIXME: this can be made faster...
			for (int x = 0; x < model.GetWidth(); ++x)
				for (int y = 0; y < model.GetHeight(); ++y) {
					auto mp = model.GetSolidBitsAt(x, y);
					for (int z = 0; z < model.GetDepth(); ++z) {
						if (mp & (1ULL << z)) {
							xx += x; yy += y; zz += z;
							++numSolids;
						}
					}
				}
			mass = numSolids;
			return Vector3(xx, yy, zz) / mass;
		}
		
		std::array<Vector3, 3> ComputeInertiaTensor(VoxelModel& model,
												const Vector3& center) {
			SPADES_MARK_FUNCTION();
			double xx = 0, yy = 0, zz = 0;
			double xy = 0, xz = 0, yz = 0;
			// FIXME: this should be more precise...
			
			for (int x = 0; x < model.GetWidth(); ++x) {
				double vx = x - center.x;
				for (int y = 0; y < model.GetHeight(); ++y) {
					auto mp = model.GetSolidBitsAt(x, y);
					double vy = y - center.y;
					double vz = -center.z;
					for (int z = 0; z < model.GetDepth(); ++z) {
						if (mp & (1ULL << z)) {
							/*
							 fns = {y^2 + z^2, x^2 + z^2, x^2 + y^2, x*y, z*x, y*z}
							 fnOut = Integrate[fns, {x, X - 1/2, X + 1/2}] //
							 
							 Integrate[#, {y, Y - 1/2, Y + 1/2}] & //
							 
							 Integrate[#, {z, Z - 1/2, Z + 1/2}] &
							 */
							xx += 1.0 / 6.0 + vy * vy + vz * vz;
							yy += 1.0 / 6.0 + vx * vx + vz * vz;
							zz += 1.0 / 6.0 + vx * vx + vy * vy;
							xy += vx * vy;
							xz += vx * vz;
							yz += vy * vz;
						}
						vz += 1.f;
					}
				}
			}
			
			return std::array<Vector3, 3> {
				Vector3(xx, -xy, -xz),
				Vector3(-xy, yy, -yz),
				Vector3(-xz, -yz, zz)
			};
		}
		
	}
	
	VoxelModelObject::VoxelModelObject(VoxelModel *m):
	model(m) {
		SPADES_MARK_FUNCTION();
		
		SPAssert(m);
		mass = 0.f;
		
		auto center = ComputeCenterOfMass(*m, mass);
		centerOfMass = center + m->GetOrigin();
		inertiaTensor = ComputeInertiaTensor(*m, center);
	}
	
	VoxelModelObject::~VoxelModelObject() { }
	
	AABB3 VoxelModelObject::GetBounds() {
		SPADES_MARK_FUNCTION_DEBUG();
		
		auto orig = model->GetOrigin() - 0.5f;
		Vector3 sz(model->GetWidth(), model->GetHeight(),
				   model->GetDepth());
		return AABB3(orig, orig + sz);
	}
	
	float VoxelModelObject::GetMass() {
		return mass;
	}
	
	Vector3 VoxelModelObject::GetCenterOfMass() {
		return centerOfMass;
	}
	
	std::array<Vector3, 3> VoxelModelObject::GetInertiaTensor() {
		return inertiaTensor;
	}
	
#pragma mark - Constraints
	
	Constraint::Constraint() { }
	Constraint::~Constraint() { }
	
	HingeConstratint::HingeConstratint():
	minAngle(-M_PI),
	maxAngle(M_PI) { }
	
	UniversalJointConstratint::UniversalJointConstratint():
	minAngle1(-M_PI),
	maxAngle1(M_PI),
	minAngle2(-M_PI),
	maxAngle2(M_PI) { }
	
#pragma mark - Frame
	
	Frame::Frame():
	mat(Matrix4::Identity()) { }
	
	Frame::~Frame() {
		SPADES_MARK_FUNCTION();
		
		for (auto c: children) {
			c->parent = nullptr;
		}
		SPAssert(!parent);
	}
	
	void Frame::AddListener(FrameListener *l) {
		SPAssert(l);
		listeners.insert(l);
	}
	
	void Frame::RemoveListener(FrameListener *l) {
		listeners.erase(l);
	}
	
	void Frame::AddChild(Frame *f) {
		SPADES_MARK_FUNCTION();
		
		Handle<Frame> ref(f);
		f->RemoveFromParent();
		children.push_back(f);
		f->parent = this;
		
		for (auto *l: listeners) l->ChildFrameAdded(this, f);
	}
	
	void Frame::RemoveFromParent() {
		SPADES_MARK_FUNCTION();
		
		if (!parent) return;
		auto& ch = parent->children;
		auto *p = parent;
		parent = nullptr;
		auto it = std::find(ch.begin(), ch.end(), this);
		SPAssert(it != ch.end());
		ch.erase(it);
		
		for (auto *l: p->listeners) l->ChildFrameRemoved(p, this);
	}
	
	void Frame::AddObject(Object *obj) {
		SPADES_MARK_FUNCTION();
		
		SPAssert(obj);
		objects.push_back(obj);
		
		for (auto *l: listeners) l->ObjectAdded(this, obj);
	}
	
	void Frame::RemoveObject(Object *obj) {
		SPADES_MARK_FUNCTION();
		
		if (!obj) return;
		auto it = std::find(objects.begin(), objects.end(), obj);
		if (it != objects.end()) {
			auto h = std::move(*it);
			objects.erase(it);
			for (auto *l: listeners) l->ObjectRemoved(this, obj);
		}
		
	}
	
	void Frame::AddConstraint(Constraint *obj) {
		SPADES_MARK_FUNCTION();
		
		SPAssert(obj);
		constraints.push_back(obj);
		
		for (auto *l: listeners) l->ConstraintAdded(this, obj);
	}
	
	void Frame::RemoveConstraint(Constraint *obj) {
		SPADES_MARK_FUNCTION();
		
		if (!obj) return;
		auto it = std::find(constraints.begin(), constraints.end(), obj);
		if (it != constraints.end()) {
			auto h = std::move(*it);
			constraints.erase(it);
			for (auto *l: listeners) l->ConstraintRemoved(this, obj);
		}
	}
	
	void Frame::AddTag(const std::string & tag) {
		tags.insert(tag);
	}
	
	void Frame::RemoveTag(const std::string &tag) {
		tags.erase(tag);
	}
	
	bool Frame::HasTag(const std::string &tag) {
		return tags.find(tag) != tags.end();
	}
	
	std::string Frame::GetFullPath() {
		auto s = parent ? parent->GetFullPath() : "";
		if (id.empty()) {
			s += "/(null)";
		} else {
			s += "/" + id;
		}
		return s;
	}
	
	std::vector<Frame *>& Frame::GetFramesByTag
	(const std::string& tag,
	 std::vector<Frame *>& result) {
		
		// itself?
		if (HasTag(tag)) {
			result.push_back(this);
		}
		
		// children?
		for (auto& child: children) {
			child->GetFramesByTag(tag, result);
		}
		
		return result;
	}
	
	std::vector<Frame *> Frame::GetFramesByTag(const std::string &tag) {
		std::vector<Frame *> results;
		GetFramesByTag(tag, results);
		return results;
	}
	
	Frame *Frame::GetFrameById(const std::string& id) {
		
		// itself?
		if (this->id == id) {
			return this;
		}
		
		// children?
		for (auto& child: children) {
			auto *f = child->GetFrameById(id);
			if (f) {
				return f;
			}
		}
		
		return nullptr;
	}
	
	Vector4 Frame::LocalToGlobal(const spades::Vector4 &v,
								 Pose *pose) {
		auto vv = (pose ? pose->GetTransform(this) : GetTransform()) * v;
		if (parent) {
			return parent->LocalToGlobal(vv, pose);
		} else {
			return vv;
		}
	}
	
	
#pragma mark - Pose
	
	Pose::Pose(Frame *frame):
	root(frame) {
		SPADES_MARK_FUNCTION();
		
		Init(root);
	}
	
	Pose::~Pose() {
		
	}
	
	void Pose::SetInitialState() {
		SPADES_MARK_FUNCTION();
		
		Init(root);
	}
	
	void Pose::Init(Frame *f) {
		SPADES_MARK_FUNCTION();
		
		auto e = elements.emplace(f, Element());
		e.first->second.SetTransform(f->GetTransform());
		for (auto& c: f->GetChildren()) {
			Init(c);
		}
	}
	
	Pose::Element::Element()
	{ }
	
	void Pose::Element::SetTransform(const Matrix4 &m) {
		SPADES_MARK_FUNCTION_DEBUG();
		
		transform = m;
		position = m.GetOrigin();
		rotation = Quaternion::FromRotationMatrix(m);
		scale = m.GetAxis(0).GetLength();
		needsUpdate = false;
	}
	
	void Pose::Element::SetPosition(const Vector3& p) {
		SPADES_MARK_FUNCTION_DEBUG();
		position = p;
		needsUpdate = true;
	}
	
	void Pose::Element::SetRotation(const Quaternion& p) {
		SPADES_MARK_FUNCTION_DEBUG();
		
		rotation = p;
		needsUpdate = true;
	}
	
	void Pose::Element::SetScale(float p) {
		SPADES_MARK_FUNCTION_DEBUG();
		
		scale = p;
		needsUpdate = true;
	}
	
	void Pose::Element::UpdateTransformIfNeeded() {
		SPADES_MARK_FUNCTION_DEBUG();
		
		if (!needsUpdate) return;
		
		transform = rotation.ToRotationMatrix() * Matrix4::Scale(scale);
		transform = Matrix4::Translate(position) * transform;
		
		needsUpdate = false;
	}
	
	Matrix4 Pose::GetTransform(Frame *f) {
		SPADES_MARK_FUNCTION();
		
		auto it = elements.find(f);
		if (it == elements.end()) {
			SPRaise("Specified frame is not a part of this pose.");
		}
		it->second.UpdateTransformIfNeeded();
		return it->second.transform;
	}
	
	void Pose::SetTransform(Frame *f, const spades::Matrix4 &m) {
		SPADES_MARK_FUNCTION();
		
		auto it = elements.find(f);
		if (it == elements.end()) {
			SPRaise("Specified frame is not a part of this pose.");
		}
		it->second.SetTransform(m);
	}
	
	void Pose::SetPosition(Frame *f, const Vector3 &m) {
		SPADES_MARK_FUNCTION();
		
		auto it = elements.find(f);
		if (it == elements.end()) {
			SPRaise("Specified frame is not a part of this pose.");
		}
		it->second.SetPosition(m);
	}
	
	void Pose::SetRotation(Frame *f, const Quaternion &m) {
		SPADES_MARK_FUNCTION();
		
		auto it = elements.find(f);
		if (it == elements.end()) {
			SPRaise("Specified frame is not a part of this pose.");
		}
		it->second.SetRotation(m);
	}
	
	void Pose::SetScale(Frame *f, float m) {
		SPADES_MARK_FUNCTION();
		
		auto it = elements.find(f);
		if (it == elements.end()) {
			SPRaise("Specified frame is not a part of this pose.");
		}
		it->second.SetScale(m);
	}
	
	Vector3 Pose::GetPosition(Frame *f) {
		SPADES_MARK_FUNCTION();
		
		auto it = elements.find(f);
		if (it == elements.end()) {
			SPRaise("Specified frame is not a part of this pose.");
		}
		return it->second.position;
	}
	
	Quaternion Pose::GetRotation(Frame *f) {
		SPADES_MARK_FUNCTION();
		
		auto it = elements.find(f);
		if (it == elements.end()) {
			SPRaise("Specified frame is not a part of this pose.");
		}
		return it->second.rotation;
	}
	
	float Pose::GetScale(Frame *f) {
		SPADES_MARK_FUNCTION();
		
		auto it = elements.find(f);
		if (it == elements.end()) {
			SPRaise("Specified frame is not a part of this pose.");
		}
		return it->second.scale;
	}
	
	void Pose::Lerp(Pose& a, Pose& b, float mix) {
		SPADES_MARK_FUNCTION();
		
		if ((Frame*)root != a.root) {
			SPRaise("Root frames of poses must match");
		} else if ((Frame*)root != b.root) {
			SPRaise("Root frames of poses must match");
		}
		
		for (auto& e: elements) {
			auto it1 = a.elements.find(e.first);
			auto it2 = b.elements.find(e.first);
			if (it1 == a.elements.end()) {
				SPRaise("Pose A doesn't have the frame '%s'",
						e.first->GetFullPath().c_str());
			}
			if (it2 == b.elements.end()) {
				SPRaise("Pose B doesn't have the frame '%s'",
						e.first->GetFullPath().c_str());
			}
			
			const auto& e1 = it1->second;
			const auto& e2 = it2->second;
			
			e.second.position = Mix(e1.position, e2.position, mix);
			e.second.rotation = Mix(e1.rotation, e2.rotation, mix);
			e.second.scale = Mix(e1.scale, e2.scale, mix);
			e.second.needsUpdate = true;
		}
	}
	
#pragma mark - Timeline
	
	AnimationChannel::AnimationChannel(Frame *frame):
	frame(frame, true) {
		SPADES_MARK_FUNCTION();
		
		SPAssert(frame);
	}
	AnimationChannel::~AnimationChannel() { }
	
	struct LinearAnimationChannel::Curve {
		ysp::spline_curve<float, float> curve;
		
		Curve(LinearAnimationChannel& c):
		curve(c.keyFrames.begin(), c.keyFrames.end(), false){ }
	};
	
	LinearAnimationChannel::LinearAnimationChannel(Frame *frame):
	AnimationChannel(frame) { }
	
	LinearAnimationChannel::~LinearAnimationChannel() { }
	
	void LinearAnimationChannel::AddKeyFrame(float time, float value) {
		SPADES_MARK_FUNCTION();
		
		keyFrames.emplace(time, value);
	}
	
	void LinearAnimationChannel::RemoveKeyFrame(float time) {
		SPADES_MARK_FUNCTION();
		
		keyFrames.erase(time);
		curve.reset();
	}
	
	std::vector<std::pair<float, float>> LinearAnimationChannel::GetKeyFrames() {
		SPADES_MARK_FUNCTION();
		
		std::vector<std::pair<float, float>> ret;
		std::copy(keyFrames.begin(), keyFrames.end(),
				  std::back_inserter(ret));
		return ret;
	}
	
	float LinearAnimationChannel::Evaluate(float time) {
		SPADES_MARK_FUNCTION();
		
		if (!curve) {
			curve.reset(new Curve(*this));
		}
		
		return curve->curve(time);
	}
	
	struct RotationalAnimationChannel::Curve {
		ysp::quaternion_spline_curve<float> curve;
		
		Curve(std::vector<std::pair<float, ysp::quaternion<float>>>& c):
		curve(c.begin(), c.end(), false){ }
	};
	
	RotationalAnimationChannel::RotationalAnimationChannel(Frame *frame):
	AnimationChannel(frame) { }
	
	RotationalAnimationChannel::~RotationalAnimationChannel() { }
	
	void RotationalAnimationChannel::AddKeyFrame(float time, const Quaternion& value) {
		SPADES_MARK_FUNCTION();
		
		keyFrames.emplace(time, value);
	}
	
	void RotationalAnimationChannel::RemoveKeyFrame(float time) {
		SPADES_MARK_FUNCTION();
		
		keyFrames.erase(time);
		curve.reset();
	}
	
	std::vector<std::pair<float, Quaternion>> RotationalAnimationChannel::GetKeyFrames() {
		SPADES_MARK_FUNCTION();
		
		std::vector<std::pair<float, Quaternion>> ret;
		std::copy(keyFrames.begin(), keyFrames.end(),
				  std::back_inserter(ret));
		return ret;
	}
	
	Quaternion RotationalAnimationChannel::Evaluate(float time) {
		SPADES_MARK_FUNCTION();
		
		if (!curve) {
			std::vector<std::pair<float, ysp::quaternion<float>>> lst;
			lst.reserve(keyFrames.size());
			for (const auto& pair: keyFrames) {
				const auto& q = pair.second;
				lst.emplace_back(pair.first,
								 ysp::quaternion<float>(q.v.w, q.v.x, q.v.y, q.v.z));
			}
			
			curve.reset(new Curve(lst));
		}
		
		auto r = curve->curve(time);
		return Quaternion(r.R_component_2(), r.R_component_3(), r.R_component_4(), r.real());
	}
	
	PositionAnimationChannel::PositionAnimationChannel(Frame *frame, Axis axis):
	LinearAnimationChannel(frame), axis(axis) { }
	
	PositionAnimationChannel::~PositionAnimationChannel() { }
	
	void PositionAnimationChannel::Move(float time, Pose &pose) {
		SPADES_MARK_FUNCTION();
		
		auto p = pose.GetPosition(GetFrame());
		switch (axis) {
			case Axis::X: p.x = Evaluate(time); break;
			case Axis::Y: p.y = Evaluate(time); break;
			case Axis::Z: p.z = Evaluate(time); break;
		}
		pose.SetPosition(GetFrame(), p);
	}
	
	ScaleAnimationChannel::ScaleAnimationChannel(Frame *frame):
	LinearAnimationChannel(frame) { }
	
	ScaleAnimationChannel::~ScaleAnimationChannel() { }
	
	void ScaleAnimationChannel::Move(float time, Pose &pose) {
		SPADES_MARK_FUNCTION();
		
		auto val = Evaluate(time);
		pose.SetScale(GetFrame(), val);
	}
	
	
	RotationAnimationChannel::RotationAnimationChannel(Frame *frame):
	RotationalAnimationChannel(frame) { }
	
	RotationAnimationChannel::~RotationAnimationChannel() { }
	
	void RotationAnimationChannel::Move(float time, Pose &pose) {
		SPADES_MARK_FUNCTION();
		
		auto val = Evaluate(time);
		pose.SetRotation(GetFrame(), val);
	}
	
	
	Timeline::Timeline() {
		
	}
	
	Timeline::~Timeline() { }
	
	void Timeline::AddChannel(AnimationChannel *c) {
		if (std::find(channels.begin(),
					  channels.end(), c) == channels.end()) {
			channels.emplace_back(c, true);
		}
	}
	
	void Timeline::RemoveChannel(AnimationChannel *c) {
		auto it = std::find(channels.begin(),
							channels.end(), c);
		if (it != channels.end())
			channels.erase(it);
	}
	
	void Timeline::Move(float time, Pose& pose) {
		for (const auto& channel: channels) {
			channel->Move(time, pose);
		}
	}
	
	
#pragma mark - Loader
	
	Loader::Loader(const std::string& rootPath):
	rootPath(rootPath) { }
	
	Loader::~Loader() { }
	
	std::string Loader::ResolvePath(const std::string &p) {
		return FileManager::ResolvePath(p, rootPath, true);
	}
	
	
	class Loader::RootLoader {
		friend class FrameLoader;
		Loader& loader;
		Json::Value& root;
		std::string const basePath;
		std::unordered_map<std::string, Handle<Object>> objects;
	public:
		RootLoader(Loader& loader, Json::Value& root,
				   const std::string& basePath):
		loader(loader), root(root), basePath(basePath) { }
		Frame *Load();
	};
	
	class Loader::FrameLoader {
		Loader& loader;
		RootLoader& rootLoader;
		const Json::Value& json;
		std::string fullPath;
		
		Vector2 ReadVector2(const Json::Value& json,
							const char *name) {
			if (json.isArray() &&
				json.size() == 2) {
				auto e1 = json.get((Json::UInt)0, Json::nullValue);
				auto e2 = json.get((Json::UInt)1, Json::nullValue);
				if (e1.isConvertibleTo(Json::ValueType::realValue) &&
					e2.isConvertibleTo(Json::ValueType::realValue)) {
					return Vector2(e1.asDouble(),
								   e2.asDouble());
				}
			}
			SPRaise("%s: %s must be vector consisting of two real values",
					fullPath.c_str(), name);
		}
		
		Vector3 ReadVector3(const Json::Value& json,
							const char *name) {
			if (json.isArray() &&
				json.size() == 3) {
				auto e1 = json.get((Json::UInt)0, Json::nullValue);
				auto e2 = json.get((Json::UInt)1, Json::nullValue);
				auto e3 = json.get((Json::UInt)2, Json::nullValue);
				if (e1.isConvertibleTo(Json::ValueType::realValue) &&
					e2.isConvertibleTo(Json::ValueType::realValue) &&
					e3.isConvertibleTo(Json::ValueType::realValue)) {
					return Vector3(e1.asDouble(),
								   e2.asDouble(),
								   e3.asDouble());
				}
			}
			SPRaise("%s: %s must be vector consisting of three real values",
					fullPath.c_str(), name);
		}
		
		stmp::optional<Vector3> ReadVector3OrNull
		(const Json::Value& json, const char *name) {
			if (json.isNull()) return stmp::optional<Vector3>();
			return ReadVector3(json, name);
		}
		
		Matrix4 ReadRotation(const Json::Value& json) {
			if (json.isArray() && json.size() == 4) {
				// quaternion.
				auto e1 = json.get((Json::UInt)0, Json::nullValue);
				auto e2 = json.get((Json::UInt)1, Json::nullValue);
				auto e3 = json.get((Json::UInt)2, Json::nullValue);
				auto e4 = json.get((Json::UInt)3, Json::nullValue);
				if (e1.isConvertibleTo(Json::ValueType::realValue) &&
					e2.isConvertibleTo(Json::ValueType::realValue) &&
					e3.isConvertibleTo(Json::ValueType::realValue) &&
					e4.isConvertibleTo(Json::ValueType::realValue)) {
					Quaternion q(e2.asDouble(),
								 e3.asDouble(),
								 e4.asDouble(),
								 e1.asDouble());
					return q.ToRotationMatrix();
				}
			} else if (json.isArray() && json.size() == 3) {
				// rotation axis vector.
				auto e1 = json.get((Json::UInt)0, Json::nullValue);
				auto e2 = json.get((Json::UInt)1, Json::nullValue);
				auto e3 = json.get((Json::UInt)2, Json::nullValue);
				if (e1.isConvertibleTo(Json::ValueType::realValue) &&
					e2.isConvertibleTo(Json::ValueType::realValue) &&
					e3.isConvertibleTo(Json::ValueType::realValue)) {
					auto q = Quaternion::MakeRotation
					(Vector3(e1.asDouble(),
							 e2.asDouble(),
							 e3.asDouble()));
					return q.ToRotationMatrix();
				}
			}
			SPRaise("%s: rotation must be an array with three real values "
					"or one with four real values.",
					fullPath.c_str());
		}
		
		Constraint *ReadConstraint(const Json::Value& json) {
			auto vType = json.get("type", Json::nullValue);
			if (!vType.isConvertibleTo(Json::ValueType::stringValue)) {
				SPRaise("%s: type of constraint should be able to be converted to"
						" a string", fullPath.c_str());
			}
			auto type = vType.asString();
			if (type == "ball") {
				auto origin = ReadVector3(json.get("origin", Json::nullValue),
										  "origin");
				Handle<BallConstratint> c(new BallConstratint(), false);
				c->origin = origin;
				return c.Unmanage();
			} else if (type == "hinge") {
				auto origin = ReadVector3(json.get("origin", Json::nullValue),
										  "origin");
				auto axis = ReadVector3(json.get("axis", Json::nullValue),
										"axis");
				auto range = ReadVector2(json.get("range", Json::nullValue),
										 "range");
				Handle<HingeConstratint> c(new HingeConstratint(), false);
				range *= M_PI / 180.0;
				c->origin = origin;
				c->axis = axis;
				c->minAngle = range.x;
				c->maxAngle = range.y;
				return c.Unmanage();
			} else if (type == "universal") {
				auto origin = ReadVector3(json.get("origin", Json::nullValue),
										  "origin");
				auto axis1 = ReadVector3(json.get("axis1", Json::nullValue),
										"axis1");
				auto axis2 = ReadVector3(json.get("axis2", Json::nullValue),
										 "axis2");
				auto range1 = ReadVector2(json.get("range1", Json::nullValue),
										 "range1");
				auto range2 = ReadVector2(json.get("range2", Json::nullValue),
										 "range2");
				Handle<UniversalJointConstratint> c(new UniversalJointConstratint(), false);
				range1 *= M_PI / 180.0;
				range2 *= M_PI / 180.0;
				c->origin = origin;
				c->axis1 = axis1;
				c->axis2 = axis2;
				c->minAngle1 = range1.x;
				c->maxAngle1 = range1.y;
				c->minAngle2 = range2.x;
				c->maxAngle2 = range2.y;
				return c.Unmanage();
			} else {
				SPRaise("%s: unknown constraint type: %s",
						fullPath.c_str(), type.c_str());
			}
		}
		
		Object *ReadObject(const Json::Value& json) {
			if (!json.isObject()) {
				SPRaise("%s: object must be object", fullPath.c_str());
			}
			auto name = json.get("object", Json::nullValue);
			if (!name.isString()) {
				SPRaise("%s: identifier of object must be string", fullPath.c_str());
			}
			auto nameStr = name.asString();
			auto it = rootLoader.objects.find(nameStr);
			if (it == rootLoader.objects.end()) {
				SPRaise("%s: unknown object: '%s'", nameStr.c_str());
			}
			return it->second;
		}
		
	public:
		FrameLoader(RootLoader& rootLoader,
					const Json::Value& json):
		loader(rootLoader.loader),
		rootLoader(rootLoader),
		json(json) { }
		
		Frame *Load(Frame *parent) {
			SPADES_MARK_FUNCTION();
			
			Handle<Frame> frame(new Frame(), false);
			if(parent) parent->AddChild(frame);
			
			fullPath = frame->GetFullPath();
			
			// read ID
			auto id = json.get("id", Json::nullValue);
			if (id.isString()) {
				frame->SetId(id.asString());
			} else if (!id.isNull()) {
				SPRaise("%s: id should be string or null",
						frame->GetFullPath().c_str());
			}
			
			// read tags
			auto tags = json.get("tags", Json::nullValue);
			if (tags.isArray()) {
				for (const auto& v: tags) {
					if (v.isString()) {
						frame->AddTag(v.asString());
					} else {
						SPRaise("%s: tags.* should be null",
								frame->GetFullPath().c_str());
					}
				}
			} else if (tags.isString()) {
				frame->AddTag(tags.asString());
			} else if (!tags.isNull()) {
				SPRaise("%s: tags should be array, string, or null",
						frame->GetFullPath().c_str());
			}
			
			// read children
			auto children = json.get("children", Json::nullValue);
			if (children.isArray()) {
				for (const auto& v: children) {
					if (v.isObject()) {
						FrameLoader(rootLoader, v).Load(frame)->Release();
					} else {
						SPRaise("%s: objects.* should be object",
								frame->GetFullPath().c_str());
					}
				}
			} else if (children.isObject()) {
				FrameLoader(rootLoader, children).Load(frame)->Release();
			} else {
				SPRaise("%s: objects should be array, object, or null",
						frame->GetFullPath().c_str());
			}
			
			// parse transform
			auto pos = ReadVector3OrNull
			(json.get("position", Json::nullValue), "position");
			auto vpos = pos ? *pos : Vector3(0, 0, 0);
			
			Matrix4 rotMat = Matrix4::Identity();
			auto rots = json.get("rotation", Json::nullValue);
			if (rots.isArray() && rots.size() > 0 &&
				rots.get((Json::UInt)0, Json::nullValue).isArray()) {
				// array of rotations
				for (const auto& v: rots) {
					rotMat = ReadRotation(v) * rotMat;
				}
			} else if (!rots.isNull() &&
					   (!rots.isArray() || rots.size() > 0)) {
				// single rotation
				rotMat = ReadRotation(rots);
			}
			
			auto jscale = json.get("scale", Json::nullValue);
			Vector3 scale(1, 1, 1);
			if (jscale.isNull()) {
				// identity
			} else if (jscale.isConvertibleTo(Json::ValueType::realValue)) {
				auto v = jscale.asDouble();
				scale = Vector3(v, v, v);
			} else {
				SPRaise("Non-uniform scaling is not supported.");
				scale = ReadVector3(jscale, "scale");
			}
			
			auto mat = Matrix4::Translate(vpos);
			mat *= rotMat;
			mat *= Matrix4::Scale(scale.x, scale.y, scale.z);
			
			frame->SetTransform(mat);
			
			auto constraints = json.get("constraints", Json::nullValue);
			if (constraints.isNull()) {
			} else if (constraints.isObject()) {
				auto *c = ReadConstraint(constraints);
				frame->AddConstraint(c);
				c->Release();
			} else if (constraints.isArray()) {
				for (const auto& v: constraints) {
					auto *c = ReadConstraint(v);
					frame->AddConstraint(c);
					c->Release();
				}
			} else {
				SPRaise("%s: constraints should be array, object, or null",
						frame->GetFullPath().c_str());
			}
			
			auto objs = json.get("objects", Json::nullValue);
			if (objs.isNull()) {
			} else if (objs.isObject()) {
				auto *obj = ReadObject(objs);
				frame->AddObject(obj);
			} else if (objs.isArray()) {
				for (const auto& v: objs) {
					auto *obj = ReadObject(v);
					frame->AddObject(obj);
				}
			} else {
				SPRaise("%s: objects should be array, object, or null",
						frame->GetFullPath().c_str());
			}
			
			return frame.Unmanage();
		}
	};
	
	Frame *Loader::RootLoader::Load() {
		SPADES_MARK_FUNCTION();
		
		// load objects
		Json::Value objs = root.get("objects", Json::nullValue);
		if (objs.isNull()) {
			// no objects (rare case)
		} else if (objs.isObject()) {
			for (auto it = objs.begin(); it != objs.end(); ++it) {
				auto key = it.key();
				if (!key.isString()) {
					SPRaise("Key of $.objects must be string");
				}
				auto val = *it;
				auto path = val.get("path", Json::nullValue);
				if (!path.isString()) {
					SPRaise("$.objects.*.path must be string");
				}
				
				auto s = FileManager::CombinePath(basePath, path.asString());
				
				objects.emplace
				(key.asString(),
				 Handle<Object>(loader.LoadObject(s), false));
			}
		}
		
		// load root
		Json::Value scene = root.get("scene", Json::nullValue);
		
		if (!scene.isObject()) {
			SPRaise("$.scene is not an object");
		}
		
		return FrameLoader(*this, scene).Load(nullptr);
	}
	
	Frame *Loader::LoadFrame(const std::string &fileName) {
		SPADES_MARK_FUNCTION();
		
		auto dir = FileManager::GetPathWithoutFileName(fileName);
		auto absPath = ResolvePath(fileName);
		auto s = FileManager::ReadAllBytes(absPath.c_str());
		
		Json::Reader reader;
		Json::Value root;
		
		if (!reader.parse(s, root, false)) {
			SPRaise("JSON parse failed: %s",
					reader.getFormatedErrorMessages().c_str());
		}
		
		return RootLoader(*this, root,
						  dir).Load();
	}
	Object *Loader::LoadObject(const std::string &filename) {
		SPADES_MARK_FUNCTION();
		auto absPath = ResolvePath(filename);
		std::unique_ptr<IStream> stream
		(FileManager::OpenForReading(absPath.c_str()));
		Handle<VoxelModel> model
		(VoxelModel::LoadKV6(stream.get()), false);
		
		Handle<Object> obj
		(new VoxelModelObject(model), true);
		objects.emplace(filename, obj);
		
		return obj;
	}
	
	class TimelineLoader {
		Loader& l;
		Frame& frame;
		Json::Value& json;
		
		template<class F>
		void ReadKeyFrames(Json::Value& json,
						   F functor) {
			auto keys = json.get("keyframes", Json::nullValue);
			if (!keys.isArray()) {
				SPRaise("keyframes must be an array");
			}
			for (auto e: keys) {
				if (!e.isObject()) {
					SPRaise("$.channels[*].keyframes[*] must be objects");
				}
				
				auto jTime = e.get("time", Json::nullValue);
				if (!jTime.isConvertibleTo(Json::ValueType::realValue)) {
					SPRaise("$.channels[*].keyframes[*].time must be number");
				}
				
				auto jValue = e.get("value", Json::nullValue);
				if (jValue.isNull()) {
					SPRaise("$.channels[*].keyframes[*].value must not be null");
				}
				
				functor((float)jTime.asDouble(),
						jValue);
			}
		}
		
		void ReadLinearKeyFrames(Json::Value& json,
								 LinearAnimationChannel& chan) {
			ReadKeyFrames(json, [&](float time, const Json::Value& v) {
				if (!v.isConvertibleTo(Json::ValueType::realValue)) {
					SPRaise("$.channels[*].keyframes[*].value must be number");
				}
				chan.AddKeyFrame(time, (float)v.asDouble());
			});
		}
		
		void ReadRotationalKeyFrames(Json::Value& json,
								 RotationalAnimationChannel& chan) {
			ReadKeyFrames(json, [&](float time, const Json::Value& v) {
				if (!v.isArray()) {
					SPRaise("$.channels[*].keyframes[*].value must be an array");
				}
				Quaternion q;
				if (v.size() == 4){
					auto e1 = v.get((Json::UInt)0, Json::nullValue);
					auto e2 = v.get((Json::UInt)1, Json::nullValue);
					auto e3 = v.get((Json::UInt)2, Json::nullValue);
					auto e4 = v.get((Json::UInt)3, Json::nullValue);
					if (e1.isConvertibleTo(Json::ValueType::realValue) &&
						e2.isConvertibleTo(Json::ValueType::realValue) &&
						e3.isConvertibleTo(Json::ValueType::realValue) &&
						e4.isConvertibleTo(Json::ValueType::realValue)) {
						q = Quaternion(e2.asDouble(),
									   e3.asDouble(),
									   e4.asDouble(),
									   e1.asDouble());
					} else {
						SPRaise("invalid value for $.channels[*].keyframes[*].value"
								" to be provided for rotational property");
					}
				} else if (v.size() == 3){
					auto e1 = v.get((Json::UInt)0, Json::nullValue);
					auto e2 = v.get((Json::UInt)1, Json::nullValue);
					auto e3 = v.get((Json::UInt)2, Json::nullValue);
					if (e1.isConvertibleTo(Json::ValueType::realValue) &&
						e2.isConvertibleTo(Json::ValueType::realValue) &&
						e3.isConvertibleTo(Json::ValueType::realValue)) {
						q = Quaternion::MakeRotation
						(Vector3(e1.asDouble(),
								 e2.asDouble(),
								 e3.asDouble()));
					} else {
						SPRaise("invalid value for $.channels[*].keyframes[*].value"
								" to be provided for rotational property");
					}
				} else {
					SPRaise("invalid value for $.channels[*].keyframes[*].value"
							" to be provided for rotational property");
				}
				
				chan.AddKeyFrame(time, q);
			});
		}
		
	public:
		TimelineLoader(Loader& l, Frame& f,
					   Json::Value& json):
		l(l), frame(f), json(json) { }
		Timeline *Load() {
			SPADES_MARK_FUNCTION();
			
			Handle<Timeline> tl(new Timeline(), false);
			
			auto jChannels = json.get("channels", Json::nullValue);
			
			if (!jChannels.isArray()) {
				SPRaise("$.channels must be an array");
			}
			
			int channelIndex = 0;
			for (auto jChan: jChannels) {
				auto jId = jChan.get("id", Json::nullValue);
				std::string id = std::to_string(channelIndex);
				if (jId.isConvertibleTo(Json::ValueType::stringValue))
					id = jId.asString();
				channelIndex++;
				
				auto jFrameId = jChan.get("frame_id", Json::nullValue);
				if (!jFrameId.isConvertibleTo(Json::ValueType::stringValue))
					SPRaise("$.channels[%s].frame_id must be string", id.c_str());
				auto frameId = jFrameId.asString();
				
				auto *f = frame.GetFrameById(frameId);
				if (f == nullptr) {
					// ignore non-existent frame
					continue;
				}
				
				auto jProp = jChan.get("property", Json::nullValue);
				if (!jProp.isConvertibleTo(Json::ValueType::stringValue))
					SPRaise("$.channels[%s].property must be string", id.c_str());
				auto prop = jProp.asString();
				
				Handle<AnimationChannel> chan;
				
				if (prop == "position.x") {
					auto *p = new PositionAnimationChannel
					(f, PositionAnimationChannel::Axis::X);
					chan.Set(p, false);
					ReadLinearKeyFrames(jChan, *p);
				} else if (prop == "position.y") {
					auto *p = new PositionAnimationChannel
					(f, PositionAnimationChannel::Axis::Y);
					chan.Set(p, false);
					ReadLinearKeyFrames(jChan, *p);
				} else if (prop == "position.z") {
					auto *p = new PositionAnimationChannel
					(f, PositionAnimationChannel::Axis::Z);
					chan.Set(p, false);
					ReadLinearKeyFrames(jChan, *p);
				} else if (prop == "rotation") {
					auto *p = new RotationAnimationChannel(f);
					chan.Set(p, false);
					ReadRotationalKeyFrames(jChan, *p);
				} else if (prop == "scale") {
					auto *p = new ScaleAnimationChannel
					(f);
					chan.Set(p, false);
					ReadLinearKeyFrames(jChan, *p);
				} else {
					SPRaise("$.channels[%s].property: invalid value: '%s'",
							id.c_str(), prop.c_str());
				}
				
				tl->AddChannel(chan.Unmanage());
			}
			
			return tl.Unmanage();
		}
	};
	
	Timeline *Loader::LoadTimeline(const std::string& fileName,
								   Frame *rootFrame) {
		SPADES_MARK_FUNCTION();
		SPAssert(rootFrame);
		
		
		auto dir = FileManager::GetPathWithoutFileName(fileName);
		auto absPath = ResolvePath(fileName);
		auto s = FileManager::ReadAllBytes(absPath.c_str());
		
		Json::Reader reader;
		Json::Value root;
		
		if (!reader.parse(s, root, false)) {
			SPRaise("JSON parse failed: %s",
					reader.getFormatedErrorMessages().c_str());
		}
		
		return TimelineLoader(*this, *rootFrame, root).Load();
	}
	
	
} }