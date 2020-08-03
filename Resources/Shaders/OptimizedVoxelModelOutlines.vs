uniform mat4 projectionViewModelMatrix;
uniform mat4 viewModelMatrix;
uniform mat4 modelMatrix;
uniform vec3 modelOrigin;
uniform vec3 viewOriginVector;

// [x, y, z]
attribute vec3 positionAttribute;

varying vec3 fogDensity;

vec4 FogDensity(float poweredLength);

void main() {

	vec4 vertexPos = vec4(positionAttribute.xyz, 1.);
	vertexPos.xyz += modelOrigin;
	gl_Position = projectionViewModelMatrix * vertexPos;
	
	vec2 horzRelativePos = (modelMatrix * vertexPos).xy - viewOriginVector.xy;
	float horzDistance = dot(horzRelativePos, horzRelativePos);
	fogDensity = FogDensity(horzDistance).xyz;
}

