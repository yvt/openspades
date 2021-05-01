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



uniform vec3 dynamicLightOrigin;
uniform mat4 dynamicLightSpotMatrix;

uniform bool dynamicLightIsLinear;
uniform vec3 dynamicLightLinearDirection;
uniform float dynamicLightLinearLength;

void PrepareForShadow_Map(vec3 vertexCoord) ;


varying vec3 lightPos;
varying vec3 lightNormal;
varying vec3 lightTexCoord;

void PrepareForDynamicLightNoBump(vec3 vertexCoord, vec3 normal) {
	PrepareForShadow_Map(vertexCoord);

	vec3 lightPosition = dynamicLightOrigin;

	if (dynamicLightIsLinear) {
		// Linear light approximation - choose the closest point on the light
		// geometry as the representative light source
		float d = dot((vertexCoord - dynamicLightOrigin), dynamicLightLinearDirection);
		d = clamp(d, 0.0, dynamicLightLinearLength);
		lightPosition += dynamicLightLinearDirection * d;
	}

	lightPos = lightPosition - vertexCoord;
	lightNormal = normal;
	
	// projection
	lightTexCoord = (dynamicLightSpotMatrix * vec4(vertexCoord,1.)).xyw;
					 
}

// TODO: bumpmapping variant (requires tangent vector)
