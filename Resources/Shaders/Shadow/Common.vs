

void PrepareForShadow_Map(vec3 vertexCoord, vec3 normal) ;
void PrepareForShadow_Model(vec3 vertexCoord, vec3 normal);
void PrepareForRadiosity_Map(vec3 vertexCoord, vec3 normal);

void PrepareForShadow(vec3 vertexCoord, vec3 normal) {
	PrepareForShadow_Map(vertexCoord, normal);
	PrepareForShadow_Model(vertexCoord, normal);
	PrepareForRadiosity_Map(vertexCoord, normal);
}