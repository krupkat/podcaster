# Tiny Podcaster

Minimalistic podcast player usable on handheld devices.
 - background service that plays audio + gui client app
 - specify podcast subscriptions in a config file
 - play / pause / resume
 - persistent playback when the client app is off

## Built with

[imgui](https://github.com/ocornut/imgui),
[sdl](https://github.com/libsdl-org/SDL),
[sdl_mixer](https://github.com/libsdl-org/SDL_mixer),
[spdlog](https://github.com/gabime/spdlog/),
[curlpp](https://github.com/jpbarrette/curlpp),
[grpc](https://grpc.io/),
[pugixml](https://github.com/zeux/pugixml),
[tidy-html5](https://github.com/htacg/tidy-html5),
[Noto fonts](https://fonts.google.com/noto)

## Build instructions

### NixOS

Run from the repository root:

```
nix-shell nix/default.nix
cmake -G Ninja -B build -S . -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=install
cmake --build build --target install
```

### Cross compile for ARM64

Build the app inside a Ubuntu based dev image:

```
docker build -f docker/Dockerfile .
```

### MuOS

See the example script which build and exports the app for use with the [MuOS](https://muos.dev/) [Archive Manager](https://muos.dev/help/archive).

```
./docker/build_muos_archive.sh
```

### Other distros

It should be possible to build on any distro using the [Conan](https://conan.io/) package manager to install prerequisites, however you'll need to edit `conanfile.py` to enable some disabled SDL features.

```
conan install . --build=missing
```

## License

Distributed under the *GPL-3.0-or-later* license. See the full [license text](https://github.com/krupkat/podcaster/blob/main/LICENSE) for more information.

## Contact

Tomas Krupka - [krupkat.cz](https://krupkat.cz)