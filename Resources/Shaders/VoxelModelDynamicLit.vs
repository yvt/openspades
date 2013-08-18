

uniform mat4 projectionViewModelMatrix;
uniform mat4 modelMatrix;
uniform mat4 modelNormalMatrix;
uniform mat4 viewModelMatrix;
uniform vec3 modelOrigin;
uniform float fogDistance;
uniform vec3 sunLightDirection;
uniform vec3 customColor;

// [x, y, z, AO ID]
attribute vec4 positionAttribute;

// [R, G, B, diffuse]
attribute vec4 colorAttribute;

// [x, y, z]
attribute vec3 normalAttribute;

varying vec4 color;
varying vec3 fogDensity;
//varying vec2 detailCoord;

void PrepareForDynamicLightNoBump(vec3 vertexCoord, vec3 normal);

void main() {
	
	vec4 vertexPos = vec4(positionAttribute.xyz, 1.);
	
	vertexPos.xyz += modelOrigin;
	
	gl_Position = projectionViewModelMatrix * vertexPos;
	
	color = colorAttribute;
	
	if(dot(color.xyz, vec3(1.)) < 0.0001){
		color.xyz = customColor;
	}
	
	// linearize
	color.xyz *= color.xyz;
	
	// calculate normal
	vec3 normal = normalAttribute;
	normal = (modelNormalMatrix * vec4(normal, 1.)).xyz;
	normal = normalize(normal);
	
	vec4 viewPos = viewModelMatrix * vertexPos;
	float distance = length(viewPos.xyz) / fogDistance;
	distance = clamp(distance, 0., 1.);
	fogDensity = vec3(distance);
	fogDensity = pow(fogDensity, vec3(1., .9, .7));
	fogDensity *= fogDensity;
	
	PrepareForDynamicLightNoBump((modelMatrix * vertexPos).xyz, normal);
}

