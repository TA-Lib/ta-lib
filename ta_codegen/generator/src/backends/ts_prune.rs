//! Drops what an emitted TS module declares and never reads, so the package
//! compiles under `noUnusedLocals` and `noUnusedParameters`.
//!
//! It runs on text because the dead names come from several emitters at once
//! (candle hoists, stream frames, transcribed bodies). Every rule only removes
//! or renames: a name it wrongly kept is reported by `tsc`, and one it wrongly
//! dropped fails to compile. Nothing with a side effect is ever removed, so a
//! value cannot change.

#[derive(Clone, Copy, PartialEq, Eq, Debug)]
enum Kind {
    Ident,
    Punct,
    Other,
    Comment,
    Space,
}

#[derive(Clone, Copy, Debug)]
struct Tok {
    kind: Kind,
    start: usize,
    end: usize,
}

const PUNCTS: &[&str] = &[
    ">>>=", "===", "!==", ">>>", "<<=", ">>=", "**=", "&&=", "||=", "??=", "...", "==", "!=", "<=", ">=", "&&", "||",
    "??", "?.", "++", "--", "+=", "-=", "*=", "/=", "%=", "&=", "|=", "^=", "<<", ">>", "**", "=>",
];

const ASSIGN_OPS: &[&str] = &["=", "+=", "-=", "*=", "/=", "%=", "&=", "|=", "^=", "<<=", ">>=", ">>>=", "**="];

const NOT_A_METHOD: &[&str] =
    &["if", "for", "while", "switch", "catch", "function", "return", "new", "typeof", "do", "else", "super"];

fn lex(s: &str) -> Vec<Tok> {
    let b = s.as_bytes();
    let is_ident = |c: u8| c.is_ascii_alphanumeric() || c == b'_' || c == b'$';
    let mut toks = Vec::new();
    let mut i = 0;
    while i < b.len() {
        let start = i;
        let c = b[i];
        let kind = if c.is_ascii_whitespace() {
            while i < b.len() && b[i].is_ascii_whitespace() {
                i += 1;
            }
            Kind::Space
        } else if s[i..].starts_with("//") {
            i = s[i..].find('\n').map_or(b.len(), |n| i + n);
            Kind::Comment
        } else if s[i..].starts_with("/*") {
            i = s[i + 2..].find("*/").map_or(b.len(), |n| i + 2 + n + 2);
            Kind::Comment
        } else if matches!(c, b'\'' | b'"' | b'`') {
            i += 1;
            while i < b.len() && b[i] != c {
                i += if b[i] == b'\\' { 2 } else { 1 };
            }
            i = (i + 1).min(b.len());
            Kind::Other
        } else if c.is_ascii_digit() {
            while i < b.len() && (is_ident(b[i]) || b[i] == b'.') {
                i += 1;
            }
            Kind::Other
        } else if is_ident(c) {
            while i < b.len() && is_ident(b[i]) {
                i += 1;
            }
            Kind::Ident
        } else {
            i += PUNCTS.iter().find(|p| s[i..].starts_with(**p)).map_or_else(|| s[i..].chars().next().map_or(1, char::len_utf8), |p| p.len());
            Kind::Punct
        };
        toks.push(Tok { kind, start, end: i });
    }
    toks
}

struct Module<'a> {
    src: &'a str,
    toks: Vec<Tok>,
    /// Indices into `toks` of everything but comments and whitespace.
    sig: Vec<usize>,
    /// For each `sig` position holding a bracket, the `sig` position of its partner.
    partner: Vec<Option<usize>>,
}

impl<'a> Module<'a> {
    fn new(src: &'a str) -> Self {
        let toks = lex(src);
        let sig: Vec<usize> = (0..toks.len()).filter(|&i| !matches!(toks[i].kind, Kind::Comment | Kind::Space)).collect();
        let mut partner = vec![None; sig.len()];
        let mut stack = Vec::new();
        for (p, &t) in sig.iter().enumerate() {
            match &src[toks[t].start..toks[t].end] {
                "(" | "[" | "{" => stack.push(p),
                ")" | "]" | "}" => {
                    if let Some(o) = stack.pop() {
                        partner[o] = Some(p);
                        partner[p] = Some(o);
                    }
                }
                _ => {}
            }
        }
        Module { src, toks, sig, partner }
    }

