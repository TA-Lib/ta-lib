//! Post-emission hygiene over generated C: one pass, run on the finished text.
//!
//! An emitter writes a function's head before it renders the body below it, so
//! it cannot yet know which of the names it just declared that body will read.
//! It emits `(void)x;` for all of them and this pass deletes the ones the body
//! turned out to read. Text rather than IR because the casts are scaffolding
//! the IR never carried, and running here means a new emitter inherits the
//! cleanup instead of re-deriving the same "will the body use it" question.
//!
//! Scope is the innermost enclosing BLOCK, so two sibling blocks that each
//! declare their own `badBar` are judged separately.
//!
//! Deleting a cast over a name the block only WRITES resurrects the
//! `-Wunused-but-set-variable` it was suppressing, so a read is PROVEN, never
//! assumed: an occurrence counts only where it is unambiguously an operand,
//! and anything else leaves the cast alone. `x = 1`, `x++`, `x += 1` and a
//! declarator (including `int a, x;`, whose `x` is preceded only by a comma)
//! are all non-reads — gcc warns on every one of them.

use regex::Regex;

/// Types a preceding token makes the occurrence a DECLARATOR rather than a use.
const TYPE_WORDS: &[&str] = &[
    "int", "double", "float", "char", "short", "long", "unsigned", "signed", "void", "const",
    "struct", "enum", "union", "static", "register", "volatile",
];

/// Delete every `(void)<ident>;` whose own block still reads `<ident>`.
#[must_use]
pub fn scrub_void_casts(src: &str) -> String {
    let cast = Regex::new(r"\(void\)([A-Za-z_][A-Za-z0-9_]*);").expect("literal pattern");
    // Every scan runs on a copy with string literals, char literals and
    // comments blanked to spaces: a brace or an identifier inside one is text,
    // not code, and the block walk below would desynchronise on it. Blanking
    // preserves length, so an offset means the same thing in both.
    let masked = mask_literals(src);
    let mut out = String::with_capacity(src.len());
    let mut at = 0usize;
    for line in src.split_inclusive('\n') {
        let start = at;
        at += line.len();
        let masked_line = &masked[start..at];
        if !cast.is_match(masked_line) {
            out.push_str(line);
            continue;
        }
        let mut kept = String::new();
        let mut last = 0usize;
        let mut cut = false;
        for m in cast.captures_iter(masked_line) {
            let (whole, name) = (m.get(0).expect("whole match"), &m[1]);
            let Some((lo, hi)) = enclosing_block(&masked, start + whole.start()) else {
                continue;
            };
            if !block_reads(&masked[lo..hi], name, &cast) {
                continue;
            }
            kept.push_str(&line[last..whole.start()]);
            last = whole.end();
            // `(void)a; (void)b;` must not leave the separator behind.
            if line[last..].starts_with(' ') {
                last += 1;
            }
            cut = true;
        }
        kept.push_str(&line[last..]);
        if !cut {
            out.push_str(line);
            continue;
        }
        // A line that held nothing but deleted casts goes with them.
        if kept.trim().is_empty() {
            continue;
        }
        let body = kept.strip_suffix('\n').unwrap_or(&kept);
        out.push_str(body.trim_end());
        if kept.ends_with('\n') {
            out.push('\n');
        }
    }
    out
}

/// A copy of `src` with every string literal, char literal and comment blanked
/// to spaces. Same length, and newlines survive so line offsets still line up.
fn mask_literals(src: &str) -> String {
    let b = src.as_bytes();
    let mut out = Vec::with_capacity(b.len());
    let mut i = 0usize;
    while i < b.len() {
        match b[i] {
            b'/' if i + 1 < b.len() && b[i + 1] == b'/' => {
                while i < b.len() && b[i] != b'\n' {
                    out.push(b' ');
                    i += 1;
                }
            }
            b'/' if i + 1 < b.len() && b[i + 1] == b'*' => {
                let end = src[i + 2..].find("*/").map_or(b.len(), |k| i + 2 + k + 2);
                while i < end {
                    out.push(if b[i] == b'\n' { b'\n' } else { b' ' });
                    i += 1;
                }
            }
            q @ (b'"' | b'\'') => {
                out.push(q);
                i += 1;
                while i < b.len() && b[i] != q {
                    let esc = b[i] == b'\\';
                    out.push(if b[i] == b'\n' { b'\n' } else { b' ' });
                    i += 1;
                    if esc && i < b.len() {
                        out.push(if b[i] == b'\n' { b'\n' } else { b' ' });
                        i += 1;
                    }
                }
                if i < b.len() {
                    out.push(q);
                    i += 1;
                }
            }
            c => {
                out.push(c);
                i += 1;
            }
        }
    }
    String::from_utf8(out).expect("blanking is byte-for-byte over ASCII punctuation")
}

