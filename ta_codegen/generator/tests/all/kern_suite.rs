//! The C block kernel (`TA_<N>_KernImpl`): the step over `nbBar` bars with the
//! handle's fields in locals. Its values are compared bitwise only by the
//! prototype harness `generate` writes under `TA_CODEGEN_KERN_HARNESS`.

use crate::common;
use ta_codegen_lib::backends::c_stream::{self, KernDecline};

fn kernel_of(src: &str, n: &str) -> Option<String> {
    let head = format!("static void TA_{n}_KernImpl(");
    let at = src.find(&head)?;
    let end = at + src[at..].find("\n}\n")?;
    Some(src[at..end].to_string())
}

#[test]
fn a_plain_step_gets_a_hoisted_kernel_and_a_composed_one_does_not() {
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
        let kernel = kernel_of(&c_stream::generate(&func, &enums, registry, helpers), &n);
        assert_eq!(decision.is_ok(), kernel.is_some(), "{n}: the decision and the emission disagree");
        kerneled += usize::from(kernel.is_some());
        match name.as_str() {
            "sma" => {
                let k = kernel.expect("SMA's loop step has a kernel");
                for line in [
                    "double periodTotal = sp->periodTotal;",
                    "double *ring_trailingIdx_inReal = sp->ring_trailingIdx_inReal;",
                    "double inReal = inRealBlk[kernBar];",
                    "outReal[kernBar] = tempReal / (double)optInTimePeriod;",
                    "sp->periodTotal = periodTotal;",
                    "sp->outRangeCount += nbBar;",
                ] {
                    assert!(k.contains(line), "SMA kernel lacks `{line}`:\n{k}");
                }
                assert!(!k.contains("sp->optInTimePeriod = "), "a field the step only reads is stored back:\n{k}");
            }
            "stoch" => assert_eq!(decision, Err(KernDecline::Composed)),
            "ma" | "mavp" => assert_eq!(decision, Err(KernDecline::HandRolled)),
            _ => {}
        }
    }
    assert!(kerneled > 0, "no function got a kernel");
}
