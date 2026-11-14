use crate::ir::{FuncDef, ParamType};
use crate::server_gen::expand_input_names;

/// The Go ride-along: Rust's, in Go's idiom. A separate module reaches only the
/// public streaming tier, and `Core`'s settings are unexported, so the dedup key
/// folds the request's own `unstablePeriod` and a generation bumped by every
/// ambient mutator.
pub(crate) fn generate_go_ridealong(funcs: &[FuncDef]) -> String {
    let mut s = RIDE_GO_SUPPORT.to_string();
    for f in funcs.iter().filter(|f| f.streaming) {
        s.push('\n');
        s.push_str(&emit_go_ridealong_fn(f));
    }
    s
}

const RIDE_GO_SUPPORT: &str = r#"
// ---- ride-along: batch-vs-stream on caller-supplied data ----

const rideMaxBars = 4096

const rideSeenN = 2048

type rideEntry struct {
	key  uint64
	open int
	fill int
}

var rideSeen [rideSeenN]rideEntry

var rideGen uint64

func rideGate(p params) bool {
	return p.i64("timed", 0) == 0 && p.i64("iters", 1) <= 1 && p.i64("no_output", 0) == 0 && p.i64("bench_mode", 0) == 0 && p.i64("use_float", 0) == 0
}

func rideFinite(a []float64) bool {
	for _, v := range a {
		if math.IsNaN(v) || math.IsInf(v, 0) {
			return false
		}
	}
	return true
}

func rideMixU64(h, v uint64) uint64 {
	for i := 0; i < 8; i++ {
		h ^= v >> (8 * i) & 0xff
		h *= 1099511628211
	}
	return h
}

func rideMixStr(h uint64, s string) uint64 {
	for i := 0; i < len(s); i++ {
		h ^= uint64(s[i])
		h *= 1099511628211
	}
	return h
}

func rideMixF64s(h uint64, a []float64) uint64 {
	for _, v := range a {
		h = rideMixU64(h, math.Float64bits(v))
	}
	return h
}

// rideCode is the code of a call's trailing error: 0 for none.
func rideCode(results ...any) int {
	if err, _ := results[len(results)-1].(error); err != nil {
		return retCodeOf(err)
	}
	return 0
}

type rideResult struct {
	ok       bool
	skip     int
	dedup    int
	openBars int
	fillBars int
	benign   int64
	m        int
	lb       int
	leg      int
	bar      int
	out      int
	batch    uint64
	stream   uint64
	rej      int
	rcBatch  int
	rcOpen   int
	rcFill   int
}

func newRide() *rideResult {
	return &rideResult{ok: true, lb: -1, bar: -1, out: -1}
}

func (r *rideResult) emit() string {
	s := fmt.Sprintf(`,"ride_ok":%d,"ride_skip":%d,"ride_dedup":%d,"ride_open_bars":%d,"ride_fill_bars":%d,"ride_benign":%d,"ride_m":%d,"ride_lb":%d,"ride_rej":%d,"ride_rc_batch":%d,"ride_rc_open":%d,"ride_rc_fill":%d`,
		b2i(r.ok), r.skip, r.dedup, r.openBars, r.fillBars, r.benign, r.m, r.lb, r.rej, r.rcBatch, r.rcOpen, r.rcFill)
	if !r.ok {
		s += fmt.Sprintf(`,"ride_leg":%d,"ride_bar":%d,"ride_out":%d,"ride_batch":"%016x","ride_stream":"%016x"`, r.leg, r.bar, r.out, r.batch, r.stream)
	}
	return s
}
"#;

/// Go's line builder for the generated server: tab indentation, one statement
/// per line, as gofmt prints it.
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
    /// `if <cond> {` with each of `body` on its own line.
    fn guard(&mut self, cond: impl AsRef<str>, body: &[&str]) {
        self.open(format!("if {} {{", cond.as_ref()));
        for b in body {
            self.line(b);
        }
        self.close("}");
    }
}

/// The Go type a public parameter takes.
fn opt_go_type(t: &ParamType) -> &'static str {
    match t {
        ParamType::Real => "float64",
        ParamType::Enum(_) => "talib.MAType",
        _ => "int",
    }
}

