use std::path::Path;

use serde::Deserialize;

use crate::ir::{FuncDef, Input, OptInput, Output, ParamType, PriceComponent, PriceRef, Series};

#[derive(Deserialize)]
#[serde(deny_unknown_fields)]
struct YamlFunc {
    name: String,
    group: String,
    description: Option<String>,
    hint: Option<String>,
    #[serde(default)]
    flags: YamlFlags,
    inputs: Vec<YamlInput>,
    optional_inputs: Option<Vec<YamlOptParam>>,
    outputs: Vec<YamlOutput>,
    /// See `ir::FuncDef::c_block_batch_min_bars`.
    c_block_batch_min_bars: Option<u32>,
}

/// Flags can be a single string or a list of strings.
#[derive(Deserialize, Default, Clone)]
#[serde(untagged)]
enum YamlFlags {
    #[default]
    None,
    Single(String),
    List(Vec<String>),
}

impl YamlFlags {
    fn into_vec(self) -> Vec<String> {
        match self {
            YamlFlags::None => vec![],
            YamlFlags::Single(s) => vec![s],
            YamlFlags::List(v) => v,
        }
    }
}

#[derive(Deserialize)]
#[serde(deny_unknown_fields)]
struct YamlInput {
    name: String,
    #[serde(rename = "type")]
    param_type: String,
    /// For price inputs: which OHLCV components are used.
    price_components: Option<Vec<String>>,
    /// For `real` inputs: see `ir::Input::series` and `ir::Input::domain`.
    series: Option<String>,
    domain: Option<Vec<f64>>,
}

#[derive(Deserialize)]
#[serde(deny_unknown_fields)]
struct YamlOptParam {
    name: String,
    #[serde(rename = "type")]
    param_type: String,
    range: Option<Vec<serde_yaml::Value>>,
    default: Option<f64>,
    display_name: Option<String>,
    hint: Option<String>,
    #[serde(default)]
    flags: YamlFlags,
    /// Optimization hints: [`suggested_start`, `suggested_end`, `suggested_increment`]
    suggested: Option<Vec<f64>>,
    /// Number of decimal digits for UI display (only for real ranges).
    precision: Option<i32>,
}

#[derive(Deserialize)]
#[serde(deny_unknown_fields)]
struct YamlOutput {
    name: String,
    #[serde(rename = "type")]
    param_type: String,
    #[serde(default)]
    flags: YamlFlags,
    levels: Option<Vec<f64>>,
}

fn yaml_val_to_f64(v: &serde_yaml::Value) -> f64 {
    match v {
        serde_yaml::Value::Number(n) => n.as_f64().unwrap_or(0.0),
        // The IR carries TA-Lib's own sentinel values, not f64::MIN/MAX: the
        // emitted range checks print whatever is here, and ta_defs.h defines
        // these bounds as ±3e37 (see backends::common).
        serde_yaml::Value::String(s) => match s.as_str() {
            "TA_REAL_MIN" => crate::backends::common::TA_REAL_MIN,
            "TA_REAL_MAX" => crate::backends::common::TA_REAL_MAX,
            "TA_REAL_DEFAULT" => crate::backends::common::TA_REAL_DEFAULT,
            "TA_INTEGER_MIN" => f64::from(crate::backends::common::TA_INTEGER_MIN),
            "TA_INTEGER_MAX" => f64::from(crate::backends::common::TA_INTEGER_MAX),
            "TA_INTEGER_DEFAULT" => f64::from(crate::backends::common::TA_INTEGER_DEFAULT),
            other => panic!("Unknown range constant: {other}"),
        },
        other => panic!("Unexpected YAML range value: {other:?}"),
    }
}

