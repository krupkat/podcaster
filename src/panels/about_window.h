#pragma once

#include <filesystem>

#include "panels/simple_window.h"

namespace podcaster {

class AboutWindow : public SimpleWindow<NoState, WindowTraits::kScrollEvents> {
 public:
  explicit AboutWindow(const std::filesystem::path& exe_path)
      : exe_path_(exe_path) {}

 private:
  const char* Title() const override { return "About"; }
  Action DrawImpl(const Action& incoming_action) override;
  void OpenImpl() override;

  std::string changelog_;
  std::filesystem::path exe_path_;
};
}  // namespace podcaster