// SPDX-FileCopyrightText: 2024 Tomas Krupka
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <filesystem>

#include "podcaster/panels/simple_window.h"

namespace podcaster {

struct PathsForAboutWindow {
  std::filesystem::path db_path;
};

class AboutWindow
    : public SimpleWindow<PathsForAboutWindow, WindowTraits::kScrollEvents> {
 public:
  explicit AboutWindow(const std::filesystem::path& exe_path)
      : exe_path_(exe_path) {}

 private:
  const char* Title() const override { return "About"; }
  Action DrawImpl(const Action& incoming_action) override;
  void OpenImpl(const PathsForAboutWindow& paths) override;

  std::string changelog_;
  std::filesystem::path exe_path_;
  std::filesystem::path db_path_;
  std::filesystem::path config_path_;
};
}  // namespace podcaster