set(IMGUI_EXTRA_SOURCES 
  imgui_impl_sdl2.cpp
  imgui_impl_sdlrenderer2.cpp
)

add_library(imgui STATIC ${IMGUI_EXTRA_SOURCES})
target_include_directories(imgui PUBLIC .)
target_link_libraries(imgui PUBLIC imgui::imgui SDL2::SDL2)