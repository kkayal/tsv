#!/usr/bin/env bash

./build_debug.sh && echo -e "\n" && build_debug/src/tsv-bin/tsv test/test.tsv
