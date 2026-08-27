//! A conservative C re-indenter for the `ta_codegen/input/*.c` source-of-truth.
//!
//! The input `.c` files are the human-maintained source of truth. The
//! pre-`ta_codegen` reference C was properly indented, but when these inputs
//! were rewritten (by an AI, during the cutover) the indentation was not
//! preserved: statements ended up flat at a fixed 4-space margin regardless of
//! nesting depth, which is hard to read. This module tidies them up by:
//!
//! 1. **re-indenting** so the left margin reflects `{}` / `()` nesting again;
//! 2. **stripping trailing whitespace** from every line;
//! 3. **collapsing blank-line runs** to at most one (and dropping leading /
//!    trailing blank lines), so files end in a single newline.
//!
//! # The safety invariant
//!
//! The property that makes this safe to run on files where comments are
//! *load-bearing* — the candlestick recognizers position `//` comments to the
//! right of individual `&&` operands inside an `if` predicate — is:
//!
//! > **The ordered sequence of non-blank lines, each trimmed of surrounding
//! > whitespace, is identical before and after.**
//!
//! Only *whitespace* (leading + trailing) and *blank-line runs* ever change. No
//! code or comment line is added, dropped, reordered, or edited; there is no
//! token reflow, no line joining, no line splitting. A trailing comment
//! therefore cannot move relative to the operand it annotates. [`reindent_source`]
//! asserts this via [`content_preserved`] on its own output in debug builds, and
//! the `format` command re-checks it before writing any file.
//!
//! Two deliberate carve-outs keep generated output byte-identical (the parser
//! already trims comment lines and is whitespace-insensitive, but it *does*
//! preserve blank lines inside `/* ... */`): blank lines inside a block comment
//! are never collapsed, and every comment line's interior is left untouched.
//!
//! Because indentation is computed by a lightweight structural scan (not a full
//! parse), an exotic construct can at worst produce *slightly wrong indentation*
//! on a line — never corrupted code and never a displaced comment.

/// Spaces per indentation level. Matches the C backend's emitter (`c.rs` uses
/// `indent + 3`), so input and generated output share one house style.
const UNIT: usize = 3;

