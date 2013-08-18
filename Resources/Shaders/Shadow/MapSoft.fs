
/*
 * Percentage-Closer Soft Shadow Based
 * Soft Shadowed Map Shadow Generator
 */

uniform sampler2D mapShadowTexture;

varying vec3 mapShadowCoord;

vec2 MapSoft_BlockSample(vec2 sample, float depth) {
	const float factor = 1. / 512.;
	float val = texture2D(mapShadowTexture, sample.xy * factor).w;
	float distance = depth - val;
	float weight = step(0., distance);
	weight = clamp(distance*100000., 0., 1.);
	return vec2(distance, 1.) * weight;
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
	vec2 samp1 = MapSoft_BlockSample(distSampPos.xy, depth);
	vec2 samp2 = MapSoft_BlockSample(distSampPos.zy, depth);
	vec2 samp3 = MapSoft_BlockSample(distSampPos.xw, depth);
	vec2 samp4 = MapSoft_BlockSample(distSampPos.zw, depth);
	vec2 distWeighted1 = samp1;
	distWeighted1 = mix(distWeighted1,
						samp2,
						fracPosHSAbs.x);
	
	vec2 distWeighted2 = samp3;
	distWeighted2 = mix(distWeighted2,
						samp4,
						fracPosHSAbs.x);
	
	vec2 distWeighted3 = mix(distWeighted1, distWeighted2,
							 fracPosHSAbs.y);
	
	distWeighted3.x /= distWeighted3.y + 1.e-10;
	
	float distance = distWeighted3.x;
	
	// blurred shadow sampling
	float blur = distance * 4.;
	blur = max(blur, 1.e-10); // avoid zero division
	
	vec2 blurWeight = 0.5 - (0.5 - fracPosHSAbs) / blur;
	blurWeight = max(blurWeight, 0.);
	
	float val1 = mix(samp1.y, samp2.y, blurWeight.x);
	float val2 = mix(samp3.y, samp4.y, blurWeight.x);
	float val = 1. - mix(val1, val2, blurWeight.y);
	
	return val;
}