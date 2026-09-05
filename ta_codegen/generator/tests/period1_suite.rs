//! `period1_identity` membership — who must declare the flag (issue #184).
//!
//! The flag itself is a promise: at a period of 1 the function performs no
//! smoothing, so its lookback is 0 and every output value is a bit-exact copy of
//! its input value. `test_period_boundary.c` holds every flagged function to that
//! promise at run time, across the batch, `TA_S_`, streaming and cross-language
//! surfaces, on the reference series and on two series built to break the naive
//! period-1 forms. **This file decides who is in that sweep.**
//!
//! Why membership needs its own gate: the promise has to be *declared*, because
//! the two ways of honouring it are indistinguishable in the source. `SMA`'s
//! window math is already exact at a period of 1; `EMA`'s recurrence reduces to
//! `(x - prev) + prev`, which returns `x` only while `x - prev` is exactly
//! representable, so it needs an explicit arm. A generator reading either body
//! cannot tell "needs no arm" from "forgot one".
//!
//! It cost four omissions of the same contract to establish that (issue #184):
//! `HMA` rejected a period of 1 outright, `VWMA` computed `(P*V)/V`, `EMA` and
//! `DEMA` ran the raw recursion while three documents promised the copy, and
//! `MACD`/`MACDFIX` ran the unguarded signal recursion. Each was found by going
//! looking, never by a gate. Three gates below, and what each would have caught:
//!
//! 1. [`every_flagged_function_can_be_driven_to_period_one`] — the flag must be
//!    *meaningful*: a period must exist and its range must admit 1. The one gate
//!    the runtime sweep cannot stand in for, since a flagged function it cannot
//!    drive is one it silently skips.
//! 2. [`matype_members_declare_period1_identity`] — a moving average that does
//!    not copy its input at a period of 1 is not a moving average, so every
//!    `MAType` member with a period must declare it, and its range must admit 1.
//!    Catches `HMA`'s rejection and `EMA`/`DEMA`'s recursion, and is the arrival
//!    gate: a `MAType` added later is held to the contract without anyone
//!    remembering to edit a list.
//! 3. [`a_reachable_identity_arm_declares_period1_identity`] — a function that
//!    hand-wrote a reachable arm must declare it. Catches `VWMA`, and is the half
//!    that matters most, because every omission but `EMA`/`DEMA` was *outside*
//!    the enum where no membership rule reached.
//!
//! `MACD`/`MACDFIX` are the fourth omission and **none of these covers them**,
//! deliberately: their arm degenerates only the signal stage, so their output is
//! not a copy of anything and this flag cannot describe them. They stay
//! hand-written and hand-tested (`testMacdFamilySignalOne`), which is the
//! conclusion the issue itself reached.
//!
//! **The converse of gate 3 does not hold and is deliberately not asserted.** A
//! flagged function need not carry an arm (`SMA`, `TRIMA`), and no static
//! analysis can decide whether one is needed — which is the whole reason the flag
//! is declarative. What holds a flagged function to the contract is the runtime
//! value sweep, which is strictly stronger than asserting an arm exists: it
//! compares the bits.

use std::collections::{HashMap, HashSet};
use std::path::Path;

use ta_codegen_lib::{
    ir::{EnumDef, FuncDef, ParamType},
    parser, streaming,
};

const FLAG: &str = "period1_identity";

fn input_dir() -> std::path::PathBuf {
    Path::new(env!("CARGO_MANIFEST_DIR")).join("../input")
}