/// An input's type and, checked against it, its `series` and `domain: [lo, hi]`
/// (a `real` input's only; a synthetic series and a domain come together).
fn parse_input_type(p: &mut YamlInput, path: &Path) -> (ParamType, Series, Option<(f64, f64)>) {
    let pt = parse_param_type(&p.param_type, p.price_components.take());
    let series = p.series.take().map_or(Series::Price, |s| {
        assert!(pt == ParamType::Real, "{}: input `{}`: `series` needs a `real` input", path.display(), p.name);
        Series::from_token(&s).unwrap_or_else(|| {
            panic!("{}: input `{}`: `series` is `price`, `ramp` or `sine`, not {s:?}", path.display(), p.name)
        })
    });
    let domain = p.domain.take().map(|d| {
        assert!(
            pt == ParamType::Real,
            "{}: input `{}`: `domain` needs a `real` input",
            path.display(),
            p.name
        );
        assert!(
            d.len() == 2 && d[0] < d[1],
            "{}: input `{}`: `domain` is `[lo, hi]` with lo < hi, not {d:?}",
            path.display(),
            p.name
        );
        (d[0], d[1])
    });
    assert!(
        (series == Series::Price) == domain.is_none(),
        "{}: input `{}`: `series: ramp` or `series: sine` and `domain: [lo, hi]` go together",
        path.display(),
        p.name
    );
    (pt, series, domain)
}

fn parse_output(p: YamlOutput, path: &Path) -> Output {
    let param_type = parse_param_type(&p.param_type, None);
    let levels = p.levels.map_or_else(Vec::new, |v| {
        assert!(
            param_type == ParamType::Real,
            "{}: output `{}`: `levels` needs a `real` output",
            path.display(),
            p.name
        );
        if let Err(e) = crate::attributes::key("levels").ty.check(&v) {
            panic!("{}: output `{}`: `levels` is {e}", path.display(), p.name);
        }
        v
    });
    Output { name: p.name, param_type, flags: p.flags.into_vec(), levels }
}

fn parse_param_type(s: &str, price_components: Option<Vec<String>>) -> ParamType {
    match s {
        "real" => ParamType::Real,
        "integer" => ParamType::Integer,
        "price" => {
            let components = price_components.unwrap_or_else(|| {
                vec!["open".into(), "high".into(), "low".into(), "close".into()]
            });
            ParamType::Price(components)
        }
        other if other.starts_with("enum:") => ParamType::Enum(other["enum:".len()..].to_string()),
        other => panic!("Unknown param type: {other}"),
    }
}

/// True for non-indicator subdirectories of `ta_codegen/input/` (shared
/// `helpers/`, library scaffolding `lib/`) that never carry a `<name>.yaml`
/// indicator definition and therefore contribute nothing to generated output.
///
/// Lives here, next to [`parse_yaml`], because every walker of the input tree
/// has to agree on it: one that skips a directory the other parses is how a
/// non-indicator directory reaches a check written for indicators.
pub fn is_reserved_dir(name: &str) -> bool {
    matches!(name, "helpers" | "lib")
}

/// One source of truth: the function-level bit says "some output carries the
/// output-level one", so it is derived and never written.
fn derive_display_shift_flag(name: &str, outputs: &[Output], flags: &mut Vec<String>) {
    let flag = crate::ir::DISPLAY_SHIFT_FLAG;
    assert!(
        !flags.iter().any(|f| f == flag),
        "{name}: `display_shift` is an output flag; the function flag is derived from it"
    );
    if outputs.iter().any(|o| o.flags.iter().any(|f| f == flag)) {
        flags.push(flag.to_string());
    }
}

