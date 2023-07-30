#ifndef HDSSS_INCLUDE_CONSTANTS_HPP
#define HDSSS_INCLUDE_CONSTANTS_HPP

constexpr int SHADER_SAMPLER_PORT_SKYBOX = 0;
constexpr int SHADER_UB_PORT_MVP = 0;

// light binding
constexpr int SHADER_UB_PORT_LIGHTS = 1;

// bone binding
constexpr int SHADER_UB_PORT_BONES = 2;

constexpr int SHADER_BINDING_PORT_MATERIAL_NORMAL = 7;

// simple material binding
constexpr int SHADER_BINDING_PORT_SM_PARAM = 2;
constexpr int SHADER_BINDING_PORT_SM_AMBIENT = 3;
constexpr int SHADER_BINDING_PORT_SM_DIFFUSE = 4;
constexpr int SHADER_BINDING_PORT_SM_SPECULAR = 5;
constexpr int SHADER_BINDING_PORT_SM_DISPLACEMENT = 6;
constexpr int SHADER_BINDING_PORT_SM_OPACITY = 8;
constexpr int SHADER_BINDING_PORT_SM_HEIGHT = 9;

// pbr metallic-roughness material binding
constexpr int SHADER_BINDING_PORT_MR_PARAM = 3;
constexpr int SHADER_BINDING_PORT_MR_BASECOLOR = 10;
constexpr int SHADER_BINDING_PORT_MR_OCCLUSION = 11;
constexpr int SHADER_BINDING_PORT_MR_METALLIC = 12;
constexpr int SHADER_BINDING_PORT_MR_ROUGHNESS = 13;
constexpr int SHADER_BINDING_PORT_MR_EMISSIVE = 14;

#endif /* HDSSS_INCLUDE_CONSTANTS_HPP */
