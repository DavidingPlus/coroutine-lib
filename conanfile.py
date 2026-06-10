from conan import ConanFile
from conan.tools.cmake import cmake_layout, CMakeToolchain


class ExampleRecipe(ConanFile):
    name = "cmake-project"
    version = "1.2.1"
    description = "C/C++ 项目的 CMake 模板。"
    languages = "C++"
    author = "DavidingPlus"
    homepage = "https://github.com/DavidingPlus/cmake-project-template"

    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeDeps"

    options = {
        "with_gtest": [True, False]
    }
    default_options = {
        "with_gtest": True
    }

    def requirements(self):
        if self.options.with_gtest:
            self.requires("gtest/1.12.1")

    def layout(self):
        cmake_layout(self)

    def generate(self):
        tc = CMakeToolchain(self)
        tc.presets_prefix = "cmake-project"
        tc.cache_variables["PACKAGE_VERSION"] = self.version
        tc.cache_variables["with_gtest"] = self.options.with_gtest
        tc.generate()
