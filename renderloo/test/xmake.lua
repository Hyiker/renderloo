add_requires("gtest 1.12.1")


target("renderloo_test")
    set_kind("binary")
    add_deps("renderloo_lib")
    set_languages("c11", "cxx17")
    add_packages("gtest")

    add_files("*.cpp")
