//! Streaming-analysis integration tests over the REAL ta_codegen/input corpus.
//!
//! These pin the stage-1 tier boundary: which functions analyze as T1/T2,
//! and — just as load-bearing — which are rejected and why. A batch rewrite
//! that changes a function's stream shape fails here before it fails in CI.

use std::path::{Path, PathBuf};

use ta_codegen_lib::ir::{self, FuncDef, StreamTier};
use ta_codegen_lib::parser;
use ta_codegen_lib::streaming::{self, StreamError};

fn input_dir() -> PathBuf {
    Path::new(env!("CARGO_MANIFEST_DIR")).join("../input")
}

fn load(name: &str) -> FuncDef {
    let dir = input_dir().join(name);
    let mut func = parser::yaml::parse_yaml(&dir.join(format!("{name}.yaml")));
    let parsed = parser::c_source::parse_c_source(&dir.join(format!("{name}.c")));
    parser::c_source::wire_parsed_source(&mut func, &parsed);
    func
}

/// Cross-function lookup over the real input tree (YAML-only, same data the
/// emitters read through the Registry).
fn lookup() -> &'static ta_codegen_lib::registry::Registry {
    crate::common::make_registry()
}

#[test]
fn mult_is_t1() {
    let f = load("mult");
    let m = streaming::analyze(&f).expect("MULT analyzes");
    assert_eq!(m.tier, StreamTier::T1);
    assert_eq!(m.bar_inputs, ["inReal0", "inReal1"]);
    assert!(m.state.is_empty() && m.lags.is_empty());
}

#[test]
fn ema_is_t2_scalar_recurrence() {
    let f = load("ema");
    let m = streaming::analyze(&f).expect("EMA analyzes");
    assert_eq!(m.tier, StreamTier::T2);
    assert!(m.lags.is_empty());
    assert!(m.state.iter().any(|(n, _)| n == "prevMA"));
}

#[test]
fn trange_is_t2_with_prev_close_lag() {
    let f = load("trange");
    let m = streaming::analyze(&f).expect("TRANGE analyzes");
    assert_eq!(m.tier, StreamTier::T2);
    assert_eq!(m.bar_inputs, ["inHigh", "inLow", "inClose"]);
    assert_eq!(m.lags.len(), 1);
    assert_eq!(m.lags[0].array, "inClose");
    assert_eq!(m.lags[0].depth, 1);
}

#[test]
fn macd_is_t2_multi_output() {
    let f = load("macd");
    let m = streaming::analyze(&f).expect("MACD analyzes");
    assert_eq!(m.tier, StreamTier::T2);
    assert_eq!(m.outputs.len(), 3);
}

#[test]
fn ad_countdown_loop_is_t2() {
    let f = load("ad");
    let m = streaming::analyze(&f).expect("AD analyzes");
    assert_eq!(m.tier, StreamTier::T2);
    assert!(m.counter().is_some());
}

#[test]
fn sma_is_t3_ring() {
    let f = load("sma");
    let m = streaming::analyze(&f).expect("SMA analyzes");
    assert_eq!(m.tier, StreamTier::T3);
    assert_eq!(m.rings().len(), 1);
    assert_eq!(m.rings()[0].arrays, ["inReal"]);
}

#[test]
fn atr_streams_as_t2() {
    // ATR dropped its `if (period <= 1) return trange(...)` delegation: the
    // Wilder coefficients are exactly (1, 0) at period 1, so the single path
    // streams as a plain T2 (carried prevATR, the two coefficients, and a lag-1
    // close read) for every period.
    let f = load("atr");
    let m = streaming::analyze(&f).expect("ATR analyzes as T2");
    assert_eq!(m.tier, StreamTier::T2);
    assert!(m.lags.iter().any(|l| l.array == "inClose"), "lag-1 close read");
}

#[test]
fn minus_dm_derives_dual_mode_plan() {
    // MINUS_DM keeps BOTH its `if (period <= 1) { raw DM1; return }` degenerate
    // arm (which ignores the unstable period) AND its Wilder general path.
    // Unlike ATR, the two arms differ under a nonzero K (the degenerate lookback
    // is 1 regardless of K), so neither an ATR-style drop nor rejection preserves
    // period=1 — it streams as a param-selected dual mode: two independent T2
    // models sharing one handle, selected by the period<=1 guard fixed at Open.
    let f = load("minus_dm");
    let plan = streaming::validate_streamable(&f, lookup()).expect("MINUS_DM derives a plan");
    let streaming::StreamPlan::DualMode(dm) = plan else {
        panic!("expected DualMode, got {plan:?}");
    };
    assert_eq!(dm.mode_a.tier, StreamTier::T2);
    assert_eq!(dm.mode_b.tier, StreamTier::T2);
    // The predicate is the batch's own period-degenerate guard.
    assert!(matches!(
        &dm.predicate,
        ta_codegen_lib::ir::Expr::BinOp(_, ta_codegen_lib::ir::BinOp::LessEq, _)
    ));
    // Mode B (general) carries the Wilder accumulator; mode A (raw DM1) does not.
    assert!(dm.mode_b.state.iter().any(|(n, _)| n == "prevMinusDM"));
    assert!(!dm.mode_a.state.iter().any(|(n, _)| n == "prevMinusDM"));
    assert!(dm.mode_a.state.len() < dm.mode_b.state.len());
}

#[test]
fn minus_di_derives_dual_mode_plan_with_tr() {
    // MINUS_DI is the DI variant: the general arm adds prevTR and divides (percent
    // DI); the degenerate arm returns the raw DM1/TR ratio (no x100 — the
    // documented period-1 quirk), preserved verbatim by transcribing the arm.
    let f = load("minus_di");
    let plan = streaming::validate_streamable(&f, lookup()).expect("MINUS_DI derives a plan");
    let streaming::StreamPlan::DualMode(dm) = plan else {
        panic!("expected DualMode, got {plan:?}");
    };
    assert!(dm.mode_b.state.iter().any(|(n, _)| n == "prevTR"));
    assert!(dm.func.streaming);
}

#[test]
fn trima_derives_dual_mode_if_else() {
    // TRIMA's `if (period % 2 == 1) { odd } else { even }` arms are genuinely
    // different triangular-sum recurrences (different factor and Step-2 order), so
    // BOTH stream, as a dual mode selected by parity. The two arms (both T3
    // tier) share rings (middleIdx, trailingIdx over inReal), so the handle
    // carries one ring set, and both fall through to a shared epilogue (the
    // if/else form).
    let f = load("trima");
    let plan = streaming::validate_streamable(&f, lookup()).expect("TRIMA derives a plan");
    let streaming::StreamPlan::DualMode(dm) = plan else {
        panic!("expected DualMode, got {plan:?}");
    };
    assert_eq!(dm.mode_a.tier, StreamTier::T3);
    assert_eq!(dm.mode_b.tier, StreamTier::T3);
    assert_eq!(dm.mode_a.rings().len(), 2, "middleIdx + trailingIdx rings");
    assert_eq!(
        dm.mode_a.rings().iter().map(|r| r.var.clone()).collect::<Vec<_>>(),
        dm.mode_b.rings().iter().map(|r| r.var.clone()).collect::<Vec<_>>(),
        "both arms carry the SAME rings (union collapses to one set)"
    );
    assert!(!dm.epilogue.is_empty(), "shared epilogue (if/else form)");
    // A modulo-equality branch: the predicate is the parity test itself.
    assert!(matches!(
        &dm.predicate,
        ta_codegen_lib::ir::Expr::BinOp(_, ta_codegen_lib::ir::BinOp::Eq, _)
    ));
}

/// The six rolling-extremum functions run a block-batched Van Herk scan in
/// batch, which cannot be transcribed per-bar, so each declares a
/// `PRAGMA TA_ALT={STREAM,ALL_LANGUAGES}` automaton. Resolving that claim is
/// what makes them streamable at all — and it must produce the *plainest* tier,
/// an ordinary `Loop` over the alternate, not a special plan kind.
#[test]
fn rolling_extremum_streams_from_its_stream_alternate() {
    for name in ["min", "max", "minmax", "midpoint", "midprice", "willr"] {
        let f = load(name);
        assert_eq!(f.alternates.len(), 1, "{name}: one alternate");
        let alt = &f.alternates[0];
        assert_eq!(alt.name, format!("{name}_ALT1"));
        assert_eq!(
            alt.claims,
            [ir::AltClaim { api: ir::ApiClaim::Stream, lang: ir::LangClaim::AllLanguages }]
        );

        for lang in ir::ALL_LANGS {
            let resolved = f.resolved_for(lang);
            let plan = streaming::validate_streamable(&resolved, lookup())
                .unwrap_or_else(|e| panic!("{name} [{}]: {e}", lang.as_str()));
            let streaming::StreamPlan::Loop(m) = plan else {
                panic!("{name}: expected a plain Loop over the alternate, got {plan:?}");
            };
            assert_eq!(m.tier, StreamTier::T4);
            assert!(m.extrema().is_some(), "{name}: the alternate is an extrema automaton");
        }

        // Without the claim the batch block scan is what the analyzer sees, and
        // it is genuinely not streamable — so the assertions above are testing
        // the resolution, not something that would hold anyway.
        assert!(
            streaming::validate_streamable(&f, lookup()).is_err(),
            "{name}: the batch body must NOT be streamable, or the alternate proves nothing"
        );
    }
}

#[test]
fn t3_identity_path_recognized() {
    let f = load("t3");
    let m = streaming::analyze(&f).expect("T3 analyzes with identity path");
    assert_eq!(m.tier, StreamTier::T2);
    let idp = m.identity.as_ref().expect("identity path");
    assert_eq!(idp.pairs, vec![("outReal".to_string(), "inReal".to_string())]);
}

#[test]
fn bbands_composed_rejected() {
    assert!(streaming::analyze(&load("bbands")).is_err());
}

/// Whole-corpus gate: every `streaming: true` function must analyze clean.
/// (The same check `generate` enforces; here it runs in `cargo test`.)
#[test]
fn all_declared_functions_are_streamable() {
    let base = input_dir();
    let lk = lookup();
    let mut checked = 0;
    for entry in std::fs::read_dir(&base).expect("input dir") {
        let dir = entry.expect("entry").path();
        if !dir.is_dir() {
            continue;
        }
        let name = dir.file_name().unwrap().to_string_lossy().to_string();
        let yaml = dir.join(format!("{name}.yaml"));
        let c = dir.join(format!("{name}.c"));
        if !yaml.exists() || !c.exists() {
            continue;
        }
        let func = load(&name);
        if func.streaming {
            // Per language: a `PRAGMA TA_ALT={STREAM,<lang>}` claim can hand one
            // backend a different body, so streamability is a per-language
            // property. This mirrors the generate-time gate.
            for lang in ir::ALL_LANGS {
                streaming::validate_streamable(&func.resolved_for(lang), lk)
                    .unwrap_or_else(|e| panic!("[{}] {e}", lang.as_str()));
            }
            checked += 1;
        }
    }
    assert!(checked >= 200, "expected the whole declared corpus, saw {checked}");
}

/* ---- CDL tranche: candle helpers, offset rings, array state ---- */

#[test]
fn cdldoji_is_t3_with_a_derived_ring() {
    let f = load("cdldoji");
    let m = streaming::analyze(&f).expect("CDLDOJI analyzes");
    assert_eq!(m.tier, StreamTier::T3);
    assert_eq!(m.rings().len(), 1);
    let r = &m.rings()[0];
    assert_eq!(r.var, "BodyDojiTrailingIdx");
    // ONE derived lane holding the computed candle range, not four raw OHLC
    // lanes: #229's collapse. The trailing subtraction needs the range, not the
    // prices, so retaining the prices was four times the memory and four times
    // the copy for a value the step recomputed anyway.
    assert_eq!(r.arrays, ["derived"]);
    assert_eq!((r.back, r.fwd), (0, 0), "plain oldest-slot ring");
    assert!(m.state.iter().any(|(n, _)| n == "BodyDojiPeriodTotal"));
}

#[test]
fn cdlonneck_ring_has_back_offset() {
    // Equal average runs on the SHIFTED candle: reads in[EqualTrailingIdx - 1]
    // -> absolute-mod ring layout with back >= 1.
    let f = load("cdlonneck");
    let m = streaming::analyze(&f).expect("CDLONNECK analyzes");
    let r = m
        .rings()
        .iter()
        .find(|r| r.var == "EqualTrailingIdx")
        .expect("Equal ring");
    assert!(r.back >= 1, "shifted-candle back-offset, got {}", r.back);
}

#[test]
fn cdleveningstar_ring_has_forward_offset() {
    // BodyShort average of the NEXT candle: reads in[BodyShortTrailingIdx + 1].
    let f = load("cdleveningstar");
    let m = streaming::analyze(&f).expect("CDLEVENINGSTAR analyzes");
    let r = m
        .rings()
        .iter()
        .find(|r| r.var == "BodyShortTrailingIdx")
        .expect("BodyShort ring");
    assert_eq!(r.fwd, 1, "forward read in[var + 1]");
    assert!(r.back >= 1, "forward reads force the absolute-mod layout");
}

/// Array names read anywhere in a transition, so a test can see which buffer a
/// read was routed to without rendering a backend.
fn read_arrays(m: &streaming::StreamModel<'_>) -> std::collections::BTreeSet<String> {
    let mut out = std::collections::BTreeSet::new();
    for st in &m.steady_stmts {
        streaming::walk_stmt_exprs(st, &mut |e| {
            streaming::walk_expr(e, &mut |x| {
                if let ir::Expr::ArrayAccess(n, _) = x {
                    out.insert(n.clone());
                }
            });
        });
    }
    out
}

/// Raw input columns still read at an index mentioning `off` — what a dropped
/// window buffer must leave none of. The lag reads (`in[cursor]`,
/// `in[cursor - 2]`) that feed the pattern logic name no counter and are not
/// this fold's business, so keying on the offset is what keeps the assertion
/// about the fold rather than about the function.
fn raw_reads_offset_by(m: &streaming::StreamModel<'_>, off: &str) -> Vec<String> {
    let mut out = Vec::new();
    for st in &m.steady_stmts {
        streaming::walk_stmt_exprs(st, &mut |e| {
            streaming::walk_expr(e, &mut |x| {
                if let ir::Expr::ArrayAccess(n, idx) = x {
                    if m.bar_inputs.iter().any(|b| b == n) && format!("{idx:?}").contains(off) {
                        out.push(format!("{n}[{idx:?}]"));
                    }
                }
            });
        });
    }
    out
}

#[test]
fn cdl3blackcrows_var_offset_ring_window_and_array_state() {
    // in[ShadowVeryShortTrailingIdx - totIdx] with for(totIdx=2; totIdx>=0;):
    // ring back = counter max (2), and the per-candle totals carry as
    // fixed-size array state.
    let f = crate::common::load_rescan_fixture("cdl3blackcrows").0;
    let m = streaming::analyze(&f).expect("CDL3BLACKCROWS analyzes");
    let r = m
        .rings()
        .iter()
        .find(|r| r.var == "ShadowVeryShortTrailingIdx")
        .expect("ShadowVeryShort ring");
    assert!(r.back >= 2, "counter-offset ring, got {}", r.back);
    assert!(
        m.state
            .iter()
            .any(|(n, t)| n == "ShadowVeryShortPeriodTotal"
                && matches!(t, ta_codegen_lib::ir::VarType::RealArray(_))),
        "fixed-size array state"
    );
}

#[test]
fn cdl3blackcrows_window_folds_into_its_ring() {
    // #229 last tranche: `in*[i - totIdx]` is read only through
    // ta_candlerange(ShadowVeryShort, ...) — the exact value the ShadowVeryShort
    // ring already stores — so the window keeps NO buffer of its own.
    let f = crate::common::load_rescan_fixture("cdl3blackcrows").0;
    let m = streaming::analyze(&f).expect("CDL3BLACKCROWS analyzes");
    assert!(
        m.windows().is_empty(),
        "the totIdx window is served by the ring, got {:?}",
        m.windows().iter().map(|w| &w.var).collect::<Vec<_>>()
    );
    // Dropping a window is only correct if the reads went SOMEWHERE: pin the
    // routing, not just the absence. An empty window list with the reads still
    // naming raw columns would be the fail-open this asserts against.
    let reads = read_arrays(&m);
    assert!(
        reads.contains("derivedAt_ShadowVeryShortTrailingIdx"),
        "window reads routed into the ShadowVeryShort ring, saw {reads:?}"
    );
    assert!(
        raw_reads_offset_by(&m, "totIdx").is_empty(),
        "no raw column is still read through the counter, saw {:?}",
        raw_reads_offset_by(&m, "totIdx")
    );
}

#[test]
fn avgdev_window_keeps_its_buffer() {
    // The control for the fold above, over the real corpus. Two guards would
    // each hold it on their own and it is the FIRST that fires: AVGDEV's window
    // is bounded by `optInTimePeriod`, not a literal, so the ring depth has
    // nothing to compare against. Its reads being raw columns is the second.
    // (The raw-column refusal itself is pinned by
    // `window_keeps_its_buffer_when_one_read_is_raw` in `streaming.rs`, where
    // the fixture can hold the two apart.)
    let f = load("avgdev");
    let m = streaming::analyze(&f).expect("AVGDEV analyzes");
    assert_eq!(
        m.windows().len(),
        1,
        "raw rescan reads keep their own buffer"
    );
    assert!(
        read_arrays(&m).contains("inReal"),
        "AVGDEV still reads the raw column"
    );
}

#[test]
fn cdlkickingbylength_ternary_index_hoisted() {
    // in[Ternary(cond, i, i-1)] normalizes to Ternary(cond, in[i], in[i-1]).
    let f = load("cdlkickingbylength");
    let m = streaming::analyze(&f).expect("CDLKICKINGBYLENGTH analyzes");
    assert_eq!(m.tier, StreamTier::T3);
    assert_eq!(m.rings().len(), 2, "BodyLong + ShadowVeryShort rings");
}

#[test]
fn cdladvanceblock_merges_window_bounds_to_widest() {
    // totIdx is bound by three loops (2, 1, 2 inclusive) — the merge keeps the
    // widest literal bound instead of rejecting. Since #229 dropped the window
    // buffer the bound is observable on the rings it sizes: every ring read at
    // `[<Setting>TrailingIdx - totIdx]` gets back = cap - 1 = 2, which is also
    // what makes the ring deep enough to serve the window read. Over the
    // pre-carry fixture: the shipped input no longer has the window.
    let f = crate::common::load_rescan_fixture("cdladvanceblock").0;
    let m = streaming::analyze(&f).expect("CDLADVANCEBLOCK analyzes");
    for setting in ["ShadowShort", "ShadowLong", "Near", "Far"] {
        let v = format!("{setting}TrailingIdx");
        let r = m
            .rings()
            .iter()
            .find(|r| r.var == v)
            .unwrap_or_else(|| panic!("{v} ring"));
        assert_eq!(r.back, 2, "widest inclusive bound 2 sizes {v}");
    }
}

#[test]
fn cdladvanceblock_window_folds_per_setting() {
    // One window counter, FOUR candle settings read through it, each routed to
    // its own ring — the case that made a single shared "derived" slot name
    // insufficient. The fixture also holds a window read and a ring read of
    // the same Near setting in ONE statement, so the two folds must be told
    // apart by index form rather than by shape.
    let f = crate::common::load_rescan_fixture("cdladvanceblock").0;
    let m = streaming::analyze(&f).expect("CDLADVANCEBLOCK analyzes");
    assert!(m.windows().is_empty(), "the totIdx window is served by rings");
    let reads = read_arrays(&m);
    for setting in ["ShadowShort", "ShadowLong", "Near", "Far"] {
        assert!(
            reads.contains(&format!("derivedAt_{setting}TrailingIdx")),
            "{setting} window read routed to its own ring, saw {reads:?}"
        );
    }
    // The trailing side of the same statements still resolves through the
    // shared slot: both folds fired, neither swallowed the other.
    assert!(reads.contains("derived"), "trailing reads still folded");
}

#[test]
fn ultosc_analyzes_t3() {
    // Unlocked by the descending-inclusive window form.
    let f = load("ultosc");
    let m = streaming::analyze(&f).expect("ULTOSC analyzes");
    assert_eq!(m.tier, StreamTier::T3);
}

#[test]
fn cdlhikkake_streams_via_countdown_refactor() {
    // The absolute `patternIdx = i` (a cursor leak) was refactored to a carried
    // confirmation countdown + cached 2nd-candle high/low, so the transition reads
    // no bare cursor and it now streams (bit-identical batch, verified vs v0.6.4).
    let f = load("cdlhikkake");
    assert!(
        streaming::validate_streamable(&f, lookup()).is_ok(),
        "CDLHIKKAKE streams after the countdown refactor"
    );
}

#[test]
fn ht_dcperiod_streams_via_carried_parity_and_gate_strip() {
    // M7c: the Hilbert-transform family reads the ABSOLUTE cursor `today` twice —
    // the `today % 2` odd/even quadrature branch and the in-loop
    // `if (today >= startIdx)` output gate. Two general steady-loop normalizations
    // (strip_cursor_output_gate + carry_cursor_parity) let it fall into the
    // ordinary Batch loop tier:
    //   (1) the output gate is STRIPPED (so `startIdx` no longer leaks into the
    //       steady loop — this used to reject at analyze with "steady loop
    //       references `startIdx`"), and
    //   (2) `today % 2` is CARRIED as an int `streamParity` field (so `today` no
    //       longer leaks into the transition — this used to reject at
    //       build_transition with "index variable `today` leaks").
    // This is the positive pin AND the neuter-check for both recognizers:
    //   * remove the parity carry  -> model.parity is None (fails the assert
    //     below) and build_transition leaks `today` (panics the C render pin
    //     test_c_ht_dcperiod_parity_stream_section);
    //   * remove the gate strip    -> analyze() errors "steady loop references
    //     `startIdx`" (fails the analyze-Ok assert below).
    let f = load("ht_dcperiod");
    let m = streaming::analyze(&f).expect("HT_DCPERIOD analyzes once the HT gates are normalized");
    assert_eq!(m.tier, StreamTier::T3, "WMA trailing ring => T3");
    // (2) parity carried as an int state field, seeded/flipped by the emitter.
    let parity = m.parity.as_ref().expect("carried-parity spec present");
    assert_eq!(parity.field, "streamParity");
    assert!(
        m.state.iter().any(|(n, t)| n == "streamParity"
            && matches!(t, ta_codegen_lib::ir::VarType::Integer)),
        "streamParity is an int state field"
    );
    // The WMA price smoother is one trailing ring over inReal (like WMA/SMA).
    assert_eq!(m.rings().len(), 1);
    assert_eq!(m.rings()[0].arrays, ["inReal"]);
    // The 8 Hilbert double[3] buffers ride as fixed-array carried state.
    assert!(
        m.state.iter().any(|(n, t)| n == "detrender_Even"
            && matches!(t, ta_codegen_lib::ir::VarType::RealArray(_))),
        "detrender_Even carried as a fixed double[3] array"
    );
    // (1) gate stripped: the steady loop no longer references `startIdx`.
    let mut steady_vars = std::collections::BTreeSet::new();
    for s in &m.steady_stmts {
        streaming::stmt_var_names(s, &mut steady_vars);
    }
    assert!(
        !steady_vars.contains("startIdx"),
        "the output gate must be stripped from the steady loop"
    );
    // The whole plan validates as an ordinary Loop model.
    assert!(matches!(
        streaming::validate_streamable(&f, lookup()),
        Ok(streaming::StreamPlan::Loop(_))
    ));
}

