// Common code for sunlight shadow rendering

float VisibilityOfSunLight_Map();
float VisibilityOfSunLight_Model();
vec3 Radiosity_Map(float detailAmbientOcclusion);

float VisibilityOfSunLight() {
	return VisibilityOfSunLight_Map() * 
	VisibilityOfSunLight_Model();
}

vec3 EvaluateSunLight(){
	return vec3(.6) * VisibilityOfSunLight();
}

vec3 EvaluateAmbientLight(float detailAmbientOcclusion) {
	return Radiosity_Map(detailAmbientOcclusion);
}
