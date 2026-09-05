//! Per-indicator x per-backend variant checks, cross-call resolution, logic
//! vs guarded validation, and indicator-specific feature tests. Split out of
//! the former `backend_suite.rs`.

#[path = "common/mod.rs"]
mod common;

use common::{
    all_abstract_rows, check_c_int_alias, check_c_variants, check_java_variants,
    check_rust_cast_parens, check_rust_generic_variants, discover_indicators, extract_section,
    generate_all, load_indicator, load_synth, make_registry, try_generate_all, try_load_indicator,
};
use std::path::Path;
use ta_codegen_lib::backends;
use ta_codegen_lib::helper_registry::HelperRegistry;
use ta_codegen_lib::ir;
use ta_codegen_lib::parser;
use ta_codegen_lib::registry::{Lang, Registry};

// 2. Auto-discovered per-indicator x per-backend variant checks
// ---------------------------------------------------------------------------

#[test]
fn test_all_indicators_all_backends() {
    let indicators = discover_indicators();
    assert!(!indicators.is_empty(), "No indicators discovered");

    let mut failures = Vec::new();
    let mut tested = 0;
    let mut skipped = 0;

    for name in &indicators {
        // Phase 1: try to load and generate (may fail for not-yet-supported indicators)
        let loaded = try_load_indicator(name);
        let (func, enums) = match loaded {
            Some(v) => v,
            None => {
                skipped += 1;
                continue;
            }
        };
        let out = match try_generate_all(&func, &enums) {
            Some(v) => v,
            None => {
                skipped += 1;
                continue;
            }
        };

        // Phase 2: run variant checks (failures here are real bugs)
        // Every backend spells the indicator exactly as `input/` names it.
        let upper = func.name.clone();
        let snake = name.clone();

        let result = std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| {
            check_c_variants(&out.c, &upper, &snake);
            check_rust_generic_variants(&out.rust, &upper);
            check_java_variants(&out.java, &upper);
            check_c_int_alias(&out.c, &upper, &snake);
            check_rust_cast_parens(&out.rust, &snake);
        }));

        if let Err(e) = result {
            let msg = if let Some(s) = e.downcast_ref::<String>() {
                s.clone()
            } else if let Some(s) = e.downcast_ref::<&str>() {
                s.to_string()
            } else {
                format!("Unknown panic for indicator {}", name)
            };
            failures.push(msg);
        } else {
            tested += 1;
        }
    }

    eprintln!(
        "Variant checks: {} tested, {} skipped (parse not yet supported), {} failed",
        tested,
        skipped,
        failures.len()
    );

    // Coverage is the gate here, not just the failure count: every check driven
    // from this loop (check_rust_cast_parens among them) is worth exactly the
    // number of indicators that reached it. `skipped` swallows a load/generate
    // panic, so a parser regression could quietly empty the corpus while the
    // suite stayed green — the floor of 6 this replaces would have allowed 162
    // of 168 to vanish silently.
    assert_eq!(
        skipped, 0,
        "{skipped} indicator(s) failed to load or generate and were silently \
         skipped, so no per-indicator check ran on them; {tested} were tested. \
         Fix the regression, or make the skip explicit if it is intended."
    );
    assert!(
        tested >= 6,
        "Expected at least 6 indicators to pass, but only {} did",
        tested
    );

    if !failures.is_empty() {
        panic!(
            "{} indicator(s) failed variant checks:\n{}",
            failures.len(),
            failures.join("\n")
        );
    }
}

// ---------------------------------------------------------------------------
// 3. Cross-call resolution tests (MA calls sma/ema lookback + logic)
// ---------------------------------------------------------------------------

#[test]
fn test_ma_c_cross_calls() {
    let (func, enums) = load_indicator("ma");
    let out = generate_all(&func, &enums);
    let c = &out.c;

    assert!(
        c.contains("TA_SMA_Lookback("),
        "C: MA should call TA_SMA_Lookback"
    );
    assert!(
        c.contains("TA_EMA_Lookback("),
        "C: MA should call TA_EMA_Lookback"
    );
    // Bare cross-indicator calls resolve to the guarded entry point. Anchored on
    // the first argument: bare `TA_SMA(` would also match a declaration.
    assert!(
        c.contains("TA_SMA(startIdx"),
        "C: MA should call guarded TA_SMA"
    );
    assert!(
        c.contains("TA_EMA(startIdx"),
        "C: MA should call guarded TA_EMA"
    );
    assert!(
        !c.contains("TA_SMA_Unguarded(") && !c.contains("TA_EMA_Unguarded("),
        "C: MA must not call the unguarded variants"
    );
}

#[test]
fn test_ma_java_cross_calls() {
    let (func, enums) = load_indicator("ma");
    let out = generate_all(&func, &enums);
    let j = &out.java;

    assert!(
        j.contains("SMA_Lookback("),
        "Java: MA should call SMA_Lookback"
    );
    assert!(
        j.contains("EMA_Lookback("),
        "Java: MA should call EMA_Lookback"
    );
    // Bare cross-indicator calls resolve to the callee's PUBLIC entry point
    // (#236 step 3), which returns an OutRange rather than writing the C-shaped
    // MInteger out-params. `= SMA(` anchors the call site so the dispatch arms
    // cannot substring-shadow one another.
    assert!(j.contains("= SMA("), "Java: MA should call the public SMA");
    assert!(j.contains("= EMA("), "Java: MA should call the public EMA");
    assert!(
        !j.contains("SMA_Impl(") && !j.contains("EMA_Impl("),
        "Java: MA must not call a callee's C-shaped tier"
    );
    assert!(
        !j.contains("smaUnguardedInternal(") && !j.contains("emaUnguardedInternal("),
        "Java: MA must not call the unguarded cores"
    );
}

#[test]
fn test_ma_rust_cross_calls() {
    let (func, enums) = load_indicator("ma");
    let out = generate_all(&func, &enums);
    let r = &out.rust;

    // Lookback calls remain the same.
    assert!(
        r.contains("self.SMA_Lookback("),
        "Rust: MA should call self.SMA_Lookback"
    );
    assert!(
        r.contains("self.EMA_Lookback("),
        "Rust: MA should call self.EMA_Lookback"
    );
    // Bare cross-indicator calls resolve to the callee's PUBLIC entry point
    // (#267), as they do in C, Java and C#: the returned OutRange is bound to a
    // `_xrN` local and both out-params are assigned from it. `match self.SMA(`
    // anchors the call site so the dispatch arms cannot substring-shadow one
    // another, and `self.` makes these calls rather than definitions, so the
    // negatives below are real.
    assert!(
        r.contains("match self.SMA("),
        "Rust: MA should call the public self.SMA"
    );
    assert!(
        r.contains("match self.EMA("),
        "Rust: MA should call the public self.EMA"
    );
    assert!(
        !r.contains("self.SMA_Impl(") && !r.contains("self.EMA_Impl("),
        "Rust: MA must not call a callee's C-shaped tier"
    );
    assert!(
        !r.contains("self.sma_unguarded(") && !r.contains("self.ema_unguarded("),
        "Rust: MA must not call the unguarded variants"
    );
}

// ---------------------------------------------------------------------------
// 4. Logic vs guarded validation tests
// ---------------------------------------------------------------------------
#[test]
fn test_c_sma_guarded_has_validation() {
    let (func, enums) = load_indicator("sma");
    let out = generate_all(&func, &enums);

    // Bounded by the float twin, which directly follows the double guarded body.
    let guarded = extract_section(&out.c, "TA_RetCode TA_SMA(", "TA_RetCode TA_S_SMA(");
    assert!(
        guarded.contains("TA_OUT_OF_RANGE_START_INDEX"),
        "C guarded SMA should have start index validation"
    );
    assert!(
        guarded.contains("TA_OUT_OF_RANGE_END_INDEX"),
        "C guarded SMA should have end index validation"
    );
}

