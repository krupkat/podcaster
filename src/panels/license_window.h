#pragma once

#include "panels/show_license_window.h"
#include "panels/simple_window.h"

namespace podcaster {

class LicenseWindow : public SimpleWindow<NoState, WindowTraits::kDefault,
                                          std::tuple<ShowLicenseWindow>> {
  const char* Title() const override { return "Licenses"; }
  Action DrawImpl(const Action& incoming_action) override;
};
}  // namespace podcaster