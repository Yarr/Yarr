#!/bin/bash

usage() { echo "Usage: $0 [-j <number of jobs>]" 1>&2; exit 1; }
jobs=${nproc}

while getopts "j:" o; do
    case "${o}" in
        j)
            jobs=${OPTARG}
            ;;
        *)
            usage
            ;;
    esac
done

mkdir checks
cmake3 -S . -B build
CodeChecker version
CodeChecker log -b  "cmake3 --build build -- -j ${jobs}" -o  checks/compilation_cmds.json
scripts/fix_cdb.py > checks/compilation_cmds_filtered.json 
CodeChecker analyze checks/compilation_cmds_filtered.json -i scripts/code_checker.ignore -j ${jobs} -o checks/results \
  --analyzers clang-tidy \
  --enable=modernize-use-nullptr \
  --enable=modernize-use-using \
  --enable=modernize-deprecated-headers \
  --enable=readability-avoid-const-params-in-decls \
  --enable=readability-make-member-function-const \
  --enable=readability-redundant-smartptr-get \
  --enable=modernize-use-equals-delete \
  --enable=modernize-use-equals-default \
  --enable=modernize-use-auto \
  --enable=modernize-make-unique \
  --enable=modernize-make-shared \
  --enable=modernize-pass-by-value \
  --enable=performance-unnecessary-value-param \
  --enable=performance-move-const-arg \
  --enable=performance-for-range-copy  \
  --enable=performance-unnecessary-copy-initialization \
  --enable=cppcoreguidelines-pro-type-member-init \
  --tidy-config scripts/tidy.config

CodeChecker parse --trim-path-prefix $(pwd) -e html checks/results -o checks/html
CodeChecker parse --trim-path-prefix $(pwd) -e codeclimate checks/results > gl-code-quality-report.json

# Previous "parse" commands have non-zero exit code
# Explicitly succeed for CI
exit 0
