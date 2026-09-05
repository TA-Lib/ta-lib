//! CIRCBUF storage: the batch/stream tier split (PR #176, issue #155).
//! Split out of the former `backend_suite.rs`.

#[path = "common/mod.rs"]
mod common;

use common::{discover_indicators, extract_section, generate_all, load_indicator, make_helpers, make_registry, try_load_indicator};
use std::path::Path;
use ta_codegen_lib::backends;
use ta_codegen_lib::ir;

// CIRCBUF storage: the batch/stream tier split (PR #176, issue #155)
// ---------------------------------------------------------------------------
//
// The Rust backend renders one IR `CircBuf` two different ways, and the two are
// not interchangeable:
//
//   batch  - C's hybrid. A zeroed stack array at the `CIRCBUF_PROLOG` static
//            size, a heap `Vec` only when a runtime `CIRCBUF_INIT` can exceed
//            it, and a `&mut` slice the body indexes through. Allocation-free
//            at every default parameter.
//   stream - an owning `Vec`. Forced, not chosen: the open path MOVES the
//            storage into the stream state struct, which outlives the frame, so
//            a `&mut` slice into a stack array would dangle.
//
// Getting this backwards does not produce wrong numbers, it produces code that
// does not compile — but only for a function that happens to exist. These pin
// the rule itself so a refactor cannot quietly apply one tier's shape to the
// other, and the sweep picks up new CIRCBUF carriers with no edit here.

/// Split a Rust function's output into (batch, stream). Both halves must be
/// non-empty for a streamable function — an empty half would turn every
/// `contains` below into a vacuous pass.
fn rust_batch_stream_halves(name: &str) -> (String, String) {
    let (func, enums) = load_indicator(name);
    let out = generate_all(&func, &enums);
    match out.rust.split_once(ta_codegen_lib::backends::rust_stream::SECTION_MARKER) {
        Some((b, s)) => (b.to_string(), s.to_string()),
        None => (out.rust.clone(), String::new()),
    }
}

