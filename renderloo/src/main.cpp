
#include <glog/logging.h>

#include <argparse/argparse.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <loo/loo.hpp>
#include <nlohmann/json.hpp>
#include <string>
#include "core/RenderLoo.hpp"
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
    app.loadModel(filename);
}

int main(int argc, char* argv[]) {
    loo::initialize(argv[0]);

    argparse::ArgumentParser program("LooRender");
    program.add_argument("-m", "--model").help("Model file path");
    program.add_argument("-b", "--skybox")
        .help(
            "Skybox directory, name the six faces as "
            "[front|back|left|right|top|bottom].jpg");

    try {
        program.parse_args(argc, argv);
    } catch (const std::runtime_error& err) {
        std::cerr << err.what() << endl;
        std::cout << program;
        exit(1);
    }

    string modelPath, skyboxDir;

    // override config with command line arguments
    if (auto path = program.present<string>("-m")) {
        modelPath = *path;
    }
    if (auto path = program.present<string>("-b")) {
        skyboxDir = *path;
    }

    RenderLoo app(1600, 1600);
    loadScene(app, modelPath.c_str());
    if (!skyboxDir.empty()) {
        app.loadSkybox(skyboxDir);
    }
    app.run();
}