
uniform mat4 projectionViewMatrix;

attribute vec4 positionAttribute;
attribute vec4 colorAttribute;

varying vec4 color;

void main() {
	
	gl_Position = projectionViewMatrix * positionAttribute;
	
	color = colorAttribute;
	
}

