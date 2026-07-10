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
    assert!(m.counter.is_some());
}

#[test]
fn sma_is_t3_ring() {
    let f = load("sma");
    let m = streaming::analyze(&f).expect("SMA analyzes");
    assert_eq!(m.tier, StreamTier::T3);
    assert_eq!(m.rings.len(), 1);
    assert_eq!(m.rings[0].arrays, ["inReal"]);
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
    assert!(checked >= 134, "expected 134+ declared functions, saw {checked}");
}

/* ---- CDL tranche: candle helpers, offset rings, array state ---- */

#[test]
fn cdldoji_is_t3_with_plain_ohlc_ring() {
    let f = load("cdldoji");
    let m = streaming::analyze(&f).expect("CDLDOJI analyzes");
    assert_eq!(m.tier, StreamTier::T3);
    assert_eq!(m.rings.len(), 1);
    let r = &m.rings[0];
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
        .rings
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
        .rings
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
        .rings
        .iter()
        .find(|r| r.var == "ShadowVeryShortTrailingIdx")
        .expect("ShadowVeryShort ring");
    assert!(r.back >= 2, "counter-offset ring, got {}", r.back);
    assert!(!m.windows.is_empty(), "in[i - totIdx] rescan window");
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
    assert_eq!(m.rings.len(), 2, "BodyLong + ShadowVeryShort rings");
}

#[test]
fn cdladvanceblock_merges_window_bounds_to_widest() {
    // totIdx is bound by three loops (2, 1, 2 inclusive) — the window keeps
    // the widest literal bound instead of rejecting.
    let f = load("cdladvanceblock");
    let m = streaming::analyze(&f).expect("CDLADVANCEBLOCK analyzes");
    let w = m.windows.iter().find(|w| w.var == "totIdx").expect("totIdx window");
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
    assert!(!m.windows.is_empty(), "reindexed rescan window");
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
    assert_eq!(cp.series, "tempBuffer");
    assert_eq!(cp.producer.tier, StreamTier::T4, "raw %K extrema producer");
    assert_eq!(cp.producer.outputs, ["tempBuffer"]);
    assert_eq!(cp.subs.len(), 2);
    // Sub 0: in-place smoothing of the raw %K; sub 1: %D from smoothed %K.
    assert_eq!(
        (cp.subs[0].callee.as_str(), cp.subs[0].src.as_str(), cp.subs[0].dst.as_str()),
        ("ma", "tempBuffer", "tempBuffer")
    );
    assert_eq!(
        (cp.subs[1].callee.as_str(), cp.subs[1].src.as_str(), cp.subs[1].dst.as_str()),
        ("ma", "tempBuffer", "outSlowD")
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
    assert_eq!(cp.subs[0].dst, "outFastD");
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
fn stochrsi_not_composed_yet() {
    // STOCHRSI has no producer loop (pure sub-call pipeline over the RSI
    // buffer) — a later shape. It must NOT accidentally derive a plan.
    assert!(streaming::validate_streamable(&load("stochrsi"), &lookup()).is_err());
}
