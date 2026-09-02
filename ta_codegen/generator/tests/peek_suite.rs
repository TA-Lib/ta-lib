//! `Peek` commits nothing — swept over the REAL ta_codegen/input corpus.
//!
//! `Peek` runs the transition on a stack copy of the handle, inline — the frame
//! has one caller and always will, since a cross-indicator call enters the
//! callee's PUBLIC `Peek`. The copy carries the scalars and the in-struct
//! arrays; it CANNOT carry the buffers, which are heap pointers it shares with
//! the live handle. What keeps those read-only is the frame itself: each store
//! lives in two locals and each load that could reach the stored slot selects
//! them back.
//!
//! Two properties are load-bearing and neither is visible at runtime:
//!
//! - **No store reaches a buffer.** `stream_verify` peeks bar t and then
//!   updates bar t with the same arguments, so a peek that wrote what the
//!   update was about to write leaves a bit-identical handle and identical
//!   values. That whole class is invisible there and has to be caught here.
//! - **Peek fuses where update fuses.** `expr_is_float_typed` classifies a
//!   `Ternary` by its THEN arm and reads `Var` and `ArrayAccess` through
//!   disjoint name lists, so a select that put the shadow local in the THEN arm
//!   would silently unfuse a multiply-add that update still fuses — a ~1 ULP
//!   divergence in the one comparison ta_regtest declares must be bitwise.

use std::collections::{BTreeSet, HashMap};
use std::path::Path;

use ta_codegen_lib::backends::c_stream;
use ta_codegen_lib::helper_registry::HelperRegistry;
use ta_codegen_lib::ir;
use ta_codegen_lib::parser;
use ta_codegen_lib::registry::Registry;

fn input_dir() -> std::path::PathBuf {
    Path::new(env!("CARGO_MANIFEST_DIR")).join("../input")
}

/// Every indicator directory in the input tree.
fn indicators() -> Vec<String> {
    let mut v: Vec<String> = std::fs::read_dir(input_dir())
        .expect("input dir")
        .filter_map(Result::ok)
        .filter(|e| e.path().is_dir())
        .filter_map(|e| {
            let name = e.file_name().to_string_lossy().to_string();
            e.path().join(format!("{name}.yaml")).exists().then_some(name)
        })
        .collect();
    v.sort();
    v
}

fn load(name: &str) -> Option<(ir::FuncDef, HashMap<String, ir::EnumDef>)> {
    let base = input_dir();
    let enums = parser::enums::load_enums(&base.join("enums.yaml"));
    let mut func = parser::yaml::parse_yaml(&base.join(format!("{name}/{name}.yaml")));
    if !func.streaming {
        return None;
    }
    let parsed = parser::c_source::parse_c_source(&base.join(format!("{name}/{name}.c")));
    parser::c_source::wire_parsed_source(&mut func, &parsed);
    Some((func, enums))
}

fn stream_c(func: &ir::FuncDef, enums: &HashMap<String, ir::EnumDef>) -> String {
    let registry = Registry::from_dir(&input_dir());
    let helpers = HelperRegistry::from_dir(&input_dir());
    c_stream::generate(func, enums, &registry, &helpers)
}

/// The brace-balanced body of the definition whose signature line contains
/// `needle`, or `None` when the tier emits no such function.
fn body_of(src: &str, needle: &str) -> Option<String> {
    let i = src.find(needle)?;
    let j = src[i..].find('{')? + i;
    let bytes = src.as_bytes();
    let (mut depth, mut k) = (0usize, j);
    loop {
        match bytes[k] {
            b'{' => depth += 1,
            b'}' => {
                depth -= 1;
                if depth == 0 {
                    break;
                }
            }
            _ => {}
        }
        k += 1;
    }
    Some(src[j..=k].to_string())
}

/// Every buffer the handle owns, read off the SHIPPED struct rather than from a
/// list of name prefixes: any pointer-to-scalar field is a buffer. Deriving it
/// from the struct is the point — a prefix list restates what the transform
/// keys on, so one omission would blind the transform and this gate together.
fn handle_buffers(src: &str, upper: &str) -> BTreeSet<String> {
    let Some(body) = body_of(src, &format!("struct TA_{upper}_Stream {{")) else {
        return BTreeSet::new();
    };
    let mut out = BTreeSet::new();
    for line in body.lines() {
        let l = line.trim().trim_end_matches(';');
        let Some((ty, name)) = l.rsplit_once('*') else { continue };
        let name = name.trim();
        if name.is_empty() || !name.chars().all(|c| c.is_alphanumeric() || c == '_') {
            continue;
        }
        // A sub-stream handle is a pointer too, and peek routes into it by
        // calling its own Peek — it is not a buffer this frame indexes.
        if ty.contains("_Stream") {
            continue;
        }
        out.insert(name.to_string());
    }
    out
}

