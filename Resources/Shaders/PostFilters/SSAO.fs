/*
 Copyright (c) 2016 yvt

 This file is part of OpenSpades.

 OpenSpades is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 OpenSpades is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with OpenSpades.  If not, see <http://www.gnu.org/licenses/>.

 */


uniform sampler2D depthTexture;
uniform sampler2D ditherTexture; // 4x4 pattern
uniform vec2 zNearFar;
uniform vec2 pixelShift;
uniform vec2 fieldOfView;
uniform vec2 sampleOffsetScale;

varying vec2 texCoord;

float decodeDepth(float w, float near, float far){
    return far * near / mix(far, near, w);
}

// d(decodeDepth)/dw
float decodeDepthDW(float w, float near, float far){
    return far * near * (far - near) / pow(mix(far, near, w), 2.0);
}

float minabs(float a, float b) {
    return abs(a) < abs(b) ? a : b;
}

vec2 complexMultiply(vec2 a, vec2 b)
{
    vec3 t = vec3(b, -b.y);
    return vec2(dot(a.xy, t.xz), dot(a.xy, t.yx));
}

void main() {
    // Try to estimate the surface normal
    float depth1 = texture2D(depthTexture, texCoord).x;
    if (depth1 >= 0.999999) {
        // skip background
        gl_FragColor = vec4(1.0);
        return;
    }
    float depth2 = texture2D(depthTexture, texCoord + vec2(pixelShift.x, 0.0)).x;
    float depth3 = texture2D(depthTexture, texCoord - vec2(pixelShift.x, 0.0)).x;
    float dDepthdX = minabs(depth2 - depth1, depth1 - depth3);
    float depth4 = texture2D(depthTexture, texCoord + vec2(0.0, pixelShift.y)).x;
    float depth5 = texture2D(depthTexture, texCoord - vec2(0.0, pixelShift.y)).x;
    float dDepthdY = minabs(depth4 - depth1, depth1 - depth5);

    float originDepth = decodeDepth(depth1, zNearFar.x, zNearFar.y);

    float diffScale = decodeDepthDW(depth1, zNearFar.x, zNearFar.y);
    vec3 viewCoordDiff1 = vec3(pixelShift.x * fieldOfView.x * originDepth, 0.0, diffScale * dDepthdX);
    vec3 viewCoordDiff2 = vec3(0.0, pixelShift.y * fieldOfView.y * originDepth, diffScale * dDepthdY);
    vec3 originNormal = normalize(cross(viewCoordDiff1, viewCoordDiff2));

    // Sampling pattern
    float patternPos1 = texture2D(ditherTexture, gl_FragCoord.xy * 0.25).x * 15.0 / 16.0;
    float patternPos2 = texture2D(ditherTexture, gl_FragCoord.xy * 0.25 + vec2(0.25, 0.0)).x * 15.0 / 16.0;

    patternPos1 += texture2D(ditherTexture, gl_FragCoord.xy * 0.25 * 0.25).x / 16.0;
    patternPos2 += texture2D(ditherTexture, gl_FragCoord.xy * 0.25 * 0.25 + vec2(0.25, 0.0)).x / 16.0;

    // generalization of x[n+1] = x[n] * 1.3 + 1, x[0] = 1:
    //   pow(10.0, -n) * pow(13.0, n + 1.0) * (1.0 / 3.0) - 10.0 / 3.0
    // polynomial approximation:
    float sampleDistance = 1.0 + patternPos2 * (1.13713 + patternPos2 * (0.147928 + patternPos2 * 0.0149391));
    const float sampleRotAngle = 2.61314;
    vec2 sampleRot = vec2(sin(sampleRotAngle), cos(sampleRotAngle));

    patternPos1 *= 3.141592654 * 2.0;
    vec2 sampleDir = vec2(sin(patternPos1), cos(patternPos1));

    // Center location
    vec3 originViewCoord = vec3((texCoord * 2.0 - 1.0) * fieldOfView, 1.0);
    originViewCoord *= originDepth;

    // Decay parameter
    float depthDecayScale = -2.0;

    float sampleDecay = 1.;
    float ret = 0.0;

    for (int i = 0; i < 16; ++i) {
        vec2 sampleRelativeCoord = sampleDir * sampleDistance * sampleOffsetScale;
        vec2 sampleCoord = texCoord + sampleRelativeCoord;

        float sampledDepth = texture2D(depthTexture, sampleCoord).x;
        float decodedDepth = decodeDepth(sampledDepth, zNearFar.x, zNearFar.y);

        decodedDepth += 0.1; // FIXME: this value needs to be tweaked?

        vec3 viewCoord =  vec3((sampleCoord * 2.0 - 1.0) * fieldOfView, 1.) * decodedDepth;
        vec3 relativeViewCoord = normalize(viewCoord - originViewCoord);
        float cosHorizon = -dot(relativeViewCoord, originNormal);
        float depthDecay = exp2(depthDecayScale * abs(decodedDepth - originDepth));

        ret = mix(ret, max(ret, cosHorizon), sampleDecay * depthDecay);

        sampleDistance += 1. + sampleDistance * 0.3;
        sampleDir = complexMultiply(sampleDir, sampleRot);
        sampleDecay *= .92;
    }

    ret = 1.0 - ret;
    gl_FragColor.xyz = vec3(ret);
    gl_FragColor.w = 1.0;
}

