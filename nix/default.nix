with import <nixpkgs> { };

stdenv.mkDerivation {
  name = "podcaster-shell";

  buildInputs = with pkgs; [
    # basic build tools
    cmake
    pkg-config
    ninja
    clang-tools
    gdb
    conan
    zip
    (python3.withPackages (
      pkgs: with pkgs; [
        autopep8
        jinja2
      ]
    ))
    # libs for sdl
    libGL.dev
    libdrm.dev
    mesa
    libdecor.dev
    # xorg
    xkeyboard_config
    libx11.dev
    libxau.dev
    libxdmcp.dev
    libfontenc.out
    libICE.dev
    libSM.dev
    libuuid.dev
    libxaw.dev
    libxext.dev
    libxpm.dev
    libxcomposite.dev
    libxcursor.dev
    libxrender.dev
    libxdamage.dev
    libxi.dev
    libxinerama.dev
    libxkbfile.dev
    libxrandr.dev
    libxres.dev
    libxscrnsaver.out
    libxtst.out
    libxv.dev
    libxxf86vm.dev
    xcbutilwm.dev
    xcbutilimage.dev
    xcbutilkeysyms.dev
    xcbutilrenderutil.dev
    xcbutil.dev
    xcb-util-cursor.dev
    xmodmap
    xev
    # wayland
    libffi.dev
    expat.dev
    libxml2.dev
    wayland-scanner
    wayland-protocols
    wayland.dev
    wayland
    expat
    yasm
  ];

  shellHook =
    let
      conan_profile = replaceVars ./conan_default {
        gccversion = lib.versions.major (lib.getVersion gcc);
        cmake = lib.getExe cmake;
      };
    in
    ''
      export CONAN_HOME=/home/tom/dev/cpp/podcaster/.conan
      mkdir -p $CONAN_HOME/profiles
      cp ${conan_profile} $CONAN_HOME/profiles/default
      cp ${./settings_user.yml} $CONAN_HOME/settings_user.yml
    '';
}