#[test]
fn carried_parity_and_gate_strip_recognized_in_isolation() {
    // A minimal synthetic body carrying BOTH HT traits (a `today % 2` branch and
    // an `if (today >= startIdx)` output gate) over a plain scalar recurrence —
    // isolates the two recognizers from HT_DCPERIOD's real source, so this
    // coverage survives any future edit to ht_dcperiod.c. Borrows ht_dcperiod's
    // YAML shape (one real input, one real output).
    let src = r#"
TA_RetCode ht_dcperiod( int startIdx, int endIdx,
   const double inReal[],
   int *outBegIdx, int *outNBElement, double outReal[] )
{
   int outIdx, today;
   double acc;
   if( startIdx < 1 )
      startIdx = 1;
   if( startIdx > endIdx )
   {
      *outBegIdx = 0;
      *outNBElement = 0;
      return TA_SUCCESS;
   }
   *outBegIdx = startIdx;
   today = 0;
   acc = 0.0;
   outIdx = 0;
   while( today <= endIdx )
   {
      if( (today%2) == 0 )
         acc = (0.5*acc) + inReal[today];
      else
         acc = (0.2*acc) + inReal[today];
      if( today >= startIdx )
         outReal[outIdx++] = acc;
      today++;
   }
   *outNBElement = outIdx;
   return TA_SUCCESS;
}
"#;
    let f = load_with_source("ht_dcperiod", src);
    let m = streaming::analyze(&f).expect("synthetic HT body analyzes after normalization");
    assert!(m.parity.is_some(), "carried parity recognized");
    assert!(m.state.iter().any(|(n, _)| n == "acc"), "the recurrence carries `acc`");
    // build_transition must succeed (no `today` leak): validate through the C
    // emitter, which builds the transition and would panic on a leak.
    assert!(streaming::validate_streamable(&f, lookup()).is_ok());
}

#[test]
fn transition_build_rejects_saved_cursor_index() {
    // Pins the STAGE-2 build_transition guard "index variable `i` leaks into the
    // transition body": a function whose steady loop SAVES the absolute cursor
    // into a carried scalar and then reads that absolute index passes analysis
    // but cannot have a transition built (a stream cannot reconstruct an absolute
    // bar number). This is the exact wall the pre-refactor CDLHIKKAKE tripped;
    // keeping a fixture here means the guard stays covered now that CDLHIKKAKE streams.
    let src = r#"
TA_RetCode cdlhikkake( int startIdx, int endIdx,
   const double inOpen[], const double inHigh[], const double inLow[], const double inClose[],
   int *outBegIdx, int *outNBElement, int outInteger[] )
{
   int i, outIdx, savedIdx;
   if( startIdx < 5 )
      startIdx = 5;
   if( startIdx > endIdx )
   {
      *outBegIdx = 0;
      *outNBElement = 0;
      return TA_SUCCESS;
   }
   savedIdx = 0;
   outIdx = 0;
   for( i = startIdx; i <= endIdx; i++ )
   {
      if( inHigh[i] > inHigh[i-1] )
         savedIdx = i;
      outInteger[outIdx++] = i - savedIdx;
   }
   *outNBElement = outIdx;
   *outBegIdx = startIdx;
   return TA_SUCCESS;
}
"#;
    let f = load_with_source("cdlhikkake", src);
    assert!(streaming::analyze(&f).is_ok(), "analysis alone passes (a plain endIdx loop)");
    assert!(
        streaming::validate_streamable(&f, lookup()).is_err(),
        "the saved absolute cursor must reject at transition build"
    );
}

/* ---- TC composed tier: dispatch plans ---- */

/// FOREVER CONTRACT (user-mandated): every `MAType` must be streamable.
///
/// A function with a `MAType` parameter — TA_MA and everything that composes
/// over it (BBANDS, STOCH, STOCHF, MACDEXT) — can only stream a given `MAType`
/// bit-exact-vs-batch if the UNDERLYING MA function streams. A `MAType` whose
/// function does not stream makes EVERY such function's stream reject (not
/// bit-exact) for that type. So each `MAType`'s function must carry a stream,
/// with a shrinking allowlist of known deep blockers.
///
/// This ratchets: the test fails if (a) a non-blocked `MAType`'s function LOSES
/// its stream (a regression that would silently narrow every consumer), or (b)
/// a blocked function GAINS a stream (the allowlist is stale — remove it so the
/// contract tightens). When the allowlist empties, every `MAType` streams.
#[test]
fn every_matype_is_streamable_except_tracked_blockers() {
    use ta_codegen_lib::streaming::CalleeLookup;
    let lk = lookup();
    // Each `TA_MAType_*` value and its input-level function name, in enum order.
    let matypes = [
        ("SMA", "sma"),
        ("EMA", "ema"),
        ("WMA", "wma"),
        ("DEMA", "dema"),
        ("TEMA", "tema"),
        ("TRIMA", "trima"),
        ("KAMA", "kama"),
        ("MAMA", "mama"),
        ("T3", "t3"),
        ("HMA", "hma"),
        ("ZLEMA", "zlema"),
        ("RMA", "rma"),
        ("VIDYA", "vidya"),
        ("ALMA", "alma"),
    ];
    // Not-yet-streamable MAType functions (deep blockers). MUST ONLY SHRINK.
    // NOW EMPTY: MAMA streamed in M7c (it is an ordinary HT function — WMA ring +
    // Hilbert arrays + `today % 2` parity + the two outputs mama/fama in an output
    // gate — covered by the same strip_cursor_output_gate + carry_cursor_parity
    // normalizations, no circbuf/window, no `startIdx` read in its steady loop).
    // Every MAType function now streams. MA's *dispatch* also streams every arm,
    // including MAType_MAMA, whose arm forwards the MAMA line and writes FAMA
    // into a scratch (pinned by ma_derives_dispatch_plan). No blockers remain.
    let blocked: [&str; 0] = [];
    for (ty, func) in matypes {
        let streams = lk.callee(func).is_some_and(|s| s.streaming);
        let is_blocked = blocked.contains(&func);
        assert_eq!(
            streams, !is_blocked,
            "MAType streaming contract: {ty} ({func}) streams={streams}, blocked={is_blocked}. \
             Every MAType function must stream; a non-blocked one that stopped streaming is a \
             regression, and a blocked one that now streams means the allowlist is stale — \
             remove it."
        );
    }
    // The allowlist is exactly the un-streamable MATypes — no stale entries.
    for func in blocked {
        assert!(
            matypes.iter().any(|(_, f)| *f == func),
            "blocklist entry `{func}` is not a MAType"
        );
    }
}

#[test]
fn ma_derives_dispatch_plan() {
    use ta_codegen_lib::streaming::OutSlot;
    // MA is the MAType-tagged dispatch over the per-MA streams. The
    // supported-arm set is DERIVED from the callees' YAML stream flags: TRIMA
    // joined when its stream landed (M6c); MA's MAMA arm forwards the MAMA line
    // to outReal and writes FAMA into a scratch it allocates and frees. Every arm
    // now streams.
    let f = load("ma");
    // The YAML flag itself is load-bearing: losing it would silently drop
    // TA_MA_Stream from every generated surface while all gates stay green
    // (backend_suite force-sets the flag for shape pinning; the regtest
    // set-equality check passes when both sides lose the stream together).
    assert!(f.streaming, "ma.yaml must carry the stream flag");
    let lk = lookup();
    let plan = streaming::validate_streamable(&f, lk).expect("MA derives a plan");
    let streaming::StreamPlan::Dispatch(dp) = plan else {
        panic!("MA must derive a dispatch plan, not a loop model");
    };
    assert_eq!(dp.param, "optInMAType");
    assert!(dp.identity.is_some(), "period==1 identity path");
    let supported: Vec<&str> = dp
        .arms
        .iter()
        .filter(|a| a.supported)
        .map(|a| a.callee.as_str())
        .collect();
    // The set below IS the count, so no separate number is pinned next to it --
    // one that has to be edited in step with a list it sits beside only ever
    // goes stale.
    assert_eq!(dp.arms.len(), supported.len(), "every batch arm is recognized");
    assert_eq!(
        supported,
        ["sma", "ema", "wma", "dema", "tema", "trima", "kama", "mama", "t3", "hma", "zlema",
         "rma", "vidya", "alma"],
        "every arm streams: single-output MAs plus MAMA with FAMA in a scratch \
         (TRIMA joined in M6c, HMA via the dual-mode buffer union in #141)"
    );
    // No reject arms remain: HMA (#139) was the last, flipped to supported by
    // its YAML `stream` flag + the dual-mode per-arm buffer union (#141).
    let rejected: Vec<&str> = dp.unsupported_labels();
    assert!(rejected.is_empty(), "no reject arms remain, got {rejected:?}");
    // The MAMA arm forwards mama's output 0 (the MAMA line) into MA's single
    // output and discards output 1 (FAMA) into the arm's scratch.
    let mama = dp.arms.iter().find(|a| a.callee == "mama").unwrap();
    assert_eq!(mama.out_map, vec![OutSlot::Forward(0), OutSlot::Discard]);
    assert_eq!(mama.opt_args.len(), 2, "fixed 0.5 / 0.05 fast/slow limits");
    // A single-output arm maps its one output straight through (no discards).
    let sma = dp.arms.iter().find(|a| a.callee == "sma").unwrap();
    assert_eq!(sma.out_map, vec![OutSlot::Forward(0)]);
    // T3's arm forwards the fixed vfactor literal positionally.
    let t3 = dp.arms.iter().find(|a| a.callee == "t3").unwrap();
    assert_eq!(t3.opt_args.len(), 2, "period + literal 0.7 vfactor");
}

/// A NULL output argument in a dispatch arm is never a delegation: every callee
/// output is required. MA's MAMA arm rewritten to the bare
/// `mama(..., outReal, NULL)` call must be a HARD gate error (`mama` is
/// stream-flagged), never a silent reject arm the verify precheck would bless;
/// so must a scratch arm that never frees its scratch.
#[test]
fn dispatch_rejects_a_null_output_argument() {
    use ta_codegen_lib::ir::{Expr, Statement};
    fn mama_arm(f: &mut FuncDef, edit: &dyn Fn(&mut Vec<Statement>)) {
        let mut hits = 0;
        for st in &mut f.body {
            if let Statement::Switch { cases, .. } = st {
                for (_, body) in cases.iter_mut() {
                    let calls_mama = body
                        .iter()
                        .any(|s| matches!(s, Statement::Assign { value: Expr::FuncCall(n, _), .. } if n == "mama"));
                    if calls_mama {
                        edit(body);
                        hits += 1;
                    }
                }
            }
        }
        assert_eq!(hits, 1, "exactly one MAMA arm rewritten");
    }
    let shipped = load("ma");
    assert!(streaming::validate_streamable(&shipped, lookup()).is_ok(), "MA streams with its scratch arm");

    let mut null_arg = shipped.clone();
    mama_arm(&mut null_arg, &|body| {
        let call = body
            .iter()
            .find(|s| matches!(s, Statement::Assign { value: Expr::FuncCall(n, _), .. } if n == "mama"))
            .cloned()
            .unwrap();
        let Statement::Assign { target, value: Expr::FuncCall(n, mut args), compound } = call else {
            unreachable!()
        };
        *args.last_mut().unwrap() = Expr::Var("NULL".into());
        *body = vec![Statement::Assign { target, value: Expr::FuncCall(n, args), compound }];
    });
    let mut leaked = shipped.clone();
    mama_arm(&mut leaked, &|body| {
        body.retain(|s| !matches!(s, Statement::Expr(Expr::FuncCall(n, _)) if n == "free"));
    });
    for (what, f) in [("NULL argument", &null_arg), ("unfreed scratch", &leaked)] {
        let err = streaming::analyze_dispatch(f, lookup()).unwrap_err();
        assert!(
            matches!(err, StreamError::Unsupported(ref m) if m.contains("whole-range")),
            "{what}: expected hard shape error, got: {err}"
        );
    }
}

#[test]
fn dispatch_hard_errors_when_flagged_callee_arm_loses_shape() {
    // A stream-flagged callee arm that is not a strict whole-range
    // delegation must be a loud gate error, never a silent reject arm
    // (that would turn a generator regression into a vacuous pass).
    struct OneFlagged;
    impl streaming::CalleeLookup for OneFlagged {
        fn callee(&self, name: &str) -> Option<streaming::CalleeSig> {
            (name == "sma").then_some(streaming::CalleeSig {
                streaming: true,
                nan_inf_output: false,
                n_inputs: 1,
                n_opts: 1,
                n_outputs: 1,
            })
        }
    }
    let mut f = load("ma");
    // Sabotage: swap the SMA arm's endIdx arg so the shape check fails.
    sabotage_first_sma_arm(&mut f);
    let err = streaming::analyze_dispatch(&f, &OneFlagged).unwrap_err();
    assert!(
        matches!(err, StreamError::Unsupported(ref m) if m.contains("whole-range")),
        "expected hard shape error, got: {err}"
    );
}

#[test]
fn dispatch_hard_errors_when_flagged_delegation_hides_behind_unflagged_call() {
    // The review-confirmed silent-downgrade hole: an arm whose FIRST
    // indicator call is unflagged (the wrapper) but which then whole-range
    // delegates to a stream-flagged callee (dema) must be a hard gate
    // error — never a reject arm the verify precheck would bless.
    // TRIMA now really streams (M6c), so we override it back to unflagged in the
    // lookup — the test pins the GATE BEHAVIOR (a hidden flagged delegation),
    // not TRIMA's flag.
    use ta_codegen_lib::ir::{Expr, Statement};
    struct TrimaUnflagged<'a>(&'a dyn streaming::CalleeLookup);
    impl streaming::CalleeLookup for TrimaUnflagged<'_> {
        fn callee(&self, name: &str) -> Option<streaming::CalleeSig> {
            let mut sig = self.0.callee(name)?;
            if name == "trima" {
                sig.streaming = false;
            }
            Some(sig)
        }
    }
    let mut f = load("ma");
    let real = lookup();
    let lk = TrimaUnflagged(real);
    fn visit(stmts: &mut [Statement]) {
        for s in stmts {
            if let Statement::Switch { cases, .. } = s {
                for (_, body) in cases.iter_mut() {
                    let is_dema = body.iter().any(|st| {
                        matches!(st,
                            Statement::Assign { value: Expr::FuncCall(n, _), .. } if n == "dema")
                    });
                    if is_dema {
                        let call = Statement::Assign {
                            target: Expr::Var("retCode".into()),
                            value: Expr::FuncCall(
                                "trima".into(),
                                vec![
                                    Expr::Var("startIdx".into()),
                                    Expr::Var("endIdx".into()),
                                    Expr::Var("inReal".into()),
                                    Expr::Var("optInTimePeriod".into()),
                                    Expr::Var("outBegIdx".into()),
                                    Expr::Var("outNBElement".into()),
                                    Expr::Var("outReal".into()),
                                ],
                            ),
                            compound: false,
                        };
                        body.insert(0, call);
                        return;
                    }
                }
            }
        }
    }
    visit(&mut f.body);
    let err = streaming::analyze_dispatch(&f, &lk).unwrap_err();
    assert!(
        matches!(err, StreamError::Unsupported(ref m)
            if m.contains("dema") && m.contains("whole-range")),
        "expected hard error naming the flagged callee, got: {err}"
    );
}

/// `ma` carries the "nothing to produce" guard every other composed core has
/// (#267), and the dispatch analyzer admits it.
///
/// Non-vacuous in both directions: the first assertion fails if `ma.c` loses the
/// guard, the second if `analyze_dispatch` stops admitting it. Before #267 the
/// second was the live one — a leading `If` was an unrecognized top-level
/// statement, which is why `ma` was the last core withheld from the phantom-I/O
/// sweep.
#[test]
fn ma_dispatch_admits_the_empty_range_guard() {
    let f = load("ma");
    assert!(
        empty_range_guard_idx(&f).is_some(),
        "ma.c must carry a top-level `ma_lookback(..) > endIdx` guard"
    );
    assert!(
        streaming::validate_streamable(&f, lookup()).is_ok(),
        "the guard must not cost ma its dispatch plan"
    );
}

