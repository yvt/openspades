

attribute vec2 positionAttribute;
attribute vec4 colorAttribute;
attribute vec2 textureCoordAttribute;

uniform vec2 screenSize;
uniform vec2 textureSize;

varying vec4 color;
varying vec2 texCoord;

void main() {
	
	vec2 pos = positionAttribute;
	
	pos /= screenSize;
	pos = pos * 2. - 1.;
	pos.y = -pos.y;
	
	gl_Position = vec4(pos, 0.5, 1.);
	
	color = colorAttribute;
	texCoord = textureCoordAttribute / textureSize;
}

