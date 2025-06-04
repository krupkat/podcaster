# syntax=docker/dockerfile:1-labs

ARG BASE_IMAGE=ubuntu:24.04

FROM ${BASE_IMAGE} AS dev

SHELL ["/bin/bash", "-c"]

# setup sources

RUN dpkg --add-architecture arm64
RUN sed -i "s/gpg/gpg\nArchitectures: amd64/g" /etc/apt/sources.list.d/ubuntu.sources
COPY docker/ubuntu_noble_ports.list /etc/apt/sources.list.d/ports.list
RUN apt update

# install dev tools

RUN apt install -y --no-install-recommends \
    gcc make cmake ninja-build python3 python3-pip libfile-copy-recursive-perl gcc-13 g++-13 \
    clangd-18 clang-tidy-18 clang-format-18 g++-13-aarch64-linux-gnu gcc-13-aarch64-linux-gnu

RUN pip3 install conan Jinja2 --break-system-packages

# setup conan

ENV CONAN_HOME=/root/.conan
COPY docker/conan_profile_default /root/.conan/profiles/default
COPY docker/conan_profile_aarch64 /root/.conan/profiles/aarch64
COPY docker/settings_user.yml /root/.conan/settings_user.yml

ARG USE_EXTERNAL_CACHE=0

# populate run mount cache
RUN --mount=source=.cache,target=/cache,rw=true \
    --mount=type=cache,target=/root/.conan/p \
    if [ $USE_EXTERNAL_CACHE -eq 1 ]; then \
        mkdir -p /cache/.conan/p && cp -r /cache/.conan/p /root/.conan; \
    fi

# install build dependencies

RUN --mount=type=tmpfs,target=/build \
    --mount=source=docker/host_tools.py,target=/build/conanfile.py \
    --mount=type=cache,target=/root/.conan/p \
    cd /build && \
    conan install . --build missing && \
    source host_tools_env.sh && \
    ln -s `which grpc_cpp_plugin` /usr/local/bin/grpc_cpp_plugin && \
    ln -s `which protoc` /usr/local/bin/protoc && \
    ln -s `which upx` /usr/local/bin/upx && \
    source deactivate_host_tools_env.sh

ENV PKG_CONFIG_PATH=/usr/lib/aarch64-linux-gnu/pkgconfig:/usr/share/pkgconfig

WORKDIR /build

COPY CMakeLists.txt conanfile.py LICENSE changelog.md .

# exclude generated files
COPY --exclude=podcaster/dependencies.h \
     --exclude=podcaster/licenses \
    podcaster podcaster

ARG CFW="muos"

RUN --mount=type=cache,target=/root/.conan/p \
    conan install . --build missing -o '&:handheld=True' -pr:b=default -pr:h=aarch64 && \
    cmake --preset conan-minsizerel -DCMAKE_INSTALL_PREFIX=/install -DPODCASTER_CFW=${CFW} && \
    cmake --build --preset conan-minsizerel --target install/strip && \
    upx /install/.podcaster/podcasterd && \
    upx /install/.podcaster/podcaster_client

# export run mount cache
WORKDIR /cache

RUN --mount=type=cache,target=/root/.conan/p \
    if [ $USE_EXTERNAL_CACHE -eq 1 ]; then \
        mkdir .conan && cp -r /root/.conan/p .conan; \
    fi

ENTRYPOINT ["/bin/bash"]