/// The guard is admitted by SHAPE, and a near-miss is a hard error rather than a
/// silent pass.
///
/// This is the only coverage the recognizer has: `ma` is the sole dispatch
/// function in the corpus and no `SYNTH*` fixture is dispatch-shaped, so a
/// recognizer one notch too loose — accepting any `If`, or any `If` that returns
/// SUCCESS — would retire the strictness contract the `other =>` arm exists to
/// enforce and nothing else in the tree would fail.
#[test]
fn dispatch_rejects_a_near_miss_empty_range_guard() {
    use ta_codegen_lib::ir::{Expr, Statement};

    // Each mutation keeps the guard where it is and breaks ONE clause of the
    // shape. All must land in the catch-all arm, which names the statement kind.
    // Plain `fn` items, not closures: a boxed-closure table is a `type_complexity`
    // warning and the nightly clippy job runs with `-D warnings`.
    type Mutation = (&'static str, fn(&mut Statement));
    fn wrong_right_operand(st: &mut Statement) {
        if let Statement::If { condition: Expr::BinOp(_, _, r), .. } = st {
            **r = Expr::Var("startIdx".into());
        }
    }
    fn not_a_lookback_call(st: &mut Statement) {
        if let Statement::If { condition: Expr::BinOp(l, _, _), .. } = st {
            **l = Expr::Var("optInTimePeriod".into());
        }
    }
    /// A FOREIGN lookback. This is the mutation the byte-identity argument
    /// depends on catching: `DispatchPlan` carries no prologue, so the four Open
    /// tiers would never see it while every batch tier transcribed it verbatim,
    /// and no gate compares the two.
    fn someone_elses_lookback(st: &mut Statement) {
        if let Statement::If { condition: Expr::BinOp(l, _, _), .. } = st {
            if let Expr::FuncCall(n, _) = l.as_mut() {
                *n = "t3_lookback".into();
            }
        }
    }
    /// The right lookback, the wrong arguments — a guard stating a lookback this
    /// call will not compute.
    fn wrong_lookback_arguments(st: &mut Statement) {
        if let Statement::If { condition: Expr::BinOp(l, _, _), .. } = st {
            if let Expr::FuncCall(_, a) = l.as_mut() {
                a[0] = Expr::IntLiteral(30);
            }
        }
    }
    fn body_does_more(st: &mut Statement) {
        if let Statement::If { then_body, .. } = st {
            then_body.insert(
                0,
                Statement::Assign {
                    target: Expr::ArrayAccess("outReal".into(), Box::new(Expr::IntLiteral(0))),
                    value: Expr::Literal(0.0),
                    compound: false,
                },
            );
        }
    }
    fn reports_a_non_empty_range(st: &mut Statement) {
        if let Statement::If { then_body, .. } = st {
            if let Some(Statement::Assign { value, .. }) = then_body.get_mut(1) {
                *value = Expr::IntLiteral(1);
            }
        }
    }
    fn returns_something_else(st: &mut Statement) {
        if let Statement::If { then_body, .. } = st {
            if let Some(last) = then_body.last_mut() {
                *last = Statement::Return { value: Some(Expr::Var("BAD_PARAM".into())) };
            }
        }
    }
    fn carries_an_else_arm(st: &mut Statement) {
        if let Statement::If { else_body, .. } = st {
            else_body.push(Statement::Return { value: Some(Expr::Var("BAD_PARAM".into())) });
        }
    }

    let mutations: [Mutation; 8] = [
        ("condition compares against something other than endIdx", wrong_right_operand),
        ("condition's left operand is not a lookback call", not_a_lookback_call),
        ("the guard states some other function's lookback", someone_elses_lookback),
        ("the guard's lookback takes arguments this call does not", wrong_lookback_arguments),
        ("the body does more than answer 0,0", body_does_more),
        ("the guard reports a non-empty range", reports_a_non_empty_range),
        ("the guard returns something other than SUCCESS", returns_something_else),
        ("the guard carries an else arm", carries_an_else_arm),
    ];

    for (what, mutate) in mutations {
        let mut f = load("ma");
        let idx = empty_range_guard_idx(&f).expect("ma carries the guard");
        mutate(&mut f.body[idx]);
        let err = streaming::analyze_dispatch(&f, lookup()).unwrap_err();
        assert!(
            matches!(err, StreamError::Unsupported(ref m)
                if m.contains("unrecognized top-level statement")),
            "{what}: expected the catch-all hard error, got: {err}"
        );
    }

    // Control: unmutated, the same body analyzes.
    let f = load("ma");
    assert!(
        streaming::analyze_dispatch(&f, lookup()).is_ok(),
        "the unmutated dispatch body must still analyze"
    );

    // A SECOND guard is not admitted either — the recognizer skips one index,
    // and anything else at the top level is still the hard error.
    let mut f = load("ma");
    let idx = empty_range_guard_idx(&f).expect("ma carries the guard");
    let dup = f.body[idx].clone();
    f.body.insert(idx, dup);
    let err = streaming::analyze_dispatch(&f, lookup()).unwrap_err();
    assert!(
        matches!(err, StreamError::Unsupported(ref m)
            if m.contains("unrecognized top-level statement")),
        "a second leading guard must not be admitted, got: {err}"
    );

    // LEADING, not merely present. Behind the switch the guard answers the same
    // 0,0 with the same values, so nothing but the nightly phantom-I/O sweep
    // could see that the forward happened first — which is the whole defect.
    let mut f = load("ma");
    let idx = empty_range_guard_idx(&f).expect("ma carries the guard");
    let guard = f.body.remove(idx);
    let sw = f
        .body
        .iter()
        .position(|s| matches!(s, Statement::Switch { .. }))
        .expect("ma's dispatch switch");
    f.body.insert(sw + 1, guard);
    let err = streaming::analyze_dispatch(&f, lookup()).unwrap_err();
    assert!(
        matches!(err, StreamError::Unsupported(ref m) if m.contains("after the switch")),
        "a guard behind the switch must be a hard error, got: {err}"
    );
}

/// Index of the top-level `<f>_lookback(..) > endIdx` guard in a body, if any.
fn empty_range_guard_idx(f: &FuncDef) -> Option<usize> {
    use ta_codegen_lib::ir::{BinOp, Expr, Statement};
    f.body.iter().position(|s| {
        matches!(s, Statement::If { condition: Expr::BinOp(l, BinOp::Greater, r), .. }
            if matches!(l.as_ref(), Expr::FuncCall(n, _) if n.ends_with("_lookback"))
                && matches!(r.as_ref(), Expr::Var(v) if v == "endIdx"))
    })
}

fn sabotage_first_sma_arm(f: &mut FuncDef) {
    use ta_codegen_lib::ir::{Expr, Statement};
    fn visit(stmts: &mut [Statement]) {
        for s in stmts {
            if let Statement::Switch { cases, .. } = s {
                for (_, body) in cases.iter_mut() {
                    for st in body.iter_mut() {
                        if let Statement::Assign {
                            value: Expr::FuncCall(name, args),
                            ..
                        } = st
                        {
                            if name == "sma" {
                                args[1] = Expr::Var("startIdx".into());
                                return;
                            }
                        }
                    }
                }
            }
        }
    }
    visit(&mut f.body);
}

#[test]
fn dx_output_feedback_carried_as_lastout() {
    // DX repeats the previous output on a zero denominator: out[idx-1] reads
    // become lastOut_* state (written after each update).
    let f = load("dx");
    let m = streaming::analyze(&f).expect("DX analyzes");
    assert_eq!(m.out_feedback, ["outReal"]);
}

#[test]
fn imi_cursor_anchored_window_reindexed() {
    // `for (i = cursor-(p-1); i <= cursor; i++)` normalizes to a descending
    // offset counter — a plain rescan window, bars still oldest-first.
    let f = load("imi");
    let m = streaming::analyze(&f).expect("IMI analyzes");
    assert_eq!(m.tier, StreamTier::T3);
    assert!(!m.windows().is_empty(), "reindexed rescan window");
    assert!(m.state.is_empty(), "pure window recompute carries no state");
}

/* ---- TC composed tier: producer + pipeline plans ---- */

#[test]
fn stoch_derives_composed_plan() {
    let f = load("stoch");
    assert!(f.streaming, "stoch.yaml must carry the stream flag");
    let lk = lookup();
    let plan = streaming::validate_streamable(&f, lk).expect("STOCH derives a plan");
    let streaming::StreamPlan::Composed(cp) = plan else {
        panic!("STOCH must derive a composed plan");
    };
    assert_eq!(cp.series.as_deref(), Some("tempBuffer"));
    let producer = cp.producer.as_ref().expect("STOCH has a producer loop");
    assert_eq!(producer.tier, StreamTier::T4, "raw %K extrema producer");
    assert_eq!(producer.outputs, ["tempBuffer"]);
    assert_eq!(cp.subs.len(), 2);
    // Sub 0: in-place smoothing of the raw %K; sub 1: %D from smoothed %K.
    assert_eq!(
        (
            cp.subs[0].callee.as_str(),
            cp.subs[0].srcs.as_slice(),
            cp.subs[0].dsts.as_slice(),
        ),
        ("ma", ["tempBuffer".to_string()].as_slice(), ["tempBuffer".to_string()].as_slice())
    );
    assert_eq!(
        (
            cp.subs[1].callee.as_str(),
            cp.subs[1].srcs.as_slice(),
            cp.subs[1].dsts.as_slice(),
        ),
        ("ma", ["tempBuffer".to_string()].as_slice(), ["outSlowD".to_string()].as_slice())
    );
    // Pipeline: sub0, sub1, then the memmove tail-align of outSlowK.
    assert_eq!(cp.steps.len(), 3);
    assert!(matches!(cp.steps[0], streaming::UpdateStep::Sub { sub_idx: 0 }));
    assert!(matches!(cp.steps[1], streaming::UpdateStep::Sub { sub_idx: 1 }));
    assert!(matches!(
        &cp.steps[2],
        streaming::UpdateStep::Align { dst, src, copy: false } if dst == "outSlowK" && src == "tempBuffer"
    ));
}

#[test]
fn stochf_derives_composed_plan() {
    let f = load("stochf");
    assert!(f.streaming, "stochf.yaml must carry the stream flag");
    let plan = streaming::validate_streamable(&f, lookup()).expect("STOCHF derives a plan");
    let streaming::StreamPlan::Composed(cp) = plan else {
        panic!("STOCHF must derive a composed plan");
    };
    assert_eq!(cp.subs.len(), 1);
    assert_eq!(cp.subs[0].dsts, ["outFastD"]);
    assert!(matches!(
        &cp.steps[1],
        streaming::UpdateStep::Align { dst, src, copy: false } if dst == "outFastK" && src == "tempBuffer"
    ));
}

#[test]
fn bbands_derives_composed_plan_after_sma_fusion() {
    // Regression for #117: the fused SMA fast path no longer calls TA_MA, so
    // `is_fastpath_block` must recognize it by its MA-type (enum) guard and keep
    // excluding it, so BBANDS still streams as the general TA_MA + TA_STDDEV
    // composition.
    let f = load("bbands");
    assert!(f.streaming, "bbands.yaml must carry the stream flag");
    let plan = streaming::validate_streamable(&f, lookup()).expect("BBANDS derives a plan");
    let streaming::StreamPlan::Composed(cp) = plan else {
        panic!("BBANDS must derive a composed plan");
    };
    let callees: Vec<&str> = cp.subs.iter().map(|s| s.callee.as_str()).collect();
    assert_eq!(
        callees,
        ["ma", "stddev"],
        "BBANDS composes TA_MA (middle band) + TA_STDDEV (deviation)"
    );
}

#[test]
fn composed_hard_errors_when_subcall_callee_lacks_stream() {
    // A composed function only streams when every sub-call does: a callee
    // without a stream is a loud error (actionable census line), never a
    // silent skip — STOCHRSI stays blocked this way until its pieces land.
    struct MaUnflagged;
    impl streaming::CalleeLookup for MaUnflagged {
        fn callee(&self, name: &str) -> Option<streaming::CalleeSig> {
            (name == "ma").then_some(streaming::CalleeSig {
                streaming: false,
                nan_inf_output: false,
                n_inputs: 1,
                n_opts: 2,
                n_outputs: 1,
            })
        }
    }
    let f = load("stoch");
    let err = streaming::analyze_composed(&f, &MaUnflagged).unwrap_err();
    assert!(
        matches!(err, StreamError::UnsupportedCall(ref m) if m.contains("no stream")),
        "expected no-stream sub-call error, got: {err}"
    );
    assert_eq!(
        classifier_verdict(&f, &MaUnflagged),
        "refused: unsupported call in steady loop: composed sub-call `ma` has no stream"
    );
}

#[test]
fn stochrsi_derives_loopless_composed_plan() {
    // STOCHRSI has no producer loop: a pure sub-call pipeline
    // rsi(inReal) -> tempRSIBuffer, then stochf(tempRSIBuffer x3) -> outFastK/D.
    let f = load("stochrsi");
    assert!(f.streaming, "stochrsi.yaml must carry the stream flag");
    let plan = streaming::validate_streamable(&f, lookup()).expect("STOCHRSI derives a plan");
    let streaming::StreamPlan::Composed(cp) = plan else {
        panic!("STOCHRSI must derive a composed plan");
    };
    assert!(cp.producer.is_none(), "loopless pipeline: no producer loop");
    assert_eq!(cp.series, None);
    // The RSI buffer is a fresh malloc'd intermediate.
    assert_eq!(cp.intermediates, ["tempRSIBuffer"]);
    assert_eq!(cp.subs.len(), 2);
    assert_eq!(cp.subs[0].callee, "rsi");
    assert_eq!(cp.subs[0].srcs, ["inReal"]);
    assert_eq!(cp.subs[0].dsts, ["tempRSIBuffer"]);
    assert_eq!(cp.subs[1].callee, "stochf");
    assert_eq!(cp.subs[1].srcs, ["tempRSIBuffer", "tempRSIBuffer", "tempRSIBuffer"]);
    assert_eq!(cp.subs[1].dsts, ["outFastK", "outFastD"]);
    // The bare `free(tempRSIBuffer)` is captured as the replayable series free.
    assert_eq!(cp.series_frees.len(), 1);
    // Both steps are sub-calls (no producer transition, no combine map).
    assert_eq!(cp.steps.len(), 2);
    assert!(matches!(cp.steps[0], streaming::UpdateStep::Sub { sub_idx: 0 }));
    assert!(matches!(cp.steps[1], streaming::UpdateStep::Sub { sub_idx: 1 }));
}

#[test]
fn stddev_derives_loopless_composed_plan() {
    // STDDEV = var(inReal) -> outReal in place, then a param-selected sqrt
    // combine map (optInNbDev != 1.0 scales; otherwise plain sqrt).
    let f = load("stddev");
    assert!(f.streaming, "stddev.yaml must carry the stream flag");
    let plan = streaming::validate_streamable(&f, lookup()).expect("STDDEV derives a plan");
    let streaming::StreamPlan::Composed(cp) = plan else {
        panic!("STDDEV must derive a composed plan");
    };
    assert!(cp.producer.is_none(), "loopless pipeline: no producer loop");
    assert!(cp.intermediates.is_empty(), "var writes the output in place");
    assert_eq!(cp.subs.len(), 1);
    assert_eq!(cp.subs[0].callee, "var");
    assert_eq!(cp.subs[0].srcs, ["inReal"]);
    assert_eq!(cp.subs[0].dsts, ["outReal"]);
    // var sub-call, then the sqrt combine map.
    assert_eq!(cp.steps.len(), 2);
    assert!(matches!(cp.steps[0], streaming::UpdateStep::Sub { sub_idx: 0 }));
    assert!(matches!(cp.steps[1], streaming::UpdateStep::Map { .. }));
    // The map is temp-free. Since #243 the combine is `sqrt(outReal[i])` (scaled
    // by optInNbDev when it is not 1.0) with nothing to hold the radicand in: the
    // TA_EPSILON test that needed a step-local is gone, because var now owns the
    // dead-zone. The map_temps machinery is pinned elsewhere, by
    // ppo_derives_composed_plan_with_division_map, whose guarded division still
    // carries one -- so this is a shape assertion, not lost coverage.
    assert!(cp.map_temps.is_empty(), "STDDEV's combine map declares no step-local");
    // No heap series -> no replayable free needed.
    assert!(cp.series_frees.is_empty());
}

/* ---- Streamable-source-form guiding errors (G1 / G2) ----
 *
 * These pin the two "here's the fix" errors the analyzer hands a TA author who
 * writes a combine in a non-streamable form. They are dev-experience infra,
 * not APO/PPO plumbing: any future two-MA combine (BBANDS, …) gets guided
 * through the same two fixes. Each fixture is a genuinely non-conforming body
 * (real APO metadata, hand-written source), so the error is proven to fire on
 * real source, never vacuously. */

/// Load a function's real YAML metadata but wire a hand-written source body —
/// lets a test exercise a non-conforming shape without a fake input tree entry.
fn load_with_source(name: &str, source: &str) -> FuncDef {
    let dir = input_dir().join(name);
    let mut func = parser::yaml::parse_yaml(&dir.join(format!("{name}.yaml")));
    let parsed = parser::c_source::parse_c_source_str(source);
    parser::c_source::wire_parsed_source(&mut func, &parsed);
    func
}

#[test]
fn g2_success_guard_subcall_guides_to_error_guard() {
    // The pre-Flat-B APO shape: the slow-MA sub-call sits inside an
    // `if (retCode == TA_SUCCESS) { ... }` success-guard. G2 must name the
    // fix — flatten to a top-level `if (rc != TA_SUCCESS) return rc;`.
    let src = r#"
TA_RetCode apo( int startIdx, int endIdx,
   const double inReal[],
   int optInFastPeriod, int optInSlowPeriod, TA_MAType optInMAType,
   int *outBegIdx, int *outNBElement, double outReal[] )
{
   double *tempBuffer;
   TA_RetCode retCode;
   int outBegIdx1, outNbElement1;
   int outBegIdx2, outNbElement2;

   tempBuffer = malloc((endIdx-startIdx+1) * sizeof(double));
   if( !tempBuffer )
      return TA_ALLOC_ERR;

   retCode = ma( startIdx, endIdx, inReal, optInFastPeriod, optInMAType,
      &outBegIdx2, &outNbElement2, tempBuffer );
   if( retCode == TA_SUCCESS )
   {
      retCode = ma( startIdx, endIdx, inReal, optInSlowPeriod, optInMAType,
         &outBegIdx1, &outNbElement1, outReal );
   }
   free(tempBuffer);
   return retCode;
}
"#;
    let f = load_with_source("apo", src);
    let err = streaming::analyze_composed(&f, lookup()).unwrap_err();
    assert!(
        matches!(err, StreamError::Unsupported(ref m)
            if m.contains("success-guard") && m.contains("flatten")),
        "G2 must guide to the error-guard flatten, got: {err}"
    );
    assert_classifier_verdict(
        &f,
        "refused: sub-call nested inside an `if (rc == TA_SUCCESS) { ... }` success-guard; flatten to a \
         top-level `if (rc != TA_SUCCESS) return rc;` error-guard (see STDDEV)",
        "",
    );
}

#[test]
fn g1_multi_cursor_combine_loop_guides_to_single_cursor() {
    // Flattened guards (G2 satisfied) but the combine is still a two-cursor
    // `for (i=0, j=offset; ...; i++, j++)` loop. G1 must name the fix — fold
    // the second cursor into a single-cursor begIdx-offset index.
    let src = r#"
TA_RetCode apo( int startIdx, int endIdx,
   const double inReal[],
   int optInFastPeriod, int optInSlowPeriod, TA_MAType optInMAType,
   int *outBegIdx, int *outNBElement, double outReal[] )
{
   double *tempBuffer;
   TA_RetCode retCode;
   int fastBeg, fastNb;
   int offset;
   int i, j;

   tempBuffer = malloc((endIdx-startIdx+1) * sizeof(double));
   if( !tempBuffer )
      return TA_ALLOC_ERR;

   retCode = ma( startIdx, endIdx, inReal, optInFastPeriod, optInMAType,
      &fastBeg, &fastNb, tempBuffer );
   if( retCode != TA_SUCCESS )
   {
      free(tempBuffer);
      return retCode;
   }
   retCode = ma( startIdx, endIdx, inReal, optInSlowPeriod, optInMAType,
      outBegIdx, outNBElement, outReal );
   if( retCode != TA_SUCCESS )
   {
      free(tempBuffer);
      return retCode;
   }
   offset = *outBegIdx - fastBeg;
   for( i=0, j=offset; i < (int)*outNBElement; i++, j++ )
      outReal[i] = tempBuffer[j] - outReal[i];
   free(tempBuffer);
   return TA_SUCCESS;
}
"#;
    let f = load_with_source("apo", src);
    let err = streaming::analyze_composed(&f, lookup()).unwrap_err();
    assert!(
        matches!(err, StreamError::Unsupported(ref m) if m.contains("multi-cursor")),
        "G1 must guide to the single-cursor begIdx-offset form, got: {err}"
    );
    assert_classifier_verdict(
        &f,
        "refused: multi-cursor combine loop; rewrite as a single cursor with a begIdx-offset index (see APO)",
        "",
    );
}

#[test]
fn apo_derives_composed_plan_with_same_bar_offset_map() {
    // The shipped APO: fast MA -> tempBuffer, slow MA -> outReal, then the
    // combine map.
    let f = load("apo");
    let plan = streaming::validate_streamable(&f, lookup()).expect("APO derives a plan");
    let streaming::StreamPlan::Composed(cp) = plan else {
        panic!("APO must derive a composed plan");
    };
    assert!(cp.producer.is_none(), "loopless pipeline: no producer loop");
    assert_eq!(cp.intermediates, ["tempBuffer"]);
    assert_eq!(cp.subs.len(), 2);
    assert_eq!(cp.subs[0].callee, "ma");
    assert_eq!(cp.subs[0].srcs, ["inReal"]);
    assert_eq!(cp.subs[0].dsts, ["tempBuffer"]);
    assert_eq!(cp.subs[1].callee, "ma");
    assert_eq!(cp.subs[1].srcs, ["inReal"]);
    assert_eq!(cp.subs[1].dsts, ["outReal"]);
    // fast sub, slow sub, then the begIdx-offset combine map.
    assert_eq!(cp.steps.len(), 3);
    assert!(matches!(cp.steps[0], streaming::UpdateStep::Sub { sub_idx: 0 }));
    assert!(matches!(cp.steps[1], streaming::UpdateStep::Sub { sub_idx: 1 }));
    assert!(matches!(cp.steps[2], streaming::UpdateStep::Map { .. }));
    // The bare free(tempBuffer) is the replayable series free.
    assert_eq!(cp.series_frees.len(), 1);
}

#[test]
fn ppo_derives_composed_plan_with_division_map() {
    // PPO is APO plus the TA_IS_ZERO-guarded division; the combine map still
    // reads tempBuffer[i + offset] at the same bar and carries tempReal.
    let f = load("ppo");
    let plan = streaming::validate_streamable(&f, lookup()).expect("PPO derives a plan");
    let streaming::StreamPlan::Composed(cp) = plan else {
        panic!("PPO must derive a composed plan");
    };
    assert_eq!(cp.subs.len(), 2);
    assert_eq!(cp.subs[1].dsts, ["outReal"]);
    assert!(matches!(cp.steps[2], streaming::UpdateStep::Map { .. }));
    assert!(cp.map_temps.iter().any(|(n, _)| n == "tempReal"));
    // The dead-window counter. `windowed` selects the map variant: decided at
    // Open, it is state no map moves, as is `slowLookback`; only `zeroRun` moves
    // bar to bar, so only it is stored back per step.
    let state: Vec<(&str, bool)> =
        cp.map_state.iter().map(|st| (st.name.as_str(), st.carried)).collect();
    assert_eq!(state, [("slowLookback", false), ("windowed", false), ("zeroRun", true)]);
}

/// PPO's shipped source with `from` replaced by `to`, for the refusals below.
fn ppo_variant(from: &str, to: &str) -> FuncDef {
    let src = std::fs::read_to_string(input_dir().join("ppo").join("ppo.c")).expect("ppo.c");
    assert!(src.contains(from), "the PPO source no longer contains {from:?}");
    load_with_source("ppo", &src.replacen(from, to, 1))
}

fn assert_refused(f: &FuncDef, needle: &str) {
    match streaming::analyze_composed(f, lookup()) {
        Err(StreamError::Unsupported(m)) if m.contains(needle) => {}
        other => panic!("expected a refusal naming {needle:?}, got {other:?}"),
    }
}

/// What [`streaming::analyze_composed`] makes of `f`: the head of its refusal,
/// or its non-Zero edges.
fn classifier_verdict(f: &FuncDef, lk: &dyn streaming::CalleeLookup) -> String {
    match streaming::analyze_composed(f, lk) {
        Ok(cp) => format!("accepted {:?}", non_zero_edges(&cp)),
        Err(e) => {
            let m = e.to_string();
            format!("refused: {}", m.split(": it must be a same-bar shift").next().unwrap_or(&m))
        }
    }
}

/// Pins [`classifier_verdict`]; `why` is the hand derivation of an acceptance.
fn assert_classifier_verdict(f: &FuncDef, want: &str, why: &str) {
    assert_eq!(classifier_verdict(f, lookup()), want, "{why}");
}

/// Pins a refusal of the delay derivation: its head, and the guidance every
/// such refusal carries.
fn assert_delay_refused(f: &FuncDef, consumer: &str, buffer: &str, reason: &str) {
    assert_delay_refused_why(f, consumer, buffer, reason, "");
}

/// [`assert_delay_refused`], naming the row in a failure.
fn assert_delay_refused_why(f: &FuncDef, consumer: &str, buffer: &str, reason: &str, why: &str) {
    let head = format!("composed {consumer} reads `{buffer}` at a delay that is not a stream constant ({reason})");
    let row = if why.is_empty() { String::new() } else { format!("{why}: ") };
    match streaming::analyze_composed(f, lookup()) {
        Err(StreamError::Unsupported(m)) => assert!(
            m.starts_with(&head) && m.contains("same-bar shift") && m.contains("element-count difference"),
            "{row}want {head:?}, got {m:?}"
        ),
        Err(e) => panic!("{row}want {head:?}, got {e}"),
        Ok(cp) => panic!("{row}want {head:?}, accepted {:?}", non_zero_edges(&cp)),
    }
}

#[test]
fn composed_map_bar_input_reads_are_judged_by_their_delay() {
    let read = "fabs(inReal[*outBegIdx + i])";
    // inReal holds endIdx + 1 bars and the map runs n = endIdx - Bslow + 1 times,
    // so inReal[i] is Bslow bars behind its newest: startIdx once the begin is
    // presumed at its start argument.
    assert_delay_refused(
        &ppo_variant(read, "fabs(inReal[i])"),
        "map",
        "inReal",
        "pairs bars at a distance that depends on where the batch started",
    );
    // inReal[fastBeg + i] is Bslow - Bfast behind: 0 with both begins presumed at
    // startIdx.
    assert_classifier_verdict(
        &ppo_variant(read, "fabs(inReal[fastBeg + i])"),
        r#"accepted ["presumed lagzero.inReal.1.map0.r0 <- (inReal,1) map0.then inReal[fastBeg+i]"]"#,
        "loud at Open: presume check fails when the MA lookbacks differ",
    );
}

#[test]
fn a_map_read_delay_renders_from_the_whole_offset_round_the_cursor() {
    use streaming::LagTerm;
    let apo = |read: &str| {
        apo_with_tail(&format!(
            r"
   retCode = ma( startIdx, endIdx, inReal, optInFastPeriod, optInMAType,
      &fastBeg, &fastNb, tempBuffer );
   retCode = ma( startIdx, endIdx, inReal, optInSlowPeriod, optInMAType,
      outBegIdx, outNBElement, outReal );
   offset = fastNb - *outNBElement;
   for( i=0; i < (int)*outNBElement; i++ )
      outReal[i] = {read} - outReal[i];"
        ))
    };
    let f = apo("tempBuffer[i+offset-1]");
    assert_classifier_verdict(
        &f,
        r#"accepted ["tap lagdepth.tempBuffer.1.t0 ge1 Oldest <- (tempBuffer,1) map0 tempBuffer[i+offset-1]"]"#,
        "fastNb - *outNBElement - (fastNb - *outNBElement - 1) = 1",
    );
    let cp = streaming::analyze_composed(&f, lookup()).expect("accepted above");
    let tap = &cp.lines[0].taps[0];
    let depth = streaming::render_lag_formula(&cp.edge_delay_formula(tap.edge), &|t| match t {
        LagTerm::Nb(s) => format!("lagNb{s}"),
        LagTerm::Const(k) => k.to_string(),
        LagTerm::Batch(x) => show(x),
    });
    assert_eq!(depth, "lagNb0 - (*outNBElement) - offset + 1");
    // The Tracker reduces it to `i + offset`, but Open renders only a `+`/`-` chain
    // holding the cursor once.
    assert_classifier_verdict(
        &apo("tempBuffer[i+i-i+offset]"),
        "refused: composed map reads `tempBuffer` at a delay that is not a stream constant (index is not affine \
         in the cursor)",
        "",
    );
}

#[test]
fn composed_map_state_is_set_only_before_the_maps() {
    // Open would see a write that follows a map; a step never runs it.
    let warm = "   for( i = *outBegIdx - slowLookback; i < *outBegIdx; i++ )\n      \
                zeroRun = fabs(inReal[i]) <= 0.0 ? zeroRun + 1 : 0;\n";
    let src = std::fs::read_to_string(input_dir().join("ppo").join("ppo.c")).expect("ppo.c");
    assert!(src.contains(warm), "the PPO warm-up loop moved");
    let tail = "   free( tempBuffer );\n\n   return";
    let moved = src.replacen(warm, "", 1).replacen(tail, &format!("{warm}{tail}"), 1);
    assert_refused(&load_with_source("ppo", &moved), "after a map has run");
    assert_refused(
        &ppo_variant(tail, &format!("   zeroRun = 0;\n{tail}")),
        "after a map has run",
    );
    let late = "refused: composed tail sets map state `zeroRun` after a map has run: Open would see the \
                write and a step never would";
    assert_classifier_verdict(&load_with_source("ppo", &moved), late, "");
    assert_classifier_verdict(&ppo_variant(tail, &format!("   zeroRun = 0;\n{tail}")), late, "");
}

#[test]
fn composed_map_variant_selector_reads_only_what_the_handle_fixes() {
    let sel = "if( windowed != 0 )";
    // A value a map moves would select per bar in a step, once in batch.
    assert_refused(&ppo_variant(sel, "if( zeroRun != 0 )"), "which a map moves");
    // A signature name is not a handle constant.
    assert_refused(&ppo_variant(sel, "if( endIdx != 0 )"), "not param-pure");
    assert_classifier_verdict(
        &ppo_variant(sel, "if( zeroRun != 0 )"),
        "refused: composed map variant condition reads `zeroRun`, which a map moves: batch selects the \
         variant once, a step on every bar",
        "",
    );
    assert_classifier_verdict(
        &ppo_variant(sel, "if( endIdx != 0 )"),
        "refused: composed map variant condition is not param-pure",
        "",
    );
}

#[test]
fn composed_map_state_moves_bar_to_bar_in_one_map_only() {
    // Batch runs the first map over every bar before the second; a step runs
    // both on one bar, so the second would see a different count.
    let f = ppo_variant(
        "   free( tempBuffer );\n\n   return",
        "   for( i=0; i < (int)*outNBElement; i++ )\n   {\n      zeroRun = zeroRun + 1;\n      \
         outReal[i] = outReal[i] + 0.0 * zeroRun;\n   }\n   free( tempBuffer );\n\n   return",
    );
    assert_refused(&f, "more than one map");
    assert_classifier_verdict(
        &f,
        "refused: composed map state `zeroRun` moves bar to bar in more than one map",
        "",
    );
}

#[test]
fn adxr_derives_composed_plan_with_a_delay_line() {
    // outReal[k] = (adx[k+(period-1)] + adx[k])/2: the current ADX plus the ADX
    // from period-1 bars ago, a lag fixed by the parameters, so the plain-cursor
    // read is one tap of a delay line on the sub-call's output.
    use streaming::{Consumer, DefSite, EdgeClass, LagTerm, OpenAction, OpenAt, TapBack};
    use ta_codegen_lib::ir::Expr;
    let f = load("adxr");
    let plan = streaming::validate_streamable(&f, lookup()).expect("ADXR derives a plan");
    let streaming::StreamPlan::Composed(cp) = plan else {
        panic!("ADXR must derive a composed plan");
    };
    assert!(cp.producer.is_none(), "loopless pipeline");
    assert_eq!(cp.intermediates, ["adx"]);
    assert_eq!(cp.subs.len(), 1);
    assert_eq!(cp.subs[0].callee, "adx");
    // Multi-price direct feed: the three raw price inputs go straight to adx().
    assert_eq!(cp.subs[0].srcs, ["inHigh", "inLow", "inClose"]);
    assert_eq!(cp.subs[0].dsts, ["adx"]);

    assert_eq!(cp.lines.len(), 1, "one delay line");
    let line = &cp.lines[0];
    let def = &cp.defs[line.def];
    assert_eq!((def.buffer.as_str(), def.ordinal), ("adx", 1));
    assert_eq!(def.site, DefSite::Sub { sub_idx: 0, out: 0 });
    assert!(matches!(&def.count_open, Some(Expr::PointerDeref(p)) if p == "outNBElement"));
    assert_eq!(line.taps.len(), 1);
    let tap = &line.taps[0];
    assert!(tap.ge1, "period - 1 >= 1 for every accepted period");
    assert_eq!(tap.back, TapBack::Oldest);
    assert_eq!(tap.key, "lagdepth.adx.1.t0");
    let edge = &cp.edges[tap.edge];
    assert_eq!(edge.class, EdgeClass::Tap { line: 0, tap: 0 });
    let Consumer::MapRead { step, arm: None, buffer, index } = &edge.consumer else {
        panic!("the tap is a top-level map read: {:?}", edge.consumer);
    };
    assert!(matches!(cp.steps[*step], streaming::UpdateStep::Map { .. }));
    assert_eq!(buffer, "adx");
    assert!(matches!(index, Expr::Var(v) if v == "outIdx"), "the plain-cursor read: {index:?}");

    // Depth captured at Open from the exact counts, never from the parameter.
    let terms = cp.edge_delay_formula(tap.edge).terms;
    assert!(
        matches!(terms.as_slice(), [(1, LagTerm::Nb(0)), (-1, LagTerm::Batch(Expr::Var(t)))] if t == "nbElement"),
        "depth = lagNb0 - nbElement: {terms:?}"
    );

    assert_eq!(cp.snapshots, [line.def], "the line's count is lagNb0");
    let call = cp.subs[0].tail_idx;
    let kinds: Vec<String> = cp
        .open_actions
        .iter()
        .map(|(at, a)| {
            let what = match a {
                OpenAction::Snapshot { def } => format!("snapshot ({},{})", cp.defs[*def].buffer, cp.defs[*def].ordinal),
                OpenAction::EmptyGuard { count } => format!("empty {count:?}"),
                OpenAction::Capture { line, tap } => format!("capture {line}.{tap}"),
                OpenAction::Seed { line } => format!("seed {line}"),
                other => format!("{other:?}"),
            };
            let at = match at {
                OpenAt::AfterStmt(i) => format!("after {}", i - call),
                OpenAt::BeforeStmt(i) => format!("before {}", i - call),
                other => format!("{other:?}"),
            };
            format!("{at}: {what}")
        })
        .collect();
    // The snapshot follows the call's returning guard; the checks and the seed
    // precede the map that reads the tap.
    let map_tail = match cp.steps[*step] {
        streaming::UpdateStep::Map { tail_idx } => tail_idx - call,
        _ => unreachable!("checked"),
    };
    assert_eq!(
        kinds,
        [
            "after 1: snapshot (adx,1)".to_string(),
            format!("before {map_tail}: empty Var(\"nbElement\")"),
            format!("before {map_tail}: capture 0.0"),
            format!("before {map_tail}: seed 0"),
        ]
    );
}

/// A BBANDS body whose first memmove is followed by `tail`.
fn bbands_with_tail(tail: &str) -> FuncDef {
    load_with_source("bbands", &bbands_source(tail))
}

fn bbands_source(tail: &str) -> String {
    format!(
        r#"
TA_RetCode bbands( int startIdx, int endIdx, const double inReal[],
   int optInTimePeriod, double optInNbDevUp, double optInNbDevDn, TA_MAType optInMAType,
   int *outBegIdx, int *outNBElement,
   double outRealUpperBand[], double outRealMiddleBand[], double outRealLowerBand[] )
{{
   double *tmp;
   TA_RetCode retCode;
   int b1, n, b2, n2;
   int i;

   tmp = malloc((endIdx-startIdx+1) * sizeof(double));
   retCode = sma( startIdx, endIdx, inReal, optInTimePeriod, &b1, &n, tmp );
   if( retCode != TA_SUCCESS )
   {{
      free( tmp );
      return retCode;
   }}
   memmove( outRealUpperBand, &tmp[0], n * sizeof(double) );
{tail}
   free( tmp );
   *outBegIdx = b1;
   *outNBElement = n;
   return TA_SUCCESS;
}}
"#
    )
}

fn align_copies(f: &FuncDef) -> Vec<bool> {
    let cp = streaming::analyze_composed(f, lookup()).unwrap_or_else(|e| panic!("accepted: {e}"));
    cp.steps
        .iter()
        .filter_map(|s| match s {
            streaming::UpdateStep::Align { copy, .. } => Some(*copy),
            _ => None,
        })
        .collect()
}

#[test]
fn align_whose_source_a_later_call_redefines_copies() {
    let f = bbands_with_tail(
        r"
   retCode = ema( startIdx, endIdx, inReal, optInTimePeriod, &b2, &n2, tmp );
   if( retCode != TA_SUCCESS )
   {
      free( tmp );
      return retCode;
   }
   for( i=0; i<n; i++ )
   {
      outRealMiddleBand[i] = tmp[i];
      outRealLowerBand[i] = tmp[i];
   }",
    );
    assert_eq!(align_copies(&f), [true]);
}

#[test]
fn align_whose_destination_a_later_map_writes_copies() {
    let f = bbands_with_tail(
        r"
   for( i=0; i<n; i++ )
   {
      outRealUpperBand[i] = outRealUpperBand[i] * 2.0;
      outRealMiddleBand[i] = tmp[i] + 1.0;
      outRealLowerBand[i] = tmp[i];
   }",
    );
    assert_eq!(align_copies(&f), [true]);
    // The copy gives the destination its own scalar, so the map's write leaves
    // the source the middle band reads unchanged.
    let c = ta_codegen_lib::backends::c_stream::generate(
        &f,
        &crate::common::load_enums(),
        lookup(),
        crate::common::make_helpers(),
    );
    let step = &c[c.find("TA_BBANDS_StepImpl(").expect("step")..];
    let step = &step[..step.find("\n}").expect("step end")];
    let copy = step.find("cur_outRealUpperBand = cur_tmp;").expect("the copy");
    let write = step.find("cur_outRealUpperBand = cur_outRealUpperBand * 2.0;").expect("the write");
    assert!(copy < write, "{step}");
    assert!(step.contains("cur_outRealMiddleBand = cur_tmp + 1.0;"), "{step}");
}

/// A tail over the first memmove that nothing later writes.
const BBANDS_READ_ONLY_TAIL: &str = r"
   for( i=0; i<n; i++ )
   {
      outRealMiddleBand[i] = tmp[i] + 1.0;
      outRealLowerBand[i] = tmp[i];
   }";

#[test]
fn align_nothing_later_writes_aliases() {
    assert_eq!(align_copies(&bbands_with_tail(BBANDS_READ_ONLY_TAIL)), [false]);
}

/// Every lag tap of a composed plan as `(definition, consumer)`. The definition is
/// `buffer,ordinal`, the ordinal counting writes to that buffer in body order from 1;
/// the consumer ends in ` ge1` when the tap is proved at least one bar deep.
fn derived_taps(cp: &streaming::ComposedPlan<'_>) -> Vec<(String, String)> {
    use streaming::Consumer;
    let mut out = Vec::new();
    for line in &cp.lines {
        let def = &cp.defs[line.def];
        for tap in &line.taps {
            let what = match &cp.edges[tap.edge].consumer {
                Consumer::SubSrc { sub_idx, input } => format!("sub {sub_idx} source {input}"),
                Consumer::Align { step } => format!("align at step {step}"),
                Consumer::MapRead { buffer, index, .. } => format!("map read of {buffer}[{index:?}]"),
                Consumer::MapPrior { buffer, .. } => format!("map prior of {buffer}"),
                Consumer::TailScalar { tail_idx } => format!("tail scalar at {tail_idx}"),
                Consumer::Return { output } => format!("return of {output}"),
            };
            let ge1 = if tap.ge1 { " ge1" } else { "" };
            out.push((format!("{},{}", def.buffer, def.ordinal), format!("{what}{ge1}")));
        }
    }
    out
}

#[test]
fn gator_start_form_derives_four_lagged_reads() {
    // Equal shifts give a leg a read-back of zero, so no tap is provably >= 1.
    let f = load("gator");
    let Ok(streaming::StreamPlan::Composed(cp)) = streaming::validate_streamable(&f, lookup()) else {
        panic!("GATOR must derive a composed plan");
    };
    let taps = derived_taps(&cp);
    let defs: std::collections::BTreeSet<&str> = taps.iter().map(|(d, _)| d.as_str()).collect();
    assert_eq!(
        defs,
        ["tempA,1", "tempA,2", "tempB,1", "tempB,2"].into_iter().collect(),
        "every leg buffer definition is read through a tap: {taps:?}"
    );
    assert!(taps.iter().all(|(_, c)| !c.ends_with(" ge1")), "no GATOR tap is ge1: {taps:?}");
}

#[test]
fn gator_end_form_taps_the_median_price_once_per_leg() {
    // The form whose legs end early: each rma ends `back` bars before tempMP's
    // newest element, so it reads tempMP `back` bars behind, exact, and 0 for one
    // leg of each pair; four depths with no fixed difference share one line. Each
    // leg then holds nbElement elements, a 0 the prover sees only by presuming each
    // begin at its start argument.
    let mut src = std::fs::read_to_string(input_dir().join("gator/gator.c")).expect("gator.c");
    let edits = [
        ("   int i, nbElement, maxBack;", "   int i, nbElement;"),
        (
            "   maxBack = backJaw + backTeethUpper;\n   if( backTeethLower + backLips > maxBack )\n      \
             maxBack = backTeethLower + backLips;\n\n",
            "",
        ),
        ("malloc((nbElement+maxBack) * sizeof(double));\n   if( !tempA )", "malloc(nbElement * sizeof(double));\n   if( !tempA )"),
        ("malloc((nbElement+maxBack) * sizeof(double));\n   if( !tempB )", "malloc(nbElement * sizeof(double));\n   if( !tempB )"),
        ("localStart-backJaw, localEnd,", "localStart-backJaw, localEnd-backJaw,"),
        ("localStart-backTeethUpper, localEnd,", "localStart-backTeethUpper, localEnd-backTeethUpper,"),
        ("localStart-backTeethLower, localEnd,", "localStart-backTeethLower, localEnd-backTeethLower,"),
        ("localStart-backLips, localEnd,", "localStart-backLips, localEnd-backLips,"),
    ];
    for (from, to) in edits {
        assert_eq!(src.matches(from).count(), 1, "the end-form edit {from:?} does not apply once");
        src = src.replacen(from, to, 1);
    }
    let want = [
        "tap lagdepth.tempMP.1.t0 Field <- (tempMP,1) sub1.in0",
        "tap lagdepth.tempMP.1.t1 Field <- (tempMP,1) sub2.in0",
        "presumed lagzero.tempA.1.map0.r0 <- (tempA,1) map0 tempA[i]",
        "presumed lagzero.tempB.1.map0.r0 <- (tempB,1) map0 tempB[i]",
        "tap lagdepth.tempMP.1.t2 Field <- (tempMP,1) sub3.in0",
        "tap lagdepth.tempMP.1.t3 Field <- (tempMP,1) sub4.in0",
        "presumed lagzero.tempA.2.map1.r0 <- (tempA,2) map1 tempA[i]",
        "presumed lagzero.tempB.2.map1.r0 <- (tempB,2) map1 tempB[i]",
    ];
    assert_classifier_verdict(&load_with_source("gator", &src), &format!("accepted {want:?}"), "the end form");
}

#[test]
fn apo_fast_ma_started_early_derives_a_tap() {
    // Only the start arguments say three: a clamp to either lookback moves it, so the
    // depth is not provably >= 1.
    let f = apo_with_tail(
        r"
   retCode = ma( startIdx-3, endIdx, inReal, optInFastPeriod, optInMAType,
      &fastBeg, &fastNb, tempBuffer );
   retCode = ma( startIdx, endIdx, inReal, optInSlowPeriod, optInMAType,
      outBegIdx, outNBElement, outReal );
   for( i=0; i < (int)*outNBElement; i++ )
      outReal[i] = tempBuffer[i] - outReal[i];",
    );
    let cp = streaming::analyze_composed(&f, lookup())
        .unwrap_or_else(|e| panic!("an early-started fast average streams through a tap: {e}"));
    let taps = derived_taps(&cp);
    assert_eq!(
        taps.iter().map(|(d, _)| d.as_str()).collect::<Vec<_>>(),
        ["tempBuffer,1"],
        "one tap, on the fast average: {taps:?}"
    );
    assert!(!taps[0].1.ends_with(" ge1"), "the tap is not ge1: {taps:?}");
}

#[test]
fn a_tapped_align_is_a_copy_to_every_later_decision() {
    // The second memmove reads tmp two bars back, so it copies its tap, and the map's
    // in-place write lands on that copy: nothing later writes tmp, so the first
    // memmove may alias it.
    let src = r#"
TA_RetCode bbands( int startIdx, int endIdx, const double inReal[],
   int optInTimePeriod, double optInNbDevUp, double optInNbDevDn, TA_MAType optInMAType,
   int *outBegIdx, int *outNBElement,
   double outRealUpperBand[], double outRealMiddleBand[], double outRealLowerBand[] )
{
   double *tmp;
   TA_RetCode retCode;
   int b1, n, nb;
   int i;

   nb = endIdx - startIdx + 1;
   tmp = malloc((nb+2) * sizeof(double));
   retCode = sma( startIdx-2, endIdx, inReal, optInTimePeriod, &b1, &n, tmp );
   if( retCode != TA_SUCCESS )
   {
      free( tmp );
      return retCode;
   }
   memmove( outRealMiddleBand, &tmp[2], nb * sizeof(double) );
   memmove( outRealUpperBand, &tmp[0], nb * sizeof(double) );
   for( i=0; i<nb; i++ )
   {
      outRealUpperBand[i] = outRealUpperBand[i] * 2.0;
      outRealLowerBand[i] = outRealMiddleBand[i];
   }
   free( tmp );
   *outBegIdx = startIdx;
   *outNBElement = nb;
   return TA_SUCCESS;
}
"#;
    let f = load_with_source("bbands", src);
    let cp = streaming::analyze_composed(&f, lookup()).unwrap_or_else(|e| panic!("accepted: {e}"));
    let si = cp.steps.iter().rposition(|s| matches!(s, streaming::UpdateStep::Align { .. })).expect("align");
    assert!(cp.align_tap(si).is_some(), "the second memmove reads a tap: {:?}", non_zero_edges(&cp));
    assert_eq!(align_copies(&f), [false, true]);
}

#[test]
fn an_open_check_never_reads_a_reassigned_parameter() {
    let src = std::fs::read_to_string(input_dir().join("adxr").join("adxr.c")).expect("adxr.c");
    let (bound, inline) = (
        "   nbElement = *outNBElement - (optInTimePeriod-1);\n   for( outIdx = 0; outIdx < nbElement; outIdx++ )",
        "   for( outIdx = 0; outIdx < *outNBElement - (optInTimePeriod-1); outIdx++ )",
    );
    assert!(src.contains(bound), "the ADXR map moved");
    let inlined = src.replacen(bound, inline, 1).replacen(
        "   *outNBElement = nbElement;",
        "   *outNBElement = *outNBElement - (optInTimePeriod-1);",
        1,
    );
    // The depth check reads the map bound, which names the period.
    streaming::analyze_composed(&load_with_source("adxr", &inlined), lookup())
        .unwrap_or_else(|e| panic!("the inlined bound composes: {e}"));
    let reassigned = inlined.replacen(inline, &format!("   optInTimePeriod = optInTimePeriod + 0;\n{inline}"), 1);
    assert_refused(
        &load_with_source("adxr", &reassigned),
        "Open check reads parameter `optInTimePeriod` after the body reassigns it",
    );
    // The check is positional: the map's EmptyGuard and Capture are the last
    // Open checks reading the period, so a reassignment after the map leaves
    // each reading the caller's value.
    let after_map = "   free(adx);\n\n   *outBegIdx";
    assert!(inlined.contains(after_map), "the ADXR tail moved");
    let late = inlined.replacen(after_map, &format!("   optInTimePeriod = optInTimePeriod + 0;\n{after_map}"), 1);
    streaming::analyze_composed(&load_with_source("adxr", &late), lookup())
        .unwrap_or_else(|e| panic!("a reassignment after the last check composes: {e}"));
}

#[test]
fn data_dependent_lag_offset_rejected() {
    // adx[outIdx + runtimeLag] is 0 bars behind (the map bound subtracts the same
    // runtimeLag), but adx[outIdx] is runtimeLag = *outNBElement / 2 behind: a
    // quotient the Tracker does not model, fixed by no parameter.
    let src = r#"
TA_RetCode adxr( int startIdx, int endIdx,
   const double inHigh[], const double inLow[], const double inClose[],
   int optInTimePeriod,
   int *outBegIdx, int *outNBElement, double outReal[] )
{
   double *adx;
   int outIdx, nbElement, runtimeLag;
   TA_RetCode retCode;

   adx = malloc((endIdx-startIdx+optInTimePeriod) * sizeof(double));
   if( !adx )
      return TA_ALLOC_ERR;
   retCode = adx( startIdx-(optInTimePeriod-1), endIdx, inHigh, inLow, inClose,
      optInTimePeriod, outBegIdx, outNBElement, adx );
   if( retCode != TA_SUCCESS )
   {
      free(adx);
      return retCode;
   }
   runtimeLag = *outNBElement / 2;
   nbElement = *outNBElement - runtimeLag;
   for( outIdx = 0; outIdx < nbElement; outIdx++ )
      outReal[outIdx] = (adx[outIdx + runtimeLag] + adx[outIdx]) / 2.0;
   free(adx);
   *outBegIdx = startIdx;
   *outNBElement = nbElement;
   return TA_SUCCESS;
}
"#;
    let f = load_with_source("adxr", src);
    assert_delay_refused(&f, "map", "adx", "depends on data or an unmodelled value");
}

#[test]
fn begidx_offset_form_rejected_steers_to_count_difference() {
    // The begIdx difference `*outBegIdx - fastBeg` is the same VALUE as the
    // element-count difference APO ships, but it underflows as a Rust `usize`
    // when the slow MA is empty (0 - fastBeg). The analyzer refuses it and
    // points at the count-difference form rather than blessing a form that
    // panics in Rust debug builds.
    let src = r#"
TA_RetCode apo( int startIdx, int endIdx,
   const double inReal[],
   int optInFastPeriod, int optInSlowPeriod, TA_MAType optInMAType,
   int *outBegIdx, int *outNBElement, double outReal[] )
{
   double *tempBuffer;
   TA_RetCode retCode;
   int fastBeg, fastNb;
   int offset;
   int i;

   tempBuffer = malloc((endIdx-startIdx+1) * sizeof(double));
   if( !tempBuffer )
      return TA_ALLOC_ERR;

   retCode = ma( startIdx, endIdx, inReal, optInFastPeriod, optInMAType,
      &fastBeg, &fastNb, tempBuffer );
   if( retCode != TA_SUCCESS )
   {
      free(tempBuffer);
      return retCode;
   }
   retCode = ma( startIdx, endIdx, inReal, optInSlowPeriod, optInMAType,
      outBegIdx, outNBElement, outReal );
   if( retCode != TA_SUCCESS )
   {
      free(tempBuffer);
      return retCode;
   }
   offset = *outBegIdx - fastBeg;
   for( i=0; i < (int)*outNBElement; i++ )
      outReal[i] = tempBuffer[i+offset] - outReal[i];
   free(tempBuffer);
   return TA_SUCCESS;
}
"#;
    let f = load_with_source("apo", src);
    assert_delay_refused(&f, "map", "tempBuffer", "subtracts a begin index with no guard proving it non-negative");
}

#[test]
fn an_endidx_lagged_slow_ma_taps_its_source() {
    let src = r#"
TA_RetCode apo( int startIdx, int endIdx,
   const double inReal[],
   int optInFastPeriod, int optInSlowPeriod, TA_MAType optInMAType,
   int *outBegIdx, int *outNBElement, double outReal[] )
{
   double *tempBuffer;
   TA_RetCode retCode;
   int fastBeg, fastNb;
   int offset;
   int i;

   tempBuffer = malloc((endIdx-startIdx+1) * sizeof(double));
   if( !tempBuffer )
      return TA_ALLOC_ERR;

   retCode = ma( startIdx, endIdx, inReal, optInFastPeriod, optInMAType,
      &fastBeg, &fastNb, tempBuffer );
   if( retCode != TA_SUCCESS )
   {
      free(tempBuffer);
      return retCode;
   }
   retCode = ma( startIdx, endIdx-1, inReal, optInSlowPeriod, optInMAType,
      outBegIdx, outNBElement, outReal );
   if( retCode != TA_SUCCESS )
   {
      free(tempBuffer);
      return retCode;
   }
   offset = fastNb - *outNBElement;
   for( i=0; i < (int)*outNBElement; i++ )
      outReal[i] = tempBuffer[i+offset] - outReal[i];
   free(tempBuffer);
   return TA_SUCCESS;
}
"#;
    let f = load_with_source("apo", src);
    assert_classifier_verdict(
        &f,
        r#"accepted ["tap lagdepth.inReal.1.t0 ge1 Oldest <- (inReal,1) sub1.in0"]"#,
        "the slow ma's last source bar is endIdx-1: delay (endIdx+1) - 1 - (endIdx-1) = 1. Its \
         output then ends a bar early, which the offset absorbs: the map reads are 0 bars behind",
    );
}

/// APO's metadata over two scratch series and a hand-written sub-call tail; the
/// spare locals are for the tails below.
fn apo_with_tail(tail: &str) -> FuncDef {
    load_with_source("apo", &apo_source(tail))
}

fn apo_source(tail: &str) -> String {
    format!(
        r#"
TA_RetCode apo( int startIdx, int endIdx, const double inReal[],
   int optInFastPeriod, int optInSlowPeriod, TA_MAType optInMAType,
   int *outBegIdx, int *outNBElement, double outReal[] )
{{
   double *tempBuffer;
   double *tempBuffer2;
   double *res;
   TA_RetCode retCode;
   int fastBeg, fastNb;
   int offset;
   int nb, backFast, backSlow;
   double last;
   int i;

   tempBuffer = malloc((endIdx-startIdx+1) * sizeof(double));
   tempBuffer2 = malloc((endIdx-startIdx+1) * sizeof(double));
{tail}
   free(tempBuffer);
   free(tempBuffer2);
   return TA_SUCCESS;
}}
"#
    )
}

/// APO's shipped tail over [`apo_with_tail`]'s scratch series.
const APO_TAIL: &str = r"
   retCode = ma( startIdx, endIdx, inReal, optInFastPeriod, optInMAType,
      &fastBeg, &fastNb, tempBuffer );
   retCode = ma( startIdx, endIdx, inReal, optInSlowPeriod, optInMAType,
      outBegIdx, outNBElement, outReal );
   offset = fastNb - *outNBElement;
   for( i=0; i < (int)*outNBElement; i++ )
      outReal[i] = tempBuffer[i+offset] - outReal[i];";

/// [`APO_TAIL`] with each `(from, to)` edit applied once.
fn apo_tail_with(edits: &[(&str, &str)]) -> FuncDef {
    let mut tail = APO_TAIL.to_string();
    for (from, to) in edits {
        assert!(tail.contains(from), "fixture edit `{from}` does not apply");
        tail = tail.replacen(from, to, 1);
    }
    apo_with_tail(&tail)
}

#[test]
fn apo_tail_reads_every_series_at_its_newest_element() {
    // nb(fast) = endIdx - Bf + 1, nb(slow) = endIdx - Bs + 1 and offset = Bs - Bf,
    // so tempBuffer[i+offset] is nb(fast) - nb(slow) - offset = 0 behind; outReal's
    // prior and return are its own count.
    assert_classifier_verdict(&apo_tail_with(&[]), "accepted []", "every delay cancels exactly");
}

/// The fast average's count receiver is rewritten by a later call ending a bar
/// early, before the offset reads it.
fn count_receiver_rewritten() -> FuncDef {
    apo_with_tail(
        r"
   retCode = ma( startIdx, endIdx, inReal, optInFastPeriod, optInMAType,
      &fastBeg, &fastNb, tempBuffer );
   retCode = ma( startIdx, endIdx, inReal, optInSlowPeriod, optInMAType,
      outBegIdx, outNBElement, outReal );
   retCode = ma( startIdx, endIdx-1, inReal, optInSlowPeriod, optInMAType,
      &fastBeg, &fastNb, tempBuffer2 );
   offset = fastNb - *outNBElement;
   for( i=0; i < (int)*outNBElement; i++ )
      outReal[i] = tempBuffer[i+offset] - outReal[i];",
    )
}

#[test]
fn a_count_receiver_rewritten_after_its_series_taps_both_reads() {
    assert_classifier_verdict(
        &count_receiver_rewritten(),
        r#"accepted ["tap lagdepth.inReal.1.t0 ge1 Oldest <- (inReal,1) sub2.in0", "tap lagdepth.tempBuffer.1.t0 Oldest <- (tempBuffer,1) map0 tempBuffer[i+offset]"]"#,
        "the third ma reads inReal only up to endIdx-1: delay (endIdx+1) - 1 - (endIdx-1) = 1, exact, \
         ge1. offset = nb(tempBuffer2) - nb(outReal) = (endIdx - B2) - (endIdx - B1 + 1), so \
         tempBuffer[i+offset] is (endIdx - B0 + 1) - (endIdx - B1 + 1) - offset = B2 - B0 + 1 behind: \
         1 with both begins presumed at startIdx, a tap that is not ge1",
    );
}

#[test]
fn a_capture_floors_at_one_only_for_a_ge1_tap() {
    let f = count_receiver_rewritten();
    let cp = streaming::analyze_composed(&f, lookup()).expect("accepted");
    let floors: Vec<(String, i32)> = cp
        .open_actions
        .iter()
        .filter_map(|(_, a)| match a {
            streaming::OpenAction::Capture { line, tap } => {
                let def = &cp.defs[cp.lines[*line].def];
                Some((format!("{},{}", def.buffer, def.ordinal), cp.lines[*line].taps[*tap].capture_floor()))
            }
            _ => None,
        })
        .collect();
    assert_eq!(floors, [("inReal,1".to_string(), 1), ("tempBuffer,1".to_string(), 0)]);
}

#[test]
fn an_endidx_argument_its_own_call_rewrites_taps_the_next_source() {
    // Both later calls spell `fastNb-1`, but the first reads tempBuffer's count
    // and the second tempBuffer2's.
    let f = apo_with_tail(
        r"
   retCode = ma( startIdx, endIdx, inReal, optInFastPeriod, optInMAType,
      &fastBeg, &fastNb, tempBuffer );
   retCode = ma( 0, fastNb-1, tempBuffer, optInSlowPeriod, optInMAType,
      &fastBeg, &fastNb, tempBuffer2 );
   retCode = ma( 0, fastNb-1, tempBuffer, optInSlowPeriod, optInMAType,
      outBegIdx, outNBElement, outReal );
   offset = fastNb - *outNBElement;
   for( i=0; i < (int)*outNBElement; i++ )
      outReal[i] = tempBuffer2[i+offset] - outReal[i];",
    );
    assert_classifier_verdict(
        &f,
        r#"accepted ["tap lagdepth.tempBuffer.1.t0 Oldest <- (tempBuffer,1) sub2.in0"]"#,
        "the second ma starts at 0, at or before its lookback LB = ma_lookback(slow), so it begins at LB \
         and rewrites fastNb to nb(tempBuffer) - LB; the third reads tempBuffer up to nb(tempBuffer) - LB \
         - 1: LB bars behind, exact, and LB is only known to be at least 0, so not ge1. The map reads \
         tempBuffer2 at nb(tempBuffer2) - nb(outReal) - offset = 0",
    );
}

/* ---- #431: sub-outputs over different index spaces, both ending on endIdx ----
 * CRSI's shape without its error paths. */
const END_ALIGNED: &str = r#"
TA_RetCode crsi( int startIdx, int endIdx, const double inReal[],
   int optInTimePeriod, int optInStreakPeriod, int optInRankPeriod,
   int *outBegIdx, int *outNBElement, double outReal[] )
{
   double *tempRSI;
   double *tempStreak;
   double *tempRank;
   TA_RetCode retCode;
   int lookbackTotal, anchorIdx, today, outIdx, i;
   int offsetRSI, offsetStreak;
   int tempBegIdx, rsiNb, streakNb, rocNb;
   double prevClose, close, streak;

   *outBegIdx = 0;
   *outNBElement = 0;
   lookbackTotal = crsi_lookback( optInTimePeriod, optInStreakPeriod, optInRankPeriod );
   if( lookbackTotal > endIdx )
      return TA_SUCCESS;
   if( startIdx < lookbackTotal )
      startIdx = lookbackTotal;
   if( startIdx > endIdx )
      return TA_SUCCESS;
   anchorIdx = startIdx - lookbackTotal;
   tempStreak = malloc((endIdx-anchorIdx) * sizeof(double));
   tempRSI = malloc((endIdx-anchorIdx-rsi_lookback(optInTimePeriod)+1) * sizeof(double));
   tempRank = malloc((endIdx-startIdx+1+optInRankPeriod) * sizeof(double));

   streak = 0.0;
   prevClose = inReal[anchorIdx];
   outIdx = 0;
   today = anchorIdx+1;
   while( today <= endIdx )
   {
      close = inReal[today];
      if( close > prevClose )
         streak = (streak > 0.0) ? streak + 1.0 : 1.0;
      else if( close < prevClose )
         streak = (streak < 0.0) ? streak - 1.0 : -1.0;
      else
         streak = 0.0;
      tempStreak[outIdx++] = streak;
      prevClose = close;
      today++;
   }

   retCode = rsi( 0, outIdx-1, tempStreak, optInStreakPeriod,
      &tempBegIdx, &streakNb, tempStreak );
   retCode = rsi( anchorIdx+rsi_lookback(optInTimePeriod), endIdx, inReal, optInTimePeriod,
      &tempBegIdx, &rsiNb, tempRSI );
   retCode = rocp( startIdx-optInRankPeriod, endIdx, inReal, 1,
      &tempBegIdx, &rocNb, tempRank );
   retCode = percentrank( 0, rocNb-1, tempRank, optInRankPeriod,
      outBegIdx, outNBElement, outReal );

   offsetRSI = rsiNb - *outNBElement;
   offsetStreak = streakNb - *outNBElement;
   for( i=0; i < (int)*outNBElement; i++ )
      outReal[i] = ( tempRSI[i+offsetRSI] + tempStreak[i+offsetStreak] + outReal[i] ) / 3.0;

   free( tempStreak );
   free( tempRSI );
   free( tempRank );
   *outBegIdx = startIdx;
   return TA_SUCCESS;
}
"#;

/// [`END_ALIGNED`] with each `(from, to)` edit applied once; an edit that does
/// not apply fails the test instead of leaving it vacuous.
fn end_aligned_with(edits: &[(&str, &str)]) -> FuncDef {
    let mut src = END_ALIGNED.to_string();
    for (from, to) in edits {
        assert!(src.contains(from), "fixture edit `{from}` does not apply");
        src = src.replacen(from, to, 1);
    }
    load_with_source("crsi", &src)
}

#[test]
fn end_aligned_fixture_derives_a_composed_plan() {
    let f = end_aligned_with(&[]);
    let plan = streaming::validate_streamable(&f, lookup()).expect("the fixture derives a plan");
    let streaming::StreamPlan::Composed(cp) = plan else {
        panic!("the fixture must derive a composed plan");
    };
    assert_eq!(cp.series.as_deref(), Some("tempStreak"));
    let callees: Vec<&str> = cp.subs.iter().map(|s| s.callee.as_str()).collect();
    assert_eq!(callees, ["rsi", "rsi", "rocp", "percentrank"]);
    assert!(matches!(cp.steps.last(), Some(streaming::UpdateStep::Map { .. })));
}

// In the rows below the producer writes N = endIdx - anchorIdx elements, one per
// bar up to endIdx, and each offset is a count difference against percentrank's
// count n, which the map runs over.

#[test]
fn end_aligned_producer_sub_call_ending_early_taps_the_producer() {
    let f = end_aligned_with(&[("rsi( 0, outIdx-1,", "rsi( 0, outIdx-2,")]);
    assert_classifier_verdict(
        &f,
        r#"accepted ["tap lagdepth.tempStreak.1.t0 ge1 Oldest <- (tempStreak,1) sub0.in0"]"#,
        "the streak rsi reads the producer up to outIdx - 2 = N - 2: delay N - 1 - (N - 2) = 1, exact. \
         Its output ends a bar early, which offsetStreak absorbs",
    );
}

#[test]
fn end_aligned_chained_sub_call_ending_early_taps_its_source() {
    let f = end_aligned_with(&[("percentrank( 0, rocNb-1,", "percentrank( 0, rocNb-2,")]);
    assert_classifier_verdict(
        &f,
        r#"accepted ["tap lagdepth.tempRank.1.t0 ge1 Oldest <- (tempRank,1) sub3.in0"]"#,
        "percentrank reads rocp's rocNb elements up to rocNb - 2: delay rocNb - 1 - (rocNb - 2) = 1, \
         exact. Every map read is a count difference against percentrank's own count, so 0",
    );
}

#[test]
fn end_aligned_rejects_an_offset_that_is_not_a_count_difference() {
    // tempRSI[i+offsetRSI] is nb(rsi) - n - (nb(rsi) - n + 1) = -1 bars behind.
    let f = end_aligned_with(&[(
        "offsetRSI = rsiNb - *outNBElement;",
        "offsetRSI = rsiNb - *outNBElement + 1;",
    )]);
    assert_delay_refused(&f, "map", "tempRSI", "reads ahead");
}

#[test]
fn end_aligned_count_receiver_reused_by_a_later_sub_call_taps_at_a_parameter_depth() {
    // `streakNb` holds the input RSI's count by the time either offset reads it.
    let f = end_aligned_with(&[
        ("&tempBegIdx, &rsiNb, tempRSI", "&tempBegIdx, &streakNb, tempRSI"),
        ("offsetRSI = rsiNb", "offsetRSI = streakNb"),
    ]);
    assert_classifier_verdict(
        &f,
        r#"accepted ["tap lagdepth.tempStreak.2.t0 Oldest <- (tempStreak,2) map0 tempStreak[i+offsetStreak]"]"#,
        "the input rsi begins at its start anchorIdx + rsi_lookback(p), at or past its lookback since \
         startIdx >= lookbackTotal, so offsetStreak = (endIdx - anchorIdx - rsi_lookback(p) + 1) - n. The \
         streak rsi begins at rsi_lookback(s) over the N producer elements, so (tempStreak,2) holds N - \
         rsi_lookback(s) and tempStreak[i+offsetStreak] is rsi_lookback(p) - rsi_lookback(s) - 1 behind: \
         exact, of either sign, so neither ge1 nor refused. A negative depth fails Open's Capture range \
         check, an internal error, never a wrong value",
    );
}

#[test]
fn end_aligned_paced_var_rewritten_before_its_sub_call_taps_the_producer() {
    let f = end_aligned_with(&[(
        "   retCode = rsi( 0, outIdx-1,",
        "   outIdx = outIdx - 1;\n   retCode = rsi( 0, outIdx-1,",
    )]);
    assert_classifier_verdict(
        &f,
        r#"accepted ["tap lagdepth.tempStreak.1.t0 ge1 Oldest <- (tempStreak,1) sub0.in0"]"#,
        "outIdx - 1 after the decrement is N - 2: the streak rsi reads 1 bar behind, exact",
    );
}

#[test]
fn end_aligned_rejects_an_endidx_rewritten_between_sub_calls() {
    // endIdx names the current bar; a body that rewrites it has none.
    let f = end_aligned_with(&[(
        "   retCode = rocp(",
        "   endIdx = endIdx - 1;\n   retCode = rocp(",
    )]);
    assert_delay_refused(&f, "body", "endIdx", "endIdx is rewritten");
}

#[test]
fn end_aligned_rejects_a_producer_that_is_not_one_element_per_bar() {
    // Each edit breaks the one write per bar up to endIdx, so the producer's count
    // is unproven, and the streak rsi's delay over it opaque: the count's reason.
    let write = "      tempStreak[outIdx++] = streak;";
    let rows: [(&str, &str, &str); 8] = [
        ("stops before endIdx", "while( today <= endIdx )", "while( today < endIdx )"),
        ("skips a write", write, "      if( streak != 0.0 )\n         tempStreak[outIdx++] = streak;"),
        ("rewrites a bar", write, "      tempStreak[outIdx++] = streak;\n      if( close == prevClose )\n         outIdx--;"),
        ("advances conditionally", "      today++;", "      if( close == close )\n         today++;"),
        ("skips a bar", "      today++;", "      today++;\n      if( close != close )\n         today++;"),
        ("exits early", "      close = inReal[today];", "      close = inReal[today];\n      if( close != close )\n         break;"),
        ("starts past 0", "   outIdx = 0;", "   outIdx = 1;"),
        ("hands the series to a call", write, "      tempStreak[outIdx++] = streak;\n      memset( tempStreak, 0, 8 );"),
    ];
    for (why, from, to) in rows {
        assert_delay_refused_why(
            &end_aligned_with(&[(from, to)]),
            "sub-call `rsi`",
            "tempStreak",
            "producer series is not provably one element per bar",
            &format!("a producer that {why}"),
        );
    }
}

#[test]
fn end_aligned_streams_a_pointer_copied_series_as_one() {
    let f = end_aligned_with(&[
        ("   double *tempRank;", "   double *tempRank;\n   double *streakBuf;"),
        (
            "   tempStreak = malloc((endIdx-anchorIdx) * sizeof(double));",
            "   streakBuf = malloc((endIdx-anchorIdx) * sizeof(double));\n   tempStreak = streakBuf;",
        ),
    ]);
    assert_classifier_verdict(
        &f,
        "accepted []",
        "tempStreak = streakBuf is an unconditional top-level copy, a must-alias: the two names are one \
         series, and every read is the shipped fixture's",
    );
}

/// Handle fields as `sp.<name>`; a composed map step names nothing else.
struct StepNames;

impl streaming::NameMap for StepNames {
    fn state(&self, name: &str) -> String {
        format!("sp.{name}")
    }
    fn bar(&self, array: &str) -> String {
        unreachable!("bar {array}")
    }
    fn output(&self, name: &str) -> ir::Expr {
        unreachable!("output {name}")
    }
    fn ring_buf(&self, var: &str, array: &str) -> String {
        unreachable!("ring_buf {var} {array}")
    }
    fn ring_pos(&self, var: &str) -> String {
        unreachable!("ring_pos {var}")
    }
    fn ring_lag(&self, var: &str) -> String {
        unreachable!("ring_lag {var}")
    }
    fn ring_cap(&self, var: &str) -> String {
        unreachable!("ring_cap {var}")
    }
    fn win_buf(&self, var: &str, array: &str) -> String {
        unreachable!("win_buf {var} {array}")
    }
    fn win_pos(&self, var: &str) -> String {
        unreachable!("win_pos {var}")
    }
    fn win_cap(&self, var: &str) -> String {
        unreachable!("win_cap {var}")
    }
    fn circ_buf(&self, storage: &str) -> String {
        unreachable!("circ_buf {storage}")
    }
    fn extrema_buf(&self, array: &str) -> String {
        unreachable!("extrema_buf {array}")
    }
    fn extrema_mask(&self) -> String {
        unreachable!("extrema_mask")
    }
}

#[test]
fn a_map_reaches_a_must_alias_series_through_the_storage_its_writes_used() {
    // The class key is the first name in body order, which no step writes.
    let renamed = end_aligned_with(&[
        ("   double *tempRSI;", "   double *streakBuf;\n   double *tempRSI;"),
        (
            "   tempStreak = malloc((endIdx-anchorIdx) * sizeof(double));",
            "   streakBuf = malloc((endIdx-anchorIdx) * sizeof(double));\n   tempStreak = streakBuf;",
        ),
    ]);
    // The map reads the output it writes through another name, before the write.
    let self_read = end_aligned_with(&[
        ("   double *tempRank;", "   double *tempRank;\n   double *res;"),
        ("   tempRank = malloc(", "   res = outReal;\n   tempRank = malloc("),
        ("+ outReal[i] ) / 3.0;", "+ res[i] ) / 3.0;"),
    ]);
    for (f, key) in [(&renamed, "streakBuf"), (&self_read, "res")] {
        let cp = streaming::analyze_composed(f, lookup()).expect("the classifier accepts it");
        assert!(cp.defs.iter().any(|d| d.buffer == key && d.name != key), "{key} keys no renamed definition");
        let si = cp
            .steps
            .iter()
            .position(|s| matches!(s, streaming::UpdateStep::Map { .. }))
            .expect("a map step");
        let lowered = format!("{:?}", streaming::lower_map_step(&cp, si, &StepNames));
        for storage in ["cur_tempStreak", "cur_tempRSI", "cur_outReal"] {
            assert!(lowered.contains(&format!("Var(\"{storage}\")")), "{key}: no {storage} in {lowered}");
        }
        assert!(!lowered.contains("ArrayAccess") && !lowered.contains(&format!("\"{key}\"")), "{key}: {lowered}");
        assert_eq!(cp.output_storage("outReal"), "cur_outReal", "{key}");
    }
}

/* ---- #543: an offset read beside a sibling series ---- */

/// The shipped SQZMOM with each `(from, to)` edit applied once.
fn sqzmom_with(edits: &[(&str, &str)]) -> FuncDef {
    let mut src = std::fs::read_to_string(input_dir().join("sqzmom/sqzmom.c")).expect("sqzmom.c");
    for (from, to) in edits {
        assert!(src.contains(from), "fixture edit `{from}` does not apply");
        src = src.replacen(from, to, 1);
    }
    load_with_source("sqzmom", &src)
}

#[test]
fn sqzmom_derives_a_composed_plan() {
    let f = sqzmom_with(&[]);
    let plan = streaming::validate_streamable(&f, lookup()).expect("SQZMOM streams");
    let streaming::StreamPlan::Composed(cp) = plan else {
        panic!("SQZMOM must derive a composed plan");
    };
    let callees: Vec<&str> = cp.subs.iter().map(|s| s.callee.as_str()).collect();
    assert_eq!(callees, ["midprice", "sma", "trange", "sma", "sma", "stddev", "linearreg"]);
}

/// What a SQZMOM variant row derives.
enum SqzWant {
    /// Accepted with the shipped edges.
    Shipped,
    /// Accepted with the shipped edges less those reading this buffer.
    ShippedLess(&'static str),
    /// Refused: a map reads this buffer ahead of its newest element.
    Ahead(&'static str),
    /// Refused with this verdict.
    Refused(&'static str),
}

// In the rows below n = endIdx - startIdx + 1 is the level map's count, and each
// begin B is presumed at its call's start argument: sqzmom_lookback takes real
// parameters, so the prover cannot order startIdx against a callee's lookback.
#[test]
fn sqzmom_offset_read_variants_classify_by_their_delays() {
    use SqzWant::{Ahead, Refused, Shipped, ShippedLess};
    type Row<'a> = (&'a str, &'a [(&'a str, &'a str)], SqzWant);
    let rows: [Row; 6] = [
        (
            "tempSD[i] is startIdx - Bsd behind, presumed 0, and the added read bb - 1 less: \
             -(bb - 1) <= -1 with Bsd entering at coefficient -1",
            &[(
                "dev = tempSD[i]*optInNbDev;",
                "dev = (tempSD[i]+tempSD[i+(optInBBPeriod-1)])*optInNbDev;",
            )],
            Ahead("tempSD"),
        ),
        (
            "kcmShift = Bsd - Bkcm + 1, so tempKCM[i+kcmShift] is startIdx - Bsd - 1 behind: -1",
            &[("kcmShift = kcmNb - sdNb;", "kcmShift = kcmNb - sdNb + 1;")],
            Ahead("tempKCM"),
        ),
        (
            "stddev reads inClose a bar back, a ge1 tap, and holds endIdx - Bsd elements, so \
             tempSD[i] is startIdx - Bsd - 1 behind: -1",
            &[(
                "stddev( startIdx, endIdx, inClose,",
                "stddev( startIdx, endIdx-1, inClose,",
            )],
            Ahead("tempSD"),
        ),
        (
            "the level map no longer reads tempSD",
            &[("dev = tempSD[i]*optInNbDev;", "dev = optInNbDev;")],
            ShippedLess("tempSD"),
        ),
        (
            "tempNbElement holds stddev's count when kcmShift reads it, and linearreg rewrites it only \
             after: the shipped values, the tempSD count snapshotted after its call",
            &[
                ("&tempBegIdx, &sdNb, tempSD", "&tempBegIdx, &tempNbElement, tempSD"),
                ("kcmShift = kcmNb - sdNb;", "kcmShift = kcmNb - tempNbElement;"),
            ],
            Shipped,
        ),
        (
            "the level map writes tempMid in place of the output",
            &[
                ("outSqueeze[i] = 3;", "tempMid[i] = 3;"),
                ("outSqueeze[i] = 2;", "tempMid[i] = 2;"),
                ("outSqueeze[i] = 1;", "tempMid[i] = 1;"),
                ("outSqueeze[i] = -1;", "tempMid[i] = -1;"),
                ("outSqueeze[i] = 0;", "tempMid[i] = 0;"),
            ],
            Refused("refused: composed pipeline never defines output `outSqueeze`"),
        ),
    ];
    // kcmShift is kcmNb - nb(stddev), so tempKCM[i+kcmShift] is nb(stddev) - n
    // behind: the 0 the tempSD[i] read presumes.
    let shipped = [
        "presumed lagzero.inClose.1.map0.r0 <- (inClose,1) map0 inClose[*outBegIdx+i]",
        "presumed lagzero.tempKCM.1.map0.r0 <- (tempKCM,1) map0 tempKCM[i]",
        "presumed lagzero.tempDev.1.map0.r0 <- (tempDev,1) map0 prior tempDev",
        "presumed lagzero.tempWork.1.sub3 <- (tempWork,1) sub3.in0",
        "presumed lagzero.tempSD.1.map1.r0 <- (tempSD,1) map1 tempSD[i]",
        "presumed lagzero.tempMid.1.map1.r0 <- (tempMid,1) map1 tempMid[i]",
        "presumed lagzero.tempKCM.1.map1.r0 <- (tempKCM,1) map1 tempKCM[i+kcmShift]",
    ];
    for (why, edits, want) in rows {
        let f = sqzmom_with(edits);
        match want {
            Shipped => assert_classifier_verdict(&f, &format!("accepted {shipped:?}"), why),
            ShippedLess(buffer) => {
                let edges: Vec<&str> =
                    shipped.iter().copied().filter(|e| !e.contains(&format!("({buffer},"))).collect();
                assert_classifier_verdict(&f, &format!("accepted {edges:?}"), why);
            }
            Ahead(buffer) => assert_delay_refused(&f, "map", buffer, "reads ahead"),
            Refused(verdict) => assert_classifier_verdict(&f, verdict, why),
        }
    }
}

/* ---- #205: the fill-mode scratch-aliasing precondition ---- */

/// Inventory of composed functions that hand one of their OWN outputs to a
/// sub-call as its **destination**.
///
/// This is the shape that bounds `sc_<out>`'s writes by the *callee's* output
/// count rather than by the caller's own final count. Since #205 the fill-mode
/// scratch IS the caller's array (exactly `historyLen - lookback` wide), so for
/// every function in this set, safety rests on the callee's count equalling our
/// final count — true for each of them because the sub-call that writes our
/// output is what defines that count, but a per-function argument rather than a
/// structural guarantee.
///
/// `fill_scratch_may_alias_output` deliberately does NOT screen this shape (it
/// would decline eight of the ten and forfeit most of the win). Pinning the set
/// is what keeps that decision honest: a new composed function joins it by
/// someone updating this list, not by silently inheriting the optimization.
/// Failure is loud in Rust (slice bound) and Java (AIOOBE) but **silent in C**.
#[test]
fn composed_sub_call_destination_funcs() {
    let lk = lookup();
    let mut found: Vec<String> = Vec::new();
    for entry in std::fs::read_dir(input_dir()).expect("input dir") {
        let path = entry.expect("dir entry").path();
        if !path.is_dir() {
            continue;
        }
        let name = path.file_name().unwrap().to_str().unwrap().to_string();
        if !path.join(format!("{name}.yaml")).exists() || !path.join(format!("{name}.c")).exists() {
            continue;
        }
        let f = load(&name);
        if !f.streaming {
            continue;
        }
        let outs: Vec<String> = f.outputs.iter().map(|o| o.name.clone()).collect();
        // The body each backend streams from: a STREAM alternate replaces the
        // base in the languages it claims.
        let writes_own_output = ir::ALL_LANGS.iter().any(|&lang| {
            let resolved = f.resolved_for(lang);
            let Ok(streaming::StreamPlan::Composed(cp)) = streaming::validate_streamable(&resolved, lk)
            else {
                return false;
            };
            cp.subs
                .iter()
                .any(|s| s.dsts.iter().any(|d| outs.contains(d)))
        });
        if writes_own_output {
            found.push(name.to_uppercase());
        }
    }
    // Membership alone would not tell the next author WHICH invariant to keep:
    // no two of these are safe for the same reason. The reason is recorded with
    // each entry and printed on failure. (Reasons proved by kevinlincg, #205.)
    let expected: [(&str, &str); 18] = [
        ("ALLIGATOR", "each rma writes one of the three lines and is entered at lookbackTotal \
                 over a median buffer that begins lookbackTotal bars before startIdx, so it \
                 clamps nothing: its count is endIdx-startIdx+1, the count ALLIGATOR returns"),
        ("APO", "sub-call uses optInSlowPeriod and the body swaps so slow == max(slow,fast); \
                 the swap is load-bearing -- see apo_family_period_swap_is_a_write_bound_precondition"),
        ("BBW", "as KDJ -- var is handed outBegIdx/outNBElement themselves and BBW returns them \
                 unmodified. Entered at the MA's begIdx its count is endIdx - max(startIdx, \
                 ma_lookback, var_lookback) + 1, which is BBW's final count because bbw_lookback \
                 delegates to bbands_lookback = max(ma_lookback, stddev_lookback -> var_lookback)"),
        ("CRSI", "as KDJ -- percentrank is handed outBegIdx/outNBElement themselves and CRSI \
                 returns the count unmodified, so the final count IS that callee's count. The \
                 rocp buffer it ranks is entered optInRankPeriod bars before startIdx, so that \
                 count is endIdx-startIdx+1"),
        ("ICHIMOKU", "the last midprice writes outSenkouSpanB and medprice writes \
                 outSenkouSpanA. midprice is entered at a startIdx already clamped to the \
                 longest of the three lookbacks, so it clamps nothing and its count is \
                 n = endIdx-startIdx+1. medprice has a lookback of 0 and is entered at 0 over \
                 those n elements, so its count is n too: the expression ICHIMOKU returns"),
        ("KC", "the moving average is entered at exactly ema_lookback over a typical-price buffer \
                 that begins ema_lookback bars before startIdx, so it clamps nothing: its first \
                 output lands on startIdx and its count is endIdx-startIdx+1, the same expression \
                 KC returns. Equal by construction, not by a lookback identity between two callees"),
        ("KDJ", "the sub-call is the WHOLE producer: it is handed outBegIdx/outNBElement \
                 themselves and KDJ returns them unmodified, so the final count IS the \
                 callee's count rather than a quantity derived from it. kdj_lookback \
                 delegates to stoch_lookback, so the two anchors agree by construction too"),
        ("KSTEXT", "the signal average writes outKSTSignal and KSTEXT returns its count unmodified. \
                 Every leg average lands its first output on startIdx-lookbackSignal, so the line \
                 buffer begins exactly ma_lookback(signal) bars before startIdx: entered at 0 the \
                 signal average clamps to that lookback and its count is endIdx-startIdx+1, the \
                 length the memmove gives outKST"),
        ("MACDEXT", "the body RUNTIME-CHECKS the premise (outNbElement1 == endIdx-startIdx+1+lookbackSignal) \
                 and bails otherwise, so signal count == N_MACDEXT"),
        ("PERCENTB", "as BBW, in the STREAM alternate every backend streams from -- its var \
                 writes outReal and is handed outBegIdx/outNBElement themselves, and PERCENTB \
                 returns them unmodified. Entered at the MA's begIdx its count is endIdx - \
                 max(startIdx, ma_lookback, var_lookback) + 1, which is PERCENTB's final count \
                 because percentb_lookback delegates to bbands_lookback"),
        ("PPO", "as APO -- the slow/fast swap is the precondition"),
        ("PVO", "as APO -- the slow/fast swap is the precondition"),
        ("RVIR", "as KDJ -- the low leg is handed outBegIdx/outNBElement themselves and RVIR \
                 returns them unmodified, so the final count IS that callee's count. \
                 rvir_lookback delegates to rvi_lookback, so the high leg entered at the same \
                 startIdx agrees by construction and the averaging loop runs over one count, \
                 not two"),
        ("SQZMOM", "as KC -- linearreg is entered at 0 over tempDev, a buffer whose length is \
                 exactly (endIdx-startIdx+1) + linearreg_lookback(optInKCPeriod), so it clamps to \
                 its own lookback and its count comes out at endIdx-startIdx+1, the expression \
                 SQZMOM returns. Equal through the buffer's LENGTH, not through a lookback \
                 identity between two callees"),
        ("STDDEV", "stddev_lookback DELEGATES to var_lookback in the source, so the counts are \
                 equal by construction rather than by arithmetic coincidence"),
        ("STOCH", "the callee is handed tempBuffer[..*outNBElement], so its output cannot exceed \
                 the slice it was given -- bound holds via the INPUT length, not a lookback identity"),
        ("STOCHF", "as STOCH, with outIdx in place of *outNBElement"),
        ("STOCHRSI", "tempRSIBuffer is SIZED as endIdx-startIdx+1+lookbackSTOCHF precisely so the \
                 callee's count comes out at N_STOCHRSI"),
    ];
    found.sort();
    let want: Vec<String> = expected.iter().map(|(n, _)| (*n).to_string()).collect();
    let why: String = expected
        .iter()
        .map(|(n, r)| format!("\n  {n}: {r}"))
        .collect();
    assert_eq!(
        found, want,
        "a composed function's sub-call writes into one of its own outputs. Since #205 that \
         output IS the caller's array in fill mode, so the callee's count must equal this \
         function's FINAL count -- not whichever intermediate a sub-call left in *outNBElement \
         (ADXR is the case where the intermediate is LARGER). State the reason for the new \
         function here; the existing ones hold for four different reasons:{why}"
    );
}

/// A [`CalleeLookup`] that reports ONE callee as `nan_inf_output`, leaving the
/// rest of the real corpus exactly as it is.
struct FlagOneCallee<'a> {
    inner: &'a ta_codegen_lib::registry::Registry,
    flagged: &'a str,
}

impl streaming::CalleeLookup for FlagOneCallee<'_> {
    fn callee(&self, name: &str) -> Option<streaming::CalleeSig> {
        let mut sig = self.inner.callee(name)?;
        if name == self.flagged {
            sig.nan_inf_output = true;
        }
        Some(sig)
    }
}

/// A function that can return NaN or ±Inf (`nan_inf_output`, #191) may not drive
/// another function's sub-stream: the streaming tier checks finiteness only at
/// the caller boundary, and the composed step has nowhere to put a sub-stream's
/// rejection, so the bar's state advance would be dropped silently.
///
/// No flagged function is composed by anything today, so the gate is dormant
/// against the shipped corpus, which is exactly why it needs a test that can
/// see it fire. The flag is injected rather than written to `ma.yaml`, so the
/// corpus is untouched.
#[test]
fn nan_inf_callee_is_refused() {
    let reg = lookup();
    let bbands = load("bbands"); // composes MA (sub0) and STDDEV (sub1)
    let sma = load("sma"); // loop tier: composes nothing

    // Control: as shipped, MA is finite-output and BBANDS composes it happily.
    assert!(
        streaming::validate_streamable(&bbands, reg).is_ok(),
        "BBANDS must derive a plan against the unmodified corpus, or the probe below \
         proves nothing"
    );

    let flagged = FlagOneCallee { inner: reg, flagged: "ma" };
    let err = streaming::validate_streamable(&bbands, &flagged)
        .expect_err("BBANDS composes MA, so a nan_inf_output MA must be refused");
    assert!(
        err.contains("nan_inf_output")
            && err.contains("composing ma,")
            && err.contains("BBANDS"),
        "the refusal must name the flag, the callee and the composing function: {err}"
    );
    // `contains("ma")` would be satisfied by the fixed prose ("...MAy not drive
    // a sub-stream"), i.e. by a message that named the wrong callee entirely.
    // Anchoring on "composing ma," is what makes the callee half discriminating.

    // ...and only the composing functions: SMA calls nothing, so flagging MA
    // must not disturb it. Without this the gate could pass by rejecting
    // everything.
    assert!(
        streaming::validate_streamable(&sma, &flagged).is_ok(),
        "SMA composes no sub-stream; flagging MA must not touch it"
    );

    // `plan_callees` has a separate arm per tier, and one arm being right says
    // nothing about the others. BBANDS above is Composed; MAVP is the
    // PERIOD-BANK arm (its bank of MA sub-streams) and MA itself is the
    // DISPATCH arm (its per-MAType sub-streams).
    let mavp = load("mavp");
    let err = streaming::validate_streamable(&mavp, &flagged)
        .expect_err("MAVP banks MA sub-streams, so a nan_inf_output MA must be refused");
    assert!(
        err.contains("nan_inf_output") && err.contains("composing ma,") && err.contains("MAVP"),
        "the period-bank arm must name the flag, the callee and the function: {err}"
    );

    let ma = load("ma");
    let flagged_sma = FlagOneCallee { inner: reg, flagged: "sma" };
    let err = streaming::validate_streamable(&ma, &flagged_sma)
        .expect_err("MA dispatches to SMA, so a nan_inf_output SMA must be refused");
    assert!(
        err.contains("nan_inf_output") && err.contains("composing sma,") && err.contains("MA"),
        "the dispatch arm must name the flag, the callee and the function: {err}"
    );
    // Control for both: unflagged, they derive plans as usual.
    assert!(streaming::validate_streamable(&mavp, reg).is_ok(), "MAVP plans normally");
    assert!(streaming::validate_streamable(&ma, reg).is_ok(), "MA plans normally");
}

/// Window mode is taken only for MA types whose value at a bar is the batch
/// over that bar's window alone. A false positive gives wrong stream values,
/// so every other arm must be refused, each by the clause that applies to it.
#[test]
fn mavp_window_labels_are_alma_and_disabled() {
    let mut labels = lookup().window_labels("mavp").to_vec();
    labels.sort();
    assert_eq!(labels, ["MAType_ALMA", "MAType_DISABLED"]);
    assert!(streaming::window_evaluable(&load("alma"), lookup()).is_ok());
    let persists = "persists across bars";
    let history = "carries history other than lags of its input";
    let not_loop = "not a single-loop stream";
    for (name, clause) in [
        ("sma", persists), ("ema", persists), ("wma", persists), ("t3", persists),
        ("zlema", persists), ("vidya", persists), ("dema", persists), ("tema", persists),
        ("kama", history), ("rma", history), ("mama", history),
        ("trima", not_loop), ("hma", not_loop),
    ] {
        let why = streaming::window_evaluable(&load(name), lookup())
            .expect_err(&format!("{name} must not be window-evaluable"));
        assert!(why.contains(clause), "{name}: {why}");
    }
}

/// The input-taint clause is the one no shipped arm reaches: ALMA with its
/// normaliser seeded from the input keeps every other property.
#[test]
fn window_evaluable_refuses_state_derived_from_the_input() {
    let src = std::fs::read_to_string(input_dir().join("alma/alma.c")).unwrap();
    assert_eq!(src.matches("   norm = 0.0;").count(), 2, "fixture anchor moved");
    let (clean, _) = crate::common::load_indicator_with_source("alma", &src);
    assert!(streaming::window_evaluable(&clean, lookup()).is_ok(), "the unchanged source must pass");
    let tainted = src.replace("   norm = 0.0;", "   norm = inReal[0] - inReal[0];");
    let (func, _) = crate::common::load_indicator_with_source("alma", &tainted);
    let why = streaming::window_evaluable(&func, lookup()).expect_err("input-derived weights");
    assert!(why.contains("depends on the input or the range"), "{why}");
}

// ---------------------------------------------------------------------------
// Composed delay classes
// ---------------------------------------------------------------------------

fn show(e: &ir::Expr) -> String {
    use ir::{BinOp, Expr};
    match e {
        Expr::Var(v) => v.clone(),
        Expr::IntLiteral(k) => k.to_string(),
        Expr::PointerDeref(p) => format!("*{p}"),
        Expr::Cast(_, x) => show(x),
        Expr::BinOp(l, op, r) => {
            let op = match op {
                BinOp::Add => "+",
                BinOp::Sub => "-",
                BinOp::Mul => "*",
                _ => "?",
            };
            format!("{}{op}{}", show(l), show(r))
        }
        other => format!("{other:?}"),
    }
}

/// Each non-Zero edge of a composed plan: its class, then what reads what.
fn non_zero_edges(cp: &streaming::ComposedPlan) -> Vec<String> {
    use streaming::{Consumer, EdgeClass, UpdateStep};
    let map_no = |step: usize| cp.steps[..step].iter().filter(|s| matches!(s, UpdateStep::Map { .. })).count();
    let copy_no = |step: usize| cp.steps[..step].iter().filter(|s| matches!(s, UpdateStep::Align { .. })).count();
    let arm_of = |arm: &Option<(bool, usize)>| match arm {
        Some((true, _)) => ".then",
        Some((false, _)) => ".else",
        None => "",
    };
    let mut out = Vec::new();
    for e in &cp.edges {
        let def = &cp.defs[e.def];
        let reader = match &e.consumer {
            Consumer::SubSrc { sub_idx, input } => format!("sub{sub_idx}.in{input}"),
            Consumer::Align { step } => format!("copy{}", copy_no(*step)),
            Consumer::MapRead { step, arm, buffer, index } => {
                format!("map{}{} {buffer}[{}]", map_no(*step), arm_of(arm), show(index))
            }
            Consumer::MapPrior { step, arm, buffer } => {
                format!("map{}{} prior {buffer}", map_no(*step), arm_of(arm))
            }
            Consumer::TailScalar { tail_idx } => format!("tail{tail_idx}"),
            Consumer::Return { output } => format!("ret {output}"),
        };
        let class = match &e.class {
            EdgeClass::Zero => continue,
            EdgeClass::ZeroPresumed { key } => format!("presumed {key}"),
            EdgeClass::Tap { line, tap } => {
                let t = &cp.lines[*line].taps[*tap];
                format!("tap {} {}{:?}", t.key, if t.ge1 { "ge1 " } else { "" }, t.back)
            }
        };
        out.push(format!("{class} <- ({},{}) {reader}", def.buffer, def.ordinal));
    }
    out
}

/// Every composed plan of every streaming function under `input/`, one per
/// language, with its classification.
fn classified_corpus() -> Vec<(String, ir::Lang, Result<Vec<String>, String>)> {
    let lk = lookup();
    let mut out = Vec::new();
    for name in crate::common::discover_indicators() {
        if name.starts_with("synth") {
            continue;
        }
        let f = load(&name);
        if !f.streaming {
            continue;
        }
        for lang in ir::ALL_LANGS {
            let resolved = f.resolved_for(lang);
            let got = match streaming::validate_streamable(&resolved, lk) {
                Ok(streaming::StreamPlan::Composed(cp)) => Ok(non_zero_edges(&cp)),
                Err(e) if e.contains("composed body is not streamable") => Err(e),
                _ => continue,
            };
            out.push((name.to_uppercase(), lang, got));
        }
    }
    out
}

/// Zero is implied for every edge not listed. A Tap on a shipped function is
/// a per-bar delay line in its Update; a ZeroPresumed edge is a 0 the prover
/// could not show, checked at Open on the exact counts, so its reason is the
/// hand derivation of that 0.
#[test]
fn composed_edge_classes_are_pinned() {
    let expected: [(&str, &[&str], &str); 26] = [
        ("ADXR", &["tap lagdepth.adx.1.t0 ge1 Oldest <- (adx,1) map0 adx[outIdx]"],
            "outReal[i] averages adx[i+(p-1)] with adx[i], a lag fixed by the period and \
             at least 1 for every accepted one"),
        ("ALLIGATOR", &[
            "presumed lagzero.outJaw.1.ret.outJaw <- (outJaw,1) ret outJaw",
            "presumed lagzero.outTeeth.1.ret.outTeeth <- (outTeeth,1) ret outTeeth",
        ], "every rma starts at lookbackTotal, the largest of the three rma lookbacks, so no leg \
            clamps and each line holds the lips count the body returns; the prover only sees each \
            begin as max(lookbackTotal, that leg's rma_lookback)"),
        ("APO", &[], ""),
        ("BBANDS", &["presumed lagzero.tempBuffer1.1.copy0 <- (tempBuffer1,1) copy0"],
            "stddev starts at the ma's begin, so it begins no earlier and shiftIdx is exactly the \
             begin difference: the memmove takes the ma's last n elements; the guarded difference \
             is an opaque join to the prover"),
        ("BBW", &[], ""),
        ("CRSI", &[], ""),
        ("GATOR", &[
            "tap lagdepth.tempA.1.t0 Oldest <- (tempA,1) map0 tempA[i]",
            "tap lagdepth.tempB.1.t0 Oldest <- (tempB,1) map0 tempB[i]",
            "tap lagdepth.tempA.2.t0 Oldest <- (tempA,2) map1 tempA[i]",
            "tap lagdepth.tempB.2.t0 Oldest <- (tempB,2) map1 tempB[i]",
        ], "each rma starts max(its shift - its partner's shift, 0) bars before lookbackTotal, so \
            its cursor read is that many bars behind its newest element: 0 for one leg of every \
            pair at any parameter set, so no tap is ge1"),
        ("ICHIMOKU", &[
            "presumed lagzero.tempT.1.sub3 <- (tempT,1) sub3.in0",
            "presumed lagzero.tempK.1.sub3 <- (tempK,1) sub3.in1",
            "presumed lagzero.tempT.1.map0.r0 <- (tempT,1) map0 tempT[i]",
            "presumed lagzero.tempK.1.map0.r0 <- (tempK,1) map0 tempK[i]",
            "presumed lagzero.outSenkouSpanB.1.ret.outSenkouSpanB <- (outSenkouSpanB,1) ret outSenkouSpanB",
        ], "startIdx is clamped to the longest period minus 1, so every midprice begins at startIdx \
            and holds n elements; the prover does not see through the if-chain lookback"),
        ("KC", &[
            "presumed lagzero.tempTP.1.sub2 <- (tempTP,1) sub2.in0",
            "presumed lagzero.tempATR.1.map0.r0 <- (tempATR,1) map0 tempATR[i]",
        ], "startIdx is clamped to max(ema_lookback, atr_lookback), so typprice starts at or after \
            0 and atr at or after its lookback: neither clamps, and both hold the ema's count; the \
            prover keeps that max opaque"),
        ("KDJ", &[], ""),
        ("KSTEXT", &[
            "presumed lagzero.tempBuffer.2.map0.r0 <- (tempBuffer,2) map0 tempBuffer[i]",
            "presumed lagzero.tempBuffer.4.map1.r0 <- (tempBuffer,4) map1 tempBuffer[i]",
            "presumed lagzero.tempBuffer.6.map2.r0 <- (tempBuffer,6) map2 tempBuffer[i]",
            "presumed lagzero.kstBuffer.5.copy0 <- (kstBuffer,5) copy0",
            "presumed lagzero.outKST.1.ret.outKST <- (outKST,1) ret outKST",
        ], "startIdx is clamped to the longest ROC period plus its MA lookback, plus the signal \
            lookback, so every roc begins at its start and each leg holds kstNb elements; the \
            prover keeps the if-chain max opaque"),
        ("MACDEXT", &[
            "presumed lagzero.fastMABuffer.1.map0.r0 <- (fastMABuffer,1) map0 prior fastMABuffer",
            "presumed lagzero.fastMABuffer.2.copy0 <- (fastMABuffer,2) copy0",
            "presumed lagzero.outMACD.1.map1.r0 <- (outMACD,1) map1 outMACD[i]",
            "presumed lagzero.outMACD.1.ret.outMACD <- (outMACD,1) ret outMACD",
        ], "after the period swap both MAs start at startIdx - lookbackSignal, at or after the \
            larger of their lookbacks, so both begin there with equal counts; an MA lookback over \
            a swapped parameter has no interval, so the prover cannot order it against that max"),
        ("PERCENTB", &[
            "presumed lagzero.tempX.1.map0.r0 <- (tempX,1) map0.then tempX[i]",
            "presumed lagzero.tempX.1.map0.r1 <- (tempX,1) map0.else tempX[i]",
        ], "one tempX read per variant arm. percentb_lookback = bbands_lookback = max(ma_lookback, p-1), \
            so firstIdx = max(startIdx, LB_ma, p-1). var starts at the ma's begin \
            max(startIdx, LB_ma) and its own lookback p-1 clamps that begin to \
            max(startIdx, LB_ma, p-1) = firstIdx: the read is 0 bars behind. The prover cannot \
            see it: percentb_lookback takes real parameters and is not inlined"),
        ("PPO", &[], ""),
        ("PVO", &[], ""),
        ("RVIR", &[], ""),
        ("SQZMOM", &[
            "presumed lagzero.inClose.1.map0.r0 <- (inClose,1) map0 inClose[*outBegIdx+i]",
            "presumed lagzero.tempKCM.1.map0.r0 <- (tempKCM,1) map0 tempKCM[i]",
            "presumed lagzero.tempDev.1.map0.r0 <- (tempDev,1) map0 prior tempDev",
            "presumed lagzero.tempWork.1.sub3 <- (tempWork,1) sub3.in0",
            "presumed lagzero.tempSD.1.map1.r0 <- (tempSD,1) map1 tempSD[i]",
            "presumed lagzero.tempMid.1.map1.r0 <- (tempMid,1) map1 tempMid[i]",
            "presumed lagzero.tempKCM.1.map1.r0 <- (tempKCM,1) map1 tempKCM[i+kcmShift]",
        ], "(tempWork,1) sub3.in0: sqzmom_lookback >= trange_lookback + sma_lookback(k) = k, so the \
            clamped startIdx >= k and stageIdx = startIdx - linearreg_lookback(k) = startIdx - \
            (k-1) >= 1 = trange_lookback; trange begins at its start and the delay is 0. The \
            other edges read buffers whose calls start at startIdx or stageIdx, which the \
            clamp to sqzmom_lookback puts at or past each call's own lookback. The prover \
            cannot see any of it: sqzmom_lookback takes real parameters"),
        ("STDDEV", &[], ""),
        ("STOCH", &[], ""),
        ("STOCHF", &[], ""),
        ("STOCHRSI", &[], "Zero, not presumed: lookbackTotal = rsi_lookback(p) + lookbackSTOCHF, \
            so the rsi begins at its start, and the stochf feed ends at the rsi count - 1: \
            delay = count - 1 - end = 0"),
        ("SYNTH13", &[], ""),
        ("SYNTH14", &[], ""),
        ("SYNTH25", &[
            "tap lagdepth.tempM.1.t0 Field <- (tempM,1) sub2.in0",
            "tap lagdepth.tempM.1.t1 Field <- (tempM,1) sub3.in0",
            "tap lagdepth.tempA.1.t0 Oldest <- (tempA,1) map0 tempA[i]",
            "presumed lagzero.tempB.1.map0.r0 <- (tempB,1) map0 tempB[i]",
            "presumed lagzero.tempC.1.map0.r0 <- (tempC,1) map0 tempC[i]",
        ], "the if-chain lookback LT = p + max(shA,shB,shC) stays opaque, so each midpoint begin \
            is presumed at its start argument. tempM holds nbElement + LT - 1 elements ending at \
            localEnd, so the legs ending shB and shC early read it shB and shC behind, exact. The \
            first leg starts lastLookback - shA >= p - 1 bars in and holds nbElement + shA, so \
            tempA[i] is shA behind; tempB and tempC hold nbElement"),
        ("SYNTH26", &[
            "tap lagdepth.tempX.1.t0 ge1 Literal(2) <- (tempX,1) map0 tempX[i+lookbackTotal-1]",
            "tap lagdepth.tempX.1.t1 ge1 Literal(1) <- (tempX,1) map0 tempX[i+lookbackTotal-2]",
            "tap lagdepth.tempX.1.t2 ge1 Literal(0) <- (tempX,1) map0 tempX[i+lookbackTotal-3]",
            "tap lagdepth.inReal.1.t0 ge1 Oldest <- (inReal,1) map0 inReal[startIdx+i-1]",
        ], "LT = p + 2 inlines: the producer holds nbElement + LT elements and the midpoint \
            begins at LT, 3 past its lookback, so nbM = nbElement. tempX[i+LT-j] is j behind, \
            inReal[startIdx+i-1] is endIdx + 1 - nbM - (startIdx - 1) = 1 behind"),
        ("SYNTH28", &[
            "tap lagdepth.tempA.1.t0 ge1 Oldest <- (tempA,1) copy1",
            "tap lagdepth.tempA.2.t0 Oldest <- (tempA,2) map1 prior tempA",
            "tap lagdepth.tempD.1.t0 ge1 Oldest <- (tempD,1) map2 tempD[i]",
        ], "LT = p - 1 + lag inlines, so tempA begins at startIdx - lag and holds nA = nbElement + \
            lag: the second memmove takes it lag behind. The in-place period-2 midpoint holds \
            nA - 1, so the partial write's prior is lag - 1 behind. tempD holds nA, so tempD[i] \
            is lag behind"),
    ];
    let want: std::collections::BTreeMap<&str, Vec<String>> = expected
        .iter()
        .map(|(n, e, _)| (*n, e.iter().map(|s| (*s).to_string()).collect()))
        .collect();
    for (name, edges, why) in &expected {
        assert!(edges.is_empty() || !why.is_empty(), "{name}: a non-Zero edge needs its reason");
    }
    let mut seen = std::collections::BTreeSet::new();
    let mut check = |name: String, lang: &str, got: Result<Vec<String>, String>| {
        let got = got.unwrap_or_else(|e| panic!("{name} ({lang}) is refused: {e}"));
        let w = want.get(name.as_str()).unwrap_or_else(|| panic!("{name} composes but is not pinned: {got:#?}"));
        assert_eq!(&got, w, "{name} ({lang})");
        seen.insert(name);
    };
    for (name, lang, got) in classified_corpus() {
        check(name, &format!("{lang:?}"), got);
    }
    for synth in ["synth13", "synth14", "synth25", "synth26", "synth28"] {
        let f = crate::common::load_synth(synth).0;
        let got = streaming::analyze_composed(&f, lookup()).map(|cp| non_zero_edges(&cp));
        check(synth.to_uppercase(), "base", got.map_err(|e| e.to_string()));
    }
    let pinned: std::collections::BTreeSet<String> = want.keys().map(|n| (*n).to_string()).collect();
    assert_eq!(seen, pinned, "a pinned function no longer composes");
}

#[test]
fn synth26_reads_through_its_unconditional_copy() {
    use streaming::{Consumer, EdgeClass};
    let f = crate::common::load_synth("synth26").0;
    let cp = streaming::analyze_composed(&f, lookup()).expect("SYNTH26 composes");
    let work = cp
        .edges
        .iter()
        .find(|e| matches!(&e.consumer, Consumer::MapRead { buffer, .. } if buffer == "work"))
        .expect("the map reads through work");
    let def = &cp.defs[work.def];
    assert_eq!((def.buffer.as_str(), def.ordinal, &work.class), ("tempM", 1, &EdgeClass::Zero));
}

#[test]
fn synth28_copies_its_tapped_align_and_seeds_a_tapped_prior() {
    use ir::{Expr, Statement};
    let f = crate::common::load_synth("synth28").0;
    let cp = streaming::analyze_composed(&f, lookup()).expect("SYNTH28 composes");
    // The first memmove's source is rewritten by the in-place midpoint and the
    // partial write; the second copies its tap.
    assert_eq!(align_copies(&f), [true, true]);
    let maps: Vec<usize> = (0..cp.steps.len())
        .filter(|&s| matches!(cp.steps[s], streaming::UpdateStep::Map { .. }))
        .collect();
    let partial = streaming::lower_map_step(&cp, maps[1], &StepNames);
    let k = (0..cp.lines.len())
        .find(|&k| {
            let d = &cp.defs[cp.lines[k].def];
            (d.buffer.as_str(), d.ordinal) == ("tempA", 2)
        })
        .expect("a delay line on the partial write's prior");
    let tap = streaming::lag_tap_local(k, 0);
    let prologue = partial
        .iter()
        .position(|s| matches!(s, Statement::Assign { target: Expr::Var(t), value: Expr::Var(v), .. }
            if t == "cur_tempA" && *v == tap))
        .unwrap_or_else(|| panic!("no tapped prior prologue reading {tap}: {partial:?}"));
    let load = partial
        .iter()
        .position(|s| matches!(s, Statement::Assign { target: Expr::Var(t), .. } if *t == tap))
        .unwrap_or_else(|| panic!("{tap} is never loaded: {partial:?}"));
    assert!(load < prologue, "the tap is loaded before the prologue reads it: {partial:?}");
    let write = partial.iter().position(|s| matches!(s, Statement::If { .. })).expect("the partial write");
    assert!(prologue < write, "the prior is seeded before the write: {partial:?}");
    let text = format!("{partial:?}");
    assert!(text.contains("Var(\"cur_outMaxLag\")") && text.contains("Var(\"cur_outMaxNow\")"), "{text}");
    assert!(cp.intermediates.iter().any(|s| s == "tempD"), "{:?}", cp.intermediates);
    assert!(freed_series(&cp).iter().any(|s| s == "tempD"), "{:?}", freed_series(&cp));
    let last = format!("{:?}", streaming::lower_map_step(&cp, maps[2], &StepNames));
    assert!(last.contains("Var(\"cur_tempD\")"), "{last}");
}

// ---------------------------------------------------------------------------
// Delay rows: one fact varied from an accepted tail
// ---------------------------------------------------------------------------

#[test]
fn a_start_argument_sliding_with_endidx_is_refused() {
    let f = apo_tail_with(&[(
        "ma( startIdx, endIdx, inReal, optInFastPeriod",
        "ma( endIdx-5, endIdx, inReal, optInFastPeriod",
    )]);
    assert_delay_refused(&f, "sub-call `ma`", "inReal", "slides with endIdx");
}

#[test]
fn a_memmove_offset_sliding_with_endidx_is_refused() {
    let src = bbands_source(BBANDS_READ_ONLY_TAIL);
    assert!(src.contains("&tmp[0], n"), "the BBANDS memmove moved");
    let f = load_with_source("bbands", &src.replacen("&tmp[0], n", "&tmp[endIdx-startIdx], n", 1));
    assert_delay_refused(&f, "memmove", "tmp", "slides with endIdx");
}

#[test]
fn an_end_argument_halving_endidx_is_refused() {
    // The fast ma's source delay is endIdx - endIdx/2: the quotient is opaque, and
    // the endIdx term beside it survives.
    let f = apo_tail_with(&[(
        "ma( startIdx, endIdx, inReal, optInFastPeriod",
        "ma( startIdx, endIdx/2, inReal, optInFastPeriod",
    )]);
    assert_delay_refused(&f, "sub-call `ma`", "inReal", "slides with endIdx");
}

#[test]
fn an_offset_read_from_the_data_is_refused() {
    // tempBuffer[i+offset] is Bslow - Bfast - tempBuffer[0] behind.
    let f = apo_tail_with(&[("offset = fastNb - *outNBElement;", "offset = (int)tempBuffer[0];")]);
    assert_delay_refused(&f, "map", "tempBuffer", "depends on data or an unmodelled value");
}

#[test]
fn a_read_not_affine_in_the_cursor_is_refused() {
    let f = apo_tail_with(&[("tempBuffer[i+offset]", "tempBuffer[i*2]")]);
    assert_delay_refused(&f, "map", "tempBuffer", "index is not affine in the cursor");
}

#[test]
fn a_map_reading_its_own_output_at_an_offset_is_refused() {
    let f = apo_tail_with(&[("- outReal[i];", "- outReal[i+1];")]);
    assert_delay_refused(&f, "map", "outReal", "reads the map's own output at an offset");
}

#[test]
fn a_return_count_short_of_its_output_is_refused() {
    // The map writes nbElement elements and the body returns one fewer: the newest
    // element returned is one bar behind outReal's newest.
    let src = std::fs::read_to_string(input_dir().join("adxr").join("adxr.c")).expect("adxr.c");
    let ret = "   *outNBElement = nbElement;";
    assert!(src.contains(ret), "the ADXR return moved");
    let f = load_with_source("adxr", &src.replacen(ret, "   *outNBElement = nbElement - 1;", 1));
    assert_delay_refused(&f, "return", "outReal", "non-Zero return edge");
}

/// [`APO_TAIL`] with `offset` set in the prologue, by a join on `cond`.
fn apo_offset_joined_on(cond: &str) -> FuncDef {
    let first = "\n   retCode = ma( startIdx, endIdx, inReal, optInFastPeriod";
    let joined = format!("\n   if( {cond} )\n      offset = 1;\n   else\n      offset = 0;{first}");
    apo_tail_with(&[(first, &joined), ("   offset = fastNb - *outNBElement;\n", "")])
}

#[test]
fn an_offset_joined_on_data_or_endidx_is_refused() {
    // tempBuffer[i+offset] is Bslow - Bfast - offset behind, and a join on what no
    // parameter fixes is opaque.
    for cond in ["inReal[0] > 0.0", "endIdx > 100"] {
        assert_delay_refused(
            &apo_offset_joined_on(cond),
            "map",
            "tempBuffer",
            "depends on data or an unmodelled value",
        );
    }
}

#[test]
fn a_partial_map_write_with_no_prior_definition_is_refused() {
    // The path that skips the write leaves tempBuffer2 holding what nothing defined.
    let f = apo_tail_with(&[(
        "      outReal[i] = tempBuffer[i+offset] - outReal[i];",
        "   {\n      if( outReal[i] > 0.0 )\n         tempBuffer2[i] = outReal[i];\n      \
         outReal[i] = tempBuffer[i+offset] - outReal[i];\n   }",
    )]);
    assert_delay_refused(&f, "map", "tempBuffer2", "a partial map write has no prior definition");
}

#[test]
fn a_tail_read_of_the_newest_element_is_refused() {
    // fastNb - 1 is tempBuffer's newest element, a bar that moves with endIdx, read
    // where Open alone runs: once by a statement, once by a frozen prelude loop.
    let scalar = apo_tail_with(&[("   offset = fastNb", "   last = tempBuffer[fastNb-1];\n   offset = fastNb")]);
    assert_delay_refused(&scalar, "tail statement", "tempBuffer", "slides with endIdx");
    let prelude = apo_tail_with(&[(
        "   offset = fastNb",
        "   for( i=fastNb-1; i<fastNb; i++ )\n      last = tempBuffer[i];\n   offset = fastNb",
    )]);
    assert_delay_refused(&prelude, "tail loop", "tempBuffer", "slides with endIdx");
}

#[test]
fn a_may_alias_election_written_through_one_name_is_refused() {
    // Which buffer the ema writes is the mode's: in mode 1 the map reads sma - ema,
    // otherwise the ema overwrites tempBuf and it reads 0. Once the ema writes
    // `work`, every other name it may share storage with is unknown.
    let src = r#"
int gator_display_shift(int optInJawPeriod, int optInJawShift,
   int optInTeethPeriod, int optInTeethShift,
   int optInLipsPeriod, int optInLipsShift,
   int outputIdx)
{
   if( outputIdx == 0 )
      return optInJawShift < optInTeethShift ? optInJawShift : optInTeethShift;
   return optInTeethShift < optInLipsShift ? optInTeethShift : optInLipsShift;
}

TA_RetCode gator( int startIdx, int endIdx, const double inHigh[], const double inLow[],
   int optInJawPeriod, int optInJawShift, int optInTeethPeriod, int optInTeethShift,
   int optInLipsPeriod, int optInLipsShift,
   int *outBegIdx, int *outNBElement, double outUpper[], double outLower[] )
{
   double *tempBuf;
   double *work;
   TA_RetCode retCode;
   int b1, n1, b2, n2;
   int i;

   tempBuf = malloc((endIdx-startIdx+1) * sizeof(double));
   if( optInJawShift == 1 )
      work = outUpper;
   else
      work = tempBuf;
   retCode = sma( startIdx, endIdx, inHigh, optInJawPeriod, &b1, &n1, tempBuf );
   retCode = ema( startIdx, endIdx, inHigh, optInJawPeriod, &b2, &n2, work );
   for( i=0; i<n1; i++ )
   {
      outUpper[i] = tempBuf[i] - work[i];
      outLower[i] = tempBuf[i];
   }
   free( tempBuf );
   *outBegIdx = b1;
   *outNBElement = n1;
   return TA_SUCCESS;
}
"#;
    assert_delay_refused(&load_with_source("gator", src), "map", "tempBuf", "a may-alias name was written");
}

#[test]
fn a_count_receiver_rewritten_by_an_unmodelled_statement_is_refused() {
    // fastNb * fastNb is no linear form, so offset loses the endIdx term it cancelled
    // against nb(slow): tempBuffer[i+offset]'s delay keeps endIdx at coefficient 1.
    let f = apo_tail_with(&[("   offset = fastNb", "   fastNb = fastNb * fastNb;\n   offset = fastNb")]);
    assert_delay_refused(&f, "map", "tempBuffer", "slides with endIdx");
}

#[test]
fn a_presume_check_reads_the_count_its_call_left() {
    use streaming::{LagTerm, OpenAction, OpenAt};
    // Both calls write fastNb, and the map runs over the second count.
    let f = apo_with_tail(
        r"
   retCode = ma( startIdx, endIdx, inReal, optInFastPeriod, optInMAType,
      &fastBeg, &fastNb, tempBuffer );
   retCode = ma( startIdx, endIdx, inReal, optInSlowPeriod, optInMAType,
      &fastBeg, &fastNb, tempBuffer2 );
   for( i=0; i < fastNb; i++ )
      outReal[i] = tempBuffer[i] - tempBuffer2[i];
   *outBegIdx = fastBeg;
   *outNBElement = fastNb;",
    );
    assert_classifier_verdict(
        &f,
        r#"accepted ["presumed lagzero.tempBuffer.1.map0.r0 <- (tempBuffer,1) map0 tempBuffer[i]"]"#,
        "tempBuffer[i] is nb(fast) - nb(slow) = Bslow - Bfast behind: 0 with both begins presumed at \
         startIdx; tempBuffer2[i] is its own count, 0",
    );
    // The check reads the fast count from the snapshot taken before the slow call
    // overwrites fastNb, and the map bound where the map runs.
    let cp = streaming::analyze_composed(&f, lookup()).expect("accepted above");
    let slow = cp.subs[1].tail_idx;
    let snapped = cp.open_actions.iter().find_map(|(at, a)| matches!(a, OpenAction::Snapshot { .. }).then_some(at));
    assert!(matches!(snapped, Some(OpenAt::AfterStmt(i)) if *i < slow), "{snapped:?}");
    let presumed = cp
        .open_actions
        .iter()
        .find_map(|(_, a)| match a {
            OpenAction::Presume { edge } => Some(*edge),
            _ => None,
        })
        .expect("a presume check");
    let terms = cp.edge_delay_formula(presumed).terms;
    assert!(
        matches!(terms.as_slice(), [(1, LagTerm::Nb(0)), (-1, LagTerm::Batch(ir::Expr::Var(v)))] if v == "fastNb"),
        "{terms:?}"
    );
}

// ---------------------------------------------------------------------------
// Delay rows: coverage of the accepted shapes
// ---------------------------------------------------------------------------

#[test]
fn literal_offsets_share_one_line_with_literal_backs() {
    let f = apo_with_tail(
        r"
   retCode = ma( startIdx, endIdx, inReal, optInFastPeriod, optInMAType,
      &fastBeg, &fastNb, tempBuffer );
   nb = fastNb - 3;
   for( i=0; i < nb; i++ )
      outReal[i] = (tempBuffer[i+3] + tempBuffer[i+2]) + (tempBuffer[i+1] + tempBuffer[i]);
   *outBegIdx = fastBeg + 3;
   *outNBElement = nb;",
    );
    assert_classifier_verdict(
        &f,
        r#"accepted ["tap lagdepth.tempBuffer.1.t0 ge1 Literal(2) <- (tempBuffer,1) map0 tempBuffer[i+2]", "tap lagdepth.tempBuffer.1.t1 ge1 Literal(1) <- (tempBuffer,1) map0 tempBuffer[i+1]", "tap lagdepth.tempBuffer.1.t2 ge1 Literal(0) <- (tempBuffer,1) map0 tempBuffer[i]"]"#,
        "the map runs fastNb - 3 times over tempBuffer's fastNb elements, so tempBuffer[i+k] is 3 - k \
         behind: Zero at k = 3, then depths 1, 2 and 3, exact, on one line whose deepest tap sets cap 3 \
         and the others read 2, 1 and 0 slots back",
    );
}

#[test]
fn opposite_shift_legs_each_tap_at_a_depth_that_may_be_zero() {
    let f = apo_with_tail(
        r"
   backFast = 0;
   if( optInFastPeriod > optInSlowPeriod )
      backFast = optInFastPeriod - optInSlowPeriod;
   backSlow = 0;
   if( optInSlowPeriod > optInFastPeriod )
      backSlow = optInSlowPeriod - optInFastPeriod;
   retCode = ma( startIdx-backFast, endIdx, inReal, optInFastPeriod, optInMAType,
      &fastBeg, &fastNb, tempBuffer );
   retCode = ma( startIdx-backSlow, endIdx, inReal, optInSlowPeriod, optInMAType,
      outBegIdx, outNBElement, tempBuffer2 );
   nb = endIdx - startIdx + 1;
   for( i=0; i < nb; i++ )
      outReal[i] = tempBuffer[i] - tempBuffer2[i];
   *outBegIdx = startIdx;
   *outNBElement = nb;",
    );
    assert_classifier_verdict(
        &f,
        r#"accepted ["tap lagdepth.tempBuffer.1.t0 Oldest <- (tempBuffer,1) map0 tempBuffer[i]", "tap lagdepth.tempBuffer2.1.t0 Oldest <- (tempBuffer2,1) map0 tempBuffer2[i]"]"#,
        "each ma starts max(its period - the other's, 0) bars before startIdx, so with its begin presumed \
         at that start its cursor read is that many bars behind: a parameter join that is 0 for one leg \
         at any parameter set, so neither tap is ge1",
    );
}

/// CRSI's metadata over a producer writing one element per bar from the bar
/// before startIdx, then an rsi over it into the output.
const CRSI_PRODUCER: &str = r#"
TA_RetCode crsi( int startIdx, int endIdx, const double inReal[],
   int optInTimePeriod, int optInStreakPeriod, int optInRankPeriod,
   int *outBegIdx, int *outNBElement, double outReal[] )
{
   double *tempX;
   TA_RetCode retCode;
   int lookbackTotal, anchorIdx, today, outIdx, i, lb, b, nbM;

   lookbackTotal = crsi_lookback( optInTimePeriod, optInStreakPeriod, optInRankPeriod );
   if( startIdx < lookbackTotal )
      startIdx = lookbackTotal;
   if( startIdx > endIdx )
   {
      *outBegIdx = 0;
      *outNBElement = 0;
      return TA_SUCCESS;
   }
   lb = rsi_lookback( optInStreakPeriod );
   anchorIdx = startIdx - 1;
   tempX = malloc((endIdx-anchorIdx+1) * sizeof(double));
   outIdx = 0;
   today = anchorIdx;
   while( today <= endIdx )
   {
      tempX[outIdx++] = inReal[today] * 0.5;
      today++;
   }
   retCode = rsi( 0, outIdx-1, tempX, optInStreakPeriod, &b, &nbM, outReal );
   for( i=0; i < nbM; i++ )
      outReal[i] = outReal[i] + tempX[i+lb] - tempX[i+lb-1];
   free( tempX );
   *outBegIdx = startIdx;
   *outNBElement = nbM;
   return TA_SUCCESS;
}
"#;

/// [`apo_with_tail`] with a map writing a scratch series a later map reads a bar back.
const MAP_OUTPUT_LAGGED: &str = r"
   retCode = ma( startIdx, endIdx, inReal, optInFastPeriod, optInMAType,
      &fastBeg, &fastNb, tempBuffer );
   for( i=0; i < fastNb; i++ )
      tempBuffer2[i] = tempBuffer[i] * 0.5;
   nb = fastNb - 1;
   for( i=0; i < nb; i++ )
      outReal[i] = tempBuffer2[i+1] - tempBuffer2[i];
   *outBegIdx = fastBeg + 1;
   *outNBElement = nb;";

#[test]
fn lagged_bar_input_producer_and_map_output_reads_tap_their_definitions() {
    let input = apo_with_tail(
        r"
   retCode = ma( startIdx, endIdx, inReal, optInSlowPeriod, optInMAType,
      outBegIdx, outNBElement, outReal );
   for( i=0; i < (int)*outNBElement; i++ )
      outReal[i] = outReal[i] - inReal[*outBegIdx+i-1];",
    );
    assert_classifier_verdict(
        &input,
        r#"accepted ["tap lagdepth.inReal.1.t0 ge1 Oldest <- (inReal,1) map0 inReal[*outBegIdx+i-1]"]"#,
        "*outBegIdx is the ma's begin B and the map runs endIdx - B + 1 times, so inReal[*outBegIdx+i-1] \
         is (endIdx + 1) - (endIdx - B + 1) - (B - 1) = 1 behind, exact",
    );
    assert_classifier_verdict(
        &load_with_source("crsi", CRSI_PRODUCER),
        r#"accepted ["tap lagdepth.tempX.1.t0 ge1 Oldest <- (tempX,1) map0 tempX[i+lb-1]"]"#,
        "the producer holds N = endIdx - startIdx + 2 elements, one per bar. The rsi starts at 0, at or \
         before its lookback lb, so it begins at lb and the map runs N - lb times: tempX[i+lb] is 0 \
         behind and tempX[i+lb-1] 1, exact",
    );
    assert_classifier_verdict(
        &apo_with_tail(MAP_OUTPUT_LAGGED),
        r#"accepted ["tap lagdepth.tempBuffer2.1.t0 ge1 Oldest <- (tempBuffer2,1) map1 tempBuffer2[i]"]"#,
        "the first map defines tempBuffer2 with fastNb elements and the second runs fastNb - 1 times: \
         tempBuffer2[i+1] is 0 behind and tempBuffer2[i] 1, exact",
    );
}

/// Every `cur_` identifier `body` names, field accesses aside.
fn cur_uses(body: &str) -> std::collections::BTreeSet<String> {
    let mut out = std::collections::BTreeSet::new();
    let mut rest = body;
    while let Some(at) = rest.find("cur_") {
        let before = &rest[..at];
        let boundary = !before.ends_with(|c: char| c.is_alphanumeric() || c == '_')
            && !before.ends_with("->")
            && !before.ends_with('.');
        let name: String =
            rest[at..].chars().take_while(|c| c.is_alphanumeric() || *c == '_').collect();
        if boundary {
            out.insert(name.clone());
        }
        rest = &rest[at + name.len()..];
    }
    out
}

/// The names each line of `body` declares after one of `heads`.
fn declared_after(body: &str, heads: &[&str]) -> std::collections::BTreeSet<String> {
    body.lines()
        .filter_map(|l| heads.iter().find_map(|h| l.trim_start().strip_prefix(h)))
        .map(|rest| rest.chars().take_while(|c| c.is_alphanumeric() || *c == '_').collect())
        .collect()
}

/// The brace-balanced body of the first definition whose signature starts at `needle`.
fn fn_body<'s>(src: &'s str, needle: &str) -> &'s str {
    let at = src.find(needle).unwrap_or_else(|| {
        let word = needle.trim_end_matches('(').rsplit(' ').next().unwrap_or(needle);
        let near: Vec<&str> = src.lines().filter(|l| l.contains(word)).collect();
        panic!("no {needle}; lines naming {word}:\n{}", near.join("\n"))
    });
    let open = at + src[at..].find('{').expect("a body");
    let mut depth = 0usize;
    for (k, c) in src[open..].char_indices() {
        match c {
            '{' => depth += 1,
            '}' => {
                depth -= 1;
                if depth == 0 {
                    return &src[open..=open + k];
                }
            }
            _ => {}
        }
    }
    panic!("{needle}: unbalanced body")
}

