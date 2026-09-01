# Multi-stage build:
#   toolchain - compilers plus FFmpeg and GPAC built from source (shared by build and test)
#   build     - compiles caption-inspector and gathers its runtime libraries
#   test      - runs the test suite (docker build --target test; see test/Makefile)
#   runtime   - slim image with just the executable (the default target)

FROM debian:13-slim AS toolchain

# Pinned inputs. The FFmpeg checksum is of the release tarball, whose signature
# verifies against the FFmpeg release signing key (FCF986EA15E6E293A5644F10B4322F04D67658D8).
ARG FFMPEG_VERSION=8.1.2
ARG FFMPEG_SHA256=32faba5ef67340d54724941eae1425580791195312a4fd13bf6f820a2818bf22
ARG GPAC_COMMIT=bf3a79413e2d83e3b66429bfbf9bb0ef0fd52950

RUN apt-get update && \
    apt-get install -y --no-install-recommends \
        ca-certificates curl git make gcc clang nasm pkg-config uuid-dev zlib1g-dev && \
    rm -rf /var/lib/apt/lists/*

# FFmpeg from source, pinned so that builds and test results do not depend on Debian's FFmpeg version.
RUN DIR=$(mktemp -d) && cd ${DIR} && \
    curl -fSL --retry 8 --retry-delay 3 --retry-all-errors --connect-timeout 30 \
        -o ffmpeg.tar.gz https://ffmpeg.org/releases/ffmpeg-${FFMPEG_VERSION}.tar.gz && \
    echo "${FFMPEG_SHA256}  ffmpeg.tar.gz" | sha256sum -c - && \
    tar zxf ffmpeg.tar.gz && \
    cd ffmpeg-${FFMPEG_VERSION} && \
    ./configure --enable-version3 --enable-hardcoded-tables --enable-shared --enable-static \
        --enable-small --disable-lzma --enable-pthreads \
        --extra-cflags="-Wno-error=incompatible-pointer-types" && \
    make -j$(nproc) && \
    make install && \
    cd / && rm -rf ${DIR} && \
    ldconfig

# GPAC (for MOV support), as modified for caption extraction.
RUN git clone https://github.com/Comcast/gpac-caption-extractor.git /gpac-caption-extractor && \
    git -C /gpac-caption-extractor checkout -q ${GPAC_COMMIT} && \
    cp -r /gpac-caption-extractor/include/gpac /usr/local/include && \
    make -C /gpac-caption-extractor gpac && \
    cp /gpac-caption-extractor/libgpac.so /usr/local/lib && \
    rm -rf /gpac-caption-extractor && \
    ldconfig

# --- BUILD STAGE ---
FROM toolchain AS build

COPY src/ /app/src/
COPY include/ /app/include/
COPY Makefile /app/Makefile
WORKDIR /app
RUN mkdir obj python && make -C src ci_with_gpac

# Gather the runtime shared libraries into one directory with their symlink
# chains intact (cp -a). Copying this directory into the runtime stage preserves
# the links, whereas a wildcard COPY (lib.so.*) resolves them into regular files
# and makes ldconfig warn "is not a symbolic link".
RUN mkdir /runtime-libs && \
    cp -a /usr/local/lib/libavformat.so* /usr/local/lib/libavcodec.so* \
          /usr/local/lib/libavutil.so* /usr/local/lib/libswresample.so* \
          /usr/local/lib/libgpac.so /runtime-libs/

# --- TEST STAGE ---
# Built from the toolchain (not the build stage) so the source copy is the only
# layer that changes between runs. Results are written to /files.
FROM toolchain AS test

RUN apt-get update && \
    apt-get install -y --no-install-recommends python3 python3-pytest mediainfo npm && \
    rm -rf /var/lib/apt/lists/* && \
    npm install -g xunit-viewer@10.6.1

COPY . /app/caption-inspector/
WORKDIR /app/caption-inspector

ENTRYPOINT ["/usr/bin/make", "-C", "/app/caption-inspector/test", "docker-test"]

# --- RUNTIME STAGE ---
FROM debian:13-slim AS runtime

# mediainfo is used to detect drop-frame timecode.
RUN apt-get update && \
    apt-get install -y --no-install-recommends mediainfo && \
    rm -rf /var/lib/apt/lists/*

COPY --from=build /runtime-libs/ /usr/local/lib/
COPY --from=build /app/caption-inspector /usr/local/bin/

# Fail the build if any shared library is missing.
RUN ldconfig && \
    if ldd /usr/local/bin/caption-inspector /usr/local/lib/libgpac.so | grep "not found"; then false; fi

ENTRYPOINT ["/usr/local/bin/caption-inspector"]
