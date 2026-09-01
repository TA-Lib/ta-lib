//! C#/Rust enum emission, the enum-domain gate, bitwise operators (#157),
//! and the negative-cast-to-signed-local rule (#160). Split out of the
//! former `backend_suite.rs`.

#[path = "common/mod.rs"]
mod common;

use common::{
    discover_indicators, generate_all, load_enums, load_indicator, load_indicator_with_source,
    make_helpers, make_registry,
};
use std::path::Path;
use ta_codegen_lib::backends;
use ta_codegen_lib::helper_registry::HelperRegistry;
use ta_codegen_lib::registry::Lang;

// C# enums (M1). These assert on EMITTED CONTENT, not merely that the emitter
// ran: a test that only checks "generate() did not panic" is the shape that has
// passed vacuously in this repo before.
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Rust enum (#179). Same standard as the C# one below: assert on EMITTED
// CONTENT. Everything here is frozen public API the moment the crate publishes,
// and until now none of it was asserted anywhere.
// ---------------------------------------------------------------------------

#[test]
fn rust_matype_emits_every_yaml_variant_and_its_frozen_shape() {
    let enums = load_enums();
    let src = backends::rust_enums::render_matype(&enums);
    let ma = enums.get("MAType").expect("MAType in enums.yaml");

    for v in &ma.variants {
        let decl = format!("    {} = {},", v.name, v.value);
        assert!(
            src.contains(&decl),
            "Rust MAType is missing `{decl}` -- a dropped variant reorders the \
             optInMAType ABI:\n{src}"
        );
        // The conversion must accept every member, or a value that is legal at
        // the C ABI would be rejected by the Rust one.
        let arm = format!("            {} => Self::{},", v.value, v.name);
        assert!(
            src.contains(&arm),
            "TryFrom<i32> is missing `{arm}`:\n{src}"
        );
    }

    // An EXTRA emitted member fails too.
    let emitted = src
        .lines()
        .filter(|l| l.starts_with("    ") && l.contains(" = ") && l.trim_end().ends_with(','))
        .count();
    assert_eq!(emitted, ma.variants.len(), "emitted {emitted} members");

    // `#[non_exhaustive]` is what lets a member be appended without breaking
    // every downstream `match`; dropping it is a silent semver break.
    assert!(src.contains("#[non_exhaustive]"), "MAType lost #[non_exhaustive]");

    // No `#[repr]`: the crate has no FFI, so the layout is unobservable and the
    // explicit discriminants carry the ABI. Adding one would freeze a size we
    // deliberately did not promise.
    assert!(
        !src.contains("#[repr("),
        "MAType gained a #[repr]; the crate has no FFI to justify one:\n{src}"
    );

    // The sentinel arm is load-bearing: the abstract tier stores the bound int
    // verbatim as C's does, so TA_INTEGER_DEFAULT must still select the
    // parameter's declared default rather than being rejected (#162).
    assert!(
        src.contains("i32::MIN => Self::DEFAULT,"),
        "TryFrom lost the TA_INTEGER_DEFAULT arm; Rust would drop out of the \
         choice-list sentinel contract:\n{src}"
    );
    assert!(
        src.contains("_ => return Err(RetCode::BadParam),"),
        "TryFrom lost its reject arm -- out-of-domain values would not be \
         rejected by the library:\n{src}"
    );
}

// ---------------------------------------------------------------------------
// The enum domain gate. A choice-list parameter declares no `range:`, so before
// this the prologue emitted only the default substitution and each body decided
// for itself what an out-of-domain value meant -- which is how TA_MA_Lookback
// answered 0 for parameters TA_MA rejects. Both tiers now reject from one
// emitter with two failure literals, the construction that already made integer
// ranges immune. Asserted on emitted content, per this file's standard.
// ---------------------------------------------------------------------------

#[test]
fn enum_param_gets_a_domain_gate_in_both_tiers() {
    let enums = load_enums();
    let (func, _) = load_indicator("ma");
    let registry = make_registry();
    let helpers = make_helpers();

    // The gate names the generated limit constants rather than the numbers of
    // the day -- that is the whole point of them, so assert the spelling the
    // enum surface declares and never a literal.
    let ma = enums.get("MAType").expect("MAType");
    let (c_min, c_max) =
        backends::common::enum_limit_names_of(ma, Lang::C).expect("C declares MAType limits");

    let c = backends::c::generate(&func, &enums, &registry, &helpers);
    let gate = format!("(int)optInMAType < {c_min} || (int)optInMAType > {c_max}");
    // Both tiers: the lookback fails with -1, the guarded call with TA_BAD_PARAM.
    assert!(
        c.contains(&format!("{gate} )\n      return -1;")),
        "C lookback lost the enum domain gate:\n{c}"
    );
    assert!(
        c.contains(&format!("{gate} )\n      return TA_BAD_PARAM;")),
        "C guarded call lost the enum domain gate:\n{c}"
    );

    let (cs_min, cs_max) = backends::common::enum_limit_names_of(ma, Lang::CSharp)
        .expect("C# declares MAType limits");
    let cs = backends::csharp::generate(&func, &enums, &registry, &helpers);
    assert!(
        cs.contains(&format!("(int)optInMAType < {cs_min} || (int)optInMAType > {cs_max}")),
        "C# lost the enum domain gate:\n{cs}"
    );

    // And no tier carries the bound as a number any more: a reintroduced literal
    // is a value that has to be re-edited in every prologue when a member is
    // appended, which is the defect the constants exist to remove.
    let hi = ma.variants.iter().map(|v| v.value).max().expect("members");
    for (lang, src) in [("C", &c), ("C#", &cs)] {
        assert!(
            !src.contains(&format!("optInMAType > {hi}")),
            "{lang} spelled the MAType bound as a literal again:\n{src}"
        );
    }
}

#[test]
fn the_enum_limit_macros_are_declared_next_to_the_enum() {
    // The declaration is where the number now lives, so it is what has to be
    // derived. A synthetic enum that ends somewhere other than MAType's 11 is
    // what separates a derived bound from a hard-coded one.
    use ta_codegen_lib::ir::{EnumDef, EnumVariant};
    let tri = EnumDef {
        name: "Tri".to_string(),
        c_prefix: "TA_Tri_".to_string(),
        variants: (0..3)
            .map(|v| EnumVariant {
                name: format!("V{v}"),
                c_name: format!("TA_Tri_V{v}"),
                value: v,
            })
            .collect(),
    };

    let c = backends::ta_defs::render_enum_limits(&tri, "TA_Tri");
    assert!(
        c.contains("#define TA_TRI_MIN 0") && c.contains("#define TA_TRI_MAX 2"),
        "the C limit macros must span the members and take the enum's own \
         c_prefix, upper-cased:\n{c}"
    );

    // C# reaches the same numbers through the shipped enum file. Swap MAType's
    // members for the synthetic three so a hard-coded 11 could not pass.
    let (func, _) = load_indicator("ma");
    let mut enums = load_enums();
    let ma = enums.get_mut("MAType").expect("MAType");
    ma.variants.truncate(3);
    let cs = backends::csharp_enums::render_matype(std::slice::from_ref(&func), &enums);
    assert!(
        cs.contains("public const int Min = 0;") && cs.contains("public const int Max = 2;"),
        "the C# limit companion must span the members:\n{cs}"
    );
    assert!(
        cs.contains("public static class MATypes"),
        "the C# limits must live in the enum's companion class:\n{cs}"
    );
}

#[test]
fn an_enum_no_parameter_is_typed_with_gets_no_limits() {
    // FuncUnstId's pinned ALL = 65535 sits outside its member span, so limits
    // derived from the members would describe a domain its API does not have.
    // Nothing is typed with it, so nothing emits them -- assert that rule holds
    // rather than that FuncUnstId in particular is spelled out somewhere.
    let enums = load_enums();
    let (func, _) = load_indicator("ma");
    let param_enums = backends::common::param_enum_names(std::slice::from_ref(&func));
    assert!(param_enums.contains("MAType"), "MA takes an optInMAType");
    assert!(
        !param_enums.contains("FuncUnstId"),
        "no optional parameter is typed with FuncUnstId"
    );

    let cs = backends::csharp_enums::render_funcunstid(&enums);
    assert!(
        !cs.contains("Min =") && !cs.contains("Max ="),
        "FuncUnstId gained value limits its ALL wildcard falls outside of:\n{cs}"
    );
}

#[test]
fn the_gate_bound_follows_the_member_set() {
    // The bound is derived, never spelled. Asserting it against MAType's own max
    // cannot show that -- a hard-coded 11 and a derived one read identically
    // while the enum happens to end at 11. So span it against a synthetic enum
    // that ends somewhere else.
    use ta_codegen_lib::ir::{EnumDef, EnumVariant, OptInput, ParamType};
    // (What the prologue now emits is the constant's NAME; the number it
    // resolves to is asserted at the declaration, above.)
    let mut enums = load_enums();
    enums.insert(
        "Tri".to_string(),
        EnumDef {
            name: "Tri".to_string(),
            c_prefix: "TA_Tri_".to_string(),
            variants: (0..3)
                .map(|v| EnumVariant {
                    name: format!("V{v}"),
                    c_name: format!("TA_Tri_V{v}"),
                    value: v,
                })
                .collect(),
        },
    );
    let opt = OptInput {
        name: "optInTri".to_string(),
        param_type: ParamType::Enum("Tri".to_string()),
        display_name: None,
        hint: None,
        range: None,
        default: Some(0.0),
        suggested: None,
        flags: Vec::new(),
        precision: None,
    };
    assert_eq!(
        backends::common::enum_value_bounds_of(enums.get("Tri").expect("Tri")),
        Some((0, 2)),
        "the domain must span the members, not a hard-coded bound"
    );
    assert_eq!(
        backends::common::int_bound_exprs(&opt, &enums, Lang::C),
        Some(("TA_TRI_MIN".to_string(), "TA_TRI_MAX".to_string())),
        "the prologue must name the enum's own limit macros, not MAType's"
    );
}

#[test]
fn a_declared_range_still_wins_over_the_member_span() {
    // The precedence branch in `int_bound_exprs`: an `enum:` parameter that DID
    // declare a range must keep it, or a narrower intent would be silently
    // widened to the whole enum. A declared range is per-parameter, so it stays
    // a literal -- the limit constants describe the TYPE's domain, which is not
    // the same thing. Nothing in the shipped input exercises this.
    use ta_codegen_lib::ir::{OptInput, ParamType};
    let enums = load_enums();
    let opt = OptInput {
        name: "optInMAType".to_string(),
        param_type: ParamType::Enum("MAType".to_string()),
        display_name: None,
        hint: None,
        range: Some((0.0, 2.0)),
        default: Some(0.0),
        suggested: None,
        flags: Vec::new(),
        precision: None,
    };
    assert_eq!(
        backends::common::int_bound_exprs(&opt, &enums, Lang::C),
        Some(("0".to_string(), "2".to_string())),
        "a declared range must win over the member span"
    );
}

#[test]
fn csharp_matype_emits_every_yaml_variant_with_its_value() {
    let enums = load_enums();
    let (func, _) = load_indicator("ma");
    let src = backends::csharp_enums::render_matype(std::slice::from_ref(&func), &enums);
    let ma = enums.get("MAType").expect("MAType in enums.yaml");

    for v in &ma.variants {
        let decl = format!("    {} = {},", v.name, v.value);
        assert!(
            src.contains(&decl),
            "MAType.cs is missing `{decl}` -- a variant silently dropped from the \
             emitted enum reorders the optInMAType ABI:\n{src}"
        );
    }
    // Count the members, so an EXTRA emitted variant fails too. Match the
    // member shape specifically -- the BSD header has comma-terminated prose.
    let emitted = src
        .lines()
        .filter(|l| l.starts_with("    ") && l.contains(" = ") && l.trim_end().ends_with(','))
        .count();
    assert_eq!(
        emitted,
        ma.variants.len(),
        "MAType.cs emitted {emitted} members for {} YAML variants",
        ma.variants.len()
    );
}

#[test]
fn csharp_funcunstid_pins_the_all_sentinel_and_the_count() {
    let enums = load_enums();
    let src = backends::csharp_enums::render_funcunstid(&enums);
    let fu = enums.get("FuncUnstId").expect("FuncUnstId in enums.yaml");

    // The ABI pin. C pins TA_FUNC_UNST_ALL at 65535; a renumber here silently
    // repoints every caller's set_unstable_period and nothing else catches it.
    assert!(
        src.contains("ALL = 65535,"),
        "FuncUnstId.cs must pin `ALL = 65535`:\n{src}"
    );
    assert!(
        src.contains(&format!("public const int Count = {};", fu.variants.len())),
        "FuncUnstIds.Count must equal the {} function ids (and must NOT be an \
         enum member -- that would make it an id):\n{src}",
        fu.variants.len()
    );
    for v in &fu.variants {
        let decl = format!("    {} = {},", v.name, v.value);
        assert!(
            src.contains(&decl),
            "FuncUnstId.cs is missing `{decl}`:\n{src}"
        );
    }
    // Count must not silently include the All sentinel.
    assert!(
        !src.contains(&format!("public const int Count = {};", fu.variants.len() + 1)),
        "Count must exclude the All sentinel"
    );
}

#[test]
fn csharp_resolve_call_agrees_with_the_emitted_method_names() {
    // If Registry::name_of and the emitter's method naming disagree, every
    // cross-indicator call targets a method that does not exist -- and that will
    // not surface until the backend emits bodies, as a wall of CS0103.
    let registry = make_registry();
    let enums = load_enums();
    let helpers = make_helpers();

    for name in discover_indicators() {
        let (func, _) = load_indicator(&name);
        let bare = registry.resolve_call(&name, ta_codegen_lib::registry::Lang::CSharp);
        let lookback = registry.resolve_call(
            &format!("{name}_lookback"),
            ta_codegen_lib::registry::Lang::CSharp,
        );
        assert!(
            !bare.ends_with("Unguarded"),
            "{name}: bare cross-indicator call must resolve to the guarded \
             entry point, got {bare}"
        );
        // The resolved name is the YAML `name:` verbatim, and the suffix is
        // separated by an underscore.
        assert_eq!(bare, func.name, "{name}: C# base must be the YAML name verbatim");
        assert_eq!(
            lookback,
            format!("{}_Lookback", func.name),
            "{name}: lookback and guarded names disagree on the base"
        );
        // What the resolver promises must be what the emitter actually writes —
        // the literal a caller in another indicator will be compiled against.
        // Since #236 step 5 the bare name is the PUBLIC overload -- the only tier
        // left that carries it -- which is what a cross-call has bound since
        // step 3 put the callee's argument checks on the composed path.
        let src = backends::csharp::generate(&func, &enums, &registry, &helpers);
        assert!(
            src.contains(&format!("OutRange {bare}(")),
            "{name}: emitter never defines the `{bare}` the resolver hands out"
        );
        assert!(
            src.contains(&format!("public int {lookback}(")),
            "{name}: emitter never defines the `{lookback}` the resolver hands out"
        );
    }
}

/// Issue #156: the runtime FMA dispatch trio (public dispatcher +
/// `#[target_feature(enable = "fma")]` clone + `#[inline(always)]` `_impl`)
/// must be emitted for exactly the functions whose rendered body fuses — this
/// is the exact-set half of the pair `fma_suite.rs` also lists — and never
/// elsewhere.
/// Guards both directions: the dispatch silently going dark, and accidental
/// dispatch of unfused functions.
#[test]
fn rust_fma_dispatch_fires_for_exactly_the_fusing_functions() {
    let fusing = backends::fma::FUSING_INVENTORY;
    let registry = make_registry();
    let helpers = make_helpers();
    let base = Path::new(env!("CARGO_MANIFEST_DIR")).join("../../ta_codegen/input");
    let mut dispatched: Vec<String> = Vec::new();
    let mut checked = 0usize;
    for entry in std::fs::read_dir(&base).expect("input dir") {
        let entry = entry.expect("dir entry");
        if !entry.file_type().map(|t| t.is_dir()).unwrap_or(false) {
            continue;
        }
        let name = entry.file_name().to_string_lossy().to_string();
        let dir = entry.path();
        if !dir.join(format!("{name}.c")).is_file() || !dir.join(format!("{name}.yaml")).is_file() {
            continue;
        }
        let (func, enums) = load_indicator(&name);
        let out = backends::rust_lang::generate(&func, &enums, &registry, &helpers);
        checked += 1;
        if out.contains("ta_lib_dispatch::dispatch_fma!") {
            // Every dispatcher must come with exactly one clone: the
            // dispatch-call count and target_feature-attribute count match.
            let calls = out.matches("ta_lib_dispatch::dispatch_fma!").count();
            let clones = out.matches("#[target_feature(enable = \"fma\")]").count();
            assert_eq!(calls, clones, "{name}: dispatcher/clone count mismatch");
            // The batch variant must carry its clone. Dispatch sits on the
            // C-shaped `_Impl` entry point, which is where the fused body
            // lives; the public `Result`-returning wrapper only forwards. (A
            // future private-delegating fused function would trip the
            // dispatcher/clone balance above on purpose.)
            assert!(
                out.contains(&format!("fn {}_Impl_fma(", func.name)),
                "{name}: guarded variant lost its FMA clone"
            );
            // The fused sites live on in the renamed portable impl.
            assert!(
                out.contains("_impl(") && out.contains(".mul_add("),
                "{name}: dispatch emitted but trio structure incomplete"
            );
            dispatched.push(name);
        } else {
            assert!(
                !out.contains(".mul_add("),
                "{name}: fused body without a dispatch trio"
            );
        }
    }
    dispatched.sort();
    assert_eq!(dispatched, fusing, "FMA dispatch inventory drifted");
    assert!(checked >= 150, "expected ~168 functions, checked {checked}");
}

// ---------------------------------------------------------------------------
// Bitwise operators (issue #157): every C bitwise form renders correctly in
// all four backends. C/Java/C# spell `~` as `~`; Rust spells it `!` and needs
// explicit `!= 0` for C's int-truthiness conditions. C's grouping must survive
// Rust's different precedence (`&`/`^`/`|` bind tighter than `==` in Rust).
// ---------------------------------------------------------------------------

#[test]
fn bitwise_operators_render_in_all_backends() {
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
   int outIdx, i, mask, neg;

   mask = (optInTimePeriod ^ 3) & ~1;
   mask |= 2;
   mask &= 15;
   mask ^= 1;
   mask <<= 1;
   mask >>= 1;
   mask = ((mask | 4) << 1) >> 1;
   mask = (mask << 2) + 1;
   neg = ~optInTimePeriod;
   if( (mask & 1) == 9999 )
      return TA_INTERNAL_ERROR;
   if( mask & 16 )
      return TA_INTERNAL_ERROR;
   if( (mask & 1) && (neg < 0) )
      outIdx = 0;
   if( !(mask & 1) )
      return TA_INTERNAL_ERROR;
   while( mask & 1024 )
      mask = mask & ~1024;
   i = (mask & 2) ? 1 : 0;
   if( i == 9999 )
      return TA_INTERNAL_ERROR;
   outIdx = 0;
   for( i=startIdx; i <= endIdx; i++ )
      outReal[outIdx++] = inReal[i];
   *outBegIdx = startIdx;
   *outNBElement = outIdx;
   return TA_SUCCESS;
}
"#;
    let (func, enums) = load_indicator_with_source("max", source);
    let out = generate_all(&func, &enums);

    for needle in [
        "mask = (optInTimePeriod ^ 3) & ~1;",
        "mask |= 2;",
        "mask &= 15;",
        "mask ^= 1;",
        "mask <<= 1;",
        "mask >>= 1;",
        "mask = (mask | 4) << 1 >> 1;",
        "mask = (mask << 2) + 1;",
        "neg = ~optInTimePeriod;",
        "if( (mask & 1) == 9999 )",
        "if( mask & 16 )",
        "if( mask & 1 && neg < 0 )",
        "if( !(mask & 1) )",
        "while( mask & 1024 )",
        "i = (mask & 2) ? 1 : 0;",
    ] {
        assert!(out.c.contains(needle), "C output missing `{needle}`:\n{}", out.c);
    }

    for needle in [
        "mask = (optInTimePeriod ^ 3) & ~1;",
        "mask |= 2;",
        "mask &= 15;",
        "mask ^= 1;",
        "mask <<= 1;",
        "mask >>= 1;",
        "(mask & 1) == 9999",
        "(mask & 16) != 0",
        "(mask & 1) != 0 && neg < 0",
        "((mask & 1) == 0)",
        "while( (mask & 1024) != 0 )",
        "((mask & 2) != 0) ? 1 : 0",
    ] {
        assert!(out.java.contains(needle), "Java output missing `{needle}`:\n{}", out.java);
    }

    for needle in [
        "& !(1)",
        "mask |= 2;",
        "mask &= 15;",
        "mask ^= 1;",
        "mask <<= 1;",
        "mask >>= 1;",
        "mask & 1 == 9999",
        "(mask & 16) != 0",
        "(mask << 2) + 1",           // Rust shifts bind looser than + : parens required
        "let mut neg: i32",          // ~x can be negative: var must be signed
        "(mask & 1) != 0 && neg < 0",
        "(mask & 1) == 0",
        "while (mask & 1024) != 0 {",
        "if (mask & 2) != 0 {",
    ] {
        assert!(out.rust.contains(needle), "Rust output missing `{needle}`:\n{}", out.rust);
    }

    let registry = make_registry();
    let helpers = HelperRegistry::empty();
    let cs = backends::csharp::generate(&func, &enums, &registry, &helpers);
    for needle in [
        "& ~1;",
        "(mask & 1) == 9999",
        "(mask & 16) != 0",
        "(mask & 1) != 0 && neg < 0",
        "((mask & 1) == 0)",
        "while( (mask & 1024) != 0 )",
        "((mask & 2) != 0) ? 1 : 0",
    ] {
        assert!(cs.contains(needle), "C# output missing `{needle}`:\n{cs}");
    }
}

