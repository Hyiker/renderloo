#ifndef RENDERLOO_SHADERS_INCLUDE_SAMPLING_HPP
#define RENDERLOO_SHADERS_INCLUDE_SAMPLING_HPP

const int SAMPLING_CATROM = 0, SAMPLING_MITCHELL = 1;
// from https://github.com/runelite/runelite/blob/master/runelite-client/src/main/resources/net/runelite/client/plugins/gpu/scale/bicubic.glsl
/*
 * Copyright (c) 2019 logarrhythmic <https://github.com/logarrhythmic>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
// Cubic filter with Catmull-Rom parameters
float catmull_rom(float x) {
    /*
   * Generally favorable results in image upscaling are given by a cubic filter with the values b = 0 and c = 0.5.
   * This is known as the Catmull-Rom filter, and it closely approximates Jinc upscaling with Lanczos input values.
   * Placing these values into the piecewise equation gives us a more compact representation of:
   *  y = 1.5 * abs(x)^3 - 2.5 * abs(x)^2 + 1                 // abs(x) < 1
   *  y = -0.5 * abs(x)^3 + 2.5 * abs(x)^2 - 4 * abs(x) + 2   // 1 <= abs(x) < 2
   */

    float t = abs(x);
    float t2 = t * t;
    float t3 = t * t * t;

    if (t < 1)
        return 1.5 * t3 - 2.5 * t2 + 1.0;
    else if (t < 2)
        return -0.5 * t3 + 2.5 * t2 - 4.0 * t + 2.0;
    else
        return 0.0;
}

float mitchell(float x) {
    /*
   * This is another cubic filter with less aggressive sharpening than Catmull-Rom, which some users may prefer.
   * B = 1/3, C = 1/3.
   */

    float t = abs(x);
    float t2 = t * t;
    float t3 = t * t * t;

    if (t < 1)
        return 7.0 / 6.0 * t3 + -2.0 * t2 + 8.0 / 9.0;
    else if (t < 2)
        return -7.0 / 18.0 * t3 + 2.0 * t2 - 10.0 / 3.0 * t + 16.0 / 9.0;
    else
        return 0.0;
}

#define CR_AR_STRENGTH 0.9

#define FLT_MAX 3.402823466e+38
#define FLT_MIN 1.175494351e-38

// Calculates the distance between two points
float d(vec2 pt1, vec2 pt2) {
    vec2 v = pt2 - pt1;
    return sqrt(dot(v, v));
}

// Samples a texture using a 4x4 kernel.
vec4 textureCubic(sampler2D sampler, vec2 texCoords, int mode) {
    vec2 texSize = textureSize(sampler, 0);
    vec2 texelSize = 1.0 / texSize;
    vec2 texelFCoords = texCoords * texSize;
    texelFCoords -= 0.5;

    vec4 nSum = vec4(0.0, 0.0, 0.0, 0.0);
    vec4 nDenom = vec4(0.0, 0.0, 0.0, 0.0);

    vec2 coordFract = fract(texelFCoords);
    texCoords -= coordFract * texelSize;

    vec4 c;

    if (mode == SAMPLING_CATROM) {
        // catrom benefits from anti-ringing, which requires knowledge of the minimum and maximum samples in the kernel
        vec4 min_sample = vec4(FLT_MAX);
        vec4 max_sample = vec4(FLT_MIN);
        for (int m = -1; m <= 2; m++) {
            for (int n = -1; n <= 2; n++) {
                // this would use texelFetch, but that would require manual implementation of texture wrapping
                vec4 vecData =
                    texture(sampler, texCoords + vec2(m, n) * texelSize);

                // update min and max as we go
                min_sample = min(min_sample, vecData);
                max_sample = max(max_sample, vecData);

                // calculate weight based on distance of the current texel offset from the sub-texel position of the sampling location
                float w = catmull_rom(d(vec2(m, n), coordFract));

                // build the weighted average
                nSum += vecData * w;
                nDenom += w;
            }
        }
        // calculate weighted average
        c = nSum / nDenom;

        // store value before anti-ringing
        vec4 aux = c;
        // anti-ringing: clamp the color value so that it cannot exceed values already present in the kernel area
        c = clamp(c, min_sample, max_sample);
        // mix according to anti-ringing strength
        c = mix(aux, c, CR_AR_STRENGTH);
    } else if (mode == SAMPLING_MITCHELL) {
        for (int m = -1; m <= 2; m++) {
            for (int n = -1; n <= 2; n++) {
                // this would use texelFetch, but that would require manual implementation of texture wrapping
                vec4 vecData =
                    texture(sampler, texCoords + vec2(m, n) * texelSize);

                // calculate weight based on distance of the current texel offset from the sub-texel position of the sampling location
                float w = mitchell(d(vec2(m, n), coordFract));

                // build the weighted average
                nSum += vecData * w;
                nDenom += w;
            }
        }
        // calculate weighted average
        c = nSum / nDenom;
    }

    // return the weighted average
    return c;
}
// modified from https://www.shadertoy.com/view/4lscWj
#ifndef PI
#define PI 3.1415926535897932384626433832795
#endif
vec2 Hammersley(float i, float numSamples) {
    uint b = uint(i);

    b = (b << 16u) | (b >> 16u);
    b = ((b & 0x55555555u) << 1u) | ((b & 0xAAAAAAAAu) >> 1u);
    b = ((b & 0x33333333u) << 2u) | ((b & 0xCCCCCCCCu) >> 2u);
    b = ((b & 0x0F0F0F0Fu) << 4u) | ((b & 0xF0F0F0F0u) >> 4u);
    b = ((b & 0x00FF00FFu) << 8u) | ((b & 0xFF00FF00u) >> 8u);

    float radicalInverseVDC = float(b) * 2.3283064365386963e-10;

    return vec2((i / numSamples), radicalInverseVDC);
}

vec3 SampleHemisphereUniform(float i, float numSamples, out float pdf) {
    // Returns a 3D sample vector orientated around (0.0, 0.0, 1.0)
    // For practical use, must rotate with a rotation matrix (or whatever
    // your preferred approach is) for use with normals, etc.

    vec2 xi = Hammersley(i, numSamples);

    float phi = xi.y * 2.0 * PI;
    float cosTheta = 1.0 - xi.x;
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta);
    pdf = 1.0 / (2.0 * PI);

    return vec3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);
}

vec3 SampleHemisphereCosineWeighted(float i, float numSamples, out float pdf) {
    vec2 xi = Hammersley(i, numSamples);

    float phi = xi.y * 2.0 * PI;
    float cosTheta = sqrt(1.0 - xi.x);
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta);

    pdf = cosTheta / PI;

    return vec3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);
}

const vec2 Halton_2_3[8] = {
    vec2(0.0f, -1.0f / 3.0f),        vec2(-1.0f / 2.0f, 1.0f / 3.0f),
    vec2(1.0f / 2.0f, -7.0f / 9.0f), vec2(-3.0f / 4.0f, -1.0f / 9.0f),
    vec2(1.0f / 4.0f, 5.0f / 9.0f),  vec2(-1.0f / 4.0f, -5.0f / 9.0f),
    vec2(3.0f / 4.0f, 1.0f / 9.0f),  vec2(-7.0f / 8.0f, 7.0f / 9.0f)};

#endif /* RENDERLOO_SHADERS_INCLUDE_SAMPLING_HPP */
