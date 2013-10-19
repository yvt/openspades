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
	IToolSkin, IViewToolSkin, IWeaponSkin {
		// IToolSkin
		private float sprintState;
		private float raiseState;
		private Vector3 teamColor;
		private bool muted;
		
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
		
		private float aimDownSightState;
		private float aimDownSightStateSmooth;
		private float readyState;
		private bool reloading;
		private float reloadProgress;
		private int ammo, clipSize;
		private float localFireVibration;
		
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
		
		private Matrix4 eyeMatrix;
		private Vector3 swing;
		private Vector3 leftHand;
		private Vector3 rightHand;
		
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
		
		
		BasicViewWeapon() {
			localFireVibration = 0.f;
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
			if(sprintState > 0.f) {
				mat = CreateRotateMatrix(Vector3(0.f, 0.f, 1.f),
					sprintState * -1.3f) * mat;
				mat = CreateRotateMatrix(Vector3(0.f, 1.f, 0.f),
					sprintState * 0.2f) * mat;
				mat = CreateTranslateMatrix(Vector3(0.2f, -0.2f, 0.05f)
					* sprintState)  * mat;
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
	}
	
}
