use std::collections::HashMap;

use regex::Regex;

use super::{InputDef, OptInputDef, OutputDef, TableFuncDef};

// ---------------------------------------------------------------------------
// Shared definitions parsed from ta_def_ui.c
// ---------------------------------------------------------------------------

#[derive(Debug, Clone)]
pub struct IntegerRange {
    pub min: f64,
    pub max: f64,
    pub suggested_start: f64,
    pub suggested_end: f64,
    pub suggested_increment: f64,
}

#[derive(Debug, Clone)]
pub struct RealRange {
    pub min: f64,
    pub max: f64,
    pub precision: i32,
    pub suggested_start: f64,
    pub suggested_end: f64,
    pub suggested_increment: f64,
}

#[derive(Debug, Clone)]
pub enum RangeKind {
    Integer(IntegerRange),
    Real(RealRange),
}

#[derive(Debug, Clone)]
pub struct SharedInputDef {
    pub input_type: String,
    pub name: String,
    pub price_flags: Vec<String>,
}

#[derive(Debug, Clone)]
pub struct SharedOutputDef {
    pub output_type: String,
    pub name: String,
    pub flags: Vec<String>,
}

#[derive(Debug, Clone)]
pub struct SharedOptInputDef {
    pub opt_type: String, // "IntegerRange", "RealRange", "IntegerList"
    pub name: String,
    pub flags_raw: String,
    pub display_name: String,
    pub dataset_ref: String,
    pub default_value: f64,
    pub hint: String,
}

