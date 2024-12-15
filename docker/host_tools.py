import os

from conan import ConanFile
from conan.tools.env import Environment

class CapnProtoEnv(ConanFile):
    settings = "os", "compiler", "build_type", "arch"

    default_options = {
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
        self.requires("grpc/1.67.1")
        self.requires("upx/4.2.1")


    def generate(self):
        env = Environment()
        grpc = self.dependencies["grpc"]
        env.append_path("PATH", os.path.join(grpc.package_folder, 'bin'))
        protobuf = grpc.dependencies["protobuf"]
        env.append_path("PATH", os.path.join(protobuf.package_folder, 'bin'))
        upx = self.dependencies["upx"]
        env.append_path("PATH", os.path.join(upx.package_folder, 'bin'))
        envvars = env.vars(self)
        envvars.save_script("host_tools_env")
