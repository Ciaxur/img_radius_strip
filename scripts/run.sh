#!/usr/bin/env bash
set -e

CUR_DIR="$(dirname $0)"

# Build
$CUR_DIR/build.sh "$1"

# Pass remaining args into the application.
shift
time ./app "$@"
rm app
