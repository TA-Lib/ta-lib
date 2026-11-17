//! The Go emitter's refusals and lowerings on bodies no shipped indicator has.

use crate::common;
use std::collections::BTreeSet;
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

/// OBV's volume is added or subtracted on whether the close rose, a coin flip:
/// batch, open, Update and Peek select the addends, the subtrahend still
/// subtracted (an added negation flips a NaN's sign). AD's and PVT's divisor
/// guards keep their branch. Over the corpus no accumulation is left with a
/// constant `-0.0`, which Go reads as +0.0.
#[test]
fn go_accumulates_a_coin_flip_by_selected_addends() {
    let accrual = regex::Regex::new(r"(?m)^\s*([\w.]+) = ([\w.]+) [-+] selF\(").unwrap();
    let neg_zero = regex::Regex::new(r"(^|[^\w.])-\s*0(\.0*)?([eE][-+]?\d+)?([^\w.]|$)").unwrap();
    let (registry, helpers) = (common::make_registry(), common::make_helpers());
    let mut sites = 0usize;
    for name in common::discover_indicators() {
        let (func, enums) = common::load_indicator(&name);
        let src = backends::go::generate(&func, &enums, registry, helpers);
        for line in src.lines() {
            assert!(!neg_zero.is_match(line), "{name}: a constant negative zero is +0.0 in Go: {line}");
        }
        sites += accrual.captures_iter(&src).filter(|c| c[1] == c[2]).count();
        let tiers = match name.as_str() {
            "obv" => [
                ("func (c *Core) obvImpl(", "prevOBV = prevOBV + selF(tempReal > prevReal, _r3[_k1], math.Float64frombits(1<<63)) - selF(tempReal < prevReal, _r3[_k1], 0.0)"),
                ("func (c *Core) obvOpenImpl(", "prevOBV = prevOBV + selF(tempReal > prevReal, inVolume[i], math.Float64frombits(1<<63)) - selF(tempReal < prevReal, inVolume[i], 0.0)"),
                ("func (sp *OBVStream) Update(", "sp.prevOBV = sp.prevOBV + selF(tempReal > sp.prevReal, inVolume, math.Float64frombits(1<<63)) - selF(tempReal < sp.prevReal, inVolume, 0.0)"),
                ("func (sp *OBVStream) Peek(", "prevOBV = prevOBV + selF(tempReal > sp.prevReal, inVolume, math.Float64frombits(1<<63)) - selF(tempReal < sp.prevReal, inVolume, 0.0)"),
            ],
            "ad" | "pvt" => {
                let branch = if name == "ad" { "if tmp > 0.0 {" } else { "if prevClose != 0.0 {" };
                assert!(go_body(&src, &format!("func (c *Core) {name}Impl(")).contains(branch), "{name}: its divisor guard selects");
                continue;
            }
            _ => continue,
        };
        for (decl, line) in tiers {
            let body = go_body(&src, decl);
            assert!(body.contains(line) && !body.contains("if tempReal "), "{decl}: OBV's accumulation branches:\n{body}");
        }
    }
    assert!(common::sweep_filtered() || sites >= 28, "only {sites} accumulations select their addends");
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
/// start), the pass counter, and whether it counts down. Of an unrolled loop,
/// the scalar tail's lines and counter.
type ReSliced = (Vec<String>, Vec<(String, String, String)>, String, bool);

/// Each re-sliced loop of a batch body.
fn re_sliced_loops(body: &str) -> Vec<ReSliced> {
    let head = regex::Regex::new(r"^(\s*)for (_k\d+) := (range _r\d+|_n\d+ - 1; _k\d+ >= 0; _k\d+--) \{$").unwrap();
    let cut = regex::Regex::new(r"^\s*(_r\d+) := (\w+)\[(.+):\]\[:_n\d+(:_n\d+)?\]$").unwrap();
    let lines: Vec<&str> = body.lines().collect();
    let mut out = Vec::new();
    let cuts_before = |h: usize| lines[..h].iter().rev().map_while(|l| cut.captures(l)).map(|r| (r[1].to_string(), r[2].to_string(), r[3].to_string())).collect();
    for (h, line) in lines.iter().enumerate() {
        if let Some(c) = head.captures(line) {
            let end = h + lines[h..].iter().position(|l| *l == format!("{}}}", &c[1])).expect("the loop closes");
            out.push((lines[h + 1..end].iter().map(|l| (*l).to_string()).collect(), cuts_before(h), c[2].to_string(), !c[3].starts_with("range")));
        } else if let Some(u) = unrolled(&lines, h) {
            out.push((u.tail, cuts_before(h - 1), u.tail_counter, false));
        }
    }
    out
}

/// An unrolled loop: its width, each pass of a group, the scalar tail, and
/// each tail cut with the group cut of the same re-slice.
struct Unrolled {
    width: usize,
    passes: Vec<Vec<String>>,
    tail: Vec<String>,
    tail_counter: String,
    cuts: Vec<(String, String)>,
}

/// The unrolled loop whose group loop opens at line `h`: `_k := 0`, `for ;
/// _k+K <= _n; _k += K {`, a group cut per re-slice, K passes, then a tail cut
/// per re-slice and `for _j := range _t {`.
fn unrolled(lines: &[&str], h: usize) -> Option<Unrolled> {
    let head = regex::Regex::new(r"^(\s*)for ; (_k\d+)\+(\d+) <= _n\d+; _k\d+ \+= \d+ \{$").unwrap();
    let group = regex::Regex::new(r"^\s*(_g\d+) := (_r\d+)\[_k\d+ : _k\d+\+\d+ : _k\d+\+\d+\]$").unwrap();
    let tail_cut = regex::Regex::new(r"^\s*(_t\d+) := (_r\d+)\[_k\d+:\]").unwrap();
    let tail_head = regex::Regex::new(r"^\s*for (_j\d+) := range _t\d+ \{$").unwrap();
    let c = head.captures(lines[h])?;
    let indent = &c[1];
    assert_eq!(lines[h - 1].trim(), format!("{} := 0", &c[2]), "an unrolled loop starts its counter at 0");
    let width: usize = c[3].parse().expect("a width");
    let end = h + lines[h..].iter().position(|l| *l == format!("{indent}}}")).expect("the group loop closes");
    let body: Vec<String> = lines[h + 1..end].iter().skip_while(|l| group.is_match(l)).map(|l| (*l).to_string()).collect();
    assert_eq!(body.len() % width, 0, "{width} passes of equal length:\n{}", body.join("\n"));
    let passes = body.chunks(body.len() / width).map(<[String]>::to_vec).collect();
    let t = end + 1 + lines[end + 1..].iter().position(|l| tail_head.is_match(l)).expect("a scalar tail");
    let tail_end = t + lines[t..].iter().position(|l| *l == format!("{indent}}}")).expect("the tail closes");
    let tail_counter = tail_head.captures(lines[t]).expect("the tail head")[1].to_string();
    let groups: Vec<(String, String)> = lines[h + 1..end].iter().map_while(|l| group.captures(l)).map(|g| (g[2].to_string(), g[1].to_string())).collect();
    let cuts = lines[end + 1..t]
        .iter()
        .map(|l| {
            let c = tail_cut.captures(l).unwrap_or_else(|| panic!("a tail cut: {l}"));
            let g = groups.iter().find(|(r, _)| *r == c[2]).unwrap_or_else(|| panic!("no group cut of {}", &c[2]));
            (c[1].to_string(), g.1.clone())
        })
        .collect();
    Some(Unrolled { width, passes, tail: lines[t + 1..tail_end].iter().map(|l| (*l).to_string()).collect(), tail_counter, cuts })
}

/// Each unrolled loop of a batch body.
fn unrolled_loops(body: &str) -> Vec<Unrolled> {
    let lines: Vec<&str> = body.lines().collect();
    (1..lines.len()).filter_map(|h| unrolled(&lines, h)).collect()
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
    assert!(common::sweep_filtered() || functions >= 158, "only {functions} batch bodies re-slice a loop");
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

/// A product by `2^k`, `k >= 1`, is exact, so Go renders that site as the
/// plain multiply-add C's fused one rounds to; every other site keeps C's
/// fusing. HT_TRENDLINE's `4.0*a + 3.0*b + 2.0*c + d` loses its two exact
/// sites in each of its four frames and fuses no other product in their
/// place, which no value gate could see; C keeps fusing WCLPRICE.
#[test]
fn go_renders_an_exact_product_unfused() {
    let (registry, helpers) = (common::make_registry(), common::make_helpers());
    let mut sites = 0usize;
    for name in common::discover_indicators() {
        let (func, enums) = common::load_indicator(&name);
        let src = backends::go::generate(&func, &enums, registry, helpers);
        for (a, b) in common::fma_factors(&src, "math.FMA(") {
            assert!(!common::exact_literal(&a) && !common::exact_literal(&b), "{name}: `math.FMA({a}, {b}, ..)` fuses an exact product");
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
}

/// A loop whose passes depend on no other runs a group of passes over cuts
/// proved once per group, then a scalar tail: eight for ADD's one store, four
/// for TRANGE's branches, every pass of a group being the tail's pass on its
/// own element. SMA and SUM carry a sum, EXP calls out, and MAX's merge runs
/// inside its block loop: none is unrolled.
#[test]
fn go_independent_passes_are_unrolled() {
    if !common::sweep_filtered() {
        for (name, width) in [("add", 8), ("trange", 4)] {
            let body = batch_body(name);
            let loops = unrolled_loops(&body);
            let [u] = loops.as_slice() else { panic!("{name}: not one unrolled loop:\n{body}") };
            assert_eq!(u.width, width, "{name}:\n{body}");
            for (e, pass) in u.passes.iter().enumerate() {
                let expected: Vec<String> = u
                    .tail
                    .iter()
                    .map(|l| u.cuts.iter().fold(l.clone(), |l, (t, g)| l.replace(&format!("{t}[{}]", u.tail_counter), &format!("{g}[{e}]"))))
                    .collect();
                assert_eq!(pass, &expected, "{name}: pass {e} of a group is not the tail's pass on its element:\n{body}");
            }
        }
        for name in ["sma", "sum", "exp", "max"] {
            assert!(unrolled_loops(&batch_body(name)).is_empty(), "{name}: unrolled:\n{}", batch_body(name));
        }
    }
    let loops: usize = common::discover_indicators().iter().map(|n| unrolled_loops(&batch_body(n)).len()).sum();
    assert!(common::sweep_filtered() || loops >= 28, "only {loops} unrolled loops");
}

/// An index-tracking scan cuts each input once to `endIdx + 1`, runs an
/// `int` bar cursor to that length and each rescan on a fresh `int` counter
/// over the cut itself, never a cut of its window: MAXINDEX and AROONOSC,
/// each rescan keeping its tie rule. Over the corpus, every rescan loops
/// through its own label, each path stepping its counter before its back
/// edge, and every output lane is written at one counter, the later lanes
/// cut to the first's length.
#[test]
fn go_index_tracking_scans_use_native_cursors() {
    let bar = regex::Regex::new(r"^(\s*)for (_t\d+) := (_s\d+); _t\d+ < len\(_x\d+\); _t\d+\+\+ \{$").unwrap();
    let start = regex::Regex::new(r"^(\s*)(_j\d+) := _i\d+ \+ 1$").unwrap();
    let lane = regex::Regex::new(r"^\t\t(_r\d+) := \w+\[outIdx:\]\[:(.+)\]$").unwrap();
    let out = regex::Regex::new(r"^\t\t(_o\d+) := 0$").unwrap();
    let window = regex::Regex::new(r"\[_t\d+-_s\d+\]").unwrap();
    let rescans_of = |body: &str, cursor: &str| -> usize {
        let lines: Vec<&str> = body.lines().collect();
        let mut n = 0;
        for (k, l) in lines.iter().enumerate() {
            let Some(c) = start.captures(l) else { continue };
            let (ind, j) = (&c[1], &c[2]);
            let label = format!("L{j}");
            assert_eq!(lines.get(k + 1).copied(), Some(format!("{}{label}:", &ind[1..]).as_str()), "`{j}` loops through no label:\n{body}");
            assert_eq!(lines.get(k + 2).copied(), Some(format!("{ind}if {j} <= {cursor} {{").as_str()), "`{j}` does not run to the bar cursor:\n{body}");
            assert_eq!(body.matches(&format!("{label}:")).count(), 1, "`{label}` labels more than its rescan:\n{body}");
            let gotos: Vec<usize> = lines.iter().enumerate().filter(|(_, l)| l.trim() == format!("goto {label}")).map(|(g, _)| g).collect();
            assert_eq!(gotos.len(), 2, "`{label}` has not one back edge per path:\n{body}");
            assert!(gotos.iter().all(|g| lines[g - 1].trim() == format!("{j}++")), "a back edge to `{label}` skips the step:\n{body}");
            n += 1;
        }
        n
    };
    if !common::sweep_filtered() {
        let cases: [(&str, &[&str], &[&str]); 2] = [("maxindex", &["inReal"], &["if tmp >= highest {"]), ("aroonosc", &["inHigh", "inLow"], &["if tmp <= lowest {", "if tmp >= highest {"])];
        for (name, inputs, ties) in cases {
            let body = batch_body(name);
            let lines: Vec<&str> = body.lines().collect();
            let h = lines.iter().position(|l| bar.is_match(l)).unwrap_or_else(|| panic!("{name}: no bar cursor:\n{body}"));
            let c = bar.captures(lines[h]).expect("the bar loop");
            let end = h + lines[h..].iter().position(|l| *l == format!("{}}}", &c[1])).expect("the bar loop closes");
            let pass = &lines[h + 1..end];
            for a in inputs {
                assert!(body.contains(&format!(" := {a}[:int(endIdx)+1]")) || body.contains(&format!(" := {a}[:len(_x")), "{name}: `{a}` is not cut to endIdx+1:\n{body}");
                assert!(!pass.iter().any(|l| l.contains(&format!("{a}["))), "{name}: `{a}` is indexed in the loop:\n{body}");
            }
            assert_eq!(rescans_of(&pass.join("\n"), &c[2]), inputs.len(), "{name}: not one rescan per input:\n{body}");
            assert!(!pass.iter().any(|l| l.contains("[:") || l.contains(":]")), "{name}: a window is cut in the loop:\n{body}");
            for t in ties {
                assert!(pass.iter().any(|l| l.trim() == *t), "{name}: `{t}` is lost:\n{body}");
            }
        }
    }
    let (mut scans, mut rescans, mut laned) = (0, 0, 0);
    for name in common::discover_indicators() {
        let body = batch_body(&name);
        assert!(!window.is_match(&body), "{name}: an output is written at the bars since entry:\n{body}");
        let Some(c) = body.lines().find_map(|l| bar.captures(l)) else { continue };
        scans += 1;
        assert!(!body.contains("for _j"), "{name}: a rescan keeps its `for`:\n{body}");
        rescans += rescans_of(&body, &c[2]);
        let lanes: Vec<(String, String)> = body.lines().filter_map(|l| lane.captures(l)).map(|l| (l[1].to_string(), l[2].to_string())).collect();
        let Some((first, _)) = lanes.first() else { continue };
        laned += 1;
        assert!(lanes[1..].iter().all(|(_, cut)| *cut == format!("len({first})")), "{name}: a later lane is not cut to `{first}`:\n{body}");
        let o: Vec<String> = body.lines().filter_map(|l| out.captures(l)).map(|l| l[1].to_string()).collect();
        assert_eq!(o.len(), 1, "{name}: not one output counter:\n{body}");
        assert_eq!(body.lines().filter(|l| l.trim() == format!("{}++", o[0])).count(), 1, "{name}: the output counter is not stepped once a pass:\n{body}");
        assert!(body.contains(&format!("outIdx += int32(len({first}))")), "{name}: outIdx does not move by the lanes' length:\n{body}");
    }
    assert!(common::sweep_filtered() || (scans >= 9 && rescans >= scans && laned >= 7), "only {scans} index-tracking scans, {rescans} rescans, {laned} with output lanes");
}

/// A loop that calls a math function gc keeps a call stores the
/// out-parameters first, at their values after it, and returns right after
/// it, the stores after its block staying for an empty range: EXP and SINH,
/// neither unrolled. SQRT's square root is an instruction: its stores stay
/// after its loop, which is unrolled.
#[test]
fn go_out_parameters_are_stored_before_a_loop_that_calls_out() {
    let guard = regex::Regex::new(r"^\tif (_n\d+) := .+; _n\d+ > 0 \{$").unwrap();
    let early = |body: &str| body.contains("\t\t*outNBElement = outIdx + int32(_n");
    if !common::sweep_filtered() {
        for name in ["exp", "sinh"] {
            let body = batch_body(name);
            let lines: Vec<&str> = body.lines().collect();
            let g = lines.iter().position(|l| guard.is_match(l)).unwrap_or_else(|| panic!("{name}: no guarded loop:\n{body}"));
            let n = guard.captures(lines[g]).expect("the guard")[1].to_string();
            let end = g + lines[g..].iter().position(|l| *l == "\t}").expect("the guard closes");
            let after: Vec<&str> = lines[end + 1..].iter().map(|l| l.trim()).collect();
            assert_eq!(after.last(), Some(&"return Success"), "{name}:\n{body}");
            let stores = &after[..after.len() - 1];
            let ahead: Vec<String> = stores.iter().map(|l| l.replace(" = outIdx", &format!(" = outIdx + int32({n})"))).collect();
            let first: Vec<String> = lines[g + 1..=g + stores.len()].iter().map(|l| l.trim().to_string()).collect();
            assert_eq!(first, ahead, "{name}: the stores do not open the block:\n{body}");
            assert_eq!(lines[end - 1].trim(), "return Success", "{name}: no return after the loop:\n{body}");
            assert!(unrolled_loops(&body).is_empty(), "{name}: unrolled:\n{body}");
        }
        let sqrt = batch_body("sqrt");
        assert!(!early(&sqrt) && unrolled_loops(&sqrt).len() == 1, "sqrt:\n{sqrt}");
    }
    let sites = common::discover_indicators().iter().filter(|n| early(&batch_body(n))).count();
    assert!(common::sweep_filtered() || sites >= 17, "only {sites} loops store the out-parameters first");
}

/// A call gc keeps a call: a `math.` function but those Go compiles to an
/// instruction or always inlines.
fn calls_out(line: &str) -> bool {
    let inline = ["Sqrt", "Abs", "Max", "Min", "FMA", "Float64frombits", "Float64bits", "IsNaN", "IsInf", "Inf", "NaN"];
    regex::Regex::new(r"math\.(\w+)\(").unwrap().captures_iter(line).any(|c| c.get(1).is_some_and(|m| !inline.contains(&m.as_str())))
}

/// HT_PHASOR's bar loop calls `math.Atan`, carries its filters and indexes
/// their rings: it keeps its checks. LINEARREG_ANGLE's and CHOP's carry sums
/// and call out but index no ring, and EXP's carries nothing: each ranges
/// over re-slices. Over the corpus no re-sliced loop that calls out indexes
/// an array but a re-slice, an input or an output.
#[test]
fn go_a_stateful_loop_calling_out_over_a_ring_is_not_re_sliced() {
    let indexed = regex::Regex::new(r"(^|[^\w.])([A-Za-z_]\w*)\[").unwrap();
    let contract = regex::Regex::new(r"^(_|in[A-Z0-9]|out[A-Z0-9])").unwrap();
    if !common::sweep_filtered() {
        let phasor = batch_body("ht_phasor");
        assert!(!re_sliced_loops(&phasor).iter().any(|(pass, ..)| pass.iter().any(|l| calls_out(l))), "ht_phasor: its bar loop is re-sliced:\n{phasor}");
        assert!(phasor.contains("\t\ttodayValue = inReal[today]\n"), "ht_phasor: its bar loop does not read the input at the bar:\n{phasor}");
        for (name, call) in [("linearreg_angle", "math.Atan("), ("chop", "math.Log10("), ("exp", "math.Exp(")] {
            let body = batch_body(name);
            assert!(re_sliced_loops(&body).iter().any(|(pass, ..)| pass.iter().any(|l| l.contains(call))), "{name}: its loop calling `{call}` is not re-sliced:\n{body}");
        }
    }
    let mut loops = 0usize;
    for name in common::discover_indicators() {
        let body = batch_body(&name);
        for (pass, ..) in re_sliced_loops(&body).iter().filter(|(pass, ..)| pass.iter().any(|l| calls_out(l))) {
            for c in pass.iter().flat_map(|l| indexed.captures_iter(l)) {
                assert!(contract.is_match(&c[2]), "{name}: a re-sliced loop that calls out indexes `{}`:\n{}", &c[2], pass.join("\n"));
            }
            loops += 1;
        }
    }
    assert!(common::sweep_filtered() || loops >= 20, "only {loops} re-sliced loops call out");
}

/// SWAK_GAUSS shifts its output through `y2 = y1; y1 = y` into nested fused
/// sums: both its loops keep their checks. WAVETREND's `prevEsa = esa` feeds
/// no fused sum, and T3's chain copies no carried value: both range over
/// re-slices. Over the corpus no re-sliced loop holds such a shift.
#[test]
fn go_a_two_variable_shift_into_a_fused_sum_is_not_re_sliced() {
    let copy = regex::Regex::new(r"^\s*(\w+) = (\w+)$").unwrap();
    let written = |pass: &[String], v: &str| pass.iter().filter(|l| l.trim_start().starts_with(&format!("{v} = "))).count();
    if !common::sweep_filtered() {
        let gauss = batch_body("swak_gauss");
        assert!(re_sliced_loops(&gauss).is_empty(), "swak_gauss: a loop is re-sliced:\n{gauss}");
        assert_eq!(gauss.matches("\t\ty2 = y1\n").count(), 2, "swak_gauss: its two loops:\n{gauss}");
        for (name, line) in [("wavetrend", "prevEsa = esa"), ("t3", "math.FMA(")] {
            let body = batch_body(name);
            assert!(re_sliced_loops(&body).iter().any(|(pass, ..)| pass.iter().any(|l| l.contains(line))), "{name}: its recurrence is not re-sliced:\n{body}");
        }
    }
    let mut loops = 0usize;
    for name in common::discover_indicators() {
        let body = batch_body(&name);
        for (pass, ..) in re_sliced_loops(&body).iter().filter(|(pass, ..)| pass.iter().any(|l| l.contains("math.FMA("))) {
            for c in pass.iter().filter_map(|l| copy.captures(l)) {
                let (p, q) = (&c[1], &c[2]);
                let named = |v: &str| regex::Regex::new(&format!(r"\b{v}\b")).unwrap();
                let fused = pass.iter().any(|l| l.contains("math.FMA(") && named(p).is_match(l));
                let carried = pass.iter().find(|l| named(q).is_match(l)).is_some_and(|l| !l.trim_start().starts_with(&format!("{q} = ")));
                assert!(!(fused && carried && written(pass, p) == 1 && written(pass, q) >= 1), "{name}: a re-sliced loop shifts `{q}` into `{p}`, read by a fused sum:\n{}", pass.join("\n"));
            }
            loops += 1;
        }
    }
    assert!(common::sweep_filtered() || loops >= 99, "only {loops} re-sliced loops fuse");
}

/// The labelled scans of `body`: each label's step, its test, and the lines
/// of the `if` it labels.
fn back_edge_scans(body: &str) -> Vec<(String, String, Vec<String>)> {
    let label = regex::Regex::new(r"^(\t*)(scan\d+):$").unwrap();
    let lines: Vec<&str> = body.lines().collect();
    let mut found = Vec::new();
    for (k, l) in lines.iter().enumerate() {
        let Some(c) = label.captures(l) else { continue };
        let close = format!("{}\t}}", &c[1]);
        let end = k + lines[k..].iter().position(|x| *x == close).expect("the labelled if closes");
        let block: Vec<String> = lines[k + 1..=end].iter().map(|x| x.to_string()).collect();
        found.push((lines[k - 1].trim().to_string(), c[2].to_string(), block));
    }
    found
}

/// A stream frame's update scan gives the path that updates nothing its own
/// back edge: a label on the loop test, and in each exit the step then a
/// `goto` to it, the comparison unchanged. A block scan's batch body keeps its
/// loops: there the form timed slower.
#[test]
fn go_stream_update_scans_take_their_own_back_edge() {
    let source = |name: &str| {
        let (func, enums) = common::load_indicator(name);
        backends::go::generate(&func, &enums, common::make_registry(), common::make_helpers())
    };
    let cases: [(&str, &str, &[&str]); 3] = [
        ("max", "func (sp *MAXStream) Update(", &["if tmp > highest {"]),
        ("max", "func (sp *MAXStream) Peek(", &["if tmp > highest {"]),
        ("willr", "func (sp *WILLRStream) Update(", &["if tmp < lowest {", "if tmp > highest {"]),
    ];
    for (name, decl, compares) in cases {
        let src = source(name);
        let body = go_body(&src, decl);
        let scans = back_edge_scans(body);
        assert_eq!(scans.len(), compares.len(), "{decl}: one label per rescan:\n{body}");
        assert!(!body.lines().any(|l| l.trim() == "for {"), "{decl}: a rescan kept its `for`:\n{body}");
        for ((step, label, block), cmp) in scans.iter().zip(compares) {
            assert_eq!((step.as_str(), block[0].trim()), ("i++", "if i <= today {"), "{decl}: {label} is not stepped then tested:\n{body}");
            let goto = format!("goto {label}");
            let at: Vec<usize> = block.iter().enumerate().filter(|(_, l)| l.trim() == goto).map(|(i, _)| i).collect();
            assert_eq!(at.len(), 2, "{decl}: {label} has not two back edges:\n{body}");
            assert!(at.iter().all(|&i| block[i - 1].trim() == "i++"), "{decl}: a back edge of {label} skips the step:\n{body}");
            assert!(block.iter().any(|l| l.trim() == *cmp), "{decl}: `{cmp}` is lost:\n{body}");
        }
    }
    let max = source("max");
    let batch = go_body(&max, "func (c *Core) maxImpl(");
    assert!(!batch.contains("goto ") && back_edge_scans(batch).is_empty(), "maxImpl took a back edge:\n{batch}");
    if common::sweep_filtered() {
        return;
    }
    let mut rewritten = 0usize;
    for name in common::discover_indicators() {
        let scans = back_edge_scans(&source(&name));
        for (_, label, block) in &scans {
            assert!(!block.iter().any(|l| l.contains("sp.")), "{name}: {label} reads the handle:\n{}", block.join("\n"));
        }
        rewritten += scans.len();
    }
    assert!(rewritten >= 94, "only {rewritten} stream rescans take their own back edge");
}

/// A composed frame's rescan runs on locals as a plain frame's does: STOCH's
/// Update renders its producer beside owned sub-streams, loads each field its
/// scans name ahead of the label, and stores back right after the scan only
/// the fields it writes.
#[test]
fn go_a_composed_update_rescans_on_locals() {
    let (func, enums) = common::load_indicator("stoch");
    let src = backends::go::generate(&func, &enums, common::make_registry(), common::make_helpers());
    let body = go_body(&src, "func (sp *STOCHStream) Update(");
    assert!(body.contains("sp.sub0.Update(") && body.contains("sp.sub1.Update("), "STOCH's Update is not composed:\n{body}");
    let lines: Vec<&str> = body.lines().collect();
    let scans = back_edge_scans(body);
    assert_eq!(scans.len(), 2, "one label per rescan:\n{body}");
    for ((_, label, block), (best, ring)) in scans.iter().zip([("lowest", "x_inLow"), ("highest", "x_inHigh")]) {
        assert!(!block.iter().any(|l| l.contains("sp.")), "{label} reads the handle:\n{body}");
        let at = lines.iter().position(|l| l.trim() == format!("{label}:")).expect("the label");
        let loads: BTreeSet<String> = lines[..at].iter().rev().skip(1).map(|l| l.trim().to_string()).take_while(|l| l.split_once(" = sp.").is_some_and(|(a, b)| a == b)).collect();
        let want: BTreeSet<String> = ["i", best, format!("{best}Idx").as_str(), "today", "xMask", ring].iter().map(|f| format!("{f} = sp.{f}")).collect();
        assert_eq!(loads, want, "{label} does not load its fields ahead of it:\n{body}");
        let after = at + block.len() + 1;
        let stores: BTreeSet<String> = lines[after..].iter().map(|l| l.trim().to_string()).take_while(|l| l.strip_prefix("sp.").and_then(|r| r.split_once(" = ")).is_some_and(|(a, b)| a == b)).collect();
        let want: BTreeSet<String> = ["i", best, format!("{best}Idx").as_str()].iter().map(|f| format!("sp.{f} = {f}")).collect();
        assert_eq!(stores, want, "{label} does not store back exactly what it writes:\n{body}");
    }
}

/// RSI's Update computes its gain select before it stores any handle field:
/// a select after the stores reads each carried sum back from the handle, a
/// reload per bar.
#[test]
fn go_rsi_update_selects_ahead_of_its_stores() {
    let (func, enums) = common::load_indicator("rsi");
    let src = backends::go::generate(&func, &enums, common::make_registry(), common::make_helpers());
    let body = go_body(&src, "func (sp *RSIStream) Update(");
    let lines: Vec<&str> = body.lines().collect();
    let select = lines.iter().position(|l| l.contains("selF(")).unwrap_or_else(|| panic!("RSI's Update has no select:\n{body}"));
    let store = lines.iter().position(|l| l.trim().starts_with("sp.prev")).unwrap_or_else(|| panic!("RSI's Update stores no carried field:\n{body}"));
    assert!(select < store, "RSI's Update stores a carried field before its gain select:\n{body}");
}

/// An update scan whose comparison renders as a select keeps its step inside
/// its loop.
#[test]
fn go_an_update_scan_rendered_with_a_select_steps_in_its_loop() {
    let path = std::path::Path::new(env!("CARGO_MANIFEST_DIR")).join("../../ta_codegen/input/max/max.c");
    let anchor = "   outIdx = 0;\n   today       = startIdx;\n   trailingIdx = startIdx-nbInitialElementNeeded;\n";
    let scan = "   i = trailingIdx;\n   highest = inReal[i];\n   while( ++i<today )\n   {\n      tmp = inReal[i];\n      outReal[0] = tmp;\n      if( tmp > highest )\n      {\n         highest = tmp;\n      }\n   }\n";
    let base = std::fs::read_to_string(path).unwrap();
    let at = base.rfind(anchor).expect("max.c moved its anchor") + anchor.len();
    let source = format!("{}{scan}{}", &base[..at], &base[at..]);
    let (mut func, enums) = common::load_indicator_with_source("max", &source);
    func.streaming = true;
    let src = backends::go::generate(&func, &enums, common::make_registry(), common::make_helpers());
    let body = go_body(&src, "func (c *Core) maxOpenImpl(");
    let lines: Vec<&str> = body.lines().collect();
    let at = lines.iter().position(|l| l.trim().starts_with("outReal[0") && l.ends_with("] = tmp")).unwrap_or_else(|| panic!("the scan is lost:\n{body}"));
    let indent = |l: &str| l.len() - l.trim_start_matches('\t').len();
    let open = (0..at).rev().find(|&k| lines[k].trim() == "for {" && indent(lines[k]) < indent(lines[at])).expect("the scan is a loop");
    let close = format!("{}}}", &lines[open][..indent(lines[open])]);
    let end = open + lines[open..].iter().position(|l| *l == close).expect("the loop closes");
    let stepped = lines[open..end].iter().any(|l| matches!(l.trim(), "i++" | "i += 1"));
    assert!(stepped && !lines[open..end].iter().any(|l| l.contains("goto ")), "the scan's loop does not step:\n{body}");
}

fn kama_impl_with(edit: impl Fn(&str) -> String) -> String {
    let source = std::fs::read_to_string(concat!(env!("CARGO_MANIFEST_DIR"), "/../input/kama/kama.c")).unwrap();
    let patched = edit(&source);
    assert_ne!(patched, source, "the edit does not apply");
    let (func, enums) = common::load_indicator_with_source("kama", &patched);
    let src = backends::go::generate(&func, &enums, common::make_registry(), common::make_helpers());
    go_body(&src, "func (c *Core) kamaImpl(").to_string()
}

/// A real expression two neighbouring statements compute is bound once, in
/// every tier, unless a store between them could change it or the first one
/// fuses its product into a sum.
#[test]
fn go_binds_an_expression_two_neighbouring_statements_compute() {
    let (func, enums) = common::load_indicator("kama");
    let src = backends::go::generate(&func, &enums, common::make_registry(), common::make_helpers());
    let bound = regex::Regex::new(r"(?m)^\t+(_e\d+) := math\.Abs\(tempReal - [^\n]+\)\n\t+(sp\.)?sumROC1 \+= (_e\d+)\n\t+live = b2i\(!\((_e\d+) <= 0\.0\)\)$").unwrap();
    for decl in ["func (c *Core) kamaImpl(", "func (sp *KAMAStream) Update(", "func (sp *KAMAStream) Peek("] {
        let body = go_body(&src, decl);
        let caps: Vec<_> = bound.captures_iter(body).collect();
        assert!(!caps.is_empty(), "{decl}: the difference is not bound once");
        assert!(caps.iter().all(|c| c[1] == c[3] && c[1] == c[4]), "{decl}: the bound local is not the one both uses read");
        assert_eq!(body.matches("math.Abs(tempReal-").count(), 0, "{decl}: the difference is still computed twice");
    }

    let unbound = regex::Regex::new(r"_e\d+ := math\.Abs\(tempReal - ").unwrap();
    let site = "      live = fabs(tempReal - inReal[today-1]) <= 0.0 ? 0 : 1;";
    for between in ["tempReal = inReal[today];", "outReal[0] = tempReal2;", "periodROC = tempReal2 + ta_true_range(tempReal, tempReal2, periodROC);"] {
        let body = kama_impl_with(|s| s.replace(site, &format!("      {between}\n{site}")));
        assert!(!unbound.is_match(&body) && body.contains("math.Abs(tempReal-"), "`{between}` between the uses must keep both computations:\n{body}");
    }

    let product = "      periodROC = tempReal-tempReal2;\n";
    let with = |sum: &str| kama_impl_with(|s| s.replacen(product, &format!("{product}      periodROC = (tempReal*tempReal2){sum}periodROC;\n      sumROC1 += tempReal*tempReal2;\n"), 1));
    let bound_product = regex::Regex::new(r"_e\d+ := float64\(tempReal \* tempReal2\)").unwrap();
    assert!(bound_product.is_match(&with("-")), "a product two statements round is not bound");
    let fused = with("+");
    assert!(fused.contains("math.FMA(tempReal, tempReal2, periodROC)") && !bound_product.is_match(&fused), "a product one statement fuses is bound:\n{fused}");

    let (registry, helpers) = (common::make_registry(), common::make_helpers());
    let decl = regex::Regex::new(r"(?m)^\t+_e\d+ := ").unwrap();
    let total: usize = common::discover_indicators()
        .iter()
        .map(|name| {
            let (func, enums) = common::load_indicator(name);
            decl.find_iter(&backends::go::generate(&func, &enums, registry, helpers)).count()
        })
        .sum();
    assert!(common::sweep_filtered() || total >= 71, "the corpus binds {total} expressions");
}

/// CCI reads a ring element inside a call's argument, a value and not the
/// buffer, so its ring is a stack array up to C's static size.
#[test]
fn go_a_ring_read_in_a_call_argument_lives_on_the_stack() {
    let body = batch_body("cci");
    assert!(body.contains("math.Abs(circBuffer["), "CCI no longer reads its ring in a call:\n{body}");
    assert!(body.contains("\tvar local_circBuffer [30]float64\n"), "no stack array:\n{body}");
    let pick = "\tif optInTimePeriod <= 30 {\n\t\tcircBuffer = local_circBuffer[:optInTimePeriod]\n\t} else {\n\t\tcircBuffer = make([]float64, optInTimePeriod)\n\t}\n";
    assert!(body.contains(pick) && body.matches("circBuffer = make(").count() == 1, "the ring does not pick the stack array:\n{body}");
}
