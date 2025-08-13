// SPDX-FileCopyrightText: 2024 Tomas Krupka
// SPDX-License-Identifier: GPL-3.0-or-later

#include "podcaster/panels/about_window.h"

#include <imgui.h>

#include "podcaster/panels/utils.h"
#include "podcaster/version.h"

namespace podcaster {

Action AboutWindow::DrawImpl(const Action& incoming_action) {
  Action action = {};

  auto label = version::AppNameWithVersion();
  ImGui::SeparatorText(label.c_str());

  std::filesystem::path debug_logs =
#ifdef PODCASTER_HANDHELD_BUILD
      exe_path_;
#else
      db_path_;
#endif

  ImGui::TextWrapped(
      "Tiny Podcaster is a podcast client for embedded Linux devices.\n - "
      "config file: %s\n - data storage: %s\n - debug logs: %s\n - issues: "
      "https://github.com/krupkat/podcaster/issues",
      config_path_.empty() ? "[service disconnected]" : config_path_.c_str(),
      db_path_.empty() ? "[service disconnected]" : db_path_.c_str(),
      debug_logs.c_str());

  ImGui::SeparatorText("Changelog");
  ImGui::TextWrapped("%s", changelog_.c_str());
  ImGui::Spacing();

  return action;
}

void AboutWindow::OpenImpl(const PathsForAboutWindow& paths) {
  auto changelog_path =
      exe_path_ / ".." / "share" / "podcaster" / "changelog.md";
  changelog_ = ReadFile(changelog_path.string());
  db_path_ = paths.db_path;
  if (not db_path_.empty()) {
    config_path_ = db_path_ / "config.textproto";
  }
};

}  // namespace podcaster