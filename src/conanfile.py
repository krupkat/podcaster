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
        "imgui/*:shared": False,
        "spdlog/*:shared": False,
        "spdlog/*:use_std_fmt": True,
        "grpc/*:codegen": True,
        "grpc/*:cpp_plugin": True,
        "grpc/*:csharp_ext": False,
        "grpc/*:php_plugin": False,
        "grpc/*:node_plugin": False,
        "grpc/*:otel_plugin": False,
        "grpc/*:ruby_plugin": False,
        "grpc/*:csharp_plugin": False,
        "grpc/*:python_plugin": False,
        "grpc/*:with_libsystemd": False,
        "grpc/*:objective_c_plugin": False
    }

    def requirements(self):
        self.requires("imgui/1.91.5-docking")
        self.requires("sdl/2.28.5")
        self.requires("sdl_mixer/2.8.0")
        self.requires("spdlog/1.15.0")
        self.requires("curlpp/0.8.1.cci.20240530")
        self.requires("grpc/1.67.1")
        self.requires("pugixml/1.14")
        self.requires("tidy-html5/5.8.0")

    def generate(self):
        tc = CMakeToolchain(self)
        tc.variables["CMAKE_EXPORT_COMPILE_COMMANDS"] = "ON"
        tc.variables["PODCASTER_BUILD_IMGUI_EXTRA"] = "ON"
        tc.variables["PODCASTER_CURLPP_FROM_PKGCONFIG"] = "OFF"
        tc.variables["PODCASTER_HANDHELD_BUILD"] = "ON"
        tc.generate()

        imgui = self.dependencies["imgui"]
        copy(self, pattern="imgui_impl_sdl*",
            dst=os.path.join(self.source_folder, "external", "imgui"),
            src=os.path.join(imgui.package_folder, "res", "bindings"))

    def layout(self):
        cmake_layout(self)
