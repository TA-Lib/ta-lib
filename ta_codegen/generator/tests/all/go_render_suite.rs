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
    assert_eq!(go.matches("inClose[k").count(), 2, "the helper no longer reads `close` twice:\n{go}");
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

/// A re-sliced loop: its lines, the re-slices cut for it (name, array,
/// start), the pass counter, and whether it counts down.
type ReSliced = (Vec<String>, Vec<(String, String, String)>, String, bool);

/// Each re-sliced loop of a batch body.
fn re_sliced_loops(body: &str) -> Vec<ReSliced> {
    let head = regex::Regex::new(r"^(\s*)for (_k\d+) := (range _r\d+|_n\d+ - 1; _k\d+ >= 0; _k\d+--) \{$").unwrap();
    let cut = regex::Regex::new(r"^\s*(_r\d+) := (\w+)\[(.+):\]\[:_n\d+\]$").unwrap();
    let lines: Vec<&str> = body.lines().collect();
    let mut out = Vec::new();
    for (h, line) in lines.iter().enumerate() {
        let Some(c) = head.captures(line) else { continue };
        let end = h + lines[h..].iter().position(|l| *l == format!("{}}}", &c[1])).expect("the loop closes");
        let cuts = lines[..h].iter().rev().map_while(|l| cut.captures(l)).map(|r| (r[1].to_string(), r[2].to_string(), r[3].to_string())).collect();
        out.push((lines[h + 1..end].iter().map(|l| (*l).to_string()).collect(), cuts, c[2].to_string(), !c[3].starts_with("range")));
    }
    out
}

fn batch_body(name: &str) -> String {
    let (func, enums) = common::load_indicator(name);
    let src = backends::go::generate(&func, &enums, common::make_registry(), common::make_helpers());
    let camel = backends::common::camel_words(&func.name.to_uppercase());
    go_body(&src, &format!("func (c *Core) {camel}Impl(startIdx, endIdx int32")).to_string()
}

/// A lockstep counted loop ranges one `int` over re-slices of its arrays,
/// with no index step inside: ADD's comma update, MOM's three steps in one
/// statement, OBV's volume read in a branch and AD's count-down test all
/// reach that shape. Over the corpus, no name a re-slice starts from is
/// written inside its loop, which would move the slice under it.
#[test]
fn go_lockstep_loops_range_over_re_slices() {
    let step = regex::Regex::new(r"^\s*\w+(\+\+|--| \+= 1| -= 1|, \w+ = )").unwrap();
    let ident = regex::Regex::new(r"[A-Za-z_]\w*").unwrap();
    if !common::sweep_filtered() {
        for (name, arrays, cuts) in [("add", &["inReal0", "inReal1", "outReal"][..], 3), ("mom", &["inReal", "outReal"], 3), ("obv", &["inReal", "inVolume", "outReal"], 3), ("ad", &["inHigh", "inLow", "inClose", "inVolume", "outReal"], 5)] {
            let body = batch_body(name);
            let loops = re_sliced_loops(&body);
            let (pass, cut, k, _) = loops.iter().find(|(_, c, _, _)| c.len() == cuts).unwrap_or_else(|| panic!("{name}: no loop over {cuts} re-slices:\n{body}"));
            for a in arrays {
                assert!(cut.iter().any(|(_, x, _)| x == a), "{name}: `{a}` is not re-sliced:\n{body}");
                assert!(!pass.iter().any(|l| l.contains(&format!("{a}["))), "{name}: `{a}` is indexed in the loop:\n{body}");
            }
            assert!(!pass.iter().any(|l| step.is_match(l)), "{name}: an index steps inside the loop:\n{body}");
            assert!(pass.iter().any(|l| l.contains(&format!("[{k}]"))), "{name}: no element at the counter");
        }
    }
    let mut functions = 0usize;
    for name in common::discover_indicators() {
        let body = batch_body(&name);
        let loops = re_sliced_loops(&body);
        for (pass, cuts, _, _) in &loops {
            for (_, _, start) in cuts {
                for base in ident.find_iter(start).map(|m| m.as_str()) {
                    let written = regex::Regex::new(&format!(r"^\s*{}(\+\+|--| [-+*/]?= )", regex::escape(base))).unwrap();
                    assert!(!pass.iter().any(|l| written.is_match(l)), "{name}: `{base}` starts a re-slice and is written in its loop:\n{body}");
                }
            }
        }
        functions += usize::from(!loops.is_empty());
    }
    assert!(common::sweep_filtered() || functions >= 160, "only {functions} batch bodies re-slice a loop");
}

