//! The `series` and `domain` input fields: the YAML says what every example,
//! doctest and reference-data run feeds an input, and every emitter reads it.

use std::path::Path;

use crate::common;
use ta_codegen_lib::helper_registry::HelperRegistry;
use ta_codegen_lib::parser::yaml::parse_yaml_str;
use ta_codegen_lib::backends;
use ta_codegen_lib::ir::Series;

const PATH: &str = "domfixture/domfixture.yaml";

fn yaml(input: &str) -> String {
    format!(
        "name: DOMFIXTURE\ngroup: Math Transform\nhint: Fixture\nflags: [stream]\n\
         inputs:\n{input}outputs:\n  - name: outReal\n    type: real\n    flags: [line]\n"
    )
}

/// The Math Transform group rides a ramp over each function's domain; the Math
/// Operators add a sine on the price scale to a ramp through it. Everything else,
/// the rolling and statistic functions included, keeps the price series.
#[test]
fn the_math_groups_declare_synthetic_series() {
    let ramps = [
        ("acos", -1.0, 1.0), ("asin", -1.0, 1.0), ("atan", -5.0, 5.0), ("ceil", -3.0, 3.0), ("floor", -3.0, 3.0),
        ("sin", -6.3, 6.3), ("cos", -6.3, 6.3), ("tan", -1.2, 1.2), ("tanh", -3.0, 3.0), ("exp", -5.0, 5.0),
        ("sinh", -5.0, 5.0), ("cosh", -5.0, 5.0), ("ln", 0.1, 10.0), ("log10", 0.1, 10.0), ("sqrt", 0.0, 100.0),
    ];
    for (name, lo, hi) in ramps {
        let (func, _) = common::load_indicator(name);
        assert_eq!(func.inputs.len(), 1, "{name}");
        assert_eq!(func.inputs[0].series, Series::Ramp, "{name}: series");
        assert_eq!(func.inputs[0].domain, Some((lo, hi)), "{name}: domain");
    }
    for name in ["add", "sub", "mult", "div"] {
        let (func, _) = common::load_indicator(name);
        assert_eq!(func.inputs.len(), 2, "{name}");
        assert_eq!((func.inputs[0].series, func.inputs[0].domain), (Series::Sine, Some((90.0, 110.0))), "{name}: inReal0");
        assert_eq!((func.inputs[1].series, func.inputs[1].domain), (Series::Ramp, Some((95.0, 105.0))), "{name}: inReal1");
    }
    let mut on_prices = 0;
    for name in common::discover_indicators() {
        let (func, _) = common::load_indicator(&name);
        let synthetic = func.inputs.iter().any(|i| i.series != Series::Price);
        let math = matches!(func.group.as_str(), "Math Transform" | "Math Operators");
        assert_eq!(synthetic, math && !["max", "min", "sum", "cumsum", "minmax", "maxindex", "minindex", "minmaxindex", "beta", "correl"].contains(&name.as_str()), "{name}");
        on_prices += usize::from(!synthetic);
    }
    assert!(on_prices > 150, "{on_prices} functions on the price series");
}

#[test]
fn a_series_and_its_domain_parse_on_a_real_input() {
    let f = parse_yaml_str(&yaml("  - name: inReal\n    type: real\n    series: sine\n    domain: [-2.5, 4]\n"), Path::new(PATH));
    assert_eq!((f.inputs[0].series, f.inputs[0].domain), (Series::Sine, Some((-2.5, 4.0))));
    let f = parse_yaml_str(&yaml("  - name: inReal\n    type: real\n    series: ramp\n    domain: [0, 1]\n"), Path::new(PATH));
    assert_eq!((f.inputs[0].series, f.inputs[0].domain), (Series::Ramp, Some((0.0, 1.0))));
    let f = parse_yaml_str(&yaml("  - name: inReal\n    type: real\n    series: price\n"), Path::new(PATH));
    assert_eq!((f.inputs[0].series, f.inputs[0].domain), (Series::Price, None));
    let f = parse_yaml_str(&yaml("  - name: inReal\n    type: real\n"), Path::new(PATH));
    assert_eq!(f.inputs[0].series, Series::Price, "the default");
}