/// Every CIRCBUF carrier, swept: batch is the hybrid, stream keeps the owning
/// `Vec`, and neither tier borrows the other's shape.
#[test]
fn rust_circbuf_batch_is_hybrid_and_stream_stays_vec() {
    let mut carriers: Vec<(String, Vec<String>)> = Vec::new();
    let mut stream_checked: Vec<String> = Vec::new();

    for name in discover_indicators() {
        let Some((func, enums)) = try_load_indicator(&name) else { continue };
        // CIRCBUF ids come from the IR, not from the rendered text, so a
        // rendering bug cannot hide a function from this sweep. Per tier,
        // because the two tiers need not run the same algorithm: the six
        // rolling-extremum functions carry the block scan's scratch in the
        // batch body only, and their `PRAGMA TA_ALT={STREAM,...}` alternate
        // declares no CIRCBUF at all.
        let prolog_ids = |body: &[ir::Statement]| -> Vec<String> {
            body.iter()
                .filter_map(|s| match s {
                    ir::Statement::CircBuf(ir::CircBuf::Prolog { id, .. }) => Some(id.clone()),
                    _ => None,
                })
                .collect()
        };
        let ids = prolog_ids(&func.body);
        let stream_ids = prolog_ids(func.resolved_for(ir::Lang::Rust).stream_source());
        if ids.is_empty() {
            continue;
        }
        let out = generate_all(&func, &enums);
        let (batch, stream) = match out
            .rust
            .split_once(ta_codegen_lib::backends::rust_stream::SECTION_MARKER)
        {
            Some((b, s)) => (b.to_string(), s.to_string()),
            None => (out.rust.clone(), String::new()),
        };

        // A storage declaration for `id`, in the shape `kind` — matched per
        // line so a `local_`/`heap_` prefix cannot satisfy a check aimed at the
        // bare storage (`"circBuffer = vec!["` IS a substring of
        // `"heap_circBuffer = vec!["`, which silently voids the naive form).
        let decl = |text: &str, id: &str, kind: &str| {
            text.lines().any(|l| {
                let t = l.trim_start();
                t.starts_with("let mut ")
                    && t.contains(id)
                    && t.contains(kind)
                    && !t.contains("local_")
                    && !t.contains("heap_")
            })
        };

        for id in &ids {
            // --- batch: hybrid — stack array, plus a slice to index through.
            assert!(
                batch.contains(&format!("let mut local_{id}")),
                "{name}: batch CIRCBUF `{id}` has no stack array — the hybrid is gone"
            );
            assert!(
                decl(&batch, id, ": &mut ["),
                "{name}: batch CIRCBUF `{id}` is not indexed through a &mut slice"
            );
            assert!(
                !decl(&batch, id, ": Vec<"),
                "{name}: batch CIRCBUF `{id}` declares an owning Vec — that is the \
                 pre-#176 always-allocate shape"
            );

            // --- stream: owning Vec, and none of the batch's borrowed shape.
            if !stream.is_empty() && stream_ids.contains(id) {
                assert!(
                    !stream.contains(&format!("local_{id}")),
                    "{name}: stream CIRCBUF `{id}` took the batch stack array — it is moved \
                     into the state struct and would dangle"
                );
                assert!(
                    !stream.contains(&format!("heap_{id}")),
                    "{name}: stream CIRCBUF `{id}` took the batch heap name"
                );
                assert!(
                    decl(&stream, id, ": Vec<"),
                    "{name}: stream CIRCBUF `{id}` lost its owning Vec"
                );
                assert!(
                    !decl(&stream, id, ": &mut ["),
                    "{name}: stream CIRCBUF `{id}` borrows instead of owning"
                );
            }
        }
        if !stream_ids.is_empty() {
            stream_checked.push(name.clone());
        }
        carriers.push((name, ids));
    }

    // Anti-vacuity: the carriers that exist today must all be swept. A filter or
    // discovery regression fails here instead of passing an empty run.
    let names: Vec<&str> = carriers.iter().map(|(n, _)| n.as_str()).collect();
    for expected in [
        "cci", "cmf", "mfi", "ultosc", "hma", "ht_dcphase", "ht_sine", "ht_trendmode",
        "min", "max", "minmax", "midpoint", "midprice", "willr",
    ] {
        assert!(
            names.contains(&expected),
            "CIRCBUF sweep missed {expected}; swept {names:?}"
        );
    }
    // The stream half is now conditional on the id reaching the stream tier, so
    // pin the functions whose stream genuinely carries one — otherwise a bug
    // that emptied every stream body would turn that half into a silent skip.
    let streamed: Vec<&str> = stream_checked.iter().map(String::as_str).collect();
    for expected in ["cci", "cmf", "mfi", "ultosc"] {
        assert!(
            streamed.contains(&expected),
            "CIRCBUF stream half never ran for {expected}; ran for {streamed:?}"
        );
    }
    // ...and the six rolling-extremum functions must NOT reach it: their scratch
    // belongs to the batch block scan, and their alternate declares none.
    for absent in ["min", "max", "minmax", "midpoint", "midprice", "willr"] {
        assert!(
            !streamed.contains(&absent),
            "{absent}: a CIRCBUF reached the stream tier — the STREAM alternate should \
             declare none"
        );
    }
}

/// CCI — the plain layout with a runtime `CIRCBUF_INIT`. Pins the full hybrid:
/// stack array at the prolog size, a heap `Vec` behind it, and the crossover
/// guard at exactly the static size (C heaps when `Size > sizeof(local)`).
#[test]
fn rust_circbuf_runtime_init_renders_the_crossover_guard() {
    let (batch, _) = rust_batch_stream_halves("cci");

    assert!(
        batch.contains("let mut local_circBuffer: [f64; 30] = [0.0_f64; 30];"),
        "CCI: stack array at the CIRCBUF_PROLOG static size"
    );
    assert!(
        batch.contains("let mut heap_circBuffer: Vec<f64> = Vec::new();"),
        "CCI: heap fallback declared (a runtime INIT can exceed the static size)"
    );
    assert!(
        batch.contains("let mut circBuffer: &mut [f64] = &mut [];"),
        "CCI: body indexes through a &mut slice"
    );
    assert!(
        batch.contains("if (optInTimePeriod) as usize <= 30usize {"),
        "CCI: crossover guard sits at the static size, matching C's macro"
    );
    assert!(
        batch.contains("circBuffer = &mut local_circBuffer;"),
        "CCI: fits ⇒ bind the stack array, no allocation"
    );
    assert!(
        batch.contains("heap_circBuffer = vec![0.0_f64; (optInTimePeriod) as usize];")
            && batch.contains("circBuffer = &mut heap_circBuffer;"),
        "CCI: exceeds ⇒ allocate and bind the heap Vec"
    );
    // The pre-#176 shape: an unconditional allocation on every call. Matched
    // per line — `"circBuffer = vec!["` is a substring of the legitimate
    // `"heap_circBuffer = vec!["`, so `contains` alone can never fire here.
    assert!(
        !batch
            .lines()
            .any(|l| l.trim_start().starts_with("circBuffer = vec![")),
        "CCI: batch must never allocate unconditionally"
    );
}

