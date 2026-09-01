#FROM python:3.12-slim
FROM debian:13-slim

ENV FFMPEG_VERSION=8.1.2 LD_LIBRARY_PATH=/usr/local/lib

RUN apt-get update

# install ffmpeg deps
RUN apt-get install -y make curl gcc g++ nasm yasm && \
  apt-get install -y vim libass-dev libavformat-dev \
  libavutil-dev libavfilter-dev uuid-dev zlib1g-dev

# build ffmpeg libraries
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

RUN apt-get install -y git && git config --global --add safe.directory /app/caption-inspector && apt-get install -y make && apt-get install -y clang && apt-get install -y python3-pip && apt-get install -y python3-pytest && apt-get install -y mediainfo && apt-get install -y npm && npm i -g xunit-viewer

# pull and build gpac
RUN git clone https://github.com/Comcast/gpac-caption-extractor.git && \
    cp -r /gpac-caption-extractor/include/gpac /usr/local/include && \
    cd gpac-caption-extractor && make gpac && cp libgpac.so /usr/local/lib


# add directory /app/caption-inspector
WORKDIR /app
RUN mkdir caption-inspector

ENTRYPOINT ["/usr/bin/make", "-C", "/app/caption-inspector/test", "docker-test"]
