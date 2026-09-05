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
fn lookup() -> ta_codegen_lib::registry::Registry {
    ta_codegen_lib::registry::Registry::from_dir(&input_dir())
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
    let plan = streaming::validate_streamable(&f, &lookup()).expect("MINUS_DM derives a plan");
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
    let plan = streaming::validate_streamable(&f, &lookup()).expect("MINUS_DI derives a plan");
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
    let plan = streaming::validate_streamable(&f, &lookup()).expect("TRIMA derives a plan");
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
        assert_eq!(alt.api, ir::ApiClaim::Stream);
        assert_eq!(alt.lang, ir::LangClaim::AllLanguages);

        for lang in ir::ALL_LANGS {
            let resolved = f.resolved_for(lang);
            let plan = streaming::validate_streamable(&resolved, &lookup())
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
            streaming::validate_streamable(&f, &lookup()).is_err(),
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
fn rsi_memmove_identity_and_seed_boundary() {
    // RSI's period==1 memmove path is recognized as the identity fast path,
    // and its Metastock seed exit (output write, then a guarded success
    // return) is flagged: Open honestly rejects at exactly lookback+1 there.
    let f = load("rsi");
    let m = streaming::analyze(&f).expect("RSI analyzes");
    assert!(m.identity.is_some(), "memmove identity path");
    assert!(m.seed_boundary, "Metastock seed boundary flagged");
    let f2 = load("avgdev");
    let m2 = streaming::analyze(&f2).expect("AVGDEV analyzes");
    assert!(!m2.seed_boundary, "pure no-data guard is not a seed boundary");
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
                streaming::validate_streamable(&func.resolved_for(lang), &lk)
                    .unwrap_or_else(|e| panic!("[{}] {e}", lang.as_str()));
            }
            checked += 1;
        }
    }
    assert!(checked >= 136, "expected 136+ declared functions, saw {checked}");
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
    let f = load("cdl3blackcrows");
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
    let f = load("cdl3blackcrows");
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
    // what makes the ring deep enough to serve the window read.
    let f = load("cdladvanceblock");
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
    // insufficient. `ta_CDLADVANCEBLOCK.c:186` also holds a window read and a
    // ring read of the same Near setting in ONE statement, so the two folds
    // must be told apart by index form rather than by shape.
    let f = load("cdladvanceblock");
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
        streaming::validate_streamable(&f, &lookup()).is_ok(),
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
        streaming::validate_streamable(&f, &lookup()),
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
    assert!(streaming::validate_streamable(&f, &lookup()).is_ok());
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
        streaming::validate_streamable(&f, &lookup()).is_err(),
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
    ];
    // Not-yet-streamable MAType functions (deep blockers). MUST ONLY SHRINK.
    // NOW EMPTY: MAMA streamed in M7c (it is an ordinary HT function — WMA ring +
    // Hilbert arrays + `today % 2` parity + the two outputs mama/fama in an output
    // gate — covered by the same strip_cursor_output_gate + carry_cursor_parity
    // normalizations, no circbuf/window, no `startIdx` read in its steady loop).
    // Every MAType function now streams. MA's *dispatch* also streams every arm,
    // including MAType_MAMA: FAMA is a nullable output (issue #125), so MA's arm
    // forwards the MAMA line and passes NULL for FAMA — a supported trailing-NULL
    // delegation (pinned by ma_derives_dispatch_plan). No blockers remain.
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
    // joined when its stream landed (M6c); MAMA joined when `outFAMA` became a
    // nullable output (issue #125) — MA's arm forwards the MAMA line to outReal
    // and passes NULL for FAMA, a clean trailing-NULL delegation. Every arm now
    // streams.
    let f = load("ma");
    // The YAML flag itself is load-bearing: losing it would silently drop
    // TA_MA_Stream from every generated surface while all gates stay green
    // (backend_suite force-sets the flag for shape pinning; the regtest
    // set-equality check passes when both sides lose the stream together).
    assert!(f.streaming, "ma.yaml must carry the stream flag");
    let lk = lookup();
    let plan = streaming::validate_streamable(&f, &lk).expect("MA derives a plan");
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
         "rma"],
        "every arm streams: single-output MAs plus MAMA via its nullable FAMA \
         (TRIMA joined in M6c, MAMA via nullable outputs in #125, HMA via the \
         dual-mode buffer union in #141)"
    );
    // No reject arms remain: HMA (#139) was the last, flipped to supported by
    // its YAML `stream` flag + the dual-mode per-arm buffer union (#141).
    let rejected: Vec<&str> = dp.unsupported_labels();
    assert!(rejected.is_empty(), "no reject arms remain, got {rejected:?}");
    // The MAMA arm forwards mama's output 0 (the MAMA line) into MA's single
    // output and discards output 1 (FAMA, nullable) as NULL. This map is what
    // the C emitter renders as `TA_MAMA_*( ..., outReal, NULL )`.
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