// ---------------------------------------------------------------------------
// Issue #160: a C `(int)` cast of a possibly-negative double must land in a
// SIGNED Rust local (the default f64→usize cast saturates negatives to 0).
// MAVP's period clamp is the shipped case; synth2 in input_synth/ is the
// end-to-end gate. This pins the rendering so a classifier regression is a
// test failure, not a silent semantic drift.
// ---------------------------------------------------------------------------

#[test]
fn rust_negative_capable_cast_gets_signed_local() {
    let (func, enums) = load_indicator("mavp");
    let out = generate_all(&func, &enums);
    for needle in [
        "let mut tempInt: i32",             // cast-fed local is signed
        // The narrowing happens in the else arm of the real-domain clamp
        // (35a35d4b4), on a value already inside [min, max]; the two clamp arms
        // assign the bounds directly and never narrow.
        "tempPeriod = inPeriods[startIdx + i];",
        "if !(tempPeriod >= minPeriodReal) {",  // NaN-catching spelling
        "tempInt = (tempPeriod) as i32;",       // narrowed only once in range
        "if tempInt < 1 {",                     // clamps stay signed compares
    ] {
        assert!(out.rust.contains(needle), "MAVP Rust missing `{needle}`:\n{}", out.rust);
    }
    // sqrt-fed locals stay usize (provably non-negative allowlist): HMA's
    // sqrtPeriod = (int)(sqrt(...)) is the shipped case.
    let (hma, enums2) = load_indicator("hma");
    let hma_out = generate_all(&hma, &enums2);
    assert!(
        hma_out.rust.contains("let mut sqrtPeriod: usize"),
        "HMA sqrtPeriod must stay usize (allowlist regression):\n{}",
        hma_out.rust
    );
}

