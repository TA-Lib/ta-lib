//! Rule rP5 (`https://ta-lib.org/spec/inputs-outputs/#rp5`): every public batch
//! call takes its arguments in the YAML's order in all four languages. Read off
//! the committed files, so it holds the tree that ships.

use crate::common;
use std::path::{Path, PathBuf};
use ta_codegen_lib::backends::common::{camel_words, pascal_words};

fn repo_root() -> PathBuf {
    Path::new(env!("CARGO_MANIFEST_DIR")).join("../..")
}

fn read(path: &Path) -> String {
    std::fs::read_to_string(path).unwrap_or_else(|e| panic!("{}: {e}", path.display()))
}

/// The parameter lists of every declaration in `text` that starts with `head`
/// at an identifier boundary, each up to its closing `)`.
fn param_lists(text: &str, head: &str) -> Vec<Vec<String>> {
    let mut out = Vec::new();
    let mut at = 0;
    while let Some(pos) = text[at..].find(head) {
        let start = at + pos;
        at = start + head.len();
        let before = text[..start].chars().next_back();
        if before.is_some_and(|c| c.is_alphanumeric() || c == '_') {
            continue;
        }
        let close = text[at..].find(')').expect("a parameter list closes");
        let mut list = text[at..at + close].to_string();
        while let Some(open) = list.find("/*") {
            let end = list[open..].find("*/").expect("a comment closes") + open + 2;
            list.replace_range(open..end, "");
        }
        out.push(list.split(',').map(param_name).filter(|n| !n.is_empty()).collect());
    }
    out
}

/// The parameter's own name: Rust's `name: type`, else the last identifier
/// (`double inReal[]`, `ReadOnlySpan<double> inReal`, `int *outBegIdx`).
fn param_name(param: &str) -> String {
    let p = param.trim();
    if let Some((name, _)) = p.split_once(':') {
        return name.trim().to_string();
    }
    let p = p.trim_end_matches("[]").trim_end();
    if p == "&self" {
        return String::new();
    }
    p.rsplit(|c: char| !(c.is_alphanumeric() || c == '_')).next().unwrap_or("").to_string()
}

#[test]
fn every_batch_call_takes_the_yaml_argument_order_in_every_language() {
    let root = repo_root();
    let header = read(&root.join("include/ta_func.h"));
    let mut checked = 0;
    for name in common::discover_indicators() {
        let (func, _) = common::load_indicator(&name);
        let upper = func.name.to_uppercase();
        let mut want: Vec<String> = vec!["startIdx".into(), "endIdx".into()];
        want.extend(func.inputs.iter().map(|i| i.name.clone()));
        want.extend(func.optional_inputs.iter().map(|o| o.name.clone()));
        let outputs: Vec<String> = func.outputs.iter().map(|o| o.name.clone()).collect();
        let mut want_c = want.clone();
        want_c.extend(["outBegIdx".to_string(), "outNBElement".to_string()]);
        want_c.extend(outputs.iter().cloned());
        want.extend(outputs);

        let rust = read(&root.join(format!("ta_codegen/output/rust/library/src/ta_func/{name}.rs")));
        let java = read(&root.join(format!("ta_codegen/output/java/fragments/Core_{upper}.java")));
        let csharp = read(&root.join(format!("ta_codegen/output/csharp/library/src/Core_{upper}.cs")));
        let lanes = [
            ("C", param_lists(&header, &format!("TA_RetCode TA_{upper}(")), &want_c, 1),
            ("C float", param_lists(&header, &format!("TA_RetCode TA_S_{upper}(")), &want_c, 1),
            ("Rust", param_lists(&rust, &format!("pub fn {}(", func.name.to_lowercase())), &want, 1),
            ("Java", param_lists(&java, &format!("public OutRange {}(", camel_words(&func.name))), &want, 2),
            ("C#", param_lists(&csharp, &format!("public OutRange {}(", pascal_words(&func.name))), &want, 2),
        ];
        for (lang, lists, want, count) in lanes {
            assert_eq!(lists.len(), count, "{name}: {lang} has {} batch declarations, not {count}", lists.len());
            for got in &lists {
                assert_eq!(got, want, "{name}: {lang}'s batch arguments are not in the YAML's order");
                checked += 1;
            }
        }
    }
    assert!(checked >= 7 * 150, "only {checked} batch declarations checked");
}
