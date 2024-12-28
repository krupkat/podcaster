#include "podcaster/panels/cleanup_window.h"

#include <imgui.h>

namespace podcaster {

Action CleanupWindow::DrawImpl(const Action& incoming_action) {
  Action action = {};

  if (ImGui::Button("Cleanup downloads")) {
    action |= {ActionType::kCleanup, CleanupExtra{CleanupTarget::kDownloads}};
  }
  ImGui::SameLine();
  ImGui::Text("Remove all downloaded episodes from the device.");

  ImGui::Spacing();

  if (ImGui::Button("Cleanup all")) {
    action |= {ActionType::kCleanup, CleanupExtra{CleanupTarget::kAll}};
  }
  ImGui::SameLine();
  ImGui::Text("Remove all downloads and the database.");

  if (action.type != ActionType::kIdle) {
    Close();
  }

  return action;
}

}  // namespace podcaster