
uniform sampler2D mapShadowTexture;

varying vec3 mapShadowCoord;

float VisibilityOfSunLight_Map() {
	float val = texture2D(mapShadowTexture, mapShadowCoord.xy).w;
	if(val < mapShadowCoord.z - 0.0001)
		return 0.;
	else
		return 1.;
}
