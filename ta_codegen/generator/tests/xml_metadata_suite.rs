//! `ta_func_api.xml` must agree with what the YAML declares (issue #207).
//!
//! Two metadata surfaces ship for every optional parameter: the C abstract
//! table, read at run time through `TA_GetOptInputParameterInfo()`, and the
//! generated `ta_func_api.xml`, which sits at the repo root and is embedded in
//! `src/ta_abstract/ta_func_api.c` and in the Rust crate. Both are generated
//! from the same YAML. Nothing compared them.
//!
//! They disagreed for every integer optional parameter across 71 of 171
//! functions: `write_integer_opt` emitted the range *maximum* three times in
//! place of the declared sweep triple, for bug-compatibility with `gen_code`,
//! which the v0.7.1 cutover removed. A consumer reading the XML to drive a
//! parameter sweep got a one-point sweep pinned at the top of the domain —
//! most visibly `TA_DEF_HorizontalShiftPeriod`, whose parameter runs over
//! [-200, 200] and whose XML said "start at 200, end at 200, step by 200".
//! The real-typed path (`write_real_opt`) had it right the whole time, which
//! is why the divergence was invisible: nothing sampled the integer path.
//!
//! This gate reads the **emitted XML text**, not the IR a second time. That
//! distinction is the whole point: a test that renders both surfaces from the
//! IR and compares them agrees with itself no matter what either emitter does.
//! Parsing the artifact means the check fails if either the XML writer or the
//! YAML parser drifts, in either direction.

use std::path::{Path, PathBuf};

use ta_codegen_lib::{ir::ParamType, parser};

fn repo_root() -> PathBuf {
    Path::new(env!("CARGO_MANIFEST_DIR")).join("../..")
}

fn input_dir() -> PathBuf {
    Path::new(env!("CARGO_MANIFEST_DIR")).join("../input")
}

/// One optional input as the YAML declares it: identifier, display name, type,
/// and the suggested (start, end, increment) triple where one is declared.
type DeclaredOpt = (String, String, ParamType, Option<(f64, f64, f64)>);

/// The declared metadata for every function: name -> (opt name, type, suggested).
fn declared() -> Vec<(String, Vec<DeclaredOpt>)> {
    let mut out = Vec::new();
    for e in std::fs::read_dir(input_dir()).expect("read input dir").filter_map(Result::ok) {
        let dir = e.path();
        if !dir.is_dir() {
            continue;
        }
        let n = e.file_name().to_string_lossy().to_string();
        let yaml = dir.join(format!("{n}.yaml"));
        if !yaml.exists() {
            continue;
        }
        let f = parser::yaml::parse_yaml(&yaml);
        let opts = f
            .optional_inputs
            .iter()
            .map(|o| {
                // The XML carries the display name; the YAML's `name` is the
                // identifier the C API uses.
                let shown = o.display_name.clone().unwrap_or_else(|| o.name.clone());
                (o.name.clone(), shown, o.param_type.clone(), o.suggested)
            })
            .collect();
        out.push((f.name.clone(), opts));
    }
    assert!(out.len() >= 200, "expected the whole input tree, got {}", out.len());
    out
}

/// One `<OptionalInputArgument>` block: its name, and the
/// (SuggestedStart, SuggestedEnd, SuggestedIncrement) triple as written — absent
/// when the block declares no `<Range>`.
type XmlOptBlock = (String, Option<(String, String, String)>);

