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

const std::array kDependencies {
    {%- for dep in deps %}
    Dependency {
        "{{ dep.name }}",
        "{{ dep.version }}",
        "{{ dep.license }}",
        "{{ dep.license_file }}"
    },
    {%- endfor %}
};

}  // namespace podcaster