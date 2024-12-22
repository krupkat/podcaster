#pragma once

#include "panels/simple_window.h"

namespace podcaster {

class AboutWindow : public SimpleWindow<> {
  const char* Title() const override { return "About"; }
  Action DrawImpl(const Action& incoming_action) override;
};
}  // namespace podcaster