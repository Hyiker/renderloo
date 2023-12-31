#include "core/RenderLoo.hpp"

#include <GLFW/glfw3.h>
#include <imgui.h>

#include <ctime>
#include <filesystem>
#include <fstream>
#include <locale>
#include <loo/glError.hpp>
#include <memory>
#include <thread>
#include <vector>
#include "glm/gtx/string_cast.hpp"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <nfd.h>
#include <stb_image_write.h>
#include <functional>
#include <glm/gtx/hash.hpp>
#include "core/Graphics.hpp"
#include "core/PBRMaterials.hpp"
#include "core/Transforms.hpp"

#include <imgui_impl_glfw.h>
#include "glog/logging.h"
#include "shaders/deferred.frag.hpp"
#include "shaders/deferred.vert.hpp"
#include "shaders/gbuffer.frag.hpp"
#include "shaders/gbuffer.vert.hpp"
#include "shaders/shadowmap.frag.hpp"
#include "shaders/shadowmap.vert.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/ext.hpp>
using namespace loo;
using namespace std;
using namespace glm;

namespace fs = std::filesystem;

static constexpr int SHADOWMAP_RESOLUION[2]{2048, 2048};

static void mouseCallback(GLFWwindow* window, double xposIn, double yposIn) {
    ImGui_ImplGlfw_CursorPosCallback(window, xposIn, yposIn);
    if (ImGui::GetIO().WantCaptureMouse)
        return;
    static bool firstMouse = true;
    bool leftPressing =
             glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) != GLFW_RELEASE,
         rightPressing = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) !=
                         GLFW_RELEASE;
    if (!leftPressing && !rightPressing) {
        firstMouse = true;
        return;
    }

    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);
    auto myapp = static_cast<RenderLoo*>(glfwGetWindowUserPointer(window));
    static float lastX = myapp->getWidth() / 2.0;
    static float lastY = myapp->getHeight() / 2.0;

    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;

    lastX = xpos;
    lastY = ypos;
    switch (myapp->getMainCameraMode()) {
        case CameraMode::FPS: {
            auto& fpsCamera = dynamic_cast<FPSCamera&>(myapp->getMainCamera());
            if (rightPressing) {
                fpsCamera.rotateCamera(xoffset, yoffset);
            }
            break;
        }

        case CameraMode::ArcBall: {
            auto& arcballCamera =
                dynamic_cast<ArcBallCamera&>(myapp->getMainCamera());
            if (leftPressing) {
                arcballCamera.orbitCamera(xoffset, yoffset);
            } else if (rightPressing) {
                arcballCamera.panCamera(xoffset * 0.1, yoffset * 0.1);
            }
            break;
        }
    }
}

static void scrollCallback(GLFWwindow* window, double xOffset, double yOffset) {
    if (ImGui::GetIO().WantCaptureMouse) {
        ImGui_ImplGlfw_ScrollCallback(window, xOffset, yOffset);
        return;
    }
    auto myapp = static_cast<RenderLoo*>(glfwGetWindowUserPointer(window));
    myapp->getMainCamera().zoomCamera(yOffset);
}
static std::unique_ptr<loo::PerspectiveCamera> placeCameraBySceneAABB(
    const AABB& aabb, CameraMode mode) {
    vec3 center = aabb.getCenter();
    vec3 diagonal = aabb.getDiagonal();
    float r = std::max(diagonal.x, diagonal.y) / 2.0;
    constexpr float EXPECTED_FOV = 60.f, ZNEAR = 0.01f, ZFAR = 30.f;
    float dist = r / sin(glm::radians(EXPECTED_FOV) / 2.0);
    float overlookAngle = glm::radians(45.f);
    float y = dist * tan(overlookAngle);
    vec3 pos = center + vec3(0, y, dist);
    if (mode == CameraMode::FPS)
        return std::make_unique<FPSCamera>(pos, center, vec3(0, 1, 0), ZNEAR,
                                           std::max(ZFAR, dist + r),
                                           EXPECTED_FOV);
    else if (mode == CameraMode::ArcBall)
        return std::make_unique<ArcBallCamera>(pos, center, vec3(0, 1, 0),
                                               ZNEAR, std::max(ZFAR, dist + r),
                                               EXPECTED_FOV);
    else
        return nullptr;
}

