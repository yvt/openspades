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




varying vec3 mapShadowCoord;

void PrepareForShadow_Map(vec3 vertexCoord, vec3 normal) {
	mapShadowCoord = vertexCoord;
	mapShadowCoord.y -= mapShadowCoord.z;
	
	// texture value is normalized unsigned integer
	mapShadowCoord.z /= 255.;
	
	// texture coord is normalized
	// FIXME: variable texture size
	mapShadowCoord.xy /= 512.;
}