#[test]
fn every_cur_storage_a_step_uses_is_declared() {
    let f = apo_with_tail(MAP_OUTPUT_LAGGED);
    let cp = streaming::analyze_composed(&f, lookup()).expect("accepted");
    let stored: std::collections::BTreeSet<String> =
        (0..cp.defs.len()).map(|d| cp.def_storage(d)).filter(|s| s.starts_with("cur_")).collect();
    assert!(stored.contains("cur_tempBuffer2"), "{stored:?}");
    let planned: std::collections::BTreeSet<String> = cp.cur_scalars().iter().map(|n| format!("cur_{n}")).collect();
    assert_eq!(planned, stored);

    let enums = crate::common::load_enums();
    let helpers = crate::common::make_helpers();
    let c = ta_codegen_lib::backends::c_stream::generate(&f, &enums, lookup(), helpers);
    let rust = ta_codegen_lib::backends::rust_lang::generate(&f, &enums, lookup(), helpers);
    for (text, sig, heads) in [
        (&c, "TA_RetCode TA_APO_StepImpl( struct", &["double ", "int "][..]),
        (&rust, "fn apo_step_impl(", &["let mut "][..]),
    ] {
        let body = fn_body(text, sig);
        let used = cur_uses(body);
        assert!(used.contains("cur_tempBuffer2"), "{sig}: {used:?}");
        let declared = declared_after(body, heads);
        let missing: Vec<&String> = used.difference(&declared).collect();
        assert!(missing.is_empty(), "{sig} uses undeclared {missing:?}:\n{body}");
    }
}

