#include "panels/about_window.h"

namespace podcaster {

Action AboutWindow::DrawImpl(const Action& incoming_action) {
  Action action = {};

  ImGui::Text("Podcaster 0.0.1");

  return action;
}

}  // namespace podcaster