
uniform sampler2DShadow shadowMapTexture1;
uniform sampler2DShadow shadowMapTexture2;
uniform sampler2DShadow shadowMapTexture3;

varying vec4 shadowMapCoord1;
varying vec4 shadowMapCoord2;
varying vec4 shadowMapCoord3;

varying vec4 shadowMapViewPos;

bool DepthValidateRange(vec4 coord){
	return all(lessThanEqual(abs(coord.xy-vec2(0.5)), vec2(0.5)));
}

float VisibilityOfSunLight_Model() {
	
	if(/*DepthValidateRange(shadowMapCoord1)*/ shadowMapViewPos.z > -12.){
		vec4 scoord = shadowMapCoord1.xyzw;
		float v = shadow2D(shadowMapTexture1, scoord.xyz).x;
		return v;
	}else if(/*DepthValidateRange(shadowMapCoord2)*/ shadowMapViewPos.z > -40.){
		vec4 scoord = shadowMapCoord2.xyzw;
		float v = shadow2D(shadowMapTexture2, scoord.xyz).x;
		return v;
	}else {
		vec4 scoord = shadowMapCoord3.xyzw;
		float v = shadow2D(shadowMapTexture3, scoord.xyz).x;
		return v;
	}
}


void VisibilityOfSunLight_Model_Debug() {
	
	if(DepthValidateRange(shadowMapCoord1)){
		vec4 scoord = shadowMapCoord1.xyzw;
		float v = shadow2D(shadowMapTexture1, scoord.xyz).x;
		gl_FragColor = vec4(v, 0., 0., 1.);
	}else if(DepthValidateRange(shadowMapCoord2)){
		vec4 scoord = shadowMapCoord2.xyzw;
		float v = shadow2D(shadowMapTexture2, scoord.xyz).x;
		gl_FragColor = vec4(0., v, 0., 1.);
	}else {
		vec4 scoord = shadowMapCoord3.xyzw;
		float v = shadow2D(shadowMapTexture3, scoord.xyz).x;
		gl_FragColor = vec4(0., 0., v, 1.);
	}
}