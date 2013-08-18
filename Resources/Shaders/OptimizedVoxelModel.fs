

varying vec4 textureCoord;
//varying vec2 detailCoord;
varying vec3 fogDensity;
varying float flatShading;

uniform sampler2D ambientOcclusionTexture;
uniform sampler2D modelTexture;
uniform vec3 fogColor;
uniform vec3 customColor;

vec3 EvaluateSunLight();
vec3 EvaluateAmbientLight(float detailAmbientOcclusion);

void main() {
	vec4 texData = texture2D(modelTexture, textureCoord.xy);
	
	// model color
	gl_FragColor = vec4(texData.xyz, 1.);
	if(dot(gl_FragColor.xyz, vec3(1.)) < 0.0001){
		gl_FragColor.xyz = customColor;
	}
	
	// ambient occlusion
	float aoID = texData.w * (255. / 256.);
	
	float aoY = aoID * 16.;
	float aoX = fract(aoY);
	aoY = floor(aoY) / 16.;
	
	vec2 ambientOcclusionCoord = vec2(aoX, aoY);
	ambientOcclusionCoord += fract(textureCoord.zw) *
		(15. / 256.);
	ambientOcclusionCoord += .5 / 256.;
	
	// linearize
	gl_FragColor.xyz *= gl_FragColor.xyz;
	
	// shading
	vec3 shading = vec3(flatShading);
	
	shading *= EvaluateSunLight();
	
	vec3 ao = texture2D(ambientOcclusionTexture, ambientOcclusionCoord).xyz;
	shading += EvaluateAmbientLight(ao.x);
	
	gl_FragColor.xyz *= shading;
	
	gl_FragColor.xyz = mix(gl_FragColor.xyz, fogColor, fogDensity);
	gl_FragColor.xyz = sqrt(gl_FragColor.xyz);
	
}

