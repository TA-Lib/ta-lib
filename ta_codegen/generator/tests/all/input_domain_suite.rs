//! The `domain` input field: the YAML carries the range an example input has to
//! live in, and every example emitter reads it.

use std::path::Path;

use crate::common;
use ta_codegen_lib::helper_registry::HelperRegistry;
use ta_codegen_lib::parser::yaml::parse_yaml_str;
use ta_codegen_lib::backends;

const PATH: &str = "domfixture/domfixture.yaml";

fn yaml(input: &str) -> String {
    format!(
        "name: DOMFIXTURE\ngroup: Math Transform\nhint: Fixture\nflags: [stream]\n\
         inputs:\n{input}outputs:\n  - name: outReal\n    type: real\n    flags: [line]\n"
    )
}

#[test]
fn the_former_hard_coded_set_carries_a_domain() {
    for (name, lo, hi) in [("acos", -1.0, 1.0), ("asin", -1.0, 1.0), ("tanh", -3.0, 3.0)] {
        let (func, _) = common::load_indicator(name);
        assert_eq!(func.inputs.len(), 1, "{name}");
        assert_eq!(func.inputs[0].domain, Some((lo, hi)), "{name}: domain");
    }
    let (sma, _) = common::load_indicator("sma");
    assert_eq!(sma.inputs[0].domain, None, "a price-scale input carries none");
}

#[test]
fn a_domain_parses_on_a_real_input() {
    let f = parse_yaml_str(&yaml("  - name: inReal\n    type: real\n    domain: [-2.5, 4]\n"), Path::new(PATH));
    assert_eq!(f.inputs[0].domain, Some((-2.5, 4.0)));
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
    parse_yaml_str(&yaml("  - name: inReal\n    type: real\n    domain: [1, 1]\n"), Path::new(PATH));
}

#[test]
#[should_panic(expected = "lo < hi")]
fn a_three_value_domain_is_rejected() {
    parse_yaml_str(&yaml("  - name: inReal\n    type: real\n    domain: [-1, 0, 1]\n"), Path::new(PATH));
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
    assert!(lines.iter().filter(|l| l.contains("let series: Vec<f64>")).count() >= 2, "{name}: no doctest series");
    lines
}

#[test]
fn the_rust_doctests_feed_the_domain_not_the_price_scale() {
    for (name, expected) in [
        ("acos", "(0..252).map(|i| (0.1 * i as f64).sin()).collect();"),
        ("tanh", "(0..252).map(|i| 3.0 * (0.1 * i as f64).sin()).collect();"),
        ("exp", "(0..252).map(|i| 5.0 * (0.1 * i as f64).sin()).collect();"),
    ] {
        let lines = rust_doc_lines(name);
        assert!(!lines.iter().any(|l| l.contains("100.0")), "{name}: a 100.0-based series in the doctest");
        assert!(lines.iter().any(|l| l.contains(expected)), "{name}: no `{expected}`");
    }
    let sma = rust_doc_lines("sma");
    assert!(sma.iter().any(|l| l.contains("100.0 + 10.0 * (0.1 * i as f64).sin()")), "sma keeps the price series");
}

#[test]
fn the_typescript_examples_feed_the_domain_not_the_price_scale() {
    let enums = common::load_enums();
    for (name, expected, bar) in [
        ("acos", "(_, i) => Math.sin(0.1 * i));", "0.42"),
        ("tanh", "(_, i) => 3 * Math.sin(0.1 * i));", "1.26"),
        ("cosh", "(_, i) => 5 * Math.sin(0.1 * i));", "2.1"),
    ] {
        let (func, _) = common::load_indicator(name);
        let examples = backends::ts_doc::examples(&func, &enums);
        assert!(examples.len() >= 3, "{name}: {} examples", examples.len());
        // The series line wraps past WRAP, so compare whitespace-normalised text.
        let body: Vec<&str> = examples.iter().flat_map(|e| e.body.iter().map(|l| l.text.as_str())).collect();
        let text = body.join("\n").split_whitespace().collect::<Vec<_>>().join(" ");
        assert!(!text.contains("100 +"), "{name}: a 100-based series in an example");
        assert!(text.contains(expected), "{name}: no `{expected}` in {body:?}");
        assert!(text.contains(&format!("({bar})")), "{name}: no further bar `{bar}` in {body:?}");
    }
}

/// An affine map of the default series onto `[lo, hi]`, spelled once per language.
#[test]
fn the_domain_series_is_the_affine_image_of_the_default_one() {
    let lit = |v: f64| format!("{v}");
    assert_eq!(backends::rust_doc::domain_series((-1.0, 1.0), "s", lit), "s");
    assert_eq!(backends::rust_doc::domain_series((-3.0, 3.0), "s", lit), "3 * s");
    assert_eq!(backends::rust_doc::domain_series((0.0, 2.0), "s", lit), "1 + s");
    assert_eq!(backends::rust_doc::domain_series((10.0, 20.0), "s", lit), "15 + 5 * s");
    assert!((backends::rust_doc::domain_bar((10.0, 20.0), 0.42) - 17.1).abs() < 1e-12);
}
