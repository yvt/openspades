

attribute vec2 positionAttribute;
attribute vec4 colorAttribute;

uniform vec3 viewOrigin;
uniform vec3 viewAxisUp, viewAxisSide, viewAxisFront;
uniform vec2 fov;

varying vec2 texCoord;
varying vec3 viewTan;
varying vec3 viewDir;
varying vec3 shadowOrigin;
varying vec3 shadowRayDirection;

vec3 transformToShadow(vec3 v) {
	v.y -= v.z;
	v *= vec3(1., 1., 1. / 255.);
	return v;
}

void main() {
	
	vec2 pos = positionAttribute;
	
	vec2 scrPos = pos * 2. - 1.;
	
	gl_Position = vec4(scrPos, 0.5, 1.);
	
	texCoord = pos;
	viewTan.xy = mix(-fov, fov, pos);
	viewTan.z = 1.;
	
	shadowOrigin = transformToShadow(viewOrigin);
	
	viewDir = viewAxisUp * viewTan.y;
	viewDir += viewAxisSide * viewTan.x;
	viewDir += viewAxisFront;
	
	shadowRayDirection = transformToShadow(viewDir);
	
}

