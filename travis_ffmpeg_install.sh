#!/usr/bin/env bash

# This Script is only to be used by Travis CI for installing FFMPEG on its VM to build.
# It should not be run from the command line and is not needed to build outside of Travis.
# It builds the same FFmpeg release, with the same options, as the Dockerfile.

set -e

FFMPEG_VERSION=8.1.2
FFMPEG_SHA256=32faba5ef67340d54724941eae1425580791195312a4fd13bf6f820a2818bf22
apt-get update
apt-get install -y make curl gcc g++ nasm git clang pkg-config uuid-dev zlib1g-dev
DIR=$(mktemp -d) && cd ${DIR}
curl -fSL -o ffmpeg.tar.gz https://ffmpeg.org/releases/ffmpeg-${FFMPEG_VERSION}.tar.gz
echo "${FFMPEG_SHA256}  ffmpeg.tar.gz" | sha256sum -c -
tar zxf ffmpeg.tar.gz
cd ffmpeg-${FFMPEG_VERSION}
./configure --enable-version3 --enable-hardcoded-tables --enable-shared --enable-static \
    --enable-small --disable-lzma --enable-pthreads \
    --extra-cflags="-Wno-error=incompatible-pointer-types"
make
make install
ldconfig
git clone https://github.com/Comcast/gpac-caption-extractor.git
cd gpac-caption-extractor && make install
