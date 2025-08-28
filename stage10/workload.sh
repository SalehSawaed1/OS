#!/usr/bin/env bash
set -euo pipefail
CLIENT="${1:?client path}"
PORT="${2:?port}"

# small, quick, and deterministic workloads (with time caps for NP-hard ones)
"${CLIENT}" -p "${PORT}" "ALG SCC_COUNT RANDOM n=200 m=800 seed=1 directed=1" &
"${CLIENT}" -p "${PORT}" "ALG HAM_CYCLE RANDOM n=16 m=24 seed=2 directed=0 limit=16 timeout_ms=250" &
"${CLIENT}" -p "${PORT}" "ALG MAXCLIQUE RANDOM n=22 m=40 seed=3 directed=0 timeout_ms=200" &
"${CLIENT}" -p "${PORT}" "ALG NUM_MAXCLIQUES RANDOM n=22 m=40 seed=4 directed=0 timeout_ms=200" &
wait
