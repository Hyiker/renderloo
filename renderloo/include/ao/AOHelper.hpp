#ifndef RENDERLOO_INCLUDE_AO_AOHELPER_HPP
#define RENDERLOO_INCLUDE_AO_AOHELPER_HPP
#include <array>
#include <glm/vec3.hpp>
#include <loo/Texture.hpp>
// generate random textures and kernel for SSAO

constexpr int AO_KERNEL_SIZE = 64;
constexpr int AO_NOISE_SIZE = 4;

void initAO();

const loo::Texture2D& getAONoiseTexture();

const std::array<glm::vec3, AO_KERNEL_SIZE>& getAOKernel();

#endif /* RENDERLOO_INCLUDE_AO_AOHELPER_HPP */
