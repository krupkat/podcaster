#pragma once

#include <filesystem>
#include <vector>

#include "podcaster/dependencies.h"
#include "podcaster/panels/show_license_window.h"
#include "podcaster/panels/simple_window.h"

namespace podcaster {

class LicenseWindow : public SimpleWindow<NoState, WindowTraits::kDefault,
                                          std::tuple<ShowLicenseWindow>> {
 public:
  explicit LicenseWindow(const std::filesystem::path& exe_path);

 private:
  const char* Title() const override { return "Licenses"; }
  Action DrawImpl(const Action& incoming_action) override;

  std::vector<Dependency> dependencies_;
  std::filesystem::path exe_path_;
};
}  // namespace podcaster