/// `for( i = 0; i < 50; i++ ) if( i < DCPeriodInt ) S` runs `min(50,
/// DCPeriodInt)` passes in every tier, batch and stream alike; over the
/// corpus no loop of a literal cap whose whole body is one guard on its
/// variable is left in the emitted Go.
#[test]
fn go_padded_loops_stop_at_their_guard() {
    let head = regex::Regex::new(r"^(\s*)for (\w+) = -?\d+; (\w+) < (\d+); \w+ (\+= 1|\+\+) \{$").unwrap();
    let (registry, helpers) = (common::make_registry(), common::make_helpers());
    let mut capped = 0usize;
    for name in common::discover_indicators() {
        let (func, enums) = common::load_indicator(&name);
        let src = backends::go::generate(&func, &enums, registry, helpers);
        let lines: Vec<&str> = src.lines().collect();
        for (h, line) in lines.iter().enumerate() {
            let Some(c) = head.captures(line) else { continue };
            let end = h + lines[h..].iter().position(|l| *l == format!("{}}}", &c[1])).expect("the loop closes");
            let guard = format!("{}\tif {} < ", &c[1], &c[2]);
            let padded = lines[h + 1].starts_with(&guard) && lines[end - 1] == format!("{}\t}}", &c[1]) && lines[h + 1..end - 1].iter().skip(1).all(|l| l.starts_with(&format!("{}\t\t", &c[1])));
            assert!(!padded, "{name}: a padded loop:\n{}", lines[h..=end].join("\n"));
        }
        capped += src.matches(" < min(").count();
        if name == "ht_trendline" || name == "ht_trendmode" {
            let v = if name == "ht_trendline" { "i" } else { "j" };
            assert!(!src.contains(&format!("if {v} < DCPeriodInt {{")), "{name}: a guarded pass is left");
            assert_eq!(src.matches(&format!("{v} < min(50, DCPeriodInt)")).count(), 4, "{name}: batch, open, Update and Peek");
            let camel = backends::common::camel_words(&func.name.to_uppercase());
            let batch = go_body(&src, &format!("func (c *Core) {camel}Impl(startIdx, endIdx int32"));
            assert!(batch.contains(&format!("{v} < min(50, DCPeriodInt)")), "{name}: the batch body");
        }
    }
    assert!(common::sweep_filtered() || capped >= 8, "only {capped} capped loops");
}

/// A function, the arrays each of its scan passes re-slices, and the arrays
/// no pass indexes.
type BlockScan = (&'static str, &'static [&'static [&'static str]], &'static [&'static str]);

/// The block scans of the window extrema: the backward suffix pass counts
/// down over re-slices of its inputs and suffix scratch, the forward prefix
/// pass and the merge range over theirs, and none of the three indexes an
/// input, an output or a scratch array (WILLR's close, read on one arm only,
/// keeps its check). Over the corpus no ring is re-sliced.
#[test]
fn go_block_scans_range_over_re_slices() {
    if !common::sweep_filtered() {
        let scans: [BlockScan; 2] = [
            ("max", &[&["inReal", "sufHighest"], &["inReal", "preHighest"], &["sufHighest", "preHighest", "outReal"]], &["inReal", "outReal", "sufHighest", "preHighest"]),
            (
                "willr",
                &[&["inHigh", "inLow", "sufHighest", "sufLowest"], &["inHigh", "inLow", "preHighest", "preLowest"], &["sufHighest", "preHighest", "sufLowest", "preLowest", "outReal"]],
                &["inHigh", "inLow", "outReal", "sufHighest", "sufLowest", "preHighest", "preLowest"],
            ),
        ];
        for (name, passes, unindexed) in scans {
            let body = batch_body(name);
            let loops = re_sliced_loops(&body);
            for (n, arrays) in passes.iter().enumerate() {
                let (pass, _, k, down) = loops
                    .iter()
                    .find(|(_, cuts, _, _)| cuts.len() == arrays.len() && arrays.iter().all(|a| cuts.iter().any(|(_, x, _)| x == a)))
                    .unwrap_or_else(|| panic!("{name}: no loop over re-slices of {arrays:?}:\n{body}"));
                assert_eq!(*down, n == 0, "{name}: the suffix pass, and only it, counts down:\n{body}");
                assert!(pass.iter().any(|l| l.contains(&format!("[{k}]"))), "{name}: no element at the counter");
                for a in unindexed {
                    assert!(!pass.iter().any(|l| l.contains(&format!("{a}["))), "{name}: `{a}` is indexed in a scan:\n{body}");
                }
                assert!(!pass.iter().any(|l| l.trim_start().starts_with("outIdx")), "{name}: the output index steps in the loop:\n{body}");
            }
        }
    }
    let mut down = 0usize;
    for name in common::discover_indicators() {
        let body = batch_body(&name);
        for (_, cuts, _, d) in re_sliced_loops(&body) {
            for (_, array, _) in &cuts {
                assert!(!body.contains(&format!("{array}_Idx")), "{name}: the ring `{array}` is re-sliced:\n{body}");
            }
            down += usize::from(d);
        }
    }
    assert!(common::sweep_filtered() || down >= 12, "only {down} loops count down over re-slices");
}

