

uniform mat4 modelMatrix;
uniform mat4 modelNormalMatrix;
uniform vec3 modelOrigin;

// [x, y, z, AO ID]
attribute vec4 positionAttribute;

// [x, y, z]
attribute vec3 normalAttribute;

varying vec4 color;
varying vec3 fogDensity;
//varying vec2 detailCoord;

void PrepareForShadowMapRender(vec3 position, vec3 normal);

void main() {
	
	vec4 vertexPos = vec4(positionAttribute.xyz, 1.);
	
	vertexPos.xyz += modelOrigin;
	
	// direct sunlight
	vec3 normal = normalAttribute;
	normal = (modelNormalMatrix * vec4(normal, 1.)).xyz;
	normal = normalize(normal);
	
	PrepareForShadowMapRender((modelMatrix * vertexPos).xyz, normal);
}

