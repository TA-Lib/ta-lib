//! The shared abstract row model (`backends::abstract_rows`): the one
//! derivation the Rust registry, the Java server table, the shipped Java
//! registry and the shipped C# registry all render. Split out of the
//! former `backend_suite.rs`.

#[path = "common/mod.rs"]
mod common;

use common::{all_abstract_rows, check_rust_cast_parens, generate_all, load_indicator_with_source};

// The shared abstract row model
// ---------------------------------------------------------------------------
//
// `backends::abstract_rows` is the one derivation the Rust registry, the Java
// server table, the shipped Java registry and the shipped C# registry all
// render. These pin the facts that used to be hand-maintained inside one
// backend, plus the two domains that are currently unreachable — so the day one
// appears, the sweep names the renderers that need a look.


/// The unstable-period set used to live as a 20-arm hardcoded name -> variant
/// `match` inside `rust_abstract`, duplicating `enums.yaml`. It is now resolved
/// by name (`TA_FUNC_UNST_<NAME>`), the same derivation the servers use. This
/// pins the resulting set both ways: a lost mapping and a spurious one both fail.
#[test]
fn abstract_rows_unstable_period_set_is_exactly_the_twenty() {
    // (function name, its FuncUnstId ordinal). The NAME half would be a
    // tautology on its own — `unst_row` resolves `TA_FUNC_UNST_<name>` and hands
    // the variant's name back, so it can only ever equal the function's. The
    // VALUE is the half worth pinning: it is authored in enums.yaml, it is the
    // index every backend uses into `unstablePeriod[]`, and it is ABI. A slot
    // that silently renumbers is what this table exists to catch.
    const EXPECTED: &[(&str, i32)] = &[
        ("ADX", 0),
        ("ATR", 2),
        ("CMO", 3),
        ("DX", 4),
        ("EMA", 5),
        ("HT_DCPERIOD", 6),
        ("HT_DCPHASE", 7),
        ("HT_PHASOR", 8),
        ("HT_SINE", 9),
        ("HT_TRENDLINE", 10),
        ("HT_TRENDMODE", 11),
        ("KAMA", 13),
        ("MAMA", 14),
        ("MINUS_DI", 16),
        ("MINUS_DM", 17),
        ("NATR", 18),
        ("PLUS_DI", 19),
        ("PLUS_DM", 20),
        ("RSI", 21),
        ("T3", 23),
    ];

    let rows = all_abstract_rows();
    let mut got: Vec<(String, i32)> = rows
        .iter()
        .filter_map(|r| r.unst.as_ref().map(|u| (r.name.clone(), u.value)))
        .collect();
    got.sort();
    let mut want: Vec<(String, i32)> =
        EXPECTED.iter().map(|(a, b)| ((*a).to_string(), *b)).collect();
    want.sort();
    assert_eq!(got, want, "unstable-period set or ordinal changed (name -> FuncUnstId value)");

    // The `unstable_period` function flag and the resolved id must not disagree:
    // one without the other means a function that says it is recursive but has
    // no state slot, or a slot nothing declares.
    for r in &rows {
        let flagged = r.flags & 0x0800_0000 != 0;
        assert_eq!(
            flagged,
            r.unst.is_some(),
            "{}: unstable_period flag ({flagged}) disagrees with its FuncUnstId ({:?})",
            r.name,
            r.unst.as_ref().map(|u| &u.c_name)
        );
    }
}

/// Every shipped `group:` string must parse into the closed `Group` set, and
/// every variant must render back to the exact display string C's
/// `TA_GroupString` and the YAML use.
#[test]
fn abstract_rows_group_strings_round_trip() {
    use ta_codegen_lib::backends::abstract_rows::Group;
    for g in Group::ALL {
        assert_eq!(Group::parse(g.as_str()), *g, "group round-trip for {}", g.as_str());
    }
    let rows = all_abstract_rows();
    for g in Group::ALL {
        // Not an emptiness check: every declared group must actually be used,
        // so a retired group cannot linger in the closed set unnoticed.
        assert!(
            rows.iter().any(|r| r.group == *g),
            "no shipped function is in group {}",
            g.as_str()
        );
    }
}

