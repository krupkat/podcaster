# Tiny Podcaster

Minimalistic podcast player usable on handheld devices.
 - background service that plays audio + gui client app
 - specify podcast subscriptions in a config file
 - play / pause / resume
 - persistent playback when the client app is off

![Download](https://gist.githubusercontent.com/krupkat/2f44804c3e60d32ff247a1006738fd06/raw/a2dbc9b595fb23a46269d2a68f5bce95fd5805c6/screen1.png) | ![Playback](https://gist.githubusercontent.com/krupkat/2f44804c3e60d32ff247a1006738fd06/raw/a2dbc9b595fb23a46269d2a68f5bce95fd5805c6/screen2.png)
--- | ---

</div>

## Built with

[imgui](https://github.com/ocornut/imgui),
[sdl](https://github.com/libsdl-org/SDL),
[sdl_mixer](https://github.com/libsdl-org/SDL_mixer),
[spdlog](https://github.com/gabime/spdlog/),
[curlpp](https://github.com/jpbarrette/curlpp),
[grpc](https://grpc.io/),
[pugixml](https://github.com/zeux/pugixml),
[tidy-html5](https://github.com/htacg/tidy-html5),
[utfcpp](https://github.com/nemtrif/utfcpp),
[Noto fonts](https://fonts.google.com/noto)

## Install

Binaries for [MuOS](https://muos.dev/) and [Knulli](https://knulli.org/) are available in the [releases](https://github.com/krupkat/podcaster/releases) section.

 - On MuOS, install via the [archive manager](https://muos.dev/installation/archive)
 - On Knulli, unpack the zip file in `/userdata/roms/tools`

## Build instructions

### Generic

Run from the repository root:

```
conan install . --build missing
cmake --preset conan-release -DCMAKE_INSTALL_PREFIX=install
cmake --build --preset conan-release --target install
```

#### Ubuntu prerequisites

```
apt install g++ make cmake pkg-config python3 python3-pip
pip3 install conan Jinja2

conan profile detect
# in ~/.conan2/profile/default set "compiler.cppstd=20"

# inside the repository:
conan install . --build missing \
  -c tools.system.package_manager:mode=install \
  -c tools.system.package_manager:sudo=True
```

#### NixOS prerequisites

```
nix-shell nix/default.nix
```

### MuOS + Knulli

Build dev docker for cross compiling to armv8:

```
./docker/build_dev_base.sh
```

Build distribution specific zip files in the `export` directory:

```
./docker/build_muos_archive.sh
./docker/build_knulli_archive.sh
```

## License

Distributed under the *GPL-3.0-or-later* license. See the full [license text](https://github.com/krupkat/podcaster/blob/main/LICENSE) for more information.

## Contact

Tomas Krupka - [krupkat.cz](https://krupkat.cz)
