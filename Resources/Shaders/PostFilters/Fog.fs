
uniform sampler2D colorTexture;
uniform sampler2D depthTexture;
uniform sampler2D shadowMapTexture;
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
	//dir /= length(dir.xy);
	dir = normalize(dir);
	
	if(dir.z < -0.000001) {
		ceilTime = -startPos.z / dir.z - 0.0001;
		maxTime = min(maxTime, ceilTime);
	}
	
	vec2 dirSign = sign(dir.xy);
	vec2 dirSign2 = dirSign * .5 + .5;
	
	vec3 pos = startPos;
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

