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
	IToolSkin, IThirdPersonToolSkin, IWeaponSkin, IWeaponSkin2 {
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

		private float environmentRoom;
		private float environmentSize;
		private float environmentDistance;
		private Vector3 soundOrigin;

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

		// IWeaponSkin2
		void SetSoundEnvironment(float room, float size, float distance) {
			environmentRoom = room;
			environmentSize = size;
			environmentDistance = distance;
		}
		Vector3 SoundOrigin {
			set { soundOrigin = value; }
		}

		private Renderer@ renderer;
		private AudioDevice@ audioDevice;
		private Model@ model;

		private AudioChunk@[] fireMediumSounds(4);
		private AudioChunk@ fireFarSound;
		private AudioChunk@ fireStereoSound;
		private AudioChunk@[] fireSmallReverbSounds(4);
		private AudioChunk@[] fireLargeReverbSounds(4);
		private AudioChunk@ reloadSound;

		ThirdPersonSMGSkin(Renderer@ r, AudioDevice@ dev) {
			@renderer = r;
			@audioDevice = dev;
			@model = renderer.RegisterModel
				("Models/Weapons/SMG/Weapon.kv6");


			@fireMediumSounds[0] = dev.RegisterSound
				("Sounds/Weapons/SMG/V2Third1.opus");
			@fireMediumSounds[1] = dev.RegisterSound
				("Sounds/Weapons/SMG/V2Third2.opus");
			@fireMediumSounds[2] = dev.RegisterSound
				("Sounds/Weapons/SMG/V2Third3.opus");
			@fireMediumSounds[3] = dev.RegisterSound
				("Sounds/Weapons/SMG/V2Third4.opus");

			@fireSmallReverbSounds[0] = dev.RegisterSound
				("Sounds/Weapons/SMG/V2AmbienceSmall1.opus");
			@fireSmallReverbSounds[1] = dev.RegisterSound
				("Sounds/Weapons/SMG/V2AmbienceSmall2.opus");
			@fireSmallReverbSounds[2] = dev.RegisterSound
				("Sounds/Weapons/SMG/V2AmbienceSmall3.opus");
			@fireSmallReverbSounds[3] = dev.RegisterSound
				("Sounds/Weapons/SMG/V2AmbienceSmall4.opus");

			@fireLargeReverbSounds[0] = dev.RegisterSound
				("Sounds/Weapons/SMG/V2AmbienceLarge1.opus");
			@fireLargeReverbSounds[1] = dev.RegisterSound
				("Sounds/Weapons/SMG/V2AmbienceLarge2.opus");
			@fireLargeReverbSounds[2] = dev.RegisterSound
				("Sounds/Weapons/SMG/V2AmbienceLarge3.opus");
			@fireLargeReverbSounds[3] = dev.RegisterSound
				("Sounds/Weapons/SMG/V2AmbienceLarge4.opus");

			@fireFarSound = dev.RegisterSound
				("Sounds/Weapons/SMG/FireFar.opus");
			@fireStereoSound = dev.RegisterSound
				("Sounds/Weapons/SMG/FireStereo.opus");
			@reloadSound = dev.RegisterSound
				("Sounds/Weapons/SMG/Reload.opus");

		}

		void Update(float dt) {
		}

		void WeaponFired(){
			if(!muted){
				Vector3 origin = soundOrigin;
				AudioParam param;
				param.volume = 9.f;
				audioDevice.Play(fireMediumSounds[GetRandom(fireMediumSounds.length)], origin, param);

				param.volume = 8.f * environmentRoom;
				param.referenceDistance = 10.f;
				if (environmentSize < 0.5f) {
					audioDevice.Play(fireSmallReverbSounds[GetRandom(fireSmallReverbSounds.length)], origin, param);
				} else {
					audioDevice.Play(fireLargeReverbSounds[GetRandom(fireLargeReverbSounds.length)], origin, param);
				}

				param.volume = .4f;
				param.referenceDistance = 10.f;
				audioDevice.Play(fireFarSound,  origin, param);
				param.referenceDistance = 1.f;
				audioDevice.Play(fireStereoSound, origin, param);

			}
		}
		void ReloadingWeapon() {
			if(!muted){
				Vector3 origin = soundOrigin;
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
