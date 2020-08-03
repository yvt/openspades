uniform mat4 projectionViewModelMatrix;
uniform vec3 modelOrigin;

attribute vec3 positionAttribute;

void main() {
	
	vec4 vertexPos = vec4(positionAttribute.xyz, 1.0);
	vertexPos.xyz += modelOrigin;
	gl_Position = projectionViewModelMatrix * vertexPos;
}

