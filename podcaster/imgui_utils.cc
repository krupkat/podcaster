// SPDX-FileCopyrightText: 2024 Tomas Krupka
// SPDX-License-Identifier: GPL-3.0-or-later

#include "podcaster/imgui_utils.h"

#include <filesystem>

#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_sdlrenderer2.h>
#include <SDL.h>
#include <SDL_video.h>

#include "podcaster/utils.h"

namespace imgui {

const ImWchar* GetGlyphRangesCzech() {
  static const ImWchar ranges[] = {
      0x0100,
      0x017F,
      0,
  };

  return &ranges[0];
}

ImGuiContext Init(std::filesystem::path root, SDL_Window* window,
                  SDL_Renderer* renderer) {
  IMGUI_CHECKVERSION();
  auto* context = ImGui::CreateContext();
  if (context == nullptr) {
    utils::Throw<std::runtime_error>("Error initializing ImGui context");
  }
  auto imgui_quit =
      utils::DestructorCallback([context] { ImGui::DestroyContext(context); });

  ImGuiIO& imgui_io = ImGui::GetIO();
  auto imgui_ini_file =
      std::make_unique<std::filesystem::path>(root / imgui_io.IniFilename);
  imgui_io.IniFilename = imgui_ini_file->c_str();
  imgui_io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
  imgui_io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  imgui_io.ConfigNavCursorVisibleAlways = true;

  if (not ImGui_ImplSDL2_InitForSDLRenderer(window, renderer)) {
    utils::Throw<std::runtime_error>(
        "Error initializing ImGui platform backend");
  }
  auto imgui_sdl_backend_quit =
      utils::DestructorCallback([] { ImGui_ImplSDL2_Shutdown(); });

  if (not ImGui_ImplSDLRenderer2_Init(renderer)) {
    utils::Throw<std::runtime_error>(
        "Error initializing ImGui renderer backend");
  }
  auto imgui_sdl_renderer_backend_quit =
      utils::DestructorCallback([] { ImGui_ImplSDLRenderer2_Shutdown(); });

  ImVector<ImWchar> ranges;
  ImFontGlyphRangesBuilder builder;
  builder.AddRanges(imgui_io.Fonts->GetGlyphRangesDefault());
  builder.AddRanges(GetGlyphRangesCzech());
  builder.BuildRanges(&ranges);

  float scale = 1.8;
  auto font_path = root / "NotoSans-Regular.ttf";
  auto* font = imgui_io.Fonts->AddFontFromFileTTF(
      font_path.c_str(), std::roundf(13 * scale), nullptr, ranges.Data);
  imgui_io.Fonts->Build();

  ImGui::GetStyle().ScaleAllSizes(scale);

  return {std::move(imgui_ini_file), std::move(imgui_quit),
          std::move(imgui_sdl_backend_quit),
          std::move(imgui_sdl_renderer_backend_quit)};
}
}  // namespace imgui