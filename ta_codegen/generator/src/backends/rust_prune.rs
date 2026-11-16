//! Tidies the locals of an emitted Rust file: drops the locals, fields and
//! stores nothing reads and the blocks that leaves empty, a `mut` nothing
//! needs, and redundant parentheses. A declaration and its initial value stay
//! as emitted.
//!
//! Line-based, over the emitter's own layout: one statement per line, space
//! indentation, a declaration's scope ending where indentation drops below it.
//! [`tidy`] refuses a file whose indentation disagrees with its braces. A
//! dropped store, a dropped parenthesis pair and a folded condition change
//! values without a compile error when misjudged, so each fires only on a form
//! it recognises and keeps anything else.

const ASSIGN_OPS: &[&str] = &["=", "+=", "-=", "*=", "/=", "%=", "&=", "|=", "^=", "<<=", ">>="];

/// Marks a line [`tidy_params`] removed, for its caller to drop.
const DROPPED: &str = "\u{0}";

/// The generated macros whose every fragment is `$x:expr`, so parentheses
/// inside their arguments group exactly as they would in a call.
const EXPR_MACROS: &[&str] = &["c_color_max", "c_color_min", "fmadd"];

/// Methods a generated body calls on a local that leave it unchanged.
const READ_ONLY_METHODS: &[&str] = &[
    "abs", "as_ptr", "ceil", "clone", "contains", "copied", "floor", "get", "is_empty", "is_finite",
    "is_nan", "iter", "len", "max", "min", "powf", "powi", "round", "signum", "sqrt", "to_vec",
    "trunc", "wrapping_add", "wrapping_mul", "wrapping_sub", "ln", "log10", "exp", "sin", "cos",
    "tan", "asin", "acos", "atan", "atan2", "sinh", "cosh", "tanh", "mul_add", "to_bits",
];

// Called on every line of every scope scan, so these stay plain byte loops.
fn indent_of(line: &str) -> usize {
    let b = line.as_bytes();
    let mut n = 0;
    while n < b.len() && b[n] == b' ' {
        n += 1;
    }
    n
}

/// `line.trim_start()`.
fn trim_lead(line: &str) -> &str {
    let t = &line[indent_of(line)..];
    match t.as_bytes().first() {
        Some(&c) if c.is_ascii() && !matches!(c, b'\t' | b'\n' | 0x0B | 0x0C | b'\r') => t,
        _ => t.trim_start(),
    }
}

/// `line.trim().is_empty()`.
fn is_blank(line: &str) -> bool {
    trim_lead(line).is_empty()
}

fn is_ident_char(c: char) -> bool {
    c.is_ascii_alphanumeric() || c == '_'
}

/// The code of `line`, without a trailing `//` comment; a `//` inside a
/// string or char literal is code, and a line holding a raw string is all code.
fn code_of(line: &str) -> &str {
    let b = line.as_bytes();
    let mut in_str = false;
    let mut i = 0;
    while i < b.len() {
        match b[i] {
            b'\\' if in_str => i += 1,
            b'"' => in_str = !in_str,
            b'r' if !in_str && matches!(b.get(i + 1), Some(b'"' | b'#')) => return line,
            b'\'' if !in_str && b.get(i + 1) == Some(&b'\\') && b.get(i + 3) == Some(&b'\'') => i += 3,
            b'\'' if !in_str && b.get(i + 2) == Some(&b'\'') => i += 2,
            b'/' if !in_str && b.get(i + 1) == Some(&b'/') => return &line[..i],
            _ => {}
        }
        i += 1;
    }
    line
}

/// Byte offsets where `name` appears in `line` as an identifier of its own
/// (not a field after `.`), outside a `//` comment.
fn positions(line: &str, name: &str) -> Vec<usize> {
    if !line.contains(name) {
        return Vec::new();
    }
    let code = code_of(line);
    let b = code.as_bytes();
    let mut out = Vec::new();
    for (i, _) in code.match_indices(name) {
        let before_ok = i == 0 || {
            let c = b[i - 1] as char;
            let range = c == '.' && i >= 2 && b[i - 2] == b'.';
            !is_ident_char(c) && (c != '.' || range) && c != '\''
        };
        let after_ok = b.get(i + name.len()).is_none_or(|c| !is_ident_char(*c as char));
        if before_ok && after_ok {
            out.push(i);
        }
    }
    out
}

fn occurs(line: &str, name: &str) -> bool {
    !positions(line, name).is_empty()
}

struct Decl<'a> {
    name: &'a str,
    mutable: bool,
    /// The initial value, `None` for `let x: T;`.
    init: Option<&'a str>,
}

/// `let [mut] <name>[: T][ = <value>];` on one line.
fn declared(line: &str) -> Option<Decl<'_>> {
    let t = trim_lead(line).strip_prefix("let ")?;
    let (mutable, t) = match t.strip_prefix("mut ") {
        Some(rest) => (true, rest),
        None => (false, t),
    };
    let body = t.strip_suffix(';')?;
    let name_end = body.find(|c: char| !is_ident_char(c)).unwrap_or(body.len());
    let name = &body[..name_end];
    if name.is_empty() || name == "_" || name.starts_with(|c: char| c.is_ascii_digit()) {
        return None;
    }
    let rest = &body[name_end..];
    let (ty, init) = match rest.split_once(" = ") {
        Some((ty, init)) => (ty, Some(init)),
        None => (rest, None),
    };
    if !(ty.is_empty() || ty.starts_with(": ")) || ty.contains('=') {
        return None;
    }
    Some(Decl { name, mutable, init })
}

/// The value `line` stores into `name` when the line does nothing else.
fn write_value<'a>(line: &'a str, name: &str) -> Option<(&'a str, &'a str)> {
    let t = trim_lead(line);
    let rest = t.strip_prefix(name)?.strip_prefix(' ')?;
    let (op, value) = rest.split_once(' ')?;
    let value = value.strip_suffix(';')?;
    (ASSIGN_OPS.contains(&op) && !occurs(value, name)).then_some((op, value))
}

/// The value `line` stores into `name` when the line does nothing else, its
/// own old value allowed: what a store to drop may read.
fn store_value<'a>(line: &'a str, name: &str) -> Option<&'a str> {
    let rest = trim_lead(line).strip_prefix(name)?.strip_prefix(' ')?;
    let (op, value) = rest.split_once(' ')?;
    ASSIGN_OPS.contains(&op).then_some(())?;
    value.strip_suffix(';')
}

/// Whether evaluating `value` only produces it: `false` for a call that is
/// not a known pure method, a mutable borrow, `?`, or a macro.
fn is_pure(value: &str) -> bool {
    // An empty slice's borrow, the placeholder a slice local starts from.
    if value == "&mut []" {
        return true;
    }
    let b = value.as_bytes();
    // A `!` after a name is a macro call; anywhere else it negates.
    let macro_call = value.match_indices('!').any(|(i, _)| i > 0 && is_ident_char(b[i - 1] as char) && b.get(i + 1) != Some(&b'='));
    if value.contains('?') || value.contains("&mut") || macro_call {
        return false;
    }
    for (i, _) in value.match_indices('(') {
        let start = value[..i].rfind(|c: char| !is_ident_char(c)).map_or(0, |p| p + 1);
        if start == i {
            continue; // grouping parenthesis
        }
        let callee = &value[start..i];
        let is_method = start > 0 && b[start - 1] == b'.';
        let is_path = value[..start].ends_with("::");
        let pure_path = is_path && (READ_ONLY_METHODS.contains(&callee) || callee == "new");
        if !(is_method && READ_ONLY_METHODS.contains(&callee) || pure_path) {
            return false;
        }
    }
    true
}

