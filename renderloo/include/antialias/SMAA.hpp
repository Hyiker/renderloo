#ifndef RENDERLOO_INCLUDE_ANTIALIAS_SMAA_HPP
#define RENDERLOO_INCLUDE_ANTIALIAS_SMAA_HPP
#include <loo/Application.hpp>
#include <loo/Framebuffer.hpp>
#include <loo/Shader.hpp>
#include <loo/Texture.hpp>
/**
 * SMAA: Subpixel Morphological Antialiasing
 *
* The shader has three passes, chained together as follows:
 *
 *                           |input|------------------·
 *                              v                     |
 *                    [ SMAA*EdgeDetection ]          |
 *                              v                     |
 *                          |edgesTex|                |
 *                              v                     |
 *              [ SMAABlendingWeightCalculation ]     |
 *                              v                     |
 *                          |blendTex|                |
 *                              v                     |
 *                [ SMAANeighborhoodBlending ] <------·
 *                              v
 *                           |output|
 * **/
// TODO: implement SMAA T2x: temporal reprojection
class SMAA {
   public:
    SMAA(int width, int height);
    void init();
    const loo::Texture2D& apply(const loo::Application& app,
                                const loo::Texture2D& src);

   private:
    int m_width, m_height;
    loo::Framebuffer m_fb;
    // stencil buffer used for the second pass
    loo::Renderbuffer m_rb;
    // temporal textures, clear each frame
    std::unique_ptr<loo::Texture2D> m_edges, m_blend;
    // precomputed textures
    std::unique_ptr<loo::Texture2D> m_area, m_search;
    // output
    std::unique_ptr<loo::Texture2D> m_output;
    loo::ShaderProgram m_shaderpass1,  // edge detection
        m_shaderpass2,                 // blending weights
        m_shaderpass3;                 // neighborhood blending
};
#endif /* RENDERLOO_INCLUDE_ANTIALIAS_SMAA_HPP */
