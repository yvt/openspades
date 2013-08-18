//
//  Corpse.h
//  OpenSpades
//
//  Created by yvt on 7/19/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

#pragma once

#include "../Core/Math.h"

namespace spades {
	namespace client {
		class IRenderer;
		class GameMap;
		class Player;
		class IModel;
		
		class Corpse {
			enum NodeType {
				// torso in CW seen from front
				Torso1, Torso2,
				Torso3, Torso4,
				
				Arm1, // Torso1
				Arm2, // Torso2
				Leg1, // Torso3
				Leg2, // Torso4
				
				Head,
				
				NodeCount
			};
			
			struct Node {
				Vector3 pos, vel;
				Vector3 lastPos;
				
				Vector3 lastForce;
			};
			
			IRenderer *renderer;
			GameMap *map;
			Vector3 color;
			
			Node nodes[NodeCount];
			
			void SetNode(NodeType n, Vector3);
			void SetNode(NodeType n, Vector4);
			
			void Spring(NodeType n1,
						NodeType n2,
						float distance,
						float dt);
			void Spring(NodeType n1a,
						NodeType n1b,
						NodeType n2,
						float distance,
						float dt);
			void AngleSpring(NodeType base,
							 NodeType a,
							 NodeType b,
							 float minDot,
							 float maxDot,
							 float dt);
			void AngleSpring(NodeType a,
							 NodeType b,
							 Vector3 dir,
							 float minDot,
							 float maxDot,
							 float dt);
			
			void ApplyConstraint(float dt);
			
			void LineCollision(NodeType a, NodeType b, float dt);
			
			
		public:
			Corpse(IRenderer *renderer,
				   GameMap *map,
				   Player *p);
			~Corpse();
			
			void Update(float dt);
			
			void AddToScene();
			
			Vector3 GetCenter();
			bool IsVisibleFrom(Vector3 eye);
			
			void AddImpulse(Vector3);
		};
	}
}
