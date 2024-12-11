# podcaster

Podcast app compatible with [MuOS](https://muos.dev/).

## Developer workflow

1. Build dev container:
```
docker compose build
docker compose run -d podcaster
```

2. Build app:
```
conan install . --build missing -pr:b=default -pr:h=aarch64
cmake --preset conan-minsizerel
cmake --build --preset conan-minsizerel --target install/strip
```