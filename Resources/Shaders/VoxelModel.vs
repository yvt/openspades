

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

// [u, v]
attribute vec2 textureCoordAttribute;

// [R, G, B, diffuse]
attribute vec4 colorAttribute;

// [x, y, z]
attribute vec3 normalAttribute;

varying vec2 ambientOcclusionCoord;
varying vec4 color;
varying vec3 fogDensity;
//varying vec2 detailCoord;

void PrepareForShadow(vec3 worldOrigin, vec3 normal);

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
	
	// direct sunlight
	vec3 normal = normalAttribute;
	normal = (modelNormalMatrix * vec4(normal, 1.)).xyz;
	normal = normalize(normal);
	float sunlight = dot(normal, sunLightDirection);
	sunlight = max(sunlight, 0.);
	color.w *= sunlight;
	
	// ambient occlusion
	float aoID = positionAttribute.w / 256.;
	
	float aoY = aoID * 16.;
	float aoX = fract(aoY);
	aoY = floor(aoY) / 16.;
	
	ambientOcclusionCoord = vec2(aoX, aoY);
	ambientOcclusionCoord += textureCoordAttribute.xy * (15. / 256.);
	ambientOcclusionCoord += .5 / 256.;
	
	vec4 viewPos = viewModelMatrix * vertexPos;
	float distance = length(viewPos.xyz) / fogDistance;
	distance = clamp(distance, 0., 1.);
	fogDensity = vec3(distance);
	fogDensity = pow(fogDensity, vec3(1., .9, .7));
	fogDensity *= fogDensity;
	
	PrepareForShadow((modelMatrix * vertexPos).xyz, normal);
}

