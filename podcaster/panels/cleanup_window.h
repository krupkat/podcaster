#pragma once

#include "podcaster/panels/simple_window.h"

namespace podcaster {

class CleanupWindow
    : public SimpleWindow<NoState, WindowTraits::kScrollEvents> {
  const char* Title() const override { return "Cleanup"; }
  Action DrawImpl(const Action& incoming_action) override;
};
}  // namespace podcaster