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
	class ViewSMGSkin:
	IToolSkin, IViewToolSkin, IWeaponSkin, IWeaponSkin2,
	BasicViewWeapon {

		private AudioDevice@ audioDevice;
		private Model@ gunModel;
		private Model@ magazineModel;
		private Model@ sightModel1;
		private Model@ sightModel2;
		private Model@ sightModel3;

		private AudioChunk@[] fireSounds(4);
		private AudioChunk@ fireFarSound;
		private AudioChunk@ fireStereoSound;
		private AudioChunk@[] fireSmallReverbSounds(4);
		private AudioChunk@[] fireLargeReverbSounds(4);
		private AudioChunk@ reloadSound;

		ViewSMGSkin(Renderer@ r, AudioDevice@ dev){
			super(r);
			@audioDevice = dev;
			@gunModel = renderer.RegisterModel
				("Models/Weapons/SMG/WeaponNoMagazine.kv6");
			@magazineModel = renderer.RegisterModel
				("Models/Weapons/SMG/Magazine.kv6");
			@sightModel1 = renderer.RegisterModel
				("Models/Weapons/SMG/Sight1.kv6");
			@sightModel2 = renderer.RegisterModel
				("Models/Weapons/SMG/Sight2.kv6");
			@sightModel3 = renderer.RegisterModel
				("Models/Weapons/SMG/Sight3.kv6");

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

			@fireSounds[0] = dev.RegisterSound
				("Sounds/Weapons/SMG/V2Local1.opus");
			@fireSounds[1] = dev.RegisterSound
				("Sounds/Weapons/SMG/V2Local2.opus");
			@fireSounds[2] = dev.RegisterSound
				("Sounds/Weapons/SMG/V2Local3.opus");
			@fireSounds[3] = dev.RegisterSound
				("Sounds/Weapons/SMG/V2Local4.opus");
			@fireFarSound = dev.RegisterSound
				("Sounds/Weapons/SMG/FireFar.opus");
			@fireStereoSound = dev.RegisterSound
				("Sounds/Weapons/SMG/FireStereo.opus");
			@reloadSound = dev.RegisterSound
				("Sounds/Weapons/SMG/ReloadLocal.opus");

		}

		void Update(float dt) {
			BasicViewWeapon::Update(dt);
		}

		void WeaponFired(){
			BasicViewWeapon::WeaponFired();

			if(!IsMuted){
				Vector3 origin = Vector3(0.4f, -0.3f, 0.5f);
				AudioParam param;
				param.volume = 8.f;
				audioDevice.PlayLocal(fireSounds[GetRandom(fireSounds.length)], origin, param);

				param.volume = 8.f * environmentRoom;
				if (environmentSize < 0.5f) {
					audioDevice.PlayLocal(fireSmallReverbSounds[GetRandom(fireSmallReverbSounds.length)], origin, param);
				} else {
					audioDevice.PlayLocal(fireLargeReverbSounds[GetRandom(fireLargeReverbSounds.length)], origin, param);
				}
			}
		}

		void ReloadingWeapon() {
			if(!IsMuted){
				Vector3 origin = Vector3(0.4f, -0.3f, 0.5f);
				AudioParam param;
				param.volume = 0.2f;
				audioDevice.PlayLocal(reloadSound, origin, param);
			}
		}

		float GetZPos() {
			return 0.2f - AimDownSightStateSmooth * 0.038f;
		}

		// rotates gun matrix to ensure the sight is in
		// the center of screen (0, ?, 0).
		Matrix4 AdjustToAlignSight(Matrix4 mat, Vector3 sightPos, float fade) {
			Vector3 p = mat * sightPos;
			mat = CreateRotateMatrix(Vector3(0.f, 0.f, 1.f), atan(p.x / p.y) * fade) * mat;
			mat = CreateRotateMatrix(Vector3(-1.f, 0.f, 0.f), atan(p.z / p.y) * fade) * mat;
			return mat;
		}

		void Draw2D() {
			if(AimDownSightState > 0.6)
				return;
			BasicViewWeapon::Draw2D();
		}

		void AddToScene() {
			Matrix4 mat = CreateScaleMatrix(0.033f);
			mat = GetViewWeaponMatrix() * mat;

			bool reloading = IsReloading;
			float reload = ReloadProgress;
			Vector3 leftHand, rightHand;

			leftHand = mat * Vector3(1.f, 6.f, 1.f);
			rightHand = mat * Vector3(0.f, -8.f, 4.f);

			Vector3 leftHand2 = mat * Vector3(5.f, -10.f, 4.f);
			Vector3 leftHand3 = mat * Vector3(1.f, 6.f, -4.f);
			Vector3 leftHand4 = mat * Vector3(1.f, 9.f, -6.f);

			if(AimDownSightStateSmooth > 0.8f){
				mat = AdjustToAlignSight(mat, Vector3(0.f, 5.f, -4.9f), (AimDownSightStateSmooth - 0.8f) / 0.2f);
			}

			ModelRenderParam param;
			Matrix4 weapMatrix = eyeMatrix * mat;
			param.matrix = weapMatrix;
			param.depthHack = true;
			renderer.AddModel(gunModel, param);

			// draw sights
			Matrix4 sightMat = weapMatrix;
			sightMat *= CreateTranslateMatrix(0.05f, 5.f, -4.85f);
			sightMat *= CreateScaleMatrix(0.1f);
			param.matrix = sightMat;
			renderer.AddModel(sightModel1, param); // front

			sightMat = weapMatrix;
			sightMat *= CreateTranslateMatrix(0.025f, 5.f, -4.875f);
			sightMat *= CreateScaleMatrix(0.05f);
			param.matrix = sightMat;
			renderer.AddModel(sightModel3, param); // front pin

			sightMat = weapMatrix;
			sightMat *= CreateTranslateMatrix(0.04f, -9.f, -4.9f);
			sightMat *= CreateScaleMatrix(0.08f);
			param.matrix = sightMat;
			renderer.AddModel(sightModel2, param); // rear

			// magazine/reload action
			mat *= CreateTranslateMatrix(0.f, 3.f, 1.f);
			reload *= 2.5f;
			if(reloading) {
				if(reload < 0.7f){
					// magazine release
					float per = reload / 0.7f;
					mat *= CreateTranslateMatrix(0.f, 0.f, per*per*50.f);
					leftHand = Mix(leftHand, leftHand2, SmoothStep(per));
				}else if(reload < 1.4f) {
					// insert magazine
					float per = (1.4f - reload) / 0.7f;
					if(per < 0.3f) {
						// non-smooth insertion
						per *= 4.f; per -= 0.4f;
						per = Clamp(per, 0.0f, 0.3f);
					}

					mat *= CreateTranslateMatrix(0.f, 0.f, per*per*10.f);
					leftHand = mat * Vector3(0.f, 0.f, 4.f);
				}else if(reload < 1.9f){
					// move the left hand to the original position
					// and start doing something with the right hand
					float per = (reload - 1.4f) / 0.5f;
					leftHand = mat * Vector3(0.f, 0.f, 4.f);
					leftHand = Mix(leftHand, leftHand3, SmoothStep(per));
				}else if(reload < 2.2f){
					float per = (reload - 1.9f) / 0.3f;
					leftHand = Mix(leftHand3, leftHand4, SmoothStep(per));
				}else{
					float per = (reload - 2.2f) / 0.3f;
					leftHand = Mix(leftHand4, leftHand, SmoothStep(per));
				}
			}

			param.matrix = eyeMatrix * mat;
			renderer.AddModel(magazineModel, param);

			LeftHandPosition = leftHand;
			RightHandPosition = rightHand;
		}

	}

	IWeaponSkin@ CreateViewSMGSkin(Renderer@ r, AudioDevice@ dev) {
		return ViewSMGSkin(r, dev);
	}
}
