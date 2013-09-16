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


uniform sampler2D mapShadowTexture;

varying vec3 mapShadowCoord;

float VisibilityOfSunLight_Map() {
	const vec2 mapSize = vec2(512.); // TODO: variable?
	vec2 mapSizeInv = 1. / mapSize;
	
	vec2 shadowMapPixCoord = mapShadowCoord.xy * mapSize;
	vec2 shadowMapPixInt = floor(shadowMapPixCoord);
	vec2 shadowMapPixFract = fract(shadowMapPixCoord);
	vec2 shadowMapBlend = shadowMapPixFract - 0.5;
	vec2 shadowMapNeighborSign = sign(shadowMapBlend);
	shadowMapBlend = abs(shadowMapBlend);
	
	vec4 shadowMapSampleCoords = shadowMapPixInt.xyxy;
	shadowMapSampleCoords.zw += shadowMapNeighborSign;
	shadowMapSampleCoords *= mapSizeInv.xyxy;
	
	vec4 samples = vec4(
		texture2D(mapShadowTexture, shadowMapSampleCoords.xy).w,
		texture2D(mapShadowTexture, shadowMapSampleCoords.zy).w,
		texture2D(mapShadowTexture, shadowMapSampleCoords.xw).w,
		texture2D(mapShadowTexture, shadowMapSampleCoords.zw).w
	);
	
	vec2 average1 = mix(samples.xz, samples.yw, shadowMapBlend.x);
	float average = mix(average1.x, average1.y, shadowMapBlend.y);
	
	if(average > mapShadowCoord.z)
		return 1.;
	
	vec4 samples2 = samples * samples;
	vec2 average2 = mix(samples2.xz, samples2.yw, shadowMapBlend.x);
	float averageP = mix(average2.x, average2.y, shadowMapBlend.y);
	
	float variance = averageP - average * average;
	variance = max(variance, 0.000000001);
	
	float val = mapShadowCoord.z - average;
	val *= val;
	val = variance / (variance + val);
	
	return val;
}