/// HT_SINE — `CIRCBUF_INIT_LOCAL_ONLY`. There is no runtime size, so the heap
/// arm is unreachable and must not be declared at all: this tier is
/// allocation-free outright, not merely allocation-free at the default.
#[test]
fn rust_circbuf_init_local_only_declares_no_heap_arm() {
    let (batch, _) = rust_batch_stream_halves("ht_sine");

    assert!(
        batch.contains("let mut local_smoothPrice: [f64; 50] = [0.0_f64; 50];"),
        "HT_SINE: stack array at the static size"
    );
    assert!(
        !batch.contains("heap_smoothPrice"),
        "HT_SINE: INIT_LOCAL_ONLY can never reach the heap, so the Vec must not exist"
    );
    assert!(
        batch.contains("smoothPrice = &mut local_smoothPrice;"),
        "HT_SINE: INIT_LOCAL_ONLY binds the stack array directly"
    );
    assert!(
        !batch.contains("smoothPrice = vec!["),
        "HT_SINE: batch must not allocate"
    );
}

/// ULTOSC — `CIRCBUF_PROLOG_CLASS`, field-split into parallel storages. One
/// crossover guard decides for the whole struct; the fields must not be able to
/// disagree about which arm they are in.
#[test]
fn rust_circbuf_class_layout_shares_one_crossover_guard() {
    let (batch, _) = rust_batch_stream_halves("ultosc");

    for field in ["term_closeMinusTrueLow", "term_trueRange"] {
        assert!(
            batch.contains(&format!("let mut local_{field}: [f64; 32] = [0.0_f64; 32];")),
            "ULTOSC: {field} needs its own stack array"
        );
        assert!(
            batch.contains(&format!("let mut heap_{field}: Vec<f64> = Vec::new();")),
            "ULTOSC: {field} needs its own heap fallback"
        );
    }
    // Exactly one guard, with both fields bound inside its arms.
    assert_eq!(
        batch.matches("as usize <= 32usize {").count(),
        1,
        "ULTOSC: the class layout must decide once, not per field"
    );
    let guard = extract_section(&batch, "if (optInTimePeriod3) as usize <= 32usize {", "maxIdx_term =");
    assert!(
        guard.contains("term_closeMinusTrueLow = &mut local_term_closeMinusTrueLow;")
            && guard.contains("term_trueRange = &mut local_term_trueRange;"),
        "ULTOSC: both fields bind the stack arrays in the fits arm"
    );
    assert!(
        guard.contains("heap_term_closeMinusTrueLow = vec![")
            && guard.contains("heap_term_trueRange = vec!["),
        "ULTOSC: both fields allocate in the exceeds arm"
    );
}

/// `TA_MAX_INDEX` is stated as a literal in five hand-written places across four
/// languages plus the Java test server's embedded `Core` (#180). Nothing in the
/// build makes them agree — the generated prologues reference the *symbol*, so a
/// raised cap in `ta_defs.h` alone would leave C accepting calls the other three
/// reject, which is the one divergence the constant exists to prevent.
///
/// This is the parity check. It reads the value out of each surface and requires
/// one distinct value. Adding a fifth binding means adding it here.
#[test]
fn ta_max_index_agrees_across_every_surface() {
    use std::path::Path;
    let root = Path::new(env!("CARGO_MANIFEST_DIR")).join("../..");

    // (file, the text immediately preceding the literal)
    let surfaces: &[(&str, &str)] = &[
        ("include/ta_defs.h", "#define TA_MAX_INDEX "),
        ("ta_codegen/generator/templates/rust/types.rs", "pub const MAX_INDEX: usize = "),
        ("ta_codegen/output/java/library/src/main/java/io/github/talib/Core.java",
         "public static final int MAX_INDEX = "),
        ("ta_codegen/output/csharp/library/Core.cs", "public const int MAX_INDEX = "),
        ("ta_codegen/generator/src/server_gen.rs", "static final int MAX_INDEX = "),
    ];

    let mut seen: Vec<(String, u64)> = Vec::new();
    for (rel, prefix) in surfaces {
        let text = std::fs::read_to_string(root.join(rel))
            .unwrap_or_else(|e| panic!("{rel}: {e}"));
        let at = text
            .find(prefix)
            .unwrap_or_else(|| panic!("{rel}: no `{prefix}` — did the declaration move?"));
        let digits: String = text[at + prefix.len()..]
            .chars()
            .take_while(|c| c.is_ascii_digit() || *c == '_')
            .filter(|c| *c != '_')
            .collect();
        let value: u64 = digits
            .parse()
            .unwrap_or_else(|_| panic!("{rel}: `{prefix}` is not followed by a literal"));
        seen.push(((*rel).to_string(), value));
    }

    let first = seen[0].1;
    for (rel, value) in &seen {
        assert_eq!(
            *value, first,
            "TA_MAX_INDEX disagrees: {rel} says {value}, {} says {first}",
            seen[0].0
        );
    }
    // Pin the shipped value too, so raising the cap is a deliberate edit here
    // and not something a backend picks up silently.
    assert_eq!(first, 100_000_000, "TA_MAX_INDEX changed; update the docs and CHANGELOG with it");
}