/// The end (exclusive) of the scope a declaration on line `i` opens.
fn scope_end(lines: &[String], i: usize) -> usize {
    let depth = indent_of(&lines[i]);
    let mut end = i + 1;
    while end < lines.len() && (is_blank(&lines[end]) || indent_of(&lines[end]) >= depth) {
        end += 1;
    }
    end
}

/// Whether the occurrence of `name` at byte `at` of `line` can change it.
fn is_write_at(line: &str, at: usize, name: &str) -> bool {
    let code = code_of(line);
    if code[..at].trim_end().ends_with("&mut") {
        return true;
    }
    let mut rest = &code[at + name.len()..];
    loop {
        if let Some(r) = rest.strip_prefix('[') {
            let mut depth = 1;
            let mut end = None;
            for (k, c) in r.char_indices() {
                match c {
                    '[' => depth += 1,
                    ']' => {
                        depth -= 1;
                        if depth == 0 {
                            end = Some(k);
                            break;
                        }
                    }
                    _ => {}
                }
            }
            let Some(end) = end else { return true };
            rest = &r[end + 1..];
        } else if let Some(r) = rest.strip_prefix('.') {
            let id_end = r.find(|c: char| !is_ident_char(c)).unwrap_or(r.len());
            let id = &r[..id_end];
            let after = &r[id_end..];
            if after.starts_with('(') {
                return !READ_ONLY_METHODS.contains(&id);
            }
            rest = after;
        } else {
            break;
        }
    }
    let rest = rest.trim_start();
    ASSIGN_OPS.iter().any(|op| rest.strip_prefix(op).is_some_and(|r| r.starts_with(' ')))
}

/// Line `j` joined with the lines that continue its statement.
fn statement_at(lines: &[String], j: usize) -> String {
    let mut stmt = code_of(&lines[j]).trim_end().to_string();
    let mut k = j + 1;
    while k < lines.len() && !stmt.ends_with([';', '{', '}', ',']) {
        stmt.push_str(code_of(&lines[k]).trim());
        k += 1;
    }
    stmt
}

/// The next line at or after `k` that is not blank.
fn next_code(lines: &[String], mut k: usize, end: usize) -> usize {
    while k < end && is_blank(&lines[k]) {
        k += 1;
    }
    k
}

/// The end (exclusive) of the statement opening at `k`: the line, every
/// deeper line under it, and an `else` or `}` closing it at its own depth.
fn statement_end(lines: &[String], k: usize, end: usize) -> usize {
    let depth = indent_of(&lines[k]);
    let mut e = k + 1;
    loop {
        while e < end && (is_blank(&lines[e]) || indent_of(&lines[e]) > depth) {
            e += 1;
        }
        if e < end && indent_of(&lines[e]) == depth && trim_lead(&lines[e]).starts_with('}') {
            let closes = lines[e].trim_end().ends_with('{');
            e += 1;
            if closes {
                continue;
            }
        }
        return e;
    }
}

/// The label of the loop line `t` opens, `""` when unlabelled; `None` when
/// `t` opens no loop.
fn loop_label(t: &str) -> Option<&str> {
    let (label, rest) = match t.strip_prefix('\'') {
        Some(r) => {
            let end = r.find(|c: char| !is_ident_char(c))?;
            (&r[..end], r[end..].strip_prefix(':')?.trim_start())
        }
        None => ("", t),
    };
    let word = &rest[..rest.find(|c: char| !is_ident_char(c)).unwrap_or(rest.len())];
    matches!(word, "while" | "for" | "loop").then_some(label)
}

/// Whether line `k` opens a `for name in` loop, whose binding shadows the
/// local `name` in its body: `Some(true)` when its header reads the local.
fn shadowing_for(lines: &[String], k: usize, name: &str) -> Option<bool> {
    fn after_for<'a>(t: &'a str, name: &str) -> Option<&'a str> {
        t.strip_prefix("for ")?.strip_prefix(name)?.strip_prefix(" in ")
    }
    after_for(trim_lead(&lines[k]), name)?;
    let header = statement_at(lines, k);
    let iter = after_for(header.trim_start(), name).unwrap_or(&header);
    Some(occurs(iter, name))
}

/// The lines in `from..end` that use the local `name`, leaving out the
/// blocks of `for name in ..` loops, whose binding shadows it.
fn uses_in(lines: &[String], from: usize, end: usize, name: &str) -> Vec<usize> {
    let mut out = Vec::new();
    let mut j = from;
    while j < end {
        if let Some(header_reads) = shadowing_for(lines, j, name) {
            if header_reads {
                out.push(j);
            }
            j = statement_end(lines, j, end);
            continue;
        }
        if occurs(&lines[j], name) {
            out.push(j);
        }
        j += 1;
    }
    out
}

/// Whether the plain stores to an uninitialized local at `writes` can each
/// run at most once: none sits in a loop opened after the declaration on line
/// `decl`, and any two lie in different branches of one `if` chain or `match`.
fn stores_once(lines: &[String], decl: usize, writes: &[usize]) -> bool {
    for &w in writes {
        let mut depth = indent_of(&lines[w]);
        for k in (decl + 1..w).rev() {
            if is_blank(&lines[k]) || indent_of(&lines[k]) >= depth {
                continue;
            }
            depth = indent_of(&lines[k]);
            if loop_label(trim_lead(&lines[k])).is_some() {
                return false;
            }
        }
    }
    writes.windows(2).all(|p| {
        let between: Vec<&str> = (p[0] + 1..p[1]).map(|k| lines[k].as_str()).filter(|l| !l.trim().is_empty()).collect();
        let Some(m) = between.iter().map(|l| indent_of(l)).min() else { return false };
        let seams: Vec<&str> = between.iter().filter(|l| indent_of(l) == m).map(|l| l.trim()).collect();
        let is_seam = |t: &str| t == "}" || t == "} else {" || (t.starts_with("} else if ") || t.ends_with("=> {")) && t.ends_with('{');
        m < indent_of(lines[p[0]].as_str()).min(indent_of(lines[p[1]].as_str()))
            && seams.iter().all(|t| is_seam(t))
            && seams.iter().any(|t| *t != "}")
    })
}

