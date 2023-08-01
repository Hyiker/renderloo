#include "core/Deferred.hpp"
#include <memory>
using namespace loo;
using namespace std;
void GBuffer::init(int width, int height) {
    int mipmapLevel = mipmapLevelFromSize(width, height);

    position = make_unique<Texture2D>();
    position->init();
    position->setupStorage(width, height, GL_RGBA32F, mipmapLevel);
    position->setSizeFilter(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);

    bufferA = make_unique<Texture2D>();
    bufferA->init();
    bufferA->setupStorage(width, height, GL_RGBA32F, mipmapLevel);
    bufferA->setSizeFilter(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);

    bufferB = make_unique<Texture2D>();
    bufferB->init();
    bufferB->setupStorage(width, height, GL_RGBA32F, mipmapLevel);
    bufferB->setSizeFilter(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);

    bufferC = make_unique<Texture2D>();
    bufferC->init();
    bufferC->setupStorage(width, height, GL_RGBA32F, mipmapLevel);
    bufferC->setSizeFilter(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);

    bufferD = make_unique<Texture2D>();
    bufferD->init();
    bufferD->setupStorage(width, height, GL_RGBA32F, mipmapLevel);
    bufferD->setSizeFilter(GL_LINEAR, GL_LINEAR);

    panicPossibleGLError();

    depthStencil = make_unique<Texture2D>();
    depthStencil->init();
    depthStencil->setupStorage(width, height, GL_DEPTH24_STENCIL8, 1);
    depthStencil->setSizeFilter(GL_LINEAR, GL_LINEAR);
}