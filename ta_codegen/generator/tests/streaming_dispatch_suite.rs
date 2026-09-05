//! Streaming dispatch emission (composed-tier fusion, dual-mode, Hilbert
//! transform state, CIRCBUF plumbing) and scratch-buffer election (#146).
//! Split out of the former `backend_suite.rs`.

#[path = "common/mod.rs"]
mod common;

use common::{
    discover_indicators, generate_all, load_indicator, load_indicator_with_source, load_synth,
    make_helpers, make_registry,
};
use std::collections::HashMap;
use std::path::Path;
use ta_codegen_lib::backends;
use ta_codegen_lib::helper_registry::HelperRegistry;
use ta_codegen_lib::ir;
use ta_codegen_lib::parser;
use ta_codegen_lib::registry::Registry;
use ta_codegen_lib::streaming;

// Streaming dispatch emission (TC composed tier: MA)
// ---------------------------------------------------------------------------

/// Pin the generated MA dispatch stream section: tagged handle over the
/// callees' PUBLIC streams, batch-order case arms, identity fast path, and
/// the MAMA arm forwarding the MAMA line while discarding FAMA as NULL (the
/// nullable-output delegation from issue #125 — no reject arm remains).
#[test]
fn test_c_ma_dispatch_stream_section() {
    let (mut func, enums) = load_indicator("ma");
    func.streaming = true; // the YAML flag flips with this milestone
    let registry = make_registry();
    let helpers = HelperRegistry::empty();
    let c = backends::c::generate(&func, &enums, &registry, &helpers);

    // Handle: params + a single tagged sub pointer, no StepImpl.
    assert!(c.contains("struct TA_MA_Stream {"), "state struct");
    assert!(c.contains("void *sub;"), "tagged sub-stream pointer");
    // Paired with the control below, because on its own this is an assertion
    // about a word: rename the transition tier and the negative goes true for
    // all 176 at once. SMA is generated here only to prove the name is still
    // the one the emitter writes, so MA's silence means something.
    assert!(!c.contains("TA_MA_StepImpl"), "dispatch has no transition fn");
    {
        let (mut sma, sma_enums) = load_indicator("sma");
        sma.streaming = true;
        let sma_c = backends::c::generate(&sma, &sma_enums, &registry, &helpers);
        assert!(
            sma_c.contains("TA_SMA_StepImpl( struct"),
            "control: a non-dispatch tier still spells its transition fn _StepImpl"
        );
    }

    // Open: identity path first (mirrors batch order), then the dispatch. The
    // anchor is resolved once per mode and the range is reported from it.
    //
    // The ORDER of the clamp and the history re-check — the property 96d1052f8
    // is about — is pinned in open_core_suite's
    // `dispatch_open_modes_differ_only_where_intended`, per mode, on a sliced
    // body. It cannot be pinned here: `c` is the whole file, the three lines are
    // emitted byte-identically by the scalar open and the anchored fill, and a
    // `contains` over both is satisfied by whichever arm is still correct.
    // Measured — reordering the scalar arm alone left every generator test green.
    assert!(
        c.contains(concat!(
            "      sp->outRangeBegIdx = fillLb;\n",
            "      sp->outRangeCount = historyLen - fillLb;\n",
        )),
        "the identity arm reports the anchor it actually used"
    );
    for (label, callee) in [
        ("Sma", "TA_SMA"),
        ("Ema", "TA_EMA"),
        ("Wma", "TA_WMA"),
        ("Dema", "TA_DEMA"),
        ("Tema", "TA_TEMA"),
        ("Kama", "TA_KAMA"),
        ("T3", "TA_T3"),
        ("Trima", "TA_TRIMA"), // dual-mode stream (M6c): auto-promoted from reject
        ("Mama", "TA_MAMA"),   // nullable FAMA (#125): auto-promoted from reject
    ] {
        assert!(
            c.contains(&format!("case TA_MAType_{}:", label.to_uppercase())),
            "supported arm case label for {label}"
        );
        assert!(c.contains(&format!("{callee}_OpenInternal(")), "sub open for {callee}");
        assert!(c.contains(&format!("{callee}_Update(")), "sub update for {callee}");
        assert!(c.contains(&format!("{callee}_Peek(")), "sub peek for {callee}");
        assert!(c.contains(&format!("{callee}_Close(")), "sub close for {callee}");
    }
    // T3's fixed vfactor literal forwards positionally; the dispatch threads
    // its own startIdx into the arm's internal open.
    assert!(
        c.contains("TA_T3_OpenInternal( &sub, inReal, startIdx, historyLen, optInTimePeriod, 0.7"),
        "T3 arm forwards the 0.7 vfactor literal + startIdx"
    );
    // MAMA is now a supported arm: FAMA is a nullable output (issue #125), so
    // MA's arm forwards the MAMA line to outReal and passes NULL for FAMA in
    // every verb (Open / OpenAndFill / Update / Peek). No reject arm remains.
    assert!(
        c.contains(
            "TA_MAMA_OpenInternal( &sub, inReal, startIdx, historyLen, 0.5, 0.05, outReal, NULL )"
        ),
        "MAMA arm forwards outReal + discards FAMA as NULL at Open"
    );
    assert!(
        c.contains("TA_MAMA_Update( (TA_MAMA_Stream *)stream->sub, inReal, outReal, NULL )"),
        "MAMA Update forwards outReal + NULL"
    );
    assert!(
        c.contains(
            "TA_MAMA_OpenAndFill( &sub, inReal, historyLen, 0.5, 0.05, outBegIdx, outNBElement, outReal, NULL )"
        ),
        "MAMA OpenAndFill forwards outReal + NULL"
    );
    assert!(!c.contains("/* no mama stream */"), "no MAMA reject arm remains");
    // Update/Peek identity short-circuit reads the handle's params; the guard
    // also covers the period-independent TA_MAType_DISABLED identity (issue #93).
    assert!(
        c.contains(
            "if( stream->optInTimePeriod == 1 || stream->optInMAType == TA_MAType_DISABLED )"
        ),
        "identity short-circuit on the handle (period 1 or DISABLED)"
    );
    // Peek keeps the handle logically const (const sub cast, no state copy).
    assert!(
        c.contains("(const TA_SMA_Stream *)stream->sub"),
        "const sub cast in Peek"
    );
}

/// `cond ? 1 : 0` collapses to the bare condition in Java and C#, and that is
/// only valid where a boolean is wanted. C has no booleans, so the destination
/// of an assignment never is — `outInteger[i] = a > b;` does not compile in
/// either language.
///
/// Unreachable from the corpus: its four `? 1 : 0` are all
/// `return (...) ? 1 : 0;` inside helper predicates, inlined into an `if`, where
/// the collapse is right. A synthetic fixture storing a flag is what found it
/// (#262), so the control below matters as much as the assertion — the collapse
/// must still happen in boolean position, or this "fix" would churn every
/// candlestick into `(x) != 0`.
#[test]
fn test_a_stored_bool_ternary_keeps_its_int_form() {
    let (func, enums) = load_indicator("minmaxindex");
    let registry = make_registry();
    let helpers = HelperRegistry::empty();
    let out = func.outputs[0].name.clone();

    let flag = || {
        ir::Expr::Ternary(
            Box::new(ir::Expr::BinOp(
                Box::new(ir::Expr::Var("today".to_string())),
                ir::BinOp::Greater,
                Box::new(ir::Expr::IntLiteral(0)),
            )),
            Box::new(ir::Expr::IntLiteral(1)),
            Box::new(ir::Expr::IntLiteral(0)),
        )
    };

    // Stored into an integer output: the ternary has to survive.
    // Streaming off on the mutated copies: appending a statement is not a shape
    // the stream planner is asked to understand, and the batch emitters are the
    // subject here.
    let mut stored = func.clone();
    stored.streaming = false;
    stored.body.push(ir::Statement::Assign {
        target: ir::Expr::ArrayAccess(
            out.clone(),
            Box::new(ir::Expr::Var("outIdx".to_string())),
        ),
        value: flag(),
        compound: false,
    });
    for (lang, text) in [
        ("Java", backends::java::generate(&stored, &enums, &registry, &helpers)),
        ("C#", backends::csharp::generate(&stored, &enums, &registry, &helpers)),
    ] {
        assert!(
            text.contains(&format!("{out}[outIdx] = (today > 0) ? 1 : 0;")),
            "{lang}: a flag stored into an integer output must keep `? 1 : 0`"
        );
        assert!(
            !text.contains(&format!("{out}[outIdx] = today > 0;")),
            "{lang}: the collapsed form does not compile — the destination is an int"
        );
    }

    // Control: in boolean position the collapse must still happen.
    let mut tested = func.clone();
    tested.streaming = false;
    tested.body.push(ir::Statement::If {
        condition: flag(),
        // A body, because `ir_cleanup::drop_inert_guards` removes a guard that
        // does nothing — which is the point of that pass, not a problem here.
        // The subject is the condition's rendering, and it needs the `if` to
        // survive to be seen.
        then_body: vec![ir::Statement::Break],
        else_body: vec![],
        cond_comments: vec![],
    });
    for (lang, text) in [
        ("Java", backends::java::generate(&tested, &enums, &registry, &helpers)),
        ("C#", backends::csharp::generate(&tested, &enums, &registry, &helpers)),
    ] {
        assert!(
            text.contains("if( today > 0 )"),
            "{lang}: a ternary in boolean position must still collapse"
        );
    }
}

/// Guarding a nullable store is complete only while the `outIdx` advance rides a
/// store that is always made. `mama.c` says so in a comment; this is what makes
/// it true.
///
/// The failure it forbids is silent: with the advance on the declined store, a
/// caller who declines gets `outNBElement = 0`, every other output written
/// repeatedly to index 0, and `Success`. The JSON-RPC servers bind every declared
/// output, so no cross-language gate ever makes that call — which is why the
/// generator refuses to emit the shape rather than a gate catching it.
///
/// Driven through `generate` rather than the helper, because the point is that a
/// contributor cannot reach the emitters without the check: every backend now
/// asks one producer for the declinable set, and asking is what runs it.
#[test]
fn test_a_nullable_output_may_not_carry_the_cursor() {
    let (func, enums) = load_indicator("mama");
    let registry = make_registry();
    let helpers = HelperRegistry::empty();

    // Control: the shipped shape renders. Without this the refusals below would
    // pass against a generator that refused MAMA outright.
    let _ = backends::c::generate(&func, &enums, &registry, &helpers);
    let _ = backends::rust_lang::generate(&func, &enums, &registry, &helpers);

    // The forbidden store, built directly: `outFAMA[outIdx++] = fama;` where
    // outFAMA is the declinable output.
    let cursor_on_nullable = ir::Statement::Assign {
        target: ir::Expr::ArrayAccess(
            "outFAMA".to_string(),
            Box::new(ir::Expr::PostIncrement(Box::new(ir::Expr::Var(
                "outIdx".to_string(),
            )))),
        ),
        value: ir::Expr::Var("fama".to_string()),
        compound: false,
    };
    let mut moved = func.clone();
    moved.body.push(cursor_on_nullable);
    type Emit = fn(&ir::FuncDef, &HashMap<String, ir::EnumDef>, &Registry, &HelperRegistry) -> String;
    for (lang, emit) in [
        ("C", backends::c::generate as Emit),
        ("Rust", backends::rust_lang::generate as Emit),
        ("Java", backends::java::generate as Emit),
        ("C#", backends::csharp::generate as Emit),
    ] {
        let moved = moved.clone();
        let enums = enums.clone();
        let err = std::panic::catch_unwind(move || {
            emit(&moved, &enums, &make_registry(), &HelperRegistry::empty())
        })
        .expect_err("a nullable output carrying the cursor must be refused");
        let msg = panic_message(&err);
        assert!(
            msg.contains("outFAMA") && msg.contains("PostIncrement"),
            "{lang}: the refusal must name the output and the step, got: {msg}"
        );
    }

    // And a function whose outputs are ALL declinable has nowhere to put it.
    let mut all_nullable = func.clone();
    for out in &mut all_nullable.outputs {
        if !out.is_nullable() {
            out.flags.push("nullable".to_string());
        }
    }
    let e2 = enums.clone();
    let err = std::panic::catch_unwind(move || {
        backends::c::generate(&all_nullable, &e2, &make_registry(), &HelperRegistry::empty())
    })
    .expect_err("every output declinable leaves the cursor no store to ride");
    assert!(
        panic_message(&err).contains("every output"),
        "the refusal must say why"
    );
}

fn panic_message(err: &Box<dyn std::any::Any + Send>) -> String {
    err.downcast_ref::<String>()
        .cloned()
        .or_else(|| err.downcast_ref::<&str>().map(|s| (*s).to_string()))
        .unwrap_or_default()
}

/// Rule B6, Appendix E: a **cross-typed** output pair is out of scope, so the
/// distinctness guard skips it in every backend.
///
/// Not reachable from a fixture, which is why it is a render pin. Three of the
/// four backends cannot even compile such a term — `double * == int *` is a
/// constraint violation in C, `double[] == int[]` is "incomparable types" in
/// Java, `*const f64 == *const i32` is a type error in Rust — and C# has always
/// skipped them because `Overlaps` is not defined across element types.
/// SYNTH12 does declare a mixed-type function now, but it cannot stand in for
/// this pin: its cross-typed pairs are exactly the ones the emitters drop, so
/// the fixture shows the term ABSENT and never shows it absent *for this
/// reason*. Re-typing an output here is what makes the omission attributable.
///
/// MINMAXINDEX is the vehicle: two integer outputs, one of them re-typed here,
/// which turns its single same-typed pair into a single cross-typed one. The
/// guard must then disappear entirely rather than emit an uncompilable term.
/// The two frame emitters subscript `outReal[]` / `outInteger[]` by the output's
/// DECLARATION position, and describe each output's type in a per-output
/// `TA_VOutIsInt_<N>[]`.
///
/// Nothing else on the PR gate can see either property. On a type-homogeneous
/// corpus the declaration index and a per-kind packed counter emit byte-identical
/// text, so `regen-check` is blind to a revert, and the harnesses that would
/// mis-read the table only run under the nightly synth gate. So this pins both
/// against a mixed function built here — the shape SYNTH12 carries, reached
/// without depending on the fixture, which lives outside `input/`.
#[test]
fn test_frames_index_outputs_by_declaration_position() {
    let (mut func, enums) = load_indicator("minmaxindex");
    assert_eq!(func.outputs.len(), 2, "MINMAXINDEX declares two outputs");
    // [integer, real]: the real output sits at declaration index 1, where a
    // per-kind counter would have called it outReal[0].
    func.outputs[1].param_type = ir::ParamType::Real;
    let (a, b) = (func.outputs[0].name.clone(), func.outputs[1].name.clone());

    let rendered = backends::variant_frame::render(std::slice::from_ref(&func), &enums);
    assert!(
        rendered.contains(&format!("outInteger[0] /* {a} */")),
        "output 0 is integer and must be outInteger[0]"
    );
    assert!(
        rendered.contains(&format!("outReal[1] /* {b} */")),
        "output 1 is real and must be outReal[1] — a per-kind counter would emit \
         outReal[0], which the harnesses read as output 0's buffer"
    );
    assert!(
        !rendered.contains(&format!("outReal[0] /* {b} */")),
        "the packed spelling must not come back"
    );
    assert!(
        rendered.contains("static const int TA_VOutIsInt_MINMAXINDEX[] = { 1, 0 };"),
        "the per-output type vector must describe each output, in order"
    );

    // The stream table reuses that same vector rather than emitting a second one.
    func.streaming = true;
    let streamed = backends::stream_frame::render(std::slice::from_ref(&func));
    assert!(
        streamed.contains("TA_VOutIsInt_MINMAXINDEX"),
        "the stream row must point at ta_variant_frame.h's vector"
    );
    assert!(
        streamed.contains(&format!("outReal[1] /* {b} */")),
        "the stream thunks must use the same declaration-position subscript"
    );
}