/// [`apo_with_tail`] with a map writing a scratch series a later map reads at the same bar.
const MAP_SCRATCH: &str = r"
   retCode = ma( startIdx, endIdx, inReal, optInFastPeriod, optInMAType,
      &fastBeg, &fastNb, tempBuffer );
   for( i=0; i < fastNb; i++ )
      tempBuffer2[i] = (int)(tempBuffer[i] * 0.5);
   for( i=0; i < fastNb; i++ )
      outReal[i] = tempBuffer2[i] / 2;
   *outBegIdx = fastBeg;
   *outNBElement = fastNb;";

/// SQZMOM's signature over a tail whose second map reads its integer output
/// through `read`, before a third map writes that output again.
fn sqzmom_reading_its_int_output(read: &str) -> FuncDef {
    let src = format!(
        r"
TA_RetCode sqzmom(int startIdx, int endIdx,
   const double inHigh[], const double inLow[], const double inClose[],
   int optInBBPeriod, double optInNbDev, int optInKCPeriod,
   double optInFactorWide, double optInFactorNormal, double optInFactorNarrow,
   int *outBegIdx, int *outNBElement, double outMomentum[], int outSqueeze[])
{{
   TA_RetCode retCode;
   int tempBeg, tempNb, nb, i;
   double *tempSD;

   tempSD = malloc((endIdx-startIdx+1) * sizeof(double));
   retCode = sma( startIdx, endIdx, inClose, optInBBPeriod,
      &tempBeg, &tempNb, tempSD );
   for( i=0; i < tempNb; i++ )
   {{
      if( tempSD[i] > 0.0 )
         outSqueeze[i] = 3;
      else
         outSqueeze[i] = 0;
   }}
   nb = tempNb - 1;
   for( i=0; i < nb; i++ )
      outMomentum[i] = tempSD[i+1] - {read} / 2;
   for( i=0; i < nb; i++ )
   {{
      if( tempSD[i+1] > 0.0 )
         outSqueeze[i] = 3;
      else
         outSqueeze[i] = 0;
   }}
   free( tempSD );
   *outBegIdx = tempBeg + 1;
   *outNBElement = nb;
   return TA_SUCCESS;
}}
"
    );
    load_with_source("sqzmom", &src)
}

