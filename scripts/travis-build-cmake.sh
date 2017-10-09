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
    # TODO: figure out why the shared one doesn't work on ubuntu
    ARGS="${ARGS} -DBUILTIN_HDF5=TRUE -DBUILD_SHARED_HDFTUPLE=FALSE"
fi
cmake ${ARGS} ..
make -j 4
sudo make install
popd

