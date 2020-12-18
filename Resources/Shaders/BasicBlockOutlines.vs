uniform mat4 projectionViewMatrix;
uniform mat4 viewMatrix;
uniform vec3 chunkPosition;
uniform vec3 viewOriginVector;

attribute vec3 positionAttribute;

varying vec3 fogDensity;

vec4 FogDensity(float poweredLength);

void main() {
	
	vec4 vertexPos = vec4(chunkPosition, 1.);
	
	vertexPos.xyz += positionAttribute.xyz;
	
	gl_Position = projectionViewMatrix * vertexPos;

	vec4 viewPos = viewMatrix * vertexPos;
	vec2 horzRelativePos = vertexPos.xy - viewOriginVector.xy;
	float horzDistance = dot(horzRelativePos, horzRelativePos);
	fogDensity = FogDensity(horzDistance).xyz;

}

