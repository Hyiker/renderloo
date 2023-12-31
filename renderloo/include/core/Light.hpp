#ifndef LOO_INCLUDE_LOO_LIGHT_HPP
#define LOO_INCLUDE_LOO_LIGHT_HPP
#include <glm/glm.hpp>

#include <loo/UniformBuffer.hpp>
enum class LightType { SPOT = 0, POINT = 1, DIRECTIONAL = 2 };
constexpr int SHADER_LIGHTS_MAX = 12,
              SHADER_SHADOWED_DIRECTIONAL_LIGHTS_MAX = 1;

struct DirectionalShadowData {
    float strength{0.f};
    int tileIndex{0};
    int padding[2]{0};
};
struct ShaderLight {
    // spot, point
    glm::vec4 position;
    // spot, directional
    // directional base orientation (0, 1, 0)
    glm::vec4 direction;
    // all
    glm::vec4 color;
    float intensity;
    // point, spot
    // negative value stands for INF
    float range;
    // spot
    float spotAngle;
    int type;
    DirectionalShadowData shadowData;
    void setPosition(const glm::vec3& p) { position = glm::vec4(p, 1); }
    void setDirection(const glm::vec3& d);
    void setColor(const glm::vec3& c) { color = glm::vec4(c, 1); }
    void setType(LightType t) { type = static_cast<int>(t); }
    LightType getType() const { return static_cast<LightType>(type); }
    glm::mat4 getLightSpaceMatrix(bool reverseZ01 = false) const;
};

struct ShaderLightBlock {
    ShaderLight lights[SHADER_LIGHTS_MAX];
    int lightCount{0};
};

struct ShaderDirectionalShadowMatricesBlock {
    glm::mat4 matrices[SHADER_SHADOWED_DIRECTIONAL_LIGHTS_MAX];
};
// TODO
// ShaderLight createSpotLight(const glm::vec3& p, const glm::vec3& o,
//                             const glm::vec3& c, float intensity = 1.0,
//                             float range = -1.0, float spotAngle = 45.f);

// ShaderLight createPointLight(const glm::vec3& p, const glm::vec3& c,
//                              float intensity = 1.0, float range = -1.0);

ShaderLight createDirectionalLight(const glm::vec3& d, const glm::vec3& c,
                                   float intensity = 1.0,
                                   float shadowStrength = 1.0f);

#endif /* LOO_INCLUDE_LOO_LIGHT_HPP */
