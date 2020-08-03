uniform vec3 fogColor;
uniform vec3 outlineColor;

varying vec3 fogDensity;

void main() {

	gl_FragColor.rgb = mix(outlineColor, fogColor, fogDensity);
	gl_FragColor.a = 1.0;
	
#if !LINEAR_FRAMEBUFFER
	// gamma correct
	gl_FragColor.xyz = sqrt(gl_FragColor.xyz);
#endif
}

