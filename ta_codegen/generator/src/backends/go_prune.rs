//! Drops the locals an emitted Go file declares and never reads: Go rejects
//! them ("declared and not used").
//!
//! Line-based, over the emitter's own layout: one statement per line, tab
//! indentation, a declaration's scope ending where indentation drops below it.
//! It only removes, so a name it wrongly kept fails to compile. A dropped write
//! keeps its value as `_ = <value>` only when the value contains `++`, `--`,
//! `copy(`, `clear(` or an exported Core call (`c.` then an uppercase letter):
//! every other call, an unexported `c.<lower>(` included, is assumed to have no
//! side effect, and dropping one that has compiles.

const ASSIGN_OPS: &[&str] = &["=", "+=", "-=", "*=", "/=", "%=", "&=", "|=", "^=", "<<=", ">>="];

fn indent_of(line: &str) -> usize {
    line.chars().take_while(|c| *c == '\t').count()
}

fn is_ident_char(c: char) -> bool {
    c.is_ascii_alphanumeric() || c == '_'
}

/// How many times `name` appears in `line` as an identifier of its own (not a
/// field after `.`), outside a `//` comment.
fn occurrences(line: &str, name: &str) -> usize {
    let code = line.find("//").map_or(line, |i| &line[..i]);
    let b: Vec<char> = code.chars().collect();
    let n: Vec<char> = name.chars().collect();
    let mut count = 0;
    let mut i = 0;
    while i + n.len() <= b.len() {
        if b[i..i + n.len()] == n[..]
            && (i == 0 || (!is_ident_char(b[i - 1]) && b[i - 1] != '.'))
            && (i + n.len() == b.len() || !is_ident_char(b[i + n.len()]))
        {
            count += 1;
            i += n.len();
        } else {
            i += 1;
        }
    }
    count
}

/// The declared name of `var <name> <type>` or `<name> := <value>`.
fn declared(line: &str) -> Option<&str> {
    let t = line.trim_start_matches('\t');
    if let Some(rest) = t.strip_prefix("var ") {
        return rest.split(' ').next();
    }
    let (lhs, _) = t.split_once(" := ")?;
    (!lhs.is_empty() && lhs.chars().all(is_ident_char)).then_some(lhs)
}

/// The value `line` stores into `name` when the line does nothing else.
fn write_value<'a>(line: &'a str, name: &str) -> Option<&'a str> {
    let t = line.trim_start_matches('\t');
    if t == format!("{name}++") || t == format!("{name}--") {
        return Some("");
    }
    let rest = t.strip_prefix(name)?.strip_prefix(' ')?;
    let (op, value) = rest.split_once(' ')?;
    (ASSIGN_OPS.contains(&op) && occurrences(value, name) == 0).then_some(value)
}

fn is_pure(value: &str) -> bool {
    !(value.contains("copy(") || value.contains("clear(") || value.contains("++") || value.contains("--") || {
        let b = value.as_bytes();
        value.match_indices("c.").any(|(i, _)| (i == 0 || !is_ident_char(b[i - 1] as char)) && b.get(i + 2).is_some_and(u8::is_ascii_uppercase))
    })
}

/// `a, b := <value>` with every name nothing reads made `_`.
fn blank_unread_results(lines: &mut [String]) -> bool {
    for i in 0..lines.len() {
        let t = lines[i].trim_start_matches('\t');
        let Some((lhs, value)) = t.split_once(" := ") else { continue };
        if !lhs.contains(", ") || !lhs.split(", ").all(|n| !n.is_empty() && n.chars().all(is_ident_char)) {
            continue;
        }
        let depth = indent_of(&lines[i]);
        let mut end = i + 1;
        while end < lines.len() && indent_of(&lines[end]) >= depth {
            end += 1;
        }
        let names: Vec<String> = lhs
            .split(", ")
            .map(|n| if n != "_" && lines[i + 1..end].iter().all(|l| occurrences(l, n) == 0) { "_".to_string() } else { n.to_string() })
            .collect();
        if names.join(", ") != lhs {
            let op = if names.iter().all(|n| n == "_") { "=" } else { ":=" };
            lines[i] = format!("{}{} {op} {value}", "\t".repeat(depth), names.join(", "));
            return true;
        }
    }
    false
}

