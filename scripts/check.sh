#/usr/bin/env bash

cppcheck \
  --error-exitcode=1 \
  --enable=all \
  --suppress=missingIncludeSystem \
  --suppress=checkersReport \
  --language=c++ \
  --std=c++17 \
  "$1"

