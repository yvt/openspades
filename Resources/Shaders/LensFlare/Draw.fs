
uniform sampler2D visibilityTexture;

varying vec2 texCoord;

uniform vec3 color;

void main() {
	float val = texture2D(visibilityTexture, texCoord).x;
	
	gl_FragColor = vec4(color * val, 1.);
}

