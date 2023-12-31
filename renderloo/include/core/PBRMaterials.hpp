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
    glm::vec4 baseColor;
    // roughness(1) + roughness(1) + padding(2)
    glm::vec4 metallicRoughness;
    // emissive(3) + padding(1)
    glm::vec4 emissive;

    ShaderPBRMetallicMaterial(glm::vec4 baseColor, float metallic,
                              float roughness, glm::vec3 emissive)
        : baseColor(baseColor),
          metallicRoughness(metallic, roughness, 0, 0),
          emissive(emissive, 0) {}
};
class PBRMetallicMaterial : public loo::Material {
    ShaderPBRMetallicMaterial m_shadermaterial;
    static std::shared_ptr<PBRMetallicMaterial> defaultMaterial;
    unsigned int m_flags;

   public:
    ShaderPBRMetallicMaterial& getShaderMaterial() { return m_shadermaterial; }
    const ShaderPBRMetallicMaterial& getShaderMaterial() const {
        return m_shadermaterial;
    }
    PBRMetallicMaterial(glm::vec4 baseColor, float metallic, float roughness,
                        glm::vec3 emissive, unsigned int flags)
        : m_shadermaterial(baseColor, metallic, roughness, emissive),
          m_flags(flags) {}
    static void init();
    [[nodiscard]] bool needAlphaBlend() const override {
        return m_flags & loo::LOO_MATERIAL_FLAG_ALPHA_BLEND;
    }
    [[nodiscard]] bool isDoubleSided() const override {
        return m_flags & loo::LOO_MATERIAL_FLAG_DOUBLE_SIDED;
    }
    void bind(const loo::ShaderProgram& sp) override;
    std::shared_ptr<loo::Texture2D> baseColorTex{};
    std::shared_ptr<loo::Texture2D> occlusionTex{};
    std::shared_ptr<loo::Texture2D> metallicTex{};
    std::shared_ptr<loo::Texture2D> roughnessTex{};
    std::shared_ptr<loo::Texture2D> normalTex{};
    std::shared_ptr<loo::Texture2D> emissiveTex{};
};
std::shared_ptr<PBRMetallicMaterial> convertPBRMetallicMaterialFromBaseMaterial(
    const loo::BaseMaterial& baseMaterial);

#endif /* RENDERLOO_INCLUDE_CORE_PBRMATERIALS_HPP */
