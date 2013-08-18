// Common code for dynamic light rendering


// -- shadowing

float VisibilityOfLight_Map();

float VisibilityOfLight() {
	return VisibilityOfLight_Map();
}

float EvaluateDynamicLightShadow(){
	return VisibilityOfLight();
}

// -- lighting (without bumpmapping)

uniform vec3 dynamicLightColor;
uniform float dynamicLightRadius;
uniform float dynamicLightRadiusInversed;
uniform sampler2D dynamicLightProjectionTexture;

varying vec3 lightPos;
varying vec3 lightNormal;
varying vec3 lightTexCoord;

vec3 EvaluateDynamicLightNoBump() {
	if(lightTexCoord.z < 0.) discard;
	
	// diffuse lighting
	float intensity = dot(normalize(lightPos), normalize(lightNormal));
	if(intensity < 0.) discard;
	
	// attenuation
	float distance = length(lightPos);
	if(distance >= dynamicLightRadius) discard;
	distance *= dynamicLightRadiusInversed;
	distance = max(1. - distance, 0.);
	float att = distance * distance;
	
	// apply attenuation
	intensity *= att;
	
	// projection
	// if(lightTexCoord.w < 0.) discard; -- done earlier
	vec3 texValue = texture2DProj(dynamicLightProjectionTexture, lightTexCoord).xyz;
	
	// TODO: specular lighting?
	return dynamicLightColor * intensity * EvaluateDynamicLightShadow() * texValue;
}

// TODO: bumpmapping variant (requires tangent vector)