#[test]
fn a_step_holds_no_integer_series_in_a_double() {
    assert_classifier_verdict(
        &apo_with_tail(MAP_SCRATCH),
        "accepted []",
        "tempBuffer, tempBuffer2 and outReal each hold fastNb elements and both maps run fastNb \
         times, so every read is 0 behind",
    );
    // Batch divides tempBuffer2[i] / 2 in int; a step holding the series in
    // its double cur_ local would divide in double.
    let src = apo_source(MAP_SCRATCH);
    assert!(src.contains("double *tempBuffer2;"), "the fixture no longer declares tempBuffer2");
    assert_classifier_verdict(
        &load_with_source("apo", &src.replacen("double *tempBuffer2;", "int *tempBuffer2;", 1)),
        "refused: composed intermediate `tempBuffer2` is an integer series: a step holds an \
         intermediate in a double, so it would compute in double where batch computes in int",
        "",
    );
    assert_classifier_verdict(
        &sqzmom_reading_its_int_output("outSqueeze[i+1]"),
        "accepted []",
        "the first map writes outSqueeze with tempNb elements and the second runs nb = tempNb - 1 \
         times, so outSqueeze[i+1] and tempSD[i+1] are 0 behind; the third map writes nb elements \
         of outSqueeze, and both outputs return at nb",
    );
    // outSqueeze[i] is tempNb - nb = 1 behind, exact: a Tap on (outSqueeze,1).
    // Its delay line holds doubles, so `tap / 2` would divide in double where
    // batch divides in int.
    assert_classifier_verdict(
        &sqzmom_reading_its_int_output("outSqueeze[i]"),
        "refused: composed stream delays integer series `outSqueeze`: a delay line holds \
         doubles, so a lagged read would compute in double where batch computes in int",
        "",
    );
}

