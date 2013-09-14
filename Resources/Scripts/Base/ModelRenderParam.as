namespace spades {
	class ModelRenderParam {
		Matrix4 matrix;
		Vector3 customColor;
		bool depthHack;	
		
		ModelRenderParam() {
			depthHack = false;
		}
	}
}
