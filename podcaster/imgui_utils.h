#include <filesystem>
#include <memory>

#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_sdlrenderer2.h>
#include <SDL.h>

#include "podcaster/utils.h"

namespace imgui {
struct ImGuiContext {
  std::unique_ptr<std::filesystem::path> imgui_ini_file;
  utils::DestructorCallback imgui_quit;
  utils::DestructorCallback imgui_sdl_backend_quit;
  utils::DestructorCallback imgui_sdl_renderer_backend_quit;
};

ImGuiContext Init(std::filesystem::path root, SDL_Window* window,
                  SDL_Renderer* renderer);

template <typename TFunc>
void Render(TFunc render_func, SDL_Renderer* renderer) {
  ImGui_ImplSDLRenderer2_NewFrame();
  ImGui_ImplSDL2_NewFrame();
  ImGui::NewFrame();

  render_func();

  ImGui::Render();
  ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer);
}

template <typename TCallbackType>
void EnableIf(bool condition, TCallbackType callback) {
  if (!condition) {
    ImGui::BeginDisabled();
  }
  callback();
  if (!condition) {
    ImGui::EndDisabled();
  }
}

}  // namespace imgui