/// Every function in the input tree, YAML metadata plus parsed `.c` body (the
/// identity detector reads the body).
fn load() -> Vec<FuncDef> {
    let base = input_dir();
    let mut funcs = Vec::new();
    for e in std::fs::read_dir(&base).expect("read input dir").filter_map(Result::ok) {
        let dir = e.path();
        if !dir.is_dir() {
            continue;
        }
        let n = e.file_name().to_string_lossy().to_string();
        let (yaml, csrc) = (dir.join(format!("{n}.yaml")), dir.join(format!("{n}.c")));
        if !yaml.exists() || !csrc.exists() {
            continue;
        }
        let mut f = parser::yaml::parse_yaml(&yaml);
        let parsed = parser::c_source::parse_c_source(&csrc);
        parser::c_source::wire_parsed_source(&mut f, &parsed);
        funcs.push(f);
    }
    assert!(funcs.len() >= 200, "expected the whole input tree, got {}", funcs.len());
    funcs
}

fn load_matype() -> EnumDef {
    let enums = parser::enums::load_enums(&input_dir().join("enums.yaml"));
    enums.get("MAType").expect("enums.yaml declares MAType").clone()
}

fn has_flag(f: &FuncDef) -> bool {
    f.flags.iter().any(|x| x == FLAG)
}

/// The parameter the sweep drives to 1: the first integer RANGE parameter. A
/// choice list selects a type, not a period. Mirrors `pbCheckMaIdentityByName`
/// in `test_period_boundary.c` — the two must agree on which functions have a
/// period at all, or one gate's members escape the other's. `None` when the
/// function has no such parameter.
fn period_range(f: &FuncDef) -> Option<(f64, f64)> {
    f.optional_inputs
        .iter()
        .find(|p| p.param_type == ParamType::Integer && p.range.is_some())
        .and_then(|p| p.range)
}

/// Whether a caller can actually pass a period of 1. The flag is a statement
/// about the PUBLIC domain, so a function whose range starts at 2 cannot carry
/// it however its body behaves — the guarded call returns `TA_BAD_PARAM` before
/// reaching any arm.
fn admits_period_one(f: &FuncDef) -> bool {
    period_range(f).is_some_and(|(min, _)| min <= 1.0)
}

/// A `MAType` member that cannot be driven to a period of 1, with the reason.
/// Kept in step with `PB_MA_EXEMPT` in `test_period_boundary.c`, which exempts
/// the same three from the value sweep for the same reasons.
const MATYPE_EXEMPT: &[(&str, &str)] = &[
    (
        "DISABLED",
        "dispatch-only sentinel: no function of that name (MA's own identity guard covers it)",
    ),
    (
        "DEFAULT",
        "resolved before dispatch: the prologue substitutes the parameter's declared default, \
         whose own row runs the check",
    ),
    (
        "MAMA",
        "no integer-range parameter: its two parameters are the real fast/slow limits, so there \
         is no period to set to 1",
    ),
];

fn exempt_reason(name: &str) -> Option<&'static str> {
    MATYPE_EXEMPT.iter().find(|(n, _)| *n == name).map(|(_, why)| *why)
}

/// Functions carrying an identity arm their own declared range makes
/// unreachable, so they cannot carry the flag: the guarded call rejects a period
/// of 1 before the arm can run. Not a defect — dead defensive code behind a
/// narrower domain — but not something to discover twice either, which is what
/// listing it here prevents.
const UNREACHABLE_ARM: &[(&str, &str)] = &[
    (
        "RSI",
        "range starts at 2, so TA_RSI(period=1) is TA_BAD_PARAM before the arm can run. The arm \
         itself is original TA-Lib (f2249ff2f, 2003) and #137 only reshaped it from a memmove \
         into an element loop, so it has outlived whatever reached it; released v0.6.4 rejects a \
         period of 1 here too, which makes widening the range a behaviour change rather than a \
         fix, and not this issue's call to make.",
    ),
    (
        "CMO",
        "range starts at 2, same arm and same lineage as RSI (they share the Wilder body).",
    ),
];

fn unreachable_arm_reason(name: &str) -> Option<&'static str> {
    UNREACHABLE_ARM.iter().find(|(n, _)| *n == name).map(|(_, why)| *why)
}

