# HY300 Tools Makefile
# Build system for HY300 hardware detection and integration tools
#
# Copyright (C) 2025 HY300 Linux Porting Project
# SPDX-License-Identifier: GPL-2.0+

# Cross-compilation configuration
CROSS_COMPILE ?= aarch64-unknown-linux-gnu-
CC = $(CROSS_COMPILE)gcc
STRIP = $(CROSS_COMPILE)strip

# Build configuration
CFLAGS = -Wall -Wextra -O2 -std=gnu99
LDFLAGS = 
LIBS = 

# Installation paths
PREFIX ?= /usr
BINDIR = $(PREFIX)/bin
LIBDIR = $(PREFIX)/lib/hy300
SERVICEDIR = $(PREFIX)/lib/systemd/system

# Target binaries
BINS = accelerometer_detection
SERVICES = hy300-accelerometer-service.py

# Source files
ACCELEROMETER_SRCS = accelerometer_detection.c
ACCELEROMETER_OBJS = $(ACCELEROMETER_SRCS:.c=.o)

# Build rules
.PHONY: all clean install install-service test

all: $(BINS)

accelerometer_detection: $(ACCELEROMETER_OBJS)
	@echo "  LD      $@"
	@$(CC) $(LDFLAGS) -o $@ $^

%.o: %.c
	@echo "  CC      $@"
	@$(CC) $(CFLAGS) -c -o $@ $<

clean:
	@echo "  CLEAN"
	@rm -f $(BINS) *.o

# Installation rules
install: $(BINS) $(SERVICES)
	@echo "  INSTALL tools"
	@install -d $(DESTDIR)$(BINDIR)
	@install -d $(DESTDIR)$(LIBDIR)
	@install -m 755 $(BINS) $(DESTDIR)$(BINDIR)/
	@install -m 755 $(SERVICES) $(DESTDIR)$(LIBDIR)/

install-service: hy300-accelerometer.service
	@echo "  INSTALL systemd service"
	@install -d $(DESTDIR)$(SERVICEDIR)
	@install -m 644 hy300-accelerometer.service $(DESTDIR)$(SERVICEDIR)/
	@echo "  INSTALL systemd service"
	@install -d $(DESTDIR)$(SERVICEDIR)
	@install -m 644 hy300-accelerometer.service $(DESTDIR)$(SERVICEDIR)/


# Test rules
test: test-build test-syntax

test-build: $(BINS)
	@echo "  TEST    build successful"

test-syntax:
	@echo "  TEST    syntax check"
	@python3 -m py_compile hy300-accelerometer-service.py
	@echo "  TEST    syntax OK"

# Development rules
debug: CFLAGS += -g -DDEBUG
debug: $(BINS)

static-analysis:
	@echo "  ANALYZE static analysis"
	@if command -v cppcheck >/dev/null 2>&1; then \
		cppcheck --enable=all --std=c99 $(ACCELEROMETER_SRCS); \
	else \
		echo "  SKIP    cppcheck not available"; \
	fi

# Help
help:
	@echo "HY300 Tools Build System"
	@echo ""
	@echo "Targets:"
	@echo "  all              Build all tools (default)"
	@echo "  clean            Remove build artifacts"
	@echo "  install          Install tools to system"
	@echo "  install-service  Install systemd service"
	@echo "  test             Run build and syntax tests"
	@echo "  debug            Build with debug symbols"
	@echo "  static-analysis  Run static code analysis"
	@echo "  help             Show this help"
	@echo ""
	@echo "Variables:"
	@echo "  CROSS_COMPILE    Cross-compilation prefix ($(CROSS_COMPILE))"
	@echo "  PREFIX           Installation prefix ($(PREFIX))"
	@echo "  DESTDIR          Staging directory for packaging"
	@echo ""
	@echo "Examples:"
	@echo "  make                           # Build all tools"
	@echo "  make CROSS_COMPILE=            # Native build"
	@echo "  make install PREFIX=/opt/hy300 # Install to /opt/hy300"
	@echo "  make test                      # Run tests"