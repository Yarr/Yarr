#!/bin/bash

scripts/test_tunings.sh || exit 1
CTEST_OUTPUT_ON_FAILURE=1 cmake --build build --target test || exit 1
