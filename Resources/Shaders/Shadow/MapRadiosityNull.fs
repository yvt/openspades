

uniform vec3 fogColor;

vec3 Radiosity_Map(float detailAmbientOcclusion) {
	return mix(fogColor, vec3(1.), 0.5) * 0.5 * detailAmbientOcclusion;
}