#[test]
fn test_cross_typed_output_pairs_are_not_compared() {
    let (mut func, enums) = load_indicator("minmaxindex");
    assert_eq!(func.outputs.len(), 2, "MINMAXINDEX declares two outputs");
    assert!(
        func.outputs.iter().all(|o| o.param_type == ir::ParamType::Integer),
        "both are integer outputs, so the control below is a real control"
    );
    let (a, b) = (func.outputs[0].name.clone(), func.outputs[1].name.clone());
    let registry = make_registry();
    let helpers = HelperRegistry::empty();

    // Control: same-typed, so every backend compares the pair.
    for (lang, out, needle) in [
        ("C", backends::c::generate(&func, &enums, &registry, &helpers), format!("{a} == {b}")),
        ("Java", backends::java::generate(&func, &enums, &registry, &helpers), format!("{a} == {b}")),
        ("Rust", backends::rust_lang::generate(&func, &enums, &registry, &helpers), format!("{a}.as_ptr() == {b}.as_ptr()")),
        ("C#", backends::csharp::generate(&func, &enums, &registry, &helpers), format!("{a}.Overlaps({b})")),
    ] {
        assert!(out.contains(&needle), "{lang}: a same-typed pair must be compared ({needle})");
    }

    // Re-type the second output. The pair is now cross-typed and must vanish.
    func.outputs[1].param_type = ir::ParamType::Real;
    for (lang, out) in [
        ("C", backends::c::generate(&func, &enums, &registry, &helpers)),
        ("Java", backends::java::generate(&func, &enums, &registry, &helpers)),
        ("Rust", backends::rust_lang::generate(&func, &enums, &registry, &helpers)),
        ("C#", backends::csharp::generate(&func, &enums, &registry, &helpers)),
    ] {
        for needle in [
            format!("{a} == {b}"),
            format!("{a}.as_ptr() == {b}.as_ptr()"),
            format!("{a}.Overlaps({b})"),
        ] {
            assert!(
                !out.contains(&needle),
                "{lang}: a cross-typed pair must not be compared ({needle})"
            );
        }
        // And nothing is left behind: no empty `if( )` where the guard was.
        assert!(
            !out.contains("if(  )") && !out.contains("if  {"),
            "{lang}: dropping the only pair must drop the whole guard, not leave an empty one"
        );
    }
}

/// Rule B6a (`docs/error-handling-spec.md` 2.2, issue #262): an omitted output
/// is accepted iff the .yaml marks it `nullable`. C has honoured it since #125;
/// this pins the other three, each of which spells "declined" in its own way.
///
/// The pins are per-backend on purpose. The cross-language gates cannot see any
/// of this: the servers always supply every output, so a backend that quietly
/// went back to requiring `outFAMA` would stay green everywhere.
#[test]
fn test_mama_nullable_fama_is_declinable_in_every_backend() {
    let (func, enums) = load_indicator("mama");
    let registry = make_registry();
    let helpers = HelperRegistry::empty();

    // Rust: `Option<&mut [f64]>` — the one backend that can spell "declined"
    // apart from "empty" and does, which leaves C# alone in overloading it.
    let rust = backends::rust_lang::generate(&func, &enums, &registry, &helpers);
    assert!(
        rust.contains("outFAMA: Option<&mut [f64]>"),
        "Rust declines a nullable output with None, not an empty slice"
    );
    assert!(
        rust.contains("if let Some(outFAMA) = outFAMA.as_deref_mut() {"),
        "every store into the declined output is guarded"
    );
    assert!(
        rust.contains(
            "assert!(_assertStart > endIdx || outFAMA.as_deref().is_none_or(|o| endIdx - _assertStart < o.len()));"
        ),
        "B5 asks for capacity only where an output was supplied"
    );

    // Java: `null`, and the length check has to be skipped for it and kept for
    // outMAMA — the half a caller actually sees.
    let java = backends::java::generate(&func, &enums, &registry, &helpers);
    assert!(
        java.contains(
            "if( outFAMA != null ) requireLength(\"MAMA\", \"outFAMA\", outFAMA, guardOutLen);"
        ),
        "Java requires a length only for a supplied output"
    );
    assert!(
        java.contains("requireLength(\"MAMA\", \"outMAMA\", outMAMA, guardOutLen);")
            && !java.contains("if( outMAMA != null ) requireLength"),
        "the non-nullable output stays unconditionally required"
    );
    assert!(
        java.contains("if( outFAMA != null )\n               outFAMA[outIdx] = fama;"),
        "every store into the declined output is guarded"
    );
    assert!(
        java.contains("if( outFAMA != null && outMAMA == outFAMA )"),
        "two nulls compare equal, so the pair guard checks the nullable operand first"
    );

    // C#: an empty span IS the declination — a `Span<T>` is a ref struct and a
    // null array converts to an empty one, so there is nothing else to use.
    let csharp = backends::csharp::generate(&func, &enums, &registry, &helpers);
    assert!(
        csharp.contains(
            "if( !outFAMA.IsEmpty ) RequireLength(\"MAMA\", \"outFAMA\", outFAMA.Length, guardOutLen);"
        ),
        "C# requires a length only for a supplied output"
    );
    assert!(
        csharp.contains("if( !outFAMA.IsEmpty )\n               outFAMA[outIdx] = fama;"),
        "every store into the declined output is guarded"
    );
    assert!(
        csharp.contains("if( outMAMA.Overlaps(outFAMA) ) {")
            && !csharp.contains("outMAMA.IsEmpty && outFAMA.IsEmpty"),
        "the empty-pair rejection is gone: it made 'declined' unspellable (item 11)"
    );

    // And MA's cross-call declines rather than allocating a buffer to throw away.
    let (ma, ma_enums) = load_indicator("ma");
    for (lang, out, want) in [
        ("Rust", backends::rust_lang::generate(&ma, &ma_enums, &registry, &helpers),
         "MAMA(startIdx, endIdx, inReal, 0.5, 0.05, outReal, None)"),
        ("Java", backends::java::generate(&ma, &ma_enums, &registry, &helpers),
         "MAMA(startIdx, endIdx, inReal, 0.5, 0.05, outReal, null)"),
        ("C#", backends::csharp::generate(&ma, &ma_enums, &registry, &helpers),
         "MAMA(startIdx, endIdx, inReal, 0.5, 0.05, outReal, default)"),
    ] {
        assert!(out.contains(want), "{lang}: MA's MAMA arm must decline FAMA ({want})");
        assert!(
            !out.contains("new double[(int)(endIdx - startIdx + 1)]")
                && !out.contains("&mut vec![0.0_f64; (endIdx - startIdx + 1) as usize][..]"),
            "{lang}: the throwaway FAMA buffer must be gone, not merely unread"
        );
    }
}

/// The Java argument helpers exist TWICE — hand-written in the shipped
/// `Core.java`, and as string literals in `server_gen.rs` that the JSON-RPC
/// server's inlined `Core` gets — and nothing compared them until this.
///
/// Neither copy is reachable from the other's tests: the shipped one is what
/// `BatchApiTest` drives, the server one is what `--codegen` and `--xlang-hash`
/// drive, and every server request hands `OpenAndFill` a full-length output, so
/// a divergence in the bound itself is invisible to both. Compared on tokens,
/// not on text: the two are indented differently on purpose.
#[test]
fn the_java_argument_helpers_agree_between_the_library_and_the_server() {
    fn method(src: &str, sig: &str, what: &str) -> String {
        let at = src
            .find(sig)
            .unwrap_or_else(|| panic!("{what}: `{sig}` not found"));
        let mut depth = 0usize;
        let mut end = at;
        for (i, ch) in src[at..].char_indices() {
            match ch {
                '{' => depth += 1,
                '}' => {
                    depth -= 1;
                    if depth == 0 {
                        end = at + i + 1;
                        break;
                    }
                }
                _ => {}
            }
        }
        assert!(end > at, "{what}: `{sig}` has no body");
        // Comments are not the contract: the shipped copy carries the prose,
        // the spliced one is deliberately bare. Only the code has to agree.
        let mut code = String::new();
        let mut rest = &src[at..end];
        while let Some(i) = [rest.find("//"), rest.find("/*")].into_iter().flatten().min() {
            code.push_str(&rest[..i]);
            rest = if rest[i..].starts_with("//") {
                rest[i..].find('\n').map_or("", |j| &rest[i + j..])
            } else {
                rest[i..].find("*/").map_or("", |j| &rest[i + j + 2..])
            };
        }
        code.push_str(rest);
        code.split_whitespace().collect::<Vec<_>>().join(" ")
    }

    let root = Path::new(env!("CARGO_MANIFEST_DIR")).join("../..");
    let core = std::fs::read_to_string(
        root.join("ta_codegen/output/java/library/src/main/java/io/github/talib/Core.java"),
    )
    .expect("the shipped Core.java");
    let server =
        std::fs::read_to_string(root.join("ta_codegen/output/java/tools/TaCodegenServe.java"))
            .expect("the generated Java server");

    // All ten, not the four the gate started with: `checkLength` is where rule
    // S5's rejection actually happens, and `failure` is the whole RetCode ->
    // exception mapping. Adding one is a line (issue #271 item 3).
    for sig in [
        "static RuntimeException failure(String funcName, RetCode retCode) {",
        "static int clampedStart(String funcName, int startIdx, int lookback) {",
        "static void requireLength(String funcName, String argName, double[] array, int required) {",
        "static void requireLength(String funcName, String argName, float[] array, int required) {",
        "static void requireLength(String funcName, String argName, int[] array, int required) {",
        "static void checkLength(String funcName, String argName, int actual, int required) {",
        "static int openFillCount(String funcName, int historyLen, int lookback) {",
        "static void requireHistoryLength(String funcName, String argName, int actual, int historyLen) {",
        "static void requireHistory(String funcName, int historyLen) {",
        "static void requireIndexRange(String funcName, int startIdx, int endIdx) {",
        "static void requireArgument(String funcName, String argName, Object argument) {",
    ] {
        assert_eq!(
            method(&core, sig, "Core.java"),
            method(&server, sig, "TaCodegenServe.java"),
            "the two copies of `{sig}` have drifted"
        );
    }
}

/// Rule B6a at the STREAMING opener, in all four backends: a nullable output
/// may be declined there exactly as it may in the batch tier.
///
/// C has always allowed it; the other three rejected the declined output as a
/// capacity fault the moment rule S5 arrived, which is the divergence this pins.
/// Four clauses per backend, because three of them can pass while the feature is
/// broken: the bound must be conditional, the fill's store must be guarded, the
/// handle's cached value must NOT be read back from an array the caller declined
/// — that is the one that faults at run time — and `MA`'s dispatch arm must
/// decline rather than allocate a `historyLen` buffer to throw away.
#[test]
fn test_mama_nullable_fama_is_declinable_at_the_opener_in_every_backend() {
    let (func, enums) = load_indicator("mama");
    let (ma, ma_enums) = load_indicator("ma");
    let registry = make_registry();
    let helpers = HelperRegistry::empty();

    let rust = backends::rust_lang::generate(&func, &enums, &registry, &helpers);
    assert!(
        rust.contains("outMAMA: &mut [f64], outFAMA: Option<&mut [f64]>,")
            && rust.contains("outMAMA: &mut [f64], mut outFAMA: Option<&mut [f64]>, outStride: usize,"),
        "Rust: the opener family takes Option, `mut` on the transcription alone"
    );
    assert!(
        rust.contains("if outFAMA.as_deref().is_some_and(|o| o.len() < _guardOutLen) {"),
        "Rust: S5 bounds a nullable output only where it was supplied"
    );
    assert!(
        rust.contains("Some(&mut sink_outFAMA)"),
        "Rust: the scalar open reports every output, so it never declines one"
    );

    let java = backends::java::generate(&func, &enums, &registry, &helpers);
    assert!(
        java.contains("if( outFAMA != null ) requireLength(\"MAMA openAndFill\", \"outFAMA\", outFAMA, guardOutLen);"),
        "Java: S5 bounds a nullable output only where it was supplied"
    );
    assert!(
        java.contains("lastCur_outFAMA = fama;") && java.contains("sp.cur_outFAMA = lastCur_outFAMA;"),
        "Java: the handle's cached value comes from the store, not from the caller's array"
    );
    assert!(
        !java.contains("sp.cur_outFAMA = outFAMA["),
        "Java: reading a declined output back is the fault this rule creates"
    );

    let csharp = backends::csharp::generate(&func, &enums, &registry, &helpers);
    assert!(
        csharp.contains("if( !outFAMA.IsEmpty ) RequireFillLength(\"MAMA\", \"openAndFill\", \"outFAMA\", outFAMA.Length, guardOutLen);"),
        "C#: S5 bounds a nullable output only where it was supplied"
    );
    assert!(
        csharp.contains("lastCur_outFAMA = fama;") && csharp.contains("sp.cur_outFAMA = lastCur_outFAMA;"),
        "C#: the handle's cached value comes from the store, not from the caller's span"
    );
    assert!(
        !csharp.contains("sp.cur_outFAMA = outFAMA["),
        "C#: reading a declined output back is the fault this rule creates"
    );

    let c = backends::c::generate(&func, &enums, &registry, &helpers);
    let at_open = c
        .find("TA_RetCode TA_MAMA_OpenImpl(")
        .expect("C: the streaming transcription");
    let c_open = &c[at_open..];
    assert!(
        c_open.contains("if( outFAMA != NULL )"),
        "C: the FILL's store into the nullable output stays guarded — the batch \
         body's guard is not evidence about this one"
    );
    assert!(
        c_open.contains("(outFAMA != NULL && (const void *)outMAMA == (const void *)outFAMA)"),
        "C: the opener's distinctness guard treats a declined output as aliasing nothing"
    );

    // Java and C#: the shadow must be the SOURCE of the cached value, not merely
    // present — a break that keeps `lastCur_` around and still reads the array
    // back passes a bare substring test.
    for (lang, src, decl, shadow_store, capture) in [
        ("Java", &java, "double lastCur_outFAMA = 0;", "lastCur_outFAMA = fama;",
         "sp.cur_outFAMA = lastCur_outFAMA;"),
        ("C#", &csharp, "double lastCur_outFAMA = 0;", "lastCur_outFAMA = fama;",
         "sp.cur_outFAMA = lastCur_outFAMA;"),
    ] {
        let at_decl = src.find(decl).unwrap_or_else(|| panic!("{lang}: the shadow is not declared"));
        let at_store = src.find(shadow_store).unwrap_or_else(|| panic!("{lang}: the shadow is never written"));
        let at_capture = src.find(capture).unwrap_or_else(|| panic!("{lang}: the capture does not read the shadow"));
        assert!(
            at_decl < at_store && at_store < at_capture,
            "{lang}: the shadow must be declared, then written by the fill, then captured"
        );
    }

    // MA's streaming arm declines instead of allocating a throwaway.
    for (lang, out, want, gone) in [
        ("Rust", backends::rust_lang::generate(&ma, &ma_enums, &registry, &helpers),
         "outReal, None)", "vec![0.0_f64; inReal.len()][..]"),
        ("Java", backends::java::generate(&ma, &ma_enums, &registry, &helpers),
         "mamaOpenAndFill(inReal, 0.5, 0.05, outReal, null)", "new double[historyLen])"),
        ("C#", backends::csharp::generate(&ma, &ma_enums, &registry, &helpers),
         "MamaOpenAndFill(inReal, 0.5, 0.05, outReal, default)", "new double[historyLen])"),
    ] {
        assert!(out.contains(want), "{lang}: MA's streaming MAMA arm must decline FAMA ({want})");
        assert!(
            !out.contains(gone),
            "{lang}: the throwaway FAMA buffer must be gone from the opener, not merely unread"
        );
    }
}

