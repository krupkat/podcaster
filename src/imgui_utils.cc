#include "imgui_utils.h"

#include <filesystem>

#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_sdlrenderer2.h>
#include <SDL.h>
#include <SDL_video.h>

#include "utils.h"

namespace imgui {

ImGuiContext Init(std::filesystem::path root, SDL_Window* window, SDL_Renderer* renderer) {
  IMGUI_CHECKVERSION();
  auto* context = ImGui::CreateContext();
  if (context == nullptr) {
    utils::Throw<std::runtime_error>("Error initializing ImGui context");
  }
  auto imgui_quit = utils::DestructorCallback([context] { ImGui::DestroyContext(context); });

  ImGuiIO& imgui_io = ImGui::GetIO();
  auto imgui_ini_file = std::make_unique<std::filesystem::path>(root / imgui_io.IniFilename);
  imgui_io.IniFilename = imgui_ini_file->c_str();
  imgui_io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
  imgui_io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

  if (not ImGui_ImplSDL2_InitForSDLRenderer(window, renderer)) {
    utils::Throw<std::runtime_error>("Error initializing ImGui platform backend");
  }
  auto imgui_sdl_backend_quit = utils::DestructorCallback([] { ImGui_ImplSDL2_Shutdown(); });

  if (not ImGui_ImplSDLRenderer2_Init(renderer)) {
    utils::Throw<std::runtime_error>("Error initializing ImGui renderer backend");
  }
  auto imgui_sdl_renderer_backend_quit =
      utils::DestructorCallback([] { ImGui_ImplSDLRenderer2_Shutdown(); });

  return {std::move(imgui_ini_file), std::move(imgui_quit), std::move(imgui_sdl_backend_quit),
          std::move(imgui_sdl_renderer_backend_quit)};
}
}  // namespace imgui