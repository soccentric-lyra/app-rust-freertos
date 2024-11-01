//! Debug lines on USART6 through the C `app_log` layer.
//!
//! Author: Sandesh Ghimire <sandesh@soccentric.com>
//! SRS:    RFDBG-001..RFDBG-003
//!
//! USART3 carries HWLINK only. Lines use the format BOOT and the other
//! firmwares share:
//!
//! ```text
//! [   5012] R-FRT I health: passed, CONFIRMED written
//! ```
//!
//! The macros format into a stack buffer (no allocation) and never block.
//! Text past `PLAT_LOG_TEXT_MAX` bytes is cut off.

use core::fmt::{self, Write};

use crate::platform::plat;

/// Line length limit; same as `PLAT_LOG_TEXT_MAX` in `platform/platform.h`.
const TEXT_MAX: usize = 120;

/// Truncating line buffer.
struct Line {
    /// Text bytes.
    buf: [u8; TEXT_MAX],
    /// Bytes used.
    len: usize,
}

impl Write for Line {
    fn write_str(&mut self, s: &str) -> fmt::Result {
        let take = s.len().min(TEXT_MAX - self.len);
        self.buf[self.len..self.len + take].copy_from_slice(&s.as_bytes()[..take]);
        self.len += take;
        Ok(())
    }
}

/// Format `args` and log one line at `level` (`b'E'`, `b'W'`, `b'I'`, `b'D'`).
pub fn write(level: u8, args: fmt::Arguments) {
    let mut line = Line {
        buf: [0; TEXT_MAX],
        len: 0,
    };
    let _ = line.write_fmt(args);
    plat::log(level, &line.buf[..line.len]);
}

/// Log an error line, `format!` syntax.
macro_rules! log_error {
    ($($arg:tt)*) => { $crate::log::write(b'E', format_args!($($arg)*)) };
}

/// Log a warning line, `format!` syntax.
macro_rules! log_warn {
    ($($arg:tt)*) => { $crate::log::write(b'W', format_args!($($arg)*)) };
}

/// Log an info line, `format!` syntax.
macro_rules! log_info {
    ($($arg:tt)*) => { $crate::log::write(b'I', format_args!($($arg)*)) };
}

/// Once-a-second "task <n> <name>: <purpose>" line for one task.
///
/// Each task owns one and calls [`TaskReport::tick`] every loop iteration.
pub struct TaskReport {
    /// Task number in the line.
    number: u8,
    /// Task name.
    name: &'static str,
    /// One-line purpose.
    purpose: &'static str,
    /// Time of the last line in ms, `None` before the first.
    last: Option<u64>,
}

impl TaskReport {
    /// Create a report that prints on the first `tick`.
    pub const fn new(number: u8, name: &'static str, purpose: &'static str) -> Self {
        Self {
            number,
            name,
            purpose,
            last: None,
        }
    }

    /// Print the line if at least 1000 ms passed since the last one.
    pub fn tick(&mut self) {
        let now = plat::now_ms();
        let due = match self.last {
            Some(last) => now.wrapping_sub(last) >= 1000,
            None => true,
        };
        if due {
            self.last = Some(now);
            write(
                b'I',
                format_args!("task {} {}: {}", self.number, self.name, self.purpose),
            );
        }
    }
}
