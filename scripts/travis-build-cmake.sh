#!/usr/bin/env bash

if [[ $- == *i* ]]; then
    echo "don't source me!" >&2
    return 1
fi

set -eu

if [[ -d build ]]; then
    echo "removing old build directory" >&2
    rm -r build
fi
mkdir build

pushd .
cd build
ARGS='-DROOT_DEPENDANTS=OFF'
if [[ ${MINIMAL+x} ]]; then
    ARGS="${ARGS} -DH5_LOC=BUILTIN"
fi
cmake ${ARGS} ..
make -j 4
sudo make install
popd