static void drop_callback(GLFWwindow* window, int count, const char** paths) {
    auto myapp = static_cast<RenderLoo*>(glfwGetWindowUserPointer(window));
    if (count > 0) {
        for (int i = 0; i < count; ++i)
            myapp->handleDrop(paths[i]);
    }
}

void RenderLoo::loadModel(const std::string& filename) {
    pauseTime();
    LOG(INFO) << "Loading model from " << filename << endl;
    m_scene = createSceneFromFile(filename);
    AABB sceneAABB = m_scene.computeAABBWorldSpace();
    glm::vec3 diagonal = sceneAABB.getDiagonal();
    LOG(INFO) << "diagnal: " << glm::to_string(diagonal) << endl;
    LOG(INFO) << "diagnal length: " << glm::length(diagonal) << endl;
    // scale the model to approximately 10x10x10
    float scale = 17.0f / glm::length(diagonal);
    m_scene.scale(glm::vec3(scale));
    LOG(INFO) << "scaled the model by: " << scale << endl;
    sceneAABB = m_scene.computeAABBWorldSpace();
    m_mainCamera = placeCameraBySceneAABB(sceneAABB, m_cameraMode);
    LOG(INFO) << "Load done" << endl;

    m_animator.resetAnimation(m_scene.animation);
    convertMaterial();
    frameCount = 0;
    resumeTime();
}
std::vector<const char*> CommonModelExtensions{
    ".gltf", ".glb",   ".obj", ".fbx", ".dae",
    ".3ds",  ".blend", ".ase", ".ifc", ".fbx"};
std::vector<const char*> CommonHDRIExtensions{".hdr", ".exr"};
void RenderLoo::handleDrop(const string& path) {
    fs::path fp(path);
    for (auto& ext : CommonModelExtensions) {
        if (fp.extension() == ext) {
            loadModel(path);
            return;
        }
    }
    for (auto& ext : CommonHDRIExtensions) {
        if (fp.extension() == ext) {
            loadSkybox(path);
            return;
        }
    }
    LOG(ERROR) << "Unsupported file type: " << path << endl;
}
void RenderLoo::loadSkybox(const std::string& filename) {
    pauseTime();
    m_skybox.loadTexture(filename);
    frameCount = 0;
    resumeTime();
}

