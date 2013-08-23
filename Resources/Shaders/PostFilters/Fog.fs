
#define USE_COARSE_SHADOWMAP 1

uniform sampler2D colorTexture;
uniform sampler2D depthTexture;
uniform sampler2D shadowMapTexture;
#if USE_COARSE_SHADOWMAP
uniform sampler2D coarseShadowMapTexture;
#endif
uniform vec2 zNearFar;

uniform vec3 fogColor;
uniform float fogDistance;

varying vec2 texCoord;
varying vec3 viewTan;
varying vec3 viewDir;
varying vec3 shadowOrigin;
varying vec3 shadowRayDirection;


float decodeDepth(float w, float near, float far){
	return far * near / mix(far, near, w);
}

float depthAt(vec2 pt){
	float w = texture2D(depthTexture, pt).x;
	return decodeDepth(w, zNearFar.x, zNearFar.y);
}

float fogDensFunc(float time) {
	return time;// * time;
}

void main() {
	
	// raytrace
	
	float voxelDistanceFactor = length(shadowRayDirection) /
	length(viewTan);
	
	float screenDepth = depthAt(texCoord);
	float screenDistance = screenDepth * length(viewTan);
	screenDistance = min(screenDistance, fogDistance);
	float screenVoxelDistance = screenDistance * voxelDistanceFactor;
	
	const vec2 voxels = vec2(512.);
	const vec2 voxelSize = 1. / voxels;
	float fogDistanceTime = fogDistance * voxelDistanceFactor;
	float maxTime = min(screenVoxelDistance, fogDistanceTime);
	float ceilTime = 10000000000.;
	
	vec3 startPos = shadowOrigin;
	//vec2 dir = vec2(1., 4.);
	vec3 dir = shadowRayDirection.xyz; //dirVec;
	if(length(dir.xy) < .0001) dir.xy = vec2(0.0001);
	if(dir.x == 0.) dir.x = .00001;
	if(dir.y == 0.) dir.y = .00001;
	dir = normalize(dir);
	
	if(dir.z < -0.000001) {
		ceilTime = -startPos.z / dir.z - 0.0001;
		maxTime = min(maxTime, ceilTime);
	}
	
	vec2 dirSign = sign(dir.xy);
	vec2 dirSign2 = dirSign * .5 + .5; // 0, 1
	vec2 dirSign3 = dirSign * .5 - .5; // -1, 0
	
	vec3 pos = startPos + dir * 0.0001;
	vec2 voxelIndex = floor(pos.xy);
	if(pos.xy == voxelIndex) {
		// exact coord doesn't work well
		pos += 0.001;
	}
	vec2 nextVoxelIndex = voxelIndex + dirSign;
	vec2 nextVoxelIndex2 = voxelIndex + dirSign2;
	vec2 timePerVoxel = 1. / dir.xy;
	vec2 timePerVoxelAbs = abs(timePerVoxel);
	vec2 timeToNextVoxel = (nextVoxelIndex2 - pos.xy) * timePerVoxel;
	float time = 0.;
	float total = 0.;
	
#if USE_COARSE_SHADOWMAP
	const float coarseLevel = 8.;
	const float coarseLevelInv = 1. / coarseLevel;
	
	const vec2 coarseVoxels = voxels / coarseLevel;
	const vec2 coarseVoxelSize = voxelSize * coarseLevel;
	
	vec3 coarseDir = dir * vec3(coarseLevelInv, coarseLevelInv, 1.);
	vec3 coarsePos = pos * vec3(coarseLevelInv, coarseLevelInv, 1.);
	vec2 coarseVoxelIndex = floor(coarsePos.xy);
	if(coarsePos.xy == coarseVoxelIndex) {
		// exact coord doesn't work well
		coarsePos += 0.0001;
	}
	vec2 coarseNextVoxelIndex = coarseVoxelIndex + dirSign;
	vec2 coarseNextVoxelIndex2 = coarseVoxelIndex + dirSign2;
	vec2 coarseTimePerVoxel = timePerVoxel * coarseLevel;
	vec2 coarseTimePerVoxelAbs = timePerVoxelAbs * coarseLevel;
	vec2 coarseTimeToNextVoxel = (coarseNextVoxelIndex2 - coarsePos.xy) * coarseTimePerVoxel;
	
	
	for(int i = 0; i < 64; i++) {
		float diffTime;
		vec2 coarseVal = texture2D(coarseShadowMapTexture,
							  coarseVoxelIndex * coarseVoxelSize).xy;
		diffTime = min(coarseTimeToNextVoxel.x, coarseTimeToNextVoxel.y);
		
		float nextTime = min(time + diffTime, maxTime);
		float limitedDiffTime = nextTime - time;
		
		vec2 passingZ = vec2(coarsePos.z);
		passingZ.y += limitedDiffTime * coarseDir.z;
		
		bvec2 stat = bvec2(min(passingZ.x, passingZ.y) > coarseVal.y,
						   max(passingZ.x, passingZ.y) < coarseVal.x);
		
		vec2 oldCoarseVoxelIndex = coarseVoxelIndex;
		vec3 nextCoarsePos = coarsePos + diffTime * coarseDir;
		// advance coarse ray
		if(coarseTimeToNextVoxel.x < coarseTimeToNextVoxel.y) {
			coarseVoxelIndex.x += dirSign.x;
			//nextCoarsePos.x = coarseVoxelIndex.x - dirSign3.x;
			
			coarseTimeToNextVoxel.y -= diffTime;
			coarseTimeToNextVoxel.x = coarseTimePerVoxelAbs.x;
		}else{
			coarseVoxelIndex.y += dirSign.y;
			//nextCoarsePos.y = coarseVoxelIndex.y - dirSign3.y;
			
			coarseTimeToNextVoxel.x -= diffTime;
			coarseTimeToNextVoxel.y = coarseTimePerVoxelAbs.y;
		}
		
		
		if(any(stat)) {
			if(stat.y) {
				// always in light
				float diffDens = fogDensFunc(nextTime) - fogDensFunc(time);
				total += diffDens;
			}
			time = nextTime;
			diffTime = limitedDiffTime;
		}else if(limitedDiffTime < 0.000000001){
			// no advance in this coarse voxel
			time = nextTime;
			diffTime = limitedDiffTime;
		}else{
			// do detail tracing
			
			pos = coarsePos * vec3(coarseLevel, coarseLevel, 1.);
			voxelIndex = floor(pos.xy);
			if(pos.xy == voxelIndex) {
				// exact coord doesn't work well
				pos += 0.001;
			}
			nextVoxelIndex = voxelIndex + dirSign;
			nextVoxelIndex2 = voxelIndex + dirSign2;
			timeToNextVoxel = (nextVoxelIndex2 - pos.xy) * timePerVoxel;
		
			for(int j = 0; j < 64; j++){
				
				float val = texture2D(shadowMapTexture, voxelIndex * voxelSize).w;
				val = step(pos.z, val);
				
				diffTime = min(timeToNextVoxel.x, timeToNextVoxel.y);
				if(timeToNextVoxel.x < timeToNextVoxel.y) {
					voxelIndex.x += dirSign.x;
					
					timeToNextVoxel.y -= diffTime;
					timeToNextVoxel.x = timePerVoxelAbs.x;
				}else{
					voxelIndex.y += dirSign.y;
					
					timeToNextVoxel.x -= diffTime;
					timeToNextVoxel.y = timePerVoxelAbs.y;
				}
				
				pos += dir * diffTime;
				
				float nextTime = min(time + diffTime, maxTime);
				float diffDens = fogDensFunc(nextTime) - fogDensFunc(time);
				diffTime = nextTime - time;
				time = nextTime;
				
				total += val * diffDens;
				
				if(time >= maxTime) {
					break;
				}
				
				// if goes another coarse voxel,
				// return to coarse ray tracing
				if(floor((voxelIndex.xy + .5) * coarseLevelInv) !=
				   oldCoarseVoxelIndex)
					break;
			}
			
			// --- detail tracing ends here
		}
		
		
		if(time >= maxTime) {
			if(nextTime >= ceilTime) {
				float diffDens = fogDensFunc(fogDistanceTime) - fogDensFunc(time);;
				total += max(diffDens, 0.);
			}
			break;
		}
		
		coarsePos = nextCoarsePos;
	}
	
#else
	for(int i = 0; i < 512; i++){
		float diffTime;
		
		float val = texture2D(shadowMapTexture, voxelIndex * voxelSize).w;
		val = step(pos.z, val);
		
		diffTime = min(timeToNextVoxel.x, timeToNextVoxel.y);
		if(timeToNextVoxel.x < timeToNextVoxel.y) {
			//diffTime = timeToNextVoxel.x;
			voxelIndex.x += dirSign.x;
			//pos += dirVoxel * diffTime;
			//pos.x = voxelIndex.x;
			
			timeToNextVoxel.y -= diffTime;
			timeToNextVoxel.x = timePerVoxelAbs.x;
		}else{
			//diffTime = timeToNextVoxel.y;
			voxelIndex.y += dirSign.y;
			//pos += dirVoxel * diffTime;
			//pos.y = voxelIndex.y;
			
			timeToNextVoxel.x -= diffTime;
			timeToNextVoxel.y = timePerVoxelAbs.y;
		}
		
		pos += dir * diffTime;
		
		float nextTime = min(time + diffTime, maxTime);
		float diffDens = fogDensFunc(nextTime) - fogDensFunc(time);
		diffTime = nextTime - time;
		time = nextTime;
		
		
		total += val * diffDens;
		
		if(diffTime <= 0.) {
			if(nextTime >= ceilTime) {
				diffDens = fogDensFunc(fogDistanceTime) - fogDensFunc(time);;
				total += val * max(diffDens, 0.);
			}
			break;
		}
	}
#endif
	
	total /= fogDensFunc(fogDistanceTime);
	
	
	// add gradient
	vec3 sunDir = normalize(vec3(0., -1., -1.));
	float bright = dot(sunDir, normalize(viewDir));
	total *= .8 + bright * 0.3;
	bright = exp2(bright * 16. - 15.);
	total *= bright + 1.;
	
	gl_FragColor = texture2D(colorTexture, texCoord);
	gl_FragColor.xyz *= gl_FragColor.xyz; // linearize
	
	gl_FragColor.xyz += total * fogColor;
	
	gl_FragColor.xyz = sqrt(gl_FragColor.xyz);
	
}