/// The trailing-NULL delegation is sound ONLY when the discarded callee output
/// is genuinely nullable. If MAMA's FAMA were not nullable, MA's
/// `mama(..., outReal, NULL)` arm must FAIL the delegation shape — discarding a
/// non-nullable output would silently drop a real result — and, since `mama` is
/// stream-flagged, the whole dispatch becomes a HARD gate error, never a silent
/// reject the verify precheck would then bless. This proves the `out_nullable`
/// guard in `delegation_opt_args` is load-bearing (neuter it → MA streams here).
#[test]
fn dispatch_rejects_null_discard_of_non_nullable_output() {
    use ta_codegen_lib::streaming::{CalleeLookup, CalleeSig};
    struct FamaNotNullable(ta_codegen_lib::registry::Registry);
    impl CalleeLookup for FamaNotNullable {
        fn callee(&self, name: &str) -> Option<CalleeSig> {
            let mut sig = self.0.callee(name)?;
            if name == "mama" {
                // Pretend FAMA (and MAMA) are non-nullable.
                sig.out_nullable = vec![false; sig.n_outputs];
            }
            Some(sig)
        }
    }
    let f = load("ma");
    assert!(
        streaming::validate_streamable(&f, &FamaNotNullable(lookup())).is_err(),
        "NULL-discarding a non-nullable output must not be a supported delegation"
    );
    // Sanity: with the real (nullable) FAMA the same dispatch DOES stream.
    assert!(
        streaming::validate_streamable(&f, &lookup()).is_ok(),
        "MA streams with mama's real nullable FAMA"
    );
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
                out_nullable: vec![false],
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
    let lk = TrimaUnflagged(&real);
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
        streaming::validate_streamable(&f, &lookup()).is_ok(),
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
        let err = streaming::analyze_dispatch(&f, &lookup()).unwrap_err();
        assert!(
            matches!(err, StreamError::Unsupported(ref m)
                if m.contains("unrecognized top-level statement")),
            "{what}: expected the catch-all hard error, got: {err}"
        );
    }

    // Control: unmutated, the same body analyzes.
    let f = load("ma");
    assert!(
        streaming::analyze_dispatch(&f, &lookup()).is_ok(),
        "the unmutated dispatch body must still analyze"
    );

    // A SECOND guard is not admitted either — the recognizer skips one index,
    // and anything else at the top level is still the hard error.
    let mut f = load("ma");
    let idx = empty_range_guard_idx(&f).expect("ma carries the guard");
    let dup = f.body[idx].clone();
    f.body.insert(idx, dup);
    let err = streaming::analyze_dispatch(&f, &lookup()).unwrap_err();
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
    let err = streaming::analyze_dispatch(&f, &lookup()).unwrap_err();
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
    let plan = streaming::validate_streamable(&f, &lk).expect("STOCH derives a plan");
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
        streaming::UpdateStep::Align { dst, src } if dst == "outSlowK" && src == "tempBuffer"
    ));
}

#[test]
fn stochf_derives_composed_plan() {
    let f = load("stochf");
    assert!(f.streaming, "stochf.yaml must carry the stream flag");
    let plan = streaming::validate_streamable(&f, &lookup()).expect("STOCHF derives a plan");
    let streaming::StreamPlan::Composed(cp) = plan else {
        panic!("STOCHF must derive a composed plan");
    };
    assert_eq!(cp.subs.len(), 1);
    assert_eq!(cp.subs[0].dsts, ["outFastD"]);
    assert!(matches!(
        &cp.steps[1],
        streaming::UpdateStep::Align { dst, src } if dst == "outFastK" && src == "tempBuffer"
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
    let plan = streaming::validate_streamable(&f, &lookup()).expect("BBANDS derives a plan");
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
                out_nullable: vec![false],
            })
        }
    }
    let f = load("stoch");
    let err = streaming::analyze_composed(&f, &MaUnflagged).unwrap_err();
    assert!(
        matches!(err, StreamError::UnsupportedCall(ref m) if m.contains("no stream")),
        "expected no-stream sub-call error, got: {err}"
    );
}