/// Rule U6a — the same declination at `UpdateAndFill`, in all four backends
/// (issue #270). It is a property of the CALL: nothing on the handle records
/// what the opener was given, so there is no flag to set and none to compare.
///
/// Three clauses per ported backend, because two of them can pass while the
/// feature is broken: the capacity bound must be conditional, the store into the
/// caller's array must be guarded, and the value must still be COMPUTED — which
/// in Java and C# means the step's write to the handle's `cur_` field is
/// untouched, and in Rust means the declined slot is a sink rather than a
/// skipped call. The last clause is the negative one, and the only thing that
/// can see "the handle remembers what the opener was given": the number of ways
/// the fill can reject, which such a comparison would have to add to.
#[test]
fn test_mama_nullable_fama_is_declinable_at_update_and_fill_in_every_backend() {
    let (func, enums) = load_indicator("mama");
    let registry = make_registry();
    let helpers = HelperRegistry::empty();

    fn method<'a>(src: &'a str, sig: &str, what: &str) -> &'a str {
        let at = src.find(sig).unwrap_or_else(|| panic!("{what}: `{sig}` not found"));
        let rest = &src[at..];
        let open = rest.find('{').unwrap_or_else(|| panic!("{what}: `{sig}` has no body"));
        let mut depth = 0usize;
        for (i, ch) in rest[open..].char_indices() {
            match ch {
                '{' => depth += 1,
                '}' => {
                    depth -= 1;
                    if depth == 0 {
                        return &rest[..open + i + 1];
                    }
                }
                _ => {}
            }
        }
        panic!("{what}: `{sig}` has no body")
    }

    let rust = backends::rust_lang::generate(&func, &enums, &registry, &helpers);
    let r = method(
        &rust,
        "pub fn update_and_fill(&mut self,",
        "Rust update_and_fill",
    );
    assert!(
        r.contains("mut outFAMA: Option<&mut [f64]>"),
        "Rust: the fill takes Option, as the opener does"
    );
    assert!(
        r.contains("outFAMA.as_deref().is_some_and(|o| o.len() < barCount)"),
        "Rust: U6 bounds a nullable output only where it was supplied"
    );
    assert!(
        r.contains("let mut sink_outFAMA: f64 = 0.0_f64;")
            && r.contains("let slot_outFAMA = match outFAMA.as_deref_mut() { Some(_s) => &mut _s[i], None => &mut sink_outFAMA };")
            && r.contains(", &mut outMAMA[i], slot_outFAMA);"),
        "Rust: a declined output still gets a slot, so the step still computes it"
    );

    let java = backends::java::generate(&func, &enums, &registry, &helpers);
    let j = method(
        &java,
        "public void updateAndFill( double inReal[], double outMAMA[], double outFAMA[] )",
        "Java updateAndFill",
    );
    assert!(
        j.contains("(outFAMA != null && outFAMA.length < barCount)"),
        "Java: U6 bounds a nullable output only where it was supplied"
    );
    assert!(
        j.contains("if( outFAMA != null ) outFAMA[i] = this.cur_outFAMA;"),
        "Java: the store into a declined output is guarded"
    );
    assert!(
        j.contains("core.mamaStepImpl(this, inReal[i]);") && !j.contains("if( outFAMA != null ) core."),
        "Java: the step runs unconditionally — declining suppresses the write, not the computation"
    );
    // U2 is what makes U6a mean something here: a DECLINED output is accepted, an
    // ABSENT required one is `BadParam` naming it — and the presence test has to
    // precede the length, which is the thing that reads off a null array.
    let at_present = j
        .find("requireArgument(\"MAMA updateAndFill\", \"outMAMA\", outMAMA);")
        .expect("Java: the required output is checked for presence");
    assert!(
        j.contains("requireArgument(\"MAMA updateAndFill\", \"inReal\", inReal);")
            && !j.contains("requireArgument(\"MAMA updateAndFill\", \"outFAMA\""),
        "Java: every required array is checked, and the declinable one is not"
    );
    assert!(
        at_present < j.find("final int barCount").expect("Java: the bar count"),
        "Java: presence precedes the length that would read off a null array"
    );

    let csharp = backends::csharp::generate(&func, &enums, &registry, &helpers);
    let c_sharp = method(
        &csharp,
        "public void UpdateAndFill( ReadOnlySpan<double> inReal, Span<double> outMAMA, Span<double> outFAMA )",
        "C# UpdateAndFill",
    );
    assert!(
        c_sharp.contains("(!outFAMA.IsEmpty && outFAMA.Length < barCount)"),
        "C#: U6 bounds a nullable output only where it was supplied"
    );
    assert!(
        c_sharp.contains("if( !outFAMA.IsEmpty ) outFAMA[i] = cur_outFAMA;"),
        "C#: the store into a declined output is guarded"
    );
    assert!(
        c_sharp.contains("core.MamaStepImpl(this, inReal[i]);")
            && !c_sharp.contains("if( !outFAMA.IsEmpty ) core."),
        "C#: the step runs unconditionally — declining suppresses the write, not the computation"
    );

    let c = backends::c::generate(&func, &enums, &registry, &helpers);
    let c_fill = method(
        &c,
        "TA_RetCode TA_MAMA_UpdateAndFill( TA_MAMA_Stream *stream,",
        "C UpdateAndFill",
    );
    assert!(
        c_fill.contains("if( !stream || !inReal || !outMAMA ) return TA_BAD_PARAM;"),
        "C: a declined output is not an absent argument, and the required one still is"
    );
    assert!(
        c_fill.contains("outFAMA ? &outFAMA[i] : NULL"),
        "C: the declined slot is NULL rather than arithmetic on a NULL pointer"
    );

    // U6a meets U7: a declined output aliases nothing, so every alias term whose
    // operand can be absent guards it first — two declined outputs would
    // otherwise compare equal and reject a legal call. Emitted, never probed at
    // run time (a declining call has no second buffer to alias with).
    for (lang, body, term) in [
        ("Java", j, "(outFAMA != null && (Object)outMAMA == (Object)outFAMA)"),
        ("C", c_fill, "(outFAMA != NULL && (const void *)outMAMA == (const void *)outFAMA)"),
    ] {
        assert!(
            body.contains(term),
            "{lang}: the alias term guards the declinable operand"
        );
    }
    assert!(
        c_sharp.contains("outMAMA.Overlaps(outFAMA)") && !c_sharp.contains("outFAMA.IsEmpty || outMAMA.Overlaps"),
        "C#: `Overlaps` already answers false for an empty span, so the term needs no guard"
    );

    // The declination is a property of the CALL, so there is no third rejection:
    // a fill that compared its output set against the opener's would need one,
    // and counting the exits is what would catch it. The counts are the rules
    // this tier has and no more — the capacity bound and the per-bar finite test
    // everywhere, plus C's absent-argument, negative-count and aliasing guards,
    // which the other three cannot express.
    for (lang, body, exit, want) in [
        ("Rust", r, "return Err(RetCode::BadParam);", 2),
        ("Java", j, "throw new TaLibArgumentException(\"MAMA updateAndFill: BadParam\", RetCode.BadParam);", 2),
        ("C#", c_sharp, "throw Core.StreamFailure(\"MAMA\", \"updateAndFill\", RetCode.BadParam);", 2),
        ("C", c_fill, "return TA_BAD_PARAM;", 4),
    ] {
        assert_eq!(
            body.matches(exit).count(),
            want,
            "{lang}: `UpdateAndFill` rejects on {want} conditions — a third would be \
             the handle remembering what the opener was given, which this design does not do"
        );
    }
}

/// Rule U6a over the arrangement `MAMA` cannot reach: `SYNTH10` declares three
/// outputs with the FIRST and THIRD `nullable` (issue #262's fixture). Two
/// things only it can show — that the guards are per output rather than one
/// blanket branch, and that a nullable output at index 0 does not displace the
/// cursor or the required output's own bound.
///
/// `scripts/synth_gate.py` drives the same fixture end to end, but only
/// nightly; this is the PR gate's view of it.
#[test]
fn test_synth10_two_nullable_outputs_are_declinable_at_update_and_fill() {
    let (func, enums) = load_synth("synth10");
    let registry = make_registry();
    let helpers = HelperRegistry::empty();

    let rust = backends::rust_lang::generate(&func, &enums, &registry, &helpers);
    assert!(
        rust.contains("mut outFirstOptional: Option<&mut [f64]>, outRequired: &mut [f64], mut outSecondOptional: Option<&mut [f64]>) -> Result<(), RetCode>"),
        "Rust: each nullable output takes its own Option, the required one stays a slice"
    );
    for name in ["outFirstOptional", "outSecondOptional"] {
        assert!(
            rust.contains(&format!("let mut sink_{name}: f64 = 0.0_f64;")),
            "Rust: {name} gets its own sink"
        );
    }
    assert!(
        rust.contains("synth10_step_impl(&mut self.state, inReal[i], slot_outFirstOptional, &mut outRequired[i], slot_outSecondOptional);"),
        "Rust: the step takes a slot per declinable output and the array for the required one"
    );

    let java = backends::java::generate(&func, &enums, &registry, &helpers);
    let csharp = backends::csharp::generate(&func, &enums, &registry, &helpers);
    let c = backends::c::generate(&func, &enums, &registry, &helpers);
    for (lang, src, guarded, plain) in [
        (
            "Java",
            &java,
            vec![
                "if( outFirstOptional != null ) outFirstOptional[i] = this.cur_outFirstOptional;",
                "if( outSecondOptional != null ) outSecondOptional[i] = this.cur_outSecondOptional;",
            ],
            "outRequired[i] = this.cur_outRequired;",
        ),
        (
            "C#",
            &csharp,
            vec![
                "if( !outFirstOptional.IsEmpty ) outFirstOptional[i] = cur_outFirstOptional;",
                "if( !outSecondOptional.IsEmpty ) outSecondOptional[i] = cur_outSecondOptional;",
            ],
            "outRequired[i] = cur_outRequired;",
        ),
        (
            "C",
            &c,
            vec![
                "outFirstOptional ? &outFirstOptional[i] : NULL",
                "outSecondOptional ? &outSecondOptional[i] : NULL",
            ],
            "&outRequired[i]",
        ),
    ] {
        for g in &guarded {
            assert!(src.contains(g), "{lang}: `{g}` — each declinable output is guarded on its own");
        }
        assert!(src.contains(plain), "{lang}: the required output is written unconditionally");
    }

    // The required output's bound is NOT made conditional by its declinable
    // neighbours, and the required output is still an absent-argument fault in C.
    assert!(
        java.contains("|| outRequired.length < barCount ||"),
        "Java: the required output keeps an unconditional bound"
    );
    assert!(
        csharp.contains("|| outRequired.Length < barCount ||"),
        "C#: the required output keeps an unconditional bound"
    );
    assert!(
        rust.contains("|| outRequired.len() < barCount ||"),
        "Rust: the required output keeps an unconditional bound"
    );
    assert!(
        c.contains("if( !stream || !inReal || !outRequired ) return TA_BAD_PARAM;"),
        "C: the required output is the only one an absent-argument guard names"
    );
}

/// FAMA is a nullable output (issue #125). In the BATCH C: `Output::is_nullable`
/// is set from the `nullable` flag, the guarded function skips its NULL-check but
/// keeps outMAMA's, the distinctness check guards the nullable operand, and every
/// body write is NULL-guarded while the `outIdx` advance rides the non-nullable
/// outMAMA. MA's batch arm collapses to a clean NULL delegation (no malloc).
#[test]
fn test_c_mama_nullable_fama_batch() {
    let (func, enums) = load_indicator("mama");
    let fama = func.outputs.iter().find(|o| o.name == "outFAMA").unwrap();
    let mama_out = func.outputs.iter().find(|o| o.name == "outMAMA").unwrap();
    assert!(fama.is_nullable(), "outFAMA carries the nullable flag");
    assert!(!mama_out.is_nullable(), "outMAMA is not nullable");

    let registry = make_registry();
    let helpers = HelperRegistry::empty();
    let c = backends::c::generate(&func, &enums, &registry, &helpers);
    // Guarded validation: outMAMA required, outFAMA optional.
    assert!(c.contains("if( !outMAMA )"), "outMAMA still NULL-checked");
    assert!(!c.contains("if( !outFAMA )"), "outFAMA NULL-check skipped (nullable)");
    assert!(
        c.contains("if( outFAMA != NULL && outMAMA == outFAMA )"),
        "distinctness guards the nullable operand (a NULL FAMA aliases nothing)"
    );
    // Body: FAMA store NULL-guarded; outIdx advance on the non-nullable outMAMA.
    assert!(
        c.contains("if( outFAMA != NULL )") && c.contains("outFAMA[outIdx] = fama;"),
        "FAMA store NULL-guarded (no side effect inside the guard)"
    );
    assert!(c.contains("outMAMA[outIdx++] = mama;"), "outIdx advance rides outMAMA");

    // MA's batch arm: clean NULL delegation, no unchecked discard buffer.
    let (ma, ma_enums) = load_indicator("ma");
    let mac = backends::c::generate(&ma, &ma_enums, &registry, &helpers);
    assert!(
        mac.contains(
            "TA_MAMA(startIdx,endIdx,inReal,0.5,0.05,outBegIdx,outNBElement,outReal,NULL)"
        ),
        "MA batch MAMA arm passes NULL for FAMA"
    );
    assert!(
        !mac.contains("dummyBuffer") && !mac.contains("malloc"),
        "the pre-#125 discard malloc is gone"
    );
}

