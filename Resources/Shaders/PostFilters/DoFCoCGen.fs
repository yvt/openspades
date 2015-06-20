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


uniform sampler2D depthTexture;
uniform vec2 zNearFar;
uniform vec2 pixelShift;
uniform float depthScale;

uniform float maxVignetteBlur;
uniform vec2 vignetteScale;
uniform float globalBlur;
uniform float nearBlur;
uniform float farBlur;

varying vec2 texCoord;


float decodeDepth(float w, float near, float far){
	return far * near / mix(far, near, w);
}

float depthAt(vec2 pt){
	float w = texture2D(depthTexture, pt).x;
	return decodeDepth(w, zNearFar.x, zNearFar.y);
}

float CoCAt(vec2 pt) {
	float depth = depthAt(pt);
	float blur = 1. - depth * depthScale;
	return blur * (blur > 0. ? nearBlur : farBlur);
}

void main() {
	
	float val = 0.;
	
	val += CoCAt(texCoord);
	val += CoCAt(texCoord + pixelShift * vec2(1., 0.));
	val += CoCAt(texCoord + pixelShift * vec2(2., 0.));
	val += CoCAt(texCoord + pixelShift * vec2(3., 0.));
	val += CoCAt(texCoord + pixelShift * vec2(0., 1.));
	val += CoCAt(texCoord + pixelShift * vec2(1., 1.));
	val += CoCAt(texCoord + pixelShift * vec2(2., 1.));
	val += CoCAt(texCoord + pixelShift * vec2(3., 1.));
	val += CoCAt(texCoord + pixelShift * vec2(0., 2.));
	val += CoCAt(texCoord + pixelShift * vec2(1., 2.));
	val += CoCAt(texCoord + pixelShift * vec2(2., 2.));
	val += CoCAt(texCoord + pixelShift * vec2(3., 2.));
	val += CoCAt(texCoord + pixelShift * vec2(0., 3.));
	val += CoCAt(texCoord + pixelShift * vec2(1., 3.));
	val += CoCAt(texCoord + pixelShift * vec2(2., 3.));
	val += CoCAt(texCoord + pixelShift * vec2(3., 3.));
	
	gl_FragColor.x = val * (1. / 16.);
	
	float sq = length((texCoord - 0.5) * vignetteScale);
	float sq2 = sq * sq * maxVignetteBlur;
	gl_FragColor.x += sq2;
	
	// don't blur the center
	float scl = min(1., sq * 10.);
	gl_FragColor.x *= scl;
	
	gl_FragColor.x = min(gl_FragColor.x + globalBlur, 1.);
}