/// Re-indent one C translation unit. Idempotent: `reindent(reindent(x)) ==
/// reindent(x)`.
#[must_use]
pub fn reindent_source(text: &str) -> String {
    let ends_with_newline = text.ends_with('\n');
    let mut out: Vec<String> = Vec::new();

    // Physical lines to process. `split('\n')` yields a trailing "" for a file
    // ending in newline (an EOF artifact, not a real line) — drop it so it is
    // not mistaken for a trailing blank line to collapse.
    let all: Vec<&str> = text.split('\n').collect();
    let lines: &[&str] = if ends_with_newline && !all.is_empty() {
        &all[..all.len() - 1]
    } else {
        &all[..]
    };

    // Running structural state, carried across physical lines.
    let mut brace_depth: i32 = 0; // net `{` minus `}` seen before this line
    let mut paren_depth: i32 = 0; // net `(` minus `)` seen before this line
    let mut in_block_comment = false; // inside a `/* ... */` at line start

    // Block-comment interior alignment, captured when a `/*` is left open.
    let mut bc_indent: usize = 0; // opener's new indent, in spaces
    let mut bc_shift: i32 = 0; // opener's (new indent - old indent), in spaces

    // A control header (`if(...)`, `for(...)`, `else`, `do`, ...) with no `{`
    // brace and no `;` grants its single following statement one extra level.
    let mut unbraced_pending = false;

    // `switch` body: while a `case`/`default` label is active at brace depth
    // `d`, statements at depth `d` are indented one level under the label.
    let mut case_active_depth: Option<i32> = None;

    for raw in lines {
        let content = raw.trim_start();

        // --- Blank line. ----------------------------------------------------
        if content.is_empty() {
            if in_block_comment {
                // A blank line *inside* a `/* ... */` block is comment layout
                // the parser captures verbatim — preserve it exactly, never
                // collapse it (doing so would change generated output).
                out.push(String::new());
            } else if out.last().is_none_or(String::is_empty) {
                // Code-context blank: drop it when it would be a leading blank
                // (`out` empty) or a second consecutive blank — collapsing runs
                // of blank lines down to a single one.
            } else {
                out.push(String::new());
            }
            continue;
        }

        let scan = scan_line(raw, in_block_comment);

        // --- Interior / closing line of a multi-line block comment. ---------
        if in_block_comment {
            let indent = if content.starts_with('*') {
                // Align the interior `*` under the opener's `/*` star.
                bc_indent + 1
            } else {
                // Free-form comment body: preserve its offset relative to the
                // opener by applying the same shift the opener received.
                usize::try_from(to_i32(leading_ws_len(raw)) + bc_shift).unwrap_or(0)
            };
            out.push(reindent_line(indent, content.trim_end()));
            apply_scan_deltas(&scan, &mut brace_depth, &mut paren_depth);
            in_block_comment = scan.ends_in_block_comment;
            continue;
        }

        // --- Normal code line: compute its indentation level. ---------------
        let first = content.chars().next().unwrap_or(' ');
        let mut level = brace_depth;

        if first == '}' {
            // Closing brace de-indents to the enclosing level.
            level -= 1;
        } else if first == ')' {
            // A line that opens by closing a continuation paren aligns with the
            // header that opened it — i.e. no continuation bump.
        } else {
            // Continuation of an unclosed `(...)` (e.g. a split `&&` predicate).
            if paren_depth > 0 {
                level += 1;
            }
            if scan.is_label {
                // `case`/`default` labels sit at the switch-body level.
                case_active_depth = Some(brace_depth);
            } else {
                if case_active_depth == Some(brace_depth) {
                    level += 1; // statement under an active case label
                }
                // A `{` here is the header's braced body — it belongs at the
                // header's level, so only bump a genuinely braceless statement.
                if unbraced_pending && first != '{' {
                    level += 1; // single statement owned by a braceless header
                }
            }
        }

        let indent = usize::try_from(level).unwrap_or(0) * UNIT;
        out.push(reindent_line(indent, content.trim_end()));

        // Record shift for any block comment this line opens.
        if scan.ends_in_block_comment {
            bc_indent = indent;
            bc_shift = to_i32(indent) - to_i32(leading_ws_len(raw));
        }

        // --- Advance structural state for the next line. --------------------
        // `unbraced_pending` is consumed by exactly one following code line: a
        // `{` means the body was braced after all (no bump); anything else was
        // the single braceless statement (bump already applied above).
        unbraced_pending = scan.is_header;

        apply_scan_deltas(&scan, &mut brace_depth, &mut paren_depth);
        in_block_comment = scan.ends_in_block_comment;

        // A `case` label stays active until the switch's `}` drops us below it.
        if let Some(d) = case_active_depth {
            if brace_depth < d {
                case_active_depth = None;
            }
        }
    }

    // Strip trailing blank lines so the file ends with content + a single
    // newline (no blank line before EOF).
    while out.last().is_some_and(String::is_empty) {
        out.pop();
    }

    let mut result = out.join("\n");
    if ends_with_newline {
        result.push('\n');
    }

    debug_assert!(
        content_preserved(text, &result),
        "re-indenter altered non-whitespace content"
    );
    result
}

/// Build one output line, guaranteeing the safety invariant: `spaces + content`
/// where `content` is the source line with its leading whitespace stripped.
fn reindent_line(indent_spaces: usize, content: &str) -> String {
    let mut s = " ".repeat(indent_spaces);
    s.push_str(content);
    s
}

fn leading_ws_len(line: &str) -> usize {
    line.len() - line.trim_start().len()
}

/// Widen an indentation length to a signed unit for shift arithmetic. Lossless
/// for the small non-negative values this module handles (line indentation).
fn to_i32(x: usize) -> i32 {
    i32::try_from(x).unwrap_or(i32::MAX)
}