/// Pin where a dual-mode function's identity path is emitted. HMA is the only
/// dual-mode function carrying one, and its mode predicate (`period == 2 ||
/// period == 3`) EXCLUDES the identity value, so an arm-local copy of the
/// `period == 1` guard is unreachable — the defect this pins against. The guard
/// belongs above the predicate, once per step, the way Open already emits it.
///
/// Values cannot see this: an unreachable branch changes no output, so
/// ta_regtest, the bitwise stream/OpenAndFill gates, clippy and the C build are
/// all silent on a regression here. Only a render pin catches it.
#[test]
fn test_dual_mode_identity_guard_is_hoisted_above_the_predicate() {
    let (mut func, enums) = load_indicator("hma");
    func.streaming = true;
    let registry = make_registry();
    let helpers = HelperRegistry::empty();
    let c = backends::c::generate(&func, &enums, &registry, &helpers);

    let step = c
        .split("static void TA_HMA_StepImpl(")
        .nth(1)
        .and_then(|s| s.split("\n}\n").next())
        .expect("step body");
    assert_eq!(
        step.matches("sp->optInTimePeriod == 1").count(),
        1,
        "exactly one identity guard per step, not one per mode arm:\n{step}"
    );
    let guard = step.find("sp->optInTimePeriod == 1").expect("identity guard");
    let pred = step
        .find("sp->optInTimePeriod == 2 || sp->optInTimePeriod == 3")
        .expect("mode predicate");
    assert!(
        guard < pred,
        "the identity guard must precede the mode predicate, not sit inside an arm:\n{step}"
    );
}

/// Pin the generated MINUS_DM dual-mode stream section: ONE union state struct,
/// ONE StepImpl that branches on the stored (immutable) period param — no
/// separate mode tag — and an OpenInternal that selects the degenerate vs the
/// Wilder arm by the same predicate. The input `.c` is untouched: both arms are
/// transcribed verbatim, so the period<=1 raw-DM1 behavior (which ignores the
/// unstable period) is preserved by construction, not re-derived.
#[test]
fn test_c_minus_dm_dual_mode_stream_section() {
    let (mut func, enums) = load_indicator("minus_dm");
    func.streaming = true;
    let registry = make_registry();
    let helpers = HelperRegistry::empty();
    let c = backends::c::generate(&func, &enums, &registry, &helpers);

    // Exactly one StepImpl (not one per mode), branching on the stored param.
    assert_eq!(c.matches("TA_MINUS_DM_StepImpl( struct").count(), 1, "one StepImpl def");
    assert!(
        c.contains("if( sp->optInTimePeriod <= 1 )"),
        "step selects the degenerate arm from the immutable stored param"
    );
    // Wilder smoothing lives in mode B only; the degenerate arm writes raw DM1.
    assert!(
        c.contains("sp->prevMinusDM = sp->prevMinusDM - sp->prevMinusDM / sp->optInTimePeriod"),
        "Wilder recurrence in mode B"
    );
    // OpenInternal selects the arm on the bare predicate (param is a local there).
    assert!(c.contains("if( optInTimePeriod <= 1 )"), "open selects mode by bare predicate");
    // The union struct carries mode B's accumulator (mode A never touches it).
    let struct_sec = c
        .split("struct TA_MINUS_DM_Stream {")
        .nth(1)
        .and_then(|s| s.split("};").next())
        .expect("state struct");
    assert!(struct_sec.contains("double prevMinusDM;"), "union carries prevMinusDM");
}

/// Pin the generated HT_DCPERIOD stream section (M7c): the Hilbert-transform
/// family streams via two general steady-loop normalizations —
///   (1) CARRIED PARITY: the `today % 2` quadrature branch reads an int
///       `streamParity` field, seeded `historyLen % 2` in Open and flipped
///       `1 - streamParity` each step; and
///   (2) OUTPUT-GATE STRIP: the `if (today >= startIdx)` output gate is promoted
///       to an UNCONDITIONAL write in the step (Open's batch replay still
///       suppresses warm-up).
/// This render pin also neuter-checks build_transition: dropping either
/// recognizer makes `backends::c::generate` PANIC (the `today` cursor leaks into
/// the transition), so a clean render proves both fired.
#[test]
fn test_c_ht_dcperiod_parity_stream_section() {
    let (mut func, enums) = load_indicator("ht_dcperiod");
    func.streaming = true;
    let registry = make_registry();
    let helpers = HelperRegistry::empty();
    let c = backends::c::generate(&func, &enums, &registry, &helpers);
    let stream = &c[c.find("/**** Streaming API *****/").expect("stream section")..];

    // (2) carried parity: int field, seeded in Open, flipped in the step.
    assert!(stream.contains("int streamParity;"), "streamParity int state field");
    assert!(
        stream.contains("sp->streamParity = historyLen % 2;"),
        "parity seeded to the next bar's parity in Open"
    );
    assert!(
        stream.contains("if( sp->streamParity == 0 )"),
        "the step branches on the carried parity, not `today % 2`"
    );
    assert!(
        stream.contains("sp->streamParity = 1 - sp->streamParity;"),
        "parity flips each step"
    );
    // (1) output-gate strip: the step writes outReal UNCONDITIONALLY (no
    // `today >= startIdx` gate survives in the per-bar transition).
    let step = stream
        .split("TA_HT_DCPERIOD_StepImpl")
        .nth(1)
        .expect("StepImpl emitted");
    let step_body = &step[..step.find("TA_HT_DCPERIOD_OpenImpl").unwrap_or(step.len())];
    assert!(
        step_body.contains("*outReal= sp->smoothPeriod;"),
        "unconditional smoothPeriod output in the step"
    );
    // No absolute-index leak: `startIdx` (the gate RHS) and the raw `% 2` parity
    // test are both gone — the gate was stripped and `today % 2` was carried.
    // (A `todayValue` temp legitimately survives; that is the bar input, not the
    // cursor.)
    assert!(
        !step_body.contains("startIdx") && !step_body.contains("% 2"),
        "no gate (`startIdx`) or raw parity (`% 2`) leaks into the step"
    );
    // WMA price smoother rides as a trailing ring; the 8 Hilbert double[3]
    // buffers ride as fixed-array carried state (memcpy capture).
    assert!(stream.contains("double *ring_trailingWMAIdx_inReal;"), "WMA trailing ring");
    assert!(stream.contains("double detrender_Even[3];"), "fixed Hilbert array state");
    assert!(
        stream.contains("memcpy( sp->detrender_Even, detrender_Even, sizeof( sp->detrender_Even ) );"),
        "fixed arrays captured by memcpy in Open"
    );
}

/// Pin the generated HT_PHASOR stream section: the SECOND consumer of the two
/// general normalizations, and the one that stresses their nesting. Unlike
/// HT_DCPERIOD, HT_PHASOR writes its TWO outputs under an output gate NESTED
/// INSIDE each odd/even parity arm. This pins that (a) the gate strip reaches
/// nested gates (both outputs land UNCONDITIONALLY inside `if(streamParity==0)`
/// / else), (b) the carried-parity machinery is reused verbatim, and (c) both
/// outputs are written per bar in the arm that runs.
#[test]
fn test_c_ht_phasor_nested_gate_two_outputs_stream_section() {
    let (mut func, enums) = load_indicator("ht_phasor");
    func.streaming = true;
    let registry = make_registry();
    let helpers = HelperRegistry::empty();
    let c = backends::c::generate(&func, &enums, &registry, &helpers);
    let stream = &c[c.find("/**** Streaming API *****/").expect("stream section")..];

    // Reused carried-parity machinery (same as HT_DCPERIOD).
    assert!(stream.contains("int streamParity;"), "streamParity int state field");
    assert!(stream.contains("sp->streamParity = historyLen % 2;"), "parity seeded in Open");
    assert!(stream.contains("sp->streamParity = 1 - sp->streamParity;"), "parity flips each step");

    let step = stream
        .split("TA_HT_PHASOR_StepImpl")
        .nth(1)
        .expect("StepImpl emitted");
    let step_body = &step[..step.find("TA_HT_PHASOR_OpenImpl").unwrap_or(step.len())];
    // The step branches on the carried parity, and BOTH outputs are written
    // unconditionally in each arm (the nested `today >= startIdx` gate stripped).
    assert!(step_body.contains("if( sp->streamParity == 0 )"), "parity branch in the step");
    assert_eq!(
        step_body.matches("*outQuadrature= Q1;").count(),
        2,
        "outQuadrature written unconditionally in BOTH parity arms (nested gate stripped)"
    );
    assert!(
        step_body.contains("*outInPhase= sp->I1ForEvenPrev3;")
            && step_body.contains("*outInPhase= sp->I1ForOddPrev3;"),
        "outInPhase written per-arm with the arm's own carried I1"
    );
    assert!(
        !step_body.contains("startIdx") && !step_body.contains("% 2"),
        "no gate (`startIdx`) or raw parity (`% 2`) leaks into the step"
    );
}

/// Small helper: the streaming section of a generated HT function.
fn ht_stream_section(name: &str) -> String {
    let (mut func, enums) = load_indicator(name);
    func.streaming = true;
    let registry = make_registry();
    let helpers = HelperRegistry::empty();
    let c = backends::c::generate(&func, &enums, &registry, &helpers);
    let start = c.find("/**** Streaming API *****/").expect("stream section");
    c[start..].to_string()
}

/// Pin HT_DCPHASE: the first coexistence of a smoothPrice CIRCBUF, a WMA trailing
/// ring, and the eight fixed Hilbert arrays in ONE handle. The DCPhase backward
/// rescan reads the circbuf; DCPhase is carried across bars.
#[test]
fn test_c_ht_dcphase_circ_ring_fixed_coexist() {
    let s = ht_stream_section("ht_dcphase");
    assert!(s.contains("double *cb_smoothPrice;"), "smoothPrice circbuf");
    assert!(s.contains("double *ring_trailingWMAIdx_inReal;"), "WMA trailing ring");
    assert!(s.contains("double detrender_Even[3];"), "fixed Hilbert array");
    assert!(s.contains("double DCPhase;"), "DCPhase carried across bars");
    assert!(s.contains("sp->cb_smoothPrice[sp->smoothPrice_Idx] = smoothedValue;"), "circbuf write");
    assert!(s.contains("sp->cb_smoothPrice[idx]"), "circbuf backward rescan read");
    assert!(s.contains("memcpy( sp->cb_smoothPrice, smoothPrice"), "circbuf captured (contents+phase) in Open");
    assert!(s.contains("*outReal= sp->DCPhase;"), "unconditional DCPhase output (gate stripped)");
}

/// Pin HT_SINE: DCPHASE's circbuf/ring body with TWO sin() outputs.
#[test]
fn test_c_ht_sine_two_sin_outputs() {
    let s = ht_stream_section("ht_sine");
    assert!(s.contains("double *cb_smoothPrice;"), "shares DCPHASE's circbuf");
    let step = s.split("TA_HT_SINE_StepImpl").nth(1).unwrap();
    let step = &step[..step.find("TA_HT_SINE_OpenImpl").unwrap_or(step.len())];
    assert!(step.contains("*outSine="), "outSine written unconditionally");
    assert!(step.contains("*outLeadSine="), "outLeadSine written unconditionally");
    assert!(!step.contains("startIdx") && !step.contains("% 2"), "no cursor leak in the step");
}

/// Pin the ARITHMETIC of the #229 folded rescan-window read, not just the
/// election that produced it.
///
/// The election has unit tests in `streaming.rs`: which ring is chosen, which
/// shapes refuse, which slot the read is routed to. None of them sees the
/// emitted offset -- an adversarial review shifted it by one bar at the
/// generator and every one of those tests stayed green, every backend still
/// compiled, `regen-check` still passed, and `stream_verify` noticed on only
/// 3 of the 14 folded functions. So the expression itself is pinned here as
/// text, the way the window read it replaces already was (see
/// `test_c_ht_trendline_raw_price_window`).
///
/// Two things it must say, and the reason each is load-bearing:
///   * NO `ringLag_` term. A trailing read subtracts the runtime lag; this is
///     cursor-relative, and slot `pos` already holds the current bar.
///   * the conditional subtract, not `%`. `pos + cap - w` is in `[1, 2*cap)`
///     because `w <= back < cap`, so one compare is the exact modulo -- and a
///     `%` here would also hide an out-of-range offset that the compare form
///     turns into a read at exactly `cap`.
#[test]
fn test_c_folded_window_read_is_cursor_relative_and_de_moduloed() {
    let s = ht_stream_section("cdl3blackcrows");
    assert!(
        !s.contains("win_totIdx_"),
        "the rescan window keeps no buffer (#229)"
    );
    let step = s.split("TA_CDL3BLACKCROWS_StepImpl").nth(1).unwrap();
    let step = &step[..step.find("TA_CDL3BLACKCROWS_OpenImpl").unwrap_or(step.len())];
    let ring = "ring_ShadowVeryShortTrailingIdx_derived";
    let pos = "sp->ringPos_ShadowVeryShortTrailingIdx";
    let cap = "sp->ringCap_ShadowVeryShortTrailingIdx";
    let want = format!(
        "sp->{ring}[({pos} + {cap} - totIdx >= {cap}) ? \
         {pos} + {cap} - totIdx - {cap} : {pos} + {cap} - totIdx]"
    );
    assert!(
        step.contains(&want),
        "cursor-relative folded read, de-moduloed and with no lag term.\nwant: {want}\nstep: {step}"
    );
    // The trailing read in the SAME statement keeps its lag term and its `%`:
    // the two are different reads of one buffer and must not be conflated.
    assert!(
        step.contains(&format!(
            "sp->{ring}[({pos} + {cap} - sp->ringLag_ShadowVeryShortTrailingIdx - totIdx) % {cap}]"
        )),
        "the trailing read is unchanged"
    );
    // And the value the ring stores is the shape the window used to recompute.
    assert!(
        step.contains(&format!(
            "sp->{ring}[{pos}] = TA_STREAM_CANDLERANGE(ShadowVeryShort,inOpen,inHigh,inLow,inClose);"
        )),
        "one push per bar, from the same expression the fold matched"
    );
}

