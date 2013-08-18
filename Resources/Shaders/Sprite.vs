
uniform mat4 projectionViewMatrix;
uniform mat4 viewMatrix;
uniform vec3 rightVector;
uniform vec3 upVector;

uniform float fogDistance;

attribute vec4 positionAttribute;
attribute vec3 spritePosAttribute;
attribute vec4 colorAttribute;

varying vec4 color;
varying vec2 texCoord;
varying vec4 fogDensity;

void main() {
	vec3 pos = positionAttribute.xyz;
	float radius = positionAttribute.w;
	
	vec3 right = rightVector * radius;
	vec3 up = upVector * radius;
	
	float angle = spritePosAttribute.z;
	float c = cos(angle), s = sin(angle);
	vec2 sprP;
	sprP.x = dot(spritePosAttribute.xy, vec2(c, -s));
	sprP.y = dot(spritePosAttribute.xy, vec2(s, c));
	sprP *= radius;
	pos += right * sprP.x;
	pos += up * sprP.y;
	
	gl_Position = projectionViewMatrix * vec4(pos,1.);
	
	color = colorAttribute;
	
	texCoord = spritePosAttribute.xy * .5 + .5;
	
	// fog.
	// cannot gamma correct because sprite may be
	// alpha-blended.
	vec4 viewPos = viewMatrix * vec4(pos,1.);
	float distance = length(viewPos.xyz) / fogDistance;
	distance = clamp(distance, 0., 1.);
	fogDensity = vec4(distance);
	fogDensity = pow(fogDensity, vec4(1., .9, .7, 1.));
	fogDensity *= fogDensity; // FIXME
}

