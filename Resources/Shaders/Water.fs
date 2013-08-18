

//varying vec2 detailCoord;
varying vec3 fogDensity;
varying vec3 screenPosition;
varying vec3 viewPosition;
varying vec2 worldPosition;
varying vec3 worldPositionFromOrigin;

varying vec4 waveCoord;
varying vec2 waveCoord2;

uniform sampler2D screenTexture;
uniform sampler2D depthTexture;
uniform sampler2D texture;
uniform sampler2D waveTexture;
uniform vec3 fogColor;
uniform vec2 zNearFar;
uniform vec4 fovTan;
uniform vec4 waterPlane;

uniform vec2 displaceScale;

vec3 EvaluateSunLight();
vec3 EvaluateAmbientLight(float detailAmbientOcclusion);

float decodeDepth(float w, float near, float far){
	return far * near / mix(far, near, w);
}

float depthAt(vec2 pt){
	float w = texture2D(depthTexture, pt).x;
	return decodeDepth(w, zNearFar.x, zNearFar.y);
}

void main() {
	// evaluate waveform
	vec4 wave = texture2D(waveTexture, waveCoord.xy).xyzw;
	wave = mix(vec4(-1.), vec4(1.), wave);
	wave.z = sqrt(1. - dot(wave.xy, wave.xy));
	
	// detail (Far Cry seems to use this technique)
	vec4 wave2 = texture2D(waveTexture, waveCoord.zw).xyzw;
	wave2 = mix(vec4(-1.), vec4(1.), wave2);
	wave2.z = sqrt(1. - dot(wave2.xy, wave2.xy));
	wave += wave2;
	
	// rough
	wave2 = texture2D(waveTexture, waveCoord2.xy).xyzw;
	wave2 = mix(vec4(-1.), vec4(1.), wave2);
	wave2.z = sqrt(1. - dot(wave2.xy, wave2.xy));
	wave += wave2;
	
	wave.xyz = normalize(wave.xyz);
	
	vec2 origScrPos = screenPosition.xy / screenPosition.z;
	vec2 scrPos = origScrPos;
	
	// TODO: do displacement
	vec2 xToUV = dFdx(worldPosition);
	vec2 yToUV = dFdy(worldPosition);
	float scale = 1. / dot(xToUV.xy, yToUV.yx * vec2(1., -1.));
	vec2 disp = vec2(dot(xToUV, wave.xy * vec2(1., -1.)),
					 dot(yToUV, wave.xy * vec2(-1., 1.)));
	scrPos += disp * scale * displaceScale ;
	
	// check envelope length.
	// if the displaced location points the out of the water,
	// reset to the original pos.
	float depth = depthAt(scrPos);
	
	// convert to view coord
	vec3 sampledViewCoord = vec3(mix(fovTan.zw, fovTan.xy, scrPos), 1.) * -depth;
	float planeDistance = dot(vec4(sampledViewCoord, 1.), waterPlane);
 	if(planeDistance < 0.){
		// reset!
		// original pos must be in the water.
		scrPos = origScrPos;
		depth = depthAt(scrPos);
		if(depth + viewPosition.z < 0.){
			// if the pixel is obscured by a object,
			// this fragment of water is not visible
			//discard; done by early-Z test
		}
	}else{
		depth = planeDistance / dot(waterPlane, vec4(0.,0.,1.,0.));
		depth = abs(depth);
		depth -= viewPosition.z;
	}
	
	float envelope = min((depth + viewPosition.z), 1.);
	envelope = 1. - (1. - envelope) * (1. - envelope);
	
	// water color
	// TODO: correct integral
	vec2 waterCoord = worldPosition;
	vec2 integralCoord = floor(waterCoord) + .5;
	vec2 blurDir = (worldPositionFromOrigin.xy);
	blurDir /= max(length(blurDir), 1.);
	vec2 blurDirSign = mix(vec2(-1.), vec2(1.), step(0., blurDir));
	vec2 startPos = (waterCoord - integralCoord) * blurDirSign;
	vec2 diffPos = blurDir * envelope * blurDirSign * .5 /*limit blur*/;
	vec2 subCoord = 1. - clamp((vec2(0.5) - startPos) / diffPos,
						  0., 1.);
	vec2 sampCoord = integralCoord + subCoord * blurDirSign;
	vec3 waterColor = texture2D(texture, sampCoord / 512.).xyz;
	
	// underwater object color
	gl_FragColor = texture2D(screenTexture, scrPos);
	gl_FragColor.xyz *= gl_FragColor.xyz; // screen color to linear
	
	// apply fog color to water color now.
	// note that fog is already applied to underwater object.
	waterColor = mix(waterColor, fogColor, fogDensity);
	
	// blend water color with the underwater object's color.
	gl_FragColor.xyz = mix(gl_FragColor.xyz, waterColor, envelope);
	
	// attenuation factor for addition blendings below
	vec3 att = 1. - fogDensity;
	
	// specular reflection
	vec3 sunlight = EvaluateSunLight();
	vec3 ongoing = normalize(worldPositionFromOrigin);
	if(dot(sunlight, vec3(1.)) > 0.0001){
		vec3 refl = reflect(ongoing,
							wave.xyz);
		float spec = dot(refl, normalize(vec3(0., -1., -1.)));
		spec = max(spec, 0.);
		
		/*
		float sunVisRadius = cos(3.141592654 / 180. / 60. * 32.);
		
		spec = (spec - (1. - (1. - sunVisRadius) * 2.)) / ((1. - sunVisRadius) * 2.);
		float fw = fwidth(spec);
		spec = clamp(.5 + (spec - .5) / fw, 0., 1.);
		*/
		spec *= spec; // ^2
		spec *= spec; // ^4
		spec *= spec; // ^16
		spec *= spec; // ^32
		spec *= spec; // ^64
		spec *= spec; // ^128
		spec *= spec; // ^256
		spec *= spec; // ^512
		spec *= spec; // ^1024
		spec *= 1.;
		gl_FragColor.xyz += sunlight * spec * 10. * att;
		
		
	}
	
	// fresnel refrection to sky
	float fres = dot(ongoing, wave.xyz);
	fres = clamp(1. - fres, 0., 1.);
	fres *= fres;
	fres *= fres;
	fres *= fres;
	fres *= fres;
	fres += .03;
	gl_FragColor.xyz += fogColor * fres * .6 * att;
	
	
	
	
	gl_FragColor.xyz = sqrt(gl_FragColor.xyz);
	
	
	gl_FragColor.w = 1.;
}

