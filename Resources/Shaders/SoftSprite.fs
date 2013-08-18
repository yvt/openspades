
uniform sampler2D depthTexture;
uniform sampler2D texture;

uniform vec3 fogColor;
uniform vec2 zNearFar;

varying vec4 color;
varying vec4 texCoord;
varying vec4 fogDensity;
varying vec4 depthRange;

float decodeDepth(float w, float near, float far){
	return far * near / mix(far, near, w);
}

float depthAt(vec2 pt){
	float w = texture2D(depthTexture, pt).x;
	return decodeDepth(w, zNearFar.x, zNearFar.y);
}

void main() {
	
	// get depth
	float depth = depthAt(texCoord.zw);
	
	if(depth < depthRange.x){
		discard;
	}
	
	gl_FragColor = texture2D(texture, texCoord.xy);
	gl_FragColor.xyz *= gl_FragColor.w; // premultiplied alpha
	gl_FragColor *= color;
	
	vec4 fogColorPremuld = vec4(fogColor, 1.);
	fogColorPremuld *= gl_FragColor.w;
	gl_FragColor = mix(gl_FragColor, fogColorPremuld, fogDensity);
	
	
	float soft = (depth - depthRange.x) / (depthRange.y - depthRange.w);
	soft = clamp(soft, 0., 1.);
	gl_FragColor *= soft;
	
	if(dot(gl_FragColor, vec4(1.)) < .002)
		discard;
}

