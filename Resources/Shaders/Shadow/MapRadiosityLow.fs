/*
 Copyright (c) 2013 OpenSpades Developers
 
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

/**** CPU RADIOSITY (FASTER?) *****/

uniform sampler3D ambientShadowTexture;
uniform sampler3D radiosityTextureFlat;
uniform sampler3D radiosityTextureX;
uniform sampler3D radiosityTextureY;
uniform sampler3D radiosityTextureZ;
varying vec3 radiosityTextureCoord;
varying vec3 ambientShadowTextureCoord;
varying vec3 normalVarying;
uniform vec3 ambientColor;

vec3 DecodeRadiosityValue(vec3 val){
	// reverse bias
	val *= 1023. / 1022.;
	val = (val * 2.) - 1.;
	val *= val * sign(val);
	return val;
}

vec3 Radiosity_Map(float detailAmbientOcclusion) {
	vec3 col = DecodeRadiosityValue
	(texture3D(radiosityTextureFlat,
			   radiosityTextureCoord).xyz);
	vec3 normal = normalize(normalVarying);
	col += normal.x * DecodeRadiosityValue
	(texture3D(radiosityTextureX,
			   radiosityTextureCoord).xyz);
	col += normal.y * DecodeRadiosityValue
	(texture3D(radiosityTextureY,
			   radiosityTextureCoord).xyz);
	col += normal.z * DecodeRadiosityValue
	(texture3D(radiosityTextureZ,
			   radiosityTextureCoord).xyz);
	col = max(col, 0.);
	col *= 1.5;
	
	// ambient occlusion
	float amb = texture3D(ambientShadowTexture, ambientShadowTextureCoord).x;
	amb = max(amb, 0.); // by some reason, texture value becomes negative
	
	// method1:
	amb = sqrt(amb * detailAmbientOcclusion);
	// method2:
	//amb = mix(amb, detailAmbientOcclusion, 0.5);
	
	// method3:
	//amb = min(amb, detailAmbientOcclusion);
	
	amb *= .8 - normalVarying.z * .2;
	col += amb * ambientColor;
	
	return col;
}
