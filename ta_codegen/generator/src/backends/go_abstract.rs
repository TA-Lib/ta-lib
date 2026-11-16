//! The Go introspection package `ta-lib.github.io/go/abstract`, rendered from
//! [`abstract_rows::rows`]. A package of its own, so a program importing only
//! `talib` links none of it.
//!
//! Every file here opens with [`super::go::GENERATED_HEADER`]; the stale sweep
//! removes only such files, so the hand-written test in the directory survives.

use std::collections::HashMap;
use std::fmt::Write as _;
use std::path::Path;

use super::abstract_rows::{
    func_flag_bits, opt_flag_bits, output_flag_bits, rows, FuncRow, Group, InputKind, OptDomain, OutputKind,
};
use crate::ir::{EnumDef, FuncDef, ParamType, PriceComponent};

/// The package's import path.
pub const IMPORT_PATH: &str = "ta-lib.github.io/go/abstract";

#[allow(clippy::implicit_hasher)]
pub fn generate(funcs: &[FuncDef], enums: &HashMap<String, EnumDef>, dir: &Path) {
    std::fs::create_dir_all(dir).unwrap_or_else(|e| panic!("creating {}: {e}", dir.display()));
    let files = render_all(funcs, enums);
    for (name, body) in &files {
        super::write_if_changed_silent(&dir.join(name), body);
    }
    let keep: Vec<&str> = files.iter().map(|(n, _)| *n).collect();
    clean_stale(dir, &keep);
    println!("  Go abstract package -> {} ({} functions)", dir.display(), funcs.len());
}

/// Every emitted file, by name.
#[allow(clippy::implicit_hasher)]
#[must_use]
pub fn render_all(funcs: &[FuncDef], enums: &HashMap<String, EnumDef>) -> Vec<(&'static str, String)> {
    let rows = rows(funcs, enums);
    let by_name: HashMap<&str, &FuncDef> = funcs.iter().map(|f| (f.name.as_str(), f)).collect();
    vec![
        ("doc.go", format!("{}\n\n{}{DOC}", super::go::GENERATED_HEADER, super::ta_abstract_c::LICENSE)),
        ("vocabulary.go", vocabulary()),
        ("model.go", head(&["\"strconv\"", "", "talib \"ta-lib.github.io/go\""]) + MODEL),
        ("paramholder.go", render_param_holder()),
        ("functions.go", functions(&rows, &by_name)),
        ("description.go", description(funcs)),
        ("runtimeinfo.go", runtime_info()),
    ]
}

fn clean_stale(dir: &Path, keep: &[&str]) {
    let Ok(entries) = std::fs::read_dir(dir) else {
        return;
    };
    for entry in entries.flatten() {
        let name = entry.file_name().to_string_lossy().to_string();
        let is_go = Path::new(&name).extension().is_some_and(|e| e.eq_ignore_ascii_case("go"));
        if !is_go || keep.contains(&name.as_str()) {
            continue;
        }
        let generated =
            std::fs::read_to_string(entry.path()).is_ok_and(|t| t.starts_with(super::go::GENERATED_HEADER));
        if generated {
            std::fs::remove_file(entry.path()).ok();
            println!("  removed stale Go abstract file {name}");
        }
    }
}

fn head(imports: &[&str]) -> String {
    let mut s = format!("{}\n\n{}package abstract\n", super::go::GENERATED_HEADER, super::ta_abstract_c::LICENSE);
    match imports {
        [] => {}
        [one] => {
            let _ = write!(s, "\nimport {one}\n");
        }
        many => {
            s.push_str("\nimport (\n");
            for i in many {
                if i.is_empty() {
                    s.push('\n');
                } else {
                    let _ = writeln!(s, "\t{i}");
                }
            }
            s.push_str(")\n");
        }
    }
    s
}

/// A Go interpreted string literal.
fn lit(s: &str) -> String {
    let mut o = String::from("\"");
    for c in s.chars() {
        match c {
            '"' => o.push_str("\\\""),
            '\\' => o.push_str("\\\\"),
            '\n' => o.push_str("\\n"),
            '\t' => o.push_str("\\t"),
            c if (c as u32) < 0x20 || c as u32 == 0x7f => {
                let _ = write!(o, "\\x{:02x}", c as u32);
            }
            c => o.push(c),
        }
    }
    o.push('"');
    o
}

/// The shortest round-tripping literal; Rust's `Debug` form is valid Go.
fn num(v: f64) -> String {
    format!("{v:?}")
}

// ---------------------------------------------------------------------------
// doc.go
// ---------------------------------------------------------------------------

const DOC: &str = r#"// Package abstract describes every TA-Lib function at run time and calls one
// chosen by name: the Go form of TA-Lib's Abstract API.
//
// [Lookup] finds a function by name, and [All] and [InGroup] enumerate them.
// A [FuncInfo] reports the function's group, flags, inputs, optional
// parameters with their domains and defaults, outputs and attributes.
// [FuncInfo.NewCall] returns a [ParamHolder]: bind the inputs, the parameters
// and the outputs, then call it.
//
//	f, _ := abstract.Lookup("SMA")
//	call := f.NewCall(talib.NewCore())
//	_ = call.SetInput(0, closes)
//	_ = call.SetOptInputInteger(0, 30)
//	_ = call.SetOutput(0, out)
//	r, err := call.Call(0, len(closes)-1)
//
// A call goes through the function's method on [talib.Core], so it returns
// the same values and the same errors as a direct call. Every error is a
// [talib.RetCode].
//
// The descriptions are immutable and safe to share between goroutines; a
// ParamHolder belongs to one goroutine.
//
// Documentation: https://ta-lib.org/api/abstract/
package abstract
"#;

