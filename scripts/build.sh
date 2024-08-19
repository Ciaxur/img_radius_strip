#!/usr/bin/env bash

# Grab all include source files.
INCLUDE_SRCS="$(find ./include -name '*.cc' | tr '\n' ' ')"

# Generic build script, which builds ONE source file, with all warnings enabled.
bear -- g++ \
  -o app \
  --std=c++20 \
  -O3 \
  -I ./include \
  -g \
  -lpthread \
  -lfmt \
  $(pkg-config --cflags --libs libpng) \
  -Wall \
  -Wextra \
  -Wno-unused-parameter \
  $INCLUDE_SRCS \
  "$@"

