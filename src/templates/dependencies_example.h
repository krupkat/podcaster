#pragma once

#include <array>
#include <string>

namespace podcaster {

struct Dependency {
  std::string name;
  std::string version;
  std::string license;
  std::string license_file;
};

const std::array kDependencies{
    Dependency{"abseil", "20240116.2", "Apache-2.0", "licenses/abseil/LICENSE"},
    Dependency{"c-ares", "1.34.3", "MIT", "licenses/c-ares/LICENSE.md"},
    Dependency{"curlpp", "0.8.1.cci.20240530", "MIT",
               "licenses/curlpp/LICENSE"},
    Dependency{"grpc", "1.67.1", "Apache-2.0", "licenses/grpc/LICENSE"},
    Dependency{"imgui", "1.91.5-docking", "MIT", "licenses/imgui/LICENSE.txt"},
    Dependency{"libcurl", "8.9.1", "curl", "licenses/libcurl/COPYING"},
    Dependency{"openssl", "3.3.2", "Apache-2.0",
               "licenses/openssl/LICENSE.txt"},
    Dependency{"protobuf", "5.27.0", "BSD-3-Clause",
               "licenses/protobuf/LICENSE"},
    Dependency{"pugixml", "1.14", "MIT", "licenses/pugixml/LICENSE"},
    Dependency{"re2", "20230301", "BSD-3-Clause", "licenses/re2/LICENSE"},
    Dependency{"sdl", "2.28.5", "Zlib", "licenses/sdl/LICENSE.txt"},
    Dependency{"sdl_mixer", "2.8.0", "Zlib", "licenses/sdl_mixer/LICENSE.txt"},
    Dependency{"spdlog", "1.15.0", "MIT", "licenses/spdlog/LICENSE"},
    Dependency{"tidy-html5", "5.8.0", "HTMLTIDY",
               "licenses/tidy-html5/LICENSE.md"},
    Dependency{"zlib", "1.3.1", "Zlib", "licenses/zlib/LICENSE"},
};

}  // namespace podcaster