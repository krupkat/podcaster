// SPDX-FileCopyrightText: 2024 Tomas Krupka
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "podcaster/dependencies.h"
#include "podcaster/panels/simple_window.h"

namespace podcaster {

class ShowLicenseWindow
    : public SimpleWindow<Dependency, WindowTraits::kScrollEvents> {
  const char* Title() const override { return "License"; }
  Action DrawImpl(const Action& incoming_action) override;
  void OpenImpl(const Dependency& dep) override;

  Dependency dependency_;
  std::string license_text_;
};

}  // namespace podcaster