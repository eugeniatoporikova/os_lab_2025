#!/bin/sh
# The test library stops each child immediately after fork.
set -eu
cd "$(dirname "$0")/../src"
make parallel_min_max
library=$(mktemp /tmp/lab4-stop-XXXXXX.so)
trap 'rm -f "$library"' EXIT
gcc -shared -fPIC ../tests/stop_child.c -ldl -o "$library"
for mode in pipe files; do
    extra=""
    if [ "$mode" = files ]; then extra="--by_files"; fi
    status=0
    LD_PRELOAD="$library" ./parallel_min_max --seed 1 --array_size 10 \
        --pnum 2 --timeout 1 $extra || status=$?
    echo "$mode: exit code $status (expected 124)"
    test "$status" -eq 124
done
