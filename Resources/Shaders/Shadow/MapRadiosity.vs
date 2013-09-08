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

/**** CPU RADIOSITY (FASTER?) *****/

varying vec3 radiosityTextureCoord;
varying vec3 ambientShadowTextureCoord;
varying vec3 normalVarying;

void PrepareForRadiosity_Map(vec3 vertexCoord, vec3 normal) {
	
	radiosityTextureCoord = (vertexCoord + vec3(0., 0., 0.)) / vec3(512., 512., 64.);
	ambientShadowTextureCoord = (vertexCoord + vec3(0.5, 0.5, 1.5)) / vec3(512., 512., 65.);
	
	normalVarying = normal;
}
