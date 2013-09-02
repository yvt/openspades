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

uniform sampler2D waveTexture;

float DisplaceWater(vec2 worldPos){
	
	vec4 waveCoord = worldPos.xyxy * vec4(vec2(0.08), vec2(0.15704))
	+ vec4(0., 0., 0.754, 0.1315);
	
	vec2 waveCoord2 = worldPos.xy * 0.02344 + vec2(.154, .7315);
	
	
	vec4 wave = texture2DLod(waveTexture, waveCoord.xy, 0.).xyzw;
	float disp = mix(-.9,1., wave.w);
	
	vec4 wave2 = texture2DLod(waveTexture, waveCoord.zw, 0.).xyzw;
	disp += mix(-1.,1., wave2.w);
	
	wave2 = texture2DLod(waveTexture, waveCoord2.xy, 0.).xyzw;
	disp += mix(-1.,1., wave2.w);
	

	return disp * .6;
}

