# podcaster

Podcast app compatible with [MuOS](https://muos.dev/).

## Developer workflow

1. Build dev container:
```
docker compose build
docker compose -d run
```

2. Attach to running container (Dev Containers)

3. Build app:
```
conan install . --build missing -pr:b=default -pr:h=aarch64
cmake --preset conan-release
cmake --build --preset conan-release
```