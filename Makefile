ifeq ($(origin CC),default)
CC := gcc
endif
PYTHON ?= python
CFLAGS ?= -O3
STRICT_FLAGS := -std=c11 -Wall -Wextra -Wpedantic -Wconversion -Werror

ifeq ($(OS),Windows_NT)
EXEEXT := .exe
CPPFLAGS += -D__USE_MINGW_ANSI_STDIO=1
CLEAN = cmd /C "del /Q $(TARGETS) 2>NUL & exit /B 0"
else
EXEEXT :=
CLEAN = rm -f $(TARGETS)
endif

PROGRAMS := test2 test3 test4 test5 pi test7
TARGETS := $(addsuffix $(EXEEXT),$(PROGRAMS))

.PHONY: all clean test

all: $(TARGETS)

test2$(EXEEXT): test2.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(STRICT_FLAGS) $< -o $@

test3$(EXEEXT): test3.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(STRICT_FLAGS) $< -o $@

test4$(EXEEXT): test4.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(STRICT_FLAGS) $< -o $@

test5$(EXEEXT): test5.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(STRICT_FLAGS) $< -o $@

pi$(EXEEXT): test6.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(STRICT_FLAGS) $< -o $@

test7$(EXEEXT): test7.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(STRICT_FLAGS) $< -o $@

test: all
	$(PYTHON) tests/test_pi.py --bin-dir .

clean:
	$(CLEAN)
