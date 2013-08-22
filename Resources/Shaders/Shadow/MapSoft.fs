
/*
 * Percentage-Closer Soft Shadow Based
 * Soft Shadowed Map Shadow Generator
 */

uniform sampler2D mapShadowTexture;

varying vec3 mapShadowCoord;

vec3 MapSoft_BlockSample(vec2 sample, float depth,
						 float shiftedDepth) {
	const float factor = 1. / 512.;
	float val = texture2D(mapShadowTexture, sample.xy * factor).w;
	float distance = shiftedDepth - val;
	float weight = step(0., distance);
	weight = clamp(distance*255. - 1., 0., 1.);
	return vec3(distance, 1., step(0., shiftedDepth - val)) * weight;
}

float VisibilityOfSunLight_Map() {
	float depth = mapShadowCoord.z;
	vec2 iPos = (floor(mapShadowCoord.xy));
	vec2 fracPos = mapShadowCoord.xy - iPos.xy; // [0, 1]
	vec2 fracPosHS = fracPos - .5;				// [-0.5, 0.5]
	vec2 fracPosHSAbs = abs(fracPosHS);
	
	// samples from iPos + sampShift.xy * vec2({0|1},{0|1})
	vec2 sampShift = sign(fracPos - vec2(0.5));
	
	// avoid precision error
	vec2 iPosShifted = iPos + vec2(0.1);
	
	// blocker distance estimation
	vec4 distSampPos = vec4(iPosShifted, iPosShifted + sampShift);
	float depthShifted = depth + sampShift.y / 255.;
	vec3 samp1 = MapSoft_BlockSample(distSampPos.xy, depth, depth);
	vec3 samp2 = MapSoft_BlockSample(distSampPos.zy, depth, depth);
	vec3 samp3 = MapSoft_BlockSample(distSampPos.xw, depth, depthShifted);
	vec3 samp4 = MapSoft_BlockSample(distSampPos.zw, depth, depthShifted);
	vec3 distWeighted1 = samp1;
	distWeighted1 = mix(distWeighted1,
						samp2,
						fracPosHSAbs.x);
	
	vec3 distWeighted2 = samp3;
	distWeighted2 = mix(distWeighted2,
						samp4,
						fracPosHSAbs.x);
	
	vec3 distWeighted3 = mix(distWeighted1, distWeighted2,
							 fracPosHSAbs.y);
	
	distWeighted3.x /= distWeighted3.y + 1.e-10;
	
	float distance = distWeighted3.x;
	
	// blurred shadow sampling
	float blur = distance * 4.;
	blur = max(blur, 1.e-10); // avoid zero division
	
	vec2 blurWeight = 0.5 - (0.5 - fracPosHSAbs) / blur;
	blurWeight = max(blurWeight, 0.);
	
	float val1 = mix(samp1.z, samp2.z, blurWeight.x);
	float val2 = mix(samp3.z, samp4.z, blurWeight.x);
	float val = 1. - mix(val1, val2, blurWeight.y);
	
	// --- sharp shadow
	vec4 sharpCol = texture2D(mapShadowTexture, floor(mapShadowCoord.xy) / 512.);
	float sharpVal = sharpCol.w;
	
	// side shadow?
	if(sharpCol.x > .499) {
		sharpVal -= fract(mapShadowCoord.y) / 255.;
	}
	
	float dist = sharpVal - mapShadowCoord.z + 0.001;
	sharpVal = step(0., dist);
	
	float sharpWeight = clamp(4. + dist * 200., 0., 1.);
	sharpVal = mix(1., sharpVal, sharpWeight);
	
	val *= sharpVal;
	
	return val;
}