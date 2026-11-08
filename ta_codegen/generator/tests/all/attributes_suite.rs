//! Per-function attributes: the typed YAML fields, and the one JSON string the
//! C library and the managed registries each render on their own.

use std::collections::HashMap;
use std::path::Path;

use crate::common;
use ta_codegen_lib::backends::{abstract_rows, func_api_xml, ta_abstract_c};
use ta_codegen_lib::parser::yaml::parse_yaml_str;

const PATH: &str = "attrfixture/attrfixture.yaml";

fn yaml(outputs: &str) -> String {
    format!(
        "name: ATTRFIXTURE\ngroup: Momentum Indicators\nhint: Fixture\n\
         inputs:\n  - name: inReal\n    type: real\noutputs:\n{outputs}"
    )
}

fn both(outputs: &str) -> (String, String) {
    let f = parse_yaml_str(&yaml(outputs), Path::new(PATH));
    let row = abstract_rows::rows(std::slice::from_ref(&f), &common::load_enums()).remove(0);
    (ta_abstract_c::attributes_json(&f), row.attributes)
}

#[test]
fn levels_render_canonically_in_both_renderers() {
    let (c, rows) = both(
        "  - name: outA\n    type: real\n  - name: outB\n    type: real\n    levels: [-0.5, 0, 26.5, 1000000]\n",
    );
    assert_eq!(c, r#"{"outputs":{"outB":{"levels":[-0.5,0,26.5,1000000]}}}"#);
    assert_eq!(rows, c);
    let (c, rows) = both("  - name: outA\n    type: real\n");
    assert_eq!((c.as_str(), rows.as_str()), ("{}", "{}"));
}

#[test]
#[should_panic(expected = "not strictly ascending")]
fn descending_levels_are_rejected() {
    both("  - name: outReal\n    type: real\n    levels: [70, 30]\n");
}

#[test]
#[should_panic(expected = "not strictly ascending")]
fn a_repeated_level_is_rejected() {
    both("  - name: outReal\n    type: real\n    levels: [30, 30]\n");
}

#[test]
#[should_panic(expected = "-0, which the JSON would spell")]
fn a_negative_zero_level_is_rejected() {
    both("  - name: outReal\n    type: real\n    levels: [-0.0, 30]\n");
}

#[test]
#[should_panic(expected = "an empty list")]
fn empty_levels_are_rejected() {
    both("  - name: outReal\n    type: real\n    levels: []\n");
}

#[test]
#[should_panic(expected = "not finite")]
fn a_non_finite_level_is_rejected() {
    both("  - name: outReal\n    type: real\n    levels: [30, .inf]\n");
}

#[test]
#[should_panic(expected = "needs a `real` output")]
fn levels_on_an_integer_output_are_rejected() {
    both("  - name: outInteger\n    type: integer\n    levels: [0]\n");
}

#[test]
#[should_panic(expected = "unknown field `level`")]
fn a_misspelt_attribute_is_rejected() {
    both("  - name: outReal\n    type: real\n    level: [30, 70]\n");
}

/// Over the corpus the two renderings agree, RSI carries Wilder's levels, and
/// the XML embeds exactly the strings other than `{}`.
#[test]
fn the_corpus_renders_one_string_per_function() {
    let funcs: Vec<_> = common::discover_indicators().iter().map(|n| common::load_indicator(n).0).collect();
    let rows: HashMap<String, String> = abstract_rows::rows(&funcs, &common::load_enums())
        .into_iter()
        .map(|r| (r.name, r.attributes))
        .collect();
    let xml = func_api_xml::generate_string(&funcs);
    let mut non_empty = 0;
    for f in &funcs {
        let c = ta_abstract_c::attributes_json(f);
        assert_eq!(rows[&f.name], c, "{}: the two renderers disagree", f.name);
        let element = format!("<Attributes><![CDATA[{c}]]></Attributes>");
        assert_eq!(xml.contains(&element), c != "{}", "{}: {element}", f.name);
        non_empty += usize::from(c != "{}");
    }
    assert_eq!(rows["RSI"], r#"{"outputs":{"outReal":{"levels":[30,70]}}}"#);
    assert_eq!(xml.matches("<Attributes>").count(), non_empty);
}

/// The spec's key table and the registry list the same keys.
#[test]
fn the_spec_lists_every_attribute_key() {
    let page = std::fs::read_to_string(Path::new(env!("CARGO_MANIFEST_DIR")).join("../../website/src/spec/abstract/README.md"))
        .expect("the spec page");
    let section = page.split("## Attributes {#attributes}").nth(1).expect("the attributes section");
    let section = section.split("\n## ").next().unwrap();
    let rows: Vec<&str> = section
        .lines()
        .filter_map(|l| l.strip_prefix("| `")?.split('`').next())
        .collect();
    let keys: Vec<&str> = ta_codegen_lib::attributes::KEYS.iter().map(|k| k.name).collect();
    assert!(!keys.is_empty());
    assert_eq!(rows, keys, "the table at /spec/abstract/#attributes");
}
