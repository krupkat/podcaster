import os

from conan import ConanFile
from conan.tools.env import Environment

class CapnProtoEnv(ConanFile):
    settings = "os", "compiler", "build_type", "arch"

    default_options = {
        "capnproto/*:shared": False,
        "capnproto/*:with_openssl": False,
    }

    def requirements(self):
        self.requires("capnproto/1.0.2")

    def generate(self):
        env = Environment()
        env.append_path("PATH", os.path.join(self.dependencies["capnproto"].package_folder, 'bin'))
        envvars = env.vars(self)
        envvars.save_script("capnproto_env")
