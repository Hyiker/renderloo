#include "core/Graphics.hpp"
#include <loo/Shader.hpp>
#include <loo/UniformBuffer.hpp>
#include <loo/glError.hpp>
#include "core/Transforms.hpp"
#include "core/constants.hpp"
void drawMesh(const loo::Mesh& mesh, glm::mat4 transform,
              const loo::ShaderProgram& sp) {
    loo::ShaderProgram::getUniformBlock(SHADER_UB_PORT_MVP)
        .mapBufferScoped<MVP>([&](MVP& mvp) {
            mvp.model = transform * mesh.objectMatrix;
            mvp.normalMatrix = glm::transpose(glm::inverse(mvp.model));
        });
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glBindVertexArray(mesh.vao);
    // bind material uniforms
    mesh.material->bind(sp);
    logPossibleGLError();
    glDrawElements(GL_TRIANGLES, static_cast<GLuint>(mesh.indices.size()),
                   GL_UNSIGNED_INT, (void*)(0));

    glBindVertexArray(0);
}