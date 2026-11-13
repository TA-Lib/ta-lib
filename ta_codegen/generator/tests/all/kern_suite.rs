//! The C block kernel (`TA_<N>_KernImpl`): the step over `nbBar` bars with the
//! handle's fields in locals. Its values are compared bitwise only by the
//! prototype harness `generate` writes under `TA_CODEGEN_KERN_HARNESS`.

use crate::common;
use ta_codegen_lib::backends::c_stream::{self, BlockEntry, KernDecline, KernLookup};
use ta_codegen_lib::registry::Registry;
use ta_codegen_lib::streaming::{CalleeLookback, CalleeLookup, CalleeSig};

fn kernel_of(src: &str, n: &str) -> Option<String> {
    ["void", "TA_RetCode"].iter().find_map(|ret| {
        let head = format!("static {ret} TA_{n}_KernImpl(");
        let at = src.find(&head)?;
        let end = at + src[at..].find("\n}\n")?;
        Some(src[at..end].to_string())
    })
}

fn update_block_of(src: &str, n: &str) -> Option<String> {
    let head = format!("TA_RetCode TA_{n}_UpdateBlock(");
    let at = src.find(&head)?;
    let end = at + src[at..].find("\n}\n")?;
    Some(src[at..end].to_string())
}

fn block_batch_of(src: &str, n: &str) -> Option<String> {
    let head = format!("TA_RetCode TA_{n}_BlockBatch(");
    let at = src.find(&head)?;
    let end = at + src[at..].find("\n}\n")?;
    Some(src[at..end].to_string())
}

fn assert_lines(what: &str, text: &str, lines: &[&str]) {
    for line in lines {
        assert!(text.contains(line), "{what} lacks `{line}`:\n{text}");
    }
}

#[test]
fn a_plain_step_gets_a_hoisted_kernel() {
    let registry = common::make_registry();
    let helpers = common::make_helpers();
    let mut kerneled = 0;
    for name in common::discover_indicators() {
        let (func, enums) = common::load_indicator(&name);
        if !func.streaming {
            continue;
        }
        let decision = c_stream::kernel_decision(&func, registry);
        let n = name.to_uppercase();
        let src = c_stream::generate(&func, &enums, registry, helpers);
        let kernel = kernel_of(&src, &n);
        assert_eq!(decision.is_ok(), update_block_of(&src, &n).is_some(), "{n}: the decision and the emission disagree");
        assert!(kernel.is_none() || decision.is_ok(), "{n}: a kernel without a block entry");
        kerneled += usize::from(kernel.is_some());
        match name.as_str() {
            "sma" => {
                let k = kernel.expect("SMA's loop step has a kernel");
                assert_lines("SMA kernel", &k, &[
                    "double periodTotal = sp->periodTotal;",
                    "double *ring_trailingIdx_inReal = sp->ring_trailingIdx_inReal;",
                    "double inReal = inRealBlk[kernBar];",
                    "outReal[kernBar] = tempReal / (double)optInTimePeriod;",
                    "sp->periodTotal = periodTotal;",
                    "sp->outRangeCount += nbBar;",
                ]);
                assert!(!k.contains("sp->optInTimePeriod = "), "a field the step only reads is stored back:\n{k}");
            }
            "ma" => {
                assert!(kernel.is_none(), "a dispatcher has no kernel of its own");
                assert_lines("MA UpdateBlock", &update_block_of(&src, "MA").expect("MA has a block entry"), &[
                    "retCode = TA_SMA_UpdateBlock( (struct TA_SMA_Stream *)stream->sub, inReal, nbBar, outReal );",
                    "retCode = TA_MA_UpdateBlock_MAMA( (struct TA_MAMA_Stream *)stream->sub, inReal, nbBar, outReal );",
                    "stream->outRangeCount += nbBar;",
                ]);
            }
            "mavp" => assert_eq!(decision, Err(KernDecline::HandRolled)),
            _ => {}
        }
    }
    assert!(kerneled > 0, "no function got a kernel");
}

