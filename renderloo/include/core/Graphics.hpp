#ifndef RENDERLOO_INCLUDE_CORE_GRAPHICS_HPP
#define RENDERLOO_INCLUDE_CORE_GRAPHICS_HPP
#include <loo/Mesh.hpp>

void drawMesh(const loo::Mesh& mesh, glm::mat4 transform,
              const loo::ShaderProgram& sp);
#endif /* RENDERLOO_INCLUDE_CORE_GRAPHICS_HPP */
