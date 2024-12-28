// SPDX-FileCopyrightText: 2024 Tomas Krupka
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <fstream>
#include <string>

#include <spdlog/spdlog.h>

namespace podcaster {

inline std::string ReadFile(const std::string& filename) {
  std::ifstream file(filename);
  if (!file.is_open()) {
    spdlog::error("Failed to open file: {}", filename);
    return "";
  }

  std::string text;
  std::string line;
  while (std::getline(file, line)) {
    text += line + "\n";
  }

  return text;
}

}  // namespace podcaster