fn apply_scan_deltas(scan: &LineScan, brace_depth: &mut i32, paren_depth: &mut i32) {
    *brace_depth += scan.brace_delta;
    *paren_depth += scan.paren_delta;
    if *paren_depth < 0 {
        *paren_depth = 0;
    }
}

/// Structural facts extracted from one physical line, ignoring the contents of
/// strings, character literals, and comments.
struct LineScan {
    brace_delta: i32,
    paren_delta: i32,
    ends_in_block_comment: bool,
    is_label: bool,  // `case ...:` or `default:`
    is_header: bool, // braceless control header owning a single statement
}

/// The keywords that can own the next line's statement when left dangling.
const HEADER_KW: [&str; 6] = ["if", "for", "while", "switch", "else", "do"];

/// Scan a line for brace/paren balance and comment state, masking out the
/// contents of strings/chars/comments so a `{` inside `"..."` never counts.
fn scan_line(line: &str, mut in_bc: bool) -> LineScan {
    let chars: Vec<char> = line.chars().collect();
    let n = chars.len();
    let mut i = 0;
    let mut brace = 0i32;
    let mut paren = 0i32;
    let mut code = String::new(); // executable code only (comments/strings dropped)
    let mut in_str = false;
    let mut in_chr = false;

    while i < n {
        let c = chars[i];
        let next = if i + 1 < n { chars[i + 1] } else { '\0' };

        if in_bc {
            if c == '*' && next == '/' {
                in_bc = false;
                i += 2;
                continue;
            }
            i += 1;
            continue;
        }
        if in_str {
            if c == '\\' {
                i += 2; // skip escaped char
                continue;
            }
            if c == '"' {
                in_str = false;
            }
            i += 1;
            continue;
        }
        if in_chr {
            if c == '\\' {
                i += 2;
                continue;
            }
            if c == '\'' {
                in_chr = false;
            }
            i += 1;
            continue;
        }

        // Not inside any string/comment.
        if c == '/' && next == '/' {
            break; // line comment: nothing structural follows
        }
        if c == '/' && next == '*' {
            in_bc = true;
            i += 2;
            continue;
        }
        if c == '"' {
            in_str = true;
            i += 1;
            continue;
        }
        if c == '\'' {
            in_chr = true;
            i += 1;
            continue;
        }
        match c {
            '{' => brace += 1,
            '}' => brace -= 1,
            '(' => paren += 1,
            ')' => paren -= 1,
            _ => {}
        }
        code.push(c);
        i += 1;
    }

    let code = code.trim();
    let last = code.chars().last().unwrap_or('\0');
    let is_label = (starts_with_keyword(code, "case") || starts_with_keyword(code, "default"))
        && last == ':';
    // A header owns the next line's statement only when it is left DANGLING at
    // the end of this one, so the keyword test applies twice: to the line, as
    // before, and again to whatever follows the last `}` -- everything up to
    // that brace already closed here. `if( c ) { f(); }` owns nothing, while
    // `if( c ) { f(); } else` still owns the next statement. A header with no
    // brace at all is its own tail, so the second test is a no-op on it.
    let tail = match code.rfind('}') {
        Some(i) => code[i + 1..].trim(),
        None => code,
    };
    let is_header = paren == 0
        && last != '{'
        && last != ';'
        && last != ':'
        && HEADER_KW.iter().any(|kw| starts_with_keyword(code, kw))
        && HEADER_KW.iter().any(|kw| starts_with_keyword(tail, kw));

    LineScan {
        brace_delta: brace,
        paren_delta: paren,
        ends_in_block_comment: in_bc,
        is_label,
        is_header,
    }
}

/// True when `code` begins with `kw` as a whole word (next char is not an
/// identifier character), so `do` matches but `double` does not.
fn starts_with_keyword(code: &str, kw: &str) -> bool {
    code.strip_prefix(kw)
        .is_some_and(|rest| rest.chars().next().is_none_or(|c| !is_ident_char(c)))
}