    fn text(&self, p: usize) -> &'a str {
        let t = self.toks[self.sig[p]];
        &self.src[t.start..t.end]
    }

    fn is_ident(&self, p: usize) -> bool {
        self.toks[self.sig[p]].kind == Kind::Ident
    }

    fn start(&self, p: usize) -> usize {
        self.toks[self.sig[p]].start
    }

    fn end(&self, p: usize) -> usize {
        self.toks[self.sig[p]].end
    }

    /// A use of the name at `p`, as opposed to a property of something else.
    fn is_name_ref(&self, p: usize, name: &str) -> bool {
        self.is_ident(p) && self.text(p) == name && (p == 0 || !matches!(self.text(p - 1), "." | "?."))
    }

    fn at_statement_start(&self, p: usize) -> bool {
        p == 0 || matches!(self.text(p - 1), ";" | "{" | "}")
    }

    /// The `sig` position of the `;` ending the statement that starts at `p`,
    /// if it ends at the same bracket depth.
    fn statement_end(&self, mut p: usize) -> Option<usize> {
        while p < self.sig.len() {
            match self.text(p) {
                ";" => return Some(p),
                "(" | "[" | "{" => p = self.partner[p]?,
                ")" | "]" | "}" => return None,
                _ => {}
            }
            p += 1;
        }
        None
    }

    /// Whether evaluating `sig[from..to]` can do nothing but produce a value.
    fn is_pure(&self, from: usize, to: usize) -> bool {
        (from..to).all(|p| match self.text(p) {
            "++" | "--" | "await" | "yield" | "delete" => false,
            t if ASSIGN_OPS.contains(&t) => false,
            "new" => p + 1 < to && matches!(self.text(p + 1), "Float64Array" | "Int32Array"),
            "(" => p == from || !(self.is_ident(p - 1) || matches!(self.text(p - 1), ")" | "]")) || (p >= from + 2 && self.text(p - 2) == "new"),
            _ => true,
        })
    }

    fn doc_before(&self, p: usize) -> Option<&'a str> {
        let mut t = self.sig[p];
        while t > 0 {
            t -= 1;
            match self.toks[t].kind {
                Kind::Space => {}
                Kind::Comment => return Some(&self.src[self.toks[t].start..self.toks[t].end]),
                _ => return None,
            }
        }
        None
    }
}

struct Func {
    params: (usize, usize),
    body: (usize, usize),
    public: bool,
}

fn functions(m: &Module) -> Vec<Func> {
    let mut out = Vec::new();
    for p in 0..m.sig.len() {
        let (open, head) = if m.text(p) == "function" && p + 2 < m.sig.len() && m.is_ident(p + 1) && m.text(p + 2) == "(" {
            (p + 2, p)
        } else if m.is_ident(p)
            && !NOT_A_METHOD.contains(&m.text(p))
            && p + 1 < m.sig.len()
            && m.text(p + 1) == "("
            && (p == 0 || matches!(m.text(p - 1), "{" | "}" | ";" | "static" | "private" | "protected" | "public"))
        {
            (p + 1, p)
        } else {
            continue;
        };
        let Some(close) = m.partner[open] else { continue };
        let mut q = close + 1;
        if q < m.sig.len() && m.text(q) == ":" {
            q += 1;
            while q < m.sig.len() && (m.is_ident(q) || matches!(m.text(q), "." | "|" | "<" | ">" | "[" | "]")) {
                q += 1;
            }
        }
        if q >= m.sig.len() || m.text(q) != "{" {
            continue;
        }
        let Some(end) = m.partner[q] else { continue };
        let mut first = head;
        while first > 0 && matches!(m.text(first - 1), "export" | "static" | "private" | "protected" | "public") {
            first -= 1;
        }
        let modifiers: Vec<&str> = (first..head).map(|x| m.text(x)).collect();
        let internal = m.doc_before(first).is_some_and(|d| d.starts_with("/**") && d.contains("@internal"));
        let public = if m.text(head) == "function" {
            modifiers.contains(&"export") && !internal
        } else {
            !internal && !modifiers.contains(&"private") && !modifiers.contains(&"protected")
        };
        out.push(Func { params: (open, close), body: (q, end), public });
    }
    out
}

