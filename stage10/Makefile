# Stage 10: run valgrind on previous stages (no code duplication)
SHELL := /bin/bash

# Paths to prior stages (adjust if you renamed)
STAGE7 := ../stage7
STAGE8 := ../stage8
STAGE9 := ../stage9

SERVER8 := $(STAGE8)/server8
SERVER9 := $(STAGE9)/server9
CLIENT7 := $(STAGE7)/client7

PORT_LF  := 5568
PORT_PIPE:= 5569

VALGRIND ?= valgrind

.PHONY: all deps clean install-tools \
        memcheck-lf memcheck-pipe \
        helgrind-lf helgrind-pipe \
        callgrind-lf callgrind-pipe \
        report

all: report

deps:
	@$(MAKE) -C $(STAGE7)
	@$(MAKE) -C $(STAGE8)
	@$(MAKE) -C $(STAGE9)

install-tools:
	sudo apt update
	sudo apt install -y valgrind kcachegrind gcovr lcov

# ---------- MEMCHECK ----------
memcheck-lf: deps
	@echo "[memcheck] LF server -> valgrind-memcheck-lf.txt"
	@set -euo pipefail; \
	( \
	  timeout 8s $(VALGRIND) --leak-check=full --show-leak-kinds=all --track-origins=yes \
	    --errors-for-leak-kinds=all --num-callers=25 --log-file=valgrind-memcheck-lf.txt \
	    $(SERVER8) -p $(PORT_LF) -t 4 & \
	  srv=$$!; \
	  sleep 0.6; \
	  bash ./workload.sh $(CLIENT7) $(PORT_LF); \
	  wait $$srv || true \
	)

memcheck-pipe: deps
	@echo "[memcheck] Pipeline server -> valgrind-memcheck-pipe.txt"
	@set -euo pipefail; \
	( \
	  timeout 8s $(VALGRIND) --leak-check=full --show-leak-kinds=all --track-origins=yes \
	    --errors-for-leak-kinds=all --num-callers=25 --log-file=valgrind-memcheck-pipe.txt \
	    $(SERVER9) -p $(PORT_PIPE) & \
	  srv=$$!; \
	  sleep 0.6; \
	  bash ./workload.sh $(CLIENT7) $(PORT_PIPE); \
	  wait $$srv || true \
	)

# ---------- HELGRIND (data races) ----------
helgrind-lf: deps
	@echo "[helgrind] LF server -> valgrind-helgrind-lf.txt"
	@set -euo pipefail; \
	( \
	  timeout 8s $(VALGRIND) --tool=helgrind --history-level=full \
	    --log-file=valgrind-helgrind-lf.txt \
	    $(SERVER8) -p $(PORT_LF) -t 4 & \
	  srv=$$!; \
	  sleep 0.6; \
	  bash ./workload.sh $(CLIENT7) $(PORT_LF); \
	  wait $$srv || true \
	)

helgrind-pipe: deps
	@echo "[helgrind] Pipeline server -> valgrind-helgrind-pipe.txt"
	@set -euo pipefail; \
	( \
	  timeout 8s $(VALGRIND) --tool=helgrind --history-level=full \
	    --log-file=valgrind-helgrind-pipe.txt \
	    $(SERVER9) -p $(PORT_PIPE) & \
	  srv=$$!; \
	  sleep 0.6; \
	  bash ./workload.sh $(CLIENT7) $(PORT_PIPE); \
	  wait $$srv || true \
	)

# ---------- CALLGRIND (call graph / hotspots) ----------
callgrind-lf: deps
	@echo "[callgrind] LF server -> callgrind.out.*"
	@set -euo pipefail; \
	( \
	  timeout 6s $(VALGRIND) --tool=callgrind --dump-instr=yes --collect-jumps=yes \
	    $(SERVER8) -p $(PORT_LF) -t 4 & \
	  srv=$$!; \
	  sleep 0.6; \
	  bash ./workload.sh $(CLIENT7) $(PORT_LF); \
	  wait $$srv || true; \
	  ls -1 callgrind.out.* 2>/dev/null || true; \
	  echo "Open with: kcachegrind callgrind.out.*" \
	)

callgrind-pipe: deps
	@echo "[callgrind] Pipeline server -> callgrind.out.*"
	@set -euo pipefail; \
	( \
	  timeout 6s $(VALGRIND) --tool=callgrind --dump-instr=yes --collect-jumps=yes \
	    $(SERVER9) -p $(PORT_PIPE) & \
	  srv=$$!; \
	  sleep 0.6; \
	  bash ./workload.sh $(CLIENT7) $(PORT_PIPE); \
	  wait $$srv || true; \
	  ls -1 callgrind.out.* 2>/dev/null || true; \
	  echo "Open with: kcachegrind callgrind.out.*" \
	)

report: memcheck-lf memcheck-pipe helgrind-lf helgrind-pipe callgrind-pipe
	@echo
	@echo "== Stage 10 outputs =="
	@ls -1 valgrind-*.txt 2>/dev/null || true
	@ls -1 callgrind.out.* 2>/dev/null || true

clean:
	$(RM) valgrind-*.txt callgrind.out.*
