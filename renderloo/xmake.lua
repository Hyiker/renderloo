add_rules("mode.debug", "mode.release")

includes("spv2hpp")

add_requires("nativefiledialog-extended")

target("renderloo_lib")
    set_kind("static")
    add_deps("loo", "spv2hpp")
    set_symbols("debug")
    add_packages("nativefiledialog-extended")

    add_includedirs("include", {public = true})
    set_languages("c11", "cxx17")
    set_rules("glsl2hpp", {outputdir = path.join(os.scriptdir(), "include", "shaders"), defines = {"MATERIAL_PBR"}})
    add_files("shaders/*.*", "src/*.cpp")
    remove_files("src/main.cpp")

    add_defines("_CRT_SECURE_NO_WARNINGS")
    set_policy("build.warning", true)
    set_warnings("allextra")

    -- solve msvc unfriendly to unicode and utf8
    if is_plat("windows") then
        add_defines("UNICODE", "_UNICODE")
        add_cxflags("/execution-charset:utf-8", "/source-charset:utf-8", {tools = {"clang_cl", "cl"}})
    end
    add_defines("MATERIAL_PBR")

target_end()

add_requires("argparse 2.9", "nlohmann_json v3.11.2")

target("renderloo")
    set_kind("binary")
    add_deps("renderloo_lib")
    set_languages("c11", "cxx17")
    add_packages("argparse", "nlohmann_json")

    add_files("src/main.cpp")

includes("test")