// ---------------------------------------------------------------------------
// vocabulary.go
// ---------------------------------------------------------------------------

/// `(yaml_token, Go constant, doc sentence)` for every function flag.
const FUNC_FLAGS: &[(&str, &str, &str)] = &[
    ("overlap", "FuncOverlap", "the output is on the input's scale and overlays a price chart."),
    ("stream", "FuncStream", "a streaming (one bar at a time) API exists for the function."),
    ("volume", "FuncVolumeUsed", "the output is over the volume data."),
    ("unstable_period", "FuncUnstablePeriod", "the function owns an unstable-period id: see [FuncInfo.UnstableID]."),
    ("candlestick", "FuncCandlestick", "a candlestick pattern function: every integer output is a pattern output."),
    (
        "path_dependent",
        "FuncPathDependent",
        "the output depends on where the caller started, so it may never converge across ranges.",
    ),
    (
        "nan_inf_output",
        "FuncNanInfOutput",
        "some inputs have no finite result, so a successful call can return NaN or +/-Inf.",
    ),
    (
        "period1_identity",
        "FuncPeriod1Identity",
        "a period of 1 performs no smoothing: the output is a bit-exact copy of the input.",
    ),
    (
        "display_shift",
        "FuncDisplayShift",
        "at least one output carries [OutDisplayShift]; without it every output's display shift is 0.",
    ),
    (
        "uses_transcendental",
        "FuncUsesTranscendental",
        "the function calls a transcendental math function, so a value may differ slightly between \
         languages, between a stream and a batch call, and between machines. Not set on a function that \
         reaches one only when an MA-type parameter selects MAMA or ALMA.",
    ),
    (
        "positive_input",
        "FuncPositiveInput",
        "the function assumes its input is positive. A series that reaches or crosses zero, such as an \
         oscillator or a spread, gets a successful call and an output that may have no meaning: the data is \
         not checked.",
    ),
];

const OPT_FLAGS: &[(&str, &str, &str)] = &[
    ("percent", "OptIsPercent", "the value is a percentage."),
    ("degree", "OptIsDegree", "the value is in degrees."),
    ("currency", "OptIsCurrency", "the value is in a currency."),
    ("advanced", "OptAdvanced", "advanced: a basic user interface may hide it."),
];

const OUTPUT_FLAGS: &[(&str, &str, &str)] = &[
    ("line", "OutLine", "draw as a continuous line."),
    ("dot_line", "OutDotLine", "draw as a dotted line."),
    ("dash_line", "OutDashLine", "draw as a dashed line."),
    ("dot", "OutDot", "draw as unconnected dots."),
    ("histogram", "OutHistogram", "draw as a histogram."),
    ("pattern_bool", "OutPatternBool", "two states: 0 is no pattern, non-zero a pattern."),
    ("pattern_bull_bear", "OutPatternBullBear", "the sign is a call: positive bullish, negative bearish."),
    (
        "pattern_confirm",
        "OutPatternConfirm",
        "level 200 occurs too: this bar confirms the output's most recent earlier pattern.",
    ),
    ("positive", "OutPositive", "positive values occur."),
    ("negative", "OutNegative", "negative values occur."),
    (
        "zero",
        "OutZero",
        "zero occurs; on a pattern output, no pattern on this bar. An output setting any of the three sign \
         flags declares all its signs; one setting none declares nothing.",
    ),
    ("upper_limit", "OutUpperLimit", "an upper band or limit line."),
    ("lower_limit", "OutLowerLimit", "a lower band or limit line."),
    (
        "display_shift",
        "OutDisplayShift",
        "a chart draws it ahead of or behind the bar that computed it, by the bars \
         [ParamHolder.DisplayShift] reports. The values are never shifted.",
    ),
    ("pattern_weak", "OutPatternWeak", "level 80 occurs too: a weaker form of the pattern, on the same bar."),
    ("bar_index", "OutBarIndex", "the value is an absolute index into the input slices."),
    (
        "segmented",
        "OutSegmented",
        "a line whose segments restart wherever the function's integer output changes value: never connect \
         across a change.",
    ),
    (
        "pattern_color",
        "OutPatternColor",
        "the sign is the color of the bar the value is for: positive when close >= open, negative otherwise.",
    ),
];

const PRICE_COMPONENTS: &[(PriceComponent, &str, &str)] = &[
    (PriceComponent::Open, "PriceOpen", "the open price."),
    (PriceComponent::High, "PriceHigh", "the high price."),
    (PriceComponent::Low, "PriceLow", "the low price."),
    (PriceComponent::Close, "PriceClose", "the close price."),
    (PriceComponent::Volume, "PriceVolume", "the traded volume."),
    (PriceComponent::OpenInterest, "PriceOpenInterest", "the open interest."),
];

/// One `// Name is ...` line per member, wrapped as the package's docs are.
fn member_doc(s: &mut String, name: &str, sentence: &str) {
    let text = format!("{name}: {sentence}");
    let mut line = String::new();
    for w in text.split_whitespace() {
        if !line.is_empty() && line.len() + 1 + w.len() > 76 {
            let _ = writeln!(s, "\t// {line}");
            line.clear();
        }
        if !line.is_empty() {
            line.push(' ');
        }
        line.push_str(w);
    }
    let _ = writeln!(s, "\t// {line}");
}

