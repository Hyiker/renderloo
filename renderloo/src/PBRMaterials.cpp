#include "core/PBRMaterials.hpp"
#include <glog/logging.h>
#include <loo/Material.hpp>
#include "core/constants.hpp"

#include <memory>
#include <string>

#include <assimp/material.h>
#include <glm/fwd.hpp>
#include <loo/Shader.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>
using namespace std;
using namespace glm;
using namespace loo;

void PBRMetallicMaterial::bind(const ShaderProgram& sp) {
    ShaderProgram::getUniformBlock(SHADER_BINDING_PORT_MR_PARAM)
        .mapBufferScoped<ShaderPBRMetallicMaterial>(
            [&](ShaderPBRMetallicMaterial& material) {
                material = m_shadermaterial;
            });
    sp.setTexture(SHADER_BINDING_PORT_MR_BASECOLOR,
                  baseColorTex ? *baseColorTex : Texture2D::getWhiteTexture());
    sp.setTexture(SHADER_BINDING_PORT_MATERIAL_NORMAL,
                  normalTex ? *normalTex : Texture2D::getBlackTexture());
    sp.setTexture(SHADER_BINDING_PORT_MR_METALLIC,
                  metallicTex ? *metallicTex : Texture2D::getWhiteTexture());
    sp.setTexture(SHADER_BINDING_PORT_MR_ROUGHNESS,
                  roughnessTex ? *roughnessTex : Texture2D::getWhiteTexture());
    sp.setTexture(SHADER_BINDING_PORT_MR_OCCLUSION,
                  occlusionTex ? *occlusionTex : Texture2D::getWhiteTexture());
}
void PBRMetallicMaterial::init() {
    ShaderProgram::initUniformBlock(make_unique<UniformBuffer>(
        SHADER_BINDING_PORT_MR_PARAM, sizeof(ShaderPBRMetallicMaterial)));
}

shared_ptr<PBRMetallicMaterial> PBRMetallicMaterial::defaultMaterial = nullptr;

std::shared_ptr<PBRMetallicMaterial> convertPBRMetallicMaterialFromBaseMaterial(
    const loo::BaseMaterial& baseMaterial) {
    const auto& pbrMetallic = baseMaterial.mrWorkFlow;
    auto metallicMaterial = std::make_shared<PBRMetallicMaterial>(
        pbrMetallic.baseColor, pbrMetallic.metallic, pbrMetallic.roughness);
    metallicMaterial->baseColorTex = pbrMetallic.baseColorTex;
    metallicMaterial->normalTex = baseMaterial.normalTex;
    metallicMaterial->metallicTex = pbrMetallic.metallicTex;
    metallicMaterial->roughnessTex = pbrMetallic.roughnessTex;
    metallicMaterial->occlusionTex = pbrMetallic.occlusionTex;

    return metallicMaterial;
}