//! Application logic and the four FreeRTOS tasks.
//!
//! `health`, `comms`, `button` and `led` own the tasks; `ipc` holds the
//! shared state and fixed-size queues; `fault` and `identity` are services.

pub mod button;
pub mod comms;
pub mod fault;
pub mod health;
pub mod identity;
pub mod ipc;
pub mod led;