fn prune_once(lines: &mut Vec<String>) -> bool {
    if blank_unread_results(lines) {
        return true;
    }
    for i in 0..lines.len() {
        let Some(name) = declared(&lines[i]).map(str::to_string) else { continue };
        let depth = indent_of(&lines[i]);
        let mut end = i + 1;
        while end < lines.len() && indent_of(&lines[end]) >= depth && lines[end] != "}" {
            end += 1;
        }
        let mut writes = Vec::new();
        let mut read = false;
        for (j, line) in lines.iter().enumerate().take(end).skip(i + 1) {
            if occurrences(line, &name) == 0 {
                continue;
            }
            if write_value(line, &name).is_some() {
                writes.push(j);
            } else {
                read = true;
                break;
            }
        }
        if read {
            continue;
        }
        let mut edits: Vec<(usize, Option<String>)> = Vec::new();
        let decl_value = lines[i].split_once(" := ").map(|(_, v)| v.to_string());
        edits.push((i, decl_value.filter(|v| !is_pure(v)).map(|v| format!("{}_ = {v}", "\t".repeat(depth)))));
        for j in writes {
            let value = write_value(&lines[j], &name).unwrap_or("").to_string();
            let pad = "\t".repeat(indent_of(&lines[j]));
            edits.push((j, (!is_pure(&value)).then(|| format!("{pad}_ = {value}"))));
        }
        for (j, replacement) in edits.into_iter().rev() {
            match replacement {
                Some(r) => lines[j] = r,
                None => {
                    lines.remove(j);
                }
            }
        }
        return true;
    }
    false
}

/// `src` with every local that is never read removed, with its writes.
#[must_use]
pub fn prune(src: &str) -> String {
    let mut lines: Vec<String> = src.lines().map(str::to_string).collect();
    while prune_once(&mut lines) {}
    let mut out = lines.join("\n");
    out.push('\n');
    out
}

#[cfg(test)]
mod tests {
    use super::prune;

    #[test]
    fn an_unread_local_goes_with_its_writes() {
        let src = "func f() int32 {\n\tvar a int32\n\tvar b int32\n\tb = 2\n\tb++\n\ta = 1\n\treturn a\n}\n";
        assert_eq!(prune(src), "func f() int32 {\n\tvar a int32\n\ta = 1\n\treturn a\n}\n");
    }

    #[test]
    fn a_write_with_a_side_effect_keeps_its_value() {
        let src = "func f() {\n\tvar a int32\n\ta = c.X(1)\n}\n";
        assert_eq!(prune(src), "func f() {\n\t_ = c.X(1)\n}\n");
    }

    #[test]
    fn a_self_update_is_a_read_and_scopes_end_at_their_block() {
        let src = "func f() {\n\tif x {\n\t\tvar a int32\n\t}\n\tvar b int32\n\tb = b + 1\n\ta = 2\n}\n";
        assert_eq!(prune(src), "func f() {\n\tif x {\n\t}\n\tvar b int32\n\tb = b + 1\n\ta = 2\n}\n");
    }

    #[test]
    fn a_range_clause_or_an_if_initializer_is_no_declaration_list() {
        let src = "func f() {\n\tfor k, v := range s {\n\t\tg(v)\n\t}\n\tif _, err := h(); err != nil {\n\t\treturn\n\t}\n}\n";
        assert_eq!(prune(src), src);
    }

    #[test]
    fn removal_cascades() {
        let src = "func f() {\n\tvar a int32\n\tvar b int32\n\tb = a\n}\n";
        assert_eq!(prune(src), "func f() {\n}\n");
    }
}
