with import <nixpkgs> {};

stdenv.mkDerivation {
  name = "podcaster-shell";

  buildInputs = with pkgs; [
    # basic build tools
    cmake
    pkg-config
    ninja
    clang-tools_18
    gdb
    conan
    zip
    (python3.withPackages (pkgs: with pkgs; [
      autopep8
      jinja2
    ]))
    # libs for sdl
    libGL.dev
    libdrm.dev
    mesa
    libdecor.dev
    # xorg
    xkeyboard_config
    xorg.libX11.dev
    xorg.libXau.dev
    xorg.libXdmcp.dev
    xorg.libfontenc.out
    xorg.libICE.dev
    xorg.libSM.dev
    libuuid.dev
    xorg.libXaw.dev
    xorg.libXext.dev
    xorg.libXpm.dev
    xorg.libXcomposite.dev
    xorg.libXcursor.dev
    xorg.libXrender.dev
    xorg.libXdamage.dev
    xorg.libXi.dev
    xorg.libXinerama.dev
    xorg.libxkbfile.dev
    xorg.libXrandr.dev
    xorg.libXres.dev
    xorg.libXScrnSaver.out
    xorg.libXtst.out
    xorg.libXv.dev
    xorg.libXxf86vm.dev
    xorg.xcbutilwm.dev
    xorg.xcbutilimage.dev
    xorg.xcbutilkeysyms.dev
    xorg.xcbutilrenderutil.dev
    xorg.xcbutil.dev
    xcb-util-cursor.dev
    xorg.xmodmap
    xorg.xev
    # wayland
    libffi.dev
    expat.dev
    libxml2.dev
    wayland-scanner
    wayland-protocols
    wayland.dev
    wayland
    expat
  ];

  shellHook = ''
    export CONAN_HOME=/home/tom/dev/cpp/podcaster/.conan
    mkdir -p $CONAN_HOME/profiles
    cp nix/conan_default $CONAN_HOME/profiles/default
    cp nix/settings_user.yml $CONAN_HOME/settings_user.yml

    echo "tools.cmake:cmake_program=${lib.getExe cmake}" >> $CONAN_HOME/profiles/default
  '';
}
