import jinja2
import collections

from pathlib import Path

from conan import ConanFile
from conan.tools.cmake import cmake_layout, CMakeToolchain
from conan.tools.files import copy, mkdir


Dependency = collections.namedtuple(
    'Dependency', ['name', 'version', 'license', 'license_file'])


def load_template(template_src):
    jinja_templates = jinja2.Environment(
        loader=jinja2.FileSystemLoader(template_src.parent),
        autoescape=jinja2.select_autoescape(default=False)
    )
    return jinja_templates.get_template(template_src.name)


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
        self.requires("imgui/1.91.5")
        self.requires("sdl/2.28.5")
        self.requires("sdl_mixer/2.8.0")
        self.requires("spdlog/1.15.0")
        self.requires("curlpp/0.8.1.cci.20240530")
        self.requires("grpc/1.67.1")
        self.requires("pugixml/1.14")
        self.requires("tidy-html5/5.8.0")

    def bundled_dependencies(self):
        deps = {
            "sdl": self.dependencies["sdl"],
            "sdl_mixer": self.dependencies["sdl_mixer"],
        }

        # exclude sdl and sdl_mixer subdependencies as we're not bundling those (using system libraries)

        for dependency in self.dependencies.direct_host.values():
            if dependency.ref.name != "sdl" and dependency.ref.name != "sdl_mixer":
                deps[dependency.ref.name] = dependency
                for subdep in dependency.dependencies.host.values():
                    deps[subdep.ref.name] = subdep

        return deps

    def export_licenses(self, deps):
        licenses = {}

        source_dir = Path(self.source_folder) / "podcaster"
        license_dir = source_dir / "licenses"

        mkdir(self, license_dir)
        for name, dep in deps.items():
            dep_license_dir = license_dir / name
            mkdir(self, dep_license_dir)
            copy(self, pattern="*", src=Path(dep.package_folder) /
                 "licenses", dst=dep_license_dir)

            dep_licenses = list(
                entry for entry in dep_license_dir.iterdir() if entry.is_file())
            if len(dep_licenses) == 1:
                licenses[name] = dep_licenses[0].relative_to(source_dir)
            else:
                raise Exception(
                    f"License file for {name} already exists, multiple licenses not supported")

        return licenses

    def generate(self):
        tc = CMakeToolchain(self)
        tc.variables["CMAKE_EXPORT_COMPILE_COMMANDS"] = "ON"
        tc.variables["PODCASTER_BUILD_IMGUI_EXTRA"] = "ON"
        tc.variables["PODCASTER_PREFER_PKGCONFIG"] = "OFF"
        tc.variables["PODCASTER_HANDHELD_BUILD"] = "ON"
        tc.generate()

        # extra imgui files
        imgui = self.dependencies["imgui"]
        copy(self, pattern="imgui_impl_sdl*",
             src=Path(imgui.package_folder) / "res" / "bindings",
             dst=Path(self.source_folder) / "podcaster" / "external" / "imgui")

        # dependency data and license files
        dependencies = self.bundled_dependencies()
        licenses = self.export_licenses(dependencies)

        dependency_data = sorted([
            Dependency(name, dep.ref.version, dep.license, licenses[name])
            for name, dep in dependencies.items()
        ])

        dependencies_header_template = load_template(
            Path(self.source_folder) / "podcaster" / "templates" / "dependencies.h")

        with open(Path(self.source_folder) / "podcaster" / "dependencies.h", "w") as f:
            f.write(dependencies_header_template.render(deps=dependency_data))

    def layout(self):
        cmake_layout(self)