/// Drops `mut` from each parameter of the `fn` signature opening on line `i`
/// that its body never writes.
fn tidy_params(lines: &mut [String], i: usize) -> bool {
    let t = trim_lead(&lines[i]);
    if !(t.starts_with("fn ") || t.starts_with("pub fn ") || t.starts_with("pub(crate) fn ") || t.starts_with("pub(super) fn ")) {
        return false;
    }
    let Some(open) = (i..lines.len().min(i + 64)).find(|&k| code_of(&lines[k]).trim_end().ends_with('{')) else { return false };
    let depth = indent_of(&lines[i]);
    let body_end = (open + 1..lines.len()).find(|&k| !is_blank(&lines[k]) && indent_of(&lines[k]) <= depth).unwrap_or(lines.len());
    let body_depth = indent_of(&lines[next_code(lines, open + 1, body_end).min(lines.len() - 1)]);
    let mut changed = false;
    // A parameter its function never uses keeps its slot in a shared calling
    // convention: name it as unused. Only a `pub fn`'s names are documented
    // API, so every other visibility gets the rename.
    if !t.starts_with("pub fn ") {
        for k in i..=open {
            let sig = code_of(&lines[k]).to_string();
            let mut fixed = lines[k].clone();
            for (at, _) in sig.match_indices(": ").collect::<Vec<_>>().into_iter().rev() {
                let start = sig[..at].rfind(|c: char| !is_ident_char(c)).map_or(0, |p| p + 1);
                let pname = &sig[start..at];
                let before = sig[..start].trim_end();
                let is_param = before.ends_with('(') || before.ends_with(',') || before.is_empty() || before.ends_with("mut");
                if pname.is_empty() || pname.starts_with('_') || pname == "self" || !is_param || before.ends_with("mut") {
                    continue;
                }
                if !(open + 1..body_end).any(|j| occurs(&lines[j], pname)) && !(i..=open).any(|j| j != k && occurs(&lines[j], pname)) {
                    fixed.insert(start, '_');
                }
            }
            if fixed != lines[k] {
                lines[k] = fixed;
                changed = true;
            }
        }
        if changed {
            return true;
        }
    }
    for k in i..=open {
        let sig = lines[k].clone();
        let mut fixed = sig.clone();
        for (at, _) in sig.match_indices("mut ").collect::<Vec<_>>().into_iter().rev() {
            if at > 0 && (sig.as_bytes()[at - 1] == b'&' || is_ident_char(sig.as_bytes()[at - 1] as char)) {
                continue;
            }
            let rest = &sig[at + 4..];
            let n_end = rest.find(|c: char| !is_ident_char(c)).unwrap_or(rest.len());
            let pname = &rest[..n_end];
            if pname.is_empty() || !rest[n_end..].starts_with(':') {
                continue;
            }
            let redecl = (open + 1..body_end).find(|&j| {
                indent_of(&lines[j]) == body_depth && {
                    let l = trim_lead(&lines[j]);
                    let l = l.strip_prefix("let ").map(|r| r.strip_prefix("mut ").unwrap_or(r));
                    l.and_then(|r| r.strip_prefix(pname)).is_some_and(|r| r.starts_with([' ', ':']))
                }
            });
            let scan_end = redecl.unwrap_or(body_end);
            if let Some(w) = uses_in(lines, open + 1, scan_end, pname).into_iter().find(|&j| {
                write_value(&lines[j], pname).is_some_and(|(_, v)| is_pure(v)) && dead_store(lines, open, j, scan_end, pname)
            }) {
                lines[w] = DROPPED.to_string();
                return true;
            }
            let written = (open + 1..scan_end).any(|j| {
                occurs(&lines[j], pname) && {
                    let stmt = statement_at(lines, j);
                    positions(&stmt, pname).into_iter().any(|p| is_write_at(&stmt, p, pname))
                }
            });
            if !written {
                fixed.replace_range(at..at + 4, "");
            }
        }
        if fixed != sig {
            lines[k] = fixed;
            changed = true;
        }
    }
    changed
}

/// The plain `}` closing the `if` chain whose arm opens on line `k` with
/// `} else`, when every later arm is `} else ... {` or a wrapped `} else if`
/// whose condition lines sit deeper and whose `{` sits alone at its depth.
fn chain_end(lines: &[String], k: usize, end: usize) -> Option<usize> {
    let d = indent_of(&lines[k]);
    let mut j = k;
    loop {
        let a = code_of(&lines[j]).trim();
        if !a.starts_with("} else") || a.ends_with('}') {
            return None;
        }
        let mut open = a.ends_with('{');
        j += 1;
        loop {
            while j < end && (is_blank(&lines[j]) || indent_of(&lines[j]) > d) {
                j += 1;
            }
            if j >= end || indent_of(&lines[j]) != d {
                return None;
            }
            if open || lines[j].trim() != "{" {
                break;
            }
            open = true;
            j += 1;
        }
        if !open {
            return None;
        }
        if lines[j].trim() == "}" {
            return Some(j);
        }
    }
}

/// The first line after `w` in `..end` that uses `name` and can run after
/// it: leaving a branch skips the rest of its `if` chain, when [`chain_end`]
/// finds where it closes.
fn next_reachable_use(lines: &[String], w: usize, end: usize, name: &str) -> Option<usize> {
    let mut floor = indent_of(&lines[w]);
    let mut k = w + 1;
    while k < end {
        if is_blank(&lines[k]) {
            k += 1;
            continue;
        }
        let d = indent_of(&lines[k]);
        if d < floor {
            floor = d;
            if let Some(j) = chain_end(lines, k, end) {
                k = j + 1;
                continue;
            }
        }
        if let Some(header_reads) = shadowing_for(lines, k, name) {
            if header_reads {
                return Some(k);
            }
            k = statement_end(lines, k, end);
            continue;
        }
        if occurs(&lines[k], name) {
            return Some(k);
        }
        k += 1;
    }
    None
}

/// Whether each `break` or `continue` on line `k` leaves for a point before
/// line `n`: its loop closes before `n`. A loop around the store that jumps
/// back is [`dead_store`]'s own check. A jump on a line that also opens a
/// loop, or whose loop is not found, may skip `n`.
fn jumps_stay_before(lines: &[String], k: usize, n: usize) -> bool {
    let code = code_of(&lines[k]);
    let jumps: Vec<usize> = ["break", "continue"].iter().flat_map(|kw| positions(code, kw).into_iter().map(|at| at + kw.len())).collect();
    if jumps.is_empty() {
        return true;
    }
    if ["while", "for", "loop"].iter().any(|kw| occurs(code, kw)) {
        return false;
    }
    jumps.into_iter().all(|after| {
        let label = code[after..].trim_start().strip_prefix('\'').map(|r| &r[..r.find(|c: char| !is_ident_char(c)).unwrap_or(r.len())]);
        let mut depth = indent_of(&lines[k]);
        let target = (0..k).rev().find(|&j| {
            if is_blank(&lines[j]) || indent_of(&lines[j]) >= depth {
                return false;
            }
            depth = indent_of(&lines[j]);
            loop_label(trim_lead(&lines[j])).is_some_and(|l| label.is_none_or(|want| l == want))
        });
        target.is_some_and(|j| statement_end(lines, j, lines.len()) <= n)
    })
}

/// Whether the pure store on line `w` to the local declared on line `decl`
/// is never read: what follows it in `..end` names the local only to
/// overwrite it first, and no loop around it, opened after the declaration,
/// reads it on another pass.
fn dead_store(lines: &[String], decl: usize, w: usize, end: usize, name: &str) -> bool {
    // The next use, when it overwrites the local on every path out of `w`:
    // a plain store in `w`'s block or one enclosing it, with no jump between.
    let next = next_reachable_use(lines, w, end, name);
    let until = match next {
        None => end,
        Some(n) => {
            let overwrites = matches!(write_value(&lines[n], name), Some(("=", _)));
            let d = indent_of(&lines[n]);
            let reached = d <= indent_of(&lines[w])
                && (w + 1..n).all(|k| is_blank(&lines[k]) || indent_of(&lines[k]) >= d)
                && (w + 1..n).all(|k| jumps_stay_before(lines, k, n));
            if !(overwrites && reached) {
                return false;
            }
            n
        }
    };
    let mut depth = indent_of(&lines[w]);
    for k in (decl + 1..w).rev() {
        if is_blank(&lines[k]) || indent_of(&lines[k]) >= depth {
            continue;
        }
        depth = indent_of(&lines[k]);
        if loop_label(trim_lead(&lines[k])).is_some() {
            let e = statement_end(lines, k, end);
            if e > until {
                // The loop holds the overwrite too, so every pass reaches it first.
                break;
            }
            let reads = uses_in(lines, k, e, name).into_iter().any(|j| j != w && !matches!(write_value(&lines[j], name), Some(("=", _))));
            if reads {
                return false;
            }
        }
    }
    true
}

