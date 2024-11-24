import os

from conan import ConanFile
from conan.tools.cmake import cmake_layout, CMakeToolchain
from conan.tools.files import copy

class PodcasterRecipe(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeDeps"

    default_options = {
        "sdl/*:shared": True,
        "sdl/*:alsa": False,
        "sdl/*:pulse": False,
        "sdl/*:x11": False,
        "sdl/*:xcursor": False,
        "sdl/*:xinerama": False,
        "sdl/*:xinput": False,
        "sdl/*:xrandr": False,
        "sdl/*:xscrnsaver": False,
        "sdl/*:xshape": False,
        "sdl/*:xvm": False,
        "sdl/*:wayland": False,
        "sdl/*:opengl": False,
        "sdl/*:opengles": True,
        "sdl/*:vulkan": False,
        "sdl_mixer/*:shared": True,
        "sdl_mixer/*:opus": False,
        "capnproto/*:shared": False,
        "capnproto/*:with_openssl": False,
        "imgui/*:shared": False,
        "spdlog/*:shared": False,
        "spdlog/*:use_std_fmt": True,
    }

    def requirements(self):
        self.requires("imgui/1.91.5-docking")
        self.requires("sdl/2.28.5")
        self.requires("sdl_mixer/2.8.0")
        self.requires("spdlog/1.15.0")
        self.requires("capnproto/1.0.2")
        self.requires("curlpp/0.8.1.cci.20240530")

    def generate(self):
        tc = CMakeToolchain(self)
        tc.variables["CMAKE_EXPORT_COMPILE_COMMANDS"] = "ON"
        tc.variables["CMAKE_INTERPROCEDURAL_OPTIMIZATION"] = "ON"
        tc.generate()

        imgui = self.dependencies["imgui"]
        copy(self, pattern="imgui_impl_sdl*",
            dst=os.path.join(self.source_folder, "external", "imgui"),
            src=os.path.join(imgui.package_folder, "res", "bindings"))

    def layout(self):
        cmake_layout(self)
