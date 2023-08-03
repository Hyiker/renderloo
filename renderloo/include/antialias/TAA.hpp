#ifndef RENDERLOO_INCLUDE_ANTIALIAS_TAA_HPP
#define RENDERLOO_INCLUDE_ANTIALIAS_TAA_HPP
#include <loo/ComputeShader.hpp>
#include <loo/Texture.hpp>
/**
 * Temporal Anti-Aliasing
 * TAA has several stages that require to be inserted into the rendering pipeline
 * 1. GBuffer geometry pass
    * 1.1 Jittering: jitter the sampling uv(modify projection matrix in practice)
    * 1.2 Reprojection: use MVP of previous frame to compute the vertex position, 
                compute velocity vector afterwards
 * 2. Before tonemapping:
    * 2.1 do a simple tonemapping, convert from HDR to LDR
    * 2.2 execute history frame blending
 */
class TAA {
   public:
    TAA();
    void init(int width, int height);
    const loo::Texture2D& apply(loo::Texture2D& currentFrame,
                                loo::Texture2D& velocity,
                                loo::Texture2D& depthStencil);
    int getPreviousFrameIndex() const { return m_writeInIndex ^ 1; }

   private:
    std::unique_ptr<loo::Texture2D> m_historyFrame[2];  // ping-pong

    int m_writeInIndex = 0;
    loo::ComputeShader m_blendingShader;
};

#endif /* RENDERLOO_INCLUDE_ANTIALIAS_TAA_HPP */