/// The C server's #240 state-equivalence comparators are generated by reading
/// `c_stream::state_struct_text` back and emitting one compare per declared
/// field. That is only drift-proof if the text is the very text the shipped
/// struct is emitted from — so pin the identity here, over the whole streaming
/// corpus.
///
/// Without this, a tier that grew a state field through some path
/// `state_struct_text` does not take would leave that field out of the compare
/// silently, and the leg would keep reporting `state_ok:1` while no longer
/// looking at the new state. The comparator's own generation already panics on
/// a pointer field it has no rule for; this covers the other direction, where
/// the field never reaches it at all.
#[test]
fn test_c_state_struct_text_is_the_emitted_struct() {
    let registry = make_registry();
    let helpers =
        HelperRegistry::from_dir(&Path::new(env!("CARGO_MANIFEST_DIR")).join("../../ta_codegen/input"));
    let mut checked = 0usize;
    for name in discover_indicators() {
        let (func, enums) = load_indicator(&name);
        if !func.streaming {
            continue;
        }
        let c = backends::c::generate(&func, &enums, &registry, &helpers);
        let text = backends::c_stream::state_struct_text(&func, &registry);
        assert!(
            text.contains(&format!("struct TA_{}_Stream {{", name.to_uppercase())),
            "{name}: state_struct_text produced no struct"
        );
        assert!(
            c.contains(&text),
            "{name}: the state struct the #240 comparators are built from is not \
             the one emitted into the shipped .c\n--- want ---\n{text}"
        );
        checked += 1;
    }
    assert!(checked >= 200, "expected the streaming corpus, saw {checked}");
}

/// The layout `TA_StreamOutRange` reads through (#241). One public accessor
/// serves every stream only because the range sits at a fixed offset in EVERY
/// `TA_<N>_Stream`, so this pins the emitted text: the two declarations, first,
/// in that order, in every tier's struct. Nothing else can see it — the accessor
/// takes a `const void *`, so a struct that leads with something else compiles
/// and returns whatever those four bytes happened to be.
#[test]
fn c_stream_every_tier_leads_with_the_range_head() {
    let head = backends::c_stream::RANGE_HEAD_FIELDS;
    let mut checked = 0usize;
    let mut tiers: std::collections::BTreeSet<String> = std::collections::BTreeSet::new();
    for name in discover_indicators() {
        let (func, _enums) = load_indicator(&name);
        if !func.streaming {
            continue;
        }
        let registry = make_registry();
        let text = backends::c_stream::state_struct_text(&func, &registry);
        let open = format!("struct TA_{}_Stream {{", name.to_uppercase());
        let body = text.split(&open).nth(1).unwrap_or_else(|| panic!("{name}: no struct"));
        // The declarations, in order, ignoring comment and blank lines.
        let decls: Vec<&str> = body
            .lines()
            .map(str::trim)
            .filter(|l| !l.is_empty() && !l.starts_with("/*") && !l.starts_with('*'))
            .collect();
        assert!(
            decls.len() >= 2 && decls[0] == head[0] && decls[1] == head[1],
            "{name}: a stream struct must lead with the range head {head:?}, saw {:?}",
            &decls[..decls.len().min(3)]
        );
        // Once each, so a tier cannot carry a second copy further down.
        for d in head {
            assert_eq!(
                body.matches(d).count(),
                1,
                "{name}: `{d}` appears more than once in the stream struct"
            );
        }
        let resolved = func.resolved_for(ir::Lang::C);
        let plan = streaming::validate_streamable(&resolved, &registry).expect("streamable");
        tiers.insert(format!("{:?}", std::mem::discriminant(&plan)));
        checked += 1;
    }
    assert!(checked >= 200, "expected the streaming corpus, saw {checked}");
    assert_eq!(tiers.len(), 5, "all five stream tiers must be covered, saw {}", tiers.len());
}

/// The per-bar transition tier is spelled `_StepImpl` in all four backends
/// (#250) — `_Impl` for the numerics, leaving `_Internal` to mean a variant of
/// an entry point. It was three different words before: `_StepInternal` in C,
/// `_step_internal` in Rust, `_StreamStep` in Java and C#.
///
/// A name pin rather than a behavioural one, because a name is all this is: the
/// tier is private in every backend, so no runtime gate can see the spelling —
/// measured, renaming the C# emitter's method left the whole generator suite
/// green, and only the C# compiler would have objected.
///
/// Both directions per backend, and on every call site the backend has, so a
/// half-applied rename (a definition the callers no longer name, or a Peek left
/// on the old word) fails rather than passing on the half that moved. C names
/// two call sites (`Update` and `UpdateAndFill`); Rust and Java name one each,
/// because their `peek` runs a frame inline rather than the step on a copy.
#[test]
fn the_transition_tier_is_step_impl_in_every_backend() {
    // SMA's own `stream` flag, not one forced on here: a test that sets the flag
    // itself still renders a stream section after the flag is dropped from the
    // YAML, and would keep asserting names on output nothing ships.
    let (func, enums) = load_indicator("sma");
    assert!(func.streaming, "sma must carry the `stream` flag");
    let registry = make_registry();
    let helpers = HelperRegistry::empty();

    let c = backends::c::generate(&func, &enums, &registry, &helpers);
    let rust = backends::rust_lang::generate(&func, &enums, &registry, &helpers);
    let java = backends::java::generate(&func, &enums, &registry, &helpers);
    let csharp = backends::csharp::generate(&func, &enums, &registry, &helpers);

    // (backend, source, definition, call sites, the retired word)
    let cases: [(&str, &str, &str, &[&str], &str); 4] = [
        (
            "c",
            &c,
            "static void TA_SMA_StepImpl( struct TA_SMA_Stream *sp,",
            &["TA_SMA_StepImpl( stream,", "TA_SMA_StepImpl( stream, inReal[i]"],
            "StepInternal",
        ),
        (
            "rust",
            &rust,
            // No `&self`: SMA's step reads nothing from `Core`, and a step that
            // does reads only the `cs_<setting>` parameters its handle carries
            // (issue #274).
            "fn sma_step_impl(sp: &mut SmaStreamState,",
            &["Core::sma_step_impl(&mut self.state,"],
            "step_internal",
        ),
        (
            "java",
            &java,
            "void smaStepImpl( SmaStream sp,",
            &["core.smaStepImpl(this,"],
            "StreamStep",
        ),
        (
            "csharp",
            &csharp,
            "internal void SmaStepImpl( SmaStream sp,",
            // Only `Update`'s. `Peek` runs a frame inline and calls no step.
            &["core.SmaStepImpl(this,"],
            "StreamStep",
        ),
    ];

    for (lang, src, def, calls, retired) in cases {
        assert_eq!(
            src.matches(def).count(),
            1,
            "{lang}: exactly one transition definition named `{def}`"
        );
        for call in calls {
            assert!(src.contains(call), "{lang}: a call site must name `{call}`");
        }
        // Paired with the positives above: this is the word the rename retired,
        // so it discriminates only while the positives hold.
        assert!(!src.contains(retired), "{lang}: `{retired}` is the retired spelling");
    }
}

/// The clamp and its history re-check are ONE edit, in all four backends.
///
/// The identity arms resolve their own anchor — the lookback, moved up to
/// `startIdx` — and then have to test the history against the CLAMPED value.
/// Clamping and then testing the PRE-clamp anchor lets an anchor the history
/// does not reach through, and the count published is `historyLen - anchor`:
/// negative in C, Java and C#, a `usize` underflow in Rust. That is the defect
/// this branch shipped and 96d1052f8 fixed.
///
/// Pinned in the generator rather than behaviourally, because outside Rust the
/// guard is not reachable from the public API: the public openers pass
/// `startIdx = 0`, where the clamp is a no-op, and the only caller that anchors
/// is the `_OpenInternal` seam — contracted on `startIdx <= endIdx`, whose
/// transcribed bodies index before they check. Driving it out of contract is
/// undefined, not a rejection; measured, `TA_AD_OpenInternal(45, 40)` segfaults
/// under ASan. Rust's case IS publicly reachable (its MAVP has no own-lookback
/// precheck) and is covered behaviourally by
/// `an_anchor_past_the_history_is_insufficient_history`.
#[test]
fn identity_anchor_clamps_before_it_rechecks_in_every_backend() {
    let (func, enums) = load_indicator("ma");
    let registry = make_registry();
    let helpers =
        HelperRegistry::from_dir(&Path::new(env!("CARGO_MANIFEST_DIR")).join("../../ta_codegen/input"));

    // (backend, emitted text, clamp needle, re-check needle)
    let c = backends::c::generate(&func, &enums, &registry, &helpers);
    let rust = backends::rust_lang::generate(&func, &enums, &registry, &helpers);
    let java = backends::java::generate(&func, &enums, &registry, &helpers);
    let csharp = backends::csharp::generate(&func, &enums, &registry, &helpers);
    let cases: [(&str, &str, &str, &str); 4] = [
        ("c", &c, "if( startIdx > fillLb ) fillLb = startIdx;", "if( historyLen < fillLb + 1 )"),
        (
            "rust",
            &rust,
            "let fillLb = if startIdx > fillLb { startIdx } else { fillLb };",
            "if historyLen < fillLb + 1 {",
        ),
        ("java", &java, "if( startIdx > fillLb ) fillLb = startIdx;", "if( historyLen < fillLb + 1 )"),
        ("csharp", &csharp, "if( startIdx > fillLb ) fillLb = startIdx;", "if( historyLen < fillLb + 1 )"),
    ];

    let mut checked = 0usize;
    for (lang, src, clamp, recheck) in cases {
        // Every clamp in the file must be followed by its re-check before the
        // next clamp — walking them pairwise rather than taking the first of
        // each, because the same two lines are emitted by more than one arm and
        // a first-occurrence check is satisfied by whichever arm is still right.
        let mut from = 0usize;
        let mut seen = 0usize;
        while let Some(i) = src[from..].find(clamp) {
            let at = from + i;
            let rest = &src[at + clamp.len()..];
            let next_clamp = rest.find(clamp).unwrap_or(rest.len());
            let next_recheck = rest
                .find(recheck)
                .unwrap_or_else(|| panic!("{lang}: a clamp at byte {at} has no re-check after it"));
            assert!(
                next_recheck < next_clamp,
                "{lang}: the clamp at byte {at} is not followed by its history re-check before \
                 the next clamp — clamping and then testing the PRE-clamp anchor is the defect"
            );
            seen += 1;
            from = at + clamp.len();
        }
        assert!(seen >= 1, "{lang}: MA emits no startIdx clamp at all");
        checked += 1;
    }
    assert_eq!(checked, 4, "all four backends must be covered");
}

/// The #241 range leg's per-site ratchet, pinned across all four servers at once.
///
/// Each server declares the SET of range-compare sites it has as
/// `range_sites_all`; the driver ORs what actually fired across the run and
/// requires exactly that set. Two drifts fail OPEN and are what this catches:
///
///   * a site emitted but left out of the declared set — the fired mask then
///     carries a bit the ratchet never demands, so the new site can die
///     unnoticed;
///   * a site that reuses an existing bit — its death is masked by the other.
///
/// Neither is visible at run time: both leave a mask that satisfies the driver.
/// So the check is on the emitted text — the set of bits a server actually ORs
/// in must be exactly the set it declares.
///
/// The sets differ, which is why the declaration is a mask and not a count (see
/// `SvRangeSite`): C, Java and C# reach the anchored `_OpenInternal` seam and
/// Rust's server, a separate crate, cannot. All four can fork a live stream
/// since C gained `TA_<N>_Clone` (#287), so Rust is the one server whose set is
/// a strict subset — and a count still could not say WHICH four it has.
#[test]
fn sv_range_sites_mask_matches_the_declared_set() {
    let base = Path::new(env!("CARGO_MANIFEST_DIR")).join("../../ta_codegen/input");
    let enums = parser::enums::load_enums(&base.join("enums.yaml"));
    let funcs: Vec<ir::FuncDef> = discover_indicators().iter().map(|n| load_indicator(n).0).collect();

    // Fill = 1, Prefix = 2, UpdateFill = 4, Anchored = 8, Copy = 16.
    let servers = [
        ("c", ta_codegen_lib::server_gen::generate_c_server(&funcs, &enums), 1 | 2 | 4 | 8 | 16u32),
        ("java", ta_codegen_lib::server_gen::generate_java_server(&funcs, &enums), 1 | 2 | 4 | 8 | 16),
        ("csharp", ta_codegen_lib::server_gen::generate_csharp_server(&funcs, &enums), 1 | 2 | 4 | 8 | 16),
        ("rust", ta_codegen_lib::server_gen::generate_rust_server(&funcs, &enums), 1 | 2 | 4 | 16),
    ];

    for (lang, src, want_all) in servers {
        // What the server tells the driver about itself.
        let decl = format!("\\\"range_sites_all\\\":{want_all}");
        let decl_plain = format!("\"range_sites_all\":{want_all}");
        assert!(
            src.contains(&decl) || src.contains(&decl_plain),
            "{lang}: server does not declare range_sites_all = {want_all}"
        );

        // What it actually ORs in. Collect every `rangeSites |= N` / `range_sites |= N`.
        let mut bits: std::collections::BTreeSet<u32> = std::collections::BTreeSet::new();
        for needle in ["rangeSites |= ", "range_sites |= "] {
            let mut from = 0usize;
            while let Some(i) = src[from..].find(needle) {
                let at = from + i + needle.len();
                let digits: String = src[at..].chars().take_while(char::is_ascii_digit).collect();
                assert!(!digits.is_empty(), "{lang}: unparsable site bit at byte {at}");
                bits.insert(digits.parse().expect("site bit is a number"));
                from = at;
            }
        }
        let want: std::collections::BTreeSet<u32> =
            (0..32u32).map(|b| 1u32 << b).filter(|b| want_all & b != 0).collect();
        assert_eq!(
            bits, want,
            "{lang}: the site bits the server sets do not match the set it declares. \
             A bit outside the set is a site the ratchet never demands; a missing bit is a site \
             whose death nothing would see."
        );
    }
}

