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
 
 namespace spades {
	class ThirdPersonSpadeSkin: 
	IToolSkin, IThirdPersonToolSkin, ISpadeSkin {
		private float sprintState;
		private float raiseState;
		private Vector3 teamColor;
		private Matrix4 originMatrix;
		private SpadeActionType actionType;
		private float actionProgress;
		
		float SprintState { 
			set { sprintState = value; }
		}
		
		float RaiseState { 
			set { raiseState = value; }
		}
		
		Vector3 TeamColor { 
			set { teamColor = value; } 
		}
		
		bool IsMuted {
			set {
				// nothing to do
			}
		}
		
		Matrix4 OriginMatrix {
			set { originMatrix = value; }
		}
		
		float PitchBias {
			get {
				float pitch = 0.f; 
				if(actionType == spades::SpadeActionType::Bash) {
					float per = 1.f - actionProgress;
					pitch -= per * 0.7f;
				}
				return pitch;
			}
		}
		
		SpadeActionType ActionType {
			set { actionType = value; }
		}
		
		float ActionProgress {
			set { actionProgress = value; }
		}
		
		private Renderer@ renderer;
		private AudioDevice@ audioDevice;
		private Model@ model;
		
		ThirdPersonSpadeSkin(Renderer@ r, AudioDevice@ dev) {
			@renderer = r;
			@audioDevice = dev;
			@model = renderer.RegisterModel
				("Models/Weapons/Spade/Spade.kv6");
		}
		
		void Update(float dt) {
		}
		
		void AddToScene() {
			Matrix4 mat = CreateScaleMatrix(0.05f);
			
			
			mat = CreateRotateMatrix(Vector3(0.f,0.f,1.f), Pi) * mat;
			mat = CreateTranslateMatrix(0.35f, -1.f, 0.f) * mat;
			
			
			ModelRenderParam param;
			param.matrix = originMatrix * mat;
			renderer.AddModel(model, param);
		}
	}
	
	ISpadeSkin@ CreateThirdPersonSpadeSkin(Renderer@ r, AudioDevice@ dev) {
		return ThirdPersonSpadeSkin(r, dev);
	}
}
