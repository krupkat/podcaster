FROM ubuntu:24.04 AS dev

SHELL ["/bin/bash", "-c"]

# setup sources

RUN dpkg --add-architecture arm64
RUN sed -i "s/gpg/gpg\nArchitectures: amd64/g" /etc/apt/sources.list.d/ubuntu.sources
COPY ubuntu_noble_ports.list /etc/apt/sources.list.d/ports.list
RUN apt update

# install dev tools

RUN apt install -y --no-install-recommends \
    gcc make cmake ninja-build python3 python3-pip libfile-copy-recursive-perl gcc-13 g++-13 \
    clangd-18 clang-tidy-18 clang-format-18 g++-13-aarch64-linux-gnu gcc-13-aarch64-linux-gnu

RUN pip3 install conan --break-system-packages

# setup conan

ENV CONAN_HOME=/root/.conan
COPY conan_profile_default /root/.conan/profiles/default
COPY conan_profile_aarch64 /root/.conan/profiles/aarch64

# install build dependencies

RUN --mount=type=tmpfs,target=/build \
    --mount=source=capnproto.py,target=/build/conanfile.py \
    cd /build && \
    conan install . --build missing && \
    source capnproto_env.sh && \
    ln -s `which capnp` /usr/local/bin/capnp && \
    ln -s `which capnpc-c++` /usr/local/bin/capnpc-c++ && \
    source deactivate_capnproto_env.sh

ENV PKG_CONFIG_PATH=/usr/lib/aarch64-linux-gnu/pkgconfig:/usr/share/pkgconfig

RUN --mount=type=tmpfs,target=/build \
    --mount=from=src,source=conanfile.py,target=/build/conanfile.py \
    cd /build && \
    conan install . --build missing -pr:b=default -pr:h=aarch64

ENTRYPOINT ["/bin/bash"]