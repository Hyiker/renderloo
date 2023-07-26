#ifndef RENDERLOO_INCLUDE_CORE_PBRMATERIALS_HPP
#define RENDERLOO_INCLUDE_CORE_PBRMATERIALS_HPP

#include <glog/logging.h>
#include <filesystem>
#include <glm/glm.hpp>
#include <loo/Material.hpp>
#include <memory>
#include "constants.hpp"

#include <assimp/types.h>
#include <loo/Texture.hpp>
#include <loo/UniformBuffer.hpp>
#include <loo/loo.hpp>
struct ShaderPBRMetallicMaterial {
    // std140 pad vec3 to 4N(N = 4B)
    // using vec4 to save your day
    glm::vec4 baseColorMetallic;
    // roughness(1) + padding(3)
    glm::vec4 roughness;
    ShaderPBRMetallicMaterial(glm::vec3 baseColor, float metallic,
                              float roughness)
        : baseColorMetallic(baseColor, metallic),
          roughness(roughness, 0, 0, 0) {}
};
class PBRMetallicMaterial : public loo::Material {
    ShaderPBRMetallicMaterial m_shadermaterial;
    static std::shared_ptr<PBRMetallicMaterial> defaultMaterial;

   public:
    ShaderPBRMetallicMaterial& getShaderMaterial() { return m_shadermaterial; }
    const ShaderPBRMetallicMaterial& getShaderMaterial() const {
        return m_shadermaterial;
    }
    PBRMetallicMaterial(glm::vec3 baseColor, float metallic, float roughness)
        : m_shadermaterial(baseColor, metallic, roughness) {}
    static void init();
    void bind(const loo::ShaderProgram& sp) override;
    std::shared_ptr<loo::Texture2D> baseColorTex{};
    std::shared_ptr<loo::Texture2D> occlusionTex{};
    std::shared_ptr<loo::Texture2D> metallicTex{};
    std::shared_ptr<loo::Texture2D> roughnessTex{};
    std::shared_ptr<loo::Texture2D> normalTex{};
};
std::shared_ptr<PBRMetallicMaterial> convertPBRMetallicMaterialFromBaseMaterial(
    const loo::BaseMaterial& baseMaterial);

#endif /* RENDERLOO_INCLUDE_CORE_PBRMATERIALS_HPP */
