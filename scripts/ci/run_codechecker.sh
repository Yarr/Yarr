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
echo Running CodeChecker with ${jobs} jobs
mkdir checks
cmake3 -S . -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=On
CodeChecker version
# Shouldn't need to run build to make compile DB
# cmake --build build --parallel ${jobs}
scripts/fix_cdb.py > checks/compilation_cmds_filtered.json
# Checks evaluated but not yet enabled:
#   bugprone-use-after-move          -- high value, no known noise
#   bugprone-spurious-wake-up        -- relevant to threaded DataProcessor/ClipBoard
#   modernize-use-override           -- catches silent breakage on base signature changes
#   modernize-use-emplace            -- low-risk modernization
#   modernize-loop-convert           -- mechanical, needs dedicated pass
#   misc-unused-parameters           -- some noise expected in virtual override signatures
#   readability-identifier-naming    -- ~7700 violations; needs mass rename first
#   readability-magic-numbers        -- too noisy (hardware register addresses/bit masks)
#   cppcoreguidelines-pro-bounds-*   -- too noisy (raw array indexing in pixel data path)
#   clang-format (separate tool)     -- formatting not yet enforced; needs .clang-format config first
CodeChecker analyze checks/compilation_cmds_filtered.json -i scripts/code_checker.ignore -j ${jobs} -o checks/results \
  --analyzers clang-tidy \
  --enable=modernize-use-nullptr \
  --enable=modernize-use-using \
  --enable=modernize-deprecated-headers \
  --enable=readability-make-member-function-const \
  --enable=readability-redundant-smartptr-get \
  --enable=modernize-use-equals-delete \
  --enable=modernize-use-equals-default \
  --enable=modernize-make-unique \
  --enable=modernize-make-shared \
  --enable=modernize-pass-by-value \
  --enable=performance-unnecessary-value-param \
  --enable=performance-move-const-arg \
  --enable=performance-for-range-copy  \
  --enable=performance-unnecessary-copy-initialization \
  --enable=cppcoreguidelines-pro-type-member-init \
  --disable=clang-diagnostic \
  --disable=misc-confusable-identifiers \
  --disable=modernize-use-auto \
  --disable=readability-avoid-const-params-in-decls \
  --disable=performance-trivially-destructible \
  --tidy-config scripts/tidy.config

CodeChecker parse --trim-path-prefix $(pwd) -e html checks/results -o checks/html
CodeChecker parse --trim-path-prefix $(pwd) -e codeclimate checks/results > intermediate_report.json

echo "Stripping comments about external libraries"
jq 'map(select(.location.path | test("^(build|src/libUtil/lm|src/libUtil/catch)") | not))' intermediate_report.json > gl-code-quality-report.json

# Previous "parse" commands have non-zero exit code
# Explicitly succeed for CI
exit 0
