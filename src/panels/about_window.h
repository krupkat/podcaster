#pragma once

#include "panels/license_window.h"
#include "panels/simple_window.h"

namespace podcaster {

class AboutWindow : public SimpleWindow<podcaster::EmptyState, false,
                                        std::tuple<LicenseWindow>> {
  const char* Title() const override { return "About"; }
  Action DrawImpl(const Action& incoming_action) override;
};
}  // namespace podcaster