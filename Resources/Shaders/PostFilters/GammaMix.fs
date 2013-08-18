
uniform sampler2D texture1;
uniform sampler2D texture2;

varying vec2 texCoord;

uniform vec3 mix1;
uniform vec3 mix2;

void main() {
	vec3 color1, color2;
	color1 = texture2D(texture1, texCoord).xyz;
	color2 = texture2D(texture2, texCoord).xyz;
	
	vec3 color = color1 * color1 * mix1;
	color += color2 * color2 * mix2;
	
	color = sqrt(color);
	
	gl_FragColor = vec4(color, 1.);
}

