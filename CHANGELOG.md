# Changelog

All notable changes to this project are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to
[Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added

- Host test crate for `src/protocol/hwlink/frame.rs`: COBS examples, block
  boundaries, CRC corruption, reserved flags and size limits
- `scripts/check_ffi.py` compares the Rust `extern "C"` declarations with
  `platform/platform.h` and fails on a name or argument-count mismatch
- `scripts/check_boot_contract.py` and `scripts/check_image.py`, shared by
  Jenkins and GitHub Actions
- `make clippy`, `make test-host`, `make ffi-check`, `make boot-contract` and
  `make image-check` quality-gate targets
- `docs/architecture.md`: C/Rust layers, FFI ownership, memory map, boot flow,
  GDB walkthrough and a module-by-module migration guide
- `cargo audit` and `cargo deny` jobs plus a `deny.toml` policy
- GitHub Actions workflow with the same gates

### Changed

- The FFI wrappers now go through one documented `ffi_call!` macro and
  `clippy::undocumented_unsafe_blocks` is denied in `src/lib.rs`
- The Jenkins boot-contract and image stages use the shared scripts instead
  of inline copies

## [1.0.0] - 2026-11-19

### Added

- Demo application in Rust (`no_std`, no allocator) running directly on the
  FreeRTOS kernel: LED patterns, the user button, HWLINK over UART, watchdog
  supervision and self-confirmation
- C platform layer keeps board start-up, STM32 HAL drivers and the bootloader
  contract; all application logic is Rust
- CMake build compiles both sides and links the Rust staticlib into one image
- Runs from either A/B slot under the bootloader

[Unreleased]: https://github.com/soccentric-stm32/app-rust-freertos/compare/v1.0.0...HEAD
[1.0.0]: https://github.com/soccentric-stm32/app-rust-freertos/releases/tag/v1.0.0