#[test]
fn test_c_synth_private_omits_validation() {
    // Exactly one tier validates: the guarded entry point, not `_Private`.
    // Anchored on the SYNTH4 gate fixture — no shipped indicator declares an
    // explicit _private (EMA's was folded away in #183).
    let (func, enums) = load_synth("synth4");
    let out = generate_all(&func, &enums);

    let private = extract_section(
        &out.c,
        "static TA_RetCode TA_SYNTH4_Private(",
        "TA_LIB_API TA_RetCode TA_SYNTH4(",
    );
    assert!(
        !private.contains("TA_OUT_OF_RANGE_START_INDEX"),
        "C SYNTH4 _Private should NOT have start index validation"
    );
    assert!(
        !private.contains("TA_OUT_OF_RANGE_END_INDEX"),
        "C SYNTH4 _Private should NOT have end index validation"
    );
}

/// The validation prologue of one C batch entry point: from the `startIdx` guard
/// to the blank line that closes the checks. The index guards are followed by a
/// blank line of their own, so the prologue ends at the SECOND one.
fn c_batch_prologues(c: &str) -> Vec<&str> {
    const HEAD: &str = "if( (startIdx < 0) || (startIdx > TA_MAX_INDEX) )";
    let mut out = Vec::new();
    let mut from = 0;
    while let Some(rel) = c[from..].find(HEAD) {
        let start = from + rel;
        let first = start
            + c[start..]
                .find("\n\n")
                .expect("the index guards end in a blank line");
        let end = first
            + 2
            + c[first + 2..]
                .find("\n\n")
                .expect("a prologue ends in a blank line");
        out.push(&c[start..end]);
        from = end;
    }
    out
}

/// `docs/error-handling-spec.md` 2.2: B1, B2, then B3 — an optional parameter
/// outside its documented domain — and only then B4, a required argument that was
/// not supplied.
///
/// The parameter rule leads because it is the one every backend can express: a
/// Rust slice and a C# span cannot be absent, so B4 is C's and Java's alone, and
/// putting it last is what lets a multi-fault call report the same condition in
/// all four.
///
/// Structural, and it has to be: B3 and B4 both answer `TA_BAD_PARAM`, so no
/// runtime probe can see the order between them. The range out-parameters are
/// part of B4 — an absent one used to be dereferenced.
#[test]
fn c_batch_prologue_orders_parameters_before_presence() {
    const OUT_META: &str = "if( !outBegIdx || !outNBElement )";
    let mut prologues = 0usize;
    let mut with_params = 0usize;

    for name in discover_indicators() {
        let Some((func, enums)) = try_load_indicator(&name) else {
            continue;
        };
        let Some(out) = try_generate_all(&func, &enums) else {
            continue;
        };
        for prologue in c_batch_prologues(&out.c) {
            prologues += 1;
            let where_ = format!("{}: {prologue}", func.name);

            let start = prologue
                .find("TA_OUT_OF_RANGE_START_INDEX")
                .unwrap_or_else(|| panic!("{where_}\nno startIdx guard"));
            let end = prologue
                .find("TA_OUT_OF_RANGE_END_INDEX")
                .unwrap_or_else(|| panic!("{where_}\nno endIdx guard"));
            assert!(start < end, "{where_}\nB1 must precede B2");

            let meta = prologue
                .find(OUT_META)
                .unwrap_or_else(|| panic!("{where_}\nthe range out-parameters are unchecked"));
            let mut presence = vec![meta];
            let mut inputs = Vec::new();
            for input in &func.inputs {
                let at = prologue
                    .find(&format!("if( !{} )", input.name))
                    .unwrap_or_else(|| panic!("{where_}\n{} is unchecked", input.name));
                inputs.push(at);
                presence.push(at);
            }
            let mut outputs = Vec::new();
            for output in &func.outputs {
                if output.is_nullable() {
                    continue;
                }
                let at = prologue
                    .find(&format!("if( !{} )", output.name))
                    .unwrap_or_else(|| panic!("{where_}\n{} is unchecked", output.name));
                outputs.push(at);
                presence.push(at);
            }
            let first_presence = *presence.iter().min().expect("out-meta is always present");
            assert!(end < first_presence, "{where_}\nB2 must precede B4");

            // The last sentinel substitution sits in the last parameter's block,
            // and its range check is the line right after it — so every presence
            // check has to follow it.
            let last_param = prologue
                .rfind("TA_INTEGER_DEFAULT")
                .into_iter()
                .chain(prologue.rfind("TA_REAL_DEFAULT"))
                .max();
            if let Some(at) = last_param {
                with_params += 1;
                assert!(end < at, "{where_}\nB2 must precede B3");
                assert!(at < first_presence, "{where_}\nB3 must precede B4");
            }
            if let (Some(last_in), Some(first_out)) =
                (inputs.iter().max(), outputs.iter().min())
            {
                assert!(last_in < first_out, "{where_}\ninputs precede outputs");
            }
        }
    }

    // Two per function, double and float. Literal floors: derived ones move with
    // whatever the scan happens to find.
    assert!(prologues >= 340, "only {prologues} C batch prologues scanned");
    assert!(
        with_params >= 150,
        "only {with_params} prologues carried parameter validation — B3 is barely covered"
    );
}

/// The output-distinctness guard of one Rust `_Impl`, reconstructed from the
/// function's outputs exactly as `rust_lang::gen_guarded_func` writes it.
///
/// Reconstructed, not searched for: a transcribed body can hold an `as_ptr()`
/// comparison of its own — BBANDS elects its scratch with one — so a substring
/// match would find that instead and read the order backwards.
/// RUST cannot compare a real output with an integer one: `*const f64 ==
/// *const i32` is a type error, so a cross-typed pair contributes no term and a
/// function whose outputs are all cross-typed gets no guard at all. (C can and
/// does compare them, through `const void *` — Appendix E. The rule differs per
/// backend, so do not read this as a statement about the library.) Without the
/// skip, reconstructing the guard reads a correctly-absent term as a missing one.
fn same_typed_outputs(a: &ir::Output, b: &ir::Output) -> bool {
    (a.param_type == ir::ParamType::Integer) == (b.param_type == ir::ParamType::Integer)
}

fn rust_alias_guard(func: &ir::FuncDef) -> Option<String> {
    // Both operands non-empty: two zero-length slices cannot clobber each other,
    // and every unallocated `Vec` hands out the same dangling pointer, so a bare
    // `as_ptr()` comparison rejected a call rules N1 and B5 both permit
    // (Appendix D item 11, #262). A nullable output is an `Option` and
    // contributes a term only when it was supplied (rule B6a).
    let mut pairs: Vec<String> = Vec::new();
    for i in 0..func.outputs.len() {
        for j in (i + 1)..func.outputs.len() {
            let (a, b) = (&func.outputs[i], &func.outputs[j]);
            if !same_typed_outputs(a, b) {
                continue;
            }
            pairs.push(match (a.is_nullable(), b.is_nullable()) {
                (false, false) => format!(
                    "(!{0}.is_empty() && !{1}.is_empty() && {0}.as_ptr() == {1}.as_ptr())",
                    a.name, b.name
                ),
                (true, false) => format!(
                    "{0}.as_deref().is_some_and(|a| !a.is_empty() && !{1}.is_empty() && a.as_ptr() == {1}.as_ptr())",
                    a.name, b.name
                ),
                (false, true) => format!(
                    "{1}.as_deref().is_some_and(|b| !{0}.is_empty() && !b.is_empty() && {0}.as_ptr() == b.as_ptr())",
                    a.name, b.name
                ),
                (true, true) => format!(
                    "{0}.as_deref().zip({1}.as_deref()).is_some_and(|(a, b)| !a.is_empty() && !b.is_empty() && a.as_ptr() == b.as_ptr())",
                    a.name, b.name
                ),
            });
        }
    }
    if pairs.is_empty() {
        return None;
    }
    Some(format!(
        "        if {} {{\n            return RetCode::BadParam;\n        }}\n",
        pairs.join(" || ")
    ))
}

/// The term a cross-typed pair WOULD contribute if the generator stopped
/// skipping them. Reconstructed so the "no guard here" branch below asserts an
/// absence it can name, rather than the absence of any `as_ptr()` at all — a
/// transcribed body may carry one of its own.
fn rust_cross_typed_term(a: &ir::Output, b: &ir::Output) -> String {
    format!("{0}.as_ptr() == {1}.as_ptr()", a.name, b.name)
}

