{ pkgs ? import <nixpkgs> { }
}:

let
  imgui_sdl = pkgs.imgui.override {
    IMGUI_BUILD_SDL2_BINDING = true;
  };
in

pkgs.mkShell {
  buildInputs = with pkgs; [
    cmake
    ninja
    pkgconf
    SDL2
    SDL2_mixer
    spdlog
    grpc
    protobuf
    curlpp
    curl
    imgui_sdl
    html-tidy
    pugixml
    utf8cpp
    clang-tools_18
    gdb
    conan
    catch2_3
    (python3.withPackages (pkgs: with pkgs; [
      autopep8
      jinja2
    ]))
  ];

  shellHook = ''
    mkdir -p podcaster/licenses
    cp podcaster/templates/dependencies_example.h podcaster/dependencies.h
  '';
}
