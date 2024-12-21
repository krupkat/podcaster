#include "panels/license_window.h"

#include <fstream>

#include <spdlog/spdlog.h>

namespace podcaster {

namespace {
std::string ReadFile(const std::string& filename) {
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

}  // namespace

Action LicenseWindow::DrawImpl(const Action& incoming_action) {
  Action action = {};

  ImGui::Text("%s %s,", dependency_.name.c_str(), dependency_.version.c_str());
  ImGui::Separator();
  ImGui::TextWrapped("License: %s", license_text_.c_str());

  return action;
}

void LicenseWindow::OpenImpl(const Dependency& dep) {
  dependency_ = dep;
  license_text_ = ReadFile(dep.license_file);
}
}  // namespace podcaster