/// The innermost `{ … }` containing `at`, as the byte range between the braces.
fn enclosing_block(masked: &str, at: usize) -> Option<(usize, usize)> {
    let b = masked.as_bytes();
    let mut depth = 0i32;
    let mut i = at;
    let open = loop {
        if i == 0 {
            return None;
        }
        i -= 1;
        match b[i] {
            b'}' => depth += 1,
            b'{' if depth == 0 => break i,
            b'{' => depth -= 1,
            _ => {}
        }
    };
    let mut depth = 0i32;
    for (j, c) in b.iter().enumerate().skip(open) {
        match c {
            b'{' => depth += 1,
            b'}' => {
                depth -= 1;
                if depth == 0 {
                    return Some((open + 1, j));
                }
            }
            _ => {}
        }
    }
    None
}

/// Whether `block` reads `name` anywhere, casts themselves excluded.
fn block_reads(block: &str, name: &str, cast: &Regex) -> bool {
    let scanned = cast.replace_all(block, |c: &regex::Captures| " ".repeat(c[0].len()));
    let b = scanned.as_bytes();
    scanned.match_indices(name).any(|(i, _)| {
        let before_ok = i == 0 || !is_word_byte(b[i - 1]);
        let end = i + name.len();
        let after_ok = end >= b.len() || !is_word_byte(b[end]);
        before_ok && after_ok && is_proven_read(&scanned, i, name.len())
    })
}

fn is_word_byte(c: u8) -> bool {
    c.is_ascii_alphanumeric() || c == b'_'
}

/// Whether the occurrence at `at` is unambiguously an OPERAND.
///
/// One-sided on purpose: a false "read" deletes a cast that was doing its job,
/// a false "not a read" only leaves one behind.
fn is_proven_read(s: &str, at: usize, len: usize) -> bool {
    let b = s.as_bytes();
    let prev_at = (0..at).rev().find(|&i| !b[i].is_ascii_whitespace());
    let prev = prev_at.map(|i| b[i]);

    if is_declarator(s, prev_at) {
        return false;
    }
    // Preceded by an operator or an opening bracket: an operand. `,` is left
    // out — it is also what separates declarators in `int a, x;`.
    if matches!(
        prev,
        Some(
            b'=' | b'+' | b'-' | b'/' | b'%' | b'<' | b'>' | b'!' | b'&' | b'|' | b'^' | b'('
                | b'[' | b'?' | b':' | b'~'
        )
    ) {
        return true;
    }
    if prev == Some(b'*') {
        return true; // a dereference or a multiply; the declarator case already returned
    }

    let mut j = at + len;
    while j < b.len() && b[j].is_ascii_whitespace() {
        j += 1;
    }
    let op = &b[j..b.len().min(j + 3)];
    // A store, an increment or a compound assignment reads nothing gcc counts:
    // `x = 1`, `x++`, `x += 1` and `x <<= 2` all still warn. A comparison or a
    // shift does.
    match op {
        [b'=' | b'!', b'=', ..] => return true,
        [b'+', b'+', ..] | [b'-', b'-', ..] | [b'<', b'<', b'='] | [b'>', b'>', b'='] => {
            return false
        }
        [c, b'=', ..] if b"+-*/%&|^<>".contains(c) => return false,
        _ => {}
    }
    matches!(
        op.first(),
        Some(b'+' | b'-' | b'*' | b'/' | b'%' | b'&' | b'|' | b'^' | b'<' | b'>' | b')' | b']' | b'[')
    )
}

/// Whether the token before the occurrence makes it a DECLARATOR — a type
/// word, a typedef (`TA_…`, `…_t`), or a pointer star behind one.
fn is_declarator(s: &str, prev_at: Option<usize>) -> bool {
    let b = s.as_bytes();
    let Some(mut i) = prev_at else { return false };
    // Walk back over a pointer declarator's stars.
    while b[i] == b'*' {
        match (0..i).rev().find(|&k| !b[k].is_ascii_whitespace()) {
            Some(k) => i = k,
            None => return false,
        }
    }
    if !is_word_byte(b[i]) {
        return false;
    }
    let end = i + 1;
    let start = (0..=i).rev().take_while(|&k| is_word_byte(b[k])).last().unwrap_or(i);
    let word = &s[start..end];
    TYPE_WORDS.contains(&word) || word.starts_with("TA_") || word.ends_with("_t")
}

#[cfg(test)]
mod tests {
    use super::*;

    fn scrub(s: &str) -> String {
        scrub_void_casts(s)
    }