/// `docs/error-handling-spec.md` 2.2: B1, B2, B3, then B5 — a buffer too short —
/// and only then B6, two outputs that are the same buffer.
///
/// Rust is the one backend where the order between those last two is
/// *observable*, and it had them the wrong way round (#261). Here B5 is an
/// `assert!` rather than a returned code — footnote [5], the LLVM proof that
/// elides the per-access bounds checks — so a call that is both undersized and
/// aliased answered `BadParam` where the specified order makes it a panic. C,
/// Java and C# answer `TA_BAD_PARAM` for either, so no order is owed there.
///
/// **This tier, not the shipped one.** Since #265 the public entry point states
/// B5 as a returned code ahead of both of these
/// ([`rust_public_entry_orders_the_argument_contract`]), so a caller of the
/// crate meets one code for either fault and cannot see the order at all. What
/// this pins is `_Impl` as the phantom-I/O sweep reaches it — since #267 the
/// only in-crate caller that does — where the distinction is still a panic
/// against a return.
///
/// Structural, and it has to be: two `&mut [f64]` cannot alias, so safe code
/// cannot build the multi-fault call this pins.
#[test]
fn rust_batch_impl_orders_capacity_before_aliasing() {
    let mut scanned = 0usize;
    let mut with_params = 0usize;

    for name in discover_indicators() {
        let Some((func, enums)) = try_load_indicator(&name) else {
            continue;
        };
        // One output cannot alias a second one; the guard is not emitted.
        if func.outputs.len() < 2 {
            continue;
        }
        let Some(out) = try_generate_all(&func, &enums) else {
            continue;
        };
        // Spans the FMA dispatch trio where there is one: the two wrappers carry
        // no prologue, so the markers below still land in `_Impl_impl`.
        let section = extract_section(
            &out.rust,
            &format!("pub(crate) fn {}_Impl(", func.name),
            &format!("pub fn {}(", func.name),
        );
        scanned += 1;
        let where_ = format!("{}: {section}", func.name);

        let start = section
            .find("RetCode::OutOfRangeStartIndex")
            .unwrap_or_else(|| panic!("{where_}\nno startIdx guard"));
        let end = section
            .find("RetCode::OutOfRangeEndIndex")
            .unwrap_or_else(|| panic!("{where_}\nno endIdx guard"));
        assert!(start < end, "{where_}\nB1 must precede B2");

        let preamble = section
            .find("let _assertLb")
            .unwrap_or_else(|| panic!("{where_}\nno bounds-assert preamble"));
        assert!(end < preamble, "{where_}\nB2 must precede B5");

        // The sentinel substitution opens each parameter's block and its range
        // check is the arm right after, so the last one bounds all of B3.
        // Reconstructed per parameter, like the guard below: a bare `i32::MIN`
        // could come from a transcribed body. An enum parameter is skipped —
        // it has no out-of-domain value, so it emits a substitution and no check.
        let last_param = func
            .optional_inputs
            .iter()
            .filter_map(|opt| {
                let head = match opt.param_type {
                    ir::ParamType::Integer => {
                        format!("if (({}) as i32) == (i32::MIN) {{", opt.name)
                    }
                    ir::ParamType::Real => format!("if {} == Self::REAL_DEFAULT {{", opt.name),
                    _ => return None,
                };
                section.find(&head)
            })
            .max();
        if let Some(at) = last_param {
            with_params += 1;
            assert!(end < at, "{where_}\nB2 must precede B3");
            assert!(at < preamble, "{where_}\nB3 must precede B5");
        }

        // A function whose outputs are ALL cross-typed gets no guard at all here:
        // in Rust the two slice types cannot be compared, so no pair contributes
        // a term. It is still scanned — dropping it would take its capacity
        // asserts out of this sweep with it — and the absence of the guard is
        // itself asserted, since emitting one would mean the generator had
        // started comparing `*const f64` against `*const i32`.
        let guard_at = match rust_alias_guard(&func) {
            Some(g) => section
                .find(&g)
                .unwrap_or_else(|| panic!("{where_}\nthe outputs are not checked for aliasing")),
            None => {
                for i in 0..func.outputs.len() {
                    for j in (i + 1)..func.outputs.len() {
                        let term = rust_cross_typed_term(&func.outputs[i], &func.outputs[j]);
                        assert!(
                            !section.contains(&term),
                            "{where_}\ncross-typed outputs are compared for aliasing ({term})"
                        );
                    }
                }
                section.len()
            }
        };
        // Every output's capacity is B5, so the guard has to follow ALL of the
        // asserts, not merely the first.
        for output in &func.outputs {
            // A declined output has no capacity to bound, so its assert asks the
            // question only when one was supplied (rule B6a).
            let cap = if output.is_nullable() {
                format!(
                    "assert!(_assertStart > endIdx || {}.as_deref().is_none_or(|o| endIdx - _assertStart < o.len()));",
                    output.name
                )
            } else {
                format!(
                    "assert!(_assertStart > endIdx || endIdx - _assertStart < {}.len());",
                    output.name
                )
            };
            let cap_at = section
                .find(&cap)
                .unwrap_or_else(|| panic!("{where_}\n{} has no capacity assert", output.name));
            assert!(cap_at < guard_at, "{where_}\nB5 must precede B6 ({})", output.name);
        }
    }

    // Literal floors: derived ones move with whatever the scan happens to find.
    assert!(scanned >= 15, "only {scanned} multi-output Rust bodies scanned");
    assert!(
        with_params >= 12,
        "only {with_params} carried parameter validation — B3 is barely covered"
    );
}

/// The Rust public entry point states the argument contract in the specified
/// order: B1, B2, then B3, then B4/B5 — inputs before outputs (#265).
///
/// **The order is not a style choice.** `SMA(10, 9, ..)` with an eight-element
/// series has two faults at once, and the specification says `endIdx < startIdx`
/// answers first. Put the input bound at the top and it answers `BadParam`,
/// where `test_index_range_xlang` requires `TA_OUT_OF_RANGE_END_INDEX` in every
/// language — and that leg is also the sole producer of codes 12 and 13 for the
/// retCode census floor, so getting it wrong fails two gates for one reason.
/// B3 rides on the `<N>_Lookback(..)?`, which is what makes an out-of-range
/// parameter answer ahead of a short buffer, as it does in Java and C#.
///
/// Structural, because the runtime leg reaches three functions
/// (`TA_SMA`, `TA_BBANDS`, `TA_AD`) and this reaches all 176. It is the only
/// STATIC pin: `test_index_range_xlang` covers the same order at run time, and
/// covers it well — its five cases all run on a fixed eight-element series, so
/// four of them pair a short input with a malformed or oversized range — but it
/// needs a built server and a live oracle, which the PR gate has neither of.
#[test]
fn rust_public_entry_orders_the_argument_contract() {
    let mut scanned = 0usize;
    let mut with_params = 0usize;
    let mut inputs_checked = 0usize;
    let mut outputs_checked = 0usize;

    for name in discover_indicators() {
        let Some((func, enums)) = try_load_indicator(&name) else {
            continue;
        };
        let Some(out) = try_generate_all(&func, &enums) else {
            continue;
        };
        // Everything the public entry does BEFORE handing over to the numerics.
        // Bounded by that call, so a check emitted after it cannot satisfy this.
        let snake = func.name.clone();
        let section = extract_section(
            &out.rust,
            &format!("    pub fn {snake}(\n"),
            &format!("        let retCode = self.{snake}_Impl("),
        );
        scanned += 1;
        let where_ = format!("{snake}: {section}");

        let b1 = section
            .find("return Err(RetCode::OutOfRangeStartIndex);")
            .unwrap_or_else(|| panic!("{where_}\nno startIdx guard"));
        let b2 = section
            .find("return Err(RetCode::OutOfRangeEndIndex);")
            .unwrap_or_else(|| panic!("{where_}\nno endIdx guard"));
        assert!(b1 < b2, "{where_}\nB1 must precede B2");

        // B3 arrives as the lookback's `?` — rule L2 makes the lookback's
        // parameter decision this tier's own, so one call buys the check and the
        // clamp. It has to sit below B2 and above every buffer bound.
        let b3 = section
            .find(&format!("let _guardLb = self.{snake}_Lookback("))
            .unwrap_or_else(|| panic!("{where_}\nno lookback call to carry B3 and the clamp"));
        assert!(b2 < b3, "{where_}\nB2 must precede B3");
        if !func.optional_inputs.is_empty() {
            with_params += 1;
        }

        let mut last_input = b3;
        for input in &func.inputs {
            let at = section
                .find(&format!("if {}.len() < endIdx + 1 {{", input.name))
                .unwrap_or_else(|| panic!("{where_}\n{} has no input bound", input.name));
            assert!(b3 < at, "{where_}\nB3 must precede B5 ({})", input.name);
            inputs_checked += 1;
            last_input = last_input.max(at);
        }
        for output in &func.outputs {
            let needle = if output.is_nullable() {
                format!("if {}.as_deref().is_some_and(|o| o.len() < _guardOutLen) {{", output.name)
            } else {
                format!("if {}.len() < _guardOutLen {{", output.name)
            };
            let at = section
                .find(&needle)
                .unwrap_or_else(|| panic!("{where_}\n{} has no output bound", output.name));
            assert!(
                last_input < at,
                "{where_}\nevery input is bounded before any output ({})",
                output.name
            );
            outputs_checked += 1;
        }
    }

    // Literal floors: derived ones move with whatever the scan happens to find.
    assert!(scanned >= 200, "only {scanned} public entries scanned");
    assert!(with_params >= 80, "only {with_params} carried optional parameters");
    assert!(inputs_checked >= 380, "only {inputs_checked} input bounds found");
    assert!(outputs_checked >= 190, "only {outputs_checked} output bounds found");
}

