# syntax=docker/dockerfile:1-labs

ARG BASE_IMAGE=podcaster:cross

FROM ${BASE_IMAGE} AS dev

SHELL ["/bin/bash", "-c"]

WORKDIR /build
COPY CMakeLists.txt conanfile.py LICENSE changelog.md .

# exclude generated files
COPY --exclude=podcaster/dependencies.h \
     --exclude=podcaster/licenses \
    podcaster podcaster

ARG CFW="muos"

RUN conan install . --build missing -o '&:handheld=True' -pr:b=default -pr:h=aarch64 && \
    cmake --preset conan-minsizerel -DCMAKE_INSTALL_PREFIX=/install -DPODCASTER_CFW=${CFW} && \
    cmake --build --preset conan-minsizerel --target install/strip && \
    upx /install/podcasterd && \
    upx /install/podcaster_client

ENTRYPOINT ["/bin/bash"]