#[test]
#[should_panic(expected = "go together")]
fn a_ramp_without_a_domain_is_rejected() {
    parse_yaml_str(&yaml("  - name: inReal\n    type: real\n    series: ramp\n"), Path::new(PATH));
}

#[test]
#[should_panic(expected = "go together")]
fn a_sine_without_a_domain_is_rejected() {
    parse_yaml_str(&yaml("  - name: inReal\n    type: real\n    series: sine\n"), Path::new(PATH));
}

#[test]
#[should_panic(expected = "go together")]
fn a_domain_without_a_synthetic_series_is_rejected() {
    parse_yaml_str(&yaml("  - name: inReal\n    type: real\n    domain: [-1, 1]\n"), Path::new(PATH));
}

#[test]
#[should_panic(expected = "`series` is `price`, `ramp` or `sine`")]
fn an_unknown_series_is_rejected() {
    parse_yaml_str(&yaml("  - name: inReal\n    type: real\n    series: square\n    domain: [-1, 1]\n"), Path::new(PATH));
}

#[test]
#[should_panic(expected = "`series` needs a `real` input")]
fn a_series_on_an_integer_input_is_rejected() {
    parse_yaml_str(&yaml("  - name: inInteger\n    type: integer\n    series: ramp\n    domain: [-1, 1]\n"), Path::new(PATH));
}

#[test]
#[should_panic(expected = "needs a `real` input")]
fn a_domain_on_an_integer_input_is_rejected() {
    parse_yaml_str(&yaml("  - name: inInteger\n    type: integer\n    domain: [-1, 1]\n"), Path::new(PATH));
}

#[test]
#[should_panic(expected = "needs a `real` input")]
fn a_domain_on_a_price_input_is_rejected() {
    parse_yaml_str(
        &yaml("  - name: inPriceHL\n    type: price\n    price_components: [high, low]\n    domain: [-1, 1]\n"),
        Path::new(PATH),
    );
}

#[test]
#[should_panic(expected = "lo < hi")]
fn an_empty_domain_is_rejected() {
    parse_yaml_str(&yaml("  - name: inReal\n    type: real\n    series: ramp\n    domain: [1, 1]\n"), Path::new(PATH));
}

#[test]
#[should_panic(expected = "lo < hi")]
fn a_three_value_domain_is_rejected() {
    parse_yaml_str(&yaml("  - name: inReal\n    type: real\n    series: ramp\n    domain: [-1, 0, 1]\n"), Path::new(PATH));
}

/// The doc comment lines of the generated Rust module: batch and stream doctests.
fn rust_doc_lines(name: &str) -> Vec<String> {
    let (func, enums) = common::load_indicator(name);
    let registry = common::make_registry();
    let helpers = HelperRegistry::empty();
    let batch = backends::rust_lang::generate(&func, &enums, registry, &helpers);
    let stream = backends::rust_stream::generate(&func, &enums, registry, &helpers);
    let mut lines: Vec<String> = Vec::new();
    for src in [&batch, &stream] {
        lines.extend(src.lines().map(str::trim).filter(|l| l.starts_with("///")).map(String::from));
    }
    assert!(lines.iter().filter(|l| l.contains("let series") && l.contains(": Vec<f64>")).count() >= 2, "{name}: no doctest series");
    lines
}

