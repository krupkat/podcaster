#include "podcaster/panels/cleanup_window.h"

#include <imgui.h>

namespace podcaster {

Action CleanupWindow::DrawImpl(const Action& incoming_action) {
  Action action = {};
  ImGui::TextUnformatted("Cleanup options:");
  ImGui::Spacing();
  if (ImGui::Button("Downloads")) {
    action |= {ActionType::kCleanup, CleanupExtra{CleanupTarget::kDownloads}};
  }
  ImGui::SameLine();
  ImGui::Text("Delete all downloaded episodes from the device.");
  ImGui::Spacing();

  if (ImGui::Button("Database")) {
    action |= {ActionType::kCleanup, CleanupExtra{CleanupTarget::kAll}};
  }
  ImGui::SameLine();
  ImGui::Text("Delete all downloads and the app database.");
  ImGui::Spacing();

  if (action.type != ActionType::kIdle) {
    Close();
  }

  return action;
}

}  // namespace podcaster