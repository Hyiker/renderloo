#ifndef RENDERLOO_SHADERS_INCLUDE_CAMERA_GLSL
#define RENDERLOO_SHADERS_INCLUDE_CAMERA_GLSL

vec2 extractZParamFromProjection(in mat4 proj) {
    float A = proj[2][2];
    float B = proj[3][2];
    float n = B / A;
    float f = n * A / (1.0 + A);
    return vec2(n, f);
}

// 0 at camera, 1 at far plane
float linear01Depth(float depth, float zNear, float zFar) {
    float x = (zNear - zFar) / zFar;
    return 1.0 / (depth * x + 1.0);
}

#endif /* RENDERLOO_SHADERS_INCLUDE_CAMERA_GLSL */
