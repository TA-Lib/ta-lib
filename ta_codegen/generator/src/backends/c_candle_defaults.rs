//! A second copy of every candlestick body for the default range types.
//!
//! A range type cannot change during a call, yet each bar re-dispatches on it.
//! Each body that reads its settings gets the rest of its block twice: once
//! with every range type it reads replaced by the default as a literal, taken
//! only when the settings hold those defaults, and once as written. The copy
//! computes exactly what the original would, so no value can change.

use std::fmt::Write as _;

/// The default range type of each candle setting (`TA_CANDLE_DEFAULT_SETTINGS`).
const DEFAULT_RANGE: &[(&str, &str)] = &[
    ("BodyLong", "TA_RangeType_RealBody"),
    ("BodyVeryLong", "TA_RangeType_RealBody"),
    ("BodyShort", "TA_RangeType_RealBody"),
    ("BodyDoji", "TA_RangeType_HighLow"),
    ("ShadowLong", "TA_RangeType_RealBody"),
    ("ShadowVeryLong", "TA_RangeType_RealBody"),
    ("ShadowShort", "TA_RangeType_Shadows"),
    ("ShadowVeryShort", "TA_RangeType_HighLow"),
    ("Near", "TA_RangeType_HighLow"),
    ("Far", "TA_RangeType_HighLow"),
    ("Equal", "TA_RangeType_HighLow"),
];

const RANGE_TYPES: &[&str] = &["TA_RangeType_RealBody", "TA_RangeType_HighLow", "TA_RangeType_Shadows"];

fn default_range(setting: &str) -> &'static str {
    DEFAULT_RANGE
        .iter()
        .find(|(s, _)| *s == setting)
        .map(|(_, r)| *r)
        .unwrap_or_else(|| panic!("no default range type for candle setting {setting}"))
}

/// The setting a `TA_Globals->candleSettings[...]` read line unpacks, and whether
/// it is the range type.
fn settings_read(line: &str) -> Option<(&str, bool)> {
    let t = line.trim_start();
    let at = t.find(" = TA_Globals->candleSettings[TA_")?;
    let decl = &t[..at];
    let name = decl.rsplit(' ').next()?;
    let (setting, prop) = name.rsplit_once('_')?;
    Some((setting, prop == "rangeType"))
}

/// Brace depth change of one line, ignoring comments and literals. `in_comment`
/// carries a block comment across lines.
fn brace_delta(line: &str, in_comment: &mut bool) -> i32 {
    let b = line.as_bytes();
    let (mut i, mut d) = (0, 0);
    while i < b.len() {
        if *in_comment {
            if b[i] == b'*' && b.get(i + 1) == Some(&b'/') {
                *in_comment = false;
                i += 2;
                continue;
            }
            i += 1;
            continue;
        }
        match b[i] {
            b'/' if b.get(i + 1) == Some(&b'*') => {
                *in_comment = true;
                i += 2;
                continue;
            }
            b'/' if b.get(i + 1) == Some(&b'/') => break,
            b'"' | b'\'' => {
                let q = b[i];
                i += 1;
                while i < b.len() && b[i] != q {
                    i += if b[i] == b'\\' { 2 } else { 1 };
                }
            }
            b'{' => d += 1,
            b'}' => d -= 1,
            _ => {}
        }
        i += 1;
    }
    d
}

fn replace_ident(text: &str, ident: &str, with: &str) -> String {
    let mut out = String::with_capacity(text.len());
    let mut rest = text;
    while let Some(at) = rest.find(ident) {
        let before_ok = rest[..at].chars().next_back().is_none_or(|c| !(c.is_alphanumeric() || c == '_'));
        let after = &rest[at + ident.len()..];
        let after_ok = after.chars().next().is_none_or(|c| !(c.is_alphanumeric() || c == '_'));
        out.push_str(&rest[..at]);
        out.push_str(if before_ok && after_ok { with } else { ident });
        rest = after;
    }
    out.push_str(rest);
    out
}

pub fn specialize(text: &str) -> String {
    let lines: Vec<&str> = text.split_inclusive('\n').collect();
    let mut out = String::with_capacity(text.len() * 2);
    let mut i = 0;
    while i < lines.len() {
        if settings_read(lines[i]).is_none() {
            out.push_str(lines[i]);
            i += 1;
            continue;
        }
        let mut ranges: Vec<&str> = Vec::new();
        while i < lines.len() {
            let Some((setting, is_range)) = settings_read(lines[i]) else { break };
            if is_range {
                ranges.push(setting);
            }
            out.push_str(lines[i]);
            i += 1;
        }
        let start = i;
        let (mut depth, mut in_comment) = (0, false);
        while i < lines.len() {
            let d = brace_delta(lines[i], &mut in_comment);
            if depth + d < 0 {
                break;
            }
            depth += d;
            i += 1;
        }
        // Keep the checks and the lookback clamp once: copy only from the line
        // after the clamp's exit on.
        let mut start = start;
        if let Some(k) = (start..i).find(|&k| lines[k].trim_start().starts_with("if( startIdx > endIdx )")) {
            let (mut d, mut c, mut e) = (0, false, k);
            loop {
                d += brace_delta(lines[e], &mut c);
                e += 1;
                let opened = lines[k..e].iter().any(|l| l.contains('{'));
                if e >= i || (opened && d == 0) || (!opened && lines[e - 1].trim_end().ends_with(';')) {
                    break;
                }
            }
            for l in &lines[start..e] {
                out.push_str(l);
            }
            start = e;
        }
        let rest: String = lines[start..i].concat();
        let used: Vec<&str> =
            ranges.iter().copied().filter(|s| rest.contains(&format!("{s}_rangeType"))).collect();
        if used.is_empty() {
            out.push_str(&rest);
            continue;
        }
        let indent: String = lines[start..i]
            .iter()
            .find(|l| !l.trim().is_empty())
            .map_or(String::new(), |l| l.chars().take_while(|c| *c == ' ').collect());
        if let [one] = used.as_slice() {
            // One setting: a loop per range type, so every value runs folded.
            let var = format!("{one}_rangeType");
            for (k, ty) in RANGE_TYPES.iter().enumerate() {
                let kw = if k == 0 { "if" } else { "else if" };
                let _ = writeln!(out, "{indent}{kw}( {var} == {ty} )\n{indent}{{");
                out.push_str(&replace_ident(&rest, &var, ty));
                let _ = writeln!(out, "{indent}}}");
            }
            let _ = writeln!(out, "{indent}else\n{indent}{{");
            out.push_str(&rest);
            let _ = writeln!(out, "{indent}}}");
            continue;
        }
        let guard: Vec<String> =
            used.iter().map(|s| format!("{s}_rangeType == {}", default_range(s))).collect();
        let mut fast = rest.clone();
        for s in &used {
            fast = replace_ident(&fast, &format!("{s}_rangeType"), default_range(s));
        }
        let _ = writeln!(out, "{indent}if( {} )\n{indent}{{", guard.join(" && "));
        out.push_str(&fast);
        let _ = writeln!(out, "{indent}}}\n{indent}else\n{indent}{{");
        out.push_str(&rest);
        let _ = writeln!(out, "{indent}}}");
    }
    out
}
