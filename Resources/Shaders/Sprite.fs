
uniform sampler2D texture;

uniform vec3 fogColor;

varying vec4 color;
varying vec2 texCoord;
varying vec4 fogDensity;

void main() {
	gl_FragColor = texture2D(texture, texCoord);
	gl_FragColor.xyz *= gl_FragColor.w; // premultiplied alpha
	gl_FragColor *= color;
	
	vec4 fogColorPremuld = vec4(fogColor, 1.);
	fogColorPremuld *= gl_FragColor.w;
	gl_FragColor = mix(gl_FragColor, fogColorPremuld, fogDensity);
	
	if(dot(gl_FragColor, vec4(1.)) < .002)
		discard;
}