/// A flagged function must have a period a caller can set to 1. Trivial to state
/// and the only gate that can see the mis-declaration, because the runtime sweep
/// cannot: it skips a flagged name that is also a `MAType` label (the enum walk
/// owns those), and the enum walk *exempts* a member with no period. So flagging
/// `MAMA` — which promises nothing, its parameters being the real fast/slow
/// limits — would leave it flagged, unswept and unnoticed at run time.
///
/// This also subsumes the `RSI`/`CMO` case from the other direction: a range
/// starting at 2 cannot carry the flag, whatever the body contains.
#[test]
fn every_flagged_function_can_be_driven_to_period_one() {
    let funcs = load();
    let mut flagged = 0;

    for f in funcs.iter().filter(|f| has_flag(f)) {
        flagged += 1;
        assert!(
            period_range(f).is_some(),
            "{} declares `{FLAG}` but has no integer-range parameter, so there is nothing to set \
             to 1 and nothing the promise can mean",
            f.name
        );
        assert!(
            admits_period_one(f),
            "{} declares `{FLAG}` but its range is {:?}, so a period of 1 is rejected before the \
             call computes anything. The flag states what the PUBLIC domain offers, not what the \
             body would do if reached.",
            f.name,
            period_range(f).expect("checked above")
        );
    }

    println!("period1_identity: {flagged} function(s) declare the flag");
    assert!(flagged >= 11, "only {flagged} function(s) carry the flag; it was 11 at #184");
}

/// Every `MAType` member that resolves to a function with a period must declare
/// the flag. An exemption the table does not list is a failure, not a skip:
/// otherwise a member renamed to something that resolves to no function drops out
/// of both this gate and the runtime sweep silently.
#[test]
fn matype_members_declare_period1_identity() {
    let funcs = load();
    let by_name: HashMap<String, &FuncDef> =
        funcs.iter().map(|f| (f.name.to_uppercase(), f)).collect();

    let mut missing = Vec::new();
    let mut checked = 0;
    let mut exempt = 0;

    for v in &load_matype().variants {
        let Some(func) = by_name.get(&v.name.to_uppercase()) else {
            assert!(
                exempt_reason(&v.name).is_some(),
                "MAType member `{}` resolves to no function in ta_codegen/input/, and is not a \
                 documented exemption in MATYPE_EXEMPT",
                v.name
            );
            exempt += 1;
            continue;
        };
        if period_range(func).is_none() {
            assert!(
                exempt_reason(&v.name).is_some(),
                "MAType member `{}` has no integer-range parameter to set to 1, and is not a \
                 documented exemption in MATYPE_EXEMPT",
                v.name
            );
            exempt += 1;
            continue;
        }
        // A moving average that REJECTS a period of 1 is the same defect one step
        // earlier -- it is how HMA's went in (fixed, 012585d00) -- and the flag
        // would be a lie, since the guarded call never reaches the body.
        assert!(
            admits_period_one(func),
            "MAType member `{}` declares range {:?}, so a period of 1 is rejected outright. A \
             moving average of period 1 is the input unsmoothed; widen the range to start at 1.",
            v.name,
            period_range(func).expect("checked above")
        );
        if !has_flag(func) {
            missing.push(v.name.clone());
        }
        checked += 1;
    }

    assert!(
        missing.is_empty(),
        "MAType member(s) {missing:?} do not declare `flags: [{FLAG}]`.\n\
         A moving average of period 1 performs no smoothing, so its output is its input. Add \
         the flag to ta_codegen/input/<name>/<name>.yaml -- and make sure the function \
         actually honours it, because test_period_boundary.c is about to check the bits."
    );
    // Non-vacuity: the accounting is printed and asserted, so the gate cannot
    // pass by resolving nothing.
    println!(
        "period1_identity: {checked} MAType member(s) declare the flag, {exempt} exempt, \
         {} member(s) total",
        checked + exempt
    );
    assert_eq!(
        checked + exempt,
        load_matype().variants.len(),
        "every MAType member must be either checked or exempted"
    );
    assert!(checked >= 9, "only {checked} MAType member(s) reached the check");
    assert_eq!(exempt, MATYPE_EXEMPT.len(), "an exemption stopped applying");
}