/// Two shapes the model can express that nothing currently declares. Pinned
/// rather than asserted away: the renderers each have an arm for them that no
/// gate exercises, so the day a definition uses one, this says so by name.
#[test]
fn abstract_rows_unreachable_domains_stay_unreachable() {
    use ta_codegen_lib::backends::abstract_rows::{InputKind, OptDomain};
    for r in all_abstract_rows() {
        for o in &r.opt_inputs {
            assert!(
                !matches!(o.domain, OptDomain::RealList { .. }),
                "{}.{} is the first real-list parameter — re-check the RealList arm in \
                 rust_abstract, java_abstract, java_metadata and csharp_metadata",
                r.name,
                o.param_name
            );
        }
        for i in &r.inputs {
            assert!(
                i.kind != InputKind::Integer,
                "{}.{} is the first integer input — re-check every registry's Integer arm \
                 and the ParamHolder/dispatch binding",
                r.name,
                i.param_name
            );
        }
    }
}

/// A price bundle is ONE parameter carrying a component bitmask, not N arrays.
/// `Core.Adx` takes three `double[]`, but `TA_FuncInfo.nbInput` for ADX is 1 —
/// the fold every registry inherits from `price_bundle`.
#[test]
fn abstract_rows_price_bundle_is_one_parameter() {
    use ta_codegen_lib::backends::abstract_rows::InputKind;
    const HLC: u32 = 0x0000_0002 | 0x0000_0004 | 0x0000_0008;
    let rows = all_abstract_rows();
    let adx = rows.iter().find(|r| r.name == "ADX").expect("ADX row");
    assert_eq!(adx.inputs.len(), 1, "ADX must present one bundled price input");
    assert_eq!(adx.inputs[0].kind, InputKind::Price);
    assert_eq!(adx.inputs[0].param_name, "inPriceHLC");
    assert_eq!(adx.inputs[0].flags, HLC, "ADX's bundle is exactly H+L+C");

    // And the non-bundled case still carries no component bits.
    let sma = rows.iter().find(|r| r.name == "SMA").expect("SMA row");
    assert_eq!(sma.inputs.len(), 1);
    assert_eq!(sma.inputs[0].kind, InputKind::Real);
    assert_eq!(sma.inputs[0].flags, 0);
}

/// Issue #159: an `int`-array subscript compared against a `usize`-typed variable
/// must render with the whole cast parenthesized. rustc cannot *parse* a cast
/// followed by `<` — it reads `usize <` as the start of generic arguments — so
/// `(dqI[hd]) as usize < trailingIdx` is a hard error while `generate` exits 0.
///
/// No shipped indicator has this shape (the monotonic-deque rolling-extremum
/// candidates in #147 are what surfaced it), so a regenerate is byte-identical
/// here and proves nothing; this fixture is the coverage.
///
/// Both operand positions are exercised. The right-hand one is not merely
/// defensive: `render_binop_operand` leaves a higher-precedence arithmetic child
/// unparenthesized on the left of a comparison, so `trailingIdx + dqI[hd] < today`
/// puts a *right*-operand cast directly before a `<` too.
#[test]
fn int_array_vs_usize_comparison_parenthesizes_the_cast() {
    let source = r#"
int max_lookback( int optInTimePeriod )
{
   return (optInTimePeriod-1);
}

TA_RetCode max( int    startIdx,
                int    endIdx,
                const double inReal[],
                int    optInTimePeriod,
                int   *outBegIdx,
                int   *outNBElement,
                double outReal[] )
{
   int outIdx, trailingIdx, today, highestIdx;
   int dqI[4];
   int hd;

   hd = 0;
   dqI[hd] = startIdx;
   outIdx = 0;
   today = startIdx;
   trailingIdx = startIdx;
   highestIdx = -1;

   while( today <= endIdx )
   {
      /* left operand carries the cast, directly before `<` */
      if( dqI[hd] < trailingIdx )
         hd = 0;

      /* right operand carries the cast, and the enclosing `<` still follows it */
      if( trailingIdx + dqI[hd] < today )
         hd = 0;

      /* mirror: the cast lands on the right operand of the comparison itself */
      if( trailingIdx < dqI[hd] )
         hd = 0;

      /* the i32 sentinel path (the shape WILLR/MIN/MAX already emit) */
      if( highestIdx < trailingIdx )
         highestIdx = trailingIdx;

      outReal[outIdx++] = inReal[today];
      trailingIdx++;
      today++;
   }

   *outBegIdx = startIdx;
   *outNBElement = outIdx;
   return TA_SUCCESS;
}
"#;
    let (func, enums) = load_indicator_with_source("max", source);
    let out = generate_all(&func, &enums);

    // The whole cast is wrapped, in every position.
    for needle in [
        "((dqI[hd]) as usize) < trailingIdx",
        "trailingIdx + ((dqI[hd]) as usize) < today",
        "trailingIdx < ((dqI[hd]) as usize)",
        "highestIdx < ((trailingIdx) as i32)",
    ] {
        assert!(
            out.rust.contains(needle),
            "Rust output missing `{needle}`:\n{}",
            out.rust
        );
    }

    // And nowhere does a bare cast sit directly before `<`, which would not parse.
    check_rust_cast_parens(&out.rust, "max/#159");

    // C and Java are unaffected — they have no cast to place at all.
    assert!(
        out.c.contains("if( dqI[hd] < trailingIdx )"),
        "C output should compare directly:\n{}",
        out.c
    );
    assert!(
        out.java.contains("if( dqI[hd] < trailingIdx )"),
        "Java output should compare directly:\n{}",
        out.java
    );
}

