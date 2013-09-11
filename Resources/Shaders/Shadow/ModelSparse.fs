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


uniform sampler2DShadow shadowMapTexture1;
uniform sampler2D shadowMapTexture2;

uniform float pagetableSize;
uniform float pagetableSizeInv;
uniform float minLod;
uniform float shadowMapSizeInv;

varying vec4 shadowMapCoord;


float VisibilityOfSunLight_Model() {
	vec4 scoord = shadowMapCoord.xyzw;
	
	vec2 pagetableFract = fract(scoord.xy * pagetableSize);
	vec2 pagetableInt = floor(scoord.xy * pagetableSize);
	vec4 mapData = texture2D(shadowMapTexture2, pagetableInt * pagetableSizeInv);
	
	// decode map size
	if(mapData.w > .99) return 1.; // no shadow map
	mapData *= 255.01;
	
	vec2 physCoord = mapData.xy;
	physCoord = floor(physCoord);
	
	physCoord.x += floor(mod(mapData.z, 16.)) * 256.;
	physCoord.y += floor(mapData.z / 16.) * 256.;
	
	//float lod = mapData.w * minLod;
	float lod = exp2(floor(mapData.w));
	physCoord.xy += lod * pagetableFract;
	
	physCoord.xy *= shadowMapSizeInv;
	
	vec3 physCoord2 = vec3(physCoord, scoord.z);
	
	float v = shadow2D(shadowMapTexture1, physCoord2).x;
	return v;
}