/// Every `CIRCBUF_INIT` allocation-failure path must release the buffers the
/// CIRCBUFs before it already took.
///
/// `CIRCBUF_INIT` heap-allocates when the runtime size outgrows its stack
/// buffer and returns `TA_ALLOC_ERR` straight out of the function on failure —
/// so in a function holding more than one CIRCBUF, the later ones' failure
/// paths leak the earlier ones unless the cascade is emitted. Issue #147's
/// rolling-extremum block scan brought the first 2- and 4-CIRCBUF functions
/// into the tree (MIN/MAX take two, MINMAX/MIDPOINT/MIDPRICE/WILLR take four),
/// which is what made a latent generator gap live.
///
/// Swept over every indicator rather than a name list, so a new multi-CIRCBUF
/// function is covered the day it lands.
#[test]
fn c_circbuf_alloc_failure_frees_the_circbufs_before_it() {
    let base = Path::new(env!("CARGO_MANIFEST_DIR")).join("../../ta_codegen/input");
    let registry = make_registry();
    let helpers = make_helpers();

    let mut names: Vec<String> = std::fs::read_dir(&base)
        .expect("input dir")
        .filter_map(|e| {
            let path = e.ok()?.path();
            let name = path.file_name()?.to_str()?.to_string();
            (path.join(format!("{name}.yaml")).is_file() && path.join(format!("{name}.c")).is_file())
                .then_some(name)
        })
        .collect();
    names.sort();
    assert!(names.len() >= 200, "expected the full indicator set, got {}", names.len());

    let mut multi_circbuf_functions = 0usize;
    for name in &names {
        let (func, enums) = load_indicator(name);
        let c_out = backends::c::generate(&func, &enums, &registry, &helpers);

        // `declared` resets at each function body: the emitter puts a bare `{`
        // at column 0 to open one.
        let mut declared: Vec<String> = Vec::new();
        let mut pending_alloc: Option<String> = None;
        let mut in_failure_block: Option<(String, Vec<String>)> = None;

        for line in c_out.lines() {
            if line == "{" {
                declared.clear();
                continue;
            }
            let t = line.trim();

            // `double *sufLowest = &local_sufLowest[0];`
            if let Some(rest) = t.strip_prefix("double *").or_else(|| t.strip_prefix("int *")) {
                if let Some(storage) = rest.split(" = &local_").next() {
                    if rest.contains(" = &local_") {
                        declared.push(storage.to_string());
                    }
                }
            }

            if let Some(rest) = t.strip_prefix("if( !") {
                if let Some(storage) = rest.split(&[' ', ')'][..]).next() {
                    if pending_alloc.as_deref() == Some(storage) {
                        in_failure_block = Some((storage.to_string(), Vec::new()));
                    }
                }
            } else if let Some((_, freed)) = in_failure_block.as_mut() {
                if t.contains("TA_Free( ") {
                    let f = t.rsplit("TA_Free( ").next().unwrap_or("");
                    if let Some(v) = f.split(' ').next() {
                        freed.push(v.to_string());
                    }
                }
                if t == "return TA_ALLOC_ERR;" {
                    let (storage, freed) = in_failure_block.take().expect("in block");
                    let at = declared.iter().position(|d| *d == storage);
                    if let Some(at) = at {
                        if at > 0 {
                            multi_circbuf_functions += 1;
                        }
                        for earlier in &declared[..at] {
                            assert!(
                                freed.iter().any(|f| f == earlier),
                                "{name}: allocation failure for `{storage}` returns \
                                 TA_ALLOC_ERR without releasing `{earlier}`, which was \
                                 allocated before it — that leaks up to one full scratch \
                                 buffer per CIRCBUF. Freed here: {freed:?}"
                            );
                        }
                    }
                }
            }

            pending_alloc = t
                .contains("= TA_Malloc(")
                .then(|| t.split(" = TA_Malloc(").next().unwrap_or("").trim().to_string());
        }
    }

    // Guard the gate itself: if the cascade never had a case to cover, the
    // sweep above would pass vacuously.
    assert!(
        multi_circbuf_functions >= 12,
        "expected the rolling-extremum family's multi-CIRCBUF failure paths to be \
         swept, saw only {multi_circbuf_functions}"
    );
}