/// Rust's metadata tier calls the PUBLIC entry point, so every rule the batch
/// contract states holds through the binder too (#265).
///
/// It called `<N>_Impl` and re-implemented one bound of its own —
/// `end_idx - start_idx + 1`, the width of the requested range where B5 says the
/// count actually produced — and checked no input length at all, so a leg
/// shorter than the range reached the numerics and tripped their `assert!`: a
/// panic out of a `Result`-typed method. C's frames and Java's `Dispatch` have
/// always called the public tier; this is what made Rust's binder agree.
///
/// Structural, and it has to be: the runtime pin is `binder_tests` inside the
/// generated crate, which only `cargo test --tests -p ta-lib` executes — a
/// nightly step. This runs on the PR gate.
#[test]
fn rust_binder_calls_the_public_tier() {
    let base = Path::new(env!("CARGO_MANIFEST_DIR")).join("../../ta_codegen/input");
    let enums = parser::enums::load_enums(&base.join("enums.yaml"));
    let funcs: Vec<ir::FuncDef> = discover_indicators()
        .iter()
        .map(|n| parser::yaml::parse_yaml(&base.join(format!("{n}/{n}.yaml"))))
        .collect();
    let out = backends::rust_abstract::render(&funcs, &enums);

    let mut called = 0usize;
    let mut guarded = 0usize;
    for f in &funcs {
        let n = &f.name;
        assert!(
            out.contains(&format!("let res = self.core.{n}(")),
            "{n}: the binder arm does not call the public entry point"
        );
        assert!(
            !out.contains(&format!("self.core.{n}_Impl(")),
            "{n}: the binder arm still calls the numerics tier — the argument \
             contract stops applying to it"
        );
        called += 1;
    }
    // Multi-output arms take their buffers one at a time, so presence has to be
    // settled before the first take or a rejection leaves the holder with the
    // earlier ones missing and every later call answers BadParam. Reconstructed
    // from the row model, so the needle names the arm's own slots.
    for r in all_abstract_rows() {
        if r.outputs.len() < 2 {
            continue;
        }
        let slots: Vec<String> = r
            .outputs
            .iter()
            .enumerate()
            .map(|(k, o)| {
                let arr = match o.kind {
                    ta_codegen_lib::backends::abstract_rows::OutputKind::Integer => "int_out",
                    ta_codegen_lib::backends::abstract_rows::OutputKind::Real => "real_out",
                };
                format!("self.{arr}[{k}].is_none()")
            })
            .collect();
        // The guard, then the first take, with nothing between them: emitted as
        // one block, so this pins the ORDER and not merely the presence of both.
        let arr0 = match r.outputs[0].kind {
            ta_codegen_lib::backends::abstract_rows::OutputKind::Integer => "int_out",
            ta_codegen_lib::backends::abstract_rows::OutputKind::Real => "real_out",
        };
        let needle = format!(
            "if {} {{ return Err(RetCode::BadParam); }}\n                let mut o0 = self.{arr0}[0].take()",
            slots.join(" || ")
        );
        assert!(
            out.contains(&needle),
            "{}: no presence guard immediately ahead of the first take — expected `{needle}`",
            r.name
        );
        guarded += 1;
    }
    // The bound this tier stopped stating for itself must be gone, not merely
    // unreachable: `need` was the requested width and rejected a caller who had
    // sized by the published formula.
    assert!(
        !out.contains("let need = end_idx - start_idx + 1;") && !out.contains(".len() < need"),
        "the binder's own output bound is back; there must be exactly one, and it \
         is the public tier's"
    );
    assert!(called >= 200, "only {called} binder arms scanned");
    assert!(guarded >= 12, "only {guarded} multi-output arms carried a presence guard");
}

/// The metadata tier's price setter validates every consumed component before
/// writing any of them, in all four backends (#266).
///
/// A rejected setter has to leave the holder as it found it. Interleaved, it
/// committed the components ahead of the offending one, and since the bitmaps
/// only ever clear, a caller re-binding a bundle that already worked then
/// computed over a mixture of the two -- in C# with no code and no exception.
///
/// Structural, and on the PR gate, because the four runtime probes are not:
/// the C one needs a built `ta_regtest`, the Rust one runs under
/// `cargo test --tests -p ta-lib`, and the Java and C# suites are compiled by no
/// CI job at all. All four are nightly-only, so this is the one thing a PR sees.
#[test]
fn metadata_price_setter_validates_before_writing() {
    let base = Path::new(env!("CARGO_MANIFEST_DIR")).join("../../ta_codegen/input");
    let enums = parser::enums::load_enums(&base.join("enums.yaml"));
    let funcs: Vec<ir::FuncDef> = discover_indicators()
        .iter()
        .map(|n| parser::yaml::parse_yaml(&base.join(format!("{n}/{n}.yaml"))))
        .collect();

    // Each backend's setter, and the write that must not sit inside the
    // validating loop. Whole-file needles: these are fixed scaffolding, emitted
    // once, not per function.
    let rust = backends::rust_abstract::render(&funcs, &enums);
    let section = extract_section(&rust, "pub fn set_price_input(", "\n    /// Bind an optional parameter");
    let check = section
        .find("if flags.0 & (1u32 << i) != 0 && series.is_none() {")
        .expect("rust: no per-component validation");
    let write = section
        .find("self.price[slot] = given;")
        .expect("rust: the write is not the whole-bundle commit — an indexed write is \
                 the interleaved shape #266 removed");
    assert!(check < write, "rust: the bundle is committed before the last component is checked");

    // C, from the same emitter that writes src/ta_abstract/ta_abstract.c. The
    // two macro passes are the fix; one combined macro is the defect.
    let c = backends::ta_abstract_c::render_ta_abstract_c();
    let c_check = c.find("#define CHECK_PARAM_INFO").expect("c: no checking pass");
    let c_write = c.find("#define SET_PARAM_INFO").expect("c: no writing pass");
    assert!(c_check < c_write, "c: the writing macro is defined before the checking one");
    assert!(
        c.matches("CHECK_PARAM_INFO(").count() >= 7 && c.matches("SET_PARAM_INFO(").count() >= 7,
        "c: each macro must be expanded for all six components (plus its #define)"
    );
    let last_check = c.rfind("CHECK_PARAM_INFO(openInterest").expect("c: no openInterest check");
    let first_write = c
        .find("SET_PARAM_INFO(open, OPEN )")
        .expect("c: no open write");
    assert!(
        last_check < first_write,
        "c: a component is written before the last one is checked"
    );
    // The writing macro must NOT carry the NULL test any more -- if it does, the
    // two passes were merged back and the check pass is decoration.
    let set_body = extract_section(&c, "#define SET_PARAM_INFO", "SET_PARAM_INFO(open, OPEN )");
    assert!(
        !set_body.contains("return TA_BAD_PARAM;"),
        "c: the writing macro still rejects, so it is still the interleaved shape"
    );
    // ...and it must still skip an unconsumed component rather than clobbering it.
    assert!(
        set_body.contains("paramInfo->flags & TA_IN_PRICE_##upperParam"),
        "c: the writing macro lost its flag guard, so it now overwrites components \
         the function does not consume"
    );

    // C#: two loops, the writing one second.
    let registry = make_registry();
    let helpers = HelperRegistry::empty();
    let _ = (&registry, &helpers);
    let cs = backends::csharp_metadata::render_function_call();
    let cs_sec = extract_section(&cs, "public FunctionCall SetPriceInput(int slot, double[]? open", "private OptInputInfo CheckOpt(");
    let cs_check = cs_sec
        .find("if (info.Requires(all[i]) && given[i] is null)")
        .expect("csharp: no per-component validation");
    let cs_write = cs_sec
        .find("_price[slot][i] = given[i];")
        .expect("csharp: no write");
    assert!(
        cs_check < cs_write,
        "csharp: the write is inside the validating loop -- the interleaved shape"
    );
    assert_eq!(
        cs_sec.matches("for (int i = 0; i < all.Length; i++)").count(),
        2,
        "csharp: the validating and writing passes must be two separate loops"
    );

    // Java was already correct; pin it so it stays the shape the other three copy.
    let java = backends::java_metadata::render_param_holder();
    let j_sec = extract_section(&java, "public ParamHolder setPriceInput(", "\n   /**");
    let j_check = j_sec.find("throw new IllegalArgumentException(").expect("java: no validation");
    let j_write = j_sec.find("priceInputs[idx] = c;").expect("java: no commit");
    assert!(j_check < j_write, "java: the bundle is committed before validation");
}

