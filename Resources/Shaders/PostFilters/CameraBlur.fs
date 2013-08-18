
uniform sampler2D texture;
uniform sampler2D depthTexture;
uniform float shutterTimeScale;

varying vec2 newCoord;
varying vec3 oldCoord;

// linearize gamma
vec3 filter(vec3 col){
	return col * col;
}

vec4 getSample(vec2 coord){
	vec3 color = texture2D(texture, coord).xyz;
	color *= color; // linearize
	
	float depth = texture2D(depthTexture, coord).x;
	float weight = depth*depth; // [0,0.1] is for view weapon
	weight = min(weight, 1.) + 0.0001;
	
	return vec4(color * weight, weight);
}

void main() {
	vec2 nextCoord = newCoord;
	vec2 prevCoord = oldCoord.xy / oldCoord.z;
	vec2 coord;
	
	vec4 sum;
	
	coord = mix(nextCoord, prevCoord, 0.);
	sum = getSample(coord);
	
	// use latest sample's weight for camera blur strength
	float allWeight = sum.w;
	vec4 sum2;
	
	sum /= sum.w;
	
	coord = mix(nextCoord, prevCoord, shutterTimeScale * 0.2);
	sum2 = getSample(coord);
	
	coord = mix(nextCoord, prevCoord, shutterTimeScale * 0.4);
	sum2 += getSample(coord);
	
	coord = mix(nextCoord, prevCoord, shutterTimeScale * 0.6);
	sum2 += getSample(coord);
	
	coord = mix(nextCoord, prevCoord, shutterTimeScale * 0.8);
	sum2 += getSample(coord);
	
	sum += sum2 * allWeight;
	
	gl_FragColor.xyz = sum.xyz / sum.w;
	
	gl_FragColor.xyz = sqrt(gl_FragColor.xyz);
	
	gl_FragColor.w = 1.;
}

