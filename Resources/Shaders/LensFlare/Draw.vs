

attribute vec2 positionAttribute;

uniform vec4 drawRange;

varying vec2 texCoord;

void main() {
	gl_Position.xy = mix(drawRange.xy, drawRange.zw,
						 positionAttribute.xy);
	gl_Position.z = 0.5;
	gl_Position.w = 1.;
	
	texCoord = mix(vec2(0.), vec2(1.), positionAttribute.xy);
}

