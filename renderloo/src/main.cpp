
#include <glog/logging.h>

#include <argparse/argparse.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <loo/loo.hpp>
#include <nlohmann/json.hpp>
#include <string>
#include "core/RenderLoo.hpp"
#include "glm/trigonometric.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/string_cast.hpp"

using json = nlohmann::json;

namespace fs = std::filesystem;
using namespace std;

static glm::vec3 parseVec3(const json& j, const string& key,
                           glm::vec3 defaultVal) {
    if (j.contains(key)) {
        auto& v = j[key];
        return glm::vec3(v[0], v[1], v[2]);
    } else {
        return defaultVal;
    }
}

void loadScene(RenderLoo& app, const char* filename) {
    using namespace std;
    fs::path p(filename);
    auto suffix = p.extension();
    if (suffix == ".obj" || suffix == ".fbx") {
        LOG(INFO) << "Loading model from " << suffix << " file" << endl;
        app.loadModel(filename);
    } else if (suffix == ".gltf" || suffix == ".glb") {
        LOG(INFO) << "Loading scene from gltf file" << endl;
        app.loadGLTF(filename);
    } else {
        LOG(FATAL) << "Unrecognizable file extension " << suffix << endl;
    }
    app.convertMaterial();
}

int main(int argc, char* argv[]) {
    loo::initialize(argv[0]);

    argparse::ArgumentParser program("HDSSS");
    program.add_argument("-m", "--model").help("Model file path");
    program.add_argument("-s", "--scaling")
        .default_value(1.0f)
        .help("Scaling factor of the model")
        .scan<'g', float>();
    program.add_argument("-b", "--skybox")
        .help(
            "Skybox directory, name the six faces as "
            "[front|back|left|right|top|bottom].jpg");

    program.add_argument("-c", "--config").help("JSON config file path");
    try {
        program.parse_args(argc, argv);
    } catch (const std::runtime_error& err) {
        std::cerr << err.what() << endl;
        std::cout << program;
        exit(1);
    }
    float scaling = program.get<float>("--scaling");

    string modelPath, skyboxDir;

    // override config with command line arguments
    if (auto path = program.present<string>("-m")) {
        modelPath = *path;
    }
    if (auto path = program.present<string>("-b")) {
        skyboxDir = *path;
    }

    RenderLoo app(1920, 1080,
                  skyboxDir.length() == 0 ? nullptr : skyboxDir.c_str());
    loadScene(app, modelPath.c_str());
    app.run();
}