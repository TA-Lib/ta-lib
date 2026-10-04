//! The pattern-flag rules `parse_yaml` enforces, one fixture per rule. Each
//! fixture breaks exactly one rule and must get exactly one complaint, so a rule
//! that stops firing, or starts firing on its neighbour's fixture, fails here.

use std::path::Path;

use ta_codegen_lib::ir::{FuncDef, ParamType};
use ta_codegen_lib::parser::yaml::{check_flags, parse_yaml_str};

const PATH: &str = "cdlfixture/cdlfixture.yaml";

fn yaml(func_flags: &str, out_type: &str, out_flags: &str) -> String {
    format!(
        "name: CDLFIXTURE\ngroup: Pattern Recognition\nhint: Fixture\nflags: [{func_flags}]\n\
         inputs:\n  - name: inPriceOHLC\n    type: price\n    price_components: [open, high, low, close]\n\
         optional_inputs:\n  - name: optInX\n    type: integer\n    range: [1, 10]\n    default: 2\n\
         outputs:\n  - name: outInteger\n    type: {out_type}\n    flags: [{out_flags}]\n"
    )
}

/// A valid candlestick fixture, then the flags under test set directly, so the
/// fixture reaches `check_flags` instead of `parse_yaml_str`'s panic.
fn fixture(func_flags: &[&str], out_type: ParamType, out_flags: &[&str]) -> FuncDef {
    let mut f = parse_yaml_str(&yaml("candlestick", "integer", "line, pattern_bool, zero, positive"), Path::new(PATH));
    f.flags = func_flags.iter().map(|s| (*s).to_string()).collect();
    f.outputs[0].param_type = out_type;
    f.outputs[0].flags = out_flags.iter().map(|s| (*s).to_string()).collect();
    f
}

fn one_complaint(f: &FuncDef, needle: &str) {
    let errs = check_flags(f).expect_err("the fixture breaks a rule");
    assert_eq!(errs.len(), 1, "expected only `{needle}`: {errs:?}");
    assert!(errs[0].contains(needle), "expected `{needle}`: {errs:?}");
}

const CDL: &[&str] = &["candlestick", "stream"];
const PLAIN: &[&str] = &["stream"];

#[test]
fn bool_takes_exactly_zero_and_positive() {
    one_complaint(&fixture(CDL, ParamType::Integer, &["pattern_bool", "zero", "positive", "negative"]), "`pattern_bool` takes");
}

#[test]
fn bool_excludes_bull_bear() {
    one_complaint(&fixture(CDL, ParamType::Integer, &["pattern_bool", "pattern_bull_bear", "zero", "positive"]), "`pattern_bool` takes");
}

#[test]
fn bull_bear_needs_zero() {
    one_complaint(&fixture(CDL, ParamType::Integer, &["pattern_bull_bear", "positive"]), "`pattern_bull_bear` needs");
}

#[test]
fn bull_bear_needs_a_sign() {
    one_complaint(&fixture(CDL, ParamType::Integer, &["pattern_bull_bear", "zero"]), "`pattern_bull_bear` needs");
}

#[test]
fn a_candlestick_integer_output_is_a_pattern_output() {
    one_complaint(&fixture(CDL, ParamType::Integer, &["line"]), "is a pattern output");
}

#[test]
fn weak_needs_a_call_or_color() {
    one_complaint(&fixture(PLAIN, ParamType::Integer, &["zero", "positive", "negative", "pattern_weak"]), "`pattern_weak` needs");
}

#[test]
fn confirm_needs_bull_bear() {
    one_complaint(&fixture(CDL, ParamType::Integer, &["zero", "positive", "negative", "pattern_confirm"]), "`pattern_confirm` needs");
}

#[test]
fn a_pattern_flag_needs_an_integer_output() {
    one_complaint(&fixture(PLAIN, ParamType::Real, &["pattern_bull_bear", "zero", "positive"]), "needs an integer output");
}

#[test]
fn an_unknown_function_flag_is_rejected() {
    one_complaint(&fixture(&["candelstick", "stream"], ParamType::Integer, &["line"]), "unknown function flag `candelstick`");
}

#[test]
fn an_unknown_output_flag_is_rejected() {
    one_complaint(&fixture(CDL, ParamType::Integer, &["line", "pattern_bool", "zero", "positive", "lin"]), "unknown output flag `lin`");
}

#[test]
fn an_unknown_optional_input_flag_is_rejected() {
    let mut f = fixture(CDL, ParamType::Integer, &["line", "pattern_bool", "zero", "positive"]);
    f.optional_inputs[0].flags = vec!["percnt".to_string()];
    one_complaint(&f, "unknown flag `percnt`");
}

#[test]
#[should_panic(expected = "is a pattern output")]
fn parsing_applies_the_rules() {
    parse_yaml_str(&yaml("candlestick", "integer", "line"), Path::new(PATH));
}
