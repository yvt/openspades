#if 0

uniform sampler3D ambientShadowTexture;
varying vec3 radiosity;
varying vec3 ambientShadowTextureCoord;
varying vec3 ambientColor;

vec3 Radiosity_Map(float detailAmbientOcclusion) {
	vec3 col = radiosity;
	float amb = texture3D(ambientShadowTexture, ambientShadowTextureCoord).x;
	
	// method1: 
	amb = sqrt(amb * detailAmbientOcclusion);
	
	// method2:
	//amb = mix(amb, detailAmbientOcclusion, 0.5);
	
	// method3: 
	//amb = min(amb, detailAmbientOcclusion);
	
	col *= detailAmbientOcclusion;
	col += amb * ambientColor;
	
	return col;
}

#else

/**** CPU RADIOSITY (FASTER?) *****/

uniform sampler3D ambientShadowTexture;
uniform sampler3D radiosityTextureFlat;
uniform sampler3D radiosityTextureX;
uniform sampler3D radiosityTextureY;
uniform sampler3D radiosityTextureZ;
varying vec3 radiosityTextureCoord;
varying vec3 ambientShadowTextureCoord;
varying vec3 normalVarying;
uniform vec3 ambientColor;

vec3 DecodeRadiosityValue(vec3 val){
	// reverse bias
	val *= 1023. / 1022.;
	val = (val * 2.) - 1.;
	//val *= val * sign(val);
	return val;
}

vec3 Radiosity_Map(float detailAmbientOcclusion) {
	vec3 col = DecodeRadiosityValue
	(texture3D(radiosityTextureFlat,
			   radiosityTextureCoord).xyz);
	vec3 normal = normalize(normalVarying);
	col += normal.x * DecodeRadiosityValue
	(texture3D(radiosityTextureX,
			   radiosityTextureCoord).xyz);
	col += normal.y * DecodeRadiosityValue
	(texture3D(radiosityTextureY,
			   radiosityTextureCoord).xyz);
	col += normal.z * DecodeRadiosityValue
	(texture3D(radiosityTextureZ,
			   radiosityTextureCoord).xyz);
	col = max(col, 0.);
	
	// ambient occlusion
	float amb = texture3D(ambientShadowTexture, ambientShadowTextureCoord).x;
	amb = max(amb, 0.); // by some reason, texture value becomes negative
	
	// method1:
	amb = sqrt(amb * detailAmbientOcclusion);
	// method2:
	//amb = mix(amb, detailAmbientOcclusion, 0.5);
	
	// method3:
	//amb = min(amb, detailAmbientOcclusion);
	
	col *= detailAmbientOcclusion;
	col += amb * ambientColor;
	
	return col;
}

#endif