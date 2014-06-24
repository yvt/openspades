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

#include <vector>
#include <Core/RefCountedObject.h>
#include <Core/Math.h>
#include <unordered_map>
#include <map>
#include <list>
#include <set>
#include <array>
#include <Core/TMPUtils.h>

namespace spades {
	class VoxelModel;
}

namespace spades { namespace osobj {
	
	
	class Object: public RefCountedObject {
	protected:
		~Object();
	public:
		Object();
		
		virtual AABB3 GetBounds() = 0;
		virtual Vector3 GetCenterOfMass() = 0;
		virtual float GetMass() = 0;
		virtual std::array<Vector3, 3> GetInertiaTensor() = 0;
	};
	
	class VoxelModelObject: public Object {
		Handle<VoxelModel> model;
		Vector3 centerOfMass;
		float mass;
		std::array<Vector3, 3> inertiaTensor;
	protected:
		~VoxelModelObject();
	public:
		VoxelModelObject(VoxelModel *);
		
		AABB3 GetBounds() override;
		Vector3 GetCenterOfMass() override;
		float GetMass() override;
		std::array<Vector3, 3> GetInertiaTensor() override;
		
		VoxelModel& GetModel() { return *model; }
	};
	
	enum class ConstraintType {
		Ball,
		Hinge,
		UniversalJoint
	};
	
	class BallConstratint;
	class HingeConstratint;
	class UniversalJointConstratint;
	
	using ConstraintClassList = stmp::make_type_list<
		BallConstratint,
		HingeConstratint,
		UniversalJointConstratint
	>::list;
	class ConstraintVisitor:
	public stmp::visitor_generator<ConstraintClassList> {
	public:
		virtual ~ConstraintVisitor() {}
	};
	class ConstConstraintVisitor:
	public stmp::const_visitor_generator<ConstraintClassList> {
	public:
		virtual ~ConstConstraintVisitor() {}
	};
	
	class Constraint: public RefCountedObject {
	protected:
		~Constraint();
	public:
		Constraint();
		virtual ConstraintType GetType() = 0;
		virtual void Accept(ConstraintVisitor&) = 0;
		virtual void Accept(ConstConstraintVisitor&) const = 0;
	};
	
	class BallConstratint: public Constraint {
	protected:
		~BallConstratint() { }
	public:
		BallConstratint() { }
		ConstraintType GetType() override {
			return ConstraintType::Ball;
		}
		void Accept(ConstraintVisitor& visitor) override {
			visitor.visit(*this);
		}
		void Accept(ConstConstraintVisitor& visitor) const override {
			visitor.visit(*this);
		}
		
		Vector3 origin;
	};
	
	class HingeConstratint: public Constraint {
	protected:
		~HingeConstratint() { }
	public:
		HingeConstratint();
		ConstraintType GetType() override {
			return ConstraintType::Hinge;
		}
		void Accept(ConstraintVisitor& visitor) override {
			visitor.visit(*this);
		}
		void Accept(ConstConstraintVisitor& visitor) const override {
			visitor.visit(*this);
		}
		
		Vector3 origin;
		Vector3 axis;
		float minAngle;
		float maxAngle;
	};
	
	class UniversalJointConstratint: public Constraint {
	protected:
		~UniversalJointConstratint() { }
	public:
		UniversalJointConstratint();
		ConstraintType GetType() override {
			return ConstraintType::Hinge;
		}
		void Accept(ConstraintVisitor& visitor) override {
			visitor.visit(*this);
		}
		void Accept(ConstConstraintVisitor& visitor) const override {
			visitor.visit(*this);
		}
		
		Vector3 origin;
		Vector3 axis1;
		float minAngle1;
		float maxAngle1;
		Vector3 axis2;
		float minAngle2;
		float maxAngle2;
	};
	
	class Pose;
	
	class Frame: public RefCountedObject {
		Frame *parent = nullptr;
		std::vector<Handle<Frame>> children;
		std::vector<Handle<Object>> objects;
		std::set<std::string> tags;
		std::vector<Handle<Constraint>> constraints;
		std::string id;
		Matrix4 mat;
		
		
	protected:
		~Frame();
	public:
		Frame();
		
		void AddChild(Frame *);
		void RemoveFromParent();
		
		void AddObject(Object *);
		void RemoveObject(Object *);
		
		void AddConstraint(Constraint *);
		void RemoveConstraint(Constraint *);
		
		void AddTag(const std::string&);
		void RemoveTag(const std::string&);
		bool HasTag(const std::string&);
		
		void SetId(const std::string& s) { id = s; }
		std::string GetId() const { return id; }
		std::string GetFullPath();
		
		std::vector<Frame *> GetFramesByTag(const std::string& tag);
		std::vector<Frame *>& GetFramesByTag(const std::string& tag,
							std::vector<Frame *>&);
		Frame *GetFrameById(const std::string& Id);
		
		void SetTransform(const Matrix4& m) { mat = m; }
		const Matrix4& GetTransform() const { return mat; }
		
