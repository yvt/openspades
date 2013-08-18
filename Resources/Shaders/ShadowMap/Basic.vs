
uniform mat4 projectionViewMatrix;

void PrepareForShadowMapRender(vec3 position, vec3 normal) {
	gl_Position = projectionViewMatrix * vec4(position, 1.);
}

