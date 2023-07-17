/*
 Copyright (c) 2017 yvt

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


uniform sampler2D inputTexture;
uniform sampler2D depthTexture;
uniform sampler2D previousTexture;
uniform sampler2D processedInputTexture;
uniform vec2 inverseVP;
uniform mat4 reprojectionMatrix;
uniform mat4 viewProjectionMatrixInv;
uniform float fogDistance;

varying vec2 texCoord;
varying vec4 viewcentricWorldPositionPartial;
/* UE4-style temporal AA. Implementation is based on my ShaderToy submission */

// YUV-RGB conversion routine from Hyper3D
vec3 encodePalYuv(vec3 rgb)
{
    return vec3(
        dot(rgb, vec3(0.299, 0.587, 0.114)),
        dot(rgb, vec3(-0.14713, -0.28886, 0.436)),
        dot(rgb, vec3(0.615, -0.51499, -0.10001))
    );
}

vec3 decodePalYuv(vec3 yuv)
{
    return vec3(
        dot(yuv, vec3(1., 0., 1.13983)),
        dot(yuv, vec3(1., -0.39465, -0.58060)),
        dot(yuv, vec3(1., 2.03211, 0.))
    );
}


void main() {
    // ------------------------------------------------------------------------
    // Reprojection
    //
    // Calulate the Z position of the current pixel. Take the minimum Z value
    // of the neighboring pixels to preserve the antialiasing of foreground
    // objects.
    vec2 off = inverseVP;
    float inputZ0 = texture2D(depthTexture, texCoord).x;
    float inputZ1 = texture2D(depthTexture, texCoord + vec2(+off.x, 0.0)).x;
    float inputZ2 = texture2D(depthTexture, texCoord + vec2(-off.x, 0.0)).x;
    float inputZ3 = texture2D(depthTexture, texCoord + vec2(0.0, +off.y)).x;
    float inputZ4 = texture2D(depthTexture, texCoord + vec2(0.0, -off.y)).x;
    float inputZ5 = texture2D(depthTexture, texCoord + vec2(+off.x, +off.y)).x;
    float inputZ6 = texture2D(depthTexture, texCoord + vec2(-off.x, +off.y)).x;
    float inputZ7 = texture2D(depthTexture, texCoord + vec2(+off.x, -off.y)).x;
    float inputZ8 = texture2D(depthTexture, texCoord + vec2(-off.x, -off.y)).x;
	float inputZ = min(min(min(inputZ0, inputZ1), min(inputZ2, inputZ3)),
	                   min(min(inputZ4, inputZ5), min(inputZ6, min(inputZ7, inputZ8))));

    // Predict where the point was in the previous frame. The Z range [0, 0.1]
    // is for a view weapon, so assume no movement in this range.
    vec4 reprojectedTexCoord;
    if (inputZ < 0.1) {
        reprojectedTexCoord.xy = texCoord.xy;
    } else {
        reprojectedTexCoord = reprojectionMatrix * vec4(texCoord, inputZ, 1.0);
        reprojectedTexCoord.xy /= reprojectedTexCoord.w;
    }

	vec4 lastColor = texture2D(previousTexture, reprojectedTexCoord.xy);

    // ------------------------------------------------------------------------
    // Calculate the approximate fog factor
    //
    // It's used to prevent barely-visible objects from being blurred away.
    vec4 viewcentricWorldPosition =
      viewcentricWorldPositionPartial + viewProjectionMatrixInv * vec4(0.0, 0.0, inputZ, 0.0);
    viewcentricWorldPosition.xyz /= viewcentricWorldPosition.w;

    float voxlapDistanceSq = dot(viewcentricWorldPosition.xy, viewcentricWorldPosition.xy);
    voxlapDistanceSq = min(voxlapDistanceSq, fogDistance * fogDistance);

    float fogFactor = min(voxlapDistanceSq / (fogDistance * fogDistance), 1.0);

    // ------------------------------------------------------------------------
    vec3 antialiased = lastColor.xyz;
    float mixRate = min(lastColor.w, 0.5);

    vec3 in0 = texture2D(processedInputTexture, texCoord).xyz;

    antialiased = mix(antialiased, in0, mixRate);

    vec3 in1 = texture2D(inputTexture, texCoord + vec2(+off.x, 0.0)).xyz;
    vec3 in2 = texture2D(inputTexture, texCoord + vec2(-off.x, 0.0)).xyz;
    vec3 in3 = texture2D(inputTexture, texCoord + vec2(0.0, +off.y)).xyz;
    vec3 in4 = texture2D(inputTexture, texCoord + vec2(0.0, -off.y)).xyz;
    vec3 in5 = texture2D(inputTexture, texCoord + vec2(+off.x, +off.y)).xyz;
    vec3 in6 = texture2D(inputTexture, texCoord + vec2(-off.x, +off.y)).xyz;
    vec3 in7 = texture2D(inputTexture, texCoord + vec2(+off.x, -off.y)).xyz;
    vec3 in8 = texture2D(inputTexture, texCoord + vec2(-off.x, -off.y)).xyz;

    antialiased = encodePalYuv(antialiased);
    in0 = encodePalYuv(in0);
    in1 = encodePalYuv(in1);
    in2 = encodePalYuv(in2);
    in3 = encodePalYuv(in3);
    in4 = encodePalYuv(in4);
    in5 = encodePalYuv(in5);
    in6 = encodePalYuv(in6);
    in7 = encodePalYuv(in7);
    in8 = encodePalYuv(in8);

    vec3 minColor = min(min(min(in0, in1), min(in2, in3)), in4);
    vec3 maxColor = max(max(max(in0, in1), max(in2, in3)), in4);
    minColor = mix(minColor,
       min(min(min(in5, in6), min(in7, in8)), minColor), 0.5);
    maxColor = mix(maxColor,
       max(max(max(in5, in6), max(in7, in8)), maxColor), 0.5);

    vec3 preclamping = antialiased;
    antialiased = clamp(antialiased, minColor, maxColor);

    mixRate = 1.0 / (1.0 / mixRate + 1.0);

    // Increase the mix rate if the prediction is unreliable
    {
        vec3 diff = abs(antialiased - preclamping);
        float clampAmount = max(max(diff.x, diff.y), diff.z);
        mixRate += clampAmount * 8.0;
    }

	// Increase the mix rate if the fog factor is high
    // (Prevents barely-visible objects from being blurred away)
	{
		float contrast = 1.0 - fogFactor;
		const float contrastThreshold = 0.1;
		const float contrastFactor = 2.0;
		mixRate += max(0.0, contrastThreshold - contrast) / contrastThreshold * contrastFactor;
	}

	mixRate = clamp(mixRate, 0.05, 0.5);

    antialiased = decodePalYuv(antialiased);

    gl_FragColor = vec4(max(antialiased, vec3(0.0)), mixRate);
}