/// The pattern-flag rules of `docs/ta_codegen_input_yaml.md`, and an unknown
/// flag key in any list. Every violation of the function, one line each.
pub fn check_flags(func: &FuncDef) -> Result<(), Vec<String>> {
    use crate::backends::func_api_xml::{FUNC_FLAGS, OPT_INPUT_FLAGS, OUTPUT_FLAGS};
    use crate::ir::PatternSign;
    let known = |table: &[(&str, &str)], f: &str| table.iter().any(|(k, _)| *k == f);
    let mut errs = Vec::new();
    let name = &func.name;
    for f in func.flags.iter().filter(|f| !known(FUNC_FLAGS, f)) {
        errs.push(format!("{name}: unknown function flag `{f}`"));
    }
    for o in &func.optional_inputs {
        for f in o.flags.iter().filter(|f| !known(OPT_INPUT_FLAGS, f)) {
            errs.push(format!("{name}: optional input `{}`: unknown flag `{f}`", o.name));
        }
    }
    let candlestick = func.is_candlestick();
    for o in &func.outputs {
        let out = &o.name;
        let has = |f: &str| o.flags.iter().any(|x| x == f);
        for f in o.flags.iter().filter(|f| !known(OUTPUT_FLAGS, f)) {
            errs.push(format!("{name}: output `{out}`: unknown output flag `{f}`"));
        }
        let integer = o.param_type == ParamType::Integer;
        if !integer && o.flags.iter().any(|f| f.starts_with("pattern_")) {
            errs.push(format!("{name}: output `{out}`: a pattern flag needs an integer output"));
        }
        let kinds = ["pattern_bool", "pattern_bull_bear", "pattern_color"].iter().filter(|k| has(k)).count();
        if kinds > 1 {
            errs.push(format!(
                "{name}: output `{out}`: `pattern_bool`, `pattern_bull_bear` and `pattern_color` exclude each other"
            ));
        }
        if has("pattern_bool") && (has("negative") || !has("zero") || !has("positive")) {
            errs.push(format!("{name}: output `{out}`: `pattern_bool` takes the sign flags `zero, positive`"));
        }
        for kind in ["pattern_bull_bear", "pattern_color"] {
            if has(kind) && !(has("zero") && (has("positive") || has("negative"))) {
                errs.push(format!(
                    "{name}: output `{out}`: `{kind}` needs `zero` and at least one of `positive` / `negative`"
                ));
            }
        }
        let sign = o.pattern_sign();
        if sign.is_some() {
            for style in ["line", "dot_line", "dash_line", "dot", "histogram"].iter().filter(|s| has(s)) {
                errs.push(format!("{name}: output `{out}`: a pattern output takes no drawing style such as `{style}`"));
            }
        }
        if has("pattern_weak") && !matches!(sign, Some(PatternSign::Call | PatternSign::Color)) {
            errs.push(format!("{name}: output `{out}`: `pattern_weak` needs `pattern_bull_bear` or `pattern_color`"));
        }
        if has("pattern_confirm") && !has("pattern_bull_bear") {
            errs.push(format!("{name}: output `{out}`: `pattern_confirm` needs `pattern_bull_bear`"));
        }
        if has("bar_index") && !integer {
            errs.push(format!("{name}: output `{out}`: `bar_index` needs an integer output"));
        }
        if has("segmented") {
            let n_int = func.outputs.iter().filter(|x| x.param_type == ParamType::Integer).count();
            if integer || n_int != 1 {
                errs.push(format!(
                    "{name}: output `{out}`: `segmented` needs a real output in a function with exactly one integer output"
                ));
            }
        }
        if candlestick && integer && sign.is_none() {
            errs.push(format!(
                "{name}: output `{out}`: an integer output of a candlestick function is a pattern output: \
                 `pattern_bool`, `pattern_bull_bear` or `pattern_color`"
            ));
        }
    }
    if errs.is_empty() { Ok(()) } else { Err(errs) }
}

pub fn parse_yaml(path: &Path) -> FuncDef {
    let content = std::fs::read_to_string(path)
        .unwrap_or_else(|e| panic!("Failed to read {}: {}", path.display(), e));
    parse_yaml_str(&content, path)
}