/// Every `sp-><buffer>[...]` store in `body`, as `(buffer, whole line)`.
fn buffer_stores(body: &str, buffers: &BTreeSet<String>) -> Vec<(String, String)> {
    let mut out = Vec::new();
    for line in body.lines() {
        // `sp->` in a frame, `stream->` in the two tiers that hand-roll Peek —
        // keying on one of them made the sweep of those two vacuous.
        let Some((pos, skip)) = line
            .find("sp->")
            .map(|i| (i, 4))
            .or_else(|| line.find("stream->").map(|i| (i, 8)))
        else {
            continue;
        };
        let rest = &line[pos + skip..];
        let Some(br) = rest.find('[') else { continue };
        let name = &rest[..br];
        if !buffers.contains(name) {
            continue;
        }
        // Balance the index so `] =` is found on the real closing bracket.
        let mut depth = 0usize;
        let mut end = None;
        for (k, c) in rest[br..].char_indices() {
            match c {
                '[' => depth += 1,
                ']' => {
                    depth -= 1;
                    if depth == 0 {
                        end = Some(br + k);
                        break;
                    }
                }
                _ => {}
            }
        }
        let Some(end) = end else { continue };
        let after = rest[end + 1..].trim_start();
        let is_store = after.starts_with('=') && !after.starts_with("==")
            || after.starts_with("+=")
            || after.starts_with("-=");
        if is_store {
            out.push((name.to_string(), line.trim().to_string()));
        }
    }
    out
}

/// The whole point: a peek frame stores into no handle buffer, in any tier.
///
/// Both directions are asserted — the update frames must between them carry the
/// stores this looks for, or an emitter that dropped every buffer would pass.
#[test]
fn a_peek_frame_stores_into_no_handle_buffer() {
    let mut peek_frames = 0usize;
    let mut update_stores = 0usize;
    let mut buffers_seen = 0usize;
    let mut offenders: Vec<String> = Vec::new();

    for name in indicators() {
        let Some((func, enums)) = load(&name) else { continue };
        let src = stream_c(&func, &enums);
        let upper = func.name.to_uppercase();
        // The public entry too, so the two tiers that hand-roll it and emit no
        // frame of their own — Dispatch (MA) and PeriodBank (MAVP), both pure
        // delegation — are swept rather than silently skipped.
        let buffers = handle_buffers(&src, &upper);
        buffers_seen += buffers.len();
        let mut bodies: Vec<String> = Vec::new();
        if let Some(f) = body_of(&src, &format!("TA_{upper}_PeekImpl(")) {
            bodies.push(f);
        }
        bodies.push(
            body_of(&src, &format!("TA_RetCode TA_{upper}_Peek("))
                .unwrap_or_else(|| panic!("{upper} streams but emits no Peek")),
        );
        peek_frames += 1;
        for body in &bodies {
            for (buf, line) in buffer_stores(body, &buffers) {
                offenders.push(format!("{upper}: {buf} <- {line}"));
            }
        }
        if let Some(step) = body_of(&src, &format!("TA_{upper}_StepImpl(")) {
            update_stores += buffer_stores(&step, &buffers).len();
        }
    }

    assert!(peek_frames > 170, "only {peek_frames} peek entry points swept");
    assert!(
        buffers_seen > 150,
        "only {buffers_seen} handle buffer(s) found across the corpus, so the store scan \
         is looking for something that is not there"
    );
    assert!(
        update_stores > 200,
        "the update frames carry only {update_stores} buffer stores, so the peek sweep \
         proves nothing"
    );
    assert!(
        offenders.is_empty(),
        "a peek frame stores into a buffer the handle shares with the live stream \
         ({} site(s)):\n{}",
        offenders.len(),
        offenders.join("\n")
    );
}

/// A peek frame never drives a sub-stream's `Update`. That call is the composed
/// and dispatch tiers' only way to write the live handle — the sub-handles are
/// heap pointers the struct copy shares — and it carries no subscript, so the
/// store sweep above cannot see it.
#[test]
fn no_peek_entry_point_commits_a_sub_stream() {
    let mut with_subs = 0usize;
    let mut offenders: Vec<String> = Vec::new();
    for name in indicators() {
        let Some((func, enums)) = load(&name) else { continue };
        let src = stream_c(&func, &enums);
        let upper = func.name.to_uppercase();
        let Some(peek) = body_of(&src, &format!("TA_RetCode TA_{upper}_Peek(")) else { continue };
        if peek.contains("_Peek(") {
            with_subs += 1;
        }
        for line in peek.lines() {
            if line.contains("_Update(") {
                offenders.push(format!("{upper}: {}", line.trim()));
            }
        }
    }
    assert!(
        with_subs >= 12,
        "only {with_subs} peek entry point(s) drive a sub-stream, so this gate is not \
         watching the tiers it exists for"
    );
    assert!(
        offenders.is_empty(),
        "a peek entry point commits a sub-stream:\n{}",
        offenders.join("\n")
    );
}

