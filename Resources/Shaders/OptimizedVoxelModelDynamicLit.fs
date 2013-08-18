

varying vec2 textureCoord;
//varying vec2 detailCoord;
varying vec3 fogDensity;

uniform sampler2D modelTexture;
uniform vec3 customColor;
//uniform sampler2D detailTexture;

vec3 EvaluateDynamicLightNoBump();

void main() {
	
	vec4 texData = texture2D(modelTexture, textureCoord.xy);
	
	// model color
	gl_FragColor = vec4(texData.xyz, 1.);
	if(dot(gl_FragColor.xyz, vec3(1.)) < 0.0001){
		gl_FragColor.xyz = customColor;
	}
	
	// linearize
	gl_FragColor.xyz *= gl_FragColor.xyz;
	
	// lighting
	vec3 shading = EvaluateDynamicLightNoBump();
	gl_FragColor.xyz *= shading;
	
	gl_FragColor.xyz = mix(gl_FragColor.xyz, vec3(0.), fogDensity);
	gl_FragColor.xyz = sqrt(gl_FragColor.xyz);
	
}

