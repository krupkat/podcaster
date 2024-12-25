#include "panels/show_license_window.h"

#include <spdlog/spdlog.h>

#include "panels/utils.h"

namespace podcaster {

Action ShowLicenseWindow::DrawImpl(const Action& incoming_action) {
  Action action = {};

  ImGui::Text("%s %s", dependency_.name.c_str(), dependency_.version.c_str());
  ImGui::Separator();
  ImGui::TextWrapped("%s", license_text_.c_str());

  return action;
}

void ShowLicenseWindow::OpenImpl(const Dependency& dep) {
  dependency_ = dep;
  license_text_ = ReadFile(dep.license_file);
}
}  // namespace podcaster