/// `path` names the file for messages and the directory check; nothing is read
/// from it.
pub fn parse_yaml_str(content: &str, path: &Path) -> FuncDef {
    let yaml: YamlFunc = serde_yaml::from_str(content)
        .unwrap_or_else(|e| panic!("Failed to parse {}: {}", path.display(), e));

    // The directory IS the name, lower-cased. Several derivations lean on that —
    // the Rust module path and file name, the registry's dir-keyed lookups, and
    // the `<dir>/<dir>.yaml` convention that located this file. Unchecked, a
    // mismatch does not fail here — it surfaces far away as a module that does
    // not resolve, or a cross-indicator call to a name no backend defines. At
    // parse time so every loader inherits it, rather than in whichever one
    // happens to run first.
    if let Some(dir) = path.parent().and_then(std::path::Path::file_name) {
        let dir = dir.to_string_lossy();
        assert!(
            dir == yaml.name.to_lowercase(),
            "{}: directory `{dir}` does not match `name: {}` — the directory must be the \
             name lower-cased (`{}`)",
            path.display(),
            yaml.name,
            yaml.name.to_lowercase()
        );
    }

    // A `type: price` input is one `ta_abstract` descriptor but several function
    // arguments, so it expands here. Each expanded argument keeps a `PriceRef` back to
    // the bundle it came from: the fold is ABI (see `ir::PriceComponent`), so
    // `backends::price_bundle` must be able to rebuild it from data rather than by
    // re-recognising component names.
    let mut next_price_group = 0usize;
    let inputs: Vec<Input> = yaml
        .inputs
        .into_iter()
        .flat_map(|mut p| {
            let (pt, series, domain) = parse_input_type(&mut p, path);
            match pt {
                ParamType::Price(components) => {
                    let group = next_price_group;
                    next_price_group += 1;
                    components
                        .into_iter()
                        .map(|c| {
                            let component =
                                PriceComponent::from_token(&c).unwrap_or_else(|| {
                                    panic!(
                                        "{}: unknown price component {c:?} on input {:?}",
                                        path.display(),
                                        p.name
                                    )
                                });
                            Input {
                                price: Some(PriceRef { group, component }),
                                ..Input::new(component.input_name(), ParamType::Real)
                            }
                        })
                        .collect::<Vec<_>>()
                }
                other => vec![Input { name: p.name.clone(), param_type: other, price: None, series, domain }],
            }
        })
        .collect();

    let opt_inputs = yaml
        .optional_inputs
        .unwrap_or_default()
        .into_iter()
        .map(|p| OptInput {
            name: p.name,
            param_type: parse_param_type(&p.param_type, None),
            range: p
                .range
                .and_then(|r| if r.len() >= 2 { Some((yaml_val_to_f64(&r[0]), yaml_val_to_f64(&r[1]))) } else { None }),
            default: p.default,
            display_name: p.display_name,
            hint: p.hint,
            flags: p.flags.into_vec(),
            suggested: p.suggested.and_then(|s| if s.len() >= 3 { Some((s[0], s[1], s[2])) } else { None }),
            precision: p.precision,
        })
        .collect();

    let outputs: Vec<Output> = yaml
        .outputs
        .into_iter()
        .map(|p| parse_output(p, path))
        .collect();

    let mut flags = yaml.flags.into_vec();
    derive_display_shift_flag(&yaml.name, &outputs, &mut flags);
    // `stream` in the flags list opts the function into the generated
    // streaming API (it maps to TA_FUNC_FLG_STREAM like every other flag).
    let streaming = flags.iter().any(|f| f == "stream");

    checked(FuncDef {
        name: yaml.name,
        group: yaml.group,
        description: yaml.description,
        hint: yaml.hint,
        flags,
        inputs,
        optional_inputs: opt_inputs,
        outputs,
        lookback: None,
        display_shift: None,
        body: vec![],
        private_body: vec![],
        private_extra_params: vec![],
        private_param_init: vec![],
        has_explicit_private: false,
        opt_outs: vec![],
        header_comments: vec![],
        doc: None,
        streaming,
        alternates: vec![],
        resolved_stream_body: None,
        c_block_batch_min_bars: yaml.c_block_batch_min_bars,
    }, path)
}

fn checked(func: FuncDef, path: &Path) -> FuncDef {
    if let Err(errs) = check_flags(&func) {
        panic!("{}:\n  {}", path.display(), errs.join("\n  "));
    }
    func
}
