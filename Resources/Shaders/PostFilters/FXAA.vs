

attribute vec2 positionAttribute;

varying vec2 texCoord;
uniform vec2 inverseVP;

void main() {
	
	vec2 pos = positionAttribute;
	
	vec2 scrPos = pos * 2. - 1.;
	
	gl_Position = vec4(scrPos, 0.5, 1.);
	
	texCoord = pos / inverseVP;
}

