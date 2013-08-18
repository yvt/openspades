

varying vec4 color;
varying vec2 ambientOcclusionCoord;
//varying vec2 detailCoord;
varying vec3 fogDensity;

uniform sampler2D ambientOcclusionTexture;
uniform sampler2D detailTexture;
uniform vec3 fogColor;

vec3 EvaluateSunLight();
vec3 EvaluateAmbientLight(float detailAmbientOcclusion);

void main() {
	// color is linearized
	gl_FragColor = color;
	gl_FragColor.w = 1.;
	
	vec3 shading = vec3(color.w);
	
	// FIXME: prepare for shadow?
	shading *= EvaluateSunLight();
	
	vec3 ao = texture2D(ambientOcclusionTexture, ambientOcclusionCoord).xyz;
	shading += EvaluateAmbientLight(ao.x);
	
	gl_FragColor.xyz *= shading;
	
	//gl_FragColor.xyz *= texture2D(detailTexture, detailCoord).xyz * 2.;
	
	gl_FragColor.xyz = mix(gl_FragColor.xyz, fogColor, fogDensity);
	gl_FragColor.xyz = sqrt(gl_FragColor.xyz);
	
}

