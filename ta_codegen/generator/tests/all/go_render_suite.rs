//! The Go emitter's refusals and lowerings on bodies no shipped indicator has.

use crate::common;
use ta_codegen_lib::backends;

/// CDLHIGHWAVE's signature over a loop whose store reads `ta_uppershadow`, a
/// helper that reads its `close` and `open` parameters twice.
fn upper_shadow_body(close_arg: &str, step: &str) -> String {
    format!(
        r"
int cdlhighwave_lookback(void)
{{
   return 0;
}}

TA_RetCode cdlhighwave(int startIdx, int endIdx,
   const double inOpen[],
   const double inHigh[],
   const double inLow[],
   const double inClose[],
   int *outBegIdx, int *outNBElement,
   int outInteger[])
{{
   int i, k, outIdx;
   k = startIdx;
   outIdx = 0;
   for( i = startIdx; i <= endIdx; i++ ) {{
      outInteger[outIdx++] = ta_uppershadow(inHigh[i], {close_arg}, inOpen[i]) > inLow[i] ? 1 : 0;
      {step}
   }}
   *outBegIdx = startIdx;
   *outNBElement = outIdx;
   return TA_SUCCESS;
}}
"
    )
}

fn go_of(source: &str) -> std::thread::Result<String> {
    let (func, enums) = common::load_indicator_with_source("cdlhighwave", source);
    std::panic::catch_unwind(|| backends::go::generate(&func, &enums, common::make_registry(), common::make_helpers()))
}

/// Substituting the helper would step `k` once per read of `close`.
#[test]
fn a_step_in_a_substituted_helper_argument_is_refused() {
    let refused = go_of(&upper_shadow_body("inClose[k++]", "")).expect_err("the step was duplicated, not refused");
    let msg = refused.downcast_ref::<String>().cloned().unwrap_or_default();
    assert!(msg.contains("function CDLHIGHWAVE") && msg.contains("`k`") && msg.contains("ta_uppershadow"), "{msg}");

    let go = go_of(&upper_shadow_body("inClose[k]", "k++;")).expect("the same body with the step on its own line");
    assert_eq!(go.matches("inClose[k]").count(), 2, "the helper no longer reads `close` twice:\n{go}");
}

/// Each candlestick's generated Go file.
fn candlestick_sources() -> Vec<(String, String)> {
    let registry = common::make_registry();
    let helpers = common::make_helpers();
    common::discover_indicators()
        .into_iter()
        .filter(|n| n.starts_with("cdl"))
        .map(|n| {
            let (func, enums) = common::load_indicator(&n);
            (n.clone(), backends::go::generate(&func, &enums, registry, helpers))
        })
        .collect()
}

fn go_body<'a>(src: &'a str, decl: &str) -> &'a str {
    let at = src.find(decl).unwrap_or_else(|| panic!("no `{decl}`"));
    &src[at..at + src[at..].find("\n}\n").expect("a closing brace")]
}

/// The plain open decides only the last bar: its seam opens through the sink
/// frame, whose stride is built in.
#[test]
fn a_go_candlestick_open_internal_calls_its_sink_frame() {
    let (mut framed, mut single) = (0usize, 0usize);
    for (name, src) in candlestick_sources() {
        let frame = format!("func (c *Core) {name}OpenSinkFrame(");
        let internal = go_body(&src, &format!("func (c *Core) {name}OpenInternal("));
        if !src.contains(&frame) {
            assert!(internal.contains(&format!("c.{name}OpenImpl(")), "{name}: no opener");
            single += 1;
            continue;
        }
        framed += 1;
        assert!(
            internal.contains(&format!("c.{name}OpenSinkFrame(")) && !internal.contains("OpenImpl("),
            "{name}OpenInternal opens through the sink frame:\n{internal}"
        );
        assert!(!go_body(&src, &frame).contains("outStride"), "{name}: the frame has its stride built in");
    }
    if !common::sweep_filtered() {
        assert_eq!(single, 2, "only CDLHIKKAKE and CDLHIKKAKEMOD keep one body");
        assert!(framed >= 50, "only {framed} candlestick openers split");
    }
}

