//! Streaming-analysis integration tests over the REAL ta_codegen/input corpus.
//!
//! These pin the stage-1 tier boundary: which functions analyze as T1/T2,
//! and — just as load-bearing — which are rejected and why. A batch rewrite
//! that changes a function's stream shape fails here before it fails in CI.

use std::path::{Path, PathBuf};

use ta_codegen_lib::ir::{FuncDef, StreamTier};
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
fn atr_period1_delegation_rejected() {
    assert!(matches!(
        streaming::analyze(&load("atr")),
        Err(StreamError::Unsupported(m)) if m.contains("return path")
    ));
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
            streaming::validate_streamable(&func, &lk).unwrap_or_else(|e| panic!("{e}"));
            checked += 1;
        }
    }
    assert!(checked >= 136, "expected 136+ declared functions, saw {checked}");
}

/* ---- CDL tranche: candle helpers, offset rings, array state ---- */

#[test]
fn cdldoji_is_t3_with_plain_ohlc_ring() {
    let f = load("cdldoji");
    let m = streaming::analyze(&f).expect("CDLDOJI analyzes");
    assert_eq!(m.tier, StreamTier::T3);
    assert_eq!(m.rings().len(), 1);
    let r = &m.rings()[0];
    assert_eq!(r.var, "BodyDojiTrailingIdx");
    assert_eq!(r.arrays, ["inOpen", "inHigh", "inLow", "inClose"]);
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

#[test]
fn cdl3blackcrows_var_offset_ring_window_and_array_state() {
    // in[ShadowVeryShortTrailingIdx - totIdx] with for(totIdx=2; totIdx>=0;):
    // ring back = counter max (2), rescan window on totIdx, and the per-candle
    // totals carry as fixed-size array state.
    let f = load("cdl3blackcrows");
    let m = streaming::analyze(&f).expect("CDL3BLACKCROWS analyzes");
    let r = m
        .rings()
        .iter()
        .find(|r| r.var == "ShadowVeryShortTrailingIdx")
        .expect("ShadowVeryShort ring");
    assert!(r.back >= 2, "counter-offset ring, got {}", r.back);
    assert!(!m.windows().is_empty(), "in[i - totIdx] rescan window");
    assert!(
        m.state
            .iter()
            .any(|(n, t)| n == "ShadowVeryShortPeriodTotal"
                && matches!(t, ta_codegen_lib::ir::VarType::RealArray(_))),
        "fixed-size array state"
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
    // totIdx is bound by three loops (2, 1, 2 inclusive) — the window keeps
    // the widest literal bound instead of rejecting.
    let f = load("cdladvanceblock");
    let m = streaming::analyze(&f).expect("CDLADVANCEBLOCK analyzes");
    let w = m.windows().iter().find(|w| w.var == "totIdx").expect("totIdx window");
    assert!(
        matches!(w.cap, ta_codegen_lib::ir::Expr::IntLiteral(3)),
        "widest inclusive bound 2 -> exclusive cap 3, got {:?}",
        w.cap
    );
}

#[test]
fn ultosc_analyzes_t3() {
    // Unlocked by the descending-inclusive window form.
    let f = load("ultosc");
    let m = streaming::analyze(&f).expect("ULTOSC analyzes");
    assert_eq!(m.tier, StreamTier::T3);
}

#[test]
fn cdlhikkake_rejected_at_transition_build() {
    // Saves bar indices (patternIdx = i): analysis passes but the transition
    // cannot be built — the wall the census gate must keep unseeded.
    let f = load("cdlhikkake");
    assert!(streaming::analyze(&f).is_ok(), "analysis alone passes");
    assert!(
        streaming::validate_streamable(&f, &lookup()).is_err(),
        "transition build must reject the cursor leak"
    );
}

/* ---- TC composed tier: dispatch plans ---- */

#[test]
fn ma_derives_dispatch_plan() {
    // MA is the MAType-tagged dispatch over the per-MA streams. The
    // supported-arm set is DERIVED from the callees' YAML stream flags:
    // TRIMA joins automatically when its stream lands; MAMA's arm (dummy
    // FAMA buffer, discarded output) stays a documented Open reject.
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
    assert_eq!(dp.arms.len(), 9, "all nine batch arms recognized");
    let supported: Vec<&str> = dp
        .arms
        .iter()
        .filter(|a| a.supported)
        .map(|a| a.callee.as_str())
        .collect();
    assert_eq!(
        supported,
        ["sma", "ema", "wma", "dema", "tema", "kama", "t3"],
        "supported arms follow the callee stream flags, in batch order"
    );
    // Labels are TA-stripped in the IR (the C renderer restores the prefix).
    let rejected: Vec<&str> = dp.unsupported_labels();
    assert_eq!(rejected, ["MAType_TRIMA", "MAType_MAMA"]);
    // T3's arm forwards the fixed vfactor literal positionally.
    let t3 = dp.arms.iter().find(|a| a.callee == "t3").unwrap();
    assert_eq!(t3.opt_args.len(), 2, "period + literal 0.7 vfactor");
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
    // indicator call is unflagged (trima) but which then whole-range
    // delegates to a stream-flagged callee (dema) must be a hard gate
    // error — never a reject arm the verify precheck would bless.
    use ta_codegen_lib::ir::{Expr, Statement};
    let mut f = load("ma");
    let lk = lookup();
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
fn composed_hard_errors_when_subcall_callee_lacks_stream() {
    // A composed function only streams when every sub-call does: a callee
    // without a stream is a loud error (actionable census line), never a
    // silent skip — STOCHRSI stays blocked this way until its pieces land.
    struct MaUnflagged;
    impl streaming::CalleeLookup for MaUnflagged {
        fn callee(&self, name: &str) -> Option<streaming::CalleeSig> {
            (name == "ma").then_some(streaming::CalleeSig {
                streaming: false,
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
    // The map references tempReal as a step-local temp.
    assert!(cp.map_temps.iter().any(|(n, _)| n == "tempReal"));
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