		Vector4 LocalToGlobal(const Vector4&,
							  Pose *pose = nullptr);
		
		Frame *GetParent() const { return parent; }
		const decltype(children)& GetChildren() const { return children; }
		
		const decltype(objects)& GetObjects() const { return objects; }
		
		const decltype(constraints)& GetConstraints() const { return constraints; }
		
	};
	
	class Pose: public RefCountedObject {
		struct Element {
			Matrix4 transform;
			
			Vector3 position;
			Quaternion rotation;
			float scale;
			
			bool needsUpdate;
			
			Element();
			void SetTransform(const Matrix4&);
			void SetPosition(const Vector3&);
			void SetRotation(const Quaternion&);
			void SetScale(float);
			
			void UpdateTransformIfNeeded();
		};
		std::unordered_map<Frame *, Element> elements;
		Handle<Frame> root;
		void Init(Frame *);
	protected:
		~Pose();
	public:
		void SetInitialState();
		Pose(Frame *);
		Matrix4 GetTransform(Frame *);
		void SetPosition(Frame *, const Vector3&);
		void SetRotation(Frame *, const Quaternion&);
		void SetScale(Frame *, float);
		void SetTransform(Frame *, const Matrix4&);
		Vector3 GetPosition(Frame *);
		Quaternion GetRotation(Frame *);
		float GetScale(Frame *);
		void Lerp(Pose& a, Pose& b,
				 float mix);
	};
	
	enum class ChannelType {
		Linear,
		Rotation
	};
	
	enum class ChannelProperty {
		Position,
		Rotation,
		Scale
	};
	
	class AnimationChannel: public RefCountedObject {
		Handle<Frame> frame;
	protected:
		~AnimationChannel();
	public:
		AnimationChannel(Frame *);
		Frame *GetFrame() { return frame; }
		virtual void Move(float time, Pose&) = 0;
		virtual ChannelType GetType() const = 0;
		virtual ChannelProperty GetProperty() const = 0;
	};
	
	class LinearAnimationChannel: public AnimationChannel {
		std::map<float, float> keyFrames;
		struct Curve;
		std::unique_ptr<Curve> curve;
	protected:
		~LinearAnimationChannel();
		float Evaluate(float time);
	public:
		LinearAnimationChannel(Frame *);
		
		ChannelType GetType() const override { return ChannelType::Linear; }
		
		void AddKeyFrame(float time, float value);
		void RemoveKeyFrame(float);
		std::vector<std::pair<float, float>> GetKeyFrames();
	};
	
	class RotationalAnimationChannel: public AnimationChannel {
		std::map<float, Quaternion> keyFrames;
		struct Curve;
		std::unique_ptr<Curve> curve;
	protected:
		~RotationalAnimationChannel();
		Quaternion Evaluate(float time);
	public:
		RotationalAnimationChannel(Frame *);
		
		ChannelType GetType() const override { return ChannelType::Rotation; }
		
		void AddKeyFrame(float time, const Quaternion& value);
		void RemoveKeyFrame(float);
		std::vector<std::pair<float, Quaternion>> GetKeyFrames();
	};
	
	class PositionAnimationChannel: public LinearAnimationChannel {
	protected:
		~PositionAnimationChannel();
	public:
		enum class Axis {
			X, Y, Z
		};
		PositionAnimationChannel(Frame *, Axis);
		
		void Move(float time, Pose&) override;
		ChannelProperty GetProperty() const override {
			return ChannelProperty::Position;
		}
		
	private:
		Axis axis;
	};
	
	class ScaleAnimationChannel: public LinearAnimationChannel {
	protected:
		~ScaleAnimationChannel();
	public:
		ScaleAnimationChannel(Frame *);
		
		void Move(float time, Pose&) override;
		ChannelProperty GetProperty() const override {
			return ChannelProperty::Scale;
		}
	};
	
	class RotationAnimationChannel: public RotationalAnimationChannel {
	protected:
		~RotationAnimationChannel();
	public:
		RotationAnimationChannel(Frame *);
		
		void Move(float time, Pose&) override;
		ChannelProperty GetProperty() const override {
			return ChannelProperty::Rotation;
		}
	};
	
	class Timeline: public RefCountedObject {
		std::list<Handle<AnimationChannel>> channels;
	protected:
		~Timeline();
	public:
		Timeline();
		
		void AddChannel(AnimationChannel *);
		void RemoveChannel(AnimationChannel *);
		const decltype(channels)& GetChannels() const
		{ return channels; }
		
		void Move(float time, Pose&);
	};
	
	class Loader: public RefCountedObject {
		
		class RootLoader;
		class FrameLoader;
		
		std::unordered_map<std::string, Handle<Object>> objects;
		std::string const rootPath;
		
		std::string ResolvePath(const std::string&);
		
		Object *LoadObject(const std::string& filename);
	protected:
		~Loader();
	public:
		Loader(const std::string& rootPath);
		
		Frame *LoadFrame(const std::string& fileName);
		Timeline *LoadTimeline(const std::string& fileName,
							   Frame *rootFrame);
	};
	
	
} }
