

attribute vec2 positionAttribute;
attribute vec4 colorAttribute;

uniform vec2 fov;

varying vec3 angleTan;
varying vec4 texCoord1;
varying vec4 texCoord2;
varying vec4 texCoord3;
varying vec2 texCoord4;

void main() {
	
	vec2 pos = positionAttribute;
	
	vec2 scrPos = pos * 2. - 1.;
	
	gl_Position = vec4(scrPos, 0.5, 1.);
	
	// texture coords
	vec2 startCoord = pos;
	vec2 diff = vec2(.5) - startCoord;
	diff *= 0.008;
	
	startCoord += diff * .7;
	diff = -diff;
	
	texCoord1 = startCoord.xyxy + diff.xyxy * vec4(vec2(0.), vec2(.1));
	texCoord2 = startCoord.xyxy + diff.xyxy * vec4(vec2(0.2), vec2(.3));
	texCoord3 = startCoord.xyxy + diff.xyxy * vec4(vec2(0.4), vec2(.5));
	texCoord4 = pos.xy;
	
	// view angle
	angleTan.xy = scrPos * fov;
	
	// angleTan.z = brightness scale
	//            = 1 / cos(fovDiag)
	angleTan.z = length(fov) * length(fov) + 1.;
	angleTan.z = mix(angleTan.z, 1., 1.); // weaken the brightness adjust
}

