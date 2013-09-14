namespace spades {
	class SceneDefinition {
		// viewport(Left|Top|Width|Height) is actually not used
		float fovX, fovY;
		Vector3 viewOrigin;
		Vector3 viewAxisX;
		Vector3 viewAxisY;
		Vector3 viewAxisZ;
		float zNear, zFar;
		bool skipWorld;
		
		float depthOfFieldNearRange;
		uint time;
		bool denyCameraBlur;
		float blurVignette;
		
		SceneDefinition() {
			depthOfFieldNearRange = 0.f;
			time = 0;
			denyCameraBlur = true;
			blurVignette = 0.f;
			skipWorld = true;	
		}
	}
}
