//! Generates the Go `enums.go`: `MAType` and `FuncUnstId` from `enums.yaml`,
//! with the bounds a range check names and the size of the unstable-period table.

use crate::ir::{EnumDef, FuncDef};
use crate::registry::Lang;
use std::collections::HashMap;
use std::fmt::Write as _;
use std::path::Path;

#[allow(clippy::implicit_hasher)]
pub fn generate(funcs: &[FuncDef], enums: &HashMap<String, EnumDef>, path: &Path) {
    super::write_if_changed(path, &render(funcs, enums), "enums.go", enums.len());
}

#[allow(clippy::implicit_hasher)]
#[must_use]
pub fn render(funcs: &[FuncDef], enums: &HashMap<String, EnumDef>) -> String {
    let ma = enums.get("MAType").expect("MAType enum missing from enums.yaml");
    let fu = enums.get("FuncUnstId").expect("FuncUnstId enum missing from enums.yaml");
    let mut s = super::go::file_head();

    s.push_str("\n// MAType selects a moving average.\ntype MAType int32\n\n// The MAType members.\nconst (\n");
    for (i, v) in ma.variants.iter().enumerate() {
        assert_eq!(i32::try_from(i).unwrap(), v.value, "MAType values must be contiguous from 0 ({} breaks the sequence)", v.c_name);
    }
    s.push_str(&const_block("MAType", ma.variants.iter().map(|v| (v.name.as_str(), v.value))));
    s.push_str(")\n");
    if super::common::param_enum_names(funcs).contains(&ma.name) {
        let (lo, hi) = super::common::enum_value_bounds_of(ma).expect("a parameter enum has members");
        let (min, max) = super::common::enum_limit_names_of(ma, Lang::Go).expect("Go declares enum limits");
        let _ = write!(s, "\n// The range an MAType parameter accepts.\nconst (\n\t{min} = {lo}\n\t{max} = {hi}\n)\n");
    }

    s.push_str("\n// FuncUnstId names a function whose unstable period can be set.\ntype FuncUnstId int32\n\n// The FuncUnstId members.\nconst (\n");
    for (i, v) in fu.variants.iter().enumerate() {
        assert_eq!(i32::try_from(i).unwrap(), v.value, "FuncUnstId values must be contiguous from 0 ({} breaks the sequence)", v.c_name);
    }
    let mut members: Vec<(&str, i32)> = fu.variants.iter().filter(|v| !v.reserved).map(|v| (v.name.as_str(), v.value)).collect();
    members.push(("ALL", 65535));
    s.push_str(&const_block("FuncUnstId", members.into_iter()));
    s.push_str(")\n");
    let _ = write!(s, "\nconst funcUnstCount = {}\n", fu.variants.len());
    s.push_str(&candle_settings());
    s
}

/// `CandleSettingType`, `RangeType` and the default settings, from the C
/// library's own defaults table.
fn candle_settings() -> String {
    use super::range_type_copy::{shipped_defaults, RangeType};
    let defaults = shipped_defaults();
    let mut s = String::from(
        "\n// CandleSettingType names one candlestick threshold; C's order.\ntype CandleSettingType int32\n\n\
         // The CandleSettingType members. CandleSettingTypeAllCandleSettings selects\n\
         // every setting, for RestoreCandleDefault only.\nconst (\n",
    );
    let mut members: Vec<(&str, i32)> = defaults.iter().zip(0..).map(|((n, ..), i)| (*n, i)).collect();
    members.push(("AllCandleSettings", i32::try_from(defaults.len()).unwrap()));
    s.push_str(&const_block("CandleSettingType", members.into_iter()));
    let _ = write!(s, ")\n\nconst candleSettingCount = {}\n", defaults.len());
    s.push_str("\n// RangeType is how a candle range is measured; C's order.\ntype RangeType int32\n\n// The RangeType members.\nconst (\n");
    s.push_str(&const_block("RangeType", RangeType::ALL.iter().zip(0..).map(|(t, i)| (t.name(), i))));
    s.push_str(")\n\nvar defaultCandleSettings = [candleSettingCount]CandleSetting{\n");
    for (name, ty, period, factor) in &defaults {
        let _ = writeln!(s, "\t{{CandleSettingType{name}, RangeType{}, {period}, {factor}}},", ty.name());
    }
    s.push_str("}\n");
    s
}

/// `<Type><Name> <Type> = <value>` rows, aligned as gofmt aligns a const block.
fn const_block<'a>(ty: &str, members: impl Iterator<Item = (&'a str, i32)>) -> String {
    let rows: Vec<(String, i32)> = members.map(|(n, v)| (format!("{ty}{n}"), v)).collect();
    let width = rows.iter().map(|(n, _)| n.len()).max().unwrap_or(0);
    let mut s = String::new();
    for (n, v) in rows {
        let _ = writeln!(s, "\t{n:<width$} {ty} = {v}");
    }
    s
}