/// Rust's transcribed bodies call their callee's PUBLIC entry point, as C, Java
/// and C# do (#267).
///
/// They called `<N>_Impl`, which made Rust the one backend where a composed path
/// did not meet the argument contract the public tier has owned since #265: the
/// same undersized buffer was a `RetCode` through `pub fn` and a panic through a
/// sibling. C cannot go the other way — a cross-call is cross-TU there, so a C
/// `_Impl` could not be `static` and would be new ABI in the shipped `.so`.
///
/// Structural, and it has to be: the runtime pin is `no_phantom_io` inside the
/// generated crate, which only `cargo test --tests -p ta-lib` executes — a
/// nightly step. This runs on the PR gate.
///
/// Derived from the IR, not from a hand list: the callee names come from walking
/// each body for a call whose arity matches the callee's full TA signature, so a
/// new composed indicator is pinned the day it lands. Both directions per callee,
/// so a half-applied change — one site the emitter moved and one it did not —
/// fails rather than passing on the half that moved.
///
/// BOTH tiers, because `rust_lang::generate` emits only the batch one: SAR,
/// SAREXT and STOCH cross-call from their streaming `<N>_OpenImpl` as well, and
/// those three sites have no other structural cover — their only runtime cover is
/// a `stream_verify` leg, which a PR run reaches none of.
#[test]
fn rust_cross_calls_target_the_public_tier() {
    use ta_codegen_lib::streaming::CalleeLookup;

    /// Every cross-indicator INVOCATION in `stmts`, in first-seen order. Arity is
    /// what separates a scalar builtin from the same-named indicator: `sqrt(x)`
    /// is libm's, `sqrt(startIdx, endIdx, in, &beg, &nb, out)` is TA_SQRT.
    fn callees(stmts: &[ir::Statement], reg: &Registry, into: &mut Vec<String>) {
        for st in stmts {
            ta_codegen_lib::streaming::walk_stmt_exprs(st, &mut |top| {
                ta_codegen_lib::streaming::walk_expr(top, &mut |e| {
                    if let ir::Expr::FuncCall(name, args) = e {
                        if let Some(sig) = reg.callee(name) {
                            let arity = 2 + sig.n_inputs + sig.n_opts + 2 + sig.n_outputs;
                            if args.len() == arity && !into.contains(name) {
                                into.push(name.clone());
                            }
                        }
                    }
                });
            });
        }
    }

    let registry = make_registry();
    let mut callers = 0usize;
    let mut sites = 0usize;
    let mut scanned = 0usize;
    for name in discover_indicators() {
        let (func, enums) = load_indicator(&name);
        let mut found: Vec<String> = Vec::new();
        callees(&func.body, &registry, &mut found);
        callees(func.stream_source(), &registry, &mut found);
        found.retain(|c| *c != name);
        scanned += 1;
        if found.is_empty() {
            continue;
        }
        let mut rust = generate_all(&func, &enums).rust;
        if func.streaming && backends::rust_stream::emits_stream(&func, &registry) {
            rust.push_str(&backends::rust_stream::generate(
                &func, &enums, &registry, &HelperRegistry::empty(),
            ));
        }
        callers += 1;
        for c in &found {
            let public = registry.resolve_call(c, Lang::Rust);
            assert!(
                rust.contains(&format!("self.{public}(")),
                "{name}: the cross-call to {public} does not name the public tier"
            );
            assert!(
                !rust.contains(&format!("self.{public}_Impl(")),
                "{name}: still calls {public}_Impl — the argument contract stops \
                 applying to that path"
            );
        }
        // The binding shape, counted once per site rather than per callee: an
        // aliased call (STOCH's in-place `ma`) hides the callee name inside the
        // `mem::swap` block, so a per-callee needle for `match self.NAME(` would
        // be satisfied by a sibling site and go quiet on that one.
        assert!(
            rust.contains("let _xr"),
            "{name}: cross-calls, but no OutRange binding — the out-params are \
             still being passed"
        );
        sites += rust.matches("let _xr").count();
    }

    // The `&mut _dup_out` dummy existed only to give SAR/SAREXT a second mutable
    // borrow of the scalar they passed as BOTH out-params. The public tier takes
    // neither, so it must be gone, not merely unreachable.
    let (sar, sar_enums) = load_indicator("sar");
    assert!(
        !generate_all(&sar, &sar_enums).rust.contains("_dup_out"),
        "the duplicate-out-param dummy is back; the public tier takes no out-params"
    );

    // Literal floors: derived ones move with whatever the scan happens to find,
    // and they are what makes this non-vacuous — `scanned` is incremented once
    // per corpus entry and could only disagree with the corpus by panicking
    // first, so it is these two that prove the sweep found anything at all.
    assert!(scanned >= 200, "only {scanned} indicators in the corpus");
    assert!(callers >= 14, "only {callers} composed indicators scanned");
    assert!(sites >= 39, "only {sites} cross-call sites scanned");
}