/// Issue #163: arithmetic over an `int` array element, compared against a
/// `usize`-typed variable, must carry a cast. `expr_is_i32_typed` recurses through
/// arithmetic but has no `ArrayAccess` arm, while `render_binop`'s array tests knew
/// about `int` arrays but matched a *direct* subscript only — so `dqI[hd] + 1`
/// was typed by neither and rendered bare, failing to compile with E0308.
///
/// The two shapes that already worked are asserted alongside the four that did
/// not, because they are what pin the root cause: a plain `int` local is usize in
/// Rust and needs nothing, and an expression with a usize operand already got its
/// cast from the arithmetic arm. A fix that changed either of those would be
/// reaching too far — that is how the first attempt at this turned
/// `(periods[j] as usize) > longestPeriod` into `periods[j] > (longestPeriod as
/// i32)` in ULTOSC, narrowing an index-domain value.
#[test]
fn arithmetic_over_int_array_elements_is_typed_i32() {
    let source = r#"
int max_lookback( int optInTimePeriod )
{
   return (optInTimePeriod-1);
}

TA_RetCode max( int    startIdx,
                int    endIdx,
                const double inReal[],
                int    optInTimePeriod,
                int   *outBegIdx,
                int   *outNBElement,
                double outReal[] )
{
   int outIdx, trailingIdx, today;
   int dqI[4];
   int hd;

   hd = 0;
   dqI[hd] = startIdx;
   outIdx = 0;
   today = startIdx;
   trailingIdx = startIdx;

   while( today <= endIdx )
   {
      if( dqI[hd] + 1 < today )        /* was E0308 */
         hd = 0;
      if( dqI[hd] - 1 < today )        /* was E0308 */
         hd = 0;
      if( dqI[hd] * 2 < today )        /* was E0308 */
         hd = 0;
      if( dqI[hd] << 1 < today )       /* was E0308 */
         hd = 0;
      if( dqI[hd] / 2 < today )        /* was E0308 */
         hd = 0;
      if( dqI[hd] % 3 < today )        /* was E0308 */
         hd = 0;
      if( (dqI[hd] & 3) < today )      /* was E0308 */
         hd = 0;
      if( dqI[hd] + optInTimePeriod < today )  /* was E0308: i32 opt param, not a literal */
         hd = 0;
      if( today < dqI[hd] + 1 )        /* mirror: the compound is the RIGHT operand */
         hd = 0;
      if( dqI[hd] + 1 <= today )       /* <= is legal after a bare cast; still must be typed */
         hd = 0;
      if( hd + 1 < today )             /* control: plain int local, already usize */
         hd = 0;
      if( trailingIdx + dqI[hd] < today )  /* control: usize operand present */
         hd = 0;

      outReal[outIdx++] = inReal[today];
      trailingIdx++;
      today++;
   }

   *outBegIdx = startIdx;
   *outNBElement = outIdx;
   return TA_SUCCESS;
}
"#;
    let (func, enums) = load_indicator_with_source("max", source);
    let out = generate_all(&func, &enums);

    // The four that did not compile now cast, in the usize (index) domain, and
    // the cast is fully parenthesized — it sits on the left of a `<`, so without
    // #159's wrap_cast this would not even parse.
    for needle in [
        "((dqI[hd] + 1) as usize) < today",
        "((dqI[hd] - 1) as usize) < today",
        "((dqI[hd] * 2) as usize) < today",
        "((dqI[hd] << 1) as usize) < today",
        "((dqI[hd] / 2) as usize) < today",
        "((dqI[hd] % 3) as usize) < today",
        "((dqI[hd] & 3) as usize) < today",
        // An i32 opt-in param is not an IntLiteral; the first cut of stays_i32
        // rejected it and this shape still failed to compile.
        "((dqI[hd] + optInTimePeriod) as usize) < today",
        // Mirror: the compound as the RIGHT operand of the comparison.
        "today < ((dqI[hd] + 1) as usize)",
        // `<=` parses after a bare cast, so this one proves the TYPING fired,
        // independently of #159's parenthesization.
        "((dqI[hd] + 1) as usize) <= today",
    ] {
        assert!(
            out.rust.contains(needle),
            "Rust output missing `{needle}`:\n{}",
            out.rust
        );
    }

    // The two that already worked are untouched — no cast appears on either.
    for needle in ["if hd + 1 < today {", "if trailingIdx + ((dqI[hd]) as usize) < today {"] {
        assert!(
            out.rust.contains(needle),
            "Rust output should leave `{needle}` unchanged:\n{}",
            out.rust
        );
    }

    check_rust_cast_parens(&out.rust, "max/#163");

    // C and Java compare directly; neither has a cast to place.
    assert!(out.c.contains("if( dqI[hd] + 1 < today )"), "C changed:\n{}", out.c);
    assert!(out.java.contains("if( dqI[hd] + 1 < today )"), "Java changed:\n{}", out.java);
}

