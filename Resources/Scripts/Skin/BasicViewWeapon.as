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
	class BasicViewWeapon:
	IToolSkin, IViewToolSkin, IWeaponSkin, IWeaponSkin2 {
		// IToolSkin
		protected float sprintState;
		protected float raiseState;
		protected Vector3 teamColor;
		protected bool muted;

		float SprintState {
			set { sprintState = value; }
			get { return sprintState; }
		}

		float RaiseState {
			set { raiseState = value; }
			get { return raiseState; }
		}

		Vector3 TeamColor {
			set { teamColor = value; }
			get { return teamColor; }
		}

		bool IsMuted {
			set { muted = value; }
			get { return muted; }
		}

		// IWeaponSkin

		protected float aimDownSightState;
		protected float aimDownSightStateSmooth;
		protected float readyState;
		protected bool reloading;
		protected float reloadProgress;
		protected int ammo, clipSize;
		protected float localFireVibration;

		protected float sprintStateSmooth;

		float AimDownSightState {
			set {
				aimDownSightState = value;
				aimDownSightStateSmooth = SmoothStep(value);
			}
			get {
				return aimDownSightState;
			}
		}

		float AimDownSightStateSmooth {
			get { return aimDownSightStateSmooth; }
		}

		bool IsReloading {
			get { return reloading; }
			set { reloading = value; }
		}
		float ReloadProgress {
			get { return reloadProgress; }
			set { reloadProgress = value; }
		}
		int Ammo {
			set { ammo = value; }
			get { return ammo; }
		}
		int ClipSize {
			set { clipSize = value; }
			get { return clipSize; }
		}

		float ReadyState {
			set { readyState = value; }
			get { return readyState; }
		}

		// IViewToolSkin

		protected Matrix4 eyeMatrix;
		protected Vector3 swing;
		protected Vector3 leftHand;
		protected Vector3 rightHand;

		Matrix4 EyeMatrix {
			set { eyeMatrix = value; }
			get { return eyeMatrix; }
		}

		Vector3 Swing {
			set { swing = value; }
			get { return swing; }
		}

		Vector3 LeftHandPosition {
			get {
				return leftHand;
			}
			set {
				leftHand = value;
			}
		}
		Vector3 RightHandPosition {
			get  {
				return rightHand;
			}
			set {
				rightHand = value;
			}
		}

		// IWeaponSkin2
		protected float environmentRoom;
		protected float environmentSize;

		void SetSoundEnvironment(float room, float size, float distance) {
			environmentRoom = room;
			environmentSize = size;
		}
		// set_SoundOrigin is not called for first-person skin scripts
		Vector3 SoundOrigin {
			set { }
		}

		protected Renderer@ renderer;
		protected Image@ sightImage;

		BasicViewWeapon(Renderer@ renderer) {
			@this.renderer = renderer;
			localFireVibration = 0.f;
			@sightImage = renderer.RegisterImage
				("Gfx/Sight.tga");
		}

		float GetLocalFireVibration() {
			return localFireVibration;
		}

		float GetMotionGain() {
			return 1.f - AimDownSightStateSmooth * 0.4f;
		}

		float GetZPos() {
			return 0.2f - AimDownSightStateSmooth * 0.05f;
		}

		Vector3 GetLocalFireVibrationOffset() {
			float vib = GetLocalFireVibration();
			float motion = GetMotionGain();
			Vector3 hip = Vector3(
				sin(vib * PiF * 2.f) * 0.008f * motion,
				vib * (vib - 1.f) * 0.14f * motion,
				vib * (1.f - vib) * 0.03f * motion);
			Vector3 ads = Vector3(0.f, vib * (vib - 1.f) * vib * 0.3f * motion, 0.f);
			return Mix(hip, ads, AimDownSightStateSmooth);
		}

		Matrix4 GetViewWeaponMatrix() {
			Matrix4 mat;
			if(sprintStateSmooth > 0.f) {
				mat = CreateRotateMatrix(Vector3(0.f, 1.f, 0.f),
					sprintStateSmooth * -0.1f) * mat;
				mat = CreateRotateMatrix(Vector3(1.f, 0.f, 0.f),
					sprintStateSmooth * 0.3f) * mat;
				mat = CreateRotateMatrix(Vector3(0.f, 0.f, 1.f),
					sprintStateSmooth * -0.55f) * mat;
				mat = CreateTranslateMatrix(Vector3(0.23f, -0.05f, 0.15f)
					* sprintStateSmooth)  * mat;
			}

			if(raiseState < 1.f) {
				float putdown = 1.f - raiseState;
				mat = CreateRotateMatrix(Vector3(0.f, 0.f, 1.f),
					putdown * -1.3f) * mat;
				mat = CreateRotateMatrix(Vector3(0.f, 1.f, 0.f),
					putdown * 0.2f) * mat;
				mat = CreateTranslateMatrix(Vector3(0.1f, -0.3f, 0.1f)
					* putdown)  * mat;
			}

			Vector3 trans(0.f, 0.f, 0.f);
			trans += Vector3(-0.13f * (1.f - AimDownSightStateSmooth),
							 0.5f, GetZPos());
			trans += swing * GetMotionGain();
			trans += GetLocalFireVibrationOffset();
			mat = CreateTranslateMatrix(trans) * mat;

			return mat;
		}

		void Update(float dt) {
			localFireVibration -= dt * 10.f;
			if(localFireVibration < 0.f){
				localFireVibration = 0.f;
			}

			float sprintStateSS = sprintState * sprintState;
			if (sprintStateSS > sprintStateSmooth) {
				sprintStateSmooth += (sprintStateSS - sprintStateSmooth) * (1.f - pow(0.001, dt));
			} else {
				sprintStateSmooth = sprintStateSS;
			}
		}

		void WeaponFired(){
			localFireVibration = 1.f;
		}

		void AddToScene() {
		}

		void ReloadingWeapon() {
		}

		void ReloadedWeapon() {
		}

		void Draw2D() {
			renderer.ColorNP = (Vector4(1.f, 1.f, 1.f, 1.f));
			renderer.DrawImage(sightImage,
				Vector2((renderer.ScreenWidth - sightImage.Width) * 0.5f,
						(renderer.ScreenHeight - sightImage.Height) * 0.5f));
		}
	}

}