/// The series each replayable free in `cp` frees.
fn freed_series(cp: &streaming::ComposedPlan<'_>) -> Vec<String> {
    cp.series_frees
        .iter()
        .filter_map(|sf| {
            let mut freed = None;
            streaming::walk_stmt_exprs(&sf.stmt, &mut |e| {
                if let ir::Expr::FuncCall(f, args) = e {
                    if let (true, Some(ir::Expr::Var(v))) = (f == "free", args.first()) {
                        freed.get_or_insert_with(|| v.clone());
                    }
                }
            });
            freed
        })
        .collect()
}

#[test]
fn a_series_only_a_map_writes_is_freed_on_inserted_failure_returns() {
    let f = apo_with_tail(MAP_SCRATCH);
    let cp = streaming::analyze_composed(&f, lookup()).expect("accepted");
    assert_eq!(freed_series(&cp), ["tempBuffer", "tempBuffer2"]);
    let src = apo_source(MAP_SCRATCH);
    let free = "   free(tempBuffer2);\n";
    let alloc = "   tempBuffer2 = malloc((endIdx-startIdx+1) * sizeof(double));\n";
    let first_map = "   for( i=0; i < fastNb; i++ )\n      tempBuffer2[i]";
    assert!(src.contains(free) && src.contains(alloc) && src.contains(first_map), "fixture edits do not apply");
    let never_freed = src.replacen(free, "", 1);
    // Sub-call 0's failure return runs before the allocation.
    let allocated_in_tail = src.replacen(alloc, "", 1).replacen(first_map, &format!("{alloc}{first_map}"), 1);
    for (why, src) in [("never freed", never_freed), ("allocated in the tail", allocated_in_tail)] {
        match streaming::analyze_composed(&load_with_source("apo", &src), lookup()) {
            Err(StreamError::Unsupported(m))
                if m.contains("composed series `tempBuffer2` is heap-allocated but the tail has no replayable free") => {}
            other => panic!("{why}: want the no-replayable-free refusal, got {:?}", other.map(|cp| freed_series(&cp))),
        }
    }
}