fn flag_block(s: &mut String, ty: &str, doc: &str, members: &[(&str, u32, &str)]) {
    let _ = write!(s, "\n// {doc}\ntype {ty} uint32\n\n// The {ty} bits.\nconst (\n");
    for (name, bits, mdoc) in members {
        assert_ne!(*bits, 0, "go_abstract: {name} maps to no bit");
        member_doc(s, name, mdoc);
        let _ = writeln!(s, "\t{name} {ty} = 0x{bits:08X}");
    }
    s.push_str(")\n");
}

fn kind_block(s: &mut String, ty: &str, doc: &str, members: &[(&str, &str, &str)]) {
    let _ = write!(s, "\n// {doc}\ntype {ty} int32\n\n// The {ty} values.\nconst (\n");
    for (i, (name, _, mdoc)) in members.iter().enumerate() {
        member_doc(s, name, mdoc);
        let _ = writeln!(s, "\t{name} {ty} = {i}");
    }
    let _ = write!(s, ")\n\n// String returns the value's name, such as {}.\nfunc (k {ty}) String() string {{\n\tswitch k {{\n", lit(members[0].1));
    for (name, text, _) in members {
        let _ = write!(s, "\tcase {name}:\n\t\treturn {}\n", lit(text));
    }
    let _ = write!(s, "\t}}\n\treturn \"{ty}(\" + strconv.Itoa(int(k)) + \")\"\n}}\n");
}

fn vocabulary() -> String {
    let mut s = head(&["\"strconv\""]);
    s.push_str("\n// Group is a function's category.\ntype Group int32\n\n// The Group values.\nconst (\n");
    for g in Group::ALL {
        let _ = writeln!(s, "\t// Group{}: the {} group.", g.ident(), g.as_str());
        let _ = writeln!(s, "\tGroup{} Group = {}", g.ident(), *g as i32);
    }
    s.push_str(
        ")\n\n// String returns the group's display name, such as \"Overlap Studies\", spelled as C's\n\
         // TA_GroupString spells it.\nfunc (g Group) String() string {\n\tswitch g {\n",
    );
    for g in Group::ALL {
        let _ = write!(s, "\tcase Group{}:\n\t\treturn {}\n", g.ident(), lit(g.as_str()));
    }
    s.push_str("\t}\n\treturn \"Group(\" + strconv.Itoa(int(g)) + \")\"\n}\n\n");
    s.push_str("// Groups returns every group, in display-name order.\nfunc Groups() []Group {\n\treturn []Group{");
    let names: Vec<String> = Group::ALL.iter().map(|g| format!("Group{}", g.ident())).collect();
    s.push_str(&names.join(", "));
    s.push_str("}\n}\n");

    kind_block(
        &mut s,
        "InputKind",
        "InputKind says what an input carries. The values are C's TA_InputParameterType.",
        &[
            ("InputPrice", "Price", "a bundle of price components; [InputInfo.Components] says which."),
            ("InputReal", "Real", "one real series."),
            ("InputInteger", "Integer", "one integer series."),
        ],
    );
    kind_block(
        &mut s,
        "OptInputKind",
        "OptInputKind is the domain of an optional parameter. The values are C's TA_OptInputParameterType.",
        &[
            ("OptRealRange", "RealRange", "a real within a range: the domain is a [RealRange]."),
            ("OptRealList", "RealList", "a real from a fixed set: the domain is a [RealList]."),
            ("OptIntegerRange", "IntegerRange", "an integer within a range: the domain is an [IntegerRange]."),
            ("OptIntegerList", "IntegerList", "an integer from a fixed set: the domain is an [IntegerList]."),
        ],
    );
    kind_block(
        &mut s,
        "OutputKind",
        "OutputKind says what an output carries. The values are C's TA_OutputParameterType.",
        &[
            ("OutputReal", "Real", "a []float64 output."),
            ("OutputInteger", "Integer", "an []int32 output."),
        ],
    );

    let token = |bits: fn(&[String]) -> u32, t: &str| bits(&[t.to_string()]);
    let func: Vec<(&str, u32, &str)> = FUNC_FLAGS.iter().map(|(t, n, d)| (*n, token(func_flag_bits, t), *d)).collect();
    flag_block(&mut s, "FuncFlags", "FuncFlags are the bit flags of a function. The values are C's TA_FUNC_FLG_*.", &func);
    let price: Vec<(&str, u32, &str)> =
        PRICE_COMPONENTS.iter().map(|(c, n, d)| (*n, super::price_bundle::flags(&[*c]), *d)).collect();
    flag_block(
        &mut s,
        "PriceComponents",
        "PriceComponents are the components a price input consumes. The values are C's TA_IN_PRICE_*.",
        &price,
    );
    let opt: Vec<(&str, u32, &str)> = OPT_FLAGS.iter().map(|(t, n, d)| (*n, token(opt_flag_bits, t), *d)).collect();
    flag_block(
        &mut s,
        "OptInputFlags",
        "OptInputFlags are the display hints of an optional parameter. The values are C's TA_OPTIN_*.",
        &opt,
    );
    let out: Vec<(&str, u32, &str)> =
        OUTPUT_FLAGS.iter().map(|(t, n, d)| (*n, token(output_flag_bits, t), *d)).collect();
    flag_block(
        &mut s,
        "OutputFlags",
        "OutputFlags say how an output is drawn and which values it takes. The values are C's TA_OUT_*. A\n\
         // pattern output writes 0 or a sign times a level: the sign flags give its signs, the level flags\n\
         // its levels (100 always).",
        &out,
    );
    s
}

// ---------------------------------------------------------------------------
// model.go
// ---------------------------------------------------------------------------