/// The plain loops of `body` with an `if` or `else` arm between them and an
/// enclosing loop, that index an array at their own variable.
fn loops_on_an_arm(body: &str) -> usize {
    let head = regex::Regex::new(r"^for (\w+) = [^;]+; [^;]+; \w+ [-+]= 1 \{$").unwrap();
    let lines: Vec<&str> = body.lines().collect();
    let (mut found, mut stack) = (0usize, Vec::new());
    for (n, raw) in lines.iter().enumerate() {
        let line = raw.trim();
        if line.starts_with('}') {
            stack.pop();
        }
        if !line.ends_with('{') {
            continue;
        }
        if let Some(c) = head.captures(line) {
            let armed = stack.iter().position(|k| *k == 'f').is_some_and(|f| stack[f..].contains(&'a'));
            let indent = &raw[..raw.len() - raw.trim_start().len()];
            let end = n + lines[n..].iter().position(|l| *l == format!("{indent}}}")).expect("the loop closes");
            let at = regex::Regex::new(&format!(r"\w\[{}(\]| [-+] )", regex::escape(&c[1]))).unwrap();
            found += usize::from(armed && lines[n + 1..end].iter().any(|l| at.is_match(l)));
        }
        let kind = match line {
            l if l.starts_with("for ") => 'f',
            l if (l.starts_with("if ") && !l.starts_with("if _n")) || l.starts_with("} else") => 'a',
            _ => 'o',
        };
        stack.push(kind);
    }
    found
}

/// A loop on an arm of a loop over bars runs on some bars only, while what
/// its re-slices hold would stay live across every bar: VAR's three reseed
/// loops keep their checks and its prologue loop ranges over a re-slice.
/// MEDIAN's two shifts sit on the two arms of one `if`/`else`, so one runs on
/// every bar and both keep their re-slices; its warm-up shift, whose sibling
/// arm's loop is not one the rule takes, does not. ALMA's window loop, run
/// on every pass of its outer loop, keeps its re-slices, as the block scans'
/// passes do (above).
#[test]
fn go_a_loop_on_an_arm_of_a_bar_loop_is_not_re_sliced() {
    if !common::sweep_filtered() {
        let var = batch_body("var");
        let loops = re_sliced_loops(&var);
        assert_eq!(loops.len(), 1, "VAR re-slices a loop besides its prologue:\n{var}");
        assert!(loops[0].1.iter().any(|(_, a, _)| a == "inReal"), "VAR's prologue loop is not re-sliced:\n{var}");
        assert_eq!(var.matches("for j = windowStart; j <= i; j += 1 {").count(), 3, "VAR's reseed loops:\n{var}");
        assert_eq!(loops_on_an_arm(&var), 3, "VAR's reseed loops:\n{var}");

        let median = batch_body("median");
        let shifts = re_sliced_loops(&median);
        assert!(shifts.len() == 2 && shifts.iter().all(|(_, cuts, _, _)| cuts.iter().all(|(_, a, _)| a == "sorted")), "MEDIAN's bar-loop shifts are not re-sliced:\n{median}");
        let both = median.find("\n\t\tif j < pos {\n").expect("MEDIAN's shifts");
        assert!(median[..both].contains("\n\t\t\tfor j > lo {\n"), "MEDIAN's warm-up shift is re-sliced:\n{median}");

        let alma = batch_body("alma");
        let window = re_sliced_loops(&alma).into_iter().find(|(_, cuts, _, _)| cuts.iter().any(|(_, a, _)| a == "weights") && cuts.iter().any(|(_, a, _)| a == "inReal"));
        let (_, cuts, _, _) = window.unwrap_or_else(|| panic!("ALMA's window loop is not re-sliced:\n{alma}"));
        let at = alma.find(&format!("{} := weights[", cuts.iter().find(|(_, a, _)| a == "weights").expect("weights").0)).expect("the cut");
        let outer = alma.find("\n\tfor i+3 <= endIdx {\n").expect("ALMA's outer loop");
        assert!(outer < at && !alma[outer..at].contains("\n\t}\n"), "ALMA's window loop is not nested in its outer loop:\n{alma}");
    }
    let sites: usize = common::discover_indicators().iter().map(|n| loops_on_an_arm(&batch_body(n))).sum();
    assert!(common::sweep_filtered() || sites >= 55, "only {sites} loops on an arm of a loop keep their checks");
}