    #[test]
    fn a_cast_over_a_name_the_block_reads_goes() {
        let src = "void f( void )\n{\n   int startIdx;\n   (void)startIdx;\n   g( startIdx );\n}\n";
        assert_eq!(
            scrub(src),
            "void f( void )\n{\n   int startIdx;\n   g( startIdx );\n}\n"
        );
    }

    #[test]
    fn a_cast_over_a_name_the_block_only_writes_stays() {
        let src = "void f( void )\n{\n   int dummy;\n   dummy = 0;\n   (void)dummy;\n}\n";
        assert_eq!(scrub(src), src, "gcc warns `set but not used` without it");
    }

    /// The shape the emitters actually write: several casts on one line, only
    /// some of them answered.
    #[test]
    fn one_line_of_casts_keeps_exactly_the_unanswered_ones() {
        let src = "void f( int startIdx )\n{\n   int a;\n   int b;\n   a = 0;\n   b = 0;\n   \
                   (void)startIdx; (void)a; (void)b;\n   h( startIdx, b );\n}\n";
        let out = scrub(src);
        assert!(out.contains("   (void)a;\n"), "only `a` is write-only:\n{out}");
        assert!(!out.contains("(void)startIdx"), "startIdx is read:\n{out}");
        assert!(!out.contains("(void)b;"), "b is read:\n{out}");
    }

    #[test]
    fn a_line_left_holding_nothing_goes_with_the_casts() {
        let src = "void f( int x )\n{\n   (void)x;\n   g( x );\n}\n";
        assert_eq!(scrub(src), "void f( int x )\n{\n   g( x );\n}\n");
    }

    /// Sibling blocks each declaring the same name: the read in one says
    /// nothing about the other, which is why the scope is the block.
    #[test]
    fn a_read_in_a_sibling_block_does_not_answer_this_block() {
        let src = "void f( void )\n{\n   { int v; v = 1; g( v ); }\n   { int v; v = 2; (void)v; }\n}\n";
        assert_eq!(scrub(src), src);
    }

    /// A read anywhere INSIDE the block answers it, however deeply nested.
    #[test]
    fn a_read_in_a_nested_block_answers_the_enclosing_one() {
        let src = "void f( void )\n{\n   int v;\n   v = 1;\n   (void)v;\n   { g( v ); }\n}\n";
        assert!(!scrub(src).contains("(void)v;"));
    }

    /// gcc still warns on each of these, so none of them may answer a cast.
    #[test]
    fn a_store_an_increment_and_a_compound_assignment_are_not_reads() {
        for stmt in ["   v = 1;", "   v++;", "   v += 1;", "   v--;", "   v <<= 2;"] {
            let src = format!("void f( void )\n{{\n   int v;\n{stmt}\n   (void)v;\n}}\n");
            assert_eq!(scrub(&src), src, "`{stmt}` is not a read");
        }
    }

    /// `int ok = 1, badBar = -1;` — the comma is a declarator separator, not
    /// the argument comma, so it must not read as a use.
    #[test]
    fn a_comma_declarator_is_not_a_read() {
        let src = "void f( void )\n{\n   int ok = 1, badBar = -1;\n   ok = 0;\n   (void)badBar;\n}\n";
        assert_eq!(scrub(src), src);
    }

    /// A brace or a name inside a literal is text, and the block walk would
    /// desynchronise on it.
    #[test]
    fn a_brace_inside_a_string_literal_does_not_move_the_block() {
        let src = "void f( int x )\n{\n   p( \"{ x }\" );\n   (void)x;\n}\n";
        assert_eq!(scrub(src), src, "the literal's `x` is not a read of the parameter");
    }

    #[test]
    fn a_name_inside_a_comment_is_not_a_read() {
        let src = "void f( int x )\n{\n   /* x is the anchor */\n   (void)x;\n}\n";
        assert_eq!(scrub(src), src);
    }

    /// A declarator that happens to be an array is not a read of its own base.
    #[test]
    fn an_array_declarator_is_not_a_read() {
        let src = "void f( void )\n{\n   double buf[8];\n   (void)buf;\n}\n";
        assert_eq!(scrub(src), src);
    }

    #[test]
    fn indexing_a_buffer_reads_it() {
        let src = "void f( void )\n{\n   double buf[8];\n   buf[0] = 1.0;\n   (void)buf;\n}\n";
        assert!(!scrub(src).contains("(void)buf;"));
    }

    #[test]
    fn the_pass_is_idempotent() {
        let src = "void f( int startIdx )\n{\n   int a;\n   a = 0;\n   (void)startIdx; (void)a;\n   g( startIdx );\n}\n";
        let once = scrub(src);
        assert_eq!(scrub(&once), once);
    }
}