#[allow(clippy::too_many_lines)]
pub(crate) fn emit_go_ridealong_fn(func: &FuncDef) -> String {
    let name = &func.name;
    let input_names = expand_input_names(&func.inputs);
    let outs = &func.outputs;
    let nouts = outs.len();
    let mut l = L { s: String::new(), depth: 0 };

    let sig_ins: Vec<String> = input_names.iter().map(|n| format!("{n} []float64")).collect();
    let sig_opts: Vec<String> = func.optional_inputs.iter().map(|o| format!("{} {}", o.name, opt_go_type(&o.param_type))).collect();
    let opt_args: Vec<String> = func.optional_inputs.iter().map(|o| o.name.clone()).collect();
    let slice = |r: &str| -> Vec<String> { input_names.iter().map(|n| format!("{n}[{r}]")).collect() };
    let join = |parts: &[Vec<String>]| -> String { parts.iter().flatten().cloned().collect::<Vec<_>>().join(", ") };
    let upd_args: Vec<String> = input_names.iter().map(|n| format!("{n}[t]")).collect();

    // Batch reference buffers, and the same buffers again for the fill leg.
    let mut slot_of: Vec<(bool, usize)> = Vec::new();
    let (mut ref_bufs, mut fill_bufs) = (Vec::new(), Vec::new());
    {
        let (mut r, mut i) = (0usize, 0usize);
        for out in outs {
            if out.param_type == ParamType::Integer {
                ref_bufs.push((format!("rib{i}"), "[]int32"));
                fill_bufs.push((format!("fib{i}"), "[]int32"));
                slot_of.push((true, i));
                i += 1;
            } else {
                ref_bufs.push((format!("rb{r}"), "[]float64"));
                fill_bufs.push((format!("fb{r}"), "[]float64"));
                slot_of.push((false, r));
                r += 1;
            }
        }
    }
    let ref_args: Vec<String> = ref_bufs.iter().map(|(n, _)| n.clone()).collect();
    let fill_args: Vec<String> = fill_bufs.iter().map(|(n, _)| n.clone()).collect();
    let u: Vec<String> = (0..nouts).map(|k| format!("u{k}")).collect();

    let cmp_bar = |l: &mut L, idx: &str, leg: i32, bar: &str| {
        l.line("cmp := true");
        for (k, (is_int, slot)) in slot_of.iter().enumerate() {
            if *is_int {
                l.open(format!("if cmp && u{k} != rib{slot}[{idx}] {{"));
                l.line("cmp = false");
                l.line(format!("r.out = {k}"));
                l.line(format!("r.batch = math.Float64bits(float64(rib{slot}[{idx}]))"));
                l.line(format!("r.stream = math.Float64bits(float64(u{k}))"));
            } else {
                l.open(format!("if cmp && svXtierNe(rb{slot}[{idx}], u{k}, &r.benign) {{"));
                l.line("cmp = false");
                l.line(format!("r.out = {k}"));
                l.line(format!("r.batch = math.Float64bits(rb{slot}[{idx}])"));
                l.line(format!("r.stream = math.Float64bits(u{k})"));
            }
            l.close("}");
        }
        l.guard("cmp", &["r.openBars++"]);
        l.guard("!cmp", &["r.ok = false", &format!("r.leg = {leg}"), &format!("r.bar = {bar}")]);
    };

    let mut sig = vec!["core *talib.Core".to_string(), "p params".to_string(), "endIdx int".to_string()];
    sig.extend(sig_ins);
    sig.extend(sig_opts);
    l.open(format!("func ride{name}({}) string {{", sig.join(", ")));
    l.guard("!rideGate(p)", &["return \"\""]);
    l.line("r := newRide()");
    // A negative lookback is a REJECTED parameter and travels on: it is the
    // only rejection the ride can reach, and the reject leg below reads it.
    l.line(format!("lb, lbErr := core.{name}Lookback({})", opt_args.join(", ")));
    l.guard("lbErr == nil", &["r.lb = lb"]);
    l.line("navail := endIdx + 1");
    for n in &input_names {
        l.guard(format!("len({n}) < navail"), &[&format!("navail = len({n})")]);
    }
    // The success path shortens the replay to keep the ride cheap; the reject
    // path has nothing to shorten and takes the caller's own range.
    l.line("m := navail");
    l.guard("lbErr == nil", &["m = 2*lb + 10"]);
    l.guard("m > navail", &["m = navail"]);
    l.line("r.m = m");
    l.guard("m > rideMaxBars", &["r.skip = 1", "return r.emit()"]);
    l.guard("m < 1", &["r.skip = 2", "return r.emit()"]);
    l.guard("lbErr == nil && m < lb+2", &["r.skip = 3", "return r.emit()"]);
    let finite: Vec<String> = input_names.iter().map(|n| format!("!rideFinite({n}[:m])")).collect();
    l.guard(finite.join(" || "), &["r.skip = 4", "return r.emit()"]);

    l.line("key := fuzzHashInit()");
    l.line(format!("key = rideMixStr(key, \"TA_{}\")", name.to_uppercase()));
    l.line("key = rideMixU64(key, uint64(m))");
    l.line("key = rideMixU64(key, rideGen)");
    l.line("key = rideMixU64(key, uint64(p.i64(\"unstablePeriod\", 0)))");
    for o in &func.optional_inputs {
        match &o.param_type {
            ParamType::Real => l.line(format!("key = rideMixU64(key, math.Float64bits({}))", o.name)),
            _ => l.line(format!("key = rideMixU64(key, uint64({}))", o.name)),
        }
    }
    for n in &input_names {
        l.line(format!("key = rideMixF64s(key, {n}[:m])"));
    }
    l.line("key = fuzzHashFin(key)");
    l.line("slot := key % rideSeenN");
    l.guard(
        "e := rideSeen[slot]; e.key == key",
        &["r.dedup = 1", "r.openBars = e.open", "r.fillBars = e.fill", "return r.emit()"],
    );

    for (n, t) in &ref_bufs {
        l.line(format!("{n} := make({t}, m)"));
    }
    l.line(format!("br, err := core.{name}(0, m-1, {})", join(&[slice(":m"), opt_args.clone(), ref_args])));
    // The reject leg. A rejection is a property of the CALL, so the three entry
    // points owe the same answer on it, over the same range.
    l.open("if err != nil {");
    l.line("r.rcBatch = retCodeOf(err)");
    l.line(format!("r.rcOpen = rideCode(core.{name}Open({}))", join(&[slice(":m"), opt_args.clone()])));
    for (n, t) in &fill_bufs {
        l.line(format!("{n} := make({t}, m)"));
    }
    l.line(format!("r.rcFill = rideCode(core.{name}OpenAndFill({}))", join(&[slice(":m"), opt_args.clone(), fill_args.clone()])));
    // `r.rej` is the comparison's own verdict, exactly like the bar counters:
    // a count bumped beside a compare keeps climbing once the compare is gone.
    l.line("cmp := r.rcOpen == r.rcBatch");
    l.guard("cmp", &["r.rej++"]);
    l.guard("!cmp", &["r.ok = false", "r.leg = 3"]);
    l.line("cmp = r.rcFill == r.rcBatch");
    l.guard("cmp", &["r.rej++"]);
    l.guard("!cmp", &["r.ok = false", "r.leg = 3"]);
    l.line("return r.emit()");
    l.close("}");
    // Lookback said no and the batch tier said yes: the replay has no anchor.
    l.guard("lbErr != nil", &["r.skip = 7", "return r.emit()"]);
    l.line("beg, nb := br.BegIdx, br.NBElement");
    l.guard("nb == 0", &["r.skip = 5", "return r.emit()"]);
    l.guard("beg != lb", &["r.skip = 6", "return r.emit()"]);

    l.line(format!("st, {}, err := core.{name}Open({})", u.join(", "), join(&[slice(":lb+1"), opt_args.clone()])));
    l.open("if err != nil {");
    l.line("r.ok = false");
    l.line("r.leg = 1");
    l.line("r.bar = lb");
    l.mid("} else {");
    cmp_bar(&mut l, "lb-beg", 1, "lb");
    l.open("for t := lb + 1; t < m; t++ {");
    l.line(format!("{}, err := st.Update({})", u.join(", "), upd_args.join(", ")));
    l.guard("err != nil", &["r.ok = false", "r.leg = 1", "r.bar = t", "break"]);
    cmp_bar(&mut l, "t-beg", 1, "t");
    l.guard("!r.ok", &["break"]);
    l.close("}");
    l.close("}");

    l.open("if r.ok {");
    for (n, t) in &fill_bufs {
        l.line(format!("{n} := make({t}, m)"));
    }
    l.line(format!("_, rng, err := core.{name}OpenAndFill({})", join(&[slice(":m"), opt_args, fill_args])));
    l.open("if err != nil {");
    l.line("r.ok = false");
    l.line("r.leg = 2");
    l.mid("} else if rng.BegIdx != beg || rng.NBElement != nb {");
    l.line("r.ok = false");
    l.line("r.leg = 2");
    l.mid("} else {");
    l.open("for k := 0; k < nb; k++ {");
    l.line("cmp := true");
    for (k, (is_int, slot)) in slot_of.iter().enumerate() {
        if *is_int {
            l.open(format!("if cmp && fib{slot}[k] != rib{slot}[k] {{"));
            l.line("cmp = false");
            l.line(format!("r.out = {k}"));
            l.line(format!("r.batch = math.Float64bits(float64(rib{slot}[k]))"));
            l.line(format!("r.stream = math.Float64bits(float64(fib{slot}[k]))"));
        } else {
            l.open(format!("if cmp && svXtierNe(rb{slot}[k], fb{slot}[k], &r.benign) {{"));
            l.line("cmp = false");
            l.line(format!("r.out = {k}"));
            l.line(format!("r.batch = math.Float64bits(rb{slot}[k])"));
            l.line(format!("r.stream = math.Float64bits(fb{slot}[k])"));
        }
        l.close("}");
    }
    l.guard("cmp", &["r.fillBars++"]);
    l.guard("!cmp", &["r.ok = false", "r.leg = 2", "r.bar = beg + k", "break"]);
    l.close("}");
    l.close("}");
    l.close("}");

    l.guard("r.ok", &["rideSeen[slot] = rideEntry{key, r.openBars, r.fillBars}"]);
    l.line("return r.emit()");
    l.close("}");
    l.s
}
