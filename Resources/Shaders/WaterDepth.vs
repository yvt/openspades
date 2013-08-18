

uniform mat4 projectionViewModelMatrix;
// [x, y]
attribute vec2 positionAttribute;


void main() {
	
	vec4 vertexPos = vec4(positionAttribute.xy, 0., 1.);
	
	gl_Position = projectionViewModelMatrix * vertexPos;
	
}

