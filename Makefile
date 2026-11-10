ifeq ($(origin CC),default)
CC := gcc
endif

CFLAGS ?= -O3
WARN := -std=c11 -Wall -Wextra -Wpedantic -Wconversion -Werror

# python3 不一定存在；Windows 上还可能只是应用商店的空壳，所以真跑一次再决定。
ifeq ($(OS),Windows_NT)
PYTHON ?= $(shell python3 -c "import sys" >NUL 2>NUL && echo python3 || echo python)
else
PYTHON ?= $(shell python3 -c "import sys" >/dev/null 2>&1 && echo python3 || echo python)
endif

ifeq ($(OS),Windows_NT)
EXEEXT := .exe
CLEAN = cmd /C "del /Q $(subst /,\,$(TARGETS)) 2>NUL & exit /B 0"
else
EXEEXT :=
CLEAN = rm -f $(TARGETS)
endif

DEMOS := demos/01_float demos/02_long_division demos/03_half_pi demos/04_pi
TARGETS := $(addsuffix $(EXEEXT),pi pi_golf $(DEMOS))

.PHONY: all clean test test-slow

all: $(TARGETS)

%$(EXEEXT): %.c
	$(CC) $(CFLAGS) $(WARN) $< -o $@

test: all
	$(PYTHON) tests/test_pi.py --bin-dir .

test-slow: all
	$(PYTHON) tests/test_pi.py --bin-dir . --slow

clean:
	$(CLEAN)
