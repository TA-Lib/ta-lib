use std::path::Path;

use serde::Deserialize;

use crate::ir::{FuncDef, Input, OptInput, Output, ParamType, PriceComponent, PriceRef};

#[derive(Deserialize)]
struct YamlFunc {
    name: String,
    group: String,
    description: Option<String>,
    camel_case: Option<String>,
    hint: Option<String>,
    #[serde(default)]
    flags: YamlFlags,
    inputs: Vec<YamlInput>,
    optional_inputs: Option<Vec<YamlOptParam>>,
    outputs: Vec<YamlOutput>,
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
struct YamlInput {
    name: String,
    #[serde(rename = "type")]
    param_type: String,
    /// For price inputs: which OHLCV components are used.
    price_components: Option<Vec<String>>,
}

#[derive(Deserialize)]
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
struct YamlOutput {
    name: String,
    #[serde(rename = "type")]
    param_type: String,
    #[serde(default)]
    flags: YamlFlags,
}

fn yaml_val_to_f64(v: &serde_yaml::Value) -> f64 {
    match v {
        serde_yaml::Value::Number(n) => n.as_f64().unwrap_or(0.0),
        serde_yaml::Value::String(s) => match s.as_str() {
            "TA_INTEGER_MIN" | "TA_REAL_MIN" => f64::MIN,
            "TA_INTEGER_MAX" | "TA_REAL_MAX" => f64::MAX,
            "TA_INTEGER_DEFAULT" => 0.0,
            other => panic!("Unknown range constant: {other}"),
        },
        other => panic!("Unexpected YAML range value: {other:?}"),
    }
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

pub fn parse_yaml(path: &Path) -> FuncDef {
    let content = std::fs::read_to_string(path)
        .unwrap_or_else(|e| panic!("Failed to read {}: {}", path.display(), e));

    let yaml: YamlFunc = serde_yaml::from_str(&content)
        .unwrap_or_else(|e| panic!("Failed to parse {}: {}", path.display(), e));

    // A `type: price` input is one `ta_abstract` descriptor but several function
    // arguments, so it expands here. Each expanded argument keeps a `PriceRef` back to
    // the bundle it came from: the fold is ABI (see `ir::PriceComponent`), so
    // `backends::price_bundle` must be able to rebuild it from data rather than by
    // re-recognising component names.
    let mut next_price_group = 0usize;
    let inputs: Vec<Input> = yaml
        .inputs
        .into_iter()
        .flat_map(|p| {
            let pt = parse_param_type(&p.param_type, p.price_components);
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
                                name: component.input_name().to_string(),
                                param_type: ParamType::Real,
                                price: Some(PriceRef { group, component }),
                            }
                        })
                        .collect::<Vec<_>>()
                }
                other => vec![Input::new(p.name.clone(), other)],
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

    let outputs = yaml
        .outputs
        .into_iter()
        .map(|p| Output {
            name: p.name,
            param_type: parse_param_type(&p.param_type, None),
            flags: p.flags.into_vec(),
        })
        .collect();

    let flags = yaml.flags.into_vec();
    // `stream` in the flags list opts the function into the generated
    // streaming API (it maps to TA_FUNC_FLG_STREAM like every other flag).
    let streaming = flags.iter().any(|f| f == "stream");

    FuncDef {
        name: yaml.name,
        group: yaml.group,
        description: yaml.description,
        camel_case: yaml.camel_case,
        hint: yaml.hint,
        flags,
        inputs,
        optional_inputs: opt_inputs,
        outputs,
        lookback: None,
        body: vec![],
        private_body: vec![],
        private_extra_params: vec![],
        private_param_init: vec![],
        has_explicit_private: false,
        header_comments: vec![],
        doc: None,
        streaming,
    }
}
