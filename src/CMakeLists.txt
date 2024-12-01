cmake_minimum_required(VERSION 3.21)

project(podcaster)

# defaults work on nix
option(PODCASTER_BUILD_IMGUI_EXTRA "Build ImGui with extra features" OFF)
option(PODCASTER_CURLPP_FROM_PKGCONFIG "Use curlpp from pkgconfig" ON)
option(PODCASTER_HANDHELD_BUILD "Build for handheld devices" OFF)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(SDL2 REQUIRED)
find_package(SDL2_mixer REQUIRED)
find_package(spdlog REQUIRED)
find_package(gRPC REQUIRED)
find_package(Protobuf REQUIRED)

find_package(imgui REQUIRED)
if (PODCASTER_BUILD_IMGUI_EXTRA)
  add_subdirectory(external/imgui) # conan
endif()

if(PODCASTER_CURLPP_FROM_PKGCONFIG)
    find_package(PkgConfig REQUIRED) # nix
    pkg_check_modules(CURLPP REQUIRED IMPORTED_TARGET curlpp)
    add_library(curlpp::curlpp ALIAS PkgConfig::CURLPP)
else()
    find_package(curlpp REQUIRED) # conan
endif()

set(PROTOC_OUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/proto)

add_library(schema message.proto)
target_link_libraries(schema PUBLIC gRPC::grpc++)
target_include_directories(schema PUBLIC ${PROTOC_OUT_DIR})

protobuf_generate(TARGET schema
  PROTOC_OUT_DIR ${PROTOC_OUT_DIR}
  LANGUAGE cpp
)

protobuf_generate(TARGET schema
  PROTOC_OUT_DIR ${PROTOC_OUT_DIR}
  LANGUAGE grpc
  PLUGIN protoc-gen-grpc=$<TARGET_FILE:gRPC::grpc_cpp_plugin>
  GENERATE_EXTENSIONS .grpc.pb.h .grpc.pb.cc
)

add_executable(hello_muos main.cc sdl_utils.cc imgui_utils.cc podcaster_gui.cc)
target_link_libraries(hello_muos PRIVATE imgui SDL2_mixer::SDL2_mixer SDL2::SDL2 spdlog::spdlog schema)

if (PODCASTER_HANDHELD_BUILD)
  target_compile_definitions(hello_muos PRIVATE PODCASTER_HANDHELD_BUILD)
endif()

add_executable(podcaster podcaster.cc database.cc)
target_link_libraries(podcaster PRIVATE curlpp::curlpp schema protobuf::libprotobuf)