/// A cross-call's rejection is answered where the call is made, so the guard the
/// C source puts after it is dead in all three ported backends and must not
/// reach the output (#269 follow-up). C is exempt: it really does return a code.
///
/// Two tiers per backend, because a pin built on the batch emitter alone would
/// pass while every `_OpenImpl` site stayed dirty -- the miss already recorded
/// against #267. `assignments` carries a floor because "no dead guard" is
/// satisfied just as well by an emitter that stopped emitting the assignment.
///
/// What this CANNOT see is whether the fold is selective -- whether a guard
/// whose `|| count == 0` half is live kept that half. Text cannot tell that
/// survivor from an unrelated `if`. The unit tests in `backends::ir_cleanup`
/// carry that half, over IR, where it is decidable.
#[test]
fn an_answered_cross_call_guard_is_folded_in_every_ported_backend() {
    /// `(assignments, dead)` over one rendered body.
    ///
    /// `dead` is the defect: an assignment of the success literal followed --
    /// anywhere before that variable is next written -- by a test of it against
    /// the same literal. Deliberately NOT an adjacency test: the fold's own
    /// reason for skipping intervening statements is that `macdext.c` puts two
    /// `free()` calls between the call and its guard, and an adjacency test is
    /// blind to exactly the shape the fold exists to handle.
    fn scan(src: &str, success: &str) -> (usize, usize) {
        let lines: Vec<&str> = src.lines().collect();
        let (mut assigns, mut dead) = (0usize, 0usize);
        for (i, line) in lines.iter().enumerate() {
            let t = line.trim();
            let Some(var) = t
                .strip_suffix(&format!(" = {success};"))
                .filter(|v| !v.is_empty() && v.chars().all(|c| c.is_alphanumeric() || c == '_'))
            else {
                continue;
            };
            assigns += 1;
            let test = format!("{var} != {success}");
            for later in &lines[i + 1..] {
                let lt = later.trim();
                if lt.contains(&test) {
                    dead += 1;
                    break;
                }
                // Any later write to the variable -- including the declaration
                // that opens the next function, which is what bounds the window
                // to one body without parsing one.
                if lt.contains(var) && lt.contains('=') && !lt.contains("==") && !lt.contains("!=")
                {
                    break;
                }
            }
        }
        (assigns, dead)
    }

    let registry = make_registry();
    let helpers = HelperRegistry::empty();
    let (mut assigns, mut dead, mut scanned) = (0usize, 0usize, 0usize);

    for name in discover_indicators() {
        let (func, enums) = load_indicator(&name);
        scanned += 1;

        let mut rust = backends::rust_lang::generate(&func, &enums, &registry, &helpers);
        let mut java = backends::java::generate(&func, &enums, &registry, &helpers);
        let mut csharp = backends::csharp::generate(&func, &enums, &registry, &helpers);
        if func.streaming {
            if backends::rust_stream::emits_stream(&func, &registry) {
                rust.push_str(&backends::rust_stream::generate(&func, &enums, &registry, &helpers));
            }
            if backends::java_stream::emits_stream(&func, &registry) {
                java.push_str(&backends::java_stream::generate(&func, &enums, &registry, &helpers));
            }
            if backends::csharp_stream::emits_stream(&func, &registry) {
                csharp
                    .push_str(&backends::csharp_stream::generate(&func, &enums, &registry, &helpers));
            }
        }

        for (src, success, lang) in [
            (&rust, "RetCode::Success", "rust"),
            (&java, "RetCode.Success", "java"),
            (&csharp, "RetCode.Success", "csharp"),
        ] {
            let (a, d) = scan(src, success);
            assert_eq!(
                d, 0,
                "{name}/{lang}: {d} answered cross-call guard(s) still emitted — \
                 the fold did not reach this tier"
            );
            assigns += a;
            dead += d;
        }
    }

    assert_eq!(dead, 0, "{dead} dead guards survived");
    // Literal floors. Without them an emitter that stopped assigning the literal
    // would read green on a corpus with nothing left to check.
    assert!(scanned >= 200, "only {scanned} indicators in the corpus");
    assert!(assigns >= 100, "only {assigns} success assignments seen — the sweep found nothing");
}

/// Deallocation is removed from the IR for the backends that have none, and the
/// guard left behind goes with it — while C, which needs both, keeps both
/// (#269 follow-up).
///
/// The second half is the one that matters: nothing else would catch a cleanup
/// pass that leaked into `c.rs`, and the symptom would be a leaked buffer in the
/// shipped library rather than an ugly diff.
#[test]
fn deallocation_is_dropped_only_where_the_backend_has_none() {
    /// `if (...) {}` with nothing between the braces.
    fn inert_guards(src: &str, open: &str) -> usize {
        let lines: Vec<&str> = src.lines().map(str::trim).collect();
        lines
            .windows(2)
            .filter(|w| w[0].starts_with("if") && w[0].ends_with(open) && w[1] == "}")
            .count()
    }

    let registry = make_registry();
    let helpers = HelperRegistry::empty();
    let (mut ported_inert, mut c_guards, mut scanned) = (0usize, 0usize, 0usize);

    for name in discover_indicators() {
        let (func, enums) = load_indicator(&name);
        scanned += 1;

        let mut rust = backends::rust_lang::generate(&func, &enums, &registry, &helpers);
        let mut java = backends::java::generate(&func, &enums, &registry, &helpers);
        let mut csharp = backends::csharp::generate(&func, &enums, &registry, &helpers);
        let c = backends::c::generate(&func, &enums, &registry, &helpers);
        if func.streaming {
            if backends::rust_stream::emits_stream(&func, &registry) {
                rust.push_str(&backends::rust_stream::generate(&func, &enums, &registry, &helpers));
            }
            if backends::java_stream::emits_stream(&func, &registry) {
                java.push_str(&backends::java_stream::generate(&func, &enums, &registry, &helpers));
            }
            if backends::csharp_stream::emits_stream(&func, &registry) {
                csharp
                    .push_str(&backends::csharp_stream::generate(&func, &enums, &registry, &helpers));
            }
        }

        for (src, open, lang) in
            [(&rust, "{", "rust"), (&java, ") {", "java"), (&csharp, ") {", "csharp")]
        {
            let n = inert_guards(src, open);
            assert_eq!(n, 0, "{name}/{lang}: {n} guard(s) left with an empty body");
            ported_inert += n;
            assert!(
                !src.contains("free("),
                "{name}/{lang}: a free() reached the output — deallocation was not dropped"
            );
        }
        c_guards += c.matches("free(").count();
    }

    assert_eq!(ported_inert, 0);
    assert!(scanned >= 200, "only {scanned} indicators in the corpus");
    // C keeps every one. Without this floor the whole test is satisfied by a
    // pass that ran on C too and freed nothing anywhere.
    assert!(c_guards >= 10, "only {c_guards} free() call(s) left in C — the cleanup reached c.rs");
}

/// #269: a cross-indicator call's rejection must be answered before the body
/// uses anything the call was supposed to have written.
///
/// STOCH ran its `memmove` into `outSlowK` eight lines ABOVE the `retCode`
/// test, so a rejected `%D ma()` — which never wrote `*outNBElement` — copied
/// %K's count and overran the caller's buffer by `lookbackDSlow` doubles while
/// reporting an empty `OutRange`.
///
/// This is checked over the IR, not over one backend's text, because the IR is
/// what every backend transcribes. **C is the only backend where it can still go
/// wrong** — Rust, Java and C# answer the rejection at the call site now (#267)
/// and `ir_cleanup` removes the guard entirely, so their emitted code cannot
/// carry the defect and cannot witness it either.
///
/// The rule is deliberately narrow: only a statement that touches a DECLARED
/// OUTPUT or an out-param may not sit between the call and its test. Measured
/// over the corpus, the statements that legitimately sit there are an
/// `if`/`else` Peek-vs-Update pair, `sp->sub = sub` (MA storing the sub handle
/// so teardown can close it), and `free()` of a local scratch — none of which
/// touches an output. Forbidding all of them instead would fail 52 sites that
/// are correct.
#[test]
fn a_cross_call_rejection_is_answered_before_its_result_is_used() {
    use ta_codegen_lib::streaming::CalleeLookup;

    fn is_cross_call(v: &ir::Expr, reg: &Registry) -> Option<()> {
        let ir::Expr::FuncCall(name, args) = v else { return None };
        let sig = reg.callee(name)?;
        let arity = 2 + sig.n_inputs + sig.n_opts + 2 + sig.n_outputs;
        (args.len() == arity).then_some(())
    }

    /// Every name this statement mentions, in any spelling that carries one.
    fn names(st: &ir::Statement) -> Vec<String> {
        let mut out = Vec::new();
        ta_codegen_lib::streaming::walk_stmt_exprs(st, &mut |top| {
            ta_codegen_lib::streaming::walk_expr(top, &mut |e| match e {
                ir::Expr::Var(n) | ir::Expr::PointerDeref(n) | ir::Expr::ArrayAccess(n, _) => {
                    out.push(n.clone());
                }
                _ => {}
            });
        });
        out
    }

    fn is_control_flow(s: &ir::Statement) -> bool {
        matches!(
            s,
            ir::Statement::While { .. }
                | ir::Statement::DoWhile { .. }
                | ir::Statement::For { .. }
                | ir::Statement::ForC { .. }
                | ir::Statement::Switch { .. }
                | ir::Statement::If { .. }
                | ir::Statement::Return { .. }
                | ir::Statement::Break
                | ir::Statement::Continue
        )
    }

    /// Walk one statement list, then recurse. Returns the offending statements.
    fn scan(
        body: &[ir::Statement],
        outs: &[String],
        reg: &Registry,
        bad: &mut Vec<String>,
    ) {
        for (i, st) in body.iter().enumerate() {
            if let ir::Statement::Assign { target: ir::Expr::Var(v), value, .. } = st {
                if is_cross_call(value, reg).is_some() {
                    for later in &body[i + 1..] {
                        let touched = names(later);
                        // The test itself: an `if` mentioning the code variable.
                        if let ir::Statement::If { condition, .. } = later {
                            let mut hit = false;
                            ta_codegen_lib::streaming::walk_expr(condition, &mut |e| {
                                if matches!(e, ir::Expr::Var(n) if n == v) {
                                    hit = true;
                                }
                            });
                            if hit {
                                break;
                            }
                        }
                        if touched.iter().any(|n| {
                            outs.contains(n) || n == "outBegIdx" || n == "outNBElement"
                        }) {
                            bad.push(format!(
                                "a statement touching {:?} sits between the call assigning \
                                 `{v}` and its test",
                                touched
                                    .iter()
                                    .filter(|n| outs.contains(n)
                                        || *n == "outBegIdx"
                                        || *n == "outNBElement")
                                    .collect::<Vec<_>>()
                            ));
                            break;
                        }
                        if is_control_flow(later) {
                            break;
                        }
                    }
                }
            }
            // Recurse into every nested body.
            match st {
                ir::Statement::While { body, .. }
                | ir::Statement::DoWhile { body, .. }
                | ir::Statement::For { body, .. }
                | ir::Statement::ForC { body, .. }
                | ir::Statement::Block { body } => scan(body, outs, reg, bad),
                ir::Statement::If { then_body, else_body, .. } => {
                    scan(then_body, outs, reg, bad);
                    scan(else_body, outs, reg, bad);
                }
                ir::Statement::Switch { cases, default, .. } => {
                    for (_, b) in cases {
                        scan(b, outs, reg, bad);
                    }
                    scan(default, outs, reg, bad);
                }
                _ => {}
            }
        }
    }

    let registry = make_registry();
    let (mut scanned, mut composed) = (0usize, 0usize);
    for name in discover_indicators() {
        let (func, _enums) = load_indicator(&name);
        scanned += 1;
        let outs: Vec<String> = func.outputs.iter().map(|o| o.name.clone()).collect();
        let mut bad = Vec::new();
        for body in [func.body.as_slice(), func.private_body.as_slice(), func.stream_source()] {
            scan(body, &outs, &registry, &mut bad);
        }
        let mut any = false;
        for body in [func.body.as_slice(), func.stream_source()] {
            for st in body {
                if let ir::Statement::Assign { value, .. } = st {
                    if is_cross_call(value, &registry).is_some() {
                        any = true;
                    }
                }
            }
        }
        if any {
            composed += 1;
        }
        assert!(
            bad.is_empty(),
            "{name}: {} — this is #269, and C is the backend it reaches",
            bad.join("; ")
        );
    }
    assert!(scanned >= 200, "only {scanned} indicators in the corpus");
    assert!(composed >= 10, "only {composed} composed indicators found — the sweep found nothing");
}

