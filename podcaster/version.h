// SPDX-FileCopyrightText: 2024 Tomas Krupka
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string>

#include <spdlog/fmt/fmt.h>

namespace podcaster::version {

struct Version {
  int major;
  int minor;
  int patch;
};

constexpr Version Current() { return {0, 4, 0}; }

inline std::string ToString() {
  const Version version = Current();
  return fmt::format("{}.{}.{}", version.major, version.minor, version.patch);
}

inline std::string AppNameWithVersion() {
  const Version version = Current();
  return fmt::format("Tiny Podcaster {}.{}.{}", version.major, version.minor,
                     version.patch);
}

}  // namespace podcaster::version