/// The registry, except that `ma` claims a composed block entry.
struct MaComposed(&'static Registry);

impl CalleeLookup for MaComposed {
    fn callee(&self, name: &str) -> Option<CalleeSig> {
        self.0.callee(name)
    }
    fn lookback(&self, name: &str) -> Option<CalleeLookback> {
        self.0.lookback(name)
    }
}

impl KernLookup for MaComposed {
    fn block_entry(&self, callee: &str) -> BlockEntry {
        if callee == "ma" { BlockEntry::Composed } else { self.0.block_entry(callee) }
    }
    fn recall_eligible(&self, callee: &str) -> bool {
        self.0.recall_eligible(callee)
    }
}

#[test]
fn a_composed_step_runs_each_pipeline_step_over_a_sub_block() {
    let registry = common::make_registry();
    let helpers = common::make_helpers();
    let (func, enums) = common::load_indicator("sqzmom");
    assert_eq!(c_stream::block_entry_of(&func, registry), BlockEntry::Composed);
    let k = kernel_of(&c_stream::generate(&func, &enums, registry, helpers), "SQZMOM").expect("SQZMOM has a kernel");
    assert_lines("SQZMOM kernel", &k, &[
        "double optInNbDev = sp->optInNbDev;",
        "double pad_tempSD[TA_KERN_BLOCK];",
        "kernLen = nbBar - kernBase < TA_KERN_BLOCK ? nbBar - kernBase : TA_KERN_BLOCK;",
        "subRc = TA_STDDEV_UpdateBlock( sp->sub5, inCloseBlk + kernBase, kernLen, pad_tempSD );",
        "subRc = TA_LINEARREG_UpdateBlock( sp->sub6, pad_tempDev, kernLen, outMomentum + kernBase );",
        "cur_tempSD = pad_tempSD[kernBar];",
        "dev = cur_tempSD * optInNbDev;",
        "outSqueeze[kernBase + kernBar] = cur_outSqueeze;",
        "sp->cur_outSqueeze = outSqueeze[nbBar - 1];",
        "sp->outRangeCount += nbBar;",
    ]);

    let (stoch, enums) = common::load_indicator("stoch");
    let k = kernel_of(&c_stream::generate(&stoch, &enums, registry, helpers), "STOCH").expect("STOCH has a kernel");
    assert_lines("STOCH kernel", &k, &[
        "pad_tempBuffer[kernBar] = cur_tempBuffer;",
        "subRc = TA_MA_UpdateBlock( sp->sub0, pad_tempBuffer, kernLen, pad_tempBuffer );",
        "outSlowK[kernBase + kernBar] = pad_tempBuffer[kernBar];",
        "sp->today = today;",
    ]);
    assert_eq!(
        c_stream::kernel_decision(&stoch, &MaComposed(registry)),
        Err(KernDecline::Composed("an in-place sub-call into a composed kernel".to_string()))
    );
}

#[test]
fn a_blocked_batch_re_calls_a_restart_exact_sub_only_on_the_callers_arrays() {
    let registry = common::make_registry();
    let helpers = common::make_helpers();
    let (func, enums) = common::load_indicator("ichimoku");
    let src = c_stream::generate(&func, &enums, registry, helpers);
    let k = kernel_of(&src, "ICHIMOKU").expect("ICHIMOKU has a kernel");
    assert_lines("ICHIMOKU kernel", &k, &[
        "recallLb0 = TA_MIDPRICE_Lookback( sp->optInTenkanPeriod );",
        "if( histBar <= 0 || histBar < recallLb0 )",
        "subRc = TA_MIDPRICE( recallLb0, recallLb0 + kernLen - 1, inHighBlk + kernBase - recallLb0, inLowBlk + kernBase - recallLb0, sp->optInTenkanPeriod, &recallBeg, &recallNb, pad_tempT );",
        "subRc = TA_MEDPRICE_UpdateBlock( sp->sub3, pad_tempT, pad_tempK, kernLen, outSenkouSpanA + kernBase );",
    ]);
    assert!(!k.contains("TA_MEDPRICE("), "a sub fed by a pad is re-called:\n{k}");
    assert_eq!(c_stream::recall_subs(&func, registry), ["MIDPRICE"; 3]);
    assert_lines("ICHIMOKU entries", &src, &[
        "return TA_ICHIMOKU_KernImpl( stream, inHigh, inLow, 0, nbBar,",
        "retCode = TA_ICHIMOKU_KernImpl( stream, inHigh + firstIdx + 1, inLow + firstIdx + 1, firstIdx + 1, endIdx - firstIdx,",
    ]);
}

#[test]
fn every_kerneled_function_gets_a_block_batch_anchored_at_the_callers_start() {
    let registry = common::make_registry();
    let helpers = common::make_helpers();
    let mut seen = 0;
    for name in common::discover_indicators() {
        let (func, enums) = common::load_indicator(&name);
        if !func.streaming {
            continue;
        }
        let n = name.to_uppercase();
        let bb = block_batch_of(&c_stream::generate(&func, &enums, registry, helpers), &n);
        assert_eq!(c_stream::kernel_decision(&func, registry).is_ok(), bb.is_some(), "{n}: kernel and BlockBatch disagree");
        seen += usize::from(bb.is_some());
        if name == "sma" {
            assert_lines("SMA BlockBatch", &bb.expect("SMA has a BlockBatch"), &[
                "lookbackTotal = TA_SMA_Lookback( optInTimePeriod );",
                "firstIdx = startIdx < lookbackTotal ? lookbackTotal : startIdx;",
                "retCode = TA_SMA_OpenInternal( &stream, inReal, startIdx, firstIdx + 1, optInTimePeriod, outReal );",
                "TA_SMA_KernImpl( stream, inReal + firstIdx + 1, endIdx - firstIdx, outReal + 1 );",
                "*outBegIdx = firstIdx;",
            ]);
        }
    }
    assert!(seen > 0, "no function got a BlockBatch");
}
