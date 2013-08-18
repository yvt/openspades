
uniform sampler2D texture;

varying vec3 angleTan;
varying vec4 texCoord1;
varying vec4 texCoord2;
varying vec4 texCoord3;
varying vec2 texCoord4;

// linearize gamma
vec3 filter(vec3 col){
	return col * col;
}

void main() {
	vec3 sum = vec3(0.), val;
	
#if 0
	val = vec3(0.0, 0.2, 1.0);
	sum += val;
	val *= filter(texture2D(texture, texCoord1.xy).xyz);
	gl_FragColor.xyz = val;
	
	val = vec3(0.0, 0.5, 1.0);
	sum += val;
	val *= filter(texture2D(texture, texCoord1.zw).xyz);
	gl_FragColor.xyz += val;
	
	val = vec3(0.0, 1.0, 0.0);
	sum += val;
	val *= filter(texture2D(texture, texCoord2.xy).xyz);
	gl_FragColor.xyz += val;
	
	val = vec3(1.0, 0.5, 0.0);
	sum += val;
	val *= filter(texture2D(texture, texCoord2.zw).xyz);
	gl_FragColor.xyz += val;
	
	val = vec3(1.0, 0.4, 0.0);
	sum += val;
	val *= filter(texture2D(texture, texCoord3.xy).xyz);
	gl_FragColor.xyz += val;
	
	val = vec3(1.0, 0.3, 1.0);
	sum += val;
	val *= filter(texture2D(texture, texCoord3.zw).xyz);
	gl_FragColor.xyz += val;
	
	gl_FragColor.xyz *= 1. / sum;
	gl_FragColor.xyz = sqrt(gl_FragColor.xyz);
#else
	gl_FragColor = texture2D(texture, texCoord4);
	gl_FragColor.w = 1.;
#endif
	
	// calc brightness (cos^4)
	// note that this is gamma corrected
	float tanValue = length(angleTan.xy);
	float brightness = 1. / (1. + tanValue * tanValue);
	brightness *= angleTan.z;
	brightness = mix(brightness, 1., 0.4); // weaken
	
	gl_FragColor.xyz *= brightness;
	
	gl_FragColor.w = 1.;
}

