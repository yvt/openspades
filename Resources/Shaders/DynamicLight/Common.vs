
uniform vec3 dynamicLightOrigin;
uniform mat4 dynamicLightSpotMatrix;

void PrepareForShadow_Map(vec3 vertexCoord) ;


varying vec3 lightPos;
varying vec3 lightNormal;
varying vec3 lightTexCoord;

void PrepareForDynamicLightNoBump(vec3 vertexCoord, vec3 normal) {
	PrepareForShadow_Map(vertexCoord);
	
	lightPos = dynamicLightOrigin - vertexCoord;
	lightNormal = normal;
	
	// projection
	lightTexCoord = (dynamicLightSpotMatrix * vec4(vertexCoord,1.)).xyw;
					 
}

// TODO: bumpmapping variant (requires tangent vector)
