#!/bin/bash
# build-deps.sh <prefix>: the libraries ECCE.app bundles, built from source
# for MACOSX_DEPLOYMENT_TARGET (default 11.0) and the machine's own
# architecture.  Homebrew bottles carry the runner's macOS version as their
# minimum, so they cannot give an app that runs on older systems (#133).
# All libraries are shared and install with absolute install names, which
# bundle_libs.py then rewrites.  CI caches <prefix> keyed on this file.
set -euo pipefail
P=$1
mkdir -p "$P"
P=$(cd "$P" && pwd)
export MACOSX_DEPLOYMENT_TARGET=${MACOSX_DEPLOYMENT_TARGET:-11.0}
JOBS=$(sysctl -n hw.ncpu)
ARCH=$(uname -m)
W=$(mktemp -d)
trap 'rm -rf "$W"' EXIT
cd "$W"

fetch() { # url dir
  echo "== fetch $1"
  curl -fsSL --retry 3 "$1" -o src.tar
  mkdir -p "$2"
  tar -xf src.tar -C "$2" --strip-components=1
  rm -f src.tar
}
cm() { # srcdir extra cmake args...
  local s=$1; shift
  cmake -G Ninja -S "$s" -B "$s/build" -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="$P" -DCMAKE_PREFIX_PATH="$P" \
    -DCMAKE_OSX_DEPLOYMENT_TARGET="$MACOSX_DEPLOYMENT_TARGET" \
    -DCMAKE_INSTALL_NAME_DIR="$P/lib" -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
    -DBUILD_SHARED_LIBS=ON "$@"
  cmake --build "$s/build" -j "$JOBS"
  cmake --install "$s/build"
}

# openssl: libssh and the broker
fetch https://github.com/openssl/openssl/releases/download/openssl-3.5.4/openssl-3.5.4.tar.gz openssl
( cd openssl
  ./Configure "darwin64-$( [ "$ARCH" = arm64 ] && echo arm64 || echo x86_64 )-cc" shared \
    --prefix="$P" --openssldir="$P/ssl" --libdir=lib no-tests no-docs \
    "-mmacosx-version-min=$MACOSX_DEPLOYMENT_TARGET"
  make -j "$JOBS" build_libs && make install_dev install_runtime_libs ) > openssl.log 2>&1 \
  || { tail -50 openssl.log; exit 1; }

fetch https://github.com/DaveGamble/cJSON/archive/refs/tags/v1.7.19.tar.gz cjson
cm cjson -DENABLE_CJSON_TEST=OFF -DENABLE_CJSON_UNINSTALL=OFF

fetch https://mosquitto.org/files/source/mosquitto-2.1.2.tar.gz mosquitto
cm mosquitto -DOPENSSL_ROOT_DIR="$P" -DWITH_TESTS=OFF -DWITH_WEBSOCKETS=OFF \
  -DWITH_CLIENTS=OFF -DWITH_PLUGINS=OFF -DWITH_DOCS=OFF -DWITH_LTO=OFF \
  -DWITH_CTRL_SHELL=OFF -DWITH_SRV=OFF

fetch https://www.libssh.org/files/0.11/libssh-0.11.3.tar.xz libssh
cm libssh -DOPENSSL_ROOT_DIR="$P" -DWITH_EXAMPLES=OFF -DWITH_SERVER=OFF \
  -DUNIT_TESTING=OFF -DCLIENT_TESTING=OFF -DWITH_GSSAPI=OFF

fetch https://archive.apache.org/dist/xerces/c/3/sources/xerces-c-3.3.0.tar.xz xerces
cm xerces -Dnetwork=OFF

fetch https://github.com/freetype/freetype/archive/refs/tags/VER-2-13-3.tar.gz freetype
cm freetype -DFT_DISABLE_BZIP2=ON -DFT_DISABLE_PNG=ON -DFT_DISABLE_HARFBUZZ=ON \
  -DFT_DISABLE_BROTLI=ON

fetch https://github.com/coin3d/coin/releases/download/v4.0.10/coin-4.0.10-src.tar.gz coin
cm coin -DCOIN_BUILD_SHARED_LIBS=ON -DCOIN_BUILD_TESTS=OFF -DHAVE_SOUND=OFF \
  -DCOIN_HAVE_JAVASCRIPT=OFF

# wxWidgets 3.2 (the version ECCE is written for); the image libraries
# are wx's own copies, so nothing else is needed at run time.
fetch https://github.com/wxWidgets/wxWidgets/releases/download/v3.2.8/wxWidgets-3.2.8.tar.bz2 wx
( cd wx
  ./configure --prefix="$P" --enable-shared --enable-unicode --with-cocoa \
    --with-opengl --with-macosx-version-min="$MACOSX_DEPLOYMENT_TARGET" \
    --with-libpng=builtin --with-libjpeg=builtin --with-libtiff=builtin \
    --with-zlib=builtin --with-expat=builtin --with-regex=builtin \
    --disable-webview --disable-mediactrl --without-libcurl
  make -j "$JOBS" && make install ) > wx.log 2>&1 \
  || { tail -60 wx.log; exit 1; }
echo "deps installed in $P"
