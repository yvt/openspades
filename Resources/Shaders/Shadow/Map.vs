


varying vec3 mapShadowCoord;

void PrepareForShadow_Map(vec3 vertexCoord, vec3 normal) {
	mapShadowCoord = vertexCoord;
	mapShadowCoord.y -= mapShadowCoord.z;
	
	// texture value is normalized unsigned integer
	mapShadowCoord.z /= 255.;
	
	// texture coord is normalized
	// FIXME: variable texture size
	mapShadowCoord.xy /= 512.;
}