//! The committed C is a fixed point of the hygiene pass.
//!
//! Read off DISK rather than re-rendered, so it fails for a C producer that
//! never routed its text through the phase — which is the whole maintenance
//! contract: a new emitter inherits the cleanup, and one that bypasses it says
//! so here instead of shipping a cast that claims a name is unused when the
//! block below reads it.

use std::path::{Path, PathBuf};

use ta_codegen_lib::backends::c_hygiene::scrub_void_casts;

fn repo_root() -> PathBuf {
    Path::new(env!("CARGO_MANIFEST_DIR")).join("../..")
}

/// Every `.c` under a directory the generator owns.
fn generated_c() -> Vec<PathBuf> {
    let root = repo_root();
    let mut files = Vec::new();
    for dir in ["src/ta_func", "src/ta_abstract", "ta_codegen/output/c/tools"] {
        let mut stack = vec![root.join(dir)];
        while let Some(d) = stack.pop() {
            let Ok(rd) = std::fs::read_dir(&d) else { continue };
            for e in rd.flatten() {
                let p = e.path();
                if p.is_dir() {
                    stack.push(p);
                } else if p.extension().is_some_and(|x| x == "c") {
                    files.push(p);
                }
            }
        }
    }
    files.sort();
    files
}

#[test]
fn no_committed_c_file_casts_away_a_name_its_block_reads() {
    let files = generated_c();
    assert!(files.len() >= 200, "only {} generated .c file(s) found", files.len());
    let (mut casts, mut with_casts) = (0usize, 0usize);
    let mut offenders: Vec<String> = Vec::new();
    for p in &files {
        let src = std::fs::read_to_string(p).expect("readable");
        let n = src.matches("(void)").count();
        casts += n;
        with_casts += usize::from(n > 0);
        if scrub_void_casts(&src) != src {
            offenders.push(p.display().to_string());
        }
    }
    // A corpus with no casts left would satisfy the sweep saying nothing.
    assert!(
        casts > 0 && with_casts > 0,
        "{casts} cast(s) across {with_casts} file(s) — the sweep has nothing to judge"
    );
    assert!(
        offenders.is_empty(),
        "{} file(s) still cast away a name their own block reads — route the producer's \
         text through `c_hygiene::scrub_void_casts`:\n{}",
        offenders.len(),
        offenders.join("\n")
    );
}
