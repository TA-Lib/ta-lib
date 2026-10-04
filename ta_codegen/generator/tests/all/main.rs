//! The generator's integration tests, one crate so the harness and the library
//! link once. A suite file here runs only if it is declared below.

mod common;

mod abstract_rows_suite;
mod alt_suite;
mod c_hygiene_suite;
mod c_render_statement_suite;
mod candle_range_suite;
mod circbuf_suite;
mod cond_comment_suite;
mod csharp_stream_suite;
mod display_shift_suite;
mod divisor_guard_suite;
mod doc_emphasis_suite;
mod enums_suite;
mod fma_suite;
mod indicator_variants_suite;
mod integration_test;
mod internal_error_id_suite;
mod java_render_statement_suite;
mod java_stream_suite;
mod managed_stream_suite;
mod open_core_suite;
mod open_validation_suite;
mod out_range_advance_suite;
mod peek_suite;
mod pattern_flags_suite;
mod period1_suite;
mod render_features_suite;
mod ride_along_suite;
mod rust_doc_suite;
mod rust_render_statement_suite;
mod rust_stream_suite;
mod spec_catalog_suite;
mod stability_suite;
mod streaming_dispatch_suite;
mod streaming_suite;
mod xml_metadata_suite;

#[test]
fn every_suite_file_is_declared() {
    let dir = std::path::Path::new(env!("CARGO_MANIFEST_DIR")).join("tests/all");
    let main = std::fs::read_to_string(dir.join("main.rs")).unwrap();
    let mut checked = 0;
    for entry in std::fs::read_dir(&dir).unwrap() {
        let path = entry.unwrap().path();
        let stem = path.file_stem().unwrap().to_str().unwrap().to_string();
        if path.extension().is_some_and(|e| e == "rs") && stem != "main" {
            let decl = format!("mod {stem};");
            assert!(main.lines().any(|l| l == decl), "tests/all/{stem}.rs is not declared in main.rs");
            checked += 1;
        }
    }
    assert!(checked > 0);
}
