
uniform sampler2DShadow depthTexture;

varying vec3 scanPos;
varying vec2 circlePos;

uniform float radius;

void main() {
	float val = shadow2D(depthTexture, scanPos).x;
	
	// circle trim
	float rad = length(circlePos);
	rad *= radius;
	rad = max(radius - 1. - rad, 0.);
	val *= rad;
	
	gl_FragColor = vec4(vec3(val), 1.);
}