/// Every exit from a `Peek` answers a code. The transition's own early exit —
/// the param-degenerate identity short-circuit — is valueless, because the
/// transition is `void`; inlined into `Peek` it would be a `return;` in a
/// function returning `TA_RetCode`, which C accepts with a warning and answers
/// with whatever was in the return register. Measured before the fix:
/// `EMA(1) stream Peek: retCode 2`.
#[test]
fn every_return_in_a_peek_answers_a_code() {
    let mut early_exits = 0usize;
    let mut with_short_circuit = 0usize;
    let mut offenders: Vec<String> = Vec::new();
    for name in indicators() {
        let Some((func, enums)) = load(&name) else { continue };
        let src = stream_c(&func, &enums);
        let upper = func.name.to_uppercase();
        let Some(peek) = body_of(&src, &format!("TA_RetCode TA_{upper}_Peek(")) else { continue };
        // More than the tail return means the body carries an early exit — the
        // identity short-circuit, which is exactly where a valueless one lives.
        if peek.matches("return TA_SUCCESS;").count() > 1 {
            with_short_circuit += 1;
        }
        for line in peek.lines() {
            let l = line.trim();
            if l == "return;" {
                offenders.push(format!("{upper}: {l}"));
            } else if l.starts_with("return ") {
                early_exits += 1;
            }
        }
    }
    assert!(
        early_exits > 150 && with_short_circuit >= 5,
        "{early_exits} answered exit(s) over {with_short_circuit} function(s) with an \
         early exit — too few for this to be checking the shape the defect lived in"
    );
    assert!(
        offenders.is_empty(),
        "a Peek exits without answering a code:\n{}",
        offenders.join("\n")
    );
}


/// Replace every `sp-><buffer>[...]` with one token, so two bodies that differ
/// only in which slot they name compare equal.
fn mask_buffer_reads(body: &str, buffers: &BTreeSet<String>) -> String {
    let b: Vec<char> = body.chars().collect();
    let mut out = String::with_capacity(body.len());
    let mut i = 0usize;
    while i < b.len() {
        if b[i..].starts_with(&['s', 'p', '-', '>']) {
            let rest: String = b[i + 4..].iter().collect();
            if let Some(br) = rest.find('[') {
                let name = &rest[..br];
                if buffers.contains(name) {
                    // Skip to the bracket that closes this subscript.
                    let mut depth = 0usize;
                    let mut k = i + 4 + br;
                    while k < b.len() {
                        match b[k] {
                            '[' => depth += 1,
                            ']' => {
                                depth -= 1;
                                if depth == 0 {
                                    break;
                                }
                            }
                            _ => {}
                        }
                        k += 1;
                    }
                    out.push_str("<BUF>");
                    i = k + 1;
                    continue;
                }
            }
        }
        out.push(b[i]);
        i += 1;
    }
    out
}

/// Collapse `(<idx> != pkSlotN) ? <read> : pkValN` down to `<read>` — the
/// inverse of the peek rewrite, so what is left is the expression update
/// renders. Per LINE, and only where the `pkSlotN` found is the comparison:
/// every peek body declares its shadows first, and a scan that started from a
/// declaration would find no comparison to unwind and collapse nothing.
fn undo_selects(line: &str) -> String {
    let mut s = line.to_string();
    while let Some((lo, hi, arm)) = leftmost_select(&s) {
        s = format!("{}{}{}", &s[..lo], arm, &s[hi..]);
    }
    s
}

/// The span of the leftmost select and the read it picks.
fn leftmost_select(s: &str) -> Option<(usize, usize, String)> {
    let b = s.as_bytes();
    let hit = s.find("pkSlot")?;
    let mut name_end = hit;
    while name_end < b.len() && (b[name_end].is_ascii_alphanumeric() || b[name_end] == b'_') {
        name_end += 1;
    }
    if !s[name_end..].starts_with(") ? ") {
        return None;
    }
    // Back to the `(` that opens the comparison.
    let mut depth = 0i32;
    let mut open = None;
    for k in (0..hit).rev() {
        match b[k] {
            b')' => depth += 1,
            b'(' => {
                if depth == 0 {
                    open = Some(k);
                    break;
                }
                depth -= 1;
            }
            _ => {}
        }
    }
    let open = open?;
    // Past `) ? ` to the arm, then over the arm to ` : pkValN`.
    let arm_start = name_end + 4;
    let mut depth = 0i32;
    let mut colon = None;
    for (k, c) in b.iter().enumerate().skip(arm_start) {
        match *c {
            b'(' | b'[' => depth += 1,
            b')' | b']' => {
                if depth == 0 {
                    break;
                }
                depth -= 1;
            }
            b':' if depth == 0 => {
                colon = Some(k);
                break;
            }
            _ => {}
        }
    }
    let arm = s[arm_start..colon?].trim().to_string();
    let mut hi = colon? + s[colon?..].find("pkVal")?;
    while hi < b.len() && (b[hi].is_ascii_alphanumeric() || b[hi] == b'_') {
        hi += 1;
    }
    // The rewrite wraps the select in its own parens, and those must go with it
    // — but only when they ARE its own: `fabs(<select>)` opens and closes the
    // same way and taking that pair leaves the line unbalanced.
    let wrapped = open > 0
        && b[open - 1] == b'('
        && hi < b.len()
        && b[hi] == b')'
        && !(open >= 2 && (b[open - 2].is_ascii_alphanumeric() || b[open - 2] == b'_'));
    Some(if wrapped { (open - 1, hi + 1, arm) } else { (open, hi, arm) })
}

