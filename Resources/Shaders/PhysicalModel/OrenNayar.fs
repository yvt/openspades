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

// http://en.wikipedia.org/wiki/Oren–Nayar_reflectance_model
float OrenNayar(float sigma, float dotLight, float dotEye) {
	if(dotLight < 0.)
		return 0.;
	
	float sigma2 = sigma * sigma;
	float A = 1. - 0.5 * sigma2 / (sigma2 + 0.33);
	float B = 0.45 * sigma2 / (sigma2 + 0.09);
	float scale = 1. / A;
	float scaledB = B * scale;
	
	vec2 dotLightEye = vec2(dotLight, dotEye);
	vec2 sinLightEye = sqrt(1. - dotLightEye * dotLightEye);
	float alphaSin = max(sinLightEye.x, sinLightEye.y);
	float betaCos = max(dotLight, dotEye);
	
	// (tan x)^2 + 1 = 1 / (cosx)^2
	// tan x = sqrt(1 / (cosx)^2 - 1)
	float betaCos2 = betaCos * betaCos;
	
	// rsq optimization; 1/x-1 = (1-x)/x
	float betaTan = 1. / sqrt(betaCos2 / (1. - betaCos2)); //sqrt(1. / betaCos2 - 1.);
	
	// cos(dotLight - dotEye)
	vec4 vecs = vec4(dotLightEye, sinLightEye);
	float diffCos = dot(vecs.xz, vecs.yw);
	
	// compute
	return dotLight * (1. + scaledB * diffCos * alphaSin * betaTan);
}


