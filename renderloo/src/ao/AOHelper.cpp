#include "ao/AOHelper.hpp"
#include <glm/glm.hpp>
#include <random>
using namespace loo;
using namespace glm;
using namespace std;

unique_ptr<Texture2D> AORandomTexture{};
array<glm::vec3, AO_KERNEL_SIZE> AOKernel{};
bool AOInitialized = false;
void initAO() {
    if (AOInitialized)
        return;
    std::uniform_real_distribution<float> randomFloats(0.0, 1.0);
    std::default_random_engine generator;
    for (int i = 0; i < AO_KERNEL_SIZE; ++i) {
        glm::vec3 sample(randomFloats(generator) * 2.0 - 1.0,
                         randomFloats(generator) * 2.0 - 1.0,
                         randomFloats(generator));
        sample = glm::normalize(sample);
        sample *= randomFloats(generator);
        float scale = float(i) / AO_KERNEL_SIZE;
        scale = glm::mix(0.1f, 1.0f, scale * scale);
        AOKernel[i] = sample * scale;
    }

    std::vector<glm::vec3> ssaoNoise;
    for (int i = 0; i < AO_NOISE_SIZE * AO_NOISE_SIZE; i++) {
        float x = randomFloats(generator) * 2.0 - 1.0,
              y = randomFloats(generator) * 2.0 - 1.0;
        glm::vec3 noise(x, y, 0.0f);
        noise = glm::normalize(noise);
        ssaoNoise.push_back(noise);
    }
    AORandomTexture = std::make_unique<loo::Texture2D>();
    AORandomTexture->init();
    AORandomTexture->setup(ssaoNoise.data(), AO_NOISE_SIZE, AO_NOISE_SIZE,
                           GL_RGB16F, GL_RGB, GL_FLOAT, 1);
    AORandomTexture->setSizeFilter(GL_NEAREST, GL_NEAREST);
    AORandomTexture->setWrapFilter(GL_REPEAT);
    AOInitialized = true;
}

const loo::Texture2D& getAONoiseTexture() {
    return *AORandomTexture;
}

const std::array<glm::vec3, AO_KERNEL_SIZE>& getAOKernel() {
    return AOKernel;
}