/// The cast-parens gate must key on the operators rustc actually cannot parse.
/// `<` and `<<` after a bare cast are errors; `<=` and `<<=` are legal, and 22
/// shipped sites spell `<=` after a cast — matching them would fail the suite on
/// correct output. Verified against rustc, not assumed.
#[test]
fn cast_parens_gate_flags_only_the_ambiguous_operators() {
    let must_flag = [
        "        if (dqI[hd]) as usize < trailingIdx {",
        "        x = (dqI[hd]) as i32 << 2;",
        "        if a + (dqI[hd]) as usize < today {",
    ];
    for line in must_flag {
        assert!(
            std::panic::catch_unwind(|| check_rust_cast_parens(line, "fixture")).is_err(),
            "gate failed to flag an unparseable cast: {line}"
        );
    }

    let must_pass = [
        "        if ((dqI[hd]) as usize) < trailingIdx {",   // correctly wrapped
        "        if (dqI[hd]) as usize <= trailingIdx {",    // `<=` parses
        "        x = (dqI[hd]) as i32 <<= 2;",               // `<<=` parses
        "        if (dqI[hd]) as usize > trailingIdx {",     // `>` parses
        "        let n = (x) as usize;",                     // terminal position
        "        v[(i) as usize] = 0.0;",                    // index position
        "        // prose mentioning as usize < in a comment",
    ];
    for line in must_pass {
        assert!(
            std::panic::catch_unwind(|| check_rust_cast_parens(line, "fixture")).is_ok(),
            "gate false-positived on legal output: {line}"
        );
    }
}

