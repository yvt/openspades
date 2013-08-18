
uniform mat4 shadowMapMatrix1;
uniform mat4 shadowMapMatrix2;
uniform mat4 shadowMapMatrix3;
uniform mat4 shadowMapViewMatrix;

varying vec4 shadowMapCoord1;
varying vec4 shadowMapCoord2;
varying vec4 shadowMapCoord3;
varying vec4 shadowMapViewPos;

void TransformShadowMatrix(out vec4 shadowMapCoord,
						   in vec3 vertexCoord,
						   in mat4 matrix) {
	vec4 c;
	c = matrix * vec4(vertexCoord, 1.);
	c.xyz = (c.xyz * 0.5) + c.w * 0.5;
	// bias
	c.z -= c.w * 0.0003;
	shadowMapCoord = c;
}

void PrepareForShadow_Model(vec3 vertexCoord, vec3 normal) {
	shadowMapViewPos = shadowMapViewMatrix * vec4(vertexCoord, 1.);
	TransformShadowMatrix(shadowMapCoord1,
						  vertexCoord,
						  shadowMapMatrix1);
	TransformShadowMatrix(shadowMapCoord2,
						  vertexCoord,
						  shadowMapMatrix2);
	TransformShadowMatrix(shadowMapCoord3,
						  vertexCoord,
						  shadowMapMatrix3);
	
}