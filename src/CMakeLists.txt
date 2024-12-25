cmake_minimum_required(VERSION 3.21)

project(podcaster)

# defaults work on nix
option(PODCASTER_BUILD_IMGUI_EXTRA "Build ImGui with extra features" OFF)
option(PODCASTER_PREFER_PKGCONFIG "Use curlpp from pkgconfig" ON)
option(PODCASTER_HANDHELD_BUILD "Build for handheld devices" OFF)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(SDL2 REQUIRED)
find_package(SDL2_mixer REQUIRED)
find_package(spdlog REQUIRED)
find_package(gRPC REQUIRED)
find_package(Protobuf REQUIRED)
find_package(pugixml REQUIRED)

find_package(imgui REQUIRED)
if (PODCASTER_BUILD_IMGUI_EXTRA)
  add_subdirectory(external/imgui) # conan
else()
  add_library(imgui ALIAS imgui::imgui) # nix
endif()

if(PODCASTER_PREFER_PKGCONFIG)
    find_package(PkgConfig REQUIRED) # nix
    pkg_check_modules(CURLPP REQUIRED IMPORTED_TARGET curlpp)
    add_library(curlpp::curlpp ALIAS PkgConfig::CURLPP)
    pkg_check_modules(TIDY REQUIRED IMPORTED_TARGET tidy)
    add_library(tidy-html5::tidy-html5 ALIAS PkgConfig::TIDY)
else()
    find_package(curlpp REQUIRED) # conan
    find_package(tidy-html5 REQUIRED)
endif()

set(PROTOC_OUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/proto)

add_library(podcaster_api message.proto)
target_link_libraries(podcaster_api PUBLIC gRPC::grpc++)
target_include_directories(podcaster_api PUBLIC ${PROTOC_OUT_DIR})

protobuf_generate(TARGET podcaster_api
  PROTOC_OUT_DIR ${PROTOC_OUT_DIR}
  LANGUAGE cpp
)

protobuf_generate(TARGET podcaster_api
  PROTOC_OUT_DIR ${PROTOC_OUT_DIR}
  LANGUAGE grpc
  PLUGIN protoc-gen-grpc=$<TARGET_FILE:gRPC::grpc_cpp_plugin>
  GENERATE_EXTENSIONS .grpc.pb.h .grpc.pb.cc
)

add_executable(podcaster_client
  main.cc
  sdl_utils.cc
  imgui_utils.cc
  podcaster_gui.cc
  panels/license_window.cc
  panels/show_license_window.cc
  panels/show_more_window.cc
  panels/about_window.cc
)
target_link_libraries(podcaster_client PRIVATE
  podcaster_api
  imgui
  SDL2::SDL2
  SDL2_mixer::SDL2_mixer # TODO: remove
  spdlog::spdlog
)
target_include_directories(podcaster_client PRIVATE .)

if (PODCASTER_HANDHELD_BUILD)
  target_compile_definitions(podcaster_client PRIVATE PODCASTER_HANDHELD_BUILD)
endif()

add_executable(podcaster
  podcaster.cc
  database.cc
  sdl_utils.cc
  tidy_utils.cc
)
target_link_libraries(podcaster PRIVATE
  podcaster_api
  curlpp::curlpp
  protobuf::libprotobuf
  spdlog::spdlog
  pugixml::static
  tidy-html5::tidy-html5
  SDL2::SDL2
  SDL2_mixer::SDL2_mixer
)

install(TARGETS podcaster_client podcaster RUNTIME)