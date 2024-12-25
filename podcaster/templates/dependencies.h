#pragma once

#include <vector>

namespace podcaster {

struct Dependency {
    std::string name;
    std::string version;
    std::string license;
    std::string license_file;
};

std::vector<Dependency> kDependencies = {
    {%- for dep in deps %}
    {
        "{{ dep.name }}",
        "{{ dep.version }}",
        "{{ dep.license }}",
        "{{ dep.license_file }}"
    },
    {%- endfor %}
};

}  // namespace podcaster