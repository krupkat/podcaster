{ pkgs ? import <nixpkgs> { }
}:

let
  imgui_docking = (pkgs.imgui.overrideAttrs (oldAttrs: rec {
      version = "1.91.5-docking";

      src = pkgs.fetchFromGitHub {
        owner = "ocornut";
        repo = "imgui";
        rev = "v${version}";
        hash = "sha256-6VOs7a31bEfAG75SQAY2X90h/f/HvqZmN615WXYkUOA=";
      };
    })).override {
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
    pugixml
    clang-tools_18
  ];

}