const MODEL: &str = r#"
// FuncInfo is everything the library knows about one function's batch form.
// Obtain one from [Lookup], [All] or [InGroup]. Its accessors return copies, so
// nothing a caller does with them changes the description.
type FuncInfo struct {
	name         string
	group        Group
	hint         string
	flags        FuncFlags
	unst         talib.FuncUnstId
	inputs       []InputInfo
	optInputs    []OptInputInfo
	outputs      []OutputInfo
	attributes   string
	lookback     func(*talib.Core, *ParamHolder) (int, error)
	displayShift func(*talib.Core, *ParamHolder, int) (int, error)
	call         func(*talib.Core, *ParamHolder, int, int) (talib.OutRange, error)
}

// Name returns the canonical name, such as "SMA": also the name of the
// [talib.Core] method that computes the function.
func (f FuncInfo) Name() string { return f.name }

// Group returns the function's category.
func (f FuncInfo) Group() Group { return f.group }

// Hint returns a one-line description, or "" when there is none.
func (f FuncInfo) Hint() string { return f.hint }

// Flags returns the function's flags.
func (f FuncInfo) Flags() FuncFlags { return f.flags }

// HasFlags reports whether every bit of mask is set.
func (f FuncInfo) HasFlags(mask FuncFlags) bool { return f.flags&mask == mask }

// UnstableID returns the function's unstable-period id, and false when it has
// none. It has one exactly when [FuncUnstablePeriod] is set.
func (f FuncInfo) UnstableID() (talib.FuncUnstId, bool) {
	return f.unst, f.HasFlags(FuncUnstablePeriod)
}

// Inputs returns the inputs, in call order. The components of a price input
// are one input, so ADX has one input, not three: see
// [InputInfo.SignatureOrder].
func (f FuncInfo) Inputs() []InputInfo {
	out := make([]InputInfo, len(f.inputs))
	for i, in := range f.inputs {
		in.SignatureOrder = append([]PriceComponents(nil), in.SignatureOrder...)
		out[i] = in
	}
	return out
}

// OptInputs returns the optional parameters, in call order.
func (f FuncInfo) OptInputs() []OptInputInfo {
	out := make([]OptInputInfo, len(f.optInputs))
	for i, o := range f.optInputs {
		switch d := o.Domain.(type) {
		case IntegerList:
			d.Values = append([]NamedValue(nil), d.Values...)
			o.Domain = d
		case RealList:
			d.Values = append([]NamedRealValue(nil), d.Values...)
			o.Domain = d
		}
		out[i] = o
	}
	return out
}

// Outputs returns the outputs, in call order.
func (f FuncInfo) Outputs() []OutputInfo {
	return append([]OutputInfo(nil), f.outputs...)
}

// Attributes returns what no flag can carry, such as an output's reference
// levels, as one JSON object, or "{}" when there is none: the same bytes as C's
// TA_GetFuncAttributes. Ignore a key you do not know, since a release may add
// one: https://ta-lib.org/spec/abstract/#attributes.
func (f FuncInfo) Attributes() string { return f.attributes }

// String returns the function's name.
func (f FuncInfo) String() string { return f.name }

// NewCall returns a holder to bind the function's arguments into, computing
// with the unstable periods and candlestick settings of core; a nil core
// stands for [talib.NewCore].
func (f FuncInfo) NewCall(core *talib.Core) *ParamHolder {
	if core == nil {
		core = talib.NewCore()
	}
	h := &ParamHolder{
		info:     f,
		core:     core,
		reals:    make([][]float64, len(f.inputs)),
		ints:     make([][]int32, len(f.inputs)),
		prices:   make([][6][]float64, len(f.inputs)),
		intOpts:  make([]int, len(f.optInputs)),
		realOpts: make([]float64, len(f.optInputs)),
		realOuts: make([][]float64, len(f.outputs)),
		intOuts:  make([][]int32, len(f.outputs)),
	}
	for k := range f.optInputs {
		h.intOpts[k] = talib.IntegerDefault
		h.realOpts[k] = talib.RealDefault
	}
	return h
}

// InputInfo is one input of a function.
type InputInfo struct {
	// Kind says what the input carries.
	Kind InputKind
	// ParamName is the parameter's name, such as inReal or inPriceHLC.
	ParamName string
	// Components are, for a price input, the components it consumes, and 0
	// otherwise.
	Components PriceComponents
	// SignatureOrder lists, for a price input, its components in the order the
	// [talib.Core] method takes them as separate slices, and is empty
	// otherwise.
	SignatureOrder []PriceComponents
}

// Requires reports whether the input consumes every component of c.
func (i InputInfo) Requires(c PriceComponents) bool { return i.Components&c == c }

// OutputInfo is one output of a function.
type OutputInfo struct {
	// Kind says whether the output is real or integer.
	Kind OutputKind
	// ParamName is the parameter's name, such as outRealUpperBand.
	ParamName string
	// Flags are the drawing and value hints.
	Flags OutputFlags
}

// OptInputInfo is one optional parameter of a function.
type OptInputInfo struct {
	// Kind names the domain, and so the type Domain holds.
	Kind OptInputKind
	// ParamName is the parameter's name, such as optInTimePeriod.
	ParamName string
	// DisplayName is a short label for a user interface, such as "Time Period".
	DisplayName string
	// Hint is longer help text, or "" when there is none.
	Hint string
	// Flags are the display hints.
	Flags OptInputFlags
	// DefaultValue is the value an unset parameter takes, as C reports it in
	// defaultValue: the one [talib.IntegerDefault] or [talib.RealDefault]
	// selects.
	DefaultValue float64
	// Domain holds the values the parameter accepts: a [RealRange],
	// [RealList], [IntegerRange] or [IntegerList], as Kind says.
	Domain Domain
}

