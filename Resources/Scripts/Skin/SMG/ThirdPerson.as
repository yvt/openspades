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
	class ThirdPersonSMGSkin: 
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
		private AudioChunk@[] fireMechSounds(4);
		private AudioChunk@ reloadSound;
		
		ThirdPersonSMGSkin(Renderer@ r, AudioDevice@ dev) {
			@renderer = r;
			@audioDevice = dev;
			@model = renderer.RegisterModel
				("Models/Weapons/SMG/Weapon.kv6");
				
				
			@fireSound = dev.RegisterSound
				("Sounds/Weapons/SMG/Fire.wav");
			@fireFarSound = dev.RegisterSound
				("Sounds/Weapons/SMG/FireFar.wav");
			@fireStereoSound = dev.RegisterSound
				("Sounds/Weapons/SMG/FireStereo.wav");
			@reloadSound = dev.RegisterSound
				("Sounds/Weapons/SMG/Reload.wav");
				
			@fireMechSounds[0] = dev.RegisterSound
				("Sounds/Weapons/SMG/Mech1.wav");
			@fireMechSounds[1] = dev.RegisterSound
				("Sounds/Weapons/SMG/Mech2.wav");
			@fireMechSounds[2] = dev.RegisterSound
				("Sounds/Weapons/SMG/Mech3.wav");
			@fireMechSounds[3] = dev.RegisterSound
				("Sounds/Weapons/SMG/Mech4.wav");
		}
		
		void Update(float dt) {
		}
		
		void WeaponFired(){
			if(!muted){
				Vector3 origin = originMatrix * Vector3(0.f, 0.f, 0.f);
				AudioParam param;
				param.volume = 8.f;
				audioDevice.Play(fireSound, origin, param);
				
				param.volume = 1.f;
				audioDevice.Play(fireFarSound, origin, param);
				audioDevice.Play(fireStereoSound, origin, param);
				
				AudioChunk@ mechSound;
				@mechSound = fireMechSounds[GetRandom(fireMechSounds.length)];
				param.volume = 1.4;
				audioDevice.PlayLocal(mechSound, origin, param);
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
	
	IWeaponSkin@ CreateThirdPersonSMGSkin(Renderer@ r, AudioDevice@ dev) {
		return ThirdPersonSMGSkin(r, dev);
	}
}