/// The #240 state-equivalence leg, pinned where #229 taught us to pin: as the
/// EMITTED text of the comparison, not as the election that produced it.
///
/// Two properties, and both were false at some point while this was written:
///   * one comparator per streaming function — the set closes under a fixpoint
///     over sub-handles, so a callee losing its comparator silently drops every
///     caller's leg with it;
///   * the ring compare is LOGICAL (rotated by each handle's own cursor), not
///     slot-by-slot. Slot-by-slot failed 90 of 175 functions on nothing but the
///     rotation: the plain oldest-slot layout re-bases every open to phase 0
///     while an update just advances the cursor. A future "simplification" back
///     to `ring[k] == ring[k]` would turn the leg permanently red, and the
///     obvious fix for THAT is to delete the leg.
#[test]
fn test_c_server_state_equivalence_leg() {
    let base = Path::new(env!("CARGO_MANIFEST_DIR")).join("../../ta_codegen/input");
    let enums = parser::enums::load_enums(&base.join("enums.yaml"));
    let funcs: Vec<ir::FuncDef> = discover_indicators()
        .iter()
        .map(|n| load_indicator(n).0)
        .collect();
    let streaming: Vec<&ir::FuncDef> = funcs.iter().filter(|f| f.streaming).collect();
    assert!(streaming.len() >= 200, "expected the streaming corpus, saw {}", streaming.len());
    let srv = ta_codegen_lib::server_gen::generate_c_server(&funcs, &enums);

    for f in &streaming {
        let n = &f.name;
        assert!(
            srv.contains(&format!("static int sv_steq_TA_{n}( const struct TA_{n}_Stream *a,")),
            "{n}: no state-equivalence comparator — the fixpoint dropped it"
        );
        assert!(
            srv.contains(&format!("if( sv_steq_TA_{n}( st, stEq, &stateWhat, &svZsign ) ) stateOk = 0;")),
            "{n}: comparator emitted but the leg never calls it"
        );
    }

    // CDLPIERCING: two shifted windows of ONE setting over one derived ring —
    // the #229 fold's own shape, and the reason the leg exists.
    let body = srv
        .split("static int sv_steq_TA_CDLPIERCING( const struct TA_CDLPIERCING_Stream *a, const struct TA_CDLPIERCING_Stream *b, const char **w, int *z )\n{")
        .nth(1)
        .expect("CDLPIERCING comparator body");
    let body = &body[..body.find("\n}").expect("comparator end")];
    for want in [
        // the running totals, both windows, bitwise
        "for( k = 0; k < 2; k++ ) if( sv_xtier_ne(a->BodyLongPeriodTotal[k], b->BodyLongPeriodTotal[k], z) )",
        // the ring, rotated by each handle's own cursor
        "ia = (a->ringPos_BodyLongTrailingIdx + k) % a->ringCap_BodyLongTrailingIdx;",
        "ib = (b->ringPos_BodyLongTrailingIdx + k) % b->ringCap_BodyLongTrailingIdx;",
        "if( sv_xtier_ne(a->ring_BodyLongTrailingIdx_derived[ia], b->ring_BodyLongTrailingIdx_derived[ib], z) )",
        // the produced-bar range (#241): a leading scalar, so the comparator
        // picks it up with no rule of its own — and Open(P)+k updates and
        // Open(P+k) must agree on it, which is what makes it worth comparing.
        "if( a->outRangeBegIdx != b->outRangeBegIdx )",
        "if( a->outRangeCount != b->outRangeCount )",
        // the carried bar, and the ring geometry
        "if( sv_xtier_ne(a->lag1_inClose, b->lag1_inClose, z) )",
        "if( a->ringCap_BodyLongTrailingIdx != b->ringCap_BodyLongTrailingIdx )",
        "if( a->ringLag_BodyLongTrailingIdx != b->ringLag_BodyLongTrailingIdx )",
    ] {
        assert!(body.contains(want), "CDLPIERCING comparator missing:\n  {want}\nbody:\n{body}");
    }
    // The cursor itself is the ring's PHASE, absorbed by the rotation above.
    // Comparing it directly is the 90-of-175 mistake.
    assert!(
        !body.contains("if( a->ringPos_BodyLongTrailingIdx != b->ringPos_BodyLongTrailingIdx )"),
        "the ring cursor must not be compared directly — it is the phase the rotation absorbs"
    );
    // The Peek scratch mirror is not state: Peek is the only writer, so it holds
    // malloc leftovers on a handle Peek has not been called on.
    assert!(
        !body.contains("ringMirror_BodyLongTrailingIdx_derived["),
        "the Peek mirror must not be compared"
    );

    // AROON: the extrema automaton reads only the live window, never the
    // power-of-two slack above it (never written, so it is malloc leftovers).
    let ar = srv
        .split("static int sv_steq_TA_AROON( const struct TA_AROON_Stream *a, const struct TA_AROON_Stream *b, const char **w, int *z )\n{")
        .nth(1)
        .expect("AROON comparator body");
    assert!(
        ar.contains("for( k = 0; k < a->xCap; k++ )")
            && ar.contains("ix = (a->trailingIdx - 1 + a->xPhys + k) & a->xMask;"),
        "AROON must compare exactly the xCap live slots, not the xPhys allocation"
    );
}

/// Pin HT_TRENDLINE: a rescan window over the RAW input (the padded-loop source
/// rewrite of `inReal[idx--]`), no circbuf, single output.
#[test]
fn test_c_ht_trendline_raw_price_window() {
    let s = ht_stream_section("ht_trendline");
    assert!(s.contains("double *win_i_inReal;"), "rescan window over raw inReal");
    assert!(!s.contains("cb_smoothPrice"), "no smoothPrice circbuf (removed, issue #88)");
    let step = s.split("TA_HT_TRENDLINE_StepImpl").nth(1).unwrap();
    let step = &step[..step.find("TA_HT_TRENDLINE_OpenImpl").unwrap_or(step.len())];
    assert!(step.contains("sp->win_i_inReal[(sp->winPos_i + sp->winCap_i - i >= sp->winCap_i) ?"), "de-modulo window read of bar today-i");
    assert!(step.contains("if( i < DCPeriodInt )"), "guarded to the first DCPeriodInt bars");
    assert!(step.contains("*outReal= tempReal2;"), "unconditional trendline output");
}

/// Pin HT_TRENDMODE: the full HT union — WMA ring + smoothPrice circbuf + a
/// raw-price rescan window (separate counter j) + an INTEGER output.
#[test]
fn test_c_ht_trendmode_full_union() {
    let s = ht_stream_section("ht_trendmode");
    assert!(s.contains("double *ring_trailingWMAIdx_inReal;"), "WMA ring");
    assert!(s.contains("double *cb_smoothPrice;"), "smoothPrice circbuf");
    assert!(s.contains("double *win_j_inReal;"), "raw-price rescan window (counter j)");
    let step = s.split("TA_HT_TRENDMODE_StepImpl").nth(1).unwrap();
    let step = &step[..step.find("TA_HT_TRENDMODE_OpenImpl").unwrap_or(step.len())];
    assert!(step.contains("*outInteger="), "integer trend-mode output, unconditional");
    assert!(step.contains("sp->cb_smoothPrice[idx]"), "circbuf DC-phase read");
    assert!(step.contains("sp->win_j_inReal[(sp->winPos_j + sp->winCap_j - j >= sp->winCap_j) ?"), "de-modulo window trendline read");
    assert!(!step.contains("startIdx") && !step.contains("% 2"), "no cursor leak in the step");
}

/// Pin MAMA — an ordinary HT function (WMA ring + parity) with two real optional
/// params and two coupled outputs (mama/fama) written in a top-level gate. FAMA
/// is a nullable output (issue #125): its per-bar write is NULL-guarded so a
/// caller (MA's dispatch) can discard it — see `test_c_ma_dispatch_stream_section`.
#[test]
fn test_c_mama_two_outputs_and_params() {
    let s = ht_stream_section("mama");
    assert!(s.contains("double optInFastLimit;") && s.contains("double optInSlowLimit;"), "real params carried in the handle");
    assert!(s.contains("double mama;") && s.contains("double fama;"), "coupled mama/fama carried");
    let step = s.split("TA_MAMA_StepImpl").nth(1).unwrap();
    let step = &step[..step.find("TA_MAMA_OpenImpl").unwrap_or(step.len())];
    assert!(step.contains("if( sp->streamParity == 0 )"), "parity branch");
    // MAMA line always written; FAMA (nullable) write is NULL-guarded so the
    // step never dereferences a NULL FAMA pointer (the gate itself is stripped).
    assert!(step.contains("*outMAMA= mama;"), "MAMA line written unconditionally");
    // The GUARD is the invariant, not the right-hand side's spelling: FAMA is
    // carried as a local since the value accessor reads it back (#287), and a
    // future carry could move it again without weakening anything here.
    assert!(
        step.contains("if( outFAMA != NULL )")
            && (step.contains("*outFAMA= fama;") || step.contains("*outFAMA= sp->fama;")),
        "FAMA is nullable (#125): its write is NULL-guarded"
    );
    // The retained copy must NOT be guarded — a declined FAMA is still computed,
    // and `TA_MAMA_Value` has to report it (that is what the accessor is for).
    assert!(
        step.contains("sp->cur_outFAMA = fama;") || step.contains("sp->cur_outFAMA = sp->fama;"),
        "the value retain reads the body's own variable, never the declinable sink"
    );
    assert!(step.contains("sp->optInFastLimit") && step.contains("sp->optInSlowLimit"), "params drive the adaptive alpha");
    assert!(!step.contains("startIdx") && !step.contains("% 2"), "no cursor leak in the step");
}

/// Pin MAVP — the last function and the campaign's one genuinely-new tier: a
/// moving average whose period varies per bar, streamed as a BANK of sub-MA
/// streams. Open builds `maxPeriod - minPeriod + 1` sub-streams (each via the
/// callee's OpenInternal) with all-freed-so-far OOM; Update advances the whole
/// bank in lockstep and indexes by the clamped period; Peek previews only the
/// selected slot; Close frees the bank.
#[test]
fn test_c_mavp_period_bank() {
    let s = ht_stream_section("mavp");
    // Bank of sub-MA streams + scratch, sized at Open.
    assert!(s.contains("struct TA_MA_Stream **bank;"), "bank of sub-MA handles");
    assert!(s.contains("double *scratch;"), "per-slot lockstep output scratch");
    assert!(s.contains("sp->nBank = optInMaxPeriod - optInMinPeriod + 1;"), "one slot per possible period");
    assert!(s.contains("if( optInMinPeriod > optInMaxPeriod ) return TA_BAD_PARAM;"), "inverted window rejected");
    // Every sub-MA is seeded at the SHARED max-period lookback (matching batch),
    // NOT at its own lookback — else period < maxPeriod diverges. This bug fooled
    // every objective gate (the fuzz period-selector always clamped to maxPeriod);
    // pin the anchor so it can never regress.
    assert!(s.contains("lookbackTotal = TA_MA_Lookback( optInMaxPeriod, optInMAType );"), "shared max-period lookback anchor");
    assert!(s.contains("subStart = startIdx < lookbackTotal ? lookbackTotal : startIdx;"), "clamp start to the shared anchor");
    // Open: bank loop opening each period's sub-stream at subStart, all-freed-so-far on OOM.
    assert!(s.contains("TA_MA_OpenInternal( &sp->bank[k], inReal, subStart, historyLen, optInMinPeriod + k, optInMAType,"), "sub-open per period at the shared anchor, MAType forwarded");
    assert!(s.contains("for( j = 0; j < k; j++ ) TA_MA_Close( sp->bank[j] );"), "frees sub-streams opened so far on failure");
    // Update: lockstep advance + clamp-indexed output.
    let upd = s.split("TA_MAVP_Update").nth(1).unwrap();
    let upd = &upd[..upd.find("TA_MAVP_Peek").unwrap_or(upd.len())];
    assert!(upd.contains("for( k = 0; k < stream->nBank; k++ )") && upd.contains("TA_MA_Update( stream->bank[k], inReal, &stream->scratch[k] );"), "advances the whole bank in lockstep");
    // The clamp compares in the REAL domain and narrows only once inside the
    // window (35a35d4b4): `(int)cpReal` on a value already known to be within
    // [min, max] cannot overflow, where narrowing first and clamping after
    // turned a huge positive period into the minimum.
    assert!(upd.contains("if( !(cpReal >= stream->optInMinPeriod) ) cp = stream->optInMinPeriod;")
            && upd.contains("else if( cpReal > stream->optInMaxPeriod ) cp = stream->optInMaxPeriod;")
            && upd.contains("else cp = (int)cpReal;"),
            "clamps the per-bar period in the real domain, then narrows");
    assert!(upd.contains("*outReal = stream->scratch[cp - stream->optInMinPeriod];"), "outputs the selected slot");
    // Peek: only the selected slot (non-committing).
    let peek = s.split("TA_MAVP_Peek").nth(1).unwrap();
    // Up to the NEXT entry point, which is the n-bar filler (#246) — it drives
    // the bank the way Update does, so slicing all the way to Close would read
    // its body as Peek's.
    let peek = &peek[..peek.find("TA_MAVP_UpdateAndFill").unwrap_or(peek.len())];
    assert!(peek.contains("TA_MA_Peek( stream->bank[cp - stream->optInMinPeriod], inReal, outReal );"), "peeks only the selected slot");
    assert!(!peek.contains("TA_MA_Update"), "peek never advances the bank");
    // Close frees every sub-stream + the arrays.
    assert!(s.contains("if( stream->bank[k] ) TA_MA_Close( stream->bank[k] );"), "close frees each sub-stream");
}

/// Pin the generated TRIMA dual-mode (if/else) stream section: the odd/even arms
/// are genuinely different but share identical rings, so the handle carries ONE
/// ring set + one StepImpl branching on the stored parity; the ring buffers are
/// freed by ReleaseImpl and mirrored in Peek.
#[test]
fn test_c_trima_dual_mode_rings_stream_section() {
    let (mut func, enums) = load_indicator("trima");
    func.streaming = true;
    let registry = make_registry();
    let helpers = HelperRegistry::empty();
    let c = backends::c::generate(&func, &enums, &registry, &helpers);

    // Union struct: shared triangular-sum scalars + the SHARED rings (one set).
    assert!(
        c.contains("double *ring_middleIdx_inReal;") && c.contains("double *ring_trailingIdx_inReal;"),
        "shared middleIdx/trailingIdx rings (one set, both arms)"
    );
    assert!(c.contains("double numerator;"), "shared triangular-sum accumulator");
    assert_eq!(c.matches("TA_TRIMA_StepImpl( struct").count(), 1, "one StepImpl");
    assert!(
        c.contains("if( sp->optInTimePeriod % 2 == 1 )"),
        "step branches on the stored parity"
    );
    assert!(c.contains("TA_TRIMA_ReleaseImpl"), "ReleaseImpl frees the rings");
    // Both modes' rings are shared, so the peek frame carries ONE shadow pair
    // per ring across the two arms rather than a per-arm copy of the buffer.
    assert!(!c.contains("Mirror"), "the peek frame replaced the per-handle ring mirror");
}