fn is_ident_char(c: char) -> bool {
    c.is_alphanumeric() || c == '_'
}

/// Verify the safety invariant across two texts: the ordered sequence of
/// non-blank lines, each trimmed of surrounding whitespace, is byte-identical.
///
/// This is what guarantees the re-indenter only ever changes *whitespace* and
/// *blank-line runs*: no code or comment line can be added, dropped, reordered,
/// or edited — only its indentation, its trailing whitespace, and the number of
/// blank lines around it may differ. In particular a trailing comment can never
/// detach from the operand it annotates.
#[must_use]
pub fn content_preserved(before: &str, after: &str) -> bool {
    fn norm(s: &str) -> Vec<&str> {
        s.split('\n')
            .map(str::trim)
            .filter(|l| !l.is_empty())
            .collect()
    }
    norm(before) == norm(after)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn content_is_preserved() {
        let src = "int f(void){\nreturn 1;\n}\n";
        let out = reindent_source(src);
        assert!(content_preserved(src, &out));
    }

    #[test]
    fn nests_braces_by_depth() {
        let src = "int f(void)\n{\nwhile( i < n ) {\nx += 1;\ni++;\n}\nreturn x;\n}\n";
        let out = reindent_source(src);
        assert_eq!(
            out,
            "int f(void)\n{\n   while( i < n ) {\n      x += 1;\n      i++;\n   }\n   return x;\n}\n"
        );
    }

    #[test]
    fn is_idempotent() {
        let src = "int f(void)\n{\nif( a ) {\ndo_x();\n}\n}\n";
        let once = reindent_source(src);
        let twice = reindent_source(&once);
        assert_eq!(once, twice);
    }

    #[test]
    fn preserves_trailing_predicate_comments_verbatim() {
        // The load-bearing candlestick case: `//` anchored to an `&&` operand.
        let src = "if( a &&        // 1st\nb &&      // 2nd\nc )        // 3rd\n{\nx = 1;\n}\n";
        let out = reindent_source(src);
        // Every comment survives byte-for-byte, still attached to its operand.
        assert!(out.contains("a &&        // 1st"));
        assert!(out.contains("b &&      // 2nd"));
        assert!(out.contains("c )        // 3rd"));
        assert!(content_preserved(src, &out));
    }

    #[test]
    fn indents_split_predicate_continuation() {
        let src = "if( a &&\nb &&\nc )\n{\nx = 1;\n}\n";
        let out = reindent_source(src);
        let lines: Vec<&str> = out.lines().collect();
        assert_eq!(lines[0], "if( a &&");
        assert_eq!(lines[1], "   b &&"); // continuation inside open paren
        assert_eq!(lines[2], "   c )");
        assert_eq!(lines[3], "{");
        assert_eq!(lines[4], "   x = 1;");
    }

    #[test]
    fn indents_unbraced_body() {
        let src = "for( i = 0; i < n; i++ )\nsum += a[i];\nreturn sum;\n";
        let out = reindent_source(src);
        let lines: Vec<&str> = out.lines().collect();
        assert_eq!(lines[0], "for( i = 0; i < n; i++ )");
        assert_eq!(lines[1], "   sum += a[i];"); // single braceless statement
        assert_eq!(lines[2], "return sum;"); // back to base level
    }

    #[test]
    fn does_not_bump_after_self_contained_one_line_if() {
        // The body is on the header's own line, so the next statement is a
        // sibling, not the header's.
        let src = "if( allocated ) { free( p ); }\n*outBegIdx = 0;\n";
        let out = reindent_source(src);
        let lines: Vec<&str> = out.lines().collect();
        assert_eq!(lines[0], "if( allocated ) { free( p ); }");
        assert_eq!(lines[1], "*outBegIdx = 0;");
    }

    #[test]
    fn bumps_after_a_one_line_if_left_dangling_by_else() {
        // The `{ ... }` closed here, but the trailing `else` still owns the
        // next statement -- the whole point of judging the tail, not the line.
        let src = "if( a ) { g(); } else\nh();\ni();\n";
        let out = reindent_source(src);
        let lines: Vec<&str> = out.lines().collect();
        assert_eq!(lines[0], "if( a ) { g(); } else");
        assert_eq!(lines[1], "   h();");
        assert_eq!(lines[2], "i();");
    }

    #[test]
    fn does_not_bump_braced_body_after_header() {
        // `do` followed by `{` on the next line must not get an extra level.
        let src = "do\n{\nx = 1;\n} while( x < 3 );\n";
        let out = reindent_source(src);
        let lines: Vec<&str> = out.lines().collect();
        assert_eq!(lines[0], "do");
        assert_eq!(lines[1], "{");
        assert_eq!(lines[2], "   x = 1;");
        assert_eq!(lines[3], "} while( x < 3 );");
    }

    #[test]
    fn aligns_block_comment_stars() {
        let src = "int f(void)\n{\n/* line one\n* line two\n*/\nreturn 0;\n}\n";
        let out = reindent_source(src);
        let lines: Vec<&str> = out.lines().collect();
        assert_eq!(lines[2], "   /* line one");
        assert_eq!(lines[3], "    * line two"); // star aligned under `/*`
        assert_eq!(lines[4], "    */");
    }

    #[test]
    fn indents_switch_cases() {
        let src =
            "switch( x )\n{\ncase 1:\ny = 1;\nbreak;\ndefault:\ny = 0;\n}\n";
        let out = reindent_source(src);
        let lines: Vec<&str> = out.lines().collect();
        assert_eq!(lines[0], "switch( x )");
        assert_eq!(lines[1], "{");
        assert_eq!(lines[2], "   case 1:");
        assert_eq!(lines[3], "      y = 1;");
        assert_eq!(lines[4], "      break;");
        assert_eq!(lines[5], "   default:");
        assert_eq!(lines[6], "      y = 0;");
        assert_eq!(lines[7], "}");
    }

    #[test]
    fn braces_inside_strings_do_not_count() {
        let src = "int f(void)\n{\nprintf(\"{{{\");\nreturn 0;\n}\n";
        let out = reindent_source(src);
        let lines: Vec<&str> = out.lines().collect();
        assert_eq!(lines[2], "   printf(\"{{{\");");
        assert_eq!(lines[3], "   return 0;");
    }

    #[test]
    fn strips_trailing_whitespace() {
        let src = "int x = 1;   \nint y = 2;\t\n";
        let out = reindent_source(src);
        assert_eq!(out, "int x = 1;\nint y = 2;\n");
    }

    #[test]
    fn collapses_blank_line_runs() {
        // The add.c case: 3 consecutive blank lines collapse to one.
        let src = "int f(void)\n{\nint i;\n\n\n\nreturn i;\n}\n";
        let out = reindent_source(src);
        assert_eq!(out, "int f(void)\n{\n   int i;\n\n   return i;\n}\n");
    }

    #[test]
    fn drops_leading_and_trailing_blank_lines() {
        let src = "\n\nint x = 1;\n\n\n";
        let out = reindent_source(src);
        assert_eq!(out, "int x = 1;\n");
    }

    #[test]
    fn preserves_blank_lines_inside_block_comments() {
        // A truly blank line inside `/* ... */` is comment layout the parser
        // captures verbatim — it must survive (collapsing it would change the
        // generated output). Note: no leading `*`, so it is a real blank line.
        let src = "/* one\n\n\ntwo */\nint x = 1;\n";
        let out = reindent_source(src);
        assert_eq!(out, "/* one\n\n\ntwo */\nint x = 1;\n");
    }

    #[test]
    fn idempotent_with_all_transforms() {
        let src = "int f(void){  \nint i;\n\n\n\nreturn i;   \n}\n\n\n";
        let once = reindent_source(src);
        let twice = reindent_source(&once);
        assert_eq!(once, twice);
        assert!(content_preserved(src, &once));
    }
}
