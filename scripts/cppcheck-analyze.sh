#!/usr/bin/env bash
set -euo pipefail

cppcheck --enable=warning,style,performance,portability --std=c++20 \
    -I include \
    --suppress=missingIncludeSystem \
    --suppress=missingInclude \
    --suppress=unusedFunction \
    --inline-suppr \
    --error-exitcode=1 \
    --quiet \
    src
