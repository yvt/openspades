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
	class ViewSpadeSkin:
	IToolSkin, IViewToolSkin, ISpadeSkin {
		private float sprintState;
		private float raiseState;
		private Vector3 teamColor;
		private Matrix4 eyeMatrix;
		private Vector3 swing;
		private Vector3 leftHand;
		private Vector3 rightHand;
		private SpadeActionType actionType;
		private float actionProgress;

		private float sprintStateSmooth;

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

		Matrix4 EyeMatrix {
			set { eyeMatrix = value; }
		}

		Vector3 Swing {
			set { swing = value; }
		}

		Vector3 LeftHandPosition {
			get {
				return leftHand;
			}
		}
		Vector3 RightHandPosition {
			get  {
				return rightHand;
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
		private Image@ sightImage;

		ViewSpadeSkin(Renderer@ r, AudioDevice@ dev) {
			@renderer = r;
			@audioDevice = dev;
			@model = renderer.RegisterModel
				("Models/Weapons/Spade/Spade.kv6");
			@sightImage = renderer.RegisterImage
				("Gfx/Sight.tga");
		}

		void Update(float dt) {
			float sprintStateSS = sprintState * sprintState;
			if (sprintStateSS > sprintStateSmooth) {
				sprintStateSmooth += (sprintStateSS - sprintStateSmooth) * (1.f - pow(0.001, dt));
			} else {
				sprintStateSmooth = sprintStateSS;
			}
		}

		void AddToScene() {
			Matrix4 mat = CreateScaleMatrix(0.033f);

			if(actionType == spades::SpadeActionType::Bash) {
				float per = 1.f - actionProgress;
				mat = CreateRotateMatrix(Vector3(1.f, 0.f, 0.f),
					per * 1.7f) * mat;
				mat = CreateTranslateMatrix(0.f, per * 0.3f, 0.f)
					* mat;
			}else if(actionType == spades::SpadeActionType::DigStart ||
					 actionType == spades::SpadeActionType::Dig) {
				bool first = actionType == spades::SpadeActionType::DigStart;
				float per = actionProgress;

				// some tunes
				const float readyFront = -0.8f;
				const float digAngle = 0.6f;
				const float readyAngle = 0.6f;

				float angle;
				float front = readyFront;
				float side = 1.f;

				if(per < 0.5f) {
					if(first) {
						// bringing to the dig position
						per = 4.f * per * per;
						angle = per * readyAngle;
						side = per;
						front = per * readyFront;
					}else{
						// soon after digging
						angle = readyAngle;
						per = (0.5f - per) / 0.5f;
						per *= per; per *= per;
						angle += per * digAngle;
						front += per * 2.0f;
					}
				}else{
					per = (per - 0.5f) / 0.5f;
					per = 1.f - (1.f-per) * (1.f-per);
					angle = readyAngle + per * digAngle;
					front += per * 2.f;
				}

				mat = CreateRotateMatrix(Vector3(1.f, 0.f, 0.f),
					angle) * mat;
				mat = CreateRotateMatrix(Vector3(0.f, 0.f, 1.f),
					front * 0.15f) * mat;

				side *= 0.3f;
				front *= 0.1f;

				mat = CreateTranslateMatrix(side, front, front * 0.2f)
					* mat;
			}

			if(sprintStateSmooth > 0.f || raiseState < 1.f){
				float per = Max(sprintStateSmooth, 1.f - raiseState);
				mat = CreateRotateMatrix(Vector3(0.f, 1.f, 0.f),
					per * 1.3f) * mat;
				mat = CreateTranslateMatrix(Vector3(0.3f, -0.4f, -0.1f) * per)
					* mat;
			}

			mat = CreateTranslateMatrix(0.f, (1.f - raiseState) * -0.3f, 0.f) * mat;

			mat = CreateTranslateMatrix(-0.3f, 0.7f, 0.3f) * mat;
			mat = CreateTranslateMatrix(swing) * mat;

			leftHand = mat * Vector3(0.f, 0.f, 7.f);
			rightHand = mat * Vector3(0.f, 0.f, -2.f);

			ModelRenderParam param;
			param.matrix = eyeMatrix * mat;
			param.depthHack = true;
			renderer.AddModel(model, param);
		}

		void Draw2D() {
			renderer.ColorNP = (Vector4(1.f, 1.f, 1.f, 1.f));
			renderer.DrawImage(sightImage,
				Vector2((renderer.ScreenWidth - sightImage.Width) * 0.5f,
						(renderer.ScreenHeight - sightImage.Height) * 0.5f));
		}
	}

	ISpadeSkin@ CreateViewSpadeSkin(Renderer@ r, AudioDevice@ dev) {
		return ViewSpadeSkin(r, dev);
	}
}
