# syntax=docker/dockerfile:1-labs

ARG BASE_IMAGE=podcaster:native

FROM ${BASE_IMAGE} AS dev

SHELL ["/bin/bash", "-c"]

WORKDIR /build
COPY CMakeLists.txt conanfile.py LICENSE changelog.md .

# exclude generated files
COPY --exclude=podcaster/dependencies.h \
     --exclude=podcaster/licenses \
    podcaster podcaster

RUN conan install . --build missing && \
    cmake --preset conan-release -DCMAKE_INSTALL_PREFIX=/usr && \
    DESTDIR=AppDir cmake --build --preset conan-release --target install/strip

RUN linuxdeploy --appdir build/Release/AppDir --output appimage

ENTRYPOINT ["/bin/bash"]