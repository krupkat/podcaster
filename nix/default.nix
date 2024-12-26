{ pkgs ? import <nixpkgs> { }
}:

let
  imgui_docking = pkgs.imgui.override {
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
    imgui_docking
    html-tidy
    pugixml
    clang-tools_18
    gdb
    conan
    (python3.withPackages (pkgs: with pkgs; [
      autopep8
      jinja2
    ]))
  ];

}