/// `inner` split at its top-level commas.
fn split_top(inner: &str) -> Vec<&str> {
    let mut out = Vec::new();
    let (mut depth, mut start) = (0i32, 0);
    for (k, c) in inner.char_indices() {
        match c {
            '(' | '[' | '{' => depth += 1,
            ')' | ']' | '}' => depth -= 1,
            ',' if depth == 0 => {
                out.push(&inner[start..k]);
                start = k + 1;
            }
            _ => {}
        }
    }
    out.push(&inner[start..]);
    out
}

/// The value a struct-literal entry gives `field`: `field` or `field: value`.
fn entry_value<'a>(entry: &'a str, field: &str) -> Option<&'a str> {
    let e = entry.trim();
    if e == field {
        return Some(e);
    }
    e.strip_prefix(field)?.strip_prefix(": ")
}

/// Drops one field of a private struct that nothing reads, with the entry
/// every literal of the struct gives it; `false` when there is none left.
fn drop_unread_field(lines: &mut Vec<String>) -> bool {
    for k in 0..lines.len() {
        let Some(sname) = lines[k].strip_prefix("struct ").and_then(|r| r.strip_suffix(" {")) else { continue };
        let close = (k + 1..lines.len()).find(|&j| lines[j] == "}").unwrap_or(lines.len());
        for f in k + 1..close {
            let Some((field, _)) = lines[f].trim().strip_suffix(',').and_then(|d| d.split_once(": ")) else { continue };
            if field.is_empty() || !field.chars().all(is_ident_char) {
                continue;
            }
            let dotted = format!(".{field}");
            let read = lines.iter().any(|l| {
                code_of(l).match_indices(&dotted).any(|(at, _)| l.as_bytes().get(at + dotted.len()).is_none_or(|c| !is_ident_char(*c as char)))
            });
            if read {
                continue;
            }
            // Every literal's entry for the field, as (line, Some(index of entry) when one-line).
            let open = format!("{sname} {{");
            let mut edits: Vec<(usize, Option<usize>)> = Vec::new();
            let mut ok = true;
            for (j, l) in lines.iter().enumerate() {
                let Some(at) = l.find(&open) else { continue };
                if j == k {
                    continue;
                }
                let after = &l[at + open.len()..];
                if after.trim().is_empty() {
                    let pad = indent_of(l);
                    let end = (j + 1..lines.len()).find(|&e| indent_of(&lines[e]) <= pad && trim_lead(&lines[e]).starts_with('}')).unwrap_or(lines.len());
                    let hit = (j + 1..end).find(|&e| entry_value(lines[e].trim().trim_end_matches(','), field).is_some());
                    match hit {
                        Some(e) if entry_value(lines[e].trim().trim_end_matches(','), field).is_some_and(is_pure) => edits.push((e, None)),
                        _ => ok = false,
                    }
                } else if let Some(close_at) = after.rfind('}') {
                    let entries = split_top(&after[..close_at]);
                    match entries.iter().position(|e| entry_value(e, field).is_some()) {
                        Some(n) if entry_value(entries[n], field).is_some_and(is_pure) => edits.push((j, Some(n))),
                        _ => ok = false,
                    }
                } else {
                    ok = false;
                }
            }
            if !ok {
                continue;
            }
            for (j, n) in edits.into_iter().rev() {
                match n {
                    None => {
                        lines.remove(j);
                    }
                    Some(n) => {
                        let at = lines[j].find(&open).unwrap_or(0) + open.len();
                        let close_at = at + lines[j][at..].rfind('}').unwrap_or(0);
                        let kept: Vec<String> = split_top(&lines[j][at..close_at])
                            .into_iter()
                            .enumerate()
                            .filter(|(m, e)| *m != n && !e.trim().is_empty())
                            .map(|(_, e)| e.trim().to_string())
                            .collect();
                        lines[j] = format!("{} {}, {}", &lines[j][..at], kept.join(", "), &lines[j][close_at..]);
                    }
                }
            }
            lines.remove(f);
            return true;
        }
    }
    false
}

/// `line` with string and char literal contents and any comment blanked to
/// spaces, so its brackets can be matched by position.
fn masked(line: &str) -> Vec<u8> {
    let b = line.as_bytes();
    let mut out = b.to_vec();
    let mut i = 0;
    let mut in_str = false;
    while i < b.len() {
        let c = b[i];
        if in_str {
            out[i] = b' ';
            if c == b'\\' && i + 1 < b.len() {
                out[i + 1] = b' ';
                i += 1;
            } else if c == b'"' {
                in_str = false;
            }
        } else if c == b'"' {
            in_str = true;
        } else if c == b'/' && matches!(b.get(i + 1), Some(b'/' | b'*')) {
            out[i..].fill(b' ');
            break;
        } else if c == b'\'' && b.get(i + 2) == Some(&b'\'') {
            out[i + 1] = b' ';
            i += 2;
        }
        i += 1;
    }
    out
}

/// The index of the bracket closing the one at `open`, on the same line.
fn closing(m: &[u8], open: usize) -> Option<usize> {
    let mut depth = 0i32;
    for (k, &c) in m.iter().enumerate().skip(open) {
        match c {
            b'(' | b'[' | b'{' => depth += 1,
            b')' | b']' | b'}' => {
                depth -= 1;
                if depth == 0 {
                    return Some(k);
                }
            }
            _ => {}
        }
    }
    None
}

/// The whole of `t` is one plain decimal literal, its type suffix allowed.
fn is_number(t: &str) -> bool {
    let digits = t.find(|c: char| !(c.is_ascii_digit() || c == '.')).unwrap_or(t.len());
    let suffix = t[digits..].strip_prefix('_').unwrap_or(&t[digits..]);
    digits > 0
        && t.as_bytes()[0].is_ascii_digit()
        && ["", "f64", "f32", "usize", "isize", "i32", "i64", "u32", "u64", "u8"].contains(&suffix)
}

/// Whether `inner` is one operand that no operator could split: a name or
/// path, a number, or either followed by indexes, fields and calls, or a
/// dereferenced name.
fn is_atom(ms: &str, from: usize, to: usize) -> bool {
    let m = ms.as_bytes();
    let mut k = from;
    if m.get(k) == Some(&b'*') {
        k += 1;
    }
    let start = k;
    let text = &ms[from..to];
    if text.trim() != text || text.is_empty() {
        return false;
    }
    if is_number(text) {
        return true;
    }
    while k < to {
        let c = m[k];
        if c.is_ascii_alphanumeric() || c == b'_' || c == b'.' && k > start {
            k += 1;
        } else if c == b':' && m.get(k + 1) == Some(&b':') {
            k += 2;
        } else if c == b'!' && m.get(k + 1) == Some(&b'(') {
            k += 1;
        } else if matches!(c, b'(' | b'[') && k > start {
            let Some(e) = closing(m, k) else { return false };
            if e >= to {
                return false;
            }
            k = e + 1;
        } else {
            return false;
        }
    }
    k > start && (m[start].is_ascii_alphabetic() || m[start] == b'_')
}

