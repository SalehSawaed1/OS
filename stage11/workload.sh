#!/usr/bin/env bash
set -euo pipefail
CLIENT="${1:?client path}"
PORT="${2:?port}"

# short, diverse workload to tick server paths and algorithms
"${CLIENT}" -p "${PORT}" "ALG SCC_COUNT RANDOM n=40 m=120 seed=1 directed=1" &
"${CLIENT}" -p "${PORT}" "ALG HAM_CYCLE RANDOM n=12 m=18 seed=2 directed=0 limit=12 timeout_ms=200" &
"${CLIENT}" -p "${PORT}" "ALG MAXCLIQUE RANDOM n=16 m=30 seed=3 directed=0 timeout_ms=200" &
"${CLIENT}" -p "${PORT}" "ALG NUM_MAXCLIQUES RANDOM n=16 m=30 seed=4 directed=0 timeout_ms=200" &
wait