#[test]
fn test_java_sma_guarded_has_validation() {
    let (func, enums) = load_indicator("sma");
    let out = generate_all(&func, &enums);

    // Extract the double-precision core, bounded before the float overload
    // Bounded to the DOUBLE core alone: the float twin is an overload with the
    // same name, so a marker that spans both would let it satisfy the assertion.
    let guarded = extract_section(&out.java, "RetCode SMA_Impl( int startIdx", "double inReal[]");
    let guarded = format!("{guarded}{}", extract_section(&out.java, "double inReal[]", "float inReal[]"));
    assert!(
        guarded.contains("OutOfRangeStartIndex"),
        "Java guarded SMA should have start index validation"
    );
}

#[test]
fn test_java_synth_private_omits_validation() {
    let (func, enums) = load_synth("synth4");
    let out = generate_all(&func, &enums);

    let private = extract_section(&out.java, "RetCode SYNTH4_Private(", "RetCode SYNTH4_Impl(");
    assert!(
        !private.contains("OutOfRangeStartIndex"),
        "Java SYNTH4_Private should NOT have start index validation"
    );
}

#[test]
fn test_rust_sma_guarded_has_validation() {
    let (func, enums) = load_indicator("sma");
    let out = generate_all(&func, &enums);

    // The guarded Rust function holds the algorithm and validates first, bounded
    // by the end of the impl block.
    let guarded = extract_section(&out.rust, "pub(crate) fn SMA_Impl(", "\n}\n");
    assert!(
        guarded.contains("endIdx < startIdx"),
        "Rust guarded SMA should have endIdx < startIdx check"
    );
}

#[test]
fn test_rust_synth_private_omits_validation() {
    let (func, enums) = load_synth("synth4");
    let out = generate_all(&func, &enums);

    // `pub(crate)`, matching C's file-`static` TA_SYNTH4_Private (#180): skipping
    // validation is only sound while the callers are the guarded bodies.
    let private = extract_section(&out.rust, "pub(crate) fn SYNTH4_Private(", "\n}\n");
    assert!(
        !private.contains("OutOfRangeStartIndex"),
        "Rust SYNTH4_Private should NOT have range validation"
    );
    assert!(
        !out.rust.contains("pub fn SYNTH4_Private("),
        "Rust synth4_private must not be crate-public: it is the one entry point with no \
         validation prologue, so a `pub` here bypasses the TA_MAX_INDEX bound (#180)"
    );
}

// Also test a different indicator for validation (RSI)
#[test]
fn test_c_rsi_guarded_has_validation() {
    let (func, enums) = load_indicator("rsi");
    let out = generate_all(&func, &enums);

    let guarded = extract_section(&out.c, "TA_RetCode TA_RSI(", "TA_RetCode TA_S_RSI(");
    assert!(
        guarded.contains("TA_OUT_OF_RANGE_START_INDEX"),
        "C guarded RSI should have start index validation"
    );
}

// ---------------------------------------------------------------------------
// 5. Indicator-specific feature tests
// ---------------------------------------------------------------------------

// --- RSI: unstable period + compatibility ---

#[test]
fn test_rsi_c_unstable_period() {
    let (func, enums) = load_indicator("rsi");
    let out = generate_all(&func, &enums);

    assert!(
        out.c.contains("TA_GLOBALS_UNSTABLE_PERIOD"),
        "C RSI should use TA_GLOBALS_UNSTABLE_PERIOD"
    );
    assert!(
        out.c.contains("TA_GLOBALS_COMPATIBILITY"),
        "C RSI should use TA_GLOBALS_COMPATIBILITY"
    );
}

// The IS_ZERO family reaches C as a macro call rather than being expanded to a
// literal comparison, so the epsilon has one definition (ta_utility.h) instead
// of one per body. Checked on the two forms an indicator still uses: the fixed
// band, which after #253 survives only where the guarded quantity is
// dimensionless (ADX tests a sum of two ratios), and the operand-scaled one
// that replaced it everywhere else (CCI tests a deviation against its own
// price level).
#[test]
fn test_c_keeps_is_zero_family_as_macros() {
    let (func, enums) = load_indicator("adx");
    let out = generate_all(&func, &enums);
    assert!(
        out.c.contains("TA_IS_ZERO("),
        "C ADX should use the TA_IS_ZERO macro"
    );

    let (func, enums) = load_indicator("cci");
    let out = generate_all(&func, &enums);
    assert!(
        out.c.contains("TA_IS_ZERO_SCALED("),
        "C CCI should use the TA_IS_ZERO_SCALED macro"
    );
}

#[test]
fn test_rsi_rust_unstable_period() {
    let (func, enums) = load_indicator("rsi");
    let out = generate_all(&func, &enums);

    assert!(
        out.rust.contains("unstable_period"),
        "Rust RSI should reference unstable_period"
    );
}

#[test]
fn test_rsi_java_unstable_period() {
    let (func, enums) = load_indicator("rsi");
    let out = generate_all(&func, &enums);

    assert!(
        out.java.contains("this.unstablePeriod"),
        "Java RSI should reference this.unstablePeriod"
    );
}

