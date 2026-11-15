//! Go `stream_verify`: Rust's legs, in Go's idiom, plus the aliasing probes a
//! slice can reach and the peek-repeat probe. The server is a separate module,
//! so like Rust's it cannot reach the anchored `_OpenInternal` seam; anchored
//! opens run inside the package through every composed function's own legs.

use super::{collect_pin_ids, sv_guard_enum_ints, sv_input_suffix, sv_range_bit, sv_reject_condition, SvRangeSite, SV_RANGE_MASK_GO};
use crate::ir::{EnumDef, FuncDef, ParamType};
use std::collections::HashMap;
use std::fmt::Write as _;

fn sv_go_input_array(name: &str, generic_idx: &mut usize) -> &'static str {
    match sv_input_suffix(name, generic_idx) {
        "o" => "fzO",
        "h" => "fzH",
        "l" => "fzL",
        "c" => "fzC",
        "v" => "fzV",
        _ => "fzOI",
    }
}

struct L {
    s: String,
    depth: usize,
}

impl L {
    fn line(&mut self, t: impl AsRef<str>) {
        for _ in 0..self.depth {
            self.s.push('\t');
        }
        self.s.push_str(t.as_ref());
        self.s.push('\n');
    }
    fn open(&mut self, t: impl AsRef<str>) {
        self.line(t);
        self.depth += 1;
    }
    fn close(&mut self, t: impl AsRef<str>) {
        self.depth -= 1;
        self.line(t);
    }
    fn mid(&mut self, t: impl AsRef<str>) {
        self.depth -= 1;
        self.line(t);
        self.depth += 1;
    }
    fn guard(&mut self, cond: impl AsRef<str>, body: &[&str]) {
        self.open(format!("if {} {{", cond.as_ref()));
        for b in body {
            self.line(b);
        }
        self.close("}");
    }
}

/// `allOk = false` with the first diagnostic kept.
fn fail_diag(l: &mut L, flag: &str, diag: &str) {
    l.line(format!("{flag} = false"));
    l.guard("diag == \"\"", &[&format!("diag = {diag}")]);
}

/// A Go raw string for a JSON fragment.
fn raw(json: &str) -> String {
    format!("`{json}`")
}

