

varying vec4 color;
varying vec2 detailCoord;
varying vec3 fogDensity;

uniform sampler2D detailTexture;
uniform vec3 fogColor;

vec3 EvaluateDynamicLightNoBump();

void main() {
	// color is linearized
	gl_FragColor = vec4(color.xyz, 1.);
	
	vec3 shading = EvaluateDynamicLightNoBump();
	
	gl_FragColor.xyz *= shading;
	
	// fog fade
	gl_FragColor.xyz = mix(gl_FragColor.xyz, vec3(0.), fogDensity);
	gl_FragColor.xyz = sqrt(gl_FragColor.xyz);
	
}

