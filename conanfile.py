from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMakeDeps, cmake_layout


class ChallengeConan(ConanFile):
    name = "challenge"
    version = "1.0.0"
    settings = "os", "compiler", "build_type", "arch"

    def requirements(self):
        # Boost.Log is used by the existing plugin
        self.requires("boost/1.84.0")
        # GTest for the test binary
        self.requires("gtest/1.14.0")

    def configure(self):
        # Force all Boost components to build as shared libraries.
        # The task explicitly requires third-party deps linked as shared.
        self.options["boost"].shared = True
        self.options["boost"].without_python = True
        self.options["boost"].without_test = True

        # GTest as static is simple and not a runtime dependency.
        self.options["gtest"].shared = False

    def layout(self):
        cmake_layout(self)

    def generate(self):
        tc = CMakeToolchain(self, generator="Ninja")
        tc.user_presets_path = 'ConanPresets.json'
        tc.generate()

        deps = CMakeDeps(self)
        deps.generate()