#[test]
fn stochrsi_derives_loopless_composed_plan() {
    // STOCHRSI has no producer loop: a pure sub-call pipeline
    // rsi(inReal) -> tempRSIBuffer, then stochf(tempRSIBuffer x3) -> outFastK/D.
    let f = load("stochrsi");
    assert!(f.streaming, "stochrsi.yaml must carry the stream flag");
    let plan = streaming::validate_streamable(&f, &lookup()).expect("STOCHRSI derives a plan");
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
    let plan = streaming::validate_streamable(&f, &lookup()).expect("STDDEV derives a plan");
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
    let err = streaming::analyze_composed(&f, &lookup()).unwrap_err();
    assert!(
        matches!(err, StreamError::Unsupported(ref m)
            if m.contains("success-guard") && m.contains("flatten")),
        "G2 must guide to the error-guard flatten, got: {err}"
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
    let err = streaming::analyze_composed(&f, &lookup()).unwrap_err();
    assert!(
        matches!(err, StreamError::Unsupported(ref m) if m.contains("multi-cursor")),
        "G1 must guide to the single-cursor begIdx-offset form, got: {err}"
    );
}

#[test]
fn apo_derives_composed_plan_with_same_bar_offset_map() {
    // The shipped (Flat-B) APO: fast MA -> tempBuffer, slow MA -> outReal, then
    // a single-cursor combine map reading tempBuffer[i + offset] where
    // `offset = fastNb - *outNBElement` is proven a same-bar element-count
    // difference (both sub-calls share endIdx).
    let f = load("apo");
    let plan = streaming::validate_streamable(&f, &lookup()).expect("APO derives a plan");
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
    let plan = streaming::validate_streamable(&f, &lookup()).expect("PPO derives a plan");
    let streaming::StreamPlan::Composed(cp) = plan else {
        panic!("PPO must derive a composed plan");
    };
    assert_eq!(cp.subs.len(), 2);
    assert_eq!(cp.subs[1].dsts, ["outReal"]);
    assert!(matches!(cp.steps[2], streaming::UpdateStep::Map { .. }));
    assert!(cp.map_temps.iter().any(|(n, _)| n == "tempReal"));
}

#[test]
fn adxr_derives_composed_plan_with_sub_lag_ring() {
    // ADXR = adx(inHigh,inLow,inClose) over an extended range -> adx buffer,
    // then outReal[k] = (adx[k+(period-1)] + adx[k])/2: the current ADX plus the
    // ADX from (period-1) bars ago. That self-lag over the sub-output is a lag
    // ring (a param depth), NOT a same-bar combine.
    let f = load("adxr");
    let plan = streaming::validate_streamable(&f, &lookup()).expect("ADXR derives a plan");
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
    // The lag ring over the ADX sub-output.
    assert_eq!(cp.sub_lag_rings.len(), 1, "one sub-output lag ring");
    assert_eq!(cp.sub_lag_rings[0].series, "adx");
    // The lag depth is the parameter expression optInTimePeriod - 1.
    use ta_codegen_lib::ir::{BinOp, Expr};
    match &cp.sub_lag_rings[0].lag {
        Expr::BinOp(l, BinOp::Sub, r) => {
            assert!(matches!(l.as_ref(), Expr::Var(v) if v == "optInTimePeriod"));
            assert!(matches!(r.as_ref(), Expr::IntLiteral(1)));
        }
        other => panic!("lag depth must be optInTimePeriod - 1, got {other:?}"),
    }
}

#[test]
fn data_dependent_lag_offset_rejected() {
    // A lag ring has a FIXED capacity sized at open, so its depth must be a
    // parameter expression. A data-dependent offset (here `*outNBElement / 2`,
    // varying with history length) cannot be a ring and must be refused —
    // otherwise the analyzer would size a ring from a value it cannot know
    // at open. This pins the param-purity guard on the lag depth.
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
    let err = streaming::analyze_composed(&f, &lookup()).unwrap_err();
    assert!(
        matches!(err, StreamError::Unsupported(ref m) if m.contains("same-bar shift")),
        "a data-dependent lag must be refused (not sized into a fixed ring), got: {err}"
    );
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
    let err = streaming::analyze_composed(&f, &lookup()).unwrap_err();
    assert!(
        matches!(err, StreamError::Unsupported(ref m)
            if m.contains("element-count difference") && m.contains("same-bar")),
        "begIdx offset must be refused with the count-difference guidance, got: {err}"
    );
}

#[test]
fn mismatched_endidx_combine_rejected() {
    // The count-difference `nb(a) - nb(b)` equals the begIdx shift ONLY when the
    // two producers share an endIdx. Here the slow MA runs over `endIdx - 1`, so
    // `offset = fastNb - *outNBElement` still satisfies the receiver-provenance
    // check but is NOT a same-bar shift (the windows end on different bars). The
    // shared-endIdx clause must reject it — otherwise the emitter's index-blind
    // rewrite would ship a silently-lagged stream. This pins that clause (its
    // provenance sibling is pinned by the begIdx-form test above).
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
    let err = streaming::analyze_composed(&f, &lookup()).unwrap_err();
    assert!(
        matches!(err, StreamError::Unsupported(ref m) if m.contains("same-bar")),
        "combine over sub-calls with different endIdx must be refused as not same-bar, got: {err}"
    );
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
        let Ok(streaming::StreamPlan::Composed(cp)) = streaming::validate_streamable(&f, &lk) else {
            continue;
        };
        let outs: Vec<String> = f.outputs.iter().map(|o| o.name.clone()).collect();
        if cp
            .subs
            .iter()
            .any(|s| s.dsts.iter().any(|d| outs.contains(d)))
        {
            found.push(name.to_uppercase());
        }
    }
    // Membership alone would not tell the next author WHICH invariant to keep:
    // no two of these are safe for the same reason. The reason is recorded with
    // each entry and printed on failure. (Reasons proved by kevinlincg, #205.)
    let expected: [(&str, &str); 9] = [
        ("APO", "sub-call uses optInSlowPeriod and the body swaps so slow == max(slow,fast); \
                 the swap is load-bearing -- see apo_family_period_swap_is_a_write_bound_precondition"),
        ("KC", "the moving average is entered at exactly ema_lookback over a typical-price buffer \
                 that begins ema_lookback bars before startIdx, so it clamps nothing: its first \
                 output lands on startIdx and its count is endIdx-startIdx+1, the same expression \
                 KC returns. Equal by construction, not by a lookback identity between two callees"),
        ("MACDEXT", "the body RUNTIME-CHECKS the premise (outNbElement1 == endIdx-startIdx+1+lookbackSignal) \
                 and bails otherwise, so signal count == N_MACDEXT"),
        ("PPO", "as APO -- the slow/fast swap is the precondition"),
        ("PVO", "as APO -- the slow/fast swap is the precondition"),
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
/// None of the seven flagged functions (ACOS, ASIN, LN, LOG10, SQRT, DIV, VWMA)
/// is composed by anything today, so the gate is dormant against the shipped
/// corpus — which is exactly why it needs a test that can see it fire. The
/// flag is injected rather than written to `ma.yaml`, so the corpus is untouched.
#[test]
fn nan_inf_callee_is_refused() {
    let reg = lookup();
    let bbands = load("bbands"); // composes MA (sub0) and STDDEV (sub1)
    let sma = load("sma"); // loop tier: composes nothing

    // Control: as shipped, MA is finite-output and BBANDS composes it happily.
    assert!(
        streaming::validate_streamable(&bbands, &reg).is_ok(),
        "BBANDS must derive a plan against the unmodified corpus, or the probe below \
         proves nothing"
    );

    let flagged = FlagOneCallee { inner: &reg, flagged: "ma" };
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
    let flagged_sma = FlagOneCallee { inner: &reg, flagged: "sma" };
    let err = streaming::validate_streamable(&ma, &flagged_sma)
        .expect_err("MA dispatches to SMA, so a nan_inf_output SMA must be refused");
    assert!(
        err.contains("nan_inf_output") && err.contains("composing sma,") && err.contains("MA"),
        "the dispatch arm must name the flag, the callee and the function: {err}"
    );
    // Control for both: unflagged, they derive plans as usual.
    assert!(streaming::validate_streamable(&mavp, &reg).is_ok(), "MAVP plans normally");
    assert!(streaming::validate_streamable(&ma, &reg).is_ok(), "MA plans normally");
}