/// Java pins compatibility to Default and carries no such field, so the branches
/// are constant-folded at render time. RSI is the witness: its lookback has a
/// bare `== METASTOCK` test and its body a compound
/// `unstablePeriod == 0 && ... == METASTOCK` one, and both arms are dead here.
///
/// C renders the same IR and must keep both arms — that contrast is what makes
/// this non-vacuous (an empty Java body would satisfy the first assert alone).
#[test]
fn java_compatibility_is_folded_away() {
    for name in ["rsi", "cmo", "ema", "dema", "tema", "trix", "macd", "macdfix"] {
        let (func, enums) = load_indicator(name);
        let out = generate_all(&func, &enums);

        assert!(
            !out.java.contains("compatibility ==") && !out.java.contains("Compatibility."),
            "Java {name} must not reference the compatibility field — it is folded away"
        );
        assert!(
            out.c.contains("TA_GLOBALS_COMPATIBILITY"),
            "C {name} must keep both compatibility arms (proves the Java fold is \
             a backend choice, not an empty input)"
        );
    }
}

// --- EMA: unstable period + ARRAY_COPY ---

#[test]
fn test_ema_c_unstable_period() {
    let (func, enums) = load_indicator("ema");
    let out = generate_all(&func, &enums);

    assert!(
        out.c.contains("TA_GLOBALS_UNSTABLE_PERIOD"),
        "C EMA should use TA_GLOBALS_UNSTABLE_PERIOD"
    );
}

#[test]
fn test_ema_c_smoothing_factor() {
    let (func, enums) = load_indicator("ema");
    let out = generate_all(&func, &enums);

    // EMA takes optInK_1 as the smoothing factor parameter
    assert!(
        out.c.contains("optInK_1"),
        "C EMA should use optInK_1 smoothing factor parameter"
    );
}

#[test]
fn test_ema_java_smoothing_factor() {
    let (func, enums) = load_indicator("ema");
    let out = generate_all(&func, &enums);

    // Java EMA also uses optInK_1
    assert!(
        out.java.contains("optInK_1"),
        "Java EMA should use optInK_1 smoothing factor parameter"
    );
}

#[test]
fn test_ema_rust_unstable_period() {
    let (func, enums) = load_indicator("ema");
    let out = generate_all(&func, &enums);

    assert!(
        out.rust.contains("unstable_period"),
        "Rust EMA should reference unstable_period"
    );
}

// --- MA: switch/case with enum labels ---

#[test]
fn test_ma_c_switch_statement() {
    let (func, enums) = load_indicator("ma");
    let out = generate_all(&func, &enums);

    assert!(
        out.c.contains("switch(") || out.c.contains("switch ("),
        "C MA should contain a switch statement"
    );
    // Enum labels render as plain C enumerator names
    assert!(
        out.c.contains("case TA_MAType_SMA:"),
        "C MA should use plain C enumerator names for switch labels"
    );
}

#[test]
fn test_ma_java_switch_statement() {
    let (func, enums) = load_indicator("ma");
    let out = generate_all(&func, &enums);

    assert!(
        out.java.contains("switch(") || out.java.contains("switch ("),
        "Java MA should contain a switch statement"
    );
    // Enum switch case labels must be UNQUALIFIED ("case SMA:") — qualified
    // labels are Java 21+ syntax and the shipped Core.java must compile on
    // older JDKs.
    assert!(
        out.java.contains("case SMA:") || out.java.contains("case EMA:"),
        "Java MA should use unqualified enum case labels"
    );
    assert!(
        !out.java.contains("case MAType."),
        "Java switch case labels must not be qualified (Java 21+ only)"
    );
}

#[test]
fn test_ma_rust_switch_statement() {
    let (func, enums) = load_indicator("ma");
    let out = generate_all(&func, &enums);

    // Rust uses match instead of switch
    assert!(
        out.rust.contains("match "),
        "Rust MA should contain a match statement"
    );
}

// --- MULT: simple expression, no optional inputs ---

#[test]
fn test_mult_simplicity() {
    let (func, enums) = load_indicator("mult");
    let out = generate_all(&func, &enums);

    // MULT has no optional inputs
    assert!(
        func.optional_inputs.is_empty(),
        "MULT should have no optional inputs"
    );

    // MULT has exactly 2 inputs
    assert_eq!(func.inputs.len(), 2, "MULT should have exactly 2 inputs");

    // MULT has exactly 1 output
    assert_eq!(func.outputs.len(), 1, "MULT should have exactly 1 output");

    // C output should have multiplication
    assert!(
        out.c.contains("inReal0[") && out.c.contains("inReal1["),
        "C MULT should reference both input arrays"
    );

    // No unstable period, no COMPATIBILITY
    assert!(
        !out.c.contains("UNSTABLE_PERIOD"),
        "C MULT should NOT use UNSTABLE_PERIOD"
    );
}

// ---------------------------------------------------------------------------
// 6. Non-empty output checks for all discovered indicators
// ---------------------------------------------------------------------------

#[test]
fn test_all_indicators_nonempty_output() {
    let indicators = discover_indicators();
    assert!(!indicators.is_empty(), "No indicators discovered");

    let mut failures = Vec::new();
    let mut tested = 0;

    for name in &indicators {
        let loaded = try_load_indicator(name);
        let (func, enums) = match loaded {
            Some(v) => v,
            None => continue,
        };
        let out = match try_generate_all(&func, &enums) {
            Some(v) => v,
            None => continue,
        };

        let result = std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| {
            assert!(!out.c.is_empty(), "{}: C output is empty", name);
            assert!(!out.rust.is_empty(), "{}: Rust output is empty", name);
            assert!(!out.java.is_empty(), "{}: Java output is empty", name);

            assert!(out.c.len() > 200, "{}: C output suspiciously short", name);
            assert!(
                out.rust.len() > 200,
                "{}: Rust output suspiciously short",
                name
            );
            assert!(
                out.java.len() > 100,
                "{}: Java output suspiciously short",
                name
            );
        }));
        if let Err(e) = result {
            let msg = if let Some(s) = e.downcast_ref::<String>() {
                s.clone()
            } else if let Some(s) = e.downcast_ref::<&str>() {
                s.to_string()
            } else {
                format!("Unknown panic for indicator {}", name)
            };
            failures.push(msg);
        } else {
            tested += 1;
        }
    }

    assert!(
        tested >= 6,
        "Expected at least 6 indicators to pass non-empty checks, got {}",
        tested
    );

    if !failures.is_empty() {
        panic!(
            "{} indicator(s) failed non-empty checks:\n{}",
            failures.len(),
            failures.join("\n")
        );
    }

    eprintln!(
        "{} indicators produce non-empty output for all backends",
        tested
    );
}

// ---------------------------------------------------------------------------
// 10. Rust impl Core block structure (all indicators)
// ---------------------------------------------------------------------------

#[test]
fn test_rust_impl_core_structure() {
    let indicators = discover_indicators();
    let mut failures = Vec::new();

    for name in &indicators {
        let (func, enums) = match try_load_indicator(name) {
            Some(v) => v,
            None => continue,
        };
        let out = match try_generate_all(&func, &enums) {
            Some(v) => v,
            None => continue,
        };

        let result = std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| {
            assert!(
                out.rust.contains("impl Core {"),
                "Rust {}: missing impl Core block",
                name
            );
            assert!(
                out.rust.contains("use super::*;"),
                "Rust {}: missing use super::* import",
                name
            );
        }));
        if let Err(e) = result {
            let msg = if let Some(s) = e.downcast_ref::<String>() {
                s.clone()
            } else if let Some(s) = e.downcast_ref::<&str>() {
                s.to_string()
            } else {
                format!("Unknown panic for indicator {}", name)
            };
            failures.push(msg);
        }
    }

    if !failures.is_empty() {
        panic!(
            "{} indicator(s) failed Rust structure checks:\n{}",
            failures.len(),
            failures.join("\n")
        );
    }
}

// ---------------------------------------------------------------------------
// 11. WMA-specific: verify lookback uses optInTimePeriod
// ---------------------------------------------------------------------------

#[test]
fn test_wma_lookback_uses_time_period() {
    let (func, enums) = load_indicator("wma");
    let out = generate_all(&func, &enums);

    assert!(
        out.c.contains("optInTimePeriod"),
        "C WMA should reference optInTimePeriod"
    );
    assert!(
        out.rust.contains("optInTimePeriod"),
        "Rust WMA should reference optInTimePeriod"
    );
    assert!(
        out.java.contains("optInTimePeriod"),
        "Java WMA should reference optInTimePeriod"
    );
}

// ---------------------------------------------------------------------------
