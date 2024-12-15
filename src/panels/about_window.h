#pragma once

#include "panels/simple_window.h"

namespace podcaster {

class AboutWindow : public SimpleWindow<bool> {
  const char* Title() const override { return "About"; }

  void OpenImpl(const bool& extra) override {}

  Action DrawImpl(const Action& incoming_action) override;
};
}  // namespace podcaster