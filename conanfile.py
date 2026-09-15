from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMakeDeps, cmake_layout


class EDRConan(ConanFile):
    name = "edr_agent"
    version = "0.1.0"

    settings = "os", "compiler", "build_type", "arch"

    def requirements(self):
        self.requires("boost/1.83.0")

    def configure(self):
        self.options["boost"].without_program_options = False
        self.options["boost"].without_asio = False

    def generate(self):
        tc = CMakeToolchain(self)
        tc.generate()
        deps = CMakeDeps(self)
        deps.generate()

    def layout(self):
        cmake_layout(self)
