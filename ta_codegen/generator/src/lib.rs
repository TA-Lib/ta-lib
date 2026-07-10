// Strict linting for ta_codegen source code
#![deny(clippy::pedantic)]
#![warn(clippy::cognitive_complexity)]
#![warn(clippy::too_many_lines)]
#![warn(clippy::needless_pass_by_value)]
// Allow these common patterns in codegen code
#![allow(clippy::module_name_repetitions)]
#![allow(clippy::must_use_candidate)]
#![allow(clippy::missing_errors_doc)]
#![allow(clippy::missing_panics_doc)]
#![allow(clippy::struct_excessive_bools)]
// Code generators build strings — push_str(&format!(...)) is the natural pattern
#![allow(clippy::format_push_string)]
// Generated items have doc comments from upstream C, backtick enforcement is noise
#![allow(clippy::doc_markdown)]

pub mod backends;
pub mod bench_gen;
pub mod candle_settings;
pub mod extractor;
pub mod formatter;
pub mod helper_registry;
pub mod ir;
pub mod parser;
pub mod registry;
pub mod server_gen;
pub mod streaming;
