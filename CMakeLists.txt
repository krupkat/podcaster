cmake_minimum_required(VERSION 3.21)

project(podcaster)

option(BUILD_TESTING "Build tests" OFF)
option(PODCASTER_HANDHELD_BUILD "Build for handheld devices" OFF)
set(PODCASTER_CFW "muos" CACHE STRING "Custom firmware to build for (muos, knulli)")

include(CTest)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(imgui REQUIRED)
find_package(SDL2 REQUIRED)
find_package(SDL2_mixer REQUIRED)
find_package(spdlog REQUIRED)
find_package(curlpp REQUIRED)
find_package(gRPC REQUIRED)
find_package(Protobuf REQUIRED)
find_package(pugixml REQUIRED)
find_package(tidy-html5 REQUIRED)
find_package(utf8cpp REQUIRED)

add_subdirectory(podcaster/external/imgui)

set(PROTOC_OUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/proto)
file(MAKE_DIRECTORY ${PROTOC_OUT_DIR})

add_library(podcaster_api podcaster/message.proto)
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
  podcaster/main.cc
  podcaster/sdl_utils.cc
  podcaster/imgui_utils.cc
  podcaster/podcaster_gui.cc
  podcaster/panels/license_window.cc
  podcaster/panels/show_license_window.cc
  podcaster/panels/show_more_window.cc
  podcaster/panels/about_window.cc
  podcaster/panels/config_window.cc
  podcaster/panels/cleanup_window.cc
)
target_link_libraries(podcaster_client PRIVATE
  podcaster_api
  imgui
  SDL2::SDL2
  SDL2_mixer::SDL2_mixer # TODO: remove
  spdlog::spdlog
)
target_include_directories(podcaster_client PRIVATE .)

add_library(podcaster_impl STATIC
  podcaster/podcaster_impl.cc
  podcaster/database.cc
  podcaster/sdl_utils.cc
  podcaster/tidy_utils.cc
  podcaster/xml_utils.cc
)

target_link_libraries(podcaster_impl PUBLIC
  podcaster_api
  curlpp::curlpp
  protobuf::libprotobuf
  spdlog::spdlog
  pugixml::static
  tidy-html5::tidy-html5
  SDL2::SDL2
  SDL2_mixer::SDL2_mixer
  utf8cpp
)

target_include_directories(podcaster_impl PUBLIC .)

add_executable(podcasterd podcaster/podcaster.cc)
target_link_libraries(podcasterd PRIVATE podcaster_impl)

if (BUILD_TESTING)
  find_package(Catch2 REQUIRED)
  include(Catch)

  add_executable(podcaster_impl_test podcaster/podcaster_impl_test.cc)
  target_link_libraries(podcaster_impl_test PRIVATE podcaster_impl Catch2::Catch2WithMain)
  catch_discover_tests(podcaster_impl_test)

  add_executable(sdl_utils_test podcaster/sdl_utils_test.cc podcaster/sdl_utils.cc)
  target_link_libraries(sdl_utils_test PRIVATE SDL2::SDL2 spdlog::spdlog Catch2::Catch2WithMain)
  target_include_directories(sdl_utils_test PRIVATE .)
  catch_discover_tests(sdl_utils_test)
endif()

if (PODCASTER_HANDHELD_BUILD)
  target_compile_definitions(podcaster_client PRIVATE PODCASTER_HANDHELD_BUILD)
  target_compile_definitions(podcaster_impl PUBLIC PODCASTER_HANDHELD_BUILD)
endif()

set(CMAKE_INSTALL_BINDIR ".")

install(TARGETS
  podcaster_client
  podcasterd
  RUNTIME
)
install(DIRECTORY
  "podcaster/licenses"
  DESTINATION "."
)
install(FILES
  "podcaster/assets/OFL.txt"
  DESTINATION "./licenses"
)
install(FILES
  "LICENSE"
  DESTINATION "./licenses"
  RENAME "gpl.txt"
)
install(FILES
  "changelog.md"
  "podcaster/assets/NotoSans-Regular.ttf"
  DESTINATION "."
)
if (PODCASTER_CFW STREQUAL "muos")
  install(FILES
    "podcaster/assets/Podcaster_muos.sh"
    DESTINATION "."
    RENAME "mux_launch.sh"
  )
endif()
if (PODCASTER_CFW STREQUAL "knulli")
  install(FILES
    "podcaster/assets/Podcaster_knulli.sh"
    DESTINATION "."
    RENAME "Podcaster.sh"
  )
  install(FILES
    "podcaster/assets/podcaster.png"
    DESTINATION "images"
  )
endif()