#[derive(Debug, Clone, Default)]
pub struct SharedDefs {
    pub group_ids: HashMap<String, String>,
    pub inputs: HashMap<String, SharedInputDef>,
    pub outputs: HashMap<String, SharedOutputDef>,
    pub integer_ranges: HashMap<String, IntegerRange>,
    pub real_ranges: HashMap<String, RealRange>,
    pub opt_inputs: HashMap<String, SharedOptInputDef>,
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

/// Strip C block comments like `/* min */` from a string.
fn strip_c_comments(s: &str) -> String {
    let re = Regex::new(r"(?s)/\*.*?\*/").unwrap();
    re.replace_all(s, "").to_string()
}

/// Parse a numeric literal, handling `TA_REAL_MIN` / `TA_REAL_MAX` sentinels.
fn parse_num(s: &str) -> f64 {
    let s = s.trim();
    if s == "TA_REAL_MIN" {
        return f64::MIN;
    }
    if s == "TA_REAL_MAX" {
        return f64::MAX;
    }
    s.parse::<f64>().unwrap_or(0.0)
}

/// Map a C group-id variable name to its string value.
fn resolve_group(name: &str, shared: &SharedDefs) -> String {
    shared
        .group_ids
        .get(name)
        .cloned()
        .unwrap_or_else(|| name.to_string())
}

/// Map C function flags to our string representation.
fn parse_func_flags(raw: &str) -> Vec<String> {
    let raw = strip_c_comments(raw);
    let raw = raw.trim();
    if raw == "0" || raw.is_empty() {
        return vec![];
    }
    let mut out = Vec::new();
    for part in raw.split('|') {
        let part = part.trim();
        match part {
            "TA_FUNC_FLG_OVERLAP" => out.push("overlap".to_string()),
            "TA_FUNC_FLG_VOLUME" => out.push("volume".to_string()),
            "TA_FUNC_FLG_CANDLESTICK" => out.push("candlestick".to_string()),
            "TA_FUNC_FLG_UNST_PER" => out.push("unstable_period".to_string()),
            _ => {}
        }
    }
    out
}

/// Map C output flags to our string representation.
fn parse_output_flags(raw: &str) -> Vec<String> {
    let raw = strip_c_comments(raw);
    let raw = raw.trim();
    if raw == "0" || raw.is_empty() {
        return vec![];
    }
    let mut out = Vec::new();
    for part in raw.split('|') {
        let part = part.trim();
        match part {
            "TA_OUT_LINE" => out.push("line".to_string()),
            "TA_OUT_DASH_LINE" => out.push("dash_line".to_string()),
            "TA_OUT_DOT_LINE" => out.push("dot_line".to_string()),
            "TA_OUT_DOT" => out.push("dot".to_string()),
            "TA_OUT_HISTO" => out.push("histogram".to_string()),
            "TA_OUT_PATTERN_BOOL" => out.push("pattern_bool".to_string()),
            "TA_OUT_PATTERN_BULL_BEAR" => out.push("pattern_bull_bear".to_string()),
            "TA_OUT_PATTERN_STRENGTH" => out.push("pattern_strength".to_string()),
            "TA_OUT_POSITIVE" => out.push("positive".to_string()),
            "TA_OUT_NEGATIVE" => out.push("negative".to_string()),
            "TA_OUT_ZERO" => out.push("zero".to_string()),
            "TA_OUT_UPPER_LIMIT" => out.push("upper_limit".to_string()),
            "TA_OUT_LOWER_LIMIT" => out.push("lower_limit".to_string()),
            _ => {}
        }
    }
    out
}

/// Map `TA_IN_PRICE`_* flags to lowercase price component names.
fn parse_price_flags(raw: &str) -> Vec<String> {
    let raw = strip_c_comments(raw);
    let raw = raw.trim();
    if raw == "0" || raw.is_empty() {
        return vec![];
    }
    let mut out = Vec::new();
    for part in raw.split('|') {
        let part = part.trim();
        match part {
            "TA_IN_PRICE_OPEN" => out.push("open".to_string()),
            "TA_IN_PRICE_HIGH" => out.push("high".to_string()),
            "TA_IN_PRICE_LOW" => out.push("low".to_string()),
            "TA_IN_PRICE_CLOSE" => out.push("close".to_string()),
            "TA_IN_PRICE_VOLUME" => out.push("volume".to_string()),
            "TA_IN_PRICE_OPENINTEREST" => out.push("open_interest".to_string()),
            _ => {}
        }
    }
    out
}

/// Map opt-input flags to string representations.
fn parse_opt_input_flags(raw: &str) -> Vec<String> {
    let raw = strip_c_comments(raw);
    let raw = raw.trim();
    if raw == "0" || raw.is_empty() {
        return vec![];
    }
    let mut out = Vec::new();
    for part in raw.split('|') {
        let part = part.trim();
        match part {
            "TA_OPTIN_IS_PERCENT" => out.push("percent".to_string()),
            "TA_OPTIN_IS_DEGREE" => out.push("degree".to_string()),
            "TA_OPTIN_IS_CURRENCY" => out.push("currency".to_string()),
            _ => {}
        }
    }
    out
}

// ---------------------------------------------------------------------------
// parse_shared_defs — parse ta_def_ui.c
// ---------------------------------------------------------------------------

pub fn parse_shared_defs(source: &str) -> SharedDefs {
    let mut defs = SharedDefs::default();

    parse_group_ids(source, &mut defs);
    parse_input_defs(source, &mut defs);
    parse_output_defs(source, &mut defs);
    parse_integer_ranges(source, &mut defs);
    parse_real_ranges(source, &mut defs);
    parse_opt_input_defs(source, &mut defs);

    defs
}

fn parse_group_ids(source: &str, defs: &mut SharedDefs) {
    // Match: const char TA_GroupId_XxxString[] = "Value";
    let re =
        Regex::new(r#"const\s+char\s+(TA_GroupId_\w+String)\s*\[\s*\]\s*=\s*"([^"]+)""#).unwrap();
    for cap in re.captures_iter(source) {
        let var_name = cap[1].to_string();
        let value = cap[2].to_string();
        // Also store without "String" suffix as the groupId reference
        let id_name = var_name.replace("String", "");
        defs.group_ids.insert(id_name, value.clone());
        defs.group_ids.insert(var_name, value);
    }
}

fn parse_input_defs(source: &str, defs: &mut SharedDefs) {
    // Match: const TA_InputParameterInfo TA_DEF_UI_Input_XXX = { type, "name", flags };
    // Multi-line, so we use a pattern that captures the whole initializer block.
    let re =
        Regex::new(r"(?s)const\s+TA_InputParameterInfo\s+(TA_DEF_UI_Input_\w+)\s*=\s*\{([^}]+)\}")
            .unwrap();

    for cap in re.captures_iter(source) {
        let var_name = cap[1].to_string();
        let body = strip_c_comments(&cap[2]);
        let parts: Vec<&str> = body.split(',').collect();
        if parts.len() >= 3 {
            let input_type_raw = parts[0].trim();
            let name = parts[1].trim().trim_matches('"').to_string();
            let flags_raw: String = parts[2..]
                .iter()
                .map(|s| s.trim())
                .collect::<Vec<_>>()
                .join("|");

            let input_type = match input_type_raw {
                "TA_Input_Real" => "real",
                "TA_Input_Integer" => "integer",
                "TA_Input_Price" => "price",
                _ => "unknown",
            }
            .to_string();

            let price_flags = if input_type == "price" {
                parse_price_flags(&flags_raw)
            } else {
                vec![]
            };

            defs.inputs.insert(
                var_name,
                SharedInputDef {
                    input_type,
                    name,
                    price_flags,
                },
            );
        }
    }
}

fn parse_output_defs(source: &str, defs: &mut SharedDefs) {
    let re = Regex::new(
        r"(?s)const\s+TA_OutputParameterInfo\s+(TA_DEF_UI_Output_\w+)\s*=\s*\{([^}]+)\}",
    )
    .unwrap();

    for cap in re.captures_iter(source) {
        let var_name = cap[1].to_string();
        let body = strip_c_comments(&cap[2]);
        let parts: Vec<&str> = body.split(',').collect();
        if parts.len() >= 3 {
            let output_type_raw = parts[0].trim();
            let name = parts[1].trim().trim_matches('"').to_string();
            let flags_raw = parts[2..]
                .iter()
                .map(|s| s.trim())
                .collect::<Vec<_>>()
                .join("|");

            let output_type = match output_type_raw {
                "TA_Output_Real" => "real",
                "TA_Output_Integer" => "integer",
                _ => "unknown",
            }
            .to_string();

            let flags = parse_output_flags(&flags_raw);

            defs.outputs.insert(
                var_name,
                SharedOutputDef {
                    output_type,
                    name,
                    flags,
                },
            );
        }
    }
}

fn parse_integer_ranges(source: &str, defs: &mut SharedDefs) {
    // Match: const TA_IntegerRange NAME = { val, val, val, val, val };
    let re = Regex::new(r"(?s)const\s+TA_IntegerRange\s+(TA_DEF_\w+)\s*=\s*\{([^}]+)\}").unwrap();

    for cap in re.captures_iter(source) {
        let var_name = cap[1].to_string();
        let body = strip_c_comments(&cap[2]);
        let vals: Vec<f64> = body.split(',').map(|s| parse_num(s.trim())).collect();
        if vals.len() >= 5 {
            defs.integer_ranges.insert(
                var_name,
                IntegerRange {
                    min: vals[0],
                    max: vals[1],
                    suggested_start: vals[2],
                    suggested_end: vals[3],
                    suggested_increment: vals[4],
                },
            );
        }
    }
}

fn parse_real_ranges(source: &str, defs: &mut SharedDefs) {
    let re = Regex::new(r"(?s)const\s+TA_RealRange\s+(TA_DEF_\w+)\s*=\s*\{([^}]+)\}").unwrap();

    for cap in re.captures_iter(source) {
        let var_name = cap[1].to_string();
        let body = strip_c_comments(&cap[2]);
        let vals: Vec<f64> = body.split(',').map(|s| parse_num(s.trim())).collect();
        if vals.len() >= 6 {
            defs.real_ranges.insert(
                var_name,
                RealRange {
                    min: vals[0],
                    max: vals[1],
                    #[allow(clippy::cast_possible_truncation)]
                    precision: vals[2] as i32,
                    suggested_start: vals[3],
                    suggested_end: vals[4],
                    suggested_increment: vals[5],
                },
            );
        }
    }
}

fn parse_opt_input_defs(source: &str, defs: &mut SharedDefs) {
    // Match both `const` and `static const` opt input parameter info blocks.
    let re = Regex::new(
        r"(?s)(?:static\s+)?const\s+TA_OptInputParameterInfo\s+(TA_DEF_UI_\w+)\s*=\s*\{([^}]+)\}",
    )
    .unwrap();

    for cap in re.captures_iter(source) {
        let var_name = cap[1].to_string();
        let body = &cap[2];
        if let Some(opt) = parse_opt_input_body(body) {
            defs.opt_inputs.insert(var_name, opt);
        }
    }
}

fn parse_opt_input_body(body: &str) -> Option<SharedOptInputDef> {
    let body = strip_c_comments(body);
    // Split on commas but be careful with nested parens in cast expressions.
    let parts = split_opt_input_fields(&body);
    if parts.len() < 7 {
        return None;
    }

    let opt_type_raw = parts[0].trim();
    let name = parts[1].trim().trim_matches('"').to_string();
    let flags_raw = parts[2].trim().to_string();
    let display_name = parts[3].trim().trim_matches('"').to_string();
    // parts[4] is the dataset pointer like `(const void *)&TA_DEF_TimePeriod_Positive_Minimum2`
    let dataset_ref = extract_dataset_ref(parts[4].trim());
    let default_value = parse_num(parts[5].trim());
    let hint = parts[6].trim().trim_matches('"').to_string();

    let opt_type = match opt_type_raw {
        "TA_OptInput_IntegerRange" => "IntegerRange",
        "TA_OptInput_RealRange" => "RealRange",
        "TA_OptInput_IntegerList" => "IntegerList",
        _ => "Unknown",
    }
    .to_string();

    Some(SharedOptInputDef {
        opt_type,
        name,
        flags_raw,
        display_name,
        dataset_ref,
        default_value,
        hint,
    })
}

/// Split the opt-input initializer body on commas, skipping commas inside parens.
fn split_opt_input_fields(s: &str) -> Vec<String> {
    let mut parts = Vec::new();
    let mut depth = 0;
    let mut current = String::new();
    for ch in s.chars() {
        match ch {
            '(' => {
                depth += 1;
                current.push(ch);
            }
            ')' => {
                depth -= 1;
                current.push(ch);
            }
            ',' if depth == 0 => {
                parts.push(current.trim().to_string());
                current.clear();
            }
            _ => current.push(ch),
        }
    }
    let last = current.trim().to_string();
    if !last.is_empty() {
        parts.push(last);
    }
    parts
}

/// Extract the referenced variable name from a dataset pointer expression.
/// e.g. `(const void *)&TA_DEF_TimePeriod_Positive_Minimum2` -> `TA_DEF_TimePeriod_Positive_Minimum2`
fn extract_dataset_ref(s: &str) -> String {
    // Find the & and extract the identifier after it
    if let Some(pos) = s.find('&') {
        let rest = &s[pos + 1..];
        let ident: String = rest
            .chars()
            .take_while(|c| c.is_alphanumeric() || *c == '_')
            .collect();
        return ident;
    }
    s.to_string()
}

// ---------------------------------------------------------------------------
// Resolve an OptInputDef from a SharedOptInputDef + shared ranges
// ---------------------------------------------------------------------------

fn resolve_opt_input(opt: &SharedOptInputDef, shared: &SharedDefs) -> OptInputDef {
    let flags = parse_opt_input_flags(&opt.flags_raw);

    match opt.opt_type.as_str() {
        "IntegerRange" => {
            let range_data = shared.integer_ranges.get(&opt.dataset_ref);
            OptInputDef {
                name: opt.name.clone(),
                param_type: "integer".to_string(),
                display_name: opt.display_name.clone(),
                hint: opt.hint.clone(),
                range: range_data.map(|r| (r.min, r.max)),
                default: Some(opt.default_value),
                suggested: range_data
                    .map(|r| (r.suggested_start, r.suggested_end, r.suggested_increment)),
                flags,
            }
        }
        "RealRange" => {
            let range_data = shared.real_ranges.get(&opt.dataset_ref);
            OptInputDef {
                name: opt.name.clone(),
                param_type: "real".to_string(),
                display_name: opt.display_name.clone(),
                hint: opt.hint.clone(),
                range: range_data.map(|r| (r.min, r.max)),
                default: Some(opt.default_value),
                suggested: range_data
                    .map(|r| (r.suggested_start, r.suggested_end, r.suggested_increment)),
                flags,
            }
        }
        "IntegerList" => {
            // For now, identify the list type from the dataset ref.
            let list_type = if opt.dataset_ref.contains("MA_Type") {
                "enum:MAType".to_string()
            } else {
                format!("enum:{}", opt.dataset_ref)
            };
            OptInputDef {
                name: opt.name.clone(),
                param_type: list_type,
                display_name: opt.display_name.clone(),
                hint: opt.hint.clone(),
                range: None,
                default: Some(opt.default_value),
                suggested: None,
                flags,
            }
        }
        _ => OptInputDef {
            name: opt.name.clone(),
            param_type: "unknown".to_string(),
            display_name: opt.display_name.clone(),
            hint: opt.hint.clone(),
            range: None,
            default: Some(opt.default_value),
            suggested: None,
            flags,
        },
    }
}

// ---------------------------------------------------------------------------
// parse_table — parse a table_*.c file
// ---------------------------------------------------------------------------

pub fn parse_table(table_source: &str, shared: &SharedDefs) -> Vec<TableFuncDef> {
    let mut results = Vec::new();

    // First, parse any local definitions (ranges, opt-inputs, inputs, outputs)
    // that exist in this table file but NOT in the shared defs.
    let mut local = shared.clone();
    parse_local_defs(table_source, &mut local);

    // Parse DEF_MATH_UNARY_OPERATOR macros
    parse_math_unary_operators(table_source, &mut results);

    // Parse DEF_MATH_BINARY_OPERATOR macros
    parse_math_binary_operators(table_source, &mut results);

    // Parse DEF_FUNCTION blocks
    parse_def_functions(table_source, &local, &mut results);

    results
}

#[allow(clippy::too_many_lines, clippy::similar_names)]
fn parse_local_defs(source: &str, defs: &mut SharedDefs) {
    // Parse local TA_RealRange (static or const)
    let re_real =
        Regex::new(r"(?s)(?:static\s+)?const\s+TA_RealRange\s+(\w+)\s*=\s*\{([^}]+)\}").unwrap();
    for cap in re_real.captures_iter(source) {
        let var_name = cap[1].to_string();
        if let std::collections::hash_map::Entry::Vacant(e) = defs.real_ranges.entry(var_name) {
            let body = strip_c_comments(&cap[2]);
            let vals: Vec<f64> = body.split(',').map(|s| parse_num(s.trim())).collect();
            if vals.len() >= 6 {
                e.insert(RealRange {
                    min: vals[0],
                    max: vals[1],
                    #[allow(clippy::cast_possible_truncation)]
                    precision: vals[2] as i32,
                    suggested_start: vals[3],
                    suggested_end: vals[4],
                    suggested_increment: vals[5],
                });
            }
        }
    }

    // Parse local TA_IntegerRange (static or const)
    let re_int =
        Regex::new(r"(?s)(?:static\s+)?const\s+TA_IntegerRange\s+(\w+)\s*=\s*\{([^}]+)\}").unwrap();
    for cap in re_int.captures_iter(source) {
        let var_name = cap[1].to_string();
        if let std::collections::hash_map::Entry::Vacant(e) = defs.integer_ranges.entry(var_name) {
            let body = strip_c_comments(&cap[2]);
            let vals: Vec<f64> = body.split(',').map(|s| parse_num(s.trim())).collect();
            if vals.len() >= 5 {
                e.insert(IntegerRange {
                    min: vals[0],
                    max: vals[1],
                    suggested_start: vals[2],
                    suggested_end: vals[3],
                    suggested_increment: vals[4],
                });
            }
        }
    }

    // Parse local opt-input definitions
    let re_opt =
        Regex::new(r"(?s)(?:static\s+)?const\s+TA_OptInputParameterInfo\s+(\w+)\s*=\s*\{([^}]+)\}")
            .unwrap();
    for cap in re_opt.captures_iter(source) {
        let var_name = cap[1].to_string();
        if let std::collections::hash_map::Entry::Vacant(e) = defs.opt_inputs.entry(var_name) {
            if let Some(opt) = parse_opt_input_body(&cap[2]) {
                e.insert(opt);
            }
        }
    }

    // Parse local output definitions
    #[allow(clippy::similar_names)]
    let re_out =
        Regex::new(r"(?s)(?:static\s+)?const\s+TA_OutputParameterInfo\s+(\w+)\s*=\s*\{([^}]+)\}")
            .unwrap();
    for cap in re_out.captures_iter(source) {
        let var_name = cap[1].to_string();
        if let std::collections::hash_map::Entry::Vacant(e) = defs.outputs.entry(var_name) {
            let body = strip_c_comments(&cap[2]);
            let parts: Vec<&str> = body.split(',').collect();
            if parts.len() >= 3 {
                let output_type_raw = parts[0].trim();
                let name = parts[1].trim().trim_matches('"').to_string();
                let flags_raw = parts[2..]
                    .iter()
                    .map(|s| s.trim())
                    .collect::<Vec<_>>()
                    .join("|");
                let output_type = match output_type_raw {
                    "TA_Output_Real" => "real",
                    "TA_Output_Integer" => "integer",
                    _ => "unknown",
                }
                .to_string();
                let flags = parse_output_flags(&flags_raw);
                e.insert(SharedOutputDef {
                    output_type,
                    name,
                    flags,
                });
            }
        }
    }

    // Parse local input definitions
    let re_in =
        Regex::new(r"(?s)(?:static\s+)?const\s+TA_InputParameterInfo\s+(\w+)\s*=\s*\{([^}]+)\}")
            .unwrap();
    for cap in re_in.captures_iter(source) {
        let var_name = cap[1].to_string();
        if let std::collections::hash_map::Entry::Vacant(e) = defs.inputs.entry(var_name) {
            let body = strip_c_comments(&cap[2]);
            let parts: Vec<&str> = body.split(',').collect();
            if parts.len() >= 3 {
                let input_type_raw = parts[0].trim();
                let name = parts[1].trim().trim_matches('"').to_string();
                let flags_raw: String = parts[2..]
                    .iter()
                    .map(|s| s.trim())
                    .collect::<Vec<_>>()
                    .join("|");
                let input_type = match input_type_raw {
                    "TA_Input_Real" => "real",
                    "TA_Input_Integer" => "integer",
                    "TA_Input_Price" => "price",
                    _ => "unknown",
                }
                .to_string();
                let price_flags = if input_type == "price" {
                    parse_price_flags(&flags_raw)
                } else {
                    vec![]
                };
                e.insert(SharedInputDef {
                    input_type,
                    name,
                    price_flags,
                });
            }
        }
    }
}

fn parse_math_unary_operators(source: &str, results: &mut Vec<TableFuncDef>) {
    let re =
        Regex::new(r#"DEF_MATH_UNARY_OPERATOR\(\s*(\w+)\s*,\s*"([^"]+)"\s*,\s*"([^"]+)"\s*\)"#)
            .unwrap();

    for cap in re.captures_iter(source) {
        results.push(TableFuncDef {
            name: cap[1].to_string(),
            camel_case: cap[3].to_string(),
            group: "Math Transform".to_string(),
            hint: cap[2].to_string(),
            flags: vec![],
            inputs: vec![InputDef {
                name: "inReal".to_string(),
                param_type: "real".to_string(),
                price_flags: vec![],
            }],
            optional_inputs: vec![],
            outputs: vec![OutputDef {
                name: "outReal".to_string(),
                param_type: "real".to_string(),
                flags: vec!["line".to_string()],
            }],
        });
    }
}

fn parse_math_binary_operators(source: &str, results: &mut Vec<TableFuncDef>) {
    let re =
        Regex::new(r#"DEF_MATH_BINARY_OPERATOR\(\s*(\w+)\s*,\s*"([^"]+)"\s*,\s*"([^"]+)"\s*\)"#)
            .unwrap();

    for cap in re.captures_iter(source) {
        results.push(TableFuncDef {
            name: cap[1].to_string(),
            camel_case: cap[3].to_string(),
            group: "Math Operators".to_string(),
            hint: cap[2].to_string(),
            flags: vec![],
            inputs: vec![
                InputDef {
                    name: "inReal0".to_string(),
                    param_type: "real".to_string(),
                    price_flags: vec![],
                },
                InputDef {
                    name: "inReal1".to_string(),
                    param_type: "real".to_string(),
                    price_flags: vec![],
                },
            ],
            optional_inputs: vec![],
            outputs: vec![OutputDef {
                name: "outReal".to_string(),
                param_type: "real".to_string(),
                flags: vec!["line".to_string()],
            }],
        });
    }
}

fn parse_def_functions(source: &str, local: &SharedDefs, results: &mut Vec<TableFuncDef>) {
    // Strip C comments first so they don't interfere with regex matching
    let stripped = strip_c_comments(source);
    // Match DEF_FUNCTION(...) calls with 5 arguments
    let re = Regex::new(
        r#"(?s)DEF_FUNCTION\(\s*(\w+)\s*,\s*(\w+)\s*,\s*"([^"]+)"\s*,\s*"([^"]+)"\s*,\s*([^)]+)\)"#,
    )
    .unwrap();

    for cap in re.captures_iter(&stripped) {
        let name = cap[1].to_string();
        let group_id = cap[2].trim().to_string();
        let hint = cap[3].to_string();
        let camel_case = cap[4].to_string();
        let flags_raw = cap[5].trim().to_string();

        let group = resolve_group(&group_id, local);
        let flags = parse_func_flags(&flags_raw);

        // Resolve inputs
        let inputs = resolve_inputs(&name, source, local);
        let optional_inputs = resolve_opt_inputs(&name, source, local);
        let outputs = resolve_outputs(&name, source, local);

        results.push(TableFuncDef {
            name,
            camel_case,
            group,
            hint,
            flags,
            inputs,
            optional_inputs,
            outputs,
        });
    }
}

fn resolve_inputs(func_name: &str, source: &str, defs: &SharedDefs) -> Vec<InputDef> {
    // Find the TA_FUNCNAME_Inputs[] array
    let pattern = format!(
        r"(?s)\*TA_{}_Inputs\s*\[\s*\]\s*=\s*\{{([^}}]*)\}}",
        regex::escape(func_name)
    );
    let re = Regex::new(&pattern).unwrap();

    let Some(cap) = re.captures(source) else {
        return vec![];
    };

    let body = &cap[1];
    let mut inputs = Vec::new();

    // Find all &REF entries
    let ref_re = Regex::new(r"&(\w+)").unwrap();
    for ref_cap in ref_re.captures_iter(body) {
        let ref_name = &ref_cap[1];
        if ref_name == "NULL" {
            continue;
        }
        if let Some(input_def) = defs.inputs.get(ref_name) {
            inputs.push(InputDef {
                name: input_def.name.clone(),
                param_type: input_def.input_type.clone(),
                price_flags: input_def.price_flags.clone(),
            });
        }
    }

    inputs
}

fn resolve_opt_inputs(func_name: &str, source: &str, defs: &SharedDefs) -> Vec<OptInputDef> {
    let pattern = format!(
        r"(?s)\*TA_{}_OptInputs\s*\[\s*\]\s*=\s*\{{([^}}]*)\}}",
        regex::escape(func_name)
    );
    let re = Regex::new(&pattern).unwrap();

    let Some(cap) = re.captures(source) else {
        return vec![];
    };

    let body = &cap[1];
    let mut opt_inputs = Vec::new();

    let ref_re = Regex::new(r"&(\w+)").unwrap();
    for ref_cap in ref_re.captures_iter(body) {
        let ref_name = &ref_cap[1];
        if ref_name == "NULL" {
            continue;
        }
        if let Some(opt_def) = defs.opt_inputs.get(ref_name) {
            opt_inputs.push(resolve_opt_input(opt_def, defs));
        }
    }

    opt_inputs
}

fn resolve_outputs(func_name: &str, source: &str, defs: &SharedDefs) -> Vec<OutputDef> {
    let pattern = format!(
        r"(?s)\*TA_{}_Outputs\s*\[\s*\]\s*=\s*\{{([^}}]*)\}}",
        regex::escape(func_name)
    );
    let re = Regex::new(&pattern).unwrap();

    let Some(cap) = re.captures(source) else {
        return vec![];
    };

    let body = &cap[1];
    let mut outputs = Vec::new();

    let ref_re = Regex::new(r"&(\w+)").unwrap();
    for ref_cap in ref_re.captures_iter(body) {
        let ref_name = &ref_cap[1];
        if ref_name == "NULL" {
            continue;
        }
        if let Some(output_def) = defs.outputs.get(ref_name) {
            outputs.push(OutputDef {
                name: output_def.name.clone(),
                param_type: output_def.output_type.clone(),
                flags: output_def.flags.clone(),
            });
        }
    }

    outputs
}

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

#[cfg(test)]
#[allow(clippy::float_cmp)]
mod tests {
    use super::*;
    use std::path::Path;

    fn load_file(relative_path: &str) -> String {
        // Walk up from this file to find the project root
        let manifest_dir = env!("CARGO_MANIFEST_DIR");
        let project_root = Path::new(manifest_dir).parent().unwrap().parent().unwrap();
        let path = project_root.join(relative_path);
        std::fs::read_to_string(&path)
            .unwrap_or_else(|e| panic!("Failed to read {}: {}", path.display(), e))
    }

    fn load_shared() -> SharedDefs {
        let source = load_file("src/ta_abstract/ta_def_ui.c");
        parse_shared_defs(&source)
    }

    #[test]
    #[allow(clippy::cognitive_complexity)]
    fn test_parse_shared_defs() {
        let defs = load_shared();

        // Group IDs
        assert_eq!(
            defs.group_ids.get("TA_GroupId_OverlapStudies"),
            Some(&"Overlap Studies".to_string())
        );
        assert_eq!(
            defs.group_ids.get("TA_GroupId_MathOperators"),
            Some(&"Math Operators".to_string())
        );
        assert_eq!(
            defs.group_ids.get("TA_GroupId_MathTransform"),
            Some(&"Math Transform".to_string())
        );

        // Input definitions
        let real_input = defs.inputs.get("TA_DEF_UI_Input_Real").unwrap();
        assert_eq!(real_input.input_type, "real");
        assert_eq!(real_input.name, "inReal");

        let price_ohlcv = defs.inputs.get("TA_DEF_UI_Input_Price_OHLCV").unwrap();
        assert_eq!(price_ohlcv.input_type, "price");
        assert_eq!(price_ohlcv.name, "inPriceOHLCV");
        assert!(price_ohlcv.price_flags.contains(&"open".to_string()));
        assert!(price_ohlcv.price_flags.contains(&"volume".to_string()));

        // Output definitions
        let real_output = defs.outputs.get("TA_DEF_UI_Output_Real").unwrap();
        assert_eq!(real_output.output_type, "real");
        assert_eq!(real_output.name, "outReal");
        assert_eq!(real_output.flags, vec!["line".to_string()]);

        // Integer ranges
        let range = defs
            .integer_ranges
            .get("TA_DEF_TimePeriod_Positive_Minimum2")
            .unwrap();
        assert_eq!(range.min, 2.0);
        assert_eq!(range.max, 100_000.0);
        assert_eq!(range.suggested_start, 4.0);
        assert_eq!(range.suggested_end, 200.0);
        assert_eq!(range.suggested_increment, 1.0);

        // Real ranges
        let nb_dev = defs.real_ranges.get("TA_DEF_NbDeviation").unwrap();
        assert_eq!(nb_dev.min, f64::MIN);
        assert_eq!(nb_dev.max, f64::MAX);
        assert_eq!(nb_dev.precision, 2);

        // Opt-input definitions
        let tp30 = defs
            .opt_inputs
            .get("TA_DEF_UI_TimePeriod_30_MINIMUM2")
            .unwrap();
        assert_eq!(tp30.opt_type, "IntegerRange");
        assert_eq!(tp30.name, "optInTimePeriod");
        assert_eq!(tp30.display_name, "Time Period");
        assert_eq!(tp30.default_value, 30.0);
        assert_eq!(tp30.hint, "Time period");
        assert_eq!(tp30.dataset_ref, "TA_DEF_TimePeriod_Positive_Minimum2");

        // MA method (IntegerList)
        let ma = defs.opt_inputs.get("TA_DEF_UI_MA_Method").unwrap();
        assert_eq!(ma.opt_type, "IntegerList");
        assert_eq!(ma.name, "optInMAType");
        assert_eq!(ma.display_name, "MA Type");

        // Vertical shift has TA_OPTIN_IS_PERCENT flag
        let vshift = defs.opt_inputs.get("TA_DEF_UI_VerticalShift").unwrap();
        assert_eq!(vshift.flags_raw.trim(), "TA_OPTIN_IS_PERCENT");
    }

    #[test]
    fn test_parse_sma_from_table() {
        let shared = load_shared();
        let table_source = load_file("src/ta_abstract/tables/table_s.c");
        let funcs = parse_table(&table_source, &shared);

        let sma = funcs
            .iter()
            .find(|f| f.name == "SMA")
            .expect("SMA not found");

        assert_eq!(sma.name, "SMA");
        assert_eq!(sma.camel_case, "Sma");
        assert_eq!(sma.group, "Overlap Studies");
        assert_eq!(sma.hint, "Simple Moving Average");
        assert_eq!(sma.flags, vec!["overlap"]);

        // Inputs
        assert_eq!(sma.inputs.len(), 1);
        assert_eq!(sma.inputs[0].name, "inReal");
        assert_eq!(sma.inputs[0].param_type, "real");

        // Optional inputs
        assert_eq!(sma.optional_inputs.len(), 1);
        let opt = &sma.optional_inputs[0];
        assert_eq!(opt.name, "optInTimePeriod");
        assert_eq!(opt.param_type, "integer");
        assert_eq!(opt.display_name, "Time Period");
        assert_eq!(opt.hint, "Time period");
        // Period 1 = "no smoothing" is allowed since 0.6.5 (issues #48/#59).
        assert_eq!(opt.range, Some((1.0, 100_000.0)));
        assert_eq!(opt.default, Some(30.0));
        assert_eq!(opt.suggested, Some((1.0, 200.0, 1.0)));

        // Outputs
        assert_eq!(sma.outputs.len(), 1);
        assert_eq!(sma.outputs[0].name, "outReal");
        assert_eq!(sma.outputs[0].param_type, "real");
        assert_eq!(sma.outputs[0].flags, vec!["line"]);
    }

    #[test]
    fn test_parse_mult_from_table() {
        let shared = load_shared();
        let table_source = load_file("src/ta_abstract/tables/table_m.c");
        let funcs = parse_table(&table_source, &shared);

        let mult = funcs
            .iter()
            .find(|f| f.name == "MULT")
            .expect("MULT not found");

        assert_eq!(mult.name, "MULT");
        assert_eq!(mult.camel_case, "Mult");
        assert_eq!(mult.group, "Math Operators");
        assert_eq!(mult.hint, "Vector Arithmetic Mult");
        assert!(mult.flags.is_empty());

        // Binary operator: 2 real inputs
        assert_eq!(mult.inputs.len(), 2);
        assert_eq!(mult.inputs[0].name, "inReal0");
        assert_eq!(mult.inputs[0].param_type, "real");
        assert_eq!(mult.inputs[1].name, "inReal1");
        assert_eq!(mult.inputs[1].param_type, "real");

        // No optional inputs
        assert!(mult.optional_inputs.is_empty());

        // 1 real output
        assert_eq!(mult.outputs.len(), 1);
        assert_eq!(mult.outputs[0].name, "outReal");
        assert_eq!(mult.outputs[0].param_type, "real");
    }

    #[test]
    #[allow(clippy::similar_names)]
    fn test_parse_stoch_from_table() {
        let shared = load_shared();
        let table_source = load_file("src/ta_abstract/tables/table_s.c");
        let funcs = parse_table(&table_source, &shared);

        let stoch = funcs
            .iter()
            .find(|f| f.name == "STOCH")
            .expect("STOCH not found");

        assert_eq!(stoch.name, "STOCH");
        assert_eq!(stoch.camel_case, "Stoch");
        assert_eq!(stoch.group, "Momentum Indicators");
        assert_eq!(stoch.hint, "Stochastic");
        assert!(stoch.flags.is_empty());

        // Input: price HLC
        assert_eq!(stoch.inputs.len(), 1);
        assert_eq!(stoch.inputs[0].param_type, "price");
        assert_eq!(stoch.inputs[0].name, "inPriceHLC");
        assert!(stoch.inputs[0].price_flags.contains(&"high".to_string()));
        assert!(stoch.inputs[0].price_flags.contains(&"low".to_string()));
        assert!(stoch.inputs[0].price_flags.contains(&"close".to_string()));

        // Multiple outputs
        assert_eq!(stoch.outputs.len(), 2);
        assert_eq!(stoch.outputs[0].name, "outSlowK");
        assert_eq!(stoch.outputs[0].flags, vec!["dash_line"]);
        assert_eq!(stoch.outputs[1].name, "outSlowD");
        assert_eq!(stoch.outputs[1].flags, vec!["dash_line"]);

        // Multiple opt inputs including MAType enums
        assert_eq!(stoch.optional_inputs.len(), 5);

        let fast_k = &stoch.optional_inputs[0];
        assert_eq!(fast_k.name, "optInFastK_Period");
        assert_eq!(fast_k.param_type, "integer");
        assert_eq!(fast_k.default, Some(5.0));

        let slow_k_ma = &stoch.optional_inputs[2];
        assert_eq!(slow_k_ma.name, "optInSlowK_MAType");
        assert_eq!(slow_k_ma.param_type, "enum:MAType");
        assert_eq!(slow_k_ma.display_name, "Slow-K MA");

        let slow_d_ma = &stoch.optional_inputs[4];
        assert_eq!(slow_d_ma.name, "optInSlowD_MAType");
        assert_eq!(slow_d_ma.param_type, "enum:MAType");
    }

    #[test]
    fn test_parse_sin_from_table() {
        let shared = load_shared();
        let table_source = load_file("src/ta_abstract/tables/table_s.c");
        let funcs = parse_table(&table_source, &shared);

        let sin = funcs
            .iter()
            .find(|f| f.name == "SIN")
            .expect("SIN not found");

        assert_eq!(sin.name, "SIN");
        assert_eq!(sin.camel_case, "Sin");
        assert_eq!(sin.group, "Math Transform");
        assert_eq!(sin.hint, "Vector Trigonometric Sin");
        assert!(sin.flags.is_empty());

        // 1 real input
        assert_eq!(sin.inputs.len(), 1);
        assert_eq!(sin.inputs[0].name, "inReal");
        assert_eq!(sin.inputs[0].param_type, "real");

        // No opt inputs
        assert!(sin.optional_inputs.is_empty());

        // 1 real output
        assert_eq!(sin.outputs.len(), 1);
        assert_eq!(sin.outputs[0].name, "outReal");
        assert_eq!(sin.outputs[0].param_type, "real");
    }

    #[test]
    fn test_parse_sar_from_table() {
        let shared = load_shared();
        let table_source = load_file("src/ta_abstract/tables/table_s.c");
        let funcs = parse_table(&table_source, &shared);

        let sar = funcs
            .iter()
            .find(|f| f.name == "SAR")
            .expect("SAR not found");

        assert_eq!(sar.name, "SAR");
        assert_eq!(sar.camel_case, "Sar");
        assert_eq!(sar.group, "Overlap Studies");
        assert_eq!(sar.hint, "Parabolic SAR");
        assert_eq!(sar.flags, vec!["overlap"]);

        // Input: price HL
        assert_eq!(sar.inputs.len(), 1);
        assert_eq!(sar.inputs[0].param_type, "price");
        assert_eq!(sar.inputs[0].name, "inPriceHL");
        assert!(sar.inputs[0].price_flags.contains(&"high".to_string()));
        assert!(sar.inputs[0].price_flags.contains(&"low".to_string()));

        // Locally-defined opt inputs
        assert_eq!(sar.optional_inputs.len(), 2);

        let accel = &sar.optional_inputs[0];
        assert_eq!(accel.name, "optInAcceleration");
        assert_eq!(accel.param_type, "real");
        assert_eq!(accel.display_name, "Acceleration Factor");
        assert_eq!(accel.default, Some(0.02));
        assert_eq!(accel.range, Some((0.0, f64::MAX)));
        assert_eq!(accel.suggested, Some((0.01, 0.20, 0.01)));

        let max = &sar.optional_inputs[1];
        assert_eq!(max.name, "optInMaximum");
        assert_eq!(max.param_type, "real");
        assert_eq!(max.display_name, "AF Maximum");
        assert_eq!(max.default, Some(0.20));
    }
}