RenderLoo::RenderLoo(int width, int height)
    : Application(width, height, "RenderLoo"),
      m_baseshader{Shader(GBUFFER_VERT, ShaderType::Vertex),
                   Shader(GBUFFER_FRAG, ShaderType::Fragment)},
      m_scene(),
      m_mainCamera(),
      m_ssao(getWidth(), getHeight()),
      m_deferredshader{Shader(DEFERRED_VERT, ShaderType::Vertex),
                       Shader(DEFERRED_FRAG, ShaderType::Fragment)},
      m_smaa(getWidth(), getHeight()),
      m_finalprocess(getWidth(), getHeight()) {

    PBRMetallicMaterial::init();

    initVelocity();
    initGBuffers();
    m_shadowMapPass.init();
    m_ssao.init();
    m_gtao.init(getWidth(), getHeight());
    initDeferredPass();
    m_transparentPass.init(*m_gbuffers.depthStencil, *m_deferredResult);
    m_bloomPass.init(getWidth(), getHeight());
    m_smaa.init();
    m_taa.init(getWidth(), getHeight());

    // final pass related
    { m_finalprocess.init(); }
    m_debugOutputPass.init(getWidth(), getHeight());
    // create sun light
    if (m_lights.empty()) {
        m_lights.push_back(
            createDirectionalLight(vec3(-1, -1, 0), vec3(1.0), 0.8, 1.0));
    }
    // init lights buffer
    ShaderProgram::initUniformBlock(std::make_unique<UniformBuffer>(
        SHADER_UB_PORT_LIGHTS, sizeof(ShaderLightBlock)));
    // init mvp uniform buffer
    ShaderProgram::initUniformBlock(
        std::make_unique<UniformBuffer>(SHADER_UB_PORT_MVP, sizeof(MVP)));
    panicPossibleGLError();

    // init bone uniform buffer
    ShaderProgram::initUniformBlock(std::make_unique<UniformBuffer>(
        SHADER_UB_PORT_BONES, sizeof(mat4) * BONES_MAX_COUNT));

    { glClipControl(GL_LOWER_LEFT, GL_ZERO_TO_ONE); }

    // render info setup
    ShaderProgram::initUniformBlock(std::make_unique<UniformBuffer>(
        SHADER_UB_PORT_RENDER_INFO, sizeof(RenderInfo)));

    glfwSetDropCallback(getWindow(), drop_callback);
    NFD_Init();
}
void RenderLoo::initGBuffers() {
    m_gbufferfb.init();

    m_gbuffers.init(getWidth(), getHeight());

    m_gbufferfb.attachTexture(*m_gbuffers.position, GL_COLOR_ATTACHMENT0, 0);
    m_gbufferfb.attachTexture(*m_gbuffers.bufferA, GL_COLOR_ATTACHMENT1, 0);
    m_gbufferfb.attachTexture(*m_gbuffers.bufferB, GL_COLOR_ATTACHMENT2, 0);
    m_gbufferfb.attachTexture(*m_gbuffers.bufferC, GL_COLOR_ATTACHMENT3, 0);
    m_gbufferfb.attachTexture(*m_gbuffers.bufferD, GL_COLOR_ATTACHMENT4, 0);
    m_gbufferfb.attachTexture(*m_velocityTexture, GL_COLOR_ATTACHMENT5, 0);
    m_gbufferfb.attachTexture(*m_gbuffers.depthStencil,
                              GL_DEPTH_STENCIL_ATTACHMENT, 0);
}
void RenderLoo::initDeferredPass() {
    m_deferredfb.init();
    m_deferredResult = make_shared<Texture2D>();
    m_deferredResult->init();
    m_deferredResult->setupStorage(getWidth(), getHeight(), GL_RGBA32F, 1);
    m_deferredResult->setSizeFilter(GL_LINEAR, GL_LINEAR);

    m_deferredfb.attachTexture(*m_deferredResult, GL_COLOR_ATTACHMENT0, 0);
    m_deferredfb.attachTexture(*m_gbuffers.depthStencil, GL_DEPTH_ATTACHMENT,
                               0);
    panicPossibleGLError();
}

void RenderLoo::initVelocity() {
    m_velocityTexture = make_unique<Texture2D>();
    m_velocityTexture->init();
    m_velocityTexture->setupStorage(getWidth(), getHeight(), GL_RG16F, 1);
    m_velocityTexture->setSizeFilter(GL_LINEAR, GL_LINEAR);
    m_velocityTexture->setWrapFilter(GL_CLAMP_TO_EDGE);

    ShaderProgram::initUniformBlock(std::make_unique<UniformBuffer>(
        SHADER_UB_PORT_PREVIOUS_FRAME_MVP, sizeof(MVP)));
    panicPossibleGLError();
}

void RenderLoo::saveScreenshot(fs::path filename) const {
    std::vector<unsigned char> pixels(getWidth() * getHeight() * 3);
    glReadPixels(0, 0, getWidth(), getHeight(), GL_RGB, GL_UNSIGNED_BYTE,
                 pixels.data());
    stbi_flip_vertically_on_write(true);
    stbi_write_png(filename.string().c_str(), getWidth(), getHeight(), 3,
                   pixels.data(), getWidth() * 3);
    stbi_flip_vertically_on_write(false);
    panicPossibleGLError();
}

static void popupFileSelector(
    const std::vector<nfdfilteritem_t>& filter, const nfdchar_t* defaultPath,
    std::function<void(const std::string&)> cb,
    std::function<void(nfdresult_t)> cancelCb = [](nfdresult_t) {}) {
    nfdchar_t* outPath = nullptr;
    nfdresult_t result =
        NFD_OpenDialog(&outPath, filter.data(), filter.size(), defaultPath);
    if (result == NFD_OKAY) {
        cb(outPath);
    } else {
        cancelCb(result);
    }
    free(outPath);
}