#[test]
fn a_map_reads_back_what_it_wrote_on_one_path_through_a_must_alias_name() {
    use streaming::Consumer;
    let f = apo_with_tail(
        r"
   res = outReal;
   retCode = ma( startIdx, endIdx, inReal, optInFastPeriod, optInMAType,
      &fastBeg, &fastNb, tempBuffer );
   retCode = ma( startIdx, endIdx, inReal, optInSlowPeriod, optInMAType,
      outBegIdx, outNBElement, outReal );
   offset = fastNb - *outNBElement;
   if( optInFastPeriod > 3 )
   {
      for( i=0; i < (int)*outNBElement; i++ )
      {
         if( tempBuffer[i+offset] > 0.0 )
            outReal[i] = tempBuffer[i+offset];
         outReal[i] = res[i] - tempBuffer[i+offset];
      }
   }
   else
   {
      for( i=0; i < (int)*outNBElement; i++ )
         outReal[i] = tempBuffer[i+offset] - outReal[i];
   }",
    );
    // Batch reads res[i] after a write on one path and the slow ma's value on the
    // other; the step runs the same statements over one storage, seeded with that
    // value. Every read is 0 behind.
    assert_classifier_verdict(&f, "accepted []", "the APO tail's delays, in each arm");
    let cp = streaming::analyze_composed(&f, lookup()).expect("accepted above");
    assert!(
        cp.edges.iter().any(|e| matches!(&e.consumer,
            Consumer::MapPrior { arm: Some((true, _)), buffer, .. } if buffer == "outReal")),
        "the then-arm reads outReal's prior: {:?}",
        cp.edges
    );
    let si = cp.steps.iter().position(|s| matches!(s, streaming::UpdateStep::Map { .. })).expect("a map step");
    let lowered = format!("{:?}", streaming::lower_map_step(&cp, si, &StepNames));
    assert!(lowered.contains("Var(\"cur_outReal\")") && !lowered.contains("\"res\""), "{lowered}");
}

#[test]
fn a_tap_fuses_in_the_step_where_its_read_fuses_in_batch() {
    let f = apo_with_tail(
        r"
   retCode = ma( startIdx, endIdx, inReal, optInFastPeriod, optInMAType,
      &fastBeg, &fastNb, tempBuffer );
   nb = fastNb - 1;
   for( i=0; i < nb; i++ )
      outReal[i] = 0.5*tempBuffer[i] + tempBuffer[i+1];
   *outBegIdx = fastBeg + 1;
   *outNBElement = nb;",
    );
    assert_classifier_verdict(
        &f,
        r#"accepted ["tap lagdepth.tempBuffer.1.t0 ge1 Oldest <- (tempBuffer,1) map0 tempBuffer[i]"]"#,
        "tempBuffer[i] is fastNb - (fastNb - 1) = 1 behind, exact",
    );
    use ta_codegen_lib::backends;
    let enums = crate::common::load_enums();
    let (reg, helpers) = (lookup(), crate::common::make_helpers());
    let rust = backends::rust_lang::generate(&f, &enums, reg, helpers);
    let rust_batch = if rust.contains("fn apo_impl_scalar(") { "fn apo_impl_scalar(" } else { "fn apo_impl(" };
    let rows = [
        ("c", backends::c::generate(&f, &enums, reg, helpers), "TA_RetCode TA_APO(", "TA_RetCode TA_APO_StepImpl( struct", "fma("),
        ("rust", rust.clone(), rust_batch, "fn apo_step_impl(", ".mul_add("),
        ("java", backends::java::generate(&f, &enums, reg, helpers), "RetCode apoImpl(", "private void apoStepImpl(", "Math.fma("),
        ("csharp", backends::csharp::generate(&f, &enums, reg, helpers), "internal RetCode ApoImpl(", "private void ApoStepImpl(", "Math.FusedMultiplyAdd("),
        ("ts", backends::ts::generate(&f, &enums, reg, helpers), "export function apoImpl(", "function apoStepImpl(", "fma("),
    ];
    for (lang, text, batch, step, marker) in &rows {
        let fused = fn_body(text, batch).matches(marker).count();
        let stepped = fn_body(text, step).matches(marker).count();
        assert!(fused > 0, "{lang}: the batch map fuses nothing");
        assert_eq!(stepped, fused, "{lang}: the step fuses {stepped} sites, batch {fused}");
    }
}