/// One function's `<OptionalInputArgument>` blocks, in document order.
fn xml_opt_blocks(xml: &str, func: &str) -> Vec<XmlOptBlock> {
    // The function's own <FinancialFunction> element, delimited so a name that
    // is a prefix of another (MIN / MININDEX) cannot bleed across.
    let open = format!("<Abbreviation>{func}</Abbreviation>");
    let start = match xml.find(&open) {
        Some(i) => i,
        None => panic!("{func} missing from ta_func_api.xml"),
    };
    let end = xml[start..]
        .find("</FinancialFunction>")
        .map_or(xml.len(), |i| start + i);
    let body = &xml[start..end];

    let mut blocks = Vec::new();
    for chunk in body.split("<OptionalInputArgument>").skip(1) {
        let field = |tag: &str| -> Option<String> {
            let o = format!("<{tag}>");
            let c = format!("</{tag}>");
            let i = chunk.find(&o)? + o.len();
            let j = chunk[i..].find(&c)? + i;
            Some(chunk[i..j].to_string())
        };
        let name = field("Name").unwrap_or_default();
        let trio = match (
            field("SuggestedStart"),
            field("SuggestedEnd"),
            field("SuggestedIncrement"),
        ) {
            (Some(a), Some(b), Some(c)) => Some((a, b, c)),
            _ => None,
        };
        blocks.push((name, trio));
    }
    blocks
}

/// The shipped XML must carry the declared sweep triple for every optional
/// parameter, integer and real alike.
///
/// Guarded against passing vacuously: the run asserts it actually reached
/// integer parameters, which is the path that was wrong. A refactor that
/// stopped emitting `<Range>` for integers, or a parse that silently matched
/// nothing, would otherwise leave this test green with nothing compared.
#[test]
fn xml_suggested_matches_the_declaration() {
    let xml_path = repo_root().join("ta_func_api.xml");
    let xml = std::fs::read_to_string(&xml_path)
        .unwrap_or_else(|e| panic!("read {}: {e}", xml_path.display()));

    let mut checked_int = 0usize;
    let mut checked_real = 0usize;
    let mut failures: Vec<String> = Vec::new();

    for (func, opts) in declared() {
        let blocks = xml_opt_blocks(&xml, &func);
        assert_eq!(
            blocks.len(),
            opts.len(),
            "{func}: YAML declares {} optional inputs, XML has {}",
            opts.len(),
            blocks.len()
        );

        for ((name, shown, ptype, suggested), (xml_name, trio)) in opts.into_iter().zip(blocks) {
            assert_eq!(shown, xml_name, "{func}: optional input order differs");
            let Some((sx, ex, ix)) = trio else {
                // No <Range> block: the parameter declares no range, so there
                // is no sweep to carry. Nothing to compare.
                continue;
            };
            let Some((s, e, i)) = suggested else {
                continue;
            };

            let (want, got) = match ptype {
                ParamType::Integer => {
                    checked_int += 1;
                    (
                        format!("{} {} {}", s as i32, e as i32, i as i32),
                        format!("{sx} {ex} {ix}"),
                    )
                }
                ParamType::Real => {
                    checked_real += 1;
                    // Compare numerically: the XML renders reals through
                    // double_to_str, so "0.5" and "0.500000" are the same value
                    // and only a value difference is a defect.
                    let parse = |v: &str| v.parse::<f64>().unwrap_or(f64::NAN);
                    let (ps, pe, pi) = (parse(&sx), parse(&ex), parse(&ix));
                    if (ps - s).abs() < 1e-12 && (pe - e).abs() < 1e-12 && (pi - i).abs() < 1e-12 {
                        continue;
                    }
                    (format!("{s} {e} {i}"), format!("{ps} {pe} {pi}"))
                }
                _ => continue,
            };

            if want != got {
                failures.push(format!(
                    "  {func}.{name}: YAML declares ({want}), XML emits ({got})"
                ));
            }
        }
    }

    assert!(
        checked_int > 50,
        "only {checked_int} integer parameters compared -- this gate exists for the \
         integer path (#207), so a run that barely reaches it is not testing it"
    );
    assert!(
        checked_real > 0,
        "no real parameters compared; the real path should still be sampled"
    );
    assert!(
        failures.is_empty(),
        "ta_func_api.xml disagrees with the YAML for {} parameter(s):\n{}",
        failures.len(),
        failures.join("\n")
    );
}