/// An identity arm a caller can REACH obliges the flag. Decidable, so it needs no
/// exemption table for the rule itself: the detector either recognises the arm or
/// it does not, and a shape it does not recognise simply obliges nothing here.
///
/// `MACD`/`MACDFIX` are the reason this reads the *whole-function* arm rather
/// than "mentions period == 1": their guard sits inside the loop and degenerates
/// only the signal stage, so the output is not a copy of anything and the
/// detector correctly declines them.
#[test]
fn a_reachable_identity_arm_declares_period1_identity() {
    let funcs = load();
    let mut missing = Vec::new();
    let mut detected = Vec::new();
    let mut unreachable = Vec::new();

    for f in &funcs {
        if streaming::identity_path(f).is_none() {
            continue;
        }
        let name = f.name.to_uppercase();
        detected.push(name.clone());
        if !admits_period_one(f) {
            // The arm exists but the declared range forbids the value that
            // selects it, so the flag would promise a call that returns
            // TA_BAD_PARAM. Pinned rather than failed: a defensive arm behind a
            // narrower domain is dead code, not a defect. Asserted both ways
            // below, so neither the list nor the reason can go stale quietly.
            #[allow(clippy::cast_possible_truncation)]
            let min = period_range(f).map_or(0, |(min, _)| min as i64);
            unreachable.push((name.clone(), min));
            assert!(
                unreachable_arm_reason(&name).is_some(),
                "{name} carries a period-1 identity arm its own range {:?} makes unreachable, and \
                 is not listed in UNREACHABLE_ARM. Either widen the range (making the promise \
                 real, and the flag required) or record why the arm stays.",
                period_range(f)
            );
            continue;
        }
        if !has_flag(f) {
            missing.push(name);
        }
    }

    assert!(
        missing.is_empty(),
        "function(s) {missing:?} carry a period-1 identity arm but do not declare \
         `flags: [{FLAG}]`.\n\
         Without the flag the arm is never checked: test_period_boundary.c sweeps the flagged \
         set, so an undeclared arm can be deleted or broken by a later refactor with nothing \
         to notice. This is how VWMA's (P*V)/V shipped."
    );
    detected.sort();
    unreachable.sort();
    println!(
        "period1_identity: identity arm detected in {}: {detected:?}\n\
         period1_identity: unreachable arm(s) (range forbids a period of 1): {unreachable:?}",
        detected.len()
    );
    assert!(
        detected.len() >= 8,
        "the identity detector found only {} arm(s) -- it used to find at least 8, so either \
         the arms are being deleted or the detector stopped recognising them",
        detected.len()
    );
    // The other direction: a name whose arm became reachable (or whose arm was
    // deleted) must leave the list rather than sit there stating something that
    // stopped being true.
    for (name, _) in UNREACHABLE_ARM {
        assert!(
            unreachable.iter().any(|(n, _)| n == name),
            "UNREACHABLE_ARM lists `{name}`, but it no longer has an arm its range forbids. If \
             the range was widened, the flag is now required; drop the entry either way."
        );
    }

    // MACD/MACDFIX degenerate their signal stage only. If the detector ever
    // starts claiming them, the flag's meaning has drifted and the runtime sweep
    // would compare a 3-output function against its own input.
    let claimed: HashSet<&str> = detected.iter().map(String::as_str).collect();
    for name in ["MACD", "MACDFIX"] {
        assert!(
            !claimed.contains(name),
            "{name}'s period-1 arm is a per-step signal degeneracy, not a whole-function \
             identity -- it must not be detected as one"
        );
    }
}