/// Whether the parenthesis pair at `open`..`close` of `m` can go without
/// changing how the line parses.
fn redundant_parens(ms: &str, open: usize, close: usize) -> bool {
    let m = ms.as_bytes();
    let before = &m[..open];
    let prev = before.last().copied();
    // A call's, a macro's or a generic's own parenthesis.
    if prev.is_some_and(|c| c.is_ascii_alphanumeric() || matches!(c, b'_' | b')' | b']' | b'>'))
        || prev == Some(b'!') && before.len() >= 2 && (before[before.len() - 2].is_ascii_alphanumeric() || before[before.len() - 2] == b'_')
    {
        return false;
    }
    let inner = &m[open + 1..close];
    // `()` is the unit value, not a grouping.
    if inner.iter().all(u8::is_ascii_whitespace) {
        return false;
    }
    let next = m.get(close + 1).copied();
    let trimmed_before = ms[..open].trim_end();
    let after = ms[close + 1..].trim_start();
    let top_comma = {
        let mut depth = 0i32;
        inner.iter().any(|&c| {
            match c {
                b'(' | b'[' | b'{' => depth += 1,
                b')' | b']' | b'}' => depth -= 1,
                _ => {}
            }
            c == b',' && depth == 0
        })
    };
    if top_comma || inner.contains(&b'{') && !trimmed_before.ends_with(['(', ',', '[']) && !trimmed_before.ends_with('=') {
        return false;
    }
    if is_atom(ms, open + 1, close) {
        let num = is_number(&ms[open + 1..close]);
        let deref = inner.first() == Some(&b'*');
        let calls = next == Some(b'(');
        let member = matches!(next, Some(b'.' | b'['));
        return !calls && !((num || deref) && member);
    }
    // The whole of a list element, an index, an assignment's value, or a
    // condition or scrutinee.
    let element = trimmed_before.ends_with(['(', ',', '[']) && after.starts_with([')', ',', ']']);
    let assigned = (trimmed_before.ends_with('=')
        && !trimmed_before.ends_with("==")
        && !trimmed_before.ends_with("!=")
        && !trimmed_before.ends_with("<=")
        && !trimmed_before.ends_with(">="))
        || trimmed_before.ends_with(" return")
        || trimmed_before == "return";
    let assigned = assigned && after.starts_with(';');
    let keyword = |k: &str| trimmed_before.strip_suffix(k).is_some_and(|p| p.is_empty() || p.ends_with(' '));
    let condition = (keyword("if") || keyword("while") || keyword("match")) && after.starts_with('{');
    element || assigned || condition
}

/// `line` with every parenthesis pair [`redundant_parens`] accepts removed.
fn drop_parens(line: &str) -> String {
    let mut cur = line.to_string();
    loop {
        // Masking blanks whole characters, so `ms` stays valid UTF-8.
        let ms = String::from_utf8(masked(&cur)).expect("masking keeps UTF-8");
        let m = ms.as_bytes();
        // A macro's arguments are tokens whose grouping its rules may match,
        // except in the generated macros that take only `$x:expr` fragments.
        let opaque_macro = |k: usize| {
            if k < 2 || m[k - 1] != b'!' {
                return false;
            }
            let start = m[..k - 1].iter().rposition(|c| !(c.is_ascii_alphanumeric() || *c == b'_')).map_or(0, |p| p + 1);
            start < k - 1 && !EXPR_MACROS.contains(&&ms[start..k - 1])
        };
        // Each opener's closer counts every bracket kind, as [`closing`] does.
        let mut close_of = vec![usize::MAX; m.len()];
        let mut in_macro = vec![false; m.len()];
        let mut stack: Vec<(usize, bool)> = Vec::new();
        let mut opaque_open = 0usize;
        for k in 0..m.len() {
            in_macro[k] = opaque_open > 0;
            match m[k] {
                b'(' | b'[' | b'{' => {
                    let opaque = opaque_macro(k);
                    opaque_open += usize::from(opaque);
                    stack.push((k, opaque));
                }
                b')' | b']' | b'}' => {
                    if let Some((o, opaque)) = stack.pop() {
                        close_of[o] = k;
                        opaque_open -= usize::from(opaque);
                    }
                }
                _ => {}
            }
        }
        let found = (0..m.len()).filter(|&k| m[k] == b'(' && !in_macro[k]).find_map(|o| {
            let c = close_of[o];
            (c != usize::MAX && m[c] == b')' && redundant_parens(&ms, o, c)).then_some((o, c))
        });
        let Some((o, c)) = found else { return cur };
        cur.remove(c);
        cur.remove(o);
    }
}

/// Drops `lines[i]`, and the `#[allow(..)]` lines that annotate it.
fn remove_with_attrs(lines: &mut Vec<String>, i: usize) {
    lines.remove(i);
    let mut k = i;
    while k > 0 && trim_lead(&lines[k - 1]).starts_with("#[allow(") {
        lines.remove(k - 1);
        k -= 1;
    }
}

/// Drops an empty `else` arm, or an empty `if` with a pure condition and no
/// `else`, opening on line `k`.
fn drop_empty_block(lines: &mut Vec<String>, k: usize) -> bool {
    let Some(next) = lines.get(k + 1) else { return false };
    let (pad, t) = (indent_of(&lines[k]), lines[k].trim());
    if indent_of(next) != pad || next.trim() != "}" {
        return false;
    }
    if t == "} else {" {
        lines.remove(k);
        return true;
    }
    if let Some(cond) = t.strip_prefix("if ").and_then(|c| c.strip_suffix(" {")) {
        if is_pure(cond) && !cond.starts_with("let ") {
            lines.drain(k..=k + 1);
            return true;
        }
    }
    false
}

/// `if a == b {}` directly followed by `} else if c {`, the last arm:
/// `if a != b && c {`. A later arm would then also run when `a == b`.
fn fold_empty_then(lines: &mut Vec<String>, k: usize) -> bool {
    let Some(next) = lines.get(k + 1) else { return false };
    let pad = indent_of(&lines[k]);
    let Some(cond) = lines[k].trim().strip_prefix("if ").and_then(|c| c.strip_suffix(" {")) else { return false };
    let Some(other) = next.trim().strip_prefix("} else if ").and_then(|c| c.strip_suffix(" {")) else { return false };
    if indent_of(next) != pad {
        return false;
    }
    let arm_end = (k + 2..lines.len()).find(|&j| !is_blank(&lines[j]) && indent_of(&lines[j]) <= pad);
    if arm_end.is_none_or(|j| {
        indent_of(&lines[j]) != pad
            || lines[j].trim() != "}"
            || (j + 1..lines.len()).find(|&m| !code_of(&lines[m]).trim().is_empty()).is_some_and(|m| trim_lead(&lines[m]).starts_with("else"))
    }) {
        return false;
    }
    let Some((l, r)) = cond.split_once(" == ") else { return false };
    let plain = |x: &str| !x.contains("==") && !x.contains("&&") && !x.contains("||") && !x.contains('!') && !x.contains('<') && !x.contains('>');
    if !(plain(l) && plain(r)) {
        return false;
    }
    let other = if other.contains("||") { format!("({other})") } else { other.to_string() };
    lines[k] = format!("{}if {l} != {r} && {other} {{", " ".repeat(pad));
    lines.remove(k + 1);
    true
}

