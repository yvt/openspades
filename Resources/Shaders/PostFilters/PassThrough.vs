

attribute vec2 positionAttribute;
attribute vec4 colorAttribute;

uniform vec4 texCoordRange;

varying vec4 color;
varying vec2 texCoord;

void main() {
	
	vec2 pos = positionAttribute;
	
	vec2 scrPos = pos * 2. - 1.;
	
	gl_Position = vec4(scrPos, 0.5, 1.);
	
	color = colorAttribute;
	texCoord = pos * texCoordRange.zw + texCoordRange.xy;
}

