//! Rust demo application on FreeRTOS: start-up entry and panic handler.
//!
//! Author:  Sandesh Ghimire <sandesh@soccentric.com>
//! Date:    2026-11-04
//! Target:  NUCLEO-F767ZI (STM32F767ZIT6), started by BOOT from slot A or B
//! SRS:     SRS-RFREERTOS-F767, SRS-DEMO-F767
//!
//! C `main()` brings up the board, calls [`rust_startup`] before the
//! scheduler, then creates the four tasks exported by `health.rs`,
//! `comms.rs`, `button.rs` and `led.rs`. The only flash write is the
//! CONFIRMED record (`boot.rs`).

#![no_std]
#![deny(warnings)]
#![deny(clippy::undocumented_unsafe_blocks)]

use core::ffi::c_void;

#[macro_use]
mod log;

mod app;
mod platform;
mod protocol;

// RFRT-001: task priorities as created by main.c (stack sizes live in C);
// reported in Status only.
/// Health task priority (highest).
pub(crate) const PRIO_HEALTH: i32 = 5;
/// Comms task priority.
pub(crate) const PRIO_COMMS: i32 = 4;
/// Button task priority.
pub(crate) const PRIO_BUTTON: i32 = 3;
/// LED task priority.
pub(crate) const PRIO_LED: i32 = 2;

// Compile-time check that every task entry is a FreeRTOS TaskFunction_t,
// void (*)(void *).
const _: [extern "C" fn(*mut c_void); 4] = [
    app::health::rust_health_task,
    app::comms::rust_comms_task,
    app::button::rust_button_task,
    app::led::rust_led_task,
];

/// Publish the boot record, log start-up, check VTOR and latch LD3.
///
/// Called once from C `main()` before the scheduler starts. `trial` is 0
/// unknown, 1 confirmed, 2 pending; an invalid `slot` gives an unknown
/// hand-over. Resets through the fatal path when VTOR is not the slot base.
#[no_mangle]
pub extern "C" fn rust_startup(reset_cause: u32, slot: u8, trial: u8, trial_count: u8) {
    // RFRT-004: keep C's decoded hand-over and reset flags for the tasks.
    let handover = if slot == platform::board::SLOT_A || slot == platform::board::SLOT_B {
        platform::boot::Handover {
            slot,
            trial_state: trial,
            trial_count,
        }
    } else {
        platform::boot::Handover::unknown()
    };
    app::ipc::set_boot_info(app::ipc::BootInfo {
        reset_cause,
        handover,
    });

    // RFDBG-001: start-up line on the debug UART.
    log_info!(
        "start v{} {}, slot {} {}, reset {}",
        platform::build_info::VERSION,
        platform::build_info::GIT_COMMIT,
        match handover.slot {
            platform::board::SLOT_A => 'A',
            platform::board::SLOT_B => 'B',
            _ => '?',
        },
        match handover.trial_state {
            platform::board::TRIAL_PENDING => "on trial",
            platform::board::TRIAL_CONFIRMED => "confirmed",
            _ => "no hand-over",
        },
        platform::boot::reset_cause_text(reset_cause)
    );

    // RFBT-001/002: the vector table must be the image's own table.
    if platform::plat::vtor() != platform::board::SLOT_BASE {
        app::fault::fatal();
    }

    // LED-005: latch LD3 for the last fault or watchdog reset.
    let last_fault = app::fault::last();
    let initial_fault = last_fault.is_some() || platform::boot::reset_was_watchdog(reset_cause);
    if last_fault.is_some() {
        log_warn!("last fault recorded before this reset");
    }
    app::ipc::update_status(|s| s.fault_active = initial_fault);
}

/// Log the panic message (max 120 bytes) and reset through the C fatal path.
///
/// RFDBG-001/RFSAFE-003. Formats into a stack buffer; never allocates.
#[panic_handler]
fn panic(info: &core::panic::PanicInfo) -> ! {
    use core::fmt::{self, Write as _};

    /// Truncating fixed-size text buffer.
    struct Buffer {
        /// Text bytes.
        data: [u8; 120],
        /// Bytes used.
        len: usize,
    }

    impl fmt::Write for Buffer {
        fn write_str(&mut self, text: &str) -> fmt::Result {
            let take = text.len().min(self.data.len() - self.len);
            self.data[self.len..self.len + take].copy_from_slice(&text.as_bytes()[..take]);
            self.len += take;
            Ok(())
        }
    }

    let mut line = Buffer {
        data: [0; 120],
        len: 0,
    };
    let _ = line.write_fmt(format_args!("panic: {}", info.message()));
    platform::plat::log(b'E', &line.data[..line.len]);
    platform::plat::fatal();
}
