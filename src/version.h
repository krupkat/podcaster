#pragma once

#include <format>
#include <string>

namespace podcaster::version {

struct Version {
  int major;
  int minor;
  int patch;
};

constexpr Version Current() { return {0, 0, 1}; }

inline std::string ToString() {
  const Version version = Current();
  return std::format("{}.{}.{}", version.major, version.minor, version.patch);
}

inline std::string AppNameWithVersion() {
  const Version version = Current();
  return std::format("Podcaster {}.{}.{}", version.major, version.minor,
                     version.patch);
}

}  // namespace podcaster::version