/// A CIRCBUF whose cursor is never read gets no cursor.
///
/// `CIRCBUF_PROLOG` used to declare `<id>_Idx` and `maxIdx_<id>` unconditionally
/// and `CIRCBUF_INIT` to assign them, which is right for a ring (the body
/// advances with `CIRCBUF_NEXT` and indexes with the cursor) and wrong for a
/// CIRCBUF used only as a period-sized scratch buffer. The #147 block scan
/// indexes its arrays directly, so the six rolling-extremum functions were
/// emitting eight write-only ints apiece — 80 `-Wunused-but-set-variable`
/// across the family in any consumer building with `-Wall -Wextra`.
///
/// Both halves are asserted: the scratch users must NOT carry the pair, and the
/// ring users must still carry it. Dropping it from a ring would not compile,
/// but this says so at the generator rather than in a downstream build.
#[test]
fn c_circbuf_omits_the_cursor_when_nothing_reads_it() {
    let registry = make_registry();
    let helpers = make_helpers();

    for name in ["min", "max", "minmax", "midpoint", "midprice", "willr"] {
        let (func, enums) = load_indicator(name);
        let c_out = backends::c::generate(&func, &enums, &registry, &helpers);
        for decl in ["_Idx;", "maxIdx_"] {
            assert!(
                !c_out.contains(decl),
                "{name} uses its CIRCBUFs as plain scratch, so `{decl}` must not be \
                 emitted — a write-only int is a -Wunused-but-set-variable in every \
                 consumer's build: {c_out}"
            );
        }
        // The buffers themselves must survive: this trims the cursor, not the scratch.
        assert!(
            c_out.contains("= &local_") && c_out.contains("TA_Malloc("),
            "{name} must still declare and size its scratch buffers: {c_out}"
        );
    }

    // The ring users are the control arm — if the predicate went blanket-true,
    // these would lose a cursor they genuinely read and this test would say so.
    for name in ["cci", "ultosc"] {
        let (func, enums) = load_indicator(name);
        let c_out = backends::c::generate(&func, &enums, &registry, &helpers);
        assert!(
            c_out.contains("_Idx;") && c_out.contains("maxIdx_"),
            "{name} advances a real ring, so it must keep its cursor: {c_out}"
        );
    }
}

/// A local read ONLY by a `CIRCBUF_INIT` size must survive the dead-store pass.
///
/// `drop_unused_decls` (c_stream) removes a hoisted prologue decl the arm never
/// reads — HMA's degenerate arm inherits `halfPeriod = optInTimePeriod / 2` and
/// never uses it. The trap is that `walk_stmt_exprs` treats `Statement::CircBuf`
/// as opaque, so a use-analysis built on it alone cannot see `CIRCBUF_INIT`'s
/// size expression. HMA's general arm sizes its de-lag ring with `ringSize`,
/// which is otherwise only ever assigned — miss that read and the pass deletes a
/// live declaration, emitting C that does not compile.
///
/// Both directions are asserted so the pass cannot pass by doing nothing.
#[test]
fn c_stream_keeps_a_local_read_only_by_a_circbuf_size() {
    let registry = make_registry();
    let helpers = make_helpers();
    let (func, enums) = load_indicator("hma");
    let stream_c = backends::c_stream::generate(&func, &enums, &registry, &helpers);

    // Read only through CIRCBUF_INIT's size — the case a naive walker misses.
    assert!(
        stream_c.contains("int ringSize;") && stream_c.contains("ringSize = sqrtPeriod - 1;"),
        "`ringSize` is read only by CIRCBUF_INIT's size expression, so a use-analysis \
         that does not descend into Statement::CircBuf will drop it and emit C that \
         references an undeclared variable: {stream_c}"
    );

    // ...and the dead store the pass exists for is gone. HMA's stream section
    // carries exactly one `halfPeriod` assignment: the general arm's. The
    // degenerate arm's copy is the dead one.
    assert_eq!(
        stream_c.matches("halfPeriod = optInTimePeriod / 2;").count(),
        1,
        "the degenerate arm must not carry a write-only `halfPeriod` (that is a \
         -Wunused-but-set-variable in the consumer's build), and the general arm \
         must keep the one it reads: {stream_c}"
    );
}