/// Makes the first edit at or after line `from`, returning where to resume.
fn tidy_from(lines: &mut Vec<String>, from: usize) -> Option<usize> {
    for i in from..lines.len() {
        if drop_empty_block(lines, i) || fold_empty_then(lines, i) {
            return Some(i.saturating_sub(2));
        }
        if tidy_params(lines, i) {
            lines.retain(|l| l != DROPPED);
            return Some(i);
        }
        let Some(d) = declared(&lines[i]) else { continue };
        let (name, mutable) = (d.name.to_string(), d.mutable);
        let init = d.init.map(str::to_string);
        let end = scope_end(lines, i);
        let uses = uses_in(lines, i + 1, end, &name);

        // Never read: drop it and its pure writes.
        let writes: Vec<usize> = uses.iter().copied().filter(|&j| write_value(&lines[j], &name).is_some()).collect();
        if writes.len() == uses.len()
            && init.as_deref().is_none_or(is_pure)
            && writes.iter().all(|&j| is_pure(write_value(&lines[j], &name).unwrap().1))
        {
            for &j in writes.iter().rev() {
                lines.remove(j);
            }
            remove_with_attrs(lines, i);
            return Some(i.saturating_sub(2));
        }

        // A copy of a binding of the same name, which it can only shadow.
        if !mutable && init.as_deref() == Some(name.as_str()) {
            remove_with_attrs(lines, i);
            return Some(i.saturating_sub(2));
        }

        // A store nothing reads.
        if let Some(&w) = uses.iter().find(|&&j| {
            store_value(&lines[j], &name).is_some_and(is_pure) && dead_store(lines, i, j, end, &name)
        }) {
            lines.remove(w);
            return Some(i);
        }

        // An initial value no path reads stays: removing it changes how rustc
        // lowers the declaration, and LLVM then allocates registers differently
        // with no instruction saved, slower on some functions.

        // `mut` nothing needs.
        // An uninitialized one may need it for a write in a loop.
        if mutable
            && init.is_some()
            && !uses.iter().any(|&j| {
                let stmt = statement_at(lines, j);
                positions(&stmt, &name).into_iter().any(|at| is_write_at(&stmt, at, &name))
            })
        {
            lines[i] = lines[i].replacen("let mut ", "let ", 1);
            return Some(i);
        }
        if mutable && init.is_none() {
            let plain = uses.iter().all(|&j| {
                write_value(&lines[j], &name).is_some_and(|(op, _)| op == "=") || {
                    let stmt = statement_at(lines, j);
                    !positions(&stmt, &name).into_iter().any(|at| is_write_at(&stmt, at, &name))
                }
            });
            let writes: Vec<usize> = uses.iter().copied().filter(|&j| write_value(&lines[j], &name).is_some()).collect();
            if plain && !writes.is_empty() && stores_once(lines, i, &writes) {
                lines[i] = lines[i].replacen("let mut ", "let ", 1);
                return Some(i);
            }
        }
    }
    None
}

/// `line`'s code with string and char literals and any comment blanked out,
/// so only real braces remain.
fn braces_of(line: &str) -> String {
    let b: Vec<char> = line.chars().collect();
    let mut out = String::new();
    let mut i = 0;
    let mut in_str = false;
    while i < b.len() {
        let c = b[i];
        if in_str {
            if c == '\\' {
                i += 1;
            } else if c == '"' {
                in_str = false;
            }
        } else if c == '"' {
            in_str = true;
        } else if c == '/' && matches!(b.get(i + 1), Some('/' | '*')) {
            break;
        } else if c == '\'' && b.get(i + 1) == Some(&'\\') && b.get(i + 3) == Some(&'\'') {
            i += 3;
        } else if c == '\'' && b.get(i + 2) == Some(&'\'') {
            i += 2;
        } else {
            out.push(c);
        }
        i += 1;
    }
    out
}

/// The first line whose indentation disagrees with its brace nesting: every
/// line inside a block sits deeper than the statement that opens it, and its
/// closing `}` level with that statement. Every rule here scopes by
/// indentation, so a mismatch would scope a local wrongly without a sound.
fn misplaced_line(lines: &[String]) -> Option<usize> {
    let mut stack: Vec<usize> = Vec::new();
    let mut stmt_indent = 0;
    let mut stmt_ended = true;
    for (k, line) in lines.iter().enumerate() {
        let t = trim_lead(line);
        if t.is_empty() || t.starts_with("//") || t.starts_with('*') || t.starts_with("/*") {
            continue;
        }
        let code = braces_of(line);
        let indent = indent_of(line);
        let closes = code.trim_start().starts_with('}');
        if stmt_ended || closes {
            stmt_indent = indent;
        }
        let end = code.trim_end();
        stmt_ended = end.is_empty() || end.ends_with([';', '{', '}', ',']);
        if let Some(&top) = stack.last() {
            if (closes && indent != top) || (!closes && indent <= top) {
                return Some(k);
            }
        }
        for c in code.chars() {
            if c == '{' {
                stack.push(stmt_indent);
            } else if c == '}' {
                stack.pop();
            }
        }
    }
    None
}

/// `src` with its locals tidied (module doc).
///
/// # Panics
///
/// When a line's indentation disagrees with its brace nesting: an emitter
/// wrote it at the wrong depth, which must be fixed there.
#[must_use]
pub fn tidy(src: &str) -> String {
    let mut lines: Vec<String> = src.lines().map(str::to_string).collect();
    while drop_unread_field(&mut lines) {}
    if let Some(k) = misplaced_line(&lines) {
        let from = k.saturating_sub(3);
        panic!("emitted Rust is indented against its braces at line {}:\n{}", k + 1, lines[from..(k + 2).min(lines.len())].join("\n"));
    }
    // A pass resumes after each edit; another runs until one edits nothing,
    // because a dropped write can leave a local declared earlier unread.
    loop {
        let mut edited = false;
        let mut at = 0;
        while let Some(next) = tidy_from(&mut lines, at) {
            edited = true;
            at = next;
        }
        if !edited {
            break;
        }
    }
    for line in &mut lines {
        if line.contains('(') && !trim_lead(line).starts_with("//") {
            *line = drop_parens(line);
        }
    }
    let mut out = lines.join("\n");
    if src.ends_with('\n') {
        out.push('\n');
    }
    out
}

#[cfg(test)]
mod tests {
    use super::tidy;

    #[test]
    fn an_unread_local_goes_with_its_pure_writes() {
        let src = "fn f() -> i32 {\n    let mut a: i32 = 0;\n    #[allow(non_snake_case)]\n    let B_x: i32 = s.b;\n    a = 1;\n    return a;\n}\n";
        assert_eq!(tidy(src), "fn f() -> i32 {\n    let mut a: i32 = 0;\n    a = 1;\n    return a;\n}\n");
    }

    #[test]
    fn a_write_with_a_side_effect_keeps_its_local() {
        let src = "fn f() {\n    let mut a: i32 = 0;\n    a = self.g()?;\n}\n";
        assert_eq!(tidy(src), src);
        let src = "fn f() {\n    let mut a: i32 = 0;\n    if c {\n        a = self.g()?;\n    }\n}\n";
        assert_eq!(tidy(src), src);
    }

    #[test]
    fn a_declaration_and_its_initial_value_stay_where_they_are() {
        let src = "fn f() {\n    let mut a: usize = 0_usize;\n    if c {\n        a = 2;\n    }\n    g(a);\n}\n";
        assert_eq!(tidy(src), src);
        let src = "fn f() {\n    let mut a: usize = 0_usize;\n    while c {\n        a = 2;\n        g(a);\n    }\n}\n";
        assert_eq!(tidy(src), src);
        let src = "fn f() {\n    let mut a: usize = 0_usize;\n    while a < 3 {\n        a = 2;\n    }\n}\n";
        assert_eq!(tidy(src), src);
        let src = "fn f() {\n    let mut a: usize = 0_usize;\n    h();\n    a = a + 1;\n    g(a);\n}\n";
        assert_eq!(tidy(src), src);
    }

