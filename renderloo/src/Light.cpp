#include "core/Light.hpp"
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "loo/glError.hpp"

using namespace std;
using namespace glm;
void ShaderLight::setDirection(const glm::vec3& d) {
    direction = glm::vec4(glm::normalize(d), 1);
}
glm::mat4 ShaderLight::getLightSpaceMatrix(bool reverseZ01) const {
    LightType type = static_cast<LightType>(this->type);
    switch (type) {
        case LightType::DIRECTIONAL: {
            glm::vec3 up{0.0f, 1.0f, 0.0f};
            if (direction.x == 0.0f && direction.z == 0.0f)
                up = glm::vec3(1.0f, 0.0f, 0.0f);
            float boxSize = 20.0f;
            glm::mat4 lookAt = glm::lookAt(
                glm::vec3(0, 0, 0), glm::normalize(glm::vec3(direction)), up);
            float zNear = -20.0f, zFar = 20.0f;
            if (reverseZ01) {
                std::swap(zNear, zFar);
            }
            return glm::ortho<float>(boxSize, -boxSize, boxSize, -boxSize,
                                     zNear, zFar) *
                   lookAt;
        }
        default: {
            NOT_IMPLEMENTED_RUNTIME();
        }
    }
}
static int
    shadowedDirectionalLightTileIndex[SHADER_SHADOWED_DIRECTIONAL_LIGHTS_MAX]{};
static int shadowedDirectionalLightTileIndexCount = 0;
ShaderLight createDirectionalLight(const glm::vec3& d, const glm::vec3& c,
                                   float intensity, float shadowStrength) {
    ShaderLight direction;
    direction.type = static_cast<int>(LightType::DIRECTIONAL);
    direction.setDirection(d);
    direction.setColor(c);
    direction.intensity = intensity;
    DirectionalShadowData shadowData;
    shadowData.strength = shadowStrength;
    if (shadowStrength > 0.0f) {
        shadowData.tileIndex = shadowedDirectionalLightTileIndexCount++;
        shadowedDirectionalLightTileIndex[shadowData.tileIndex] =
            shadowData.tileIndex;
        if (shadowedDirectionalLightTileIndexCount >
            SHADER_SHADOWED_DIRECTIONAL_LIGHTS_MAX) {
            LOG(ERROR) << "Too many shadowed directional lights!";
        }
    }
    direction.shadowData = shadowData;
    return direction;

}  // namespace loo