void RenderLoo::gui() {
    beginEvent("GUI");
    auto& io = ImGui::GetIO();
    float h = io.DisplaySize.y;
    ImGuiWindowFlags windowFlags =
        ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav |
        ImGuiWindowFlags_NoMove;
    {
        ImGui::SetNextWindowBgAlpha(0.5f);
        ImGui::SetNextWindowSize(ImVec2(300, h * 0.3));
        ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
        if (ImGui::Begin("Dashboard", nullptr, windowFlags)) {
            // OpenGL option
            if (ImGui::CollapsingHeader("General info",
                                        ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::Text("Press [H] to toggle GUI");
                ImGui::Text(
                    "Frame generation delay: %.3f ms/frame\n"
                    "FPS: %.1f",
                    1000.0f / io.Framerate, io.Framerate);

                const GLubyte* renderer = glGetString(GL_RENDERER);
                const GLubyte* version = glGetString(GL_VERSION);
                ImGui::TextWrapped("Renderer: %s", renderer);
                ImGui::TextWrapped("OpenGL Version: %s", version);
                int triangleCount = m_scene.countTriangle();
                const char* base = "";
                if (triangleCount > 5000) {
                    triangleCount /= 1000;
                    base = "k";
                }
                if (triangleCount > 5000) {
                    triangleCount /= 1000;
                    base = "M";
                }
                ImGui::Text(
                    "Scene meshes: %d\n"
                    "Scene triangles: %d%s",
                    (int)m_scene.countMesh(), triangleCount, base);
                bool hasAnimation = m_animator.hasAnimation();
                ImGui::TextColored(
                    ImVec4(hasAnimation ? 0.0f : 1.0f,
                           hasAnimation ? 1.0f : 0.0f, 0.0f, 1.0f),
                    "Animation: %s", hasAnimation ? "Yes" : "No");
            }
        }
    }

    {
        ImGui::SetNextWindowBgAlpha(0.5f);
        ImGui::SetNextWindowSize(ImVec2(300, h * 0.7));
        ImGui::SetNextWindowPos(ImVec2(0, h * 0.3), ImGuiCond_Always);
        if (ImGui::Begin("Options", nullptr, windowFlags)) {
            // Sun
            if (!m_lights.empty()) {
                if (ImGui::CollapsingHeader("Sun",
                                            ImGuiTreeNodeFlags_DefaultOpen)) {
                    ImGui::ColorEdit3("Color", (float*)&m_lights[0].color);
                    ImGui::SliderFloat3("Direction",
                                        (float*)&m_lights[0].direction, -1, 1);
                    ImGui::SliderFloat(
                        "Intensity", (float*)&m_lights[0].intensity, 0.0, 3.0);
                }
                // Shadow
                if (ImGui::CollapsingHeader("Shadow",
                                            ImGuiTreeNodeFlags_DefaultOpen)) {

                    const char* transparentMode[] = {"Solid", "Alpha Test"};
                    ImGui::Combo("Transparent",
                                 (int*)(&m_shadowMapPass.transparentShadowMode),
                                 transparentMode,
                                 IM_ARRAYSIZE(transparentMode));
                }
            }

            // Camera
            if (ImGui::CollapsingHeader("Camera",
                                        ImGuiTreeNodeFlags_DefaultOpen)) {
                const char* cameraMode[] = {"FPS", "ArcBall"};
                if (ImGui::Combo("Mode", (int*)(&m_cameraMode), cameraMode,
                                 IM_ARRAYSIZE(cameraMode))) {
                    if (m_cameraMode == CameraMode::FPS) {
                        m_mainCamera = make_unique<FPSCamera>(*m_mainCamera);
                    } else if (m_cameraMode == CameraMode::ArcBall) {
                        auto arcballCam =
                            make_unique<ArcBallCamera>(*m_mainCamera);
                        arcballCam->setCenter(
                            m_scene.computeAABBWorldSpace().getCenter());
                        m_mainCamera = std::move(arcballCam);
                    }
                }
                if (m_cameraMode == CameraMode::ArcBall) {
                    static float rotationDPS = 0.f;
                    float deltaTime = getDeltaTime();
                    ImGui::SliderFloat("Rot(°/s)", &rotationDPS, 0, 360);
                    dynamic_cast<ArcBallCamera&>(*m_mainCamera)
                        .orbitCameraAroundWorldUp(rotationDPS * deltaTime);
                }
                ImGui::TextWrapped(
                    "General info: \n\tPosition: (%.1f, %.1f, "
                    "%.1f)\n\tDirection: (%.2f, %.2f, %.2f)\n\tFOV(°): %.2f",
                    m_mainCamera->position.x, m_mainCamera->position.y,
                    m_mainCamera->position.z, m_mainCamera->getDirection().x,
                    m_mainCamera->getDirection().y,
                    m_mainCamera->getDirection().z,
                    glm::degrees(m_mainCamera->getFov()));
                if (ImGui::Button("Screenshot")) {
                    Framebuffer fbo;
                    fbo.init();
                    Texture2D tex;
                    tex.init();
                    tex.setupStorage(getWidth(), getHeight(), GL_RGB8, 1);
                    fbo.attachTexture(tex, GL_COLOR_ATTACHMENT0, 0);
                    glBlitNamedFramebuffer(0, fbo.getId(), 0, 0, getWidth(),
                                           getHeight(), 0, 0, getWidth(),
                                           getHeight(), GL_COLOR_BUFFER_BIT,
                                           GL_LINEAR);
                    // encode datetime
                    time_t now = time(0);
                    tm* ltm = localtime(&now);
                    char buffer[80];
                    strftime(buffer, 80, "%Y-%m-%d_%H-%M", ltm);
                    string filename = string(buffer) + ".png";
                    fs::path path = fs::current_path() / filename;
                    tex.save(path.string());
                    LOG(INFO) << "Screenshot saved to " << path.string();
                }
            }
            // Model
            if (ImGui::CollapsingHeader("Model",
                                        ImGuiTreeNodeFlags_DefaultOpen)) {
                static float modelRotation = 0.0f;
                if (ImGui::SliderFloat("Rotation(°)", &modelRotation, 0, 360)) {
                    float rotationRad = glm::radians(modelRotation);
                    m_scene.rotation =
                        glm::angleAxis(rotationRad, glm::vec3(0, 1, 0));
                }
            }
            // OpenGL option
            if (ImGui::CollapsingHeader("Render options",
                                        ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::Checkbox("Wire frame mode", &m_wireframe);
                ImGui::Checkbox("Normal mapping", &m_enablenormal);
                ImGui::Checkbox("Enable DFG Compensation",
                                &m_enableDFGCompensation);
                ImGui::Checkbox("Bloom", &m_enableBloom);
                if (m_enableBloom) {
                    int range = m_bloomPass.getBloomRange();
                    if (ImGui::SliderInt("Bloom Range", &range, 1,
                                         m_bloomPass.getMaxBloomRange())) {
                        m_bloomPass.setBloomRange(range);
                    }
                }
                const char* debugOutputItems[7] = {
                    "None",   "Base Color", "Metalness", "Roughness",
                    "Normal", "Emission",   "AO"};
                if (ImGui::TreeNodeEx("Debug Output",
                                      ImGuiTreeNodeFlags_DefaultOpen)) {
                    for (int i = 0; i < 7; i++) {
                        if (ImGui::Selectable(
                                debugOutputItems[i],
                                (int)m_debugOutputPass.debugOutputOption ==
                                    i)) {
                            m_debugOutputPass.debugOutputOption =
                                static_cast<DebugOutputOption>(i);
                        }
                    }
                    ImGui::TreePop();
                }
                const char* antialiasmethod[] = {"None", "SMAA", "TAA"};
                ImGui::Combo("Antialias", (int*)(&m_antialiasmethod),
                             antialiasmethod, IM_ARRAYSIZE(antialiasmethod));
                const char* ambientocclusionmethod[] = {"None", "SSAO", "GTAO"};
                ImGui::Combo("AO", (int*)(&m_aomethod), ambientocclusionmethod,
                             IM_ARRAYSIZE(ambientocclusionmethod));
                if (m_aomethod == AOMethod::SSAO) {
                    ImGui::SliderFloat("bias", &m_ssao.bias, 0.0, 0.5);
                    ImGui::SliderFloat("radius", &m_ssao.radius, 0.0, 1.0);
                }
            }
            // Resources
            if (ImGui::CollapsingHeader("Assets",
                                        ImGuiTreeNodeFlags_DefaultOpen)) {
                {
                    string path = m_skybox.path;
                    fs::path p(path);
                    string filename = p.filename().string();
                    ImGui::PushItemWidth(150);
                    ImGui::InputText("", const_cast<char*>(filename.c_str()),
                                     filename.size(),
                                     ImGuiInputTextFlags_ReadOnly);
                    ImGui::SameLine();
                    if (ImGui::Button("Load HDRI")) {
                        pauseTime();
                        popupFileSelector({{"HDR Image", "hdr,exr"}}, nullptr,
                                          [this](const string& outPath) {
                                              loadSkybox(outPath);
                                          });
                    }
                    static glm::vec3 skyboxColor = glm::vec3(1.f);
                    ImGui::ColorEdit3("", (float*)&skyboxColor,
                                      ImGuiColorEditFlags_HDR);
                    ImGui::SameLine();
                    if (ImGui::Button("Apply")) {
                        m_skybox.loadPureColor(skyboxColor);
                        frameCount = 0;
                    }
                }
                {
                    ImGui::PushItemWidth(150);
                    ImGui::InputText(
                        "", const_cast<char*>(m_scene.modelName.c_str()),
                        m_scene.modelName.size(), ImGuiInputTextFlags_ReadOnly);
                    ImGui::SameLine();
                    if (ImGui::Button("Load Model")) {
                        popupFileSelector({{"Model Files", "glb,gltf,obj,fbx"}},
                                          nullptr,
                                          [this](const string& outPath) {
                                              loadModel(outPath);
                                          });
                    }
                }
            }
        }
    }
    endEvent();
}

void RenderLoo::finalScreenPass(const loo::Texture2D& texture) {
    beginEvent("Final Screen Pass");
    m_finalprocess.render(texture);
    endEvent();
}

void RenderLoo::convertMaterial() {
    int cnt = 0;
    for (auto& mesh : m_scene.getMeshes()) {
        // Now default material is PBR material
        if (mesh->material) {
#ifdef MATERIAL_PBR
            if (!dynamic_pointer_cast<PBRMetallicMaterial>(mesh->material)) {
                auto pbrMaterial = convertPBRMetallicMaterialFromBaseMaterial(
                    *static_pointer_cast<BaseMaterial>(mesh->material));
                mesh->material = pbrMaterial;
                cnt++;
            }
#else
            LOG(INFO) << "Converting material to simple(blinn-phong) material";
            mesh->material = convertSimpleMaterialFromBaseMaterial(
                *static_pointer_cast<BaseMaterial>(mesh->material));
#endif
        }
    }
    LOG(INFO) << "Converted " << cnt << " materials to PBR materials";
}

void RenderLoo::skyboxPass() {
    Application::beginEvent("Skybox Pass");
    m_deferredfb.enableAttachments({GL_COLOR_ATTACHMENT0});
    glEnable(GL_DEPTH_TEST);

    glDepthFunc(GL_GEQUAL);
    glDisable(GL_CULL_FACE);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    m_skybox.draw();
    glDepthFunc(GL_LESS);
    glEnable(GL_CULL_FACE);

    m_deferredfb.unbind();
    Application::endEvent();
}
void RenderLoo::gbufferPass() {
    beginEvent("GBuffer Pass");
    // render gbuffer here
    m_gbufferfb.bind();

    m_gbufferfb.enableAttachments({GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1,
                                   GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3,
                                   GL_COLOR_ATTACHMENT4, GL_COLOR_ATTACHMENT5});

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_GREATER);
    glEnable(GL_STENCIL_TEST);

    glStencilMask(0xFF);
    glStencilFunc(GL_ALWAYS, 1, 0xFF);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
    glClearDepth(0.0f);
    glClearStencil(0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    m_baseshader.use();
    scene(m_baseshader, RenderFlag_Opaque);

    m_gbufferfb.unbind();
    glStencilMask(0x00);
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_CULL_FACE);
    glDepthFunc(GL_LESS);
    glDisable(GL_DEPTH_TEST);

    glStencilFunc(GL_EQUAL, 1, 0xFF);
    // disable stencil write
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
    endEvent();
}

void RenderLoo::deferredPass() {
    beginEvent("Deferred Pass");

    m_deferredfb.bind();
    m_deferredfb.enableAttachments({GL_COLOR_ATTACHMENT0});

    glClearColor(0.f, 0.f, 0.f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    logPossibleGLError();

    m_deferredshader.use();
    m_deferredshader.setTexture(0, *m_gbuffers.position);
    m_deferredshader.setTexture(1, *m_gbuffers.bufferA);
    m_deferredshader.setTexture(2, *m_gbuffers.bufferB);
    m_deferredshader.setTexture(3, *m_gbuffers.bufferC);
    m_deferredshader.setTexture(4, *m_gbuffers.bufferD);
    m_deferredshader.setTexture(5, m_shadowMapPass.getDirectionalShadowMap());
    m_deferredshader.setTexture(6, m_skybox.getDiffuseConv());
    m_deferredshader.setTexture(7, m_skybox.getSpecularConv());
    m_deferredshader.setTexture(8, m_skybox.getBRDFLUT());
    m_deferredshader.setTexture(9, getAOTexture());

    m_deferredshader.setUniform("cameraPosition", m_mainCamera->position);
    m_deferredshader.setUniform("enableCompensation", m_enableDFGCompensation);

    ShaderProgram::getUniformBlock(SHADER_UB_PORT_LIGHTS)
        .mapBufferScoped<ShaderLightBlock>([&](ShaderLightBlock& lights) {
            lights.lightCount = m_lights.size();
            for (int i = 0; i < m_lights.size(); i++) {
                lights.lights[i] = m_lights[i];
            }
        });

    glDisable(GL_DEPTH_TEST);
    Quad::globalQuad().draw();
    glEnable(GL_DEPTH_TEST);

    endEvent();
}

void RenderLoo::aoPass() {
    if (m_aomethod == AOMethod::SSAO)
        m_ssao.render(*m_gbuffers.position, *m_gbuffers.bufferC,
                      *m_gbuffers.depthStencil);
    else if (m_aomethod == AOMethod::GTAO)
        m_gtao.render(*m_gbuffers.position, *m_gbuffers.bufferC,
                      *m_gbuffers.bufferA, *m_gbuffers.depthStencil);
}
const loo::Texture2D& RenderLoo::smaaPass(const loo::Texture2D& input) {
    if (m_antialiasmethod == AntiAliasMethod::SMAA) {
        beginEvent("SMAA Pass");
        const Texture2D& texture = m_smaa.apply(input);
        endEvent();
        return texture;
    }
    return input;
}

const loo::Texture2D& RenderLoo::taaPass(loo::Texture2D& input) {
    if (m_antialiasmethod == AntiAliasMethod::TAA) {
        return m_taa.apply(input, *m_velocityTexture, *m_gbuffers.depthStencil);
    }
    return input;
}

void RenderLoo::scene(loo::ShaderProgram& shader, RenderFlag flag) {
    shader.use();

    shader.setUniform("enableNormal", m_enablenormal);
    logPossibleGLError();
    bool renderOpaque = flag & RenderFlag_Opaque,
         renderTransparent = flag & RenderFlag_Transparent;
    for (auto mesh : m_scene.getMeshes()) {
        if ((!mesh->needAlphaBlend() && renderOpaque) ||
            (mesh->needAlphaBlend() && renderTransparent)) {
            if (mesh->isDoubleSided() || renderTransparent) {
                glDisable(GL_CULL_FACE);
            } else {
                glEnable(GL_CULL_FACE);
                glCullFace(GL_BACK);
            }

            drawMesh(*mesh, m_scene.getModelMatrix(),
                     m_scene.getPreviousModelMatrix(), m_baseshader);
        }
    }
    logPossibleGLError();
}

void RenderLoo::clear() {
    glClearColor(0.f, 0.f, 0.f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    logPossibleGLError();
}
void RenderLoo::keyboard() {
    if (keyForward())
        m_mainCamera->moveCamera(CameraMovement::FORWARD, getDeltaTime());
    if (keyBackward())
        m_mainCamera->moveCamera(CameraMovement::BACKWARD, getDeltaTime());
    if (keyLeft())
        m_mainCamera->moveCamera(CameraMovement::LEFT, getDeltaTime());
    if (keyRight())
        m_mainCamera->moveCamera(CameraMovement::RIGHT, getDeltaTime());
    if (glfwGetKey(getWindow(), GLFW_KEY_R)) {
        m_mainCamera = placeCameraBySceneAABB(m_scene.aabb, m_cameraMode);
    }
}
void RenderLoo::mouse() {
    glfwSetCursorPosCallback(getWindow(), mouseCallback);
    glfwSetScrollCallback(getWindow(), scrollCallback);
}
void RenderLoo::loop() {
    m_mainCamera->setAspect(getWindowRatio());
    // render
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
    glDisable(GL_CULL_FACE);
    glViewport(0, 0, getWidth(), getHeight());

    {
        // update render info
        ShaderProgram::getUniformBlock(SHADER_UB_PORT_RENDER_INFO)
            .mapBufferScoped<RenderInfo>([this](RenderInfo& info) {
                info.deviceSize = glm::vec2(getWidth(), getHeight());
                info.frameCount = frameCount;
                info.timeSecs = getFrameTimeFromStart();
                info.enableTAA = m_antialiasmethod == AntiAliasMethod::TAA;
            });
        animation();

        // setup camera shader uniform block
        ShaderProgram::getUniformBlock(SHADER_UB_PORT_MVP)
            .mapBufferScoped<MVP>([this](MVP& mvp) {
                m_mainCamera->getViewMatrix(mvp.view);
                m_mainCamera->getProjectionMatrix(mvp.projection, true);
            });

        gbufferPass();

        m_shadowMapPass.render(m_scene, m_lights,
                               m_transparentPass.getAlphaTestThreshold());

        aoPass();

        deferredPass();

        skyboxPass();

        m_transparentPass.render(m_scene, m_skybox, *m_mainCamera,
                                 m_shadowMapPass.getDirectionalShadowMap(),
                                 m_enableDFGCompensation);
        const Texture2D& taaResult = taaPass(*m_deferredResult);

        const Texture2D& bloomResult =
            m_enableBloom ? m_bloomPass.render(taaResult) : taaResult;

        const Texture2D& smaaResult = smaaPass(bloomResult);

        if (m_debugOutputPass.debugOutputOption == DebugOutputOption::None)
            finalScreenPass(smaaResult);
        else
            m_debugOutputPass.render(m_gbuffers, getAOTexture());

        // update previous frame MVP
        ShaderProgram::getUniformBlock(SHADER_UB_PORT_PREVIOUS_FRAME_MVP)
            .mapBufferScoped<MVP>([this](MVP& mvp) {
                m_mainCamera->getViewMatrix(mvp.view);
                m_mainCamera->getProjectionMatrix(mvp.projection, true);
            });
        m_scene.savePreviousTransform();
    }

    keyboard();

    mouse();
}

void RenderLoo::animation() {
    if (!m_animator.hasAnimation())
        return;
    m_animator.updateAnimation(getDeltaTime());
    auto& ub = ShaderProgram::getUniformBlock(SHADER_UB_PORT_BONES);
    ub.updateData(m_animator.finalBoneMatrices.data());
    panicPossibleGLError();
}

void RenderLoo::afterCleanup() {
    NFD_Quit();
}
