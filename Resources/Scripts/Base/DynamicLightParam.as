#include "DynamicLightType.as"
namespace spades {
	class DynamicLightParam {
		DynamicLightType type;
		Vector3 origin;
		float radius;
		Vector3 color;
		
		Vector3 spotAxisX;
		Vector3 spotAxisY;
		Vector3 spotAxisZ;
		IImage@ image;
		float spotAngle;
		
		DynamicLightParam() {
			type = spades::DynamicLightType::Point;
		}
	}
}
