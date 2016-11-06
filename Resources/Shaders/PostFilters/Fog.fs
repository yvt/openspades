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


#define USE_COARSE_SHADOWMAP 1
#define DEBUG_TRACECOUNT 0

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
	voxelDistanceFactor *= length(viewDir.xy) / length(viewDir); // remove vertical fog

	float screenDepth = depthAt(texCoord);
	float screenDistance = screenDepth * length(viewTan);
	screenDistance = min(screenDistance, fogDistance);
	float screenVoxelDistance = screenDistance * voxelDistanceFactor;

	const vec2 voxels = vec2(512.);
	const vec2 voxelSize = 1. / voxels;
	float fogDistanceTime = fogDistance * voxelDistanceFactor;
	float zMaxTime = min(screenVoxelDistance, fogDistanceTime);
	float maxTime = zMaxTime;
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

	float time = 0.;
	float total = 0.;

	if(startPos.z > (63. / 255.) && dir.z < 0.) {
		// fog pass for mirror scene
		time = ((63. / 255.) - startPos.z) / dir.z;
		total += fogDensFunc(time);
		startPos += time * dir;
	}

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

	if(ceilTime <= 0.) {
		// camera is above ceil, and
		// ray never goes below ceil
		total = fogDensFunc(zMaxTime);
	}else{
#if USE_COARSE_SHADOWMAP
#if DEBUG_TRACECOUNT
		float traceCount = 0.;
#define TRACE()	traceCount += 1.
#else
#define TRACE()
#endif

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
			TRACE();
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
					TRACE();
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
					float diffDens = fogDensFunc(zMaxTime) - fogDensFunc(time);;
					total += max(diffDens, 0.);
				}
				break;
			}

			coarsePos = nextCoarsePos;
		}

#if DEBUG_TRACECOUNT
		traceCount /= 8.;
		if(traceCount < 1.) {
			gl_FragColor = mix(vec4(0., 0., 0., 1.),
							   vec4(1., 0., 0., 1.),
							   traceCount);
			return;
		}else if(traceCount < 2.) {
			gl_FragColor = mix(vec4(1., 0., 0., 1.),
							   vec4(1., 1., 0., 1.),
							   traceCount - 1.);
			return;
		}else if(traceCount < 3.) {
			gl_FragColor = mix(vec4(1., 1., 0., 1.),
							   vec4(0., 1., 0., 1.),
							   traceCount - 2.);
			return;
		}else if(traceCount < 4.) {
			gl_FragColor = mix(vec4(0., 1., 0., 1.),
							   vec4(0., 1., 1., 1.),
							   traceCount - 3.);
			return;
		}else if(traceCount < 5.) {
			gl_FragColor = mix(vec4(0., 1., 1., 1.),
							   vec4(0., 0., 1., 1.),
							   traceCount - 4.);
			return;
		}else if(traceCount < 6.) {
			gl_FragColor = mix(vec4(0., 0., 1., 1.),
							   vec4(1., 0., 1., 1.),
							   traceCount - 5.);
			return;
		}else if(traceCount < 7.) {
			gl_FragColor = mix(vec4(1., 0., 1., 1.),
							   vec4(1., 1., 1., 1.),
							   traceCount - 6.);
			return;
		}else {
			gl_FragColor = vec4(1., 1., 1., 1.);
			return;
		}
#endif

#else // USE_COARSE_SHADOWMAP

#warning Non-coarse routine doesnt support water reflection
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
					diffDens = fogDensFunc(zMaxTime) - fogDensFunc(time);;
					total += val * max(diffDens, 0.);
				}
				break;
			}
		}
#endif
	}

	total = mix(total, fogDensFunc(zMaxTime), 0.04);
	total /= fogDensFunc(fogDistanceTime);

	// add gradient
	vec3 sunDir = normalize(vec3(0., -1., -1.));
	float bright = dot(sunDir, normalize(viewDir));
	total *= .8 + bright * 0.3;
	bright = exp2(bright * 16. - 15.);
	total *= bright + 1.;

	gl_FragColor = texture2D(colorTexture, texCoord);
#if !LINEAR_FRAMEBUFFER
	gl_FragColor.xyz *= gl_FragColor.xyz; // linearize
#endif

	gl_FragColor.xyz += total * fogColor;

#if !LINEAR_FRAMEBUFFER
	gl_FragColor.xyz = sqrt(gl_FragColor.xyz);
#endif
}