// Domain is the values an optional parameter accepts: a [RealRange],
// [RealList], [IntegerRange] or [IntegerList], and nothing else.
type Domain interface {
	domain()
}

// RealRange is a domain of reals within [Min, Max].
type RealRange struct {
	// Min is the smallest accepted value.
	Min float64
	// Max is the largest accepted value.
	Max float64
	// Precision is how many decimals a user interface offers.
	Precision int
	// Default is the documented default.
	Default float64
	// SuggestedStart is where a parameter sweep starts.
	SuggestedStart float64
	// SuggestedEnd is where a parameter sweep ends.
	SuggestedEnd float64
	// SuggestedIncrement is a parameter sweep's step.
	SuggestedIncrement float64
}

func (RealRange) domain() {}

// Sweep returns the suggested sweep values, low to high: Default alone when
// SuggestedIncrement is not positive, otherwise SuggestedStart plus each
// multiple of SuggestedIncrement that does not pass SuggestedEnd. Each value
// is computed from its index, so the step cannot drift.
func (d RealRange) Sweep() []float64 {
	if !(d.SuggestedIncrement > 0) {
		return []float64{d.Default}
	}
	// The slack absorbs representation error and no more: SAREXT's
	// 0.01..0.15 step 0.01 lands 2.8e-17 past its end, while half a step would
	// admit T3's 1.01, outside its own range.
	limit := d.SuggestedEnd + d.SuggestedIncrement*1e-9
	var out []float64
	for i := 0; ; i++ {
		v := d.SuggestedStart + float64(i)*d.SuggestedIncrement
		if v > limit {
			return out
		}
		out = append(out, v)
	}
}

// IntegerRange is a domain of integers within [Min, Max].
type IntegerRange struct {
	// Min is the smallest accepted value.
	Min int
	// Max is the largest accepted value.
	Max int
	// Default is the documented default.
	Default int
	// SuggestedStart is where a parameter sweep starts.
	SuggestedStart int
	// SuggestedEnd is where a parameter sweep ends.
	SuggestedEnd int
	// SuggestedIncrement is a parameter sweep's step.
	SuggestedIncrement int
}

func (IntegerRange) domain() {}

// Sweep returns the suggested sweep values, low to high: Default alone when
// SuggestedIncrement is not positive.
func (d IntegerRange) Sweep() []int {
	if d.SuggestedIncrement <= 0 {
		return []int{d.Default}
	}
	var out []int
	for v := d.SuggestedStart; v <= d.SuggestedEnd; v += d.SuggestedIncrement {
		out = append(out, v)
	}
	return out
}

// NamedValue is one choice of an [IntegerList].
type NamedValue struct {
	// Value is the value to pass.
	Value int
	// Name is its name, such as "EMA".
	Name string
}

// IntegerList is a domain of named integer choices, such as an MA type.
type IntegerList struct {
	// Values are the choices, in declaration order.
	Values []NamedValue
	// Default is the documented default: a value, not an index.
	Default int
}

func (IntegerList) domain() {}

// ValueList returns the list in C's form, "0=SMA;1=EMA;...".
func (d IntegerList) ValueList() string {
	s := ""
	for i, v := range d.Values {
		if i > 0 {
			s += ";"
		}
		s += strconv.Itoa(v.Value) + "=" + v.Name
	}
	return s
}

// NamedRealValue is one choice of a [RealList].
type NamedRealValue struct {
	// Value is the value to pass.
	Value float64
	// Name is its name.
	Name string
}

// RealList is a domain of named real choices. No function declares one; a
// switch over a [Domain] still has to account for it.
type RealList struct {
	// Values are the choices, in declaration order.
	Values []NamedRealValue
	// Default is the documented default.
	Default float64
}

func (RealList) domain() {}

// ValueList returns the list in C's form, "value=name;...".
func (d RealList) ValueList() string {
	s := ""
	for i, v := range d.Values {
		if i > 0 {
			s += ";"
		}
		s += strconv.FormatFloat(v.Value, 'g', -1, 64) + "=" + v.Name
	}
	return s
}
"#;

// ---------------------------------------------------------------------------
// paramholder.go
// ---------------------------------------------------------------------------

/// The fixed `paramholder.go`, which `metadata_price_setter_validates_before_writing`
/// reads.
#[must_use]
pub fn render_param_holder() -> String {
    head(&["\"math\"", "", "talib \"ta-lib.github.io/go\""]) + PARAM_HOLDER
}

const PARAM_HOLDER: &str = r"
// PriceInput holds the components of a price input. Leave nil the ones the
// input does not consume.
type PriceInput struct {
	// Open is the open prices.
	Open []float64
	// High is the high prices.
	High []float64
	// Low is the low prices.
	Low []float64
	// Close is the close prices.
	Close []float64
	// Volume is the volumes.
	Volume []float64
	// OpenInterest is the open interest.
	OpenInterest []float64
}

// ParamHolder binds the arguments of a function chosen at run time, then calls
// it: the counterpart of C's TA_ParamHolder. Obtain one from
// [FuncInfo.NewCall].
//
// Each setter checks its argument against the function's description before
// it writes, so a rejected call leaves the holder as it was. A rejection is
// [talib.BadParam] for an index out of range or a value missing or out of
// reach, and [talib.InvalidParamHolderType] for a slot of another kind. An
// optional parameter left unset takes its default.
//
// A ParamHolder belongs to one goroutine.
type ParamHolder struct {
	info     FuncInfo
	core     *talib.Core
	reals    [][]float64
	ints     [][]int32
	prices   [][6][]float64
	intOpts  []int
	realOpts []float64
	realOuts [][]float64
	intOuts  [][]int32
}

