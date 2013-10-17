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
	class ThirdPersonShotgunSkin: 
	IToolSkin, IThirdPersonToolSkin, IWeaponSkin {
		private float sprintState;
		private float raiseState;
		private Vector3 teamColor;
		private bool muted;
		private Matrix4 originMatrix;
		private float aimDownSightState;
		private float readyState;
		private bool reloading;
		private float reloadProgress;
		private int ammo, clipSize;
		
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
			set { muted = value; }
		}
		
		Matrix4 OriginMatrix {
			set { originMatrix = value; }
		}
		
		float PitchBias {
			get { return 0.f; }
		}
		
		float AimDownSightState {
			set { aimDownSightState = value; }
		}
		
		bool IsReloading {
			set { reloading = value; }
		}
		float ReloadProgress {
			set { reloadProgress = value; }
		}
		int Ammo {
			set { ammo = value; }
		}
		int ClipSize {
			set { clipSize = value; }
		}
		
		float ReadyState {
			set { readyState = value; }
		}
		
		private Renderer@ renderer;
		private AudioDevice@ audioDevice;
		private Model@ model;
		
		private AudioChunk@ fireSound;
		private AudioChunk@ fireFarSound;
		private AudioChunk@ fireStereoSound;
		private AudioChunk@ reloadSound;
		private AudioChunk@ cockSound;
		
		ThirdPersonShotgunSkin(Renderer@ r, AudioDevice@ dev) {
			@renderer = r;
			@audioDevice = dev;
			@model = renderer.RegisterModel
				("Models/Weapons/Shotgun/Weapon.kv6");
				
			@fireSound = dev.RegisterSound
				("Sounds/Weapons/Shotgun/Fire.wav");
			@fireFarSound = dev.RegisterSound
				("Sounds/Weapons/Shotgun/FireFar.wav");
			@fireStereoSound = dev.RegisterSound
				("Sounds/Weapons/Shotgun/FireStereo.wav");
			@reloadSound = dev.RegisterSound
				("Sounds/Weapons/Shotgun/Reload.wav");
			@cockSound = dev.RegisterSound
				("Sounds/Weapons/Shotgun/Cock.wav");
		}
		
		void Update(float dt) {
		}
		
		void WeaponFired(){
			if(!muted){
				Vector3 origin = originMatrix * Vector3(0.f, 0.f, 0.f);
				AudioParam param;
				param.volume = 8.f;
				audioDevice.Play(fireSound, origin, param);
				
				param.volume = 2.f;
				audioDevice.Play(fireFarSound, origin, param);
				audioDevice.Play(fireStereoSound, origin, param);
			}
		}
		void ReloadingWeapon() {
			if(!muted){
				Vector3 origin = originMatrix * Vector3(0.f, 0.f, 0.f);
				AudioParam param;
				param.volume = 0.2f;
				audioDevice.Play(reloadSound, origin, param);
			}
		}
		
		void ReloadedWeapon() {
			if(!muted){
				Vector3 origin = originMatrix * Vector3(0.f, 0.f, 0.f);
				AudioParam param;
				param.volume = 0.2f;
				audioDevice.Play(cockSound, origin, param);
			}
		}
		
		void AddToScene() {
			Matrix4 mat = CreateScaleMatrix(0.05f);
			mat = mat * CreateScaleMatrix(-1.f, -1.f, 1.f);
			mat = CreateTranslateMatrix(0.35f, -1.f, 0.0f) * mat;
			
			ModelRenderParam param;
			param.matrix = originMatrix * mat;
			renderer.AddModel(model, param);
		}
	}
	
	IWeaponSkin@ CreateThirdPersonShotgunSkin(Renderer@ r, AudioDevice@ dev) {
		return ThirdPersonShotgunSkin(r, dev);
	}
}
