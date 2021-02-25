#!/usr/bin/env bash

./build_debug.sh && echo -e "\n" && build_debug/src/main/tsv test/test.tsv
