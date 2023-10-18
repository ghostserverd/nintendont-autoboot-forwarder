#!/bin/sh

NUM_CPUS="$(nproc)"

git config --global --add safe.directory /project
git config --global --add safe.directory /project/Nintendont
git submodule update --init
make -j${NUM_CPUS}
make -j${NUM_CPUS} DEBUG_BUILD=1
