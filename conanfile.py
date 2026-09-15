from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMakeDeps, cmake_layout


class EDRConan(ConanFile):
    name = "edr_agent"
    version = "0.1.0"

    settings = "os", "compiler", "build_type", "arch"

    def requirements(self):
        self.requires("boost-program-options/1.83.0")
        self.requires("boost-asio/1.83.0")

    def generate(self):
        tc = CMakeToolchain(self)
        tc.generate()
        deps = CMakeDeps(self)
        deps.generate()

    def layout(self):
        cmake_layout(self)