/// `(` minus `)` over `text`.
fn paren_balance(text: &str) -> i32 {
    text.bytes().map(|c| i32::from(c == b'(') - i32::from(c == b')')).sum()
}

/// A real `fma(` token at `i`, not the tail of a longer identifier.
fn fma_at(b: &[char], i: usize) -> bool {
    b[i..].starts_with(&['f', 'm', 'a', '('])
        && (i == 0 || !(b[i - 1].is_alphanumeric() || b[i - 1] == '_'))
}

/// Every `fma(...)` call `body` both opens and closes, in source order — one
/// nested in another call's arguments included, so the list is what the frame
/// fuses rather than what its outermost calls hide.
fn fma_calls(body: &str) -> Vec<String> {
    let b: Vec<char> = body.chars().collect();
    let mut out = Vec::new();
    for i in (0..b.len()).filter(|&i| fma_at(&b, i)) {
        let (mut depth, mut k) = (0usize, i + 3);
        while k < b.len() {
            match b[k] {
                '(' => depth += 1,
                ')' => depth -= 1,
                _ => {}
            }
            if depth == 0 {
                out.push(b[i..=k].iter().collect::<String>().split_whitespace().collect::<Vec<_>>().join(" "));
                break;
            }
            k += 1;
        }
    }
    out
}

/// How many `fma(` `text` opens, closed or not.
fn fma_opens(text: &str) -> usize {
    let b: Vec<char> = text.chars().collect();
    (0..b.len()).filter(|&i| fma_at(&b, i)).count()
}

/// `line` split at its top-level assignment operator, as `(target, the
/// statement with its whitespace collapsed)`.
fn assignment(line: &str) -> Option<(String, String)> {
    let l = line.trim();
    let b: Vec<char> = l.chars().collect();
    let mut depth = 0i32;
    for (i, c) in b.iter().enumerate() {
        match c {
            '(' | '[' => depth += 1,
            ')' | ']' => depth -= 1,
            '=' if depth == 0 => {
                if b.get(i + 1) == Some(&'=') || (i > 0 && matches!(b[i - 1], '=' | '!' | '<' | '>')) {
                    return None;
                }
                let lhs: String = b[..i].iter().collect();
                let lhs = lhs.trim_end_matches(['+', '-', '*', '/', '&', '|', '^', '%', ' ']);
                let stmt = l.split_whitespace().collect::<Vec<_>>().join(" ");
                return (!lhs.is_empty()).then(|| (lhs.to_string(), stmt));
            }
            _ => {}
        }
    }
    None
}

/// Every `fma(...)` in `body` keyed to the target of the assignment carrying
/// it. `<expr>` names a call that sits in no assignment.
fn fma_sites(body: &str) -> Vec<(String, String)> {
    let mut out = Vec::new();
    for line in body.lines() {
        let calls = fma_calls(line);
        if calls.is_empty() {
            continue;
        }
        let lhs = assignment(line).map_or_else(|| "<expr>".to_string(), |(t, _)| t);
        out.extend(calls.into_iter().map(|c| (lhs.clone(), c)));
    }
    out
}

/// The statements of `body` assigning one of `targets`, in order.
fn assignments_to(body: &str, targets: &BTreeSet<String>) -> Vec<String> {
    body.lines()
        .filter_map(assignment)
        .filter_map(|(t, s)| targets.contains(&t).then_some(s))
        .collect()
}

/// A frame body with the peek rewrite undone: buffer subscripts masked to one
/// token, selects collapsed to the arm update renders. Answers how many
/// statements the collapse touched, which is the only thing that says it ran.
fn unrewritten(body: &str, buffers: &BTreeSet<String>) -> (String, usize, usize) {
    let masked = mask_buffer_reads(body, buffers);
    let (mut collapsed, mut unbalanced) = (0usize, 0usize);
    let out: Vec<String> = masked
        .lines()
        .map(|l| {
            let u = undo_selects(l);
            collapsed += usize::from(u != l);
            // The collapse removes matched pairs, so it cannot move the line's
            // balance. A count of CHANGED lines cannot tell a working collapse
            // from one that corrupts a fifth of them; this can.
            unbalanced += usize::from(paren_balance(&u) != paren_balance(l));
            u
        })
        .collect();
    (out.join("\n"), collapsed, unbalanced)
}

