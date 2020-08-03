varying vec3 fogDensity;

void main() {

	if (fogDensity.x > 0.9999 && fogDensity.y > 0.9999 && fogDensity.z > 0.9999) {
		discard;
	}

	gl_FragColor.rgba = vec4(0.0, 0.0, 0.0, 1.0);
}