// Info returns the function the holder calls.
func (h *ParamHolder) Info() FuncInfo { return h.info }

// Core returns the settings the call computes with.
func (h *ParamHolder) Core() *talib.Core { return h.core }

func (h *ParamHolder) input(idx int, want InputKind) (InputInfo, error) {
	if idx < 0 || idx >= len(h.info.inputs) {
		return InputInfo{}, talib.BadParam
	}
	in := h.info.inputs[idx]
	if in.Kind != want {
		return in, talib.InvalidParamHolderType
	}
	return in, nil
}

// SetInput binds real input idx. The series is not copied.
func (h *ParamHolder) SetInput(idx int, series []float64) error {
	if _, err := h.input(idx, InputReal); err != nil {
		return err
	}
	if series == nil {
		return talib.BadParam
	}
	h.reals[idx] = series
	return nil
}

// SetIntInput binds integer input idx. The series is not copied.
func (h *ParamHolder) SetIntInput(idx int, series []int32) error {
	if _, err := h.input(idx, InputInteger); err != nil {
		return err
	}
	if series == nil {
		return talib.BadParam
	}
	h.ints[idx] = series
	return nil
}

// SetPriceInput binds price input idx. Every component the input consumes
// must be present; one it does not consume is accepted and ignored. The
// slices are not copied.
func (h *ParamHolder) SetPriceInput(idx int, p PriceInput) error {
	in, err := h.input(idx, InputPrice)
	if err != nil {
		return err
	}
	given := [6][]float64{p.Open, p.High, p.Low, p.Close, p.Volume, p.OpenInterest}
	for k, s := range given {
		if in.Components&(1<<k) != 0 && s == nil {
			return talib.BadParam
		}
	}
	h.prices[idx] = given
	return nil
}

// SetPriceInputComponent binds one component of price input idx; c names
// exactly one component, such as [PriceHigh]. A component the input does not
// consume is accepted and ignored. The series is not copied.
func (h *ParamHolder) SetPriceInputComponent(idx int, c PriceComponents, series []float64) error {
	if _, err := h.input(idx, InputPrice); err != nil {
		return err
	}
	k := 0
	for k < 6 && c != 1<<k {
		k++
	}
	if k == 6 || series == nil {
		return talib.BadParam
	}
	h.prices[idx][k] = series
	return nil
}

func (h *ParamHolder) opt(idx int) (OptInputInfo, error) {
	if idx < 0 || idx >= len(h.info.optInputs) {
		return OptInputInfo{}, talib.BadParam
	}
	return h.info.optInputs[idx], nil
}

// SetOptInputInteger binds optional parameter idx, of an integer domain. Its
// range is checked when the function runs; [talib.IntegerDefault] selects
// the default.
func (h *ParamHolder) SetOptInputInteger(idx, value int) error {
	o, err := h.opt(idx)
	if err != nil {
		return err
	}
	if o.Kind != OptIntegerRange && o.Kind != OptIntegerList {
		return talib.InvalidParamHolderType
	}
	if value < math.MinInt32 || value > math.MaxInt32 {
		return talib.BadParam
	}
	h.intOpts[idx] = value
	return nil
}

// SetOptInputReal binds optional parameter idx, of a real domain. Its range
// is checked when the function runs; [talib.RealDefault] selects the
// default.
func (h *ParamHolder) SetOptInputReal(idx int, value float64) error {
	o, err := h.opt(idx)
	if err != nil {
		return err
	}
	if o.Kind != OptRealRange && o.Kind != OptRealList {
		return talib.InvalidParamHolderType
	}
	h.realOpts[idx] = value
	return nil
}

// SetOptInputMAType binds optional parameter idx, a moving-average type. A
// parameter of another domain, a period included, is
// [talib.InvalidParamHolderType].
func (h *ParamHolder) SetOptInputMAType(idx int, value talib.MAType) error {
	o, err := h.opt(idx)
	if err != nil {
		return err
	}
	if o.Kind != OptIntegerList {
		return talib.InvalidParamHolderType
	}
	h.intOpts[idx] = int(value)
	return nil
}

// SetOptInputByName binds the optional parameter named name, such as
// optInTimePeriod, converting value to its domain: an integer domain takes an
// integral value within the int32 range, so [talib.IntegerDefault] selects the
// default there. A name the function does not declare is [talib.BadParam].
func (h *ParamHolder) SetOptInputByName(name string, value float64) error {
	for idx, o := range h.info.optInputs {
		if o.ParamName != name {
			continue
		}
		if o.Kind == OptRealRange || o.Kind == OptRealList {
			return h.SetOptInputReal(idx, value)
		}
		if value != math.Trunc(value) || value < math.MinInt32 || value > math.MaxInt32 {
			return talib.BadParam
		}
		return h.SetOptInputInteger(idx, int(value))
	}
	return talib.BadParam
}

func (h *ParamHolder) output(idx int, want OutputKind) error {
	if idx < 0 || idx >= len(h.info.outputs) {
		return talib.BadParam
	}
	if h.info.outputs[idx].Kind != want {
		return talib.InvalidParamHolderType
	}
	return nil
}

// SetOutput binds real output idx: the slice the call writes it into.
func (h *ParamHolder) SetOutput(idx int, out []float64) error {
	if err := h.output(idx, OutputReal); err != nil {
		return err
	}
	if out == nil {
		return talib.BadParam
	}
	h.realOuts[idx] = out
	return nil
}