/// Whether `small` is a subsequence of `big`, answering the index in `small`
/// that ran out of matches.
fn subsequence<T: PartialEq>(small: &[T], big: &[T]) -> Result<(), usize> {
    let mut rest = big;
    for (i, s) in small.iter().enumerate() {
        match rest.iter().position(|x| x == s) {
            Some(k) => rest = &rest[k + 1..],
            None => return Err(i),
        }
    }
    Ok(())
}

/// Peek renders every multiply-add it still EVALUATES exactly as update renders
/// it, and fuses nothing update does not.
///
/// Whether a site survived is decided from the peek text by the assignment
/// TARGET — nothing done to the expression moves it — because one list cannot
/// answer both halves:
///
/// - **Order, and no fusion of peek's own.** Peek's calls are a subsequence of
///   update's, arguments and all. A count is blind to a product moving across
///   the fusion boundary, which `canonicalize_accumulator_add` can do on its
///   own: it matches the accumulator by raw string equality against the target,
///   and the peek rewrite is what first changes one (`buf[k]` -> `pkValN`).
/// - **Deletion accounting.** For every target update fuses into, the
///   statements peek still assigns to it are a subsequence of update's BY WHOLE
///   STATEMENT. Neither the leg above nor pairing each call with its target can
///   carry this: unfusing a site DELETES it from a list of calls, leaving a
///   subsequence, which is green on exactly the divergence this exists for.
///
/// A subsequence and not a prefix because peek stops at its last output store
/// and then drops every store to a temp nothing reads, so the sites it no
/// longer evaluates go missing from the MIDDLE. Nothing is given up: the frozen
/// `stream_fma` sets guard both frames, so a deletion cannot re-classify a site
/// peek still evaluates, and the second leg is what proves a site was deleted
/// rather than unfused.
///
/// Three shapes are refused rather than handled, all absent today: an `fma(`
/// that does not close on its own line, which the per-line keying would
/// mis-read; a multiply-add stored into a buffer slot, whose target the peek
/// rewrite renames, so the anchor could not follow it; and a call in no
/// assignment, which has no anchor and so must survive verbatim.
#[test]
fn a_peek_frame_fuses_every_multiply_add_it_still_evaluates() {
    let (mut pairs, mut fusing, mut sites, mut anchors) = (0usize, 0usize, 0usize, 0usize);
    let (mut aligned, mut unanchored, mut collapsed) = (0usize, 0usize, 0usize);
    let mut refused: Vec<String> = Vec::new();
    let mut mismatches: Vec<String> = Vec::new();

    for name in indicators() {
        let Some((func, enums)) = load(&name) else { continue };
        let src = stream_c(&func, &enums);
        let upper = func.name.to_uppercase();
        let (Some(step), Some(peek)) = (
            body_of(&src, &format!("TA_{upper}_StepImpl(")),
            body_of(&src, &format!("TA_RetCode TA_{upper}_Peek(")),
        ) else {
            continue;
        };
        pairs += 1;
        let buffers = handle_buffers(&src, &upper);
        let (u, _, ub_step) = unrewritten(&step, &buffers);
        let (p, n, ub_peek) = unrewritten(&peek, &buffers);
        collapsed += n;
        if ub_step + ub_peek > 0 {
            refused.push(format!("{upper}: undoing a select left {} line(s) unbalanced", ub_step + ub_peek));
        }
        for body in [&u, &p] {
            for line in body.lines().filter(|l| fma_opens(l) != fma_calls(l).len()) {
                refused
                    .push(format!("{upper}: an `fma(` does not close on its line: {}", line.trim()));
            }
        }

        let us = fma_sites(&u);
        let ps = fma_sites(&p);
        sites += us.len();
        if us.is_empty() {
            // The anchors below are update's fused targets, so a frame with
            // none skips every leg — and peek fusing where update does not is
            // the one direction that needs no anchor to see.
            if !ps.is_empty() {
                mismatches.push(format!("{upper}: peek fuses {ps:?} where update fuses nothing"));
            }
            continue;
        }
        fusing += 1;
        let targets: BTreeSet<String> =
            us.iter().filter(|(t, _)| t != "<expr>").map(|(t, _)| t.clone()).collect();
        if targets.contains("<BUF>") || targets.iter().any(|t| t.starts_with("pkVal")) {
            refused.push(format!("{upper}: a multiply-add is stored into a buffer slot"));
        }
        anchors += targets.len();
        let calls = |v: &[(String, String)], want: bool| -> Vec<String> {
            v.iter().filter(|(t, _)| (t == "<expr>") == want).map(|(_, c)| c.clone()).collect()
        };
        let (uf, pf) = (calls(&us, false), calls(&ps, false));
        let (uc, pc) = (calls(&us, true), calls(&ps, true));
        unanchored += uc.len();
        let ua = assignments_to(&u, &targets);
        let pa = assignments_to(&p, &targets);
        aligned += pa.len();

        if let Err(i) = subsequence(&pf, &uf) {
            mismatches.push(format!(
                "{upper}: peek fuses `{}`, which is not update's next site\n    update: {uf:?}\n    peek:   {pf:?}",
                pf[i]
            ));
        } else if let Err(i) = subsequence(&pa, &ua) {
            mismatches.push(format!(
                "{upper}: peek evaluates `{}`, which update does not render that way\n    update: {ua:?}\n    peek:   {pa:?}",
                pa[i]
            ));
        } else if uc != pc {
            mismatches.push(format!(
                "{upper}: a multiply-add in no assignment differs\n    update: {uc:?}\n    peek:   {pc:?}"
            ));
        }
    }

    assert!(refused.is_empty(), "a shape this gate cannot anchor:\n{}", refused.join("\n"));
    assert!(
        pairs >= 170 && fusing >= 25 && sites >= 110 && anchors >= 80 && unanchored >= 4,
        "{pairs} frame pair(s), {fusing} fusing, {sites} update site(s) over {anchors} \
         target(s), {unanchored} unanchored — too few for this to be measuring anything"
    );
    assert!(
        aligned >= 140 && collapsed >= 170,
        "{aligned} peek statement(s) aligned on a fused target and {collapsed} select(s) \
         collapsed — each leg needs its own floor, or one can go dead while the other \
         reads green"
    );
    assert!(
        mismatches.is_empty(),
        "a peek frame does not render a multiply-add the way its update frame does, which \
         is a silent ~1 ULP divergence in a comparison that must be bitwise:\n{}",
        mismatches.join("\n")
    );
}