// Index-domain values must never narrow to i32: every runtime gate feeds
// <= 100k bars, so an i32-narrowed endIdx misbehaves only at >= 2^31 inputs —
// structurally invisible to value comparison. Pin it textually instead (the
// exact regression the #160 review caught in MAVP's dual-role temp).
#[test]
fn rust_index_domain_never_narrows_to_i32() {
    for name in discover_indicators() {
        let (func, enums) = load_indicator(&name);
        let out = generate_all(&func, &enums);
        for needle in ["(endIdx) as i32", "(startIdx) as i32", "endIdx as i32", "startIdx as i32"] {
            assert!(
                !out.rust.contains(needle),
                "{name}: generated Rust narrows an index-domain value (`{needle}`)"
            );
        }
        // The needles above are the bare forms; an arithmetic expression
        // narrows just as badly and matches none of them. A broader "any line
        // with `as i32` mentioning the range" rule is wrong — MAVP's
        // `(inPeriods[startIdx + i]) as i32` casts an i32 array element and
        // only uses the range as a subscript — so pin the arithmetic forms.
        for op in ['+', '-', '*'] {
            for needle in [
                format!("startIdx {op} "),
                format!("endIdx {op} "),
            ] {
                for line in out.rust.lines().filter(|l| l.contains("as i32")) {
                    assert!(
                        !(line.contains(&needle) && !line.contains('[')),
                        "{name}: generated Rust narrows an index-domain expression to i32: {line}"
                    );
                }
            }
        }
    }
}

// ---------------------------------------------------------------------------
