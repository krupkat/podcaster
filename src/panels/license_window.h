#pragma once

#include "dependencies.h"
#include "panels/simple_window.h"

namespace podcaster {

class LicenseWindow : public SimpleWindow<Dependency, true> {
  const char* Title() const override { return "License"; }
  Action DrawImpl(const Action& incoming_action) override;

  void OpenImpl(const Dependency& dep) override;

  Dependency dependency_;
  std::string license_text_;
};

}  // namespace podcaster