/// The mirror buffers and the composed tier's `peekMode` are gone, and nothing
/// re-grows them: both existed only to let ONE step body serve both frames.
#[test]
fn no_tier_carries_a_peek_mirror_or_a_routing_flag() {
    // A pure-absence sweep reads the same whether it examined 176 functions or
    // none, which is why every test here counts what it looked at.
    let mut swept = 0usize;
    for name in indicators() {
        let Some((func, enums)) = load(&name) else { continue };
        swept += 1;
        let src = stream_c(&func, &enums);
        assert!(
            !src.contains("Mirror"),
            "{}: a peek scratch mirror is a per-handle copy of every buffer, which is \
             what the peek frame exists to retire",
            func.name
        );
        assert!(
            !src.contains("peekMode"),
            "{}: routing one step body between frames costs `update` a branch per \
             sub-call — the peek frame is its own body now",
            func.name
        );
    }
    assert!(swept > 170, "only {swept} function(s) examined");
}

/// SMA is the whole mechanism in one function: the degenerate `cap == 0` store
/// is loaded back by the trailing read (`ringPos` is 0 exactly then), so it
/// becomes a shadow and that read becomes a select; the tail push is loaded by
/// nothing, so it is deleted rather than shadowed.
#[test]
fn sma_shadows_the_store_its_own_bar_reads_and_deletes_the_one_it_does_not() {
    let (func, enums) = load("sma").expect("SMA streams");
    let src = stream_c(&func, &enums);
    let peek = body_of(&src, "TA_RetCode TA_SMA_Peek(").expect("SMA has a Peek");
    let step = body_of(&src, "TA_SMA_StepImpl(").expect("SMA has an update frame");

    assert_eq!(
        step.matches("sp->ring_trailingIdx_inReal[").count(),
        3,
        "update reads the trailing slot and stores twice (the cap-0 guard and the \
         tail push): {step}"
    );
    assert!(
        peek.contains("pkSlot0 = 0;") && peek.contains("pkVal0 = inReal;"),
        "the cap-0 guard becomes a shadow: {peek}"
    );
    assert!(
        peek.contains(
            "(sp->ringPos_trailingIdx != pkSlot0) ? sp->ring_trailingIdx_inReal[sp->ringPos_trailingIdx] : pkVal0"
        ),
        "the trailing read selects the shadow, with the array in the THEN arm so the \
         operand classifies as it does in update: {peek}"
    );
    assert!(
        !peek.contains("pkSlot1"),
        "the tail push is loaded by nothing this bar, so it is deleted, not shadowed: {peek}"
    );
    assert_eq!(
        peek.matches("sp->ring_trailingIdx_inReal[").count(),
        1,
        "exactly one load survives, and it is a load: {peek}"
    );
}

