#
# Makefile - Top-level build, clean and flash for the Rust-on-FreeRTOS app
#
# Author:  Sandesh Ghimire <sandesh@soccentric.com>
# Target:  NUCLEO-F767ZI (STM32F767ZI)
#
# The C platform and FreeRTOS kernel are built by CMake + Ninja; CMake calls
# Cargo for the Rust staticlib and links it into the ELF.  Everything runs in
# the stm32-dev-rust-freertos container by default; set NATIVE=1 to use the
# host's OpenOCD, GDB and picocom instead (scripts/install-tools.sh).
#
#   make                build the firmware (same as make build)
#   make build          configure (if needed) and build
#   make clean          remove build/ and target/
#   make rebuild        clean, then build
#   make lint           clang-tidy static analysis of the C sources
#   make lint-strict    clang-tidy, failing on any finding
#   make fmt            cargo fmt in the container
#   make flash          build, then upload the image through BOOT (bootctl)
#   make erase-all      erase the whole flash (asks for confirmation)
#   make reset          reset the board
#   make debug          build, flash, then GDB halted at reset
#   make gdbserver      OpenOCD GDB server on :3333
#   make serial         serial console on the ST-LINK VCP (115200 8N1)
#   make probe          detect the ST-LINK and the target chip
#   make size           show the firmware size
#   make image          build the stm32-dev-rust-freertos Docker image
#   make help           show this help
#
# Variables (override on the command line, e.g. make BUILD_TYPE=Debug):
#   BUILD_TYPE          Release | Debug                       (Release)
#   NATIVE              1 = flash/debug/serial with host tools (0)
#   BOOTCTL             bootctl.py used by make flash
#                       (default ../bootloader/tools/bootctl.py)
#   PORT                serial port for make flash (default: auto-detect)
#   SOURCE_DATE_EPOCH   reproducible build timestamp (default: now)
#

# Windows: run every recipe under Git's POSIX sh, also from cmd.exe or
# PowerShell with only Git\cmd on PATH. Otherwise native make falls back to
# cmd.exe, which cannot run the recipes (quotes, pipes, find, rm -rf), and
# `sh scripts/...` is not found. Git's usr\bin goes first on PATH so find,
# sort and friends are the POSIX tools, not the Windows ones of the same name.
ifeq ($(OS),Windows_NT)
ifneq ($(origin SHELL),command line)
GIT_USR_BIN := $(firstword \
  $(if $(wildcard C:/Program?Files/Git/usr/bin/sh.exe),C:/Program?Files/Git/usr/bin) \
  $(if $(wildcard $(subst \,/,$(LOCALAPPDATA))/Programs/Git/usr/bin/sh.exe),$(subst \,/,$(LOCALAPPDATA))/Programs/Git/usr/bin) \
  $(if $(wildcard C:/msys64/usr/bin/sh.exe),C:/msys64/usr/bin))
ifneq ($(GIT_USR_BIN),)
SHELL := $(subst ?, ,$(GIT_USR_BIN))/sh.exe
export PATH := $(subst /,\,$(subst ?, ,$(GIT_USR_BIN)));$(PATH)
endif
endif
endif

BUILD_TYPE ?= Release
NATIVE     ?= 0
SOURCE_DATE_EPOCH ?= $(shell date +%s)

BUILD_DIR  := build
ELF        := $(BUILD_DIR)/rust-freertos-app.elf
IMAGE      := $(BUILD_DIR)/rust-freertos-app.img
# Gow make on Windows runs a simple recipe line itself instead of through a
# shell, and the MSYS bash.exe it starts that way exits 1 without running the
# script at all - no output, no status.  Prefixing the command with `sh` (Git
# Bash's own sh) takes the path make needs, so the script runs and its exit
# status is kept.  Linux and macOS start the scripts directly, as before.
ifeq ($(OS),Windows_NT)
DEV        := sh scripts/docker-dev.sh
SERIAL_CMD := sh scripts/serial.sh
else
DEV        := scripts/docker-dev.sh
SERIAL_CMD := scripts/serial.sh
endif
CARGO      := $(DEV) run env CARGO_HOME=/work/.cargo-home cargo
OPENOCD    := openocd -f board/st_nucleo_f7.cfg
BOOTCTL    ?= $(firstword $(wildcard ../bootloader/tools/bootctl.py ../../bootloader/bootloader/tools/bootctl.py) ../bootloader/tools/bootctl.py)
BOOT_DIR   ?= $(patsubst %/tools/bootctl.py,%,$(BOOTCTL))
SIGN_KEY   ?= $(BOOT_DIR)/keys/dev-signing-key.pem
SECURITY_COUNTER ?= 3
SIGNED     := $(IMAGE:.img=-signed.img)
PORT       ?=
# Git Bash resolves python3 to the Microsoft Store stub, which exits non-zero;
# Windows has the interpreter as python.
PYTHON     ?= $(if $(filter Windows_NT,$(OS)),python,python3)

