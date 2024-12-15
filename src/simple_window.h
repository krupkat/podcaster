#pragma once

#include <imgui.h>

#include "action.h"

namespace podcaster {

template <typename TState>
class SimpleWindow {
 public:
  void Open(const TState& state) {
    open_ = true;
    scroll_up_ = true;
    OpenImpl(state);
  }

  Action Draw(const Action& incoming_action) {
    Action action = {};
    if (open_) {
      const float kPadding = 10.0f;
      const ImGuiViewport* viewport = ImGui::GetMainViewport();
      ImGui::SetNextWindowPos(
          ImVec2(viewport->Pos.x + kPadding, viewport->Pos.y + kPadding),
          ImGuiCond_Always);
      ImGui::SetNextWindowSize(ImVec2(viewport->Size.x - kPadding * 2,
                                      viewport->Size.y - kPadding * 2),
                               ImGuiCond_Always);

      if (scroll_up_) {
        ImGui::SetNextWindowScroll(ImVec2(-1.0f, 0.0f));
        scroll_up_ = false;
      }

      if (ImGui::Begin(Title(), nullptr,
                       ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse |
                           ImGuiWindowFlags_NoMove |
                           ImGuiWindowFlags_NoSavedSettings)) {
        DrawImpl(incoming_action);

        if (ImGui::Button("Close")) {
          open_ = false;
        }
        if (incoming_action.type == ActionType::kScrollUp) {
          ImGui::SetScrollY(ImGui::GetScrollY() - 100);
        }
        if (incoming_action.type == ActionType::kScrollDown) {
          ImGui::SetScrollY(ImGui::GetScrollY() + 100);
        }
        if (incoming_action.type == ActionType::kBack) {
          open_ = false;
        }
      }
      ImGui::End();
    }
    return action;
  }

 private:
  virtual const char* Title() const = 0;
  virtual void OpenImpl(const TState& state) = 0;
  virtual Action DrawImpl(const Action& incoming_action) = 0;

  bool open_ = false;
  bool scroll_up_ = false;
};

}  // namespace podcaster