/// The factors of each `math.FMA(` call in `src`.
fn fma_factors(src: &str) -> Vec<(String, String)> {
    let mut out = Vec::new();
    for (at, _) in src.match_indices("math.FMA(") {
        let mut args = vec![String::new()];
        let mut depth = 0i32;
        for ch in src[at + "math.FMA(".len()..].chars() {
            match ch {
                '(' => depth += 1,
                ')' if depth == 0 => break,
                ')' => depth -= 1,
                ',' if depth == 0 => {
                    args.push(String::new());
                    continue;
                }
                _ => {}
            }
            args.last_mut().expect("an argument").push(ch);
        }
        assert_eq!(args.len(), 3, "`math.FMA` takes three arguments: {args:?}");
        out.push((args[0].trim().to_string(), args[1].trim().to_string()));
    }
    out
}

/// A product by `2^k`, `k >= 1`, is exact, so Go renders that site as the
/// plain multiply-add C's fused one rounds to; every other site keeps C's
/// fusing. HT_TRENDLINE's `4.0*a + 3.0*b + 2.0*c + d` loses its two exact
/// sites in each of its four frames and fuses no other product in their
/// place, which no value gate could see; C and Rust keep fusing WCLPRICE.
#[test]
fn go_renders_an_exact_product_unfused() {
    let (registry, helpers) = (common::make_registry(), common::make_helpers());
    let exact = |a: &str| a.trim_start_matches('-').parse::<f64>().is_ok_and(|v| v >= 2.0 && v.to_bits() & ((1u64 << 52) - 1) == 0);
    let mut sites = 0usize;
    for name in common::discover_indicators() {
        let (func, enums) = common::load_indicator(&name);
        let src = backends::go::generate(&func, &enums, registry, helpers);
        for (a, b) in fma_factors(&src) {
            assert!(!exact(&a) && !exact(&b), "{name}: `math.FMA({a}, {b}, ..)` fuses an exact product");
            sites += 1;
        }
        match name.as_str() {
            "wclprice" => assert!(!src.contains("math.FMA"), "wclprice: a fused site is left"),
            "ht_trendline" => {
                assert_eq!(src.matches("math.FMA(").count(), 48 - 8, "ht_trendline: its fused sites but the 4.0 and 2.0 ones");
                assert!(!src.contains("math.FMA(3.0,"), "ht_trendline: the 3.0 product took the exact one's place");
            }
            _ => {}
        }
    }
    assert!(common::sweep_filtered() || sites >= 1000, "only {sites} fused sites");

    let (func, enums) = common::load_indicator("wclprice");
    assert!(backends::c::generate(&func, &enums, registry, helpers).contains("fma(inClose[i], 2.0,"), "C stops fusing WCLPRICE");
    assert!(backends::rust_lang::generate(&func, &enums, registry, helpers).contains("fmadd!(inClose, 2.0,"), "Rust stops fusing WCLPRICE");
}