enum Edit {
    Delete(usize, usize),
    Insert(usize, String),
    Replace(usize, usize, String),
}

/// One local: its declaration and, when every other mention only writes it
/// with a value that has no side effect, those writes. `None` if it is read.
fn dead_local(m: &Module, decl: usize, body_end: usize) -> Option<Vec<(usize, usize)>> {
    let name_at = decl + 1;
    if !m.is_ident(name_at) {
        return None;
    }
    let name = m.text(name_at);
    let end = m.statement_end(name_at)?;
    let mut init = None;
    let mut q = name_at + 1;
    while q < end {
        match m.text(q) {
            "," => return None,
            "=" if init.is_none() => init = Some(q + 1),
            "(" | "[" | "{" => q = m.partner[q]?,
            _ => {}
        }
        q += 1;
    }
    if init.is_some_and(|i| !m.is_pure(i, end)) {
        return None;
    }
    let mut spans = vec![(decl, end)];
    let scope_end = (0..decl).rev().find(|&o| m.text(o) == "{" && m.partner[o].is_some_and(|c| c > end)).and_then(|o| m.partner[o]).unwrap_or(body_end);
    let mut p = end + 1;
    while p < scope_end {
        if m.is_name_ref(p, name) {
            let op = m.text(p + 1);
            if !m.at_statement_start(p) {
                return None;
            }
            let stmt_end = m.statement_end(p)?;
            if ASSIGN_OPS.contains(&op) {
                if !m.is_pure(p + 2, stmt_end) {
                    return None;
                }
            } else if !(matches!(op, "++" | "--") && stmt_end == p + 2) {
                return None;
            }
            spans.push((p, stmt_end));
            p = stmt_end;
        }
        p += 1;
    }
    Some(spans)
}

fn plan(m: &Module) -> Vec<Edit> {
    let mut edits = Vec::new();
    for f in functions(m) {
        let (open, close) = f.body;
        for p in open + 1..close {
            if matches!(m.text(p), "let" | "const") && m.at_statement_start(p) {
                if let Some(spans) = dead_local(m, p, close) {
                    edits.extend(spans.into_iter().map(|(a, b)| Edit::Delete(m.start(a), m.end(b))));
                    return edits;
                }
            }
        }
        let (po, pc) = f.params;
        let mut p = po + 1;
        while p < pc {
            if m.is_ident(p) && matches!(m.text(p - 1), "(" | ",") && m.text(p) != "this" && !m.text(p).starts_with('_') {
                let name = m.text(p);
                let used = (po + 1..close).any(|x| x != p && m.is_name_ref(x, name));
                if !used {
                    if f.public {
                        let line = m.src[..m.start(f.body.0)].rfind('\n').map_or(0, |n| n + 1);
                        let indent: String = m.src[line..].chars().take_while(|c| *c == ' ').collect();
                        edits.push(Edit::Insert(m.end(open), format!("\n{indent}  void {name};")));
                    } else {
                        edits.push(Edit::Replace(m.start(p), m.end(p), format!("_{name}")));
                    }
                }
            }
            p = match m.text(p) {
                "(" | "[" | "{" | "<" if m.partner[p].is_some() => m.partner[p].unwrap_or(p) + 1,
                _ => p + 1,
            };
        }
    }
    edits
}

/// `start..end` widened to whole lines when nothing else shares them.
fn widen(src: &str, start: usize, end: usize) -> (usize, usize) {
    let line = src[..start].rfind('\n').map_or(0, |n| n + 1);
    let eol = src[end..].find('\n').map_or(src.len(), |n| end + n);
    if src[line..start].trim().is_empty() && src[end..eol].trim().is_empty() {
        (line, (eol + 1).min(src.len()))
    } else {
        (start, end)
    }
}