.DEFAULT_GOAL := help

.PHONY: all build configure clean rebuild lint lint-strict fmt flash erase-all \
        reset debug gdbserver serial probe size image help

all: build

# Reconfigure when the build directory is new or the build type changed;
# otherwise let Ninja's own dependency tracking decide what to rebuild.
configure:
	@if ! grep -qx "CMAKE_BUILD_TYPE:STRING=$(BUILD_TYPE)" $(BUILD_DIR)/CMakeCache.txt 2>/dev/null; then \
		echo "Configuring $(BUILD_TYPE) build in $(BUILD_DIR)/"; \
		$(DEV) run env SOURCE_DATE_EPOCH=$(SOURCE_DATE_EPOCH) \
			cmake -S /work -B /work/$(BUILD_DIR) -G Ninja \
			-DCMAKE_BUILD_TYPE=$(BUILD_TYPE); \
	fi

build: configure
	$(DEV) run env SOURCE_DATE_EPOCH=$(SOURCE_DATE_EPOCH) \
		cmake --build /work/$(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR) target

rebuild: clean
	@"$(MAKE)" --no-print-directory build

# clang-tidy needs the compile commands from a configured build tree.
lint: configure
	$(DEV) run cmake --build /work/$(BUILD_DIR) --target clang-tidy

lint-strict:
	$(DEV) run cmake -S /work -B /work/$(BUILD_DIR) -G Ninja \
		-DAPP_LINT_STRICT=ON
	$(DEV) run cmake --build /work/$(BUILD_DIR) --target clang-tidy

fmt:
	$(CARGO) fmt --all

# A-6: never program the raw ELF into a slot. BOOT only starts an image
# that has its 256-byte header, metadata and records, which bootctl writes.
flash: build
	@test -f $(IMAGE) || { echo "error: $(IMAGE) not found, run make build"; exit 1; }
	@test -f $(BOOTCTL) || { echo "error: $(BOOTCTL) not found; set BOOTCTL=/path/to/bootctl.py"; exit 1; }
	@test -f $(SIGN_KEY) || { echo "error: $(SIGN_KEY) not found; set SIGN_KEY=/path/to/key.pem"; exit 1; }
	$(PYTHON) $(BOOT_DIR)/tools/mkimage.py --sign $(IMAGE) --key $(SIGN_KEY) \
		--security-counter $(SECURITY_COUNTER) --output $(SIGNED)
	$(PYTHON) $(BOOTCTL) $(if $(PORT),--port $(PORT),) update $(SIGNED) --activate

ERASE_CMD = $(if $(filter 1,$(NATIVE)),$(OPENOCD) -c "init; reset halt; flash erase_sector 0 0 last; exit",$(DEV) erase)

erase-all:
	@echo "WARNING: this erases the whole flash: BOOT, both metadata sectors"
	@echo "         and both application slots."
	@printf "Type 'yes' to continue: " && read answer && [ "$$answer" = "yes" ]
	$(ERASE_CMD)

size:
	@test -f $(ELF) || { echo "error: $(ELF) not found, run make build"; exit 1; }
	$(DEV) run arm-none-eabi-size /work/$(ELF)

image:
	$(DEV) build

ifeq ($(NATIVE),1)

reset:
	$(OPENOCD) -c "init; reset run; exit"

debug: build
	$(OPENOCD) -l $(BUILD_DIR)/openocd.log & ocd=$$!; \
	trap 'kill $$ocd 2>/dev/null' EXIT; sleep 1; \
	gdb-multiarch -q $(ELF) -ex "target extended-remote :3333" -ex "monitor reset halt" -ex load

gdbserver:
	$(OPENOCD)

serial:
	$(SERIAL_CMD)

probe:
	st-info --probe

else

reset:
	$(DEV) reset

debug: build
	$(DEV) debug $(ELF) --load

gdbserver:
	$(DEV) gdbserver

serial:
	$(DEV) serial

probe:
	$(DEV) probe

endif

help:
	@sed -n "/^#   make/p; /^# Variables/,/^#$$/p" $(firstword $(MAKEFILE_LIST)) | sed "s/^# \{0,1\}//"
