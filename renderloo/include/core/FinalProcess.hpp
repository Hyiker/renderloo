#ifndef HDSSS_INCLUDE_FINAL_PROCESS_HPP
#define HDSSS_INCLUDE_FINAL_PROCESS_HPP
#include <loo/Framebuffer.hpp>
#include <loo/Quad.hpp>
#include <loo/Shader.hpp>
#include <loo/Texture.hpp>
#include <memory>

struct FinalPassOptions {
    bool diffuse{false};
    bool specular{true};
    bool translucency{true};
    bool SSS{true};
    float SSSStrength{1.0f};
    bool directOutput{false};
};
class FinalProcess {
    loo::ShaderProgram m_shader;
    int m_width, m_height;

   public:
    FinalProcess(int width, int height);
    void init();
    void render(const loo::Texture2D& deferredTexture);
};

#endif /* HDSSS_INCLUDE_FINAL_PROCESS_HPP */
