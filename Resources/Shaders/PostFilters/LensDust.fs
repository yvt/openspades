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


uniform sampler2D blurTexture1;
uniform sampler2D blurTexture2;
uniform sampler2D blurTexture3;
uniform sampler2D blurTexture4;
uniform sampler2D dustTexture;
uniform sampler2D inputTexture;
uniform sampler2D noiseTexture;

varying vec2 texCoord;
varying vec4 dustTexCoord1;
varying vec4 dustTexCoord2;
varying vec4 noiseTexCoord;

void main() {
	// dust filter texture
	vec3 dust1 = texture2D(dustTexture, dustTexCoord1.xy).xyz;
	vec3 dust2 = texture2D(dustTexture, dustTexCoord1.zw).xyz;
	vec3 dust3 = texture2D(dustTexture, dustTexCoord2.xy).xyz;
	vec3 dust4 = texture2D(dustTexture, dustTexCoord2.zw).xyz;
	
	// linearize
	dust1 *= dust1;
	dust2 *= dust2;
	dust3 *= dust3;
	dust4 *= dust4;
	
	// blurred texture (already linearized?)
	vec3 blur1 = texture2D(blurTexture1, texCoord).xyz;
	vec3 blur2 = texture2D(blurTexture2, texCoord).xyz;
	vec3 blur3 = texture2D(blurTexture3, texCoord).xyz;
	vec3 blur4 = texture2D(blurTexture4, texCoord).xyz;
	
	vec3 sum = dust1 * blur1;
	sum += dust2 * blur2;
	sum += dust3 * blur3;
	sum += dust4 * blur4;
	
	vec3 final = texture2D(inputTexture, texCoord).xyz;
#if !LINEAR_FRAMEBUFFER
	final *= final;
#endif
	
	final *= 0.95;
	final += sum * 0.4;
	
	// add grain
	float grain = texture2D(noiseTexture, noiseTexCoord.xy).x;
	grain += texture2D(noiseTexture, noiseTexCoord.zw).x;
	grain = fract(grain) - 0.5;
	final += grain * 0.003;
	
	// non-linearize
#if !LINEAR_FRAMEBUFFER
	final = sqrt(max(final, 0.));
#endif
	
	gl_FragColor = vec4(final, 1.);
}