/// The body of `TA_<NAME>_StepImpl`, brace-balanced. Ring slots are also
/// written during Open, so the per-bar stores have to be counted here alone.
fn step_impl_body(c: &str) -> String {
    let i = c.find("_StepImpl( struct").expect("a StepImpl definition");
    let j = c[i..].find('{').expect("StepImpl has a body") + i;
    let bytes = c.as_bytes();
    let (mut depth, mut k) = (0usize, j);
    loop {
        match bytes[k] {
            b'{' => depth += 1,
            b'}' => {
                depth -= 1;
                if depth == 0 {
                    break;
                }
            }
            _ => {}
        }
        k += 1;
    }
    c[j..=k].to_string()
}

/// Every `sp->ring_X[sp->ringPos_Y] = Z;` store in a step body, tagged with the
/// id of the enclosing brace block.
///
/// The block tag is what keeps a dual-mode step honest: HMA emits the same
/// store in both arms of `if (period == 2 || period == 3) ... else ...`, which
/// is one store per path, not two per bar. Only repeats within a single
/// straight-line block are dead.
fn ring_slot_stores(step: &str) -> Vec<(usize, String)> {
    let mut out = Vec::new();
    let mut stack = vec![0usize];
    let mut next_id = 0usize;
    for line in step.lines() {
        let l = line.trim();
        if l.starts_with("sp->ring_") && l.contains("[sp->ringPos_") && l.ends_with(';') {
            out.push((*stack.last().expect("block stack"), l.to_string()));
        }
        for ch in l.chars() {
            match ch {
                '{' => {
                    next_id += 1;
                    stack.push(next_id);
                }
                '}' => {
                    stack.pop();
                }
                _ => {}
            }
        }
    }
    out
}

#[test]
fn test_c_back_offset_ring_writes_the_current_bar_once() {
    // CDLONNECK's Equal average runs on the SHIFTED candle, so its ring uses the
    // absolute-mod (back > 0) layout: the transition prologue pre-writes the
    // current bar into slot `pos` so the runtime-lag-0 case reads it through the
    // same formula. Nothing between there and the end of the step moves
    // `ringPos`, so repeating that store before the advance is a dead store.
    //
    // `test_c_no_step_impl_stores_a_ring_slot_twice` sweeps the same
    // invariant over the whole streaming corpus; this one keeps a named witness
    // for the offset-ring layout, which is the shape that made the dead store
    // possible in the first place.
    let (mut func, enums) = load_indicator("cdlonneck");
    func.streaming = true;
    let c = backends::c::generate(&func, &enums, &make_registry(), &HelperRegistry::empty());
    let step = step_impl_body(&c);

    // The ring is found by its POSITION variable, not by a hardcoded name. The
    // first version of this test spelled out `ring_EqualTrailingIdx_inOpen` and
    // friends; #229 then collapsed those four per-OHLC rings into one derived
    // ring holding the computed range, and the needle silently stopped matching
    // anything at all -- the assertion failed with a count of 0, not of 2. A
    // rename must not be able to turn this into a test of nothing, so the store
    // count is read off whatever the generator emits.
    let stores: Vec<&str> = step
        .lines()
        .map(str::trim)
        .filter(|l| l.contains("[sp->ringPos_EqualTrailingIdx] ="))
        .collect();
    assert!(
        !stores.is_empty(),
        "no store into the Equal trailing ring: this test is looking at nothing"
    );
    let mut seen = std::collections::BTreeSet::new();
    for s in &stores {
        let ring = s.split('[').next().unwrap_or(s);
        assert!(
            seen.insert(ring),
            "stored twice per bar, with ringPos unmoved between: {s}"
        );
    }
}

#[test]
fn test_c_no_step_impl_stores_a_ring_slot_twice() {
    // Corpus sweep for the same invariant: one write per ring slot per block.
    let registry = make_registry();
    let helpers =
        HelperRegistry::from_dir(&Path::new(env!("CARGO_MANIFEST_DIR")).join("../../ta_codegen/input"));
    let mut stepped = 0usize;
    let mut with_rings = 0usize;
    for name in discover_indicators() {
        let (func, enums) = load_indicator(&name);
        if !func.streaming {
            continue;
        }
        let c = backends::c::generate(&func, &enums, &registry, &helpers);
        if !c.contains("_StepImpl( struct") {
            continue;
        }
        let stores = ring_slot_stores(&step_impl_body(&c));
        let mut seen = std::collections::BTreeSet::new();
        for (block, s) in &stores {
            assert!(
                seen.insert((*block, s.clone())),
                "{name}: ring slot written twice in one straight-line block: {s}"
            );
        }
        stepped += 1;
        with_rings += usize::from(!stores.is_empty());
    }
    // Both floors matter: the first proves the sweep still walks the streaming
    // corpus, the second that it is actually looking at rings — a skip that
    // silently emptied `stores` would keep the first green on its own.
    assert!(stepped >= 195, "expected the streaming corpus, saw {stepped}");
    assert!(with_rings >= 80, "expected the ring-carrying corpus, saw {with_rings}");
}

/// Pin the generated MIDPRICE stream section: batch runs the block scan and the
/// stream runs `midprice_ALT1`'s T4 extrema automaton — one StepImpl, no
/// mode branch, and no trace of the block scan inside the Open.
///
/// Every check here asserts on a string the generator DOES produce, in both
/// directions — present in the batch tier, absent from the Open. An
/// absence-only assertion starts passing for free the day the generator stops
/// emitting the string it looks for, and says nothing from then on.
#[test]
fn test_c_midprice_stream_uses_the_declared_alternate() {
    let (mut func, enums) = load_indicator("midprice");
    func.streaming = true;
    let registry = make_registry();
    let helpers = HelperRegistry::empty();
    let c = backends::c::generate(&func, &enums, &registry, &helpers);

    assert!(c.contains("struct TA_MIDPRICE_Stream {"), "state struct");
    assert!(
        c.contains("double *x_inHigh;") && c.contains("double *x_inLow;"),
        "T4 extrema rings for high/low"
    );
    assert_eq!(c.matches("TA_MIDPRICE_StepImpl( struct").count(), 1, "one StepImpl");
    assert!(
        c.contains("*outReal= (sp->highest + sp->lowest) / 2.0;"),
        "midprice combine in the extrema step"
    );
    // The generated section names the alternate it was built from.
    assert!(
        c.contains("/* Using midprice_ALT1 for TA_ALT={STREAM,ALL_LANGUAGES} */"),
        "the stream section must name the alternate it resolved to"
    );

    // ...and the marker must be telling the truth. A marker is derived from the
    // resolution, so on its own it would agree with a resolver that picked the
    // wrong body; these check the emitted CODE. The block scan's scratch and
    // block cursor appear in the batch tier and nowhere in the Open.
    let (batch, open) = c
        .split_once("TA_MIDPRICE_OpenImpl")
        .expect("the merged open numerics emitted");
    for marker in ["sufHighest", "preHighest", "blockNext"] {
        assert!(
            batch.contains(marker),
            "batch tier lost the block scan (`{marker}` absent) — the BATCH cell should \
             resolve to the base body"
        );
        assert!(
            !open.contains(marker),
            "`{marker}` reached the Open: the STREAM cell resolved to the block scan, not to \
             midprice_ALT1"
        );
    }
    // The automaton's own state, conversely, must be there.
    assert!(open.contains("highestIdx"), "the alternate's cached-extremum index");
}

/// Pin the generated STOCH composed stream section: producer extrema state +
/// typed sub handles; Open opens each sub-stream on the materialized series
/// BEFORE the batch call that consumes it (in-place smoothing overwrites the
/// raw %K right there — order is the contract); the update frame pipelines
/// through sub-Update and the peek frame through sub-Peek.
#[test]
fn test_c_stoch_composed_stream_section() {
    let (mut func, enums) = load_indicator("stoch");
    func.streaming = true;
    let registry = make_registry();
    let helpers = HelperRegistry::empty();
    let c = backends::c::generate(&func, &enums, &registry, &helpers);
    let stream = &c[c.find("/**** Streaming API *****/").expect("stream section")..];

    // Handle: producer extrema + typed subs (no routing flag: the frames are
    // separate functions, so `update` tests nothing per sub-call).
    assert!(!stream.contains("peekMode"));
    assert!(stream.contains("TA_MA_Stream *sub0;"));
    assert!(stream.contains("TA_MA_Stream *sub1;"));

    // STOCH is the one shipped function that exercises BOTH sides of the
    // issue-#192 fusion rule, which is why this assertion lives here:
    //
    //   sub0's %K smoothing is IN PLACE — `TA_MA( .., tempBuffer, .., tempBuffer )`
    //   — so it must stay UNFUSED. A fused open would write tempBuffer during
    //   the warm-up pass while the sub-MA's own capture epilogue still has to
    //   read its input tail out of it, corrupting the handle.
    //
    //   sub1's %D writes a distinct destination, so it fuses: one pass that
    //   both warms the handle and fills sc_outSlowD, instead of a warm pass
    //   plus a batch call recomputing the same numbers.
    let sub0 = stream.find("subRc = TA_MA_OpenInternal( &sub0, tempBuffer").expect("sub0 open (must stay unfused: in-place)");
    let ma1 = stream.find("retCode = TA_MA(0,outIdx - 1,tempBuffer").expect("in-place smoothing");
    let sub1 = stream.find("subRc = TA_MA_OpenAndFillInternal( &sub1, tempBuffer").expect("sub1 open (must be fused)");
    assert!(sub0 < ma1 && ma1 < sub1, "sub-open ordering");
    // The fused sub1 replaced the %D batch call outright: nothing recomputes it.
    assert!(
        !stream.contains("optInSlowD_MAType,&dummyBegIdx,&dummyNBElement,sc_outSlowD"),
        "%D batch sub-call survived the fusion"
    );
    // Params trail the handle+history in the new Open order (input, optional, output).
    // The unfused sub0 still ends in the initial-output dummy; the fused sub1
    // carries the batch call's own out-meta and destination instead.
    assert!(stream.contains("optInSlowK_Period, optInSlowK_MAType, &subOpenDummy"), "slowK params forwarded to sub0 open");
    assert!(stream.contains("optInSlowD_Period, optInSlowD_MAType, &dummyBegIdx, &dummyNBElement, sc_outSlowD"), "slowD params + fill target forwarded to sub1 open");

    // Out-meta pointers mapped to the dummies in the transcription (the
    // Open signature has no outBegIdx/outNBElement).
    assert!(stream.contains("&dummyBegIdx,&dummyNBElement"));
    assert!(!stream.contains(",outBegIdx,"), "raw out-meta arg leaked");

    // Two bodies: the update frame drives sub-Update, the peek frame sub-Peek.
    assert!(stream.contains("TA_MA_Peek( (const TA_MA_Stream *)sp->sub0, cur_tempBuffer, &cur_tempBuffer );"));
    assert!(stream.contains("TA_MA_Update( sp->sub0, cur_tempBuffer, &cur_tempBuffer );"));
    assert!(stream.contains("TA_MA_Update( sp->sub1, cur_tempBuffer, &cur_outSlowD );"));
    assert!(stream.contains("*outSlowK = cur_tempBuffer;"), "memmove tail-align");
    assert!(stream.contains("*outSlowD = cur_outSlowD;"));

    // Peek runs the peek frame on the scratch copy; Close closes subs then frees.
    assert!(stream.contains("TA_MA_Peek( (const TA_MA_Stream *)sp->sub1,"));
    assert!(stream.contains("TA_MA_Close( stream->sub0 );"));
    assert!(stream.contains("TA_STOCH_ReleaseImpl( stream );"));
}

/// Pin the ADXR composed Open's allocation-failure cleanup. The intermediate
/// `adx` buffer's free is WITHHELD from the transcribed tail (the lag ring
/// seeds from its tail first), so it stays live through the capture epilogue —
/// every allocation-failure return there MUST free it, or an OOM leaks the
/// buffer. The adversarial review caught exactly this leak; this guards the
/// fix (each malloc-failure path frees everything allocated so far — no goto,
/// no fault-injection harness, just correct per-return cleanup).
#[test]
fn test_c_adxr_open_frees_withheld_buffer_on_oom_paths() {
    let (mut func, enums) = load_indicator("adxr");
    func.streaming = true;
    let registry = make_registry();
    let helpers = HelperRegistry::empty();
    let c = backends::c::generate(&func, &enums, &registry, &helpers);
    let open = &c[c.find("TA_RetCode TA_ADXR_Open").expect("ADXR Open")..];
    for guard in [
        "if( dummyNBElement < 1 ) { free( adx );",
        "if( !sp ) { free( adx );",
        "if( !sp->lagRing_adx ) { TA_Free( sp ); free( adx );",
    ] {
        assert!(
            open.contains(guard),
            "capture-epilogue OOM path must free the withheld adx buffer: `{guard}`"
        );
    }
    // Close releases the ring buffers (the other half of leak-freedom).
    let close = &c[c.find("TA_RetCode TA_ADXR_Close").expect("ADXR Close")..];
    assert!(close.contains("TA_Free( stream->lagRing_adx );"));
}

/// A composed Open must emit ONE null-check block per allocated intermediate,
/// not two (issue #169). Every one of these inputs writes its own `if( !x )`
/// after the malloc, and the generator injects one as well — the injected one
/// carries the cascading `free()` of the prior intermediates, so it is the
/// keeper and the transcribed one is dropped. Nothing else in the suite would
/// notice the duplicate coming back: the OOM test below uses `find()`, which
/// matches the first copy either way, so a regression would show up only in a
/// `git diff` of the generated C.
#[test]
fn test_c_composed_open_emits_one_null_check_per_intermediate() {
    for (indicator, buffers) in [
        ("adxr", &["adx"][..]),
        ("apo", &["tempBuffer"]),
        ("bbands", &["tempBuffer1", "tempBuffer2"]),
        ("macdext", &["fastMABuffer", "slowMABuffer"]),
        ("ppo", &["tempBuffer"]),
        ("pvo", &["tempBuffer"]),
        ("stoch", &["tempBuffer"]),
        ("stochf", &["tempBuffer"]),
        ("stochrsi", &["tempRSIBuffer"]),
    ] {
        let (mut func, enums) = load_indicator(indicator);
        func.streaming = true;
        let registry = make_registry();
        let helpers = HelperRegistry::empty();
        let c = backends::c::generate(&func, &enums, &registry, &helpers);
        let upper = indicator.to_uppercase();
        let open_at = c
            .find(&format!("TA_RetCode TA_{upper}_Open"))
            .unwrap_or_else(|| panic!("{upper} composed Open"));
        // One `_OpenImpl` transcribes the region for both entry points, so every
        // buffer is checked exactly once — never twice. (Before the Open family
        // was merged this read 2, one per transcription; the invariant being
        // pinned is unchanged: the source's own check must not be emitted
        // alongside the injected one.)
        let opens = &c[open_at..];
        for buf in buffers {
            let n = opens.matches(&format!("if( !{buf} )")).count();
            assert_eq!(
                n, 1,
                "{upper}: `{buf}` must be null-checked exactly once in the composed \
                 `_OpenImpl`, found {n} — the source's own check is being emitted \
                 alongside the injected one again"
            );
        }
    }
}

