uniform mat4 projectionViewModelMatrix;
uniform vec3 modelOrigin;
uniform mat4 modelMatrix;
uniform vec3 viewOriginVector;

attribute vec3 positionAttribute;

varying vec3 fogDensity;

vec4 FogDensity(float poweredLength);

void main() {
	
	vec4 vertexPos = vec4(positionAttribute.xyz, 1.0);
	vertexPos.xyz += modelOrigin;
	gl_Position = projectionViewModelMatrix * vertexPos;
	
	vec2 horzRelativePos = (modelMatrix * vertexPos).xy - viewOriginVector.xy;
	float horzDistance = dot(horzRelativePos, horzRelativePos);
	fogDensity = FogDensity(horzDistance).xyz;
}