    #[test]
    fn mut_stays_for_every_kind_of_write() {
        for w in ["a = 1;", "a += 1;", "a[0] = 1.0;", "a.x = 1;", "g(&mut a);", "a.fill(0.0);", "a[i..].copy_from_slice(b);"] {
            let src = format!("fn f() {{\n    let mut a: T = z;\n    g(a);\n    {w}\n    g(a);\n}}\n");
            assert_eq!(tidy(&src), src, "{w}");
        }
        let src = "fn f() {\n    let mut a: T = z;\n    g(a.len());\n}\n";
        assert_eq!(tidy(src), "fn f() {\n    let a: T = z;\n    g(a.len());\n}\n");
    }

    #[test]
    fn a_borrowed_local_keeps_its_scope_and_a_split_call_is_a_write() {
        let src = "fn f() {\n    let mut h: Vec<f64> = Vec::new();\n    if c {\n        h = vec![0.0; n];\n        b = &mut h;\n    }\n}\n";
        assert_eq!(tidy(src), src);
        let src = "fn f() {\n    let mut r: Vec<f64> = vec![0.0; n];\n    r[..n]\n        .copy_from_slice(&a);\n    g(r);\n}\n";
        assert_eq!(tidy(src), src);
    }

    #[test]
    fn an_uninitialized_local_stored_once_per_path_needs_no_mut() {
        let src = "fn f() {\n    let mut c: f64;\n    match r {\n        0 => {\n            c = a;\n        }\n        _ => {\n            c = b;\n        }\n    }\n    g(c);\n}\n";
        assert!(tidy(src).contains("    let c: f64;\n"));
        for kept in [
            "    c = a;\n    if d {\n        c = b;\n    }\n",
            "    c = a;\n    while d {\n        g(c);\n        c = b;\n    }\n",
        ] {
            let src = format!("fn f() {{\n    let mut c: f64;\n    h();\n{kept}    g(c);\n}}\n");
            assert!(tidy(&src).contains("let mut c: f64"), "{kept}");
        }
    }

    #[test]
    fn a_default_nothing_reads_leaves_only_the_range_check() {
        let src = "    pub fn f(&self, mut a: f64, mut b: i32) -> i32 {\n        if a == D {\n            a = 6e0;\n        } else if !(a >= 1e-2) {\n            return E;\n        }\n        if b == M {\n            b = 3;\n        } else if b < 1 || b > 9 {\n            return E;\n        }\n        return b;\n    }\n";
        assert_eq!(tidy(src), "    pub fn f(&self, a: f64, mut b: i32) -> i32 {\n        if a != D && !(a >= 1e-2) {\n            return E;\n        }\n        if b == M {\n            b = 3;\n        } else if b < 1 || b > 9 {\n            return E;\n        }\n        return b;\n    }\n");
    }

    #[test]
    fn an_empty_then_is_not_folded_into_an_arm_another_follows() {
        let src = "    pub fn f(&self, mut a: f64) -> i32 {\n        if a == D {\n            a = 6e0;\n        } else if a < 1e-2 {\n            return 1;\n        } else {\n            return 2;\n        }\n        return 0;\n    }\n";
        assert_eq!(tidy(src), "    pub fn f(&self, a: f64) -> i32 {\n        if a == D {\n        } else if a < 1e-2 {\n            return 1;\n        } else {\n            return 2;\n        }\n        return 0;\n    }\n");
    }

    #[test]
    fn a_jump_inside_a_line_can_skip_the_overwrite() {
        for jump in ["break;", "continue;", "break 'a;"] {
            let src = format!("fn f(n: usize) -> f64 {{\n    let mut x: f64 = 0.0;\n    let mut k: usize = 0;\n    g(x);\n    'a: while k < n {{\n        g(x);\n        x = v[k];\n        if q(k) {{ {jump} }}\n        x = 0.0;\n        k += 1;\n    }}\n    return x;\n}}\n");
            assert_eq!(tidy(&src), src, "{jump}");
        }
        let src = "fn f(n: usize) -> f64 {\n    let mut x: f64 = 0.0;\n    g(x);\n    x = v[n];\n    while q(n) {\n        if r(n) { break; }\n    }\n    x = 0.0;\n    return x;\n}\n";
        assert_eq!(tidy(src), "fn f(n: usize) -> f64 {\n    let mut x: f64 = 0.0;\n    g(x);\n    while q(n) {\n        if r(n) { break; }\n    }\n    x = 0.0;\n    return x;\n}\n");
        let src = "fn f(n: usize) -> usize {\n    let mut k: usize = 0;\n    g(k);\n    loop {\n        k += 1;\n        if q(n) { break; }\n    }\n    k = n;\n    return k;\n}\n";
        assert_eq!(tidy(src), src.replace("        k += 1;\n", ""));
    }

    #[test]
    fn an_if_expression_chain_ends_at_its_own_close() {
        let src = "fn f(c: bool, d: bool) -> f64 {\n    let mut x: f64 = 0.0;\n    g(x);\n    let y: f64 = if c {\n        x = 5.0;\n        1.0\n    } else {\n        2.0\n    };\n    h(x, y);\n    if d {\n        k();\n    }\n    x = 7.0;\n    return x;\n}\n";
        assert_eq!(tidy(src), src);
        let src = "fn f(c: bool) -> f64 {\n    let mut x: f64 = 0.0;\n    g(x);\n    if c {\n        x = 5.0;\n    } else if x > 1.0 ||\n        x < 0.0\n    {\n        h();\n    }\n    return 1.0;\n}\n";
        assert_eq!(tidy(src), src.replace("        x = 5.0;\n", "").replace("let mut x", "let x"));
    }

    #[test]
    fn a_labelled_loop_is_a_loop() {
        let src = "fn f() -> f64 {\n    let mut x: f64 = 0.0;\n    'outer: loop {\n        g(x);\n        if q() {\n            break 'outer;\n        }\n        x = 5.0;\n    }\n    return 1.0;\n}\n";
        assert_eq!(tidy(src), src);
        let src = "fn f() {\n    let mut c: f64;\n    'a: for k in 0..n {\n        c = h(k);\n        g(c);\n    }\n}\n";
        assert_eq!(tidy(src), src);
    }

    #[test]
    fn a_for_header_reads_the_local_its_binding_shadows() {
        let src = "fn f(n: usize) -> usize {\n    let mut i: usize = 0;\n    g(i);\n    i = 3;\n    for i in i..n {\n        g(i);\n    }\n    i = 9;\n    return i;\n}\n";
        assert_eq!(tidy(src), src);
    }

    #[test]
    fn a_quote_in_a_char_or_raw_string_does_not_hide_a_read() {
        for call in ["h('\\'','\"', \"//\", i);", "h(r#\"a\"//\"#, i);", "h(br\"\\\", \"//\", i);"] {
            let src = format!("fn f() -> usize {{\n    let mut i: usize = 0;\n    g(i);\n    i = 3;\n    {call}\n    i = 9;\n    return i;\n}}\n");
            assert!(tidy(&src).contains("    i = 3;\n"), "{call}");
        }
    }

    #[test]
    fn an_empty_then_is_not_folded_over_an_else_on_its_own_line() {
        let src = "    pub fn f(&self, mut a: f64) -> i32 {\n        if a == D {\n        } else if a < 1e-2 {\n            return 1;\n        }\n        // c\n        else {\n            return 2;\n        }\n        return 0;\n    }\n";
        assert!(tidy(src).contains("        if a == D {\n"), "{}", tidy(src));
    }

    #[test]
    fn a_double_slash_in_a_string_is_code() {
        let src = "fn f() -> usize {\n    let mut i: usize = 0;\n    g(i);\n    i = 3;\n    h(\"a//b\\\"//\", i);\n    i = 9;\n    return i;\n}\n";
        assert_eq!(tidy(src), src);
    }

