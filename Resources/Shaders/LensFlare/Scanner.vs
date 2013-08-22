

attribute vec2 positionAttribute;

uniform vec4 scanRange;
uniform vec4 drawRange;

varying vec3 scanPos;
varying vec2 circlePos;

void main() {
	scanPos.xy = mix(scanRange.xy, scanRange.zw,
				  positionAttribute.xy);
	scanPos.z = .9999999;
	gl_Position.xy = mix(drawRange.xy, drawRange.zw,
						 positionAttribute.xy);
	gl_Position.z = 0.5;
	gl_Position.w = 1.;
	
	circlePos = mix(vec2(-1.), vec2(1.), positionAttribute.xy);
}

