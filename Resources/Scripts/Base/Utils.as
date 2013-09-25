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
 
 namespace spades {
 	// AngelScript doesn't seem to support user-defined template functions...
 	
 	uint Min(uint a, uint b) {
 		return (a < b) ? a : b;
 	}
 	uint Max(uint a, uint b) {
 		return (a > b) ? a : b;
 	}
 	
 	int Min(int a, int b) {
 		return (a < b) ? a : b;
 	}
 	int Max(int a, int b) {
 		return (a > b) ? a : b;
 	}
 	
 	float Min(float a, float b) {
 		return (a < b) ? a : b;
 	}
 	float Max(float a, float b) {
 		return (a > b) ? a : b;
 	}
 	
 	float Clamp(float val, float lower, float upper) {
 		return Min(Max(val, lower), upper);
 	}
 	
 	int Clamp(int val, int lower, int upper) {
 		return Min(Max(val, lower), upper);
 	}
 	
 	uint Clamp(uint val, uint lower, uint upper) {
 		return Min(Max(val, lower), upper);
 	}
 	
 	
 }
 