/// The handle's fixed-size REAL accumulator fields — the set
/// `transition_buffers_with_state_arrays` offers. `handle_buffers` above cannot
/// see them: it keys on `*`, and these are in the struct.
fn handle_accumulators(src: &str, upper: &str) -> BTreeSet<String> {
    let Some(body) = body_of(src, &format!("struct TA_{upper}_Stream {{")) else {
        return BTreeSet::new();
    };
    let mut out = BTreeSet::new();
    for line in body.lines() {
        let l = line.trim().trim_end_matches(';');
        if l.contains('*') || !l.ends_with(']') || !l.starts_with("double ") {
            continue;
        }
        let Some((decl, size)) = l.rsplit_once('[') else { continue };
        if !size.trim_end_matches(']').chars().all(|c| c.is_ascii_digit()) {
            continue;
        }
        let Some((_, name)) = decl.rsplit_once(' ') else { continue };
        if !name.is_empty() && name.chars().all(|c| c.is_alphanumeric() || c == '_') {
            out.insert(name.to_string());
        }
    }
    out
}

/// A peek frame deletes EVERY accumulator store, so Java and C# have nothing
/// to copy the field for. A function that lands one the frame must keep fails
/// here rather than quietly reintroducing a copy per peek.
#[test]
fn a_peek_frame_deletes_every_accumulator_store() {
    let mut with_accumulators = 0usize;
    let mut step_stores = 0usize;
    let mut unhandled: Vec<String> = Vec::new();
    let mut kept: Vec<String> = Vec::new();

    for name in indicators() {
        let Some((func, enums)) = load(&name) else { continue };
        let src = stream_c(&func, &enums);
        let upper = func.name.to_uppercase();
        let accs = handle_accumulators(&src, &upper);
        if accs.is_empty() {
            continue;
        }
        let Some(peek) = body_of(&src, &format!("TA_RetCode TA_{upper}_Peek(")) else {
            unhandled.push(format!("{upper}: holds an accumulator but emits no Peek"));
            continue;
        };
        // Reported, not skipped: no such handle exists today.
        let Some(step) = body_of(&src, &format!("TA_{upper}_StepImpl(")) else {
            unhandled.push(format!("{upper}: holds an accumulator but emits no StepImpl"));
            continue;
        };
        let stored = buffer_stores(&step, &accs).len();
        if stored == 0 {
            continue; // nothing for the frame to delete
        }
        with_accumulators += 1;
        step_stores += stored;
        let left = buffer_stores(&peek, &accs);
        if !left.is_empty() {
            kept.push(format!("{upper}: {} accumulator store(s)", left.len()));
        }
    }

    assert!(unhandled.is_empty(), "unswept handle(s):\n{}", unhandled.join("\n"));
    assert!(
        with_accumulators >= 21,
        "only {with_accumulators} function(s) store into a fixed-size accumulator, so this \
         sweep is looking for something that is not there"
    );
    assert!(
        step_stores >= 60,
        "the update frames carry only {step_stores} accumulator store(s), so the peek \
         sweep proves nothing"
    );
    assert!(
        kept.is_empty(),
        "a peek frame keeps an accumulator store, so Java and C# copy the field \
         again ({} function(s)):\n{}",
        kept.len(),
        kept.join("\n")
    );
}

/// A peek frame stops at its last output store.
///
/// The commit tail below it exists for the NEXT bar, and a peek has none: the
/// ring advance, the rescan-window advance and the cursor-parity flip are the
/// three shapes it takes, and none may survive into a frame. Not only
/// bookkeeping — the arithmetic a source runs to set up the next bar goes with
/// them, which is where the win is (`HT_PHASOR` below).
///
/// Both directions, and that is the point of the step-side floors: every OTHER
/// sweep in this file is monotone under deletion — they all get more true as
/// the frame shrinks — so a trim that silently stopped trimming would read
/// green on all of them and on the runtime legs too.
#[test]
fn a_peek_frame_stops_at_its_last_output_store() {
    // A store, not a read: the marker also appears as the subscript of every
    // trailing read, and on the left of a shadow select's `!=` and of the wrap
    // guard's `>=`. So a store is `= `, a compound `?= `, or a `++`/`--` —
    // anything whose `=` is preceded by a comparison operator is not one.
    fn stores_to(body: &str, prefix: &str) -> bool {
        body.lines().map(str::trim).any(|l| {
            let Some(rest) = l.strip_prefix(prefix) else { return false };
            if rest.contains("++") || rest.contains("--") {
                return true;
            }
            rest.find('=').is_some_and(|i| {
                !rest[i + 1..].starts_with('=')
                    && !rest[..i].trim_end().ends_with(['!', '<', '>'])
            })
        })
    }

    let tail_marks = [
        ("sp->ringPos_", 60usize),
        ("sp->winPos_", 5),
        ("sp->streamParity", 5),
    ];
    let mut in_step = [0usize; 3];
    let mut offenders: Vec<String> = Vec::new();
    let mut swept = 0usize;
    let mut phasor_seen = false;

    for name in indicators() {
        let Some((func, enums)) = load(&name) else { continue };
        let src = stream_c(&func, &enums);
        let upper = func.name.to_uppercase();
        let Some(peek) = body_of(&src, &format!("TA_RetCode TA_{upper}_Peek(")) else {
            continue;
        };
        swept += 1;
        for (k, (mark, _)) in tail_marks.iter().enumerate() {
            if stores_to(&peek, mark) {
                offenders.push(format!("{upper}: a peek frame still carries `{mark}...=`"));
            }
            if body_of(&src, &format!("TA_{upper}_StepImpl(")).is_some_and(|s| stores_to(&s, mark))
            {
                in_step[k] += 1;
            }
        }
        // The concrete win, and the reason this is worth more than deleting
        // stores: HT_PHASOR's next-bar recurrence is an `atan`, four clamps and
        // three multiply-adds, all of it below the two output writes.
        if upper == "HT_PHASOR" {
            phasor_seen = true;
            assert!(
                !peek.contains("atan("),
                "HT_PHASOR: the peek frame still runs the next bar's period recurrence"
            );
            assert!(
                body_of(&src, "TA_HT_PHASOR_StepImpl(").is_some_and(|s| s.contains("atan(")),
                "HT_PHASOR: the update frame lost the recurrence, so the pin above proves nothing"
            );
        }
    }

    assert!(offenders.is_empty(), "{}", offenders.join("\n"));
    assert!(swept > 170, "only {swept} peek frame(s) examined");
    assert!(phasor_seen, "HT_PHASOR was not swept, so its pin did not run");
    for (k, (mark, floor)) in tail_marks.iter().enumerate() {
        assert!(
            in_step[k] >= *floor,
            "only {} update frame(s) carry `{mark}...=`, so the absence above is not \
             evidence of anything",
            in_step[k]
        );
    }
}

/// The locals `body` declares as a bare scalar, in declaration order. A
/// declaration with an initializer is not one: the shadow pair is written where
/// it is declared, and its reads are the selects.
fn declared_scalars(body: &str) -> Vec<String> {
    body.lines()
        .filter_map(|l| {
            let l = l.trim().strip_suffix(';')?;
            let (ty, name) = l.split_once(' ')?;
            (matches!(ty, "double" | "int" | "float" | "long" | "short" | "char")
                && !name.is_empty()
                && name.chars().all(|c| c.is_ascii_alphanumeric() || c == '_'))
            .then(|| name.to_string())
        })
        .collect()
}

/// Whether `text` names `word` other than as part of a longer identifier.
fn names_word(text: &str, word: &str) -> bool {
    text.match_indices(word).any(|(i, _)| {
        let b = text.as_bytes();
        let before = i == 0 || !(b[i - 1].is_ascii_alphanumeric() || b[i - 1] == b'_');
        let j = i + word.len();
        let after = j >= b.len() || !(b[j].is_ascii_alphanumeric() || b[j] == b'_');
        before && after
    })
}

/// Whether any statement of `body` READS `name` — which for a store to it is
/// the text right of the `=`, so `x += 1` and `x = f(x)` are told apart the way
/// the purge tells them apart. The declaration is not a statement: counting it
/// makes every local read and the sweep vacuous.
fn body_reads(body: &str, name: &str) -> bool {
    body.lines().filter(|l| declared_scalars(l).is_empty()).any(|l| {
        let scan = match assignment(l) {
            Some((t, _)) if t == name => l.split_once('=').map_or("", |(_, r)| r),
            _ => l,
        };
        names_word(scan, name)
    })
}

/// No peek frame declares a local nothing reads.
///
/// The tail trim orphans them — the readers it deletes are the recurrence for
/// the NEXT bar — and the purge is what takes the stores with them, which is
/// what lets `temps_used` drop the declaration. Nothing else would say if that
/// stopped: the shape is a `-Wall` warning in C and no CI job compiles with
/// `-Werror`, while the other three backends' compilers cannot see it at all
/// (the Rust crate allows the lint wholesale, CS0219 does not fire on a
/// non-constant, javac has no such diagnostic).
#[test]
fn no_peek_frame_declares_a_local_nothing_reads() {
    let (mut swept, mut locals) = (0usize, 0usize);
    let mut offenders: Vec<String> = Vec::new();

    for name in indicators() {
        let Some((func, enums)) = load(&name) else { continue };
        let src = stream_c(&func, &enums);
        let upper = func.name.to_uppercase();
        let Some(peek) = body_of(&src, &format!("TA_RetCode TA_{upper}_Peek(")) else { continue };
        swept += 1;
        for local in declared_scalars(&peek) {
            locals += 1;
            if !body_reads(&peek, &local) {
                offenders.push(format!("{upper}: {local}"));
            }
        }
    }

    assert!(
        swept > 170 && locals > 350,
        "{swept} peek frame(s) over {locals} local(s) — too few for this to be measuring anything"
    );
    assert!(
        offenders.is_empty(),
        "a peek frame computes into a local nothing reads, so the trim orphaned it and \
         nothing purged it ({} local(s)):\n{}",
        offenders.len(),
        offenders.join("\n")
    );
}