#[allow(clippy::too_many_lines, clippy::cast_possible_truncation, clippy::cognitive_complexity)]
fn emit_go_sv_func(func: &FuncDef, funcs: &[FuncDef], enums: &HashMap<String, EnumDef>) -> String {
    let name = &func.name;
    let candle = func.is_candlestick();
    let inputs = crate::streaming::input_array_names(func);
    let mut gi = 0usize;
    let arrays: Vec<&'static str> = inputs.iter().map(|i| sv_go_input_array(i, &mut gi)).collect();
    let n_out = func.outputs.len();
    let out_is_int: Vec<bool> = func.outputs.iter().map(|o| o.param_type == ParamType::Integer).collect();
    let opts: Vec<String> = func.optional_inputs.iter().map(|o| o.name.clone()).collect();
    let ins = |r: &str| -> Vec<String> { arrays.iter().map(|a| if r.is_empty() { (*a).to_string() } else { format!("{a}[{r}]") }).collect() };
    let args = |parts: &[Vec<String>]| -> String { parts.iter().flatten().cloned().collect::<Vec<_>>().join(", ") };
    let names = |p: &str| -> Vec<String> { (0..n_out).map(|i| format!("{p}{i}")).collect() };
    let bars_at = |t: &str| ins(t).join(", ");
    let canary = |i: usize| if out_is_int[i] { "svCanaryI(svN)" } else { "svCanaryF(svN)" };
    let fbufs: Vec<String> = names("f");
    // Same-tier compares are strict bits; cross-tier ones let +0.0/-0.0 through.
    let ne = |i: usize, a: &str, b: &str| if out_is_int[i] { format!("{a} != {b}") } else { format!("svBne({a}, {b})") };
    let off_batch = |i: usize, a: &str, idx: &str| {
        if out_is_int[i] {
            format!("{a} != b{i}[{idx}]")
        } else {
            format!("svXtierNe({a}, b{i}[{idx}], &zsign)")
        }
    };

    let open_lhs = |p: &str, err: &str| -> String {
        let mut v = vec![p.to_string()];
        v.extend((0..n_out).map(|_| "_".to_string()));
        v.push(err.to_string());
        v.join(", ")
    };

    let mut l = L { s: String::new(), depth: 0 };
    l.open(format!("func sv{name}(s *server, p params) string {{"));
    l.line("svShape := p.i32(\"gen_shape\", 0)");
    l.line("svSeed := p.i32(\"gen_seed\", 0)");
    l.line("svN := min(max(int(p.i32(\"gen_n\", 0)), 2), 256)");
    let pin_ids = collect_pin_ids(func, funcs, enums);
    if !pin_ids.is_empty() {
        l.line("svK := int(p.i32(\"unstablePeriod\", 0))");
    }
    if candle {
        l.line("candleLegs := p.i64(\"candleLegs\", 0)");
    }
    for prm in &func.optional_inputs {
        let n = &prm.name;
        match &prm.param_type {
            ParamType::Real => l.line(format!("{n} := p.realOr(\"{n}\", {:?})", prm.default.unwrap_or(0.0))),
            ParamType::Enum(e) => l.line(format!("{n} := talib.{e}(p.i32(\"{n}\", int32(talib.{e}DEFAULT)))")),
            _ => l.line(format!("{n} := int(p.i32(\"{n}\", {}))", prm.default.unwrap_or(0.0) as i64)),
        }
    }
    l.line("fzO, fzH, fzL, fzC, fzV, fzOI := make([]float64, svN), make([]float64, svN), make([]float64, svN), make([]float64, svN), make([]float64, svN), make([]float64, svN)");
    l.line("fuzzGen(svShape, svSeed, svN, fzO, fzH, fzL, fzC, fzV, fzOI)");
    {
        let lookup = crate::streaming::FuncsLookup(funcs);
        if let Ok(crate::streaming::StreamPlan::PeriodBank(pb)) = crate::streaming::validate_streamable(func, &lookup) {
            if let Some(idx) = inputs.iter().position(|i| *i == pb.period_input) {
                // A ramp over [min-1, max+1]: the fuzz period series would clamp
                // to the maximum on every bar and leave every other slot idle.
                let arr = arrays[idx];
                l.open(format!("for pi := range {arr} {{"));
                l.line(format!("{arr}[pi] = float64({} + pi%({}-{}+3) - 1)", pb.min_param, pb.max_param, pb.min_param));
                l.close("}");
            }
        }
    }
    for (i, is_int) in out_is_int.iter().enumerate() {
        l.line(format!("b{i} := make({}, svN)", if *is_int { "[]int32" } else { "[]float64" }));
    }
    for v in [
        "legs := 0",
        "roundMask := 0",
        "allOk := true",
        "peekAll := true",
        "peekReps := 0",
        "peekRejects := 0",
        "peekRepAll := true",
        "fillChecked := 0",
        "fillOk := true",
        "beg, nb := 0, 0",
        "diag := \"\"",
        "rangeChecked := 0",
        "rangeOk := true",
        "rangeLegs := 0",
        "rangeSites := 0",
        "valueChecked := 0",
        "valueOk := true",
        "valueLegs := 0",
        "cloneChecked := 0",
        "cloneOk := true",
        "cloneLegs := 0",
        "zsign := int64(0)",
    ] {
        l.line(v);
    }
    l.line("rounds := 1");
    if candle {
        l.guard("candleLegs != 0", &["rounds = 5"]);
    }
    l.open("for rd := 0; rd < rounds; rd++ {");
    l.line("core := *s.core");
    l.line("c2 := &core");
    for id in &pin_ids {
        l.guard(
            format!("c2.SetUnstablePeriod(talib.FuncUnstId({id}), svK) != nil"),
            &[&format!("return {}", raw(r#"{"error":"unstablePeriod out of range"}"#))],
        );
    }
    if candle {
        l.line("svCandleRound(c2, rd)");
    }

    if let Some(guard) = sv_reject_condition(func, funcs, None) {
        let guard = sv_guard_enum_ints(&guard, enums);
        l.open(format!("if {guard} {{"));
        l.line(format!("{} := c2.{name}Open({})", open_lhs("_", "err1"), args(&[ins(""), opts.clone()])));
        for (i, f) in fbufs.iter().enumerate() {
            l.line(format!("{f} := {}", canary(i)));
        }
        l.line(format!("_, _, err2 := c2.{name}OpenAndFill({})", args(&[ins(""), opts.clone(), fbufs.clone()])));
        l.line("okr := err1 != nil && err2 != nil");
        l.line(format!("return fmt.Sprintf({}, b2i(okr))", raw(r#"{"retCode":0,"legs":0,"unsupportedArm":1,"ok":%d,"peek_ok":1}"#)));
        l.close("}");
    }

    // Batch leg.
    l.line("rc := 0");
    l.line(format!("br, err := c2.{name}(0, svN-1, {})", args(&[ins(""), opts.clone(), names("b")])));
    l.open("if err != nil {");
    l.line("rc = retCodeOf(err)");
    l.line("beg, nb = 0, 0");
    l.mid("} else {");
    l.line("beg, nb = br.BegIdx, br.NBElement");
    l.close("}");
    l.line(format!("lb, lbErr := c2.{name}Lookback({})", opts.join(", ")));
    l.guard("lbErr != nil", &["lb = 1 << 30"]);
    l.open("if rc != 0 || nb == 0 {");
    l.line(format!("{} := c2.{name}Open({})", open_lhs("_", "oErr"), args(&[ins(""), opts.clone()])));
    l.line("openRejects := oErr != nil");
    if candle {
        l.guard("!openRejects", &["allOk = false"]);
        l.guard("rd+1 < rounds", &["continue"]);
        l.line(format!(
            "return fmt.Sprintf({}, rc, legs, nb, b2i(openRejects), b2i(allOk), b2i(peekAll), peekReps, b2i(peekRepAll), peekRejects, zsign, roundMask)",
            raw(r#"{"retCode":%d,"legs":%d,"nb":%d,"openRejects":%d,"ok":%d,"peek_ok":%d,"peek_reps":%d,"peek_rep_ok":%d,"peek_rejects":%d,"benign":%d,"round_mask":%d}"#)
        ));
    } else {
        l.line(format!(
            "return fmt.Sprintf({}, rc, nb, b2i(openRejects), b2i(openRejects))",
            raw(r#"{"retCode":%d,"legs":0,"nb":%d,"openRejects":%d,"ok":%d,"peek_ok":1}"#)
        ));
    }
    l.close("}");

    // OpenAndFill leg: the fill against batch, the canary slack above it, and
    // the aliasing rejections.
    l.line("fillChecked = 1");
    l.open("{");
    for (i, f) in fbufs.iter().enumerate() {
        l.line(format!("{f} := {}", canary(i)));
    }
    l.line(format!("fh, fr, err := c2.{name}OpenAndFill({})", args(&[ins(""), opts.clone(), fbufs.clone()])));
    l.open("if err != nil {");
    l.line("fillOk = false");
    l.mid("} else {");
    l.line("rangeChecked = 1");
    l.line("rangeLegs++");
    l.line(format!("rangeSites |= {}", sv_range_bit(SvRangeSite::Fill, SV_RANGE_MASK_GO)));
    l.guard("fh.OutRange() != br", &["rangeOk = false"]);
    l.open("if fr.BegIdx != beg || fr.NBElement != nb {");
    l.line("fillOk = false");
    l.mid("} else {");
    for i in 0..n_out {
        l.open("for i := 0; i < nb; i++ {");
        l.guard(off_batch(i, &format!("f{i}[i]"), "i"), &["fillOk = false"]);
        l.close("}");
    }
    for (i, is_int) in out_is_int.iter().enumerate() {
        l.open("for i := nb; i < svN; i++ {");
        if *is_int {
            l.guard(format!("f{i}[i] != svCanaryI32"), &["fillOk = false"]);
        } else {
            l.guard(format!("svBne(f{i}[i], svCanary)"), &["fillOk = false"]);
        }
        l.close("}");
    }
    l.close("}");
    l.close("}");
    if !out_is_int[0] {
        let mut alias = fbufs.clone();
        alias[0] = arrays[0].to_string();
        l.guard(
            format!("_, _, err := c2.{name}OpenAndFill({}); !errors.Is(err, talib.BadParam)", args(&[ins(""), opts.clone(), alias])),
            &["fillOk = false"],
        );
        if n_out > 1 && !out_is_int[1] {
            let mut alias2 = fbufs.clone();
            alias2[1] = "f0".to_string();
            l.guard(
                format!("_, _, err := c2.{name}OpenAndFill({}); !errors.Is(err, talib.BadParam)", args(&[ins(""), opts.clone(), alias2])),
                &["fillOk = false"],
            );
        }
    }
    l.close("}");

    // Prefix sweep: open at each prefix, compare the open value, then walk
    // peek/update to the end of the corpus against the batch arrays.
    l.line("pcs := []int{lb + 1, lb + 13, svN / 2, svN - 1}");
    l.line("sort.Ints(pcs)");
    l.line("first := -1");
    l.line("prevP := -1");
    l.open("for _, P := range pcs {");
    l.guard("P < lb+1 || P > svN-1 || P == prevP", &["continue"]);
    l.line("prevP = P");
    l.guard("first < 0", &["first = P"]);
    let v0 = names("v");
    l.line(format!("st, {}, err := c2.{name}Open({})", v0.join(", "), args(&[ins(":P"), opts.clone()])));
    l.open("if err != nil {");
    fail_diag(&mut l, "allOk", "fmt.Sprintf(`,\"openRejectP\":%d`, P)");
    l.line("continue");
    l.close("}");
    l.line("legs++");
    l.line("roundMask |= 1 << rd");
    for i in 0..n_out {
        l.open(format!("if {} {{", off_batch(i, &format!("v{i}"), "P-1-beg")));
        fail_diag(&mut l, "allOk", &format!("fmt.Sprintf(`,\"badBar\":%d,\"badOut\":{i},\"where\":\"open\"`, P-1)"));
        l.close("}");
    }
    // Open seeds Value from its own expression, which the dispatch tier and the
    // period bank write apart from OpenAndFill's.
    let va = names("va");
    l.line(format!("{} := st.Value()", va.join(", ")));
    l.line("valueChecked = 1");
    l.line("valueLegs++");
    for i in 0..n_out {
        l.open(format!("if {} {{", ne(i, &format!("va{i}"), &format!("v{i}"))));
        fail_diag(&mut l, "valueOk", "`,\"valueAfterOpen\":1`");
        l.close("}");
    }
    l.open("for t := P; t < svN; t++ {");
    let pk = names("pk");
    let rp = names("rp");
    let up = names("up");
    let blanks = vec!["_"; n_out].join(", ");
    l.line(format!("{}, pkErr := st.Peek({})", pk.join(", "), bars_at("t")));
    l.guard("pkErr != nil", &["peekRejects++"]);
    // The decoy peek of another bar in the middle is what lets a committing
    // peek show: without it both peeks read the same fields.
    l.open("if t%7 == 0 {");
    l.guard(format!("{blanks}, err := st.Peek({}); err != nil", bars_at("t-1")), &["peekRejects++"]);
    l.line(format!("{}, rpErr := st.Peek({})", rp.join(", "), bars_at("t")));
    l.open("if pkErr == nil && rpErr == nil {");
    l.line("peekReps++");
    for i in 0..n_out {
        l.guard(ne(i, &rp[i], &pk[i]), &["peekRepAll = false"]);
    }
    l.mid("} else {");
    l.line("peekRejects++");
    l.close("}");
    l.close("}");
    l.line(format!("{}, err := st.Update({})", up.join(", "), bars_at("t")));
    l.open("if err != nil {");
    fail_diag(&mut l, "allOk", "fmt.Sprintf(`,\"updateRejected\":%d`, t)");
    l.line("break");
    l.close("}");
    l.open("if pkErr == nil {");
    for i in 0..n_out {
        l.guard(ne(i, &pk[i], &up[i]), &["peekAll = false"]);
    }
    l.close("}");
    // Every accepted update refreshes Value; the intervening peek is what lets
    // Value == Update fail.
    l.guard(format!("{blanks}, err := st.Peek({}); err != nil", bars_at("t-1")), &["peekRejects++"]);
    let vu = names("vu");
    l.line(format!("{} := st.Value()", vu.join(", ")));
    l.line("valueLegs++");
    for i in 0..n_out {
        l.open(format!("if {} {{", ne(i, &vu[i], &up[i])));
        fail_diag(&mut l, "valueOk", "`,\"valueAfterUpdate\":1`");
        l.close("}");
    }
    for (i, is_int) in out_is_int.iter().enumerate() {
        l.open(format!("if {} {{", off_batch(i, &up[i], "t-beg")));
        let d = if *is_int {
            format!("fmt.Sprintf(`,\"badBar\":%d,\"badOut\":{i},\"batchv\":\"%d\",\"streamv\":\"%d\"`, t, b{i}[t-beg], {})", up[i])
        } else {
            format!(
                "fmt.Sprintf(`,\"badBar\":%d,\"badOut\":{i},\"batchv\":\"%016x\",\"streamv\":\"%016x\"`, t, math.Float64bits(b{i}[t-beg]), math.Float64bits({}))",
                up[i]
            )
        };
        fail_diag(&mut l, "allOk", &d);
        l.close("}");
    }
    l.close("}");
    // Open(P) plus svN-P updates has consumed svN bars and must report what
    // batch did; then one Advance, last on this handle, reports exactly +1.
    l.open("if allOk {");
    l.line("rangeChecked = 1");
    l.line("rangeLegs++");
    l.line(format!("rangeSites |= {}", sv_range_bit(SvRangeSite::Prefix, SV_RANGE_MASK_GO)));
    l.guard("st.OutRange() != br", &["rangeOk = false"]);
    l.line("rangeLegs++");
    l.line(format!("rangeSites |= {}", sv_range_bit(SvRangeSite::Advance, SV_RANGE_MASK_GO)));
    l.guard("st.Advance() != nil", &["rangeOk = false"]);
    l.guard("r := st.OutRange(); r.BegIdx != beg || r.NBElement != nb+1", &["rangeOk = false"]);
    l.close("}");
    l.close("}");

    // Clone leg: a handle opened by OpenAndFill at the earliest prefix, driven
    // to mid, forked; the fork counts two bars and runs to the end, then the
    // original does. A buffer the two shared shows on the second.
    l.open("if first > 0 {");
    l.line("p0 := first");
    for (i, f) in fbufs.iter().enumerate() {
        l.line(format!("{f} := {}", canary(i)));
    }
    l.line(format!("sa, _, err := c2.{name}OpenAndFill({})", args(&[ins(":p0"), opts.clone(), fbufs.clone()])));
    l.open("if err != nil {");
    fail_diag(&mut l, "cloneOk", "`,\"copyOpenReject\":1`");
    l.mid("} else {");
    l.line(format!("{} := sa.Value()", va.join(", ")));
    l.line("valueLegs++");
    for i in 0..n_out {
        l.open(format!("if {} {{", ne(i, &format!("va{i}"), &format!("f{i}[0]"))));
        fail_diag(&mut l, "valueOk", "`,\"valueAfterFill\":1`");
        l.close("}");
    }
    l.line("mid := (p0 + svN) / 2");
    l.line("forked := true");
    let ua = names("ua");
    l.open("for t := p0; t < mid; t++ {");
    l.line(format!("{}, err := sa.Update({})", ua.join(", "), bars_at("t")));
    l.open("if err != nil {");
    l.line("forked = false");
    fail_diag(&mut l, "cloneOk", "fmt.Sprintf(`,\"copyPreRejected\":%d`, t)");
    l.line("break");
    l.close("}");
    for (i, u) in ua.iter().enumerate() {
        l.open(format!("if {} {{", off_batch(i, u, "t-beg")));
        fail_diag(&mut l, "cloneOk", "fmt.Sprintf(`,\"copyPreDiverged\":%d`, t)");
        l.close("}");
    }
    l.close("}");
    l.line("sb := sa.Clone()");
    l.open("if sb.Advance() != nil || sb.Advance() != nil {");
    l.line("forked = false");
    fail_diag(&mut l, "cloneOk", "`,\"copyAdvanceRejected\":1`");
    l.close("}");
    for (i, is_int) in out_is_int.iter().enumerate() {
        l.line(format!("fk{i} := make({}, svN)", if *is_int { "[]int32" } else { "[]float64" }));
    }
    let ub = names("ub");
    for (handle, us, second) in [("sb", &ub, false), ("sa", &ua, true)] {
        l.open("if forked {");
        l.open("for t := mid; t < svN; t++ {");
        l.line(format!("{}, err := {handle}.Update({})", us.join(", "), bars_at("t")));
        l.open("if err != nil {");
        l.line("forked = false");
        fail_diag(&mut l, "cloneOk", "fmt.Sprintf(`,\"copyRejected\":%d`, t)");
        l.line("break");
        l.close("}");
        for (i, u) in us.iter().enumerate() {
            let cond = if second {
                format!("{} || {}", ne(i, u, &format!("fk{i}[t]")), off_batch(i, u, "t-beg"))
            } else {
                off_batch(i, u, "t-beg")
            };
            l.open(format!("if {cond} {{"));
            fail_diag(&mut l, "cloneOk", "fmt.Sprintf(`,\"copyDiverged\":%d`, t)");
            l.close("}");
            if !second {
                l.line(format!("fk{i}[t] = {u}"));
            }
        }
        l.line(format!("{} := {handle}.Value()", vu.join(", ")));
        l.line("valueLegs++");
        for i in 0..n_out {
            l.open(format!("if {} {{", ne(i, &vu[i], &us[i])));
            fail_diag(&mut l, "valueOk", "`,\"valueAfterUpdate\":1`");
            l.close("}");
        }
        l.close("}");
        l.close("}");
    }
    // Both consumed bars [p0-1, svN-1]: the original reports what batch did,
    // the fork two bars more.
    l.open("if cloneOk && forked {");
    l.line("cloneChecked = 1");
    l.line("cloneLegs++");
    l.line("rangeChecked = 1");
    l.line("rangeLegs++");
    l.line(format!("rangeSites |= {}", sv_range_bit(SvRangeSite::Copy, SV_RANGE_MASK_GO)));
    l.open("if sa.OutRange() != br {");
    fail_diag(&mut l, "rangeOk", "`,\"copyRangeSrc\":1`");
    l.close("}");
    l.open("if r := sb.OutRange(); r.BegIdx != beg || r.NBElement != nb+2 {");
    fail_diag(&mut l, "rangeOk", "`,\"copyRange\":1`");
    l.close("}");
    l.close("}");
    l.close("}");
    l.close("}");

    // Short history: at lb bars no output is defined, so both openers owe
    // InsufficientHistory.
    l.open("if lb >= 1 && lb < svN {");
    l.line(format!("{} := c2.{name}Open({})", open_lhs("_", "err"), args(&[ins(":lb"), opts.clone()])));
    l.open("if err == nil {");
    fail_diag(&mut l, "allOk", "`,\"shortHistoryAccepted\":1`");
    l.mid("} else if !errors.Is(err, talib.InsufficientHistory) {");
    fail_diag(&mut l, "allOk", "`,\"shortHistoryWrongType\":1`");
    l.close("}");
    for (i, f) in fbufs.iter().enumerate() {
        l.line(format!("{f} := {}", canary(i)));
    }
    l.line(format!("_, _, err = c2.{name}OpenAndFill({})", args(&[ins(":lb"), opts.clone(), fbufs.clone()])));
    l.open("if err == nil {");
    fail_diag(&mut l, "allOk", "`,\"shortHistoryFillAccepted\":1`");
    l.mid("} else if !errors.Is(err, talib.InsufficientHistory) {");
    fail_diag(&mut l, "allOk", "`,\"shortHistoryFillWrongType\":1`");
    l.close("}");
    l.close("}");
    l.close("}");

    let mut tail = String::new();
    let _ = write!(
        tail,
        "return fmt.Sprintf({}, beg, nb, legs, fillChecked, b2i(fillOk), rangeChecked, rangeLegs, rangeSites, b2i(rangeOk), valueChecked, valueLegs, b2i(valueOk), cloneChecked, cloneLegs, b2i(cloneOk), b2i(allOk), b2i(allOk && fillOk && rangeOk && valueOk && cloneOk), b2i(peekAll), peekReps, b2i(peekRepAll), peekRejects, zsign, roundMask, diag)",
        raw(&format!(
            r#"{{"retCode":0,"beg":%d,"nb":%d,"legs":%d,"fill_checked":%d,"fill_ok":%d,"range_checked":%d,"range_legs":%d,"range_sites":%d,"range_sites_all":{SV_RANGE_MASK_GO},"range_ok":%d,"value_checked":%d,"value_legs":%d,"value_ok":%d,"clone_checked":%d,"clone_legs":%d,"clone_ok":%d,"step_ok":%d,"ok":%d,"peek_ok":%d,"peek_reps":%d,"peek_rep_ok":%d,"peek_rejects":%d,"benign":%d,"round_mask":%d%s}}"#
        ))
    );
    l.line(tail);
    l.close("}");
    l.s
}

/// The whole Go `stream_verify` section: compare helpers, the candle rounds,
/// one `sv<NAME>` per function with a Go stream, and the dispatcher, whose
/// unknown names (TA_STREAM_PROBE included) answer `not_streamable`.
pub(crate) fn generate_go_stream_verify(funcs: &[FuncDef], enums: &HashMap<String, EnumDef>) -> String {
    let mut s = String::from(SV_GO_SUPPORT);
    let lookup = crate::streaming::FuncsLookup(funcs);
    let emitted: Vec<&FuncDef> = funcs.iter().filter(|f| crate::backends::go_stream::emits_stream(f, &lookup)).collect();
    s.push_str("\nfunc (s *server) streamVerify(p params) string {\n\tswitch p.str(\"funcName\") {\n");
    for f in &emitted {
        let _ = write!(s, "\tcase \"TA_{}\":\n\t\treturn sv{}(s, p)\n", f.name.to_uppercase(), f.name);
    }
    s.push_str("\t}\n\treturn `{\"error\":\"not_streamable\"}`\n}\n");
    for f in &emitted {
        s.push('\n');
        s.push_str(&emit_go_sv_func(f, funcs, enums));
    }
    s
}

const SV_GO_SUPPORT: &str = r"
// ---- stream_verify: Go stream vs Go batch, bitwise ----

const svCanary = -1.2345678901234e300

const svCanaryI32 = -987654321

func b2i(b bool) int {
	if b {
		return 1
	}
	return 0
}

func svBne(a, b float64) bool {
	return math.Float64bits(a) != math.Float64bits(b)
}

// svXtierNe: differing bits that compare equal are +0.0 vs -0.0, counted and
// never a mismatch.
func svXtierNe(a, b float64, zsign *int64) bool {
	if !svBne(a, b) {
		return false
	}
	if a == b {
		*zsign++
		return false
	}
	return true
}

func svCanaryF(n int) []float64 {
	a := make([]float64, n)
	for i := range a {
		a[i] = svCanary
	}
	return a
}

func svCanaryI(n int) []int32 {
	a := make([]int32, n)
	for i := range a {
		a[i] = svCanaryI32
	}
	return a
}

// svCandleRound: defaults, avgPeriod+3, avgPeriod 0, rangeType Shadows, the
// other range type, as every server sweeps them.
func svCandleRound(c *talib.Core, rd int) {
	if rd == 0 {
		return
	}
	d := talib.NewCore()
	for t := talib.CandleSettingTypeBodyLong; t < talib.CandleSettingTypeAllCandleSettings; t++ {
		cs, _ := d.CandleSetting(t)
		switch rd {
		case 1:
			cs.AvgPeriod += 3
		case 2:
			cs.AvgPeriod = 0
		case 3:
			cs.RangeType = talib.RangeTypeShadows
		default:
			if cs.RangeType == talib.RangeTypeRealBody {
				cs.RangeType = talib.RangeTypeHighLow
			} else {
				cs.RangeType = talib.RangeTypeRealBody
			}
		}
		_ = c.SetCandleSetting(t, cs.RangeType, cs.AvgPeriod, cs.Factor)
	}
}
";
