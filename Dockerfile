FROM debian:13-slim AS base

ENV FFMPEG_VERSION=8.1.2 LD_LIBRARY_PATH=/usr/local/lib

RUN apt-get update

# install mediainfo executable
RUN apt-get install -y mediainfo

# build ffmpeg libraries
RUN apt-get install -y make curl gcc g++ nasm yasm && \
  apt-get install -y vim libass-dev libavformat-dev \
  libavutil-dev libavfilter-dev uuid-dev zlib1g-dev

RUN DIR=$(mktemp -d) && cd ${DIR} && \
  curl -fSL --retry 8 --retry-delay 3 --retry-all-errors --connect-timeout 30 -o ffmpeg.tar.gz https://ffmpeg.org/releases/ffmpeg-${FFMPEG_VERSION}.tar.gz && \
  tar zxf ffmpeg.tar.gz && \
  cd ffmpeg-${FFMPEG_VERSION} && \
  ./configure  --enable-version3 --enable-hardcoded-tables --enable-shared --enable-static \
    --enable-small --enable-libass --enable-libfreetype \
    --disable-lzma --enable-pthreads \
    --extra-cflags="-Wno-error=incompatible-pointer-types" && \
  make -j$(nproc) && \
  make install && \
  make distclean && \
  rm -rf ${DIR}

# install git, make, and gcc to build gpac
RUN apt-get install -y git && apt-get install -y make && apt-get install -y gcc && apt-get install -y clang
COPY .git/ /app/.git/

# pull and build gpac
RUN git clone https://github.com/Comcast/gpac-caption-extractor.git && \
    cp -r /gpac-caption-extractor/include/gpac /usr/local/include && \
    cd gpac-caption-extractor && make gpac && cp libgpac.so /usr/local/lib

# build /app/caption-inspector executable
COPY src/ /app/src/
COPY include/ /app/include/
COPY Makefile /app/Makefile
WORKDIR /app
RUN mkdir obj && mkdir python; cd src && make ci_with_gpac

# Gather the runtime shared libraries into one directory with their symlink
# chains intact (cp -a). Copying this directory into the runtime stage preserves
# the links, whereas a wildcard COPY (lib.so.*) resolves them into regular files
# and makes ldconfig warn "is not a symbolic link".
RUN mkdir /runtime-libs && \
    cp -a /usr/local/lib/libavformat.so* /usr/local/lib/libavcodec.so* \
          /usr/local/lib/libavutil.so* /usr/local/lib/libswresample.so* \
          /usr/local/lib/libgpac.so /runtime-libs/

# --- RUNTIME STAGE ---
FROM debian:13-slim AS runtime

ENV FFMPEG_VERSION=8.1.2 LD_LIBRARY_PATH=/usr/local/lib

COPY --from=base /app/caption-inspector /usr/local/bin/
COPY --from=base /runtime-libs/ /usr/local/lib/

RUN apt-get update && \
    apt-get install -y --no-install-recommends \
        mediainfo \
    && rm -rf /var/lib/apt/lists/*

RUN ldconfig

ENV FFMPEG_VERSION=8.1.2 LD_LIBRARY_PATH=/usr/local/lib

ENTRYPOINT ["/usr/local/bin/caption-inspector"]
