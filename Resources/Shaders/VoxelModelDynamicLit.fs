

varying vec4 color;
//varying vec2 detailCoord;
varying vec3 fogDensity;

//uniform sampler2D detailTexture;

vec3 EvaluateDynamicLightNoBump();

void main() {
	// color is normalized
	gl_FragColor = color;
	gl_FragColor.w = 1.;
	
	vec3 shading = EvaluateDynamicLightNoBump();
	
	gl_FragColor.xyz *= shading;
	
	//gl_FragColor.xyz *= texture2D(detailTexture, detailCoord).xyz * 2.;

	gl_FragColor.xyz = mix(gl_FragColor.xyz, vec3(0.), fogDensity);
	gl_FragColor.xyz = sqrt(gl_FragColor.xyz);
	
}