    #[test]
    fn a_private_parameter_nothing_uses_is_named_unused() {
        let src = "    pub(crate) fn f(&self, tape: &[f64], base: usize, x: f64) -> f64 {\n        return x;\n    }\n    pub fn g(&self, inLow: &[f64], x: f64) -> f64 {\n        return x;\n    }\n";
        assert_eq!(tidy(src), "    pub(crate) fn f(&self, _tape: &[f64], _base: usize, x: f64) -> f64 {\n        return x;\n    }\n    pub fn g(&self, inLow: &[f64], x: f64) -> f64 {\n        return x;\n    }\n");
    }

    #[test]
    fn a_parameter_keeps_mut_only_when_written() {
        let src = "    pub fn f(&self, mut a: i32, mut b: i32, c: &mut [f64]) -> i32 {\n        b = 2;\n        return a + b;\n    }\n";
        assert_eq!(tidy(src), "    pub fn f(&self, a: i32, mut b: i32, c: &mut [f64]) -> i32 {\n        b = 2;\n        return a + b;\n    }\n");
    }

    #[test]
    fn a_for_binding_shadows_the_local() {
        let src = "fn f() {\n    let mut i: usize = 0_usize;\n    for i in 0..n {\n        g(i);\n    }\n    i = n + 1;\n}\n";
        assert_eq!(tidy(src), "fn f() {\n    for i in 0..n {\n        g(i);\n    }\n}\n");
    }

    #[test]
    fn a_store_nothing_reads_goes() {
        let src = "fn f() {\n    let mut i: usize = a;\n    let mut o: usize = 0;\n    for k in 0..n {\n        g(k);\n    }\n    i += n;\n    o += n;\n    h(o);\n}\n";
        assert_eq!(tidy(src), "fn f() {\n    let mut o: usize = 0;\n    for k in 0..n {\n        g(k);\n    }\n    o += n;\n    h(o);\n}\n");
        let src = "fn f() {\n    let mut i: usize = a;\n    while c {\n        g(i);\n        i += 1;\n    }\n}\n";
        assert_eq!(tidy(src), src);
        let src = "fn f() {\n    let mut i: usize = a;\n    g(i);\n    if c {\n        i += 1;\n    }\n    i = b;\n    g(i);\n}\n";
        assert_eq!(tidy(src), "fn f() {\n    let mut i: usize = a;\n    g(i);\n    i = b;\n    g(i);\n}\n");
        for kept in [
            "    if c {\n        i += 1;\n    } else {\n        i = b;\n    }\n",
            "    while c {\n        i += 1;\n        if d {\n            break;\n        }\n        i = b;\n    }\n",
            "    while i < n {\n        i += 1;\n    }\n    i = b;\n",
        ] {
            let src = format!("fn f() {{\n    let mut i: usize = a;\n    g(i);\n{kept}    g(i);\n}}\n");
            assert_eq!(tidy(&src), src, "{kept}");
        }
    }

    #[test]
    #[should_panic(expected = "indented against its braces")]
    fn a_block_written_at_the_wrong_depth_is_refused() {
        let _ = tidy("fn f() {\n    while c {\n        if d {\n            {\n    let n: usize = 1;\n            }\n        }\n    }\n}\n");
    }

    #[test]
    fn a_wrapped_condition_and_a_tail_expression_are_laid_out_right() {
        let src = "fn f() -> R {\n    if a\n        && b {\n        g();\n    }\n    if c {\n        Ok(x)\n    } else {\n        Err(y)\n    }\n}\n";
        assert_eq!(tidy(src), src);
    }

    #[test]
    fn an_emptied_negated_guard_goes() {
        let src = "fn f() {\n    let mut t: f64 = g();\n    h(t);\n    if !(t.abs() <= 1e-14) {\n        t = 0.0;\n    }\n}\n";
        assert_eq!(tidy(src), "fn f() {\n    let t: f64 = g();\n    h(t);\n}\n");
    }

    #[test]
    fn a_dead_self_update_and_a_same_name_copy_go() {
        let src = "fn f(s: usize) {\n    let s = s;\n    while c {\n        let mut k: i32 = g(s);\n        h(k);\n        k += k & 3;\n    }\n}\n";
        assert_eq!(tidy(src), "fn f(s: usize) {\n    while c {\n        let k: i32 = g(s);\n        h(k);\n    }\n}\n");
    }

    #[test]
    fn a_local_reused_by_separate_blocks_keeps_one_declaration() {
        let src = "fn f() {\n    let mut t: f64 = 0.0_f64;\n    while a {\n        t = g();\n        h(t);\n    }\n    while b {\n        t = k();\n        h(t);\n    }\n}\n";
        assert_eq!(tidy(src), src);
        let src = "fn f() {\n    let mut t: f64 = 0.0_f64;\n    while a {\n        t = g();\n    }\n    h(t);\n}\n";
        assert_eq!(tidy(src), src);
    }

    #[test]
    fn a_field_nothing_reads_goes_with_its_entries() {
        let src = "struct S {\n    a: i32,\n    b: f64,\n}\n\nfn f(a: i32, b: f64) -> f64 {\n    let s = S {\n        a,\n        b: b * 2.0,\n    };\n    let t = S { a: 1, b };\n    return s.b + t.b;\n}\n";
        assert_eq!(tidy(src), "struct S {\n    b: f64,\n}\n\nfn f(_a: i32, b: f64) -> f64 {\n    let s = S {\n        b: b * 2.0,\n    };\n    let t = S { b, };\n    return s.b + t.b;\n}\n");
    }

    #[test]
    fn parentheses_go_only_where_nothing_can_parse_differently() {
        for (src, want) in [
            ("        (*outBegIdx) = 0;", "        *outBegIdx = 0;"),
            ("        x = ((a) as f64) * (b[i - 1]);", "        x = (a as f64) * b[i - 1];"),
            ("        g((a + b), (c * d));", "        g(a + b, c * d);"),
            ("        x = (a + b);", "        x = a + b;"),
            ("        if (a < b) {", "        if a < b {"),
            ("        y = (a + b) * c;", "        y = (a + b) * c;"),
            ("        y = (*p).len();", "        y = (*p).len();"),
            ("        y = (2.0).max(x);", "        y = (2.0).max(x);"),
            ("        y = f((a, b));", "        y = f((a, b));"),
            ("        y = c_max!((a), (b[0]));", "        y = c_max!((a), (b[0]));"),
            ("        y = c_color_max!((a), (b[0] + 1.0));", "        y = c_color_max!(a, b[0] + 1.0);"),
            ("        d!(s, f, g, (x));", "        d!(s, f, g, (x));"),
            ("        s = \"(x)\";", "        s = \"(x)\";"),
            ("        y = -(a[i]) + !(b);", "        y = -a[i] + !b;"),
            ("        Ok(())", "        Ok(())"),
            ("        t += h * (1_f64 + r);", "        t += h * (1_f64 + r);"),
            ("        t = h * (1.5);", "        t = h * 1.5;"),
        ] {
            assert_eq!(super::drop_parens(src), want, "{src}");
        }
    }

    #[test]
    fn a_range_bound_is_a_read() {
        let src = "fn f() {\n    let n: usize = 3;\n    g(&v[i..][..n]);\n}\n";
        assert_eq!(tidy(src), src);
    }

    #[test]
    fn a_field_or_a_longer_name_is_not_the_local() {
        let src = "fn f() {\n    let mut a: i32 = 0;\n    s.a = 1;\n    ab = 2;\n}\n";
        assert_eq!(tidy(src), "fn f() {\n    s.a = 1;\n    ab = 2;\n}\n");
    }
}
