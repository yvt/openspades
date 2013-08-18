

varying vec4 color;
varying vec2 ambientOcclusionCoord;
varying vec2 detailCoord;
varying vec3 fogDensity;

uniform sampler2D ambientOcclusionTexture;
uniform sampler2D detailTexture;
uniform vec3 fogColor;

vec3 EvaluateSunLight();
vec3 EvaluateAmbientLight(float detailAmbientOcclusion);
//void VisibilityOfSunLight_Model_Debug();

void main() {
	// color is linear
	gl_FragColor = vec4(color.xyz, 1.);
	
	vec3 shading = vec3(color.w);
	shading *= EvaluateSunLight();
	
	float ao = texture2D(ambientOcclusionTexture, ambientOcclusionCoord).x;
	
	shading += EvaluateAmbientLight(ao);
	
	// apply diffuse shading
	gl_FragColor.xyz *= shading;
	
	// apply fog
	gl_FragColor.xyz = mix(gl_FragColor.xyz, fogColor, fogDensity);
	
	// gamma correct
	gl_FragColor.xyz = sqrt(gl_FragColor.xyz);
}

