

uniform mat4 projectionViewModelMatrix;
uniform mat4 modelMatrix;
uniform mat4 viewModelMatrix;
uniform vec3 viewOrigin;
uniform float fogDistance;

// [x, y]
attribute vec2 positionAttribute;

varying vec3 fogDensity;
varying vec3 screenPosition;
varying vec3 viewPosition;
varying vec3 worldPosition;

//varying vec2 detailCoord;

void PrepareForShadow(vec3 worldOrigin, vec3 normal);

void main() {
	
	vec4 vertexPos = vec4(positionAttribute.xy, 0., 1.);
	
	gl_Position = projectionViewModelMatrix * vertexPos;
	screenPosition = gl_Position.xyw;
	screenPosition.xy = (screenPosition.xy + screenPosition.z) * .5;
		
	vec4 viewPos = viewModelMatrix * vertexPos;
	float distance = length(viewPos.xyz) / fogDistance;
	distance = clamp(distance, 0., 1.);
	fogDensity = vec3(distance);
	fogDensity = pow(fogDensity, vec3(1., .9, .7));
	fogDensity *= fogDensity;
	
	viewPosition = viewPos.xyz;
	
	worldPosition = (modelMatrix * vertexPos).xyz;
	
	PrepareForShadow((modelMatrix * vertexPos).xyz, vec3(0., 0., -1.));
}