// SetIntOutput binds integer output idx: the slice the call writes it into.
func (h *ParamHolder) SetIntOutput(idx int, out []int32) error {
	if err := h.output(idx, OutputInteger); err != nil {
		return err
	}
	if out == nil {
		return talib.BadParam
	}
	h.intOuts[idx] = out
	return nil
}

// Lookback returns the function's lookback for the optional parameters bound
// so far, through its Lookback method on [talib.Core], and that method's
// error. Inputs and outputs need not be bound.
func (h *ParamHolder) Lookback() (int, error) {
	if h.info.lookback == nil {
		return 0, talib.InvalidParamHolder
	}
	return h.info.lookback(h.core, h)
}

// DisplayShift returns how many bars ahead (positive) or behind (negative) of
// the bar that computed it a chart draws output outputIdx, for the optional
// parameters bound so far, through the function's DisplayShift method on
// [talib.Core], and that method's error: [talib.BadParam] for a parameter
// out of range or an index that names no output. It is 0 for an output
// without [OutDisplayShift]. Inputs and outputs need not be bound.
func (h *ParamHolder) DisplayShift(outputIdx int) (int, error) {
	if h.info.displayShift == nil {
		return 0, talib.InvalidParamHolder
	}
	return h.info.displayShift(h.core, h, outputIdx)
}

// Call runs the function over bars startIdx through endIdx through its method
// on [talib.Core], which returns exactly what a direct call does. An unbound
// input is [talib.InputNotAllInitialize] and an unbound output
// [talib.OutputNotAllInitialize], and the function does not run.
func (h *ParamHolder) Call(startIdx, endIdx int) (talib.OutRange, error) {
	if h.info.call == nil {
		return talib.OutRange{}, talib.InvalidParamHolder
	}
	for i, in := range h.info.inputs {
		bound := true
		switch in.Kind {
		case InputReal:
			bound = h.reals[i] != nil
		case InputInteger:
			bound = h.ints[i] != nil
		default:
			for k := 0; k < 6; k++ {
				if in.Components&(1<<k) != 0 && h.prices[i][k] == nil {
					bound = false
				}
			}
		}
		if !bound {
			return talib.OutRange{}, talib.InputNotAllInitialize
		}
	}
	for i, o := range h.info.outputs {
		if o.Kind == OutputReal && h.realOuts[i] == nil || o.Kind == OutputInteger && h.intOuts[i] == nil {
			return talib.OutRange{}, talib.OutputNotAllInitialize
		}
	}
	return h.info.call(h.core, h, startIdx, endIdx)
}
";

// ---------------------------------------------------------------------------
// functions.go
// ---------------------------------------------------------------------------