/// A batch body's real picks are selects only in its outermost loops, the bars;
/// a true range's raises stay branches there and in Update, not in Peek.
#[test]
fn go_batch_bodies_select_only_in_bar_loops() {
    let colour = regex::Regex::new(r"selF\([^,]*[Cc]lose[^,]* >= [^,]*[Oo]pen").unwrap();
    let impl_decl = regex::Regex::new(r"(?m)^func \(c \*Core\) (\w+)Impl\(").unwrap();
    let (registry, helpers) = (common::make_registry(), common::make_helpers());
    let mut bar_loop = 0usize;
    let mut named = Vec::new();
    for name in common::discover_indicators() {
        let (func, enums) = common::load_indicator(&name);
        let src = backends::go::generate(&func, &enums, registry, helpers);
        for cap in impl_decl.captures_iter(&src).filter(|c| !c[1].ends_with("Open") && !c[1].ends_with("Step")) {
            let (mut fors, mut stack) = (0usize, Vec::new());
            for line in go_body(&src, &cap[0]).lines().skip(1).map(str::trim) {
                if line.starts_with('}') && stack.pop() == Some(true) {
                    fors -= 1;
                }
                let picks = line.matches("selF(").count() - colour.find_iter(line).count();
                assert!(picks == 0 || fors == 1, "{name}: {picks} select(s) at loop depth {fors}: {line}");
                bar_loop += picks;
                if line.ends_with('{') {
                    stack.push(line.starts_with("for "));
                    fors += usize::from(line.starts_with("for "));
                }
            }
        }
        if name == "rsi" || name == "atr" {
            named.push((name, src));
        }
    }
    if !common::sweep_filtered() {
        named.sort();
        let [(_, atr), (_, rsi)] = &named[..] else { panic!("no ATR or RSI") };
        assert!(go_body(rsi, "func (c *Core) rsiImpl(").contains("gainDelta = selF(tempValue2 > 0.0"), "RSI's gain pick is a branch");
        let count = |decl: &str| go_body(atr, decl).matches("selF(").count();
        assert_eq!(count("func (c *Core) atrImpl("), 0, "ATR's batch true range selects");
        assert_eq!(count("func (sp *ATRStream) Update("), 0, "ATR's Update true range selects");
        assert_eq!(count("func (sp *ATRStream) Peek("), 2, "ATR's Peek true range branches");
    }
    assert!(common::sweep_filtered() || bar_loop >= 110, "only {bar_loop} selects in bar loops");
}

/// A candle's colour is a coin flip on real bars: the batch and open bodies
/// pick each colour bound through `selF`, and the stream step keeps the
/// branch. A range type is folded where C folds it.
#[test]
fn go_candle_bodies_fold_the_range_type_and_take_no_colour_branch() {
    let branchy = regex::Regex::new(r"var _T\d+ float64\n\s*if inClose(\[[^\]]+\])? >= inOpen").unwrap();
    let arm = regex::Regex::new(r"(?m)^\tif \w+_rangeType == \d").unwrap();
    let (mut bodies, mut selects, mut folded, mut frames) = (0usize, 0usize, 0usize, 0usize);
    for (name, src) in candlestick_sources() {
        let upper = name.to_uppercase();
        let mut tiers = vec![format!("func (c *Core) {name}Impl("), format!("func (c *Core) {name}OpenImpl(")];
        if src.contains(&format!("func (c *Core) {name}OpenSinkFrame(")) {
            tiers.push(format!("func (c *Core) {name}OpenSinkFrame("));
        }
        let stream = [format!("func (sp *{upper}Stream) Update("), format!("func (sp *{upper}Stream) Peek(")];
        for tier in tiers.iter().chain(&stream) {
            let body = go_body(&src, tier);
            assert!(!branchy.is_match(body), "{tier} branches on the colour:\n{body}");
            selects += body.matches("selF(").count();
            bodies += 1;
        }
        frames += stream.iter().filter(|t| go_body(&src, t).contains("selF(")).count();
        let c = std::fs::read_to_string(format!("{}/../../src/ta_func/ta_{}.c", env!("CARGO_MANIFEST_DIR"), name.to_uppercase())).unwrap();
        let c_batch = &c[c.find("TA_LIB_API TA_RetCode TA_").expect("C batch")..];
        let c_folds = c_batch[..c_batch.find("\n}\n").expect("C batch closes")].contains("_rangeType == TA_RangeType_");
        let go_folds = arm.is_match(go_body(&src, &tiers[0]));
        assert_eq!(go_folds, c_folds, "{name}: Go and C disagree on folding the range type");
        folded += usize::from(go_folds);
    }
    assert!(
        common::sweep_filtered() || (bodies >= 270 && selects >= 150 && folded >= 40 && frames >= 40),
        "{bodies} bodies, {selects} selects, {folded} folded, {frames} stream frames selecting"
    );
}