#[test]
fn the_rust_doctests_feed_the_declared_series_not_the_price_scale() {
    for (name, expected) in [
        ("acos", "(0..252).map(|i| -1.0 + 2.0 * i as f64 / 251.0).collect();"),
        ("tanh", "(0..252).map(|i| -3.0 + 6.0 * i as f64 / 251.0).collect();"),
        ("sqrt", "(0..252).map(|i| 100.0 * i as f64 / 251.0).collect();"),
        ("ln", "(0..252).map(|i| 0.1 + 9.9 * i as f64 / 251.0).collect();"),
    ] {
        let lines = rust_doc_lines(name);
        assert!(!lines.iter().any(|l| l.contains("100.0 +")), "{name}: a 100.0-based series in the doctest");
        assert!(lines.iter().any(|l| l.contains(expected)), "{name}: no `{expected}` in {lines:#?}");
    }
    let add = rust_doc_lines("add");
    assert!(add.iter().any(|l| l.contains("let series0: Vec<f64> = (0..252).map(|i| 100.0 + 10.0 * (0.1 * i as f64).sin()).collect();")), "{add:#?}");
    assert!(add.iter().any(|l| l.contains("let series1: Vec<f64> = (0..252).map(|i| 95.0 + 10.0 * i as f64 / 251.0).collect();")), "{add:#?}");
    let sma = rust_doc_lines("sma");
    assert!(sma.iter().any(|l| l.contains("100.0 + 10.0 * (0.1 * i as f64).sin()")), "sma keeps the price series");
}

#[test]
fn the_typescript_examples_feed_the_declared_series_not_the_price_scale() {
    let enums = common::load_enums();
    for (name, expected, bar) in [
        ("acos", "(_, i) => -1 + 2 * i / 251.0);", "0.42"),
        ("tanh", "(_, i) => -3 + 6 * i / 251.0);", "1.26"),
        ("cosh", "(_, i) => -5 + 10 * i / 251.0);", "2.1"),
        ("div", "(_, i) => 95 + 10 * i / 251.0);", "102.1"),
    ] {
        let (func, _) = common::load_indicator(name);
        let examples = backends::ts_doc::examples(&func, &enums);
        assert!(examples.len() >= 3, "{name}: {} examples", examples.len());
        // The series line wraps past WRAP, so compare whitespace-normalised text.
        let body: Vec<&str> = examples.iter().flat_map(|e| e.body.iter().map(|l| l.text.as_str())).collect();
        let text = body.join("\n").split_whitespace().collect::<Vec<_>>().join(" ");
        assert!(name == "div" || !text.contains("100 +"), "{name}: a 100-based series in an example");
        assert!(text.contains(expected), "{name}: no `{expected}` in {body:?}");
        assert!(text.contains(&format!("{bar})")), "{name}: no further bar `{bar}` in {body:?}");
    }
}

/// The C table `ta_refdata` builds its input columns from carries the series kind,
/// so the CSV column and the doctest series come from one declaration.
#[test]
fn the_refdata_header_lists_the_series_kind() {
    let path = Path::new(env!("CARGO_MANIFEST_DIR")).join("../../src/tools/ta_refdata/ta_refdata_domains.h");
    let text = std::fs::read_to_string(&path).unwrap_or_else(|e| panic!("{}: {e}", path.display()));
    assert!(text.contains("char series; double lo, hi; } REFDATA_DOMAIN[] = {"), "{text}");
    assert!(text.contains("    { \"ACOS\", \"inReal\", 'r', -1.0, 1.0 },"), "{text}");
    assert!(text.contains("    { \"ADD\", \"inReal0\", 's', 90.0, 110.0 },"), "{text}");
    assert!(text.contains("    { \"ADD\", \"inReal1\", 'r', 95.0, 105.0 },"), "{text}");
    assert!(text.contains("    { \"LN\", \"inReal\", 'r', 0.1, 10.0 },"), "{text}");
    assert!(!text.contains("\"SMA\"") && !text.contains("\"MAX\"") && !text.contains("\"BETA\""), "{text}");
    assert!(text.ends_with("    { NULL, NULL, 0, 0.0, 0.0 }\n};\n"), "{text}");
    assert_eq!(text.matches("'r'").count(), 19, "{text}");
    assert_eq!(text.matches("'s'").count(), 4, "{text}");
}
