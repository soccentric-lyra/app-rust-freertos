# Changelog

All notable changes to this project are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to
[Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

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