/// An empty C comment must not abort `generate`. `/*  */` is ordinary C, and
/// `/* * */` reduces to the same thing because the lone `*` is eaten as a
/// continuation prefix; both reached `block_comment` with zero lines, which
/// indexed `lines[1..]` on an empty slice and panicked out of the whole run.
///
/// Found by a synth3 fixture that happened to label a multiply with `/* * */`.
#[test]
fn empty_c_comments_do_not_abort_generation() {
    for comment in ["/*  */", "/* * */", "/**/", "/*\n    *\n    */"] {
        let source = format!(
            r#"
int max_lookback( int optInTimePeriod )
{{
   return (optInTimePeriod-1);
}}

TA_RetCode max( int    startIdx,
                int    endIdx,
                const double inReal[],
                int    optInTimePeriod,
                int   *outBegIdx,
                int   *outNBElement,
                double outReal[] )
{{
   int outIdx, i;

   outIdx = 0;
   for( i=startIdx; i <= endIdx; i++ )
   {{
      {comment}
      outReal[outIdx++] = inReal[i];
   }}
   *outBegIdx = startIdx;
   *outNBElement = outIdx;
   return TA_SUCCESS;
}}
"#
        );
        let (func, enums) = load_indicator_with_source("max", &source);
        let out = generate_all(&func, &enums);
        for (lang, text) in [("C", &out.c), ("Rust", &out.rust), ("Java", &out.java)] {
            assert!(
                !text.is_empty(),
                "{lang} output empty for comment {comment:?}"
            );
        }
        // The body still renders; the comment must not have eaten it.
        assert!(
            out.c.contains("outReal[outIdx++] = inReal[i];"),
            "C body lost after comment {comment:?}:\n{}",
            out.c
        );
    }
}

/// Issue #165: a local that `collect_signed_int_vars` elected i32 (#160) must
/// stay recognisably i32 *inside an expression*, not only when it stands alone.
///
/// `expr_is_i32_typed_ctx` folded over the four arithmetic operators only, so
/// `head = lag;` took its `as usize` from the assign ladder while
/// `head = lag & 3;` took none and did not compile. The same omission left an
/// i32 local and a usize local unreconciled on either side of a bitwise
/// operator (`k & 65535 | hits << 16` → `i32 | usize`) — one gap, two symptoms,
/// which is why widening that operator set fixes both.
#[test]
fn signed_locals_stay_i32_inside_expressions() {
    let source = r#"
int max_lookback( int optInTimePeriod )
{
   return (optInTimePeriod-1);
}

TA_RetCode max( int    startIdx,
                int    endIdx,
                const double inReal[],
                int    optInTimePeriod,
                int   *outBegIdx,
                int   *outNBElement,
                double outReal[] )
{
   int outIdx, today, trailingIdx;
   int ring[4];
   int head, hits, lag, kk;
   double barVal;

   outIdx = 0;
   today = startIdx;
   trailingIdx = startIdx;

   while( today <= endIdx )
   {
      barVal = inReal[today];
      if( !(barVal > 0.0) || !(barVal < 1000000.0) )
         barVal = 0.0;
      lag = (int)barVal;
      kk = 0 - optInTimePeriod;
      if( kk < 0 )
         kk += optInTimePeriod;

      head = lag & 3;
      ring[head] = lag & 7;
      hits = 0;
      if( ring[head] < trailingIdx )
         hits += 1;
      kk += (kk & 65535) | (hits << 16);

      outReal[outIdx] = barVal;
      outIdx++;
      trailingIdx++;
      today++;
   }
   *outBegIdx = startIdx;
   *outNBElement = outIdx;
   return TA_SUCCESS;
}
"#;
    let (func, enums) = load_indicator_with_source("max", source);
    let out = generate_all(&func, &enums);

    // A: usize target, masked signed local on the right.
    assert!(
        out.rust.contains("head = (lag & 3) as usize;"),
        "Rust missing the `as usize` on a masked signed local:\n{}",
        out.rust
    );
    // B: the usize half is brought into the i32 domain by the sentinel arm.
    assert!(
        out.rust.contains("((hits << 16) as i32)"),
        "Rust left an i32 local and a usize local unreconciled across `|`:\n{}",
        out.rust
    );
    // The plain form was always right and must not have moved.
    assert!(
        out.rust.contains("ring[head] = (lag & 7) as i32;"),
        "Rust changed the int-array store:\n{}",
        out.rust
    );
    check_rust_cast_parens(&out.rust, "max/#165");

    // C and Java have no cast to place here.
    assert!(out.c.contains("head = lag & 3;"), "C changed:\n{}", out.c);
    assert!(out.java.contains("head = lag & 3;"), "Java changed:\n{}", out.java);
}

// ---------------------------------------------------------------------------
