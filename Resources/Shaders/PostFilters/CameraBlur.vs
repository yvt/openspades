

attribute vec2 positionAttribute;

uniform mat4 reverseMatrix;

varying vec2 newCoord;
varying vec3 oldCoord;


void main() {
	
	vec2 pos = positionAttribute;
	
	vec2 scrPos = pos * 2. - 1.;
	
	gl_Position = vec4(scrPos, 0.5, 1.);
	
	newCoord = pos;
	
	vec2 cvtCoord = pos - .5;
	oldCoord = (reverseMatrix * vec4(cvtCoord, 0., 1.)).xyz;
	oldCoord.xy += vec2(oldCoord.z) * .5;
}

