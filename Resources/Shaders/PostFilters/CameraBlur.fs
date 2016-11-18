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


uniform sampler2D mainTexture;
uniform sampler2D depthTexture;
uniform float shutterTimeScale;

varying vec2 newCoord;
varying vec3 oldCoord;

vec4 getSample(vec2 coord){
	vec3 color = texture2D(mainTexture, coord).xyz;
#if !LINEAR_FRAMEBUFFER
	color *= color; // linearize
#endif
	
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
	
#if !LINEAR_FRAMEBUFFER
	gl_FragColor.xyz = sqrt(gl_FragColor.xyz);
#endif
	
	gl_FragColor.w = 1.;
}