/// Pin the BBANDS composed Open's allocation-failure cleanup. The general
/// (non-SMA) path allocates TWO intermediates — `tempBuffer1` for the moving
/// average, then `tempBuffer2` for the standard deviation. If `tempBuffer2`'s
/// malloc fails, `tempBuffer1` must be freed or it leaks: the auto-injected
/// null-check must free every intermediate allocated before it. Same OOM
/// discipline as ADXR (each malloc-failure path frees everything allocated so
/// far — no goto, no fault-injection), caught here at generate time.
#[test]
fn test_c_bbands_open_frees_prior_intermediate_on_oom() {
    let (mut func, enums) = load_indicator("bbands");
    func.streaming = true;
    let registry = make_registry();
    let helpers = HelperRegistry::empty();
    let c = backends::c::generate(&func, &enums, &registry, &helpers);
    let open = &c[c.find("TA_RetCode TA_BBANDS_Open").expect("BBANDS Open")..];

    // tempBuffer2's malloc-failure block frees the prior intermediate tempBuffer1.
    let tb2 = &open[open.find("tempBuffer2 = malloc").expect("tempBuffer2 malloc")..];
    let check = tb2.find("if( !tempBuffer2 )").expect("tempBuffer2 null check");
    let ret = tb2[check..]
        .find("return TA_ALLOC_ERR")
        .expect("tempBuffer2 alloc-err return");
    assert!(
        tb2[check..check + ret].contains("free( tempBuffer1 )"),
        "tempBuffer2 malloc-failure must free the prior intermediate tempBuffer1 (else OOM leaks it)"
    );

    // tempBuffer1's own malloc-failure block must NOT reference the
    // not-yet-allocated tempBuffer2 (nothing prior is live at that point).
    let tb1 = &open[open.find("tempBuffer1 = malloc").expect("tempBuffer1 malloc")..];
    let tb1_check = tb1.find("if( !tempBuffer1 )").expect("tempBuffer1 null check");
    let tb1_ret = tb1[tb1_check..]
        .find("return TA_ALLOC_ERR")
        .expect("tempBuffer1 alloc-err return");
    assert!(
        !tb1[tb1_check..tb1_check + tb1_ret].contains("tempBuffer2"),
        "tempBuffer1 malloc-failure must not touch the not-yet-allocated tempBuffer2"
    );

    // The scratch output arrays clean up progressively (each failure frees the
    // ones already allocated).
    assert!(
        open.contains(
            "if( !sc_outRealLowerBand ) { TA_Free( sc_outRealUpperBand ); \
             TA_Free( sc_outRealMiddleBand );"
        ),
        "scratch output arrays must clean up progressively on OOM"
    );
}

/// #142 regression: period-scaled dividers/sums must compute in floating point,
/// never a bare int32 product. The WMA/HMA triangular divider (n*(n+1)/2)
/// overflows int32 at period 46341; the linear-regression cubic
/// (n*(n-1)*(2n-1)/6) overflows at period 1025. Both silently returned garbage.
/// Widening the operands to double is the fix — pin the generated form across
/// C/Rust/Java so a revert to the int expression trips here instead of at a
/// period no test data reaches.
#[test]
fn test_period_scaled_arithmetic_is_double_not_int32() {
    // WMA/HMA triangular divider: double, no int32 `>> 1` shift.
    for name in ["wma", "hma"] {
        let (func, enums) = load_indicator(name);
        let out = generate_all(&func, &enums);
        assert!(
            !out.c.contains(">> 1"),
            "{name}: C divider still uses the int32 `>> 1` shift (#142 overflow at period 46341)"
        );
        assert!(
            out.c.contains("(double)optInTimePeriod * (optInTimePeriod + 1) / 2.0"),
            "{name}: C divider not widened to double (#142)"
        );
        assert!(
            !out.rust.contains(">> 1"),
            "{name}: Rust divider still forms the int32 product before the cast (#142)"
        );
        assert!(
            !out.java.contains(">> 1"),
            "{name}: Java divider still uses the int32 `>> 1` shift (#142)"
        );
    }
    // Linear-regression family SumXSqr cubic: double, no int32 `/ 6` division.
    for name in ["linearreg", "linearreg_slope", "linearreg_intercept", "linearreg_angle", "tsf"] {
        let (func, enums) = load_indicator(name);
        let out = generate_all(&func, &enums);
        assert!(
            !out.c.contains("/ 6;"),
            "{name}: C SumXSqr still uses int32 `/ 6` division (#142 cubic overflow at period 1025)"
        );
        assert!(
            out.c
                .contains("(double)optInTimePeriod * (optInTimePeriod - 1) * (2 * optInTimePeriod - 1) / 6.0"),
            "{name}: C SumXSqr not widened to double (#142)"
        );
        assert!(
            !out.rust.contains("/ 6) as f64"),
            "{name}: Rust SumXSqr still forms the int32 cubic before the cast (#142)"
        );
    }
}

// ---------------------------------------------------------------------------
// Scratch-buffer election (issue #146)
// ---------------------------------------------------------------------------

/// The scratch election must reach the SMA fast path and *only* the SMA fast path:
/// the calculation writes straight into the caller's slices, while the general MA
/// path below it keeps its two genuine allocations.
#[test]
fn rust_bbands_elects_output_scratch_only_in_the_sma_fast_path() {
    let (func, enums) = load_indicator("bbands");
    let registry = make_registry();
    let helpers = make_helpers();
    let out = generate_all(&func, &enums);
    let rust_out = backends::rust_lang::generate(&func, &enums, &registry, &helpers);

    // No allocation-and-copy of an output slice survives anywhere.
    assert!(
        !rust_out.contains(".to_vec()"),
        "BBANDS Rust should not copy an output slice into a scratch Vec: {rust_out}"
    );
    // The SMA and the standard deviation are written into the outputs by name.
    assert!(
        rust_out.contains("outRealMiddleBand[_outIdx] = maTotal /"),
        "BBANDS Rust should write the SMA straight into outRealMiddleBand: {rust_out}"
    );
    // Pinned on the DESTINATION, which is the property this test exists for --
    // the deviation lands in outRealUpperBand by name, with no scratch Vec --
    // and on the sqrt being what lands there. NOT on the exact right-hand side:
    // #243 wrapped it in an exact-zero skip (`if variance != 0.0`), and pinning
    // the whole expression as text made an unrelated numerical fix red this
    // test for a property it never changed. Matching the line rather than a
    // substring keeps it tight -- a redirect to a scratch, or something other
    // than the root of the variance, still fails.
    let dev_write = rust_out
        .lines()
        .map(str::trim)
        .find(|l| l.starts_with("outRealUpperBand[_outIdx] ="))
        .unwrap_or_else(|| {
            panic!("BBANDS Rust should write the standard deviation into outRealUpperBand: {rust_out}")
        });
    assert!(
        dev_write.contains("(variance).sqrt()"),
        "the deviation written into outRealUpperBand should be the root of the variance, \
         got `{dev_write}`: {rust_out}"
    );
    assert!(
        rust_out.contains("tempReal = outRealUpperBand[i] * optInNbDevUp;"),
        "the band loop should read its deviation back out of outRealUpperBand: {rust_out}"
    );
    // The dead aliasing arms, the input-alias guard and the copy-back are gone.
    assert!(
        !rust_out.contains("inReal.as_ptr() == outRealUpperBand.as_ptr()"),
        "BBANDS Rust should not test inReal against an output: {rust_out}"
    );
    assert!(
        !rust_out.contains("tempBuffer1.as_ptr()"),
        "BBANDS Rust should have no pointer tests left on tempBuffer1: {rust_out}"
    );
    // The general MA path's allocations are real and must survive — one pair in
    // the batch variant, plus the stream tier's.
    let allocs = rust_out.matches("tempBuffer1 = vec![0.0_f64;").count();
    assert!(
        allocs >= 2,
        "the general MA path must keep its tempBuffer1 allocation in every variant \
         (found {allocs}): {rust_out}"
    );
    assert_eq!(
        allocs,
        rust_out.matches("tempBuffer2 = vec![0.0_f64;").count(),
        "tempBuffer1 and tempBuffer2 must be allocated in the same places: {rust_out}"
    );
    // Rust-only: the other backends assign the pointer/reference and keep C's
    // election chain verbatim.
    assert!(
        out.c.contains("tempBuffer1 = outRealMiddleBand;"),
        "the C backend must keep C's pointer election: {}",
        out.c
    );
    assert!(
        out.java.contains("tempBuffer1 = outRealMiddleBand;"),
        "the Java backend must keep C's reference election: {}",
        out.java
    );
}

/// Being general is not the same as being greedy. The matcher requires *every* arm
/// of the chain to be nothing but `scratch = someOutput;` elections, and that one
/// clause is what declines `STOCH`, `STOCHF` and `MAVP`: each mixes an allocation
/// and a `…IsAllocated = 1;` flag into a branch, so the branch is a genuine
/// in-place defence with a real buffer to allocate rather than an election.
/// `MAVP` is inverted as well — the allocation sits in the `then` and the election
/// in the `else` — so it is rejected on the very first link.
///
/// Their generated Rust must come out byte-for-byte as it was. That non-firing is
/// what lets the PR assert the other three backends were untouched, so it is
/// pinned here rather than left to `git diff`.
#[test]
fn rust_scratch_election_declines_arms_that_allocate() {
    let registry = make_registry();
    let helpers = make_helpers();

    for name in ["stoch", "stochf"] {
        let (func, enums) = load_indicator(name);
        let rust_out = backends::rust_lang::generate(&func, &enums, &registry, &helpers);
        assert!(
            rust_out.contains("tempBuffer = outSlowK.to_vec();")
                || rust_out.contains("tempBuffer = outFastK.to_vec();"),
            "{name}'s election arm must be untouched: {rust_out}"
        );
        assert!(
            rust_out.contains("tempBuffer = vec![0.0_f64;"),
            "{name} must keep the allocation on its other arm: {rust_out}"
        );
    }

    // `MAVP` is the inverted case, and the one a looser matcher reaches first.
    let (func, enums) = load_indicator("mavp");
    let rust_out = backends::rust_lang::generate(&func, &enums, &registry, &helpers);
    assert!(
        rust_out.contains("localFinalArray = outReal.to_vec();"),
        "MAVP's election must be left as it is: {rust_out}"
    );
    assert!(
        rust_out.contains("localFinalArray = vec![0.0_f64;"),
        "MAVP must keep the allocation in its `then` arm: {rust_out}"
    );
    assert!(
        rust_out.contains("localFinalArray.as_ptr() != outReal.as_ptr()"),
        "MAVP must keep its copy-back guard: {rust_out}"
    );

    // The pass must not have fired for a single function other than `BBANDS`. The
    // election note is emitted exactly when an election is installed, so its
    // absence across the whole `input/` tree is the non-firing proof — and it is
    // proven over every indicator rather than a hand-picked list, so a widening of
    // the rule cannot slip past by naming a function this test forgot.
    const NOTE: &str = "C's pointer election here is a rename";
    let mut fired = Vec::new();
    for name in discover_indicators() {
        let (func, enums) = load_indicator(&name);
        if backends::rust_lang::generate(&func, &enums, &registry, &helpers).contains(NOTE) {
            fired.push(name);
        }
    }
    assert_eq!(
        fired,
        vec!["bbands".to_string()],
        "the scratch election must fire for BBANDS and nothing else"
    );
}

/// C's scratch local is *function*-scoped: a pointer elected inside a nested
/// block still points at that output after the block ends. The rename only
/// reaches the end of the electing block, so it is equivalent only when nothing
/// afterwards can observe the local. `BBANDS` satisfies that because its fast
/// path `return`s; this fixture does not, and the election must be declined
/// rather than leave a read of a `Vec` that is never assigned.
#[test]
fn rust_scratch_election_declines_an_election_that_escapes_its_block() {
    let src = r#"
int bbands_lookback( int optInTimePeriod, double optInNbDevUp, double optInNbDevDn, TA_MAType optInMAType )
{
   return optInTimePeriod - 1;
}

TA_RetCode bbands( int startIdx, int endIdx,
   const double inReal[],
   int optInTimePeriod,
   double optInNbDevUp, double optInNbDevDn,
   TA_MAType optInMAType,
   int *outBegIdx, int *outNBElement,
   double outRealUpperBand[], double outRealMiddleBand[], double outRealLowerBand[] )
{
   double *tempBuffer1;
   double *tempBuffer2;
   int i;

   if( optInMAType == TA_MAType_SMA )
   {
      if( inReal == outRealUpperBand )
      {
         tempBuffer1 = outRealMiddleBand;
         tempBuffer2 = outRealLowerBand;
      }
      else
      {
         tempBuffer1 = outRealMiddleBand;
         tempBuffer2 = outRealUpperBand;
      }
      for( i=0; i < 10; i++ )
      {
         tempBuffer1[i] = inReal[i];
         tempBuffer2[i] = inReal[i];
      }
   }

   /* Control falls out of the electing block, and the local is read here. */
   for( i=0; i < 10; i++ )
   {
      outRealLowerBand[i] = tempBuffer1[i];
   }

   *outBegIdx = startIdx;
   *outNBElement = 10;
   return TA_SUCCESS;
}
"#;
    let (func, enums) = load_indicator_with_source("bbands", src);
    let rust = generate_all(&func, &enums).rust;
    assert!(
        rust.contains("tempBuffer1 = outRealMiddleBand.to_vec()"),
        "an election whose local is still read after the electing block must fall \
         back to the copy; renaming it would leave that read pointing at a Vec \
         nothing ever assigns: {rust}"
    );
}

// ---------------------------------------------------------------------------