/// `module` with every unread local removed and every unread parameter marked:
/// an internal one renamed `_<name>`, a public one, whose name a caller sees,
/// read once by a `void <name>;`.
#[must_use]
pub fn prune(module: &str) -> String {
    let mut text = module.to_string();
    loop {
        let m = Module::new(&text);
        let mut edits = plan(&m);
        if edits.is_empty() {
            return text;
        }
        edits.sort_by_key(|e| std::cmp::Reverse(match e {
            Edit::Delete(a, _) | Edit::Insert(a, _) | Edit::Replace(a, _, _) => *a,
        }));
        let mut out = text.clone();
        for e in edits {
            match e {
                Edit::Delete(a, b) => {
                    let (a, b) = widen(&text, a, b);
                    out.replace_range(a..b, "");
                }
                Edit::Insert(a, s) => out.insert_str(a, &s),
                Edit::Replace(a, b, s) => out.replace_range(a..b, &s),
            }
        }
        text = out;
    }
}

#[cfg(test)]
mod tests {
    use super::prune;

    #[test]
    fn a_local_written_but_never_read_goes_with_its_writes() {
        let src = "export function fImpl(core: Core): number {\n  let a = 0;\n  let rc: RetCode = RetCode.SUCCESS;\n  rc = RetCode.SUCCESS;\n  a = core.x + 1;\n  return a;\n}\n";
        assert_eq!(prune(src), "export function fImpl(core: Core): number {\n  let a = 0;\n  a = core.x + 1;\n  return a;\n}\n");
    }

    #[test]
    fn a_write_with_a_side_effect_keeps_the_local() {
        let src = "function g(): void {\n  let a = 0;\n  a = h();\n  let b = 0;\n  b = c++;\n}\n";
        assert_eq!(prune(src), src);
    }

    #[test]
    fn a_write_that_is_not_its_own_statement_counts_as_a_read() {
        let src = "function g(x: number): void {\n  let a = 0;\n  if (x) a = 1;\n  let b = 0;\n  x = b = 2;\n}\n";
        assert_eq!(prune(src), src);
    }

    #[test]
    fn removal_cascades_until_nothing_more_is_dead() {
        let src = "function g(): void {\n  let a = 0;\n  let b = 0;\n  b = a;\n}\n";
        assert_eq!(prune(src), "function g(): void {\n}\n");
    }

    #[test]
    fn unread_parameters_are_renamed_when_internal_and_read_by_void_when_public() {
        let src = "/** @internal */\nexport function fImpl(core: Core, x: number): number {\n  return x;\n}\n\
                   /** Doc. */\nexport function fLookback(core: Core = Core.DEFAULT): number {\n  return 0;\n}\n\
                   function fTape(sp: S, tape: T): void {\n  sp.n = 0;\n}\n";
        let want = "/** @internal */\nexport function fImpl(_core: Core, x: number): number {\n  return x;\n}\n\
                    /** Doc. */\nexport function fLookback(core: Core = Core.DEFAULT): number {\n  void core;\n  return 0;\n}\n\
                    function fTape(sp: S, _tape: T): void {\n  sp.n = 0;\n}\n";
        assert_eq!(prune(src), want);
    }

    #[test]
    fn a_property_of_the_same_name_is_not_a_read() {
        let src = "class S {\n  peek(x: number): number {\n    const sp = this;\n    return x + this.sp;\n  }\n}\n";
        assert_eq!(prune(src), "class S {\n  peek(x: number): number {\n    return x + this.sp;\n  }\n}\n");
    }

    #[test]
    fn element_writes_and_strings_and_comments_are_reads_or_ignored() {
        let src = "function g(n: number): void {\n  const buf = new Float64Array(n);\n  buf[0] = 1;\n  /* buf */\n  const s = 'buf';\n  out(s);\n}\n";
        assert_eq!(prune(src), src);
    }
}