fn functions(rows: &[FuncRow], by_name: &HashMap<&str, &FuncDef>) -> String {
    let mut s = head(&["\"sort\"", "", "talib \"ta-lib.github.io/go\""]);
    s.push_str(
        r#"
// All returns every function, in name order.
func All() []FuncInfo {
	return append([]FuncInfo(nil), registry[:]...)
}

// Lookup returns the function named name in any ASCII casing, such as "SMA" or
// "sma", and false when there is none.
func Lookup(name string) (FuncInfo, bool) {
	upper := []byte(name)
	for i, b := range upper {
		if 'a' <= b && b <= 'z' {
			upper[i] = b - 'a' + 'A'
		}
	}
	key := string(upper)
	i := sort.Search(len(registry), func(i int) bool { return registry[i].name >= key })
	if i < len(registry) && registry[i].name == key {
		return registry[i], true
	}
	return FuncInfo{}, false
}

// InGroup returns the functions of group g, in name order.
func InGroup(g Group) []FuncInfo {
	var out []FuncInfo
	for _, f := range registry {
		if f.group == g {
			out = append(out, f)
		}
	}
	return out
}

var registry = [...]FuncInfo{
"#,
    );
    for r in rows {
        emit_row(&mut s, r);
    }
    s.push_str("}\n");
    for r in rows {
        emit_thunks(&mut s, r, by_name[r.name.as_str()]);
    }
    s
}

fn emit_row(s: &mut String, r: &FuncRow) {
    let unst = r.unst.as_ref().map_or_else(|| "-1".to_string(), |u| format!("talib.FuncUnstId{}", u.name));
    let _ = writeln!(
        s,
        "\t{{{}, Group{}, {}, 0x{:08X}, {unst},",
        lit(&r.name),
        r.group.ident(),
        lit(&r.hint),
        r.flags
    );
    let inputs: Vec<String> = r
        .inputs
        .iter()
        .map(|i| {
            let kind = match i.kind {
                InputKind::Price => "InputPrice",
                InputKind::Real => "InputReal",
                InputKind::Integer => "InputInteger",
            };
            let order = if i.signature_components.is_empty() {
                "nil".to_string()
            } else {
                let c: Vec<&str> = i
                    .signature_components
                    .iter()
                    .map(|c| PRICE_COMPONENTS.iter().find(|(p, _, _)| p == c).expect("component").1)
                    .collect();
                format!("[]PriceComponents{{{}}}", c.join(", "))
            };
            format!("{{{kind}, {}, 0x{:08X}, {order}}}", lit(&i.param_name), i.flags)
        })
        .collect();
    let opts: Vec<String> = r
        .opt_inputs
        .iter()
        .map(|o| {
            let (kind, domain) = match &o.domain {
                OptDomain::RealRange { min, max, precision, default, suggested } => (
                    "OptRealRange",
                    format!(
                        "RealRange{{{}, {}, {precision}, {}, {}, {}, {}}}",
                        num(*min),
                        num(*max),
                        num(*default),
                        num(suggested.0),
                        num(suggested.1),
                        num(suggested.2)
                    ),
                ),
                OptDomain::IntegerRange { min, max, default, suggested } => (
                    "OptIntegerRange",
                    format!("IntegerRange{{{min}, {max}, {default}, {}, {}, {}}}", suggested.0, suggested.1, suggested.2),
                ),
                OptDomain::IntegerList { values, default } => {
                    let v: Vec<String> = values.iter().map(|(v, n)| format!("{{{v}, {}}}", lit(n))).collect();
                    ("OptIntegerList", format!("IntegerList{{[]NamedValue{{{}}}, {default}}}", v.join(", ")))
                }
                OptDomain::RealList { values, default } => {
                    let v: Vec<String> = values.iter().map(|(v, n)| format!("{{{}, {}}}", num(*v), lit(n))).collect();
                    ("OptRealList", format!("RealList{{[]NamedRealValue{{{}}}, {}}}", v.join(", "), num(*default)))
                }
            };
            format!(
                "{{{kind}, {}, {}, {}, 0x{:08X}, {}, {domain}}}",
                lit(&o.param_name),
                lit(&o.display_name),
                lit(&o.hint),
                o.flags,
                num(o.domain.default_as_f64())
            )
        })
        .collect();
    let outputs: Vec<String> = r
        .outputs
        .iter()
        .map(|o| {
            let kind = match o.kind {
                OutputKind::Real => "OutputReal",
                OutputKind::Integer => "OutputInteger",
            };
            format!("{{{kind}, {}, 0x{:08X}}}", lit(&o.param_name), o.flags)
        })
        .collect();
    for (ty, items) in [("InputInfo", &inputs), ("OptInputInfo", &opts), ("OutputInfo", &outputs)] {
        if items.is_empty() {
            s.push_str("\t\tnil,\n");
        } else {
            let _ = writeln!(s, "\t\t[]{ty}{{");
            for it in items {
                let _ = writeln!(s, "\t\t\t{it},");
            }
            s.push_str("\t\t},\n");
        }
    }
    let n = &r.name;
    let _ = writeln!(s, "\t\t{}, lookback{n}, displayShift{n}, call{n}}},", lit(&r.attributes));
}

/// The three thunks of one function, each calling its public method.
fn emit_thunks(s: &mut String, r: &FuncRow, def: &FuncDef) {
    let names = super::go::go_names(&r.name);
    let opts: Vec<String> = def
        .optional_inputs
        .iter()
        .enumerate()
        .map(|(k, o)| match &o.param_type {
            ParamType::Real => format!("h.realOpts[{k}]"),
            ParamType::Enum(e) => format!("talib.{e}(h.intOpts[{k}])"),
            _ => format!("h.intOpts[{k}]"),
        })
        .collect();
    let mut ds = opts.clone();
    ds.push("outputIdx".into());
    let mut call: Vec<String> = vec!["startIdx".into(), "endIdx".into()];
    for (slot, inp) in r.inputs.iter().enumerate() {
        match inp.kind {
            InputKind::Price => {
                for c in &inp.signature_components {
                    call.push(format!("h.prices[{slot}][{}]", *c as usize));
                }
            }
            InputKind::Real => call.push(format!("h.reals[{slot}]")),
            InputKind::Integer => call.push(format!("h.ints[{slot}]")),
        }
    }
    call.extend(opts.iter().cloned());
    for (k, out) in r.outputs.iter().enumerate() {
        call.push(match out.kind {
            OutputKind::Real => format!("h.realOuts[{k}]"),
            OutputKind::Integer => format!("h.intOuts[{k}]"),
        });
    }
    let n = &r.name;
    let _ = write!(
        s,
        "\nfunc lookback{n}(c *talib.Core, h *ParamHolder) (int, error) {{\n\treturn c.{}({})\n}}\n",
        names.lookback_public,
        opts.join(", ")
    );
    let _ = write!(
        s,
        "\nfunc displayShift{n}(c *talib.Core, h *ParamHolder, outputIdx int) (int, error) {{\n\treturn c.{}({})\n}}\n",
        names.display_shift_public,
        ds.join(", ")
    );
    let _ = write!(
        s,
        "\nfunc call{n}(c *talib.Core, h *ParamHolder, startIdx, endIdx int) (talib.OutRange, error) {{\n\treturn c.{}({})\n}}\n",
        names.public,
        call.join(", ")
    );
}

// ---------------------------------------------------------------------------
// description.go, runtimeinfo.go
// ---------------------------------------------------------------------------

/// The real XML as one raw string, so the server measures shipped bytes.
fn description(funcs: &[FuncDef]) -> String {
    let xml = super::func_api_xml::generate_string(funcs);
    assert!(
        !xml.contains('`') && !xml.contains('\r'),
        "ta_func_api.xml holds a backquote or a carriage return, which a Go raw string cannot carry"
    );
    let mut s = head(&[]);
    s.push_str(
        "\n// FunctionDescriptionXML returns the machine-readable description of every\n\
         // function: the XML document C's TA_FunctionDescriptionXML returns, byte for\n\
         // byte.\n\
         func FunctionDescriptionXML() string {\n\treturn functionDescriptionXML\n}\n\n\
         const functionDescriptionXML = `",
    );
    s.push_str(&xml);
    s.push_str("`\n");
    s
}

fn runtime_info() -> String {
    let mut s = head(&["talib \"ta-lib.github.io/go\""]);
    s.push_str(&super::runtime_info::go_fn());
    s
}
