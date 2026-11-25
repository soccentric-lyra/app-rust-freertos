# Planned improvements: app-rust-freertos

Gaps found in review and the enhancements planned to close them, highest priority first. Priority 1 is next, 3 is later.

## Gaps

| Area | Gap |
|---|---|
| Tests | No tests, on either the Rust or the C side. |
| `unsafe` | 42 `unsafe` blocks and only 8 SAFETY comments, the largest undocumented count in the set. The FFI boundary is where a reviewer will look. |
| README | Three other READMEs (`app-c-freertos`, `app-c-zephyr`, `app-rust-zephyr`) describe this repo as "Rust on Embassy". Its own README is correct. |
| Docs | No architecture page. The FFI contract between `platform/` and `src/` is not described outside the code. |
| Duplication | `src/protocol/hwlink/frame.rs` (196 lines) differs from the Embassy and Zephyr copies. |
| Build | The C and Rust halves are linked by a CMake rule, with no check that the FFI declarations on both sides still match. |
| Validation | No emulated run, and no documented way to debug across the language boundary in GDB. |

## Enhancements

| Priority | Enhancement | Result |
|---|---|---|
| 1 | Add SAFETY comments to all 42 `unsafe` blocks and enforce them with `clippy::undocumented_unsafe_blocks` | Closes the biggest credibility gap |
| 1 | Generate the FFI header with `cbindgen` and the Rust bindings with `bindgen` in CI, and fail on any diff | Makes the C and Rust boundary checked |
| 1 | Host tests for the protocol crate shared with the other Rust repos | Tested, single source of truth |
| 1 | GitHub Actions workflow with badge | Visible green status |
| 2 | Architecture page: FFI boundary diagram, which side owns which task, memory ownership rules, how static allocation is kept without an allocator | Explains the mixed-language design |
| 2 | Add a "migrating a C product to Rust one module at a time" section | Speaks to teams with an existing C code base |
| 2 | `cargo audit` and `cargo deny` in CI | Supply-chain evidence |
| 3 | GDB walkthrough that steps from C into Rust code, including the fault record | Shows debugging skill |
| 3 | Renode run with boot-log check | Hardware-free validation |

