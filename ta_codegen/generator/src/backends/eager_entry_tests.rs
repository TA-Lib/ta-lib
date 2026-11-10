//! How a loop is entered bounds which input reads may run eagerly. Each body
//! here reads `inClose[i-2]` under a guard the allowlist admits, so the read
//! folds only where the entry proves `i >= 2` on every pass.

use super::eager_select::{rewrite, Declared, EAGER_ALL, GUARD, SELECT};
use crate::helper_registry::HelperRegistry;
use crate::ir::{Output, ParamType};

const HEAD: &str = "TA_RetCode f( int startIdx, int endIdx, const double inClose[], const double inOpen[], const double inHigh[], const double inLow[], double x, double lim, int optInP, int *outBegIdx, int *outNBElement, int outInteger[] )\n{\n   int i, j, k, lookbackTotal;\n   double d, e;\n   int *q;\n";
const TERM: &str = "if( x > 1.0 && inClose[i-2] > x ) outInteger[k++] = 100; else outInteger[k++] = 0;";
const CLAMP: &str = "   lookbackTotal = f_lookback();\n   if( startIdx < lookbackTotal ) {\n      startIdx = lookbackTotal;\n   }\n";
const EXIT: &str = "   if( startIdx > endIdx ) {\n      return TA_SUCCESS;\n   }\n";

fn folds_with(src: &str, declared: &Declared) -> bool {
    folds_under(HEAD, src, declared)
}

fn folds_under(head: &str, src: &str, declared: &Declared) -> bool {
    let text = format!("{head}{src}\n   return TA_SUCCESS;\n}}\n");
    let parsed = crate::parser::c_source::parse_c_source_str(&text);
    let f = parsed.functions.into_iter().find(|f| f.name == "f").expect("parses");
    let outputs = [Output { name: "outInteger".into(), param_type: ParamType::Integer, flags: Vec::new(), levels: Vec::new() }];
    let t = format!("{:?}", rewrite(&f.body, declared, &outputs, &HelperRegistry::empty()));
    t.contains(SELECT) || t.contains(GUARD) || t.contains(EAGER_ALL)
}

/// A lookback of `max(...) + 2`, taking `params`.
fn declared(params: &[&str]) -> Declared {
    Declared {
        inputs: ["inClose", "inOpen", "inHigh", "inLow"].map(String::from).to_vec(),
        arrays: ["inClose", "inOpen", "inHigh", "inLow"].map(String::from).to_vec(),
        reals: ["x", "lim"].map(String::from).to_vec(),
        params: params.iter().map(|p| (*p).to_string()).collect(),
        lookback: "f_lookback".into(),
        fixed: 2,
    }
}

fn folds(src: &str) -> bool {
    folds_with(src, &declared(&[]))
}

fn while_loop(term: &str) -> String {
    format!("   k = 0;\n   while( i <= endIdx )\n   {{\n      {term}\n      i++;\n   }}\n")
}

fn do_loop(term: &str) -> String {
    format!("   k = 0;\n   do\n   {{\n      {term}\n      i++;\n   }} while( i <= endIdx );\n")
}

#[test]
fn the_canonical_prologue_admits_reads_back_to_the_lookback() {
    assert!(folds(&format!("{CLAMP}{EXIT}   i = startIdx;\n{}", while_loop(TERM))));
    assert!(folds(&format!("{CLAMP}{EXIT}   i = startIdx;\n{}", do_loop(TERM))));
    let three = TERM.replace("i-2", "i-3");
    assert!(!folds(&format!("{CLAMP}{EXIT}   i = startIdx;\n{}", while_loop(&three))), "past the fixed part");
    assert!(!folds(&format!("   i = startIdx;\n{}", while_loop(TERM))), "no prologue");
    assert!(!folds(&format!("{CLAMP}   i = startIdx;\n{}", while_loop(TERM))), "no early return");
}

#[test]
fn an_initialized_lookback_declaration_keeps_the_prologue() {
    let src = format!("{CLAMP}{EXIT}   i = startIdx;\n{}", while_loop(TERM));
    assert!(folds_under(&HEAD.replace("lookbackTotal;", "lookbackTotal = 0;"), &src, &declared(&[])));
}

#[test]
fn a_prologue_out_of_place_proves_nothing() {
    assert!(!folds(&format!("   i = startIdx;\n{CLAMP}{EXIT}{}", while_loop(TERM))), "the step took startIdx first");
    assert!(!folds(&format!("   i = startIdx;\n{}{CLAMP}{EXIT}", while_loop(TERM))), "the prologue after the loop");
    let nested = format!("   lookbackTotal = f_lookback();\n   if( x > 0.0 ) {{\n      if( startIdx < lookbackTotal ) {{\n         startIdx = lookbackTotal;\n      }}\n   }}\n{EXIT}   i = startIdx;\n{}", while_loop(TERM));
    assert!(!folds(&nested), "a clamp inside a conditional");
}

#[test]
fn a_written_or_aliased_bound_proves_nothing() {
    let after = |extra: &str| format!("{CLAMP}{EXIT}{extra}   i = startIdx;\n{}", while_loop(TERM));
    assert!(!folds(&after("   j = startIdx--;\n")), "startIdx lowered in an expression");
    assert!(!folds(&after("   q = &startIdx;\n   *q = 0;\n")), "startIdx address-taken");
    let lowered = format!("   lookbackTotal = f_lookback();\n   j = lookbackTotal--;\n   if( startIdx < lookbackTotal ) {{\n      startIdx = lookbackTotal;\n   }}\n{EXIT}   i = startIdx;\n{}", while_loop(TERM));
    assert!(!folds(&lowered), "the clamp value lowered");
    let param = |write: &str| {
        let src = format!("   lookbackTotal = f_lookback(optInP);\n   if( startIdx < lookbackTotal ) {{\n      startIdx = lookbackTotal;\n   }}\n{EXIT}{write}   i = startIdx;\n{}", while_loop(TERM));
        folds_with(&src, &declared(&["optInP"]))
    };
    assert!(param(""), "the lookback given its own parameters");
    assert!(!param("   if( x > 0.0 ) {\n      optInP = 0;\n   }\n"), "a parameter written in a nested block");
    let other = format!("   lookbackTotal = f_lookback(1);\n   if( startIdx < lookbackTotal ) {{\n      startIdx = lookbackTotal;\n   }}\n{EXIT}   i = startIdx;\n{}", while_loop(TERM));
    assert!(!folds_with(&other, &declared(&["optInP"])), "the lookback given another argument");
    let param_clamp = format!("   if( startIdx < optInP ) {{\n      startIdx = optInP;\n   }}\n   i = startIdx;\n{}", while_loop(TERM));
    assert!(!folds_with(&param_clamp, &declared(&["optInP"])), "a parameter clamp raises the floor no higher than 0");
}

#[test]
fn the_loop_test_bounds_a_read_only_when_nothing_moves_it() {
    let at_i = "if( x > 1.0 && inClose[i] > x ) outInteger[k++] = 100; else outInteger[k++] = 0;";
    let clamp0 = "   if( startIdx < 0 ) {\n      startIdx = 0;\n   }\n";
    assert!(folds(&format!("{clamp0}   i = startIdx;\n{}", while_loop(at_i))));
    assert!(!folds(&format!("{clamp0}   endIdx = endIdx + 3;\n   i = startIdx;\n{}", while_loop(at_i))), "endIdx written");
    let moved = format!("{CLAMP}{EXIT}   i = startIdx;\n   j = 0;\n   k = 0;\n   while( --i <= endIdx - j )\n   {{\n      e = inClose[i];\n      {TERM}\n      i++;\n      j++;\n   }}\n");
    assert!(!folds(&moved), "the step written in the loop test");
    let late_exit = format!("{CLAMP}   i = startIdx;\n{}{EXIT}", do_loop(at_i));
    assert!(!folds(&late_exit), "a do-while whose early return comes after it");
    let late_clamp = format!("{CLAMP}{EXIT}   if( startIdx < 5 ) {{\n      startIdx = 5;\n   }}\n   i = startIdx;\n{}", do_loop(at_i));
    assert!(!folds(&late_clamp), "a do-while whose early return comes before the last clamp");
    assert!(folds(&format!("{CLAMP}{EXIT}   i = startIdx;\n{}", do_loop(at_i))));
}

#[test]
fn only_reads_every_pass_makes_are_sure() {
    let average = format!("   i = startIdx;\n   k = 0;\n   while( i <= endIdx )\n   {{\n      e = ta_candleaverage(BodyLong_rangeType, BodyLong_avgPeriod, BodyLong_factor, d, inOpen[i-2], inHigh[i-2], inLow[i-2], inClose[i-2]);\n      {TERM}\n      i++;\n   }}\n");
    assert!(!folds(&average), "the candle average reads its bar only for a zero period");
    let range = average.replace("ta_candleaverage(BodyLong_rangeType, BodyLong_avgPeriod, BodyLong_factor, d,", "ta_candlerange(BodyLong_rangeType,");
    assert!(folds(&range), "the candle range reads its bar for every range type");
    let continued = format!("{CLAMP}{EXIT}   i = startIdx;\n   k = 0;\n   while( i <= endIdx )\n   {{\n      e = inClose[i-2];\n      {TERM}\n      if( x > 3.0 ) {{\n         continue;\n      }}\n      i++;\n   }}\n");
    assert!(!folds(&continued), "a pass that may end early");
    let twice = format!("   i = startIdx;\n   k = 0;\n   while( i <= endIdx )\n   {{\n      e = inClose[i-2];\n      {TERM}\n      i++;\n      i++;\n   }}\n");
    assert!(!folds(&twice), "a step written twice");
    assert!(!folds(&format!("{CLAMP}{EXIT}   i = startIdx - 1;\n{}", while_loop(TERM))), "a step entering below startIdx");
}

#[test]
fn a_for_init_is_the_step_entry() {
    let for_loop = |init: &str| format!("   k = 0;\n   for( {init}; i <= endIdx; i++ )\n   {{\n      {TERM}\n   }}\n");
    assert!(!folds(&format!("{CLAMP}{EXIT}   i = startIdx;\n{}", for_loop("i = startIdx - 2"))), "the init lowers the step");
    assert!(!folds(&format!("{CLAMP}{EXIT}   i = startIdx;\n{}", for_loop("i = 0"))), "the init resets the step");
    assert!(folds(&format!("{CLAMP}{EXIT}{}", for_loop("i = startIdx"))));
}

#[test]
fn a_step_written_after_its_entry_proves_nothing() {
    for write in ["i--;", "j = i--;", "i = i - 5;", "i -= 5;"] {
        assert!(!folds(&format!("{CLAMP}{EXIT}   i = startIdx;\n   {write}\n{}", while_loop(TERM))), "{write}");
    }
}

#[test]
fn a_step_aliased_anywhere_proves_nothing() {
    let lowered = format!("{CLAMP}{EXIT}   q = &i;\n   i = startIdx;\n   k = 0;\n   while( i <= endIdx )\n   {{\n      if( x < 0.5 ) {{\n         *q = 0;\n      }}\n      {TERM}\n      i++;\n   }}\n");
    assert!(!folds(&lowered), "lowered through a pointer taken before the loop");
    let after_sure = format!("   q = &i;\n   i = startIdx;\n   k = 0;\n   while( i <= endIdx )\n   {{\n      e = inClose[i-2];\n      *q = 0;\n      {TERM}\n      i++;\n   }}\n");
    assert!(!folds(&after_sure), "lowered between the sure read and the eager one");
}

#[test]
fn a_real_local_runs_eagerly_only_once_written() {
    let lagged = |init: &str| {
        format!("{init}   e = 0.0;\n   i = startIdx;\n   k = 0;\n   while( i <= endIdx )\n   {{\n      if( e > 0.5 && d > inClose[i] ) outInteger[k++] = 100; else outInteger[k++] = 0;\n      d = inClose[i];\n      e = 1.0;\n      i++;\n   }}\n")
    };
    assert!(!folds(&lagged("")), "d is read before any write on the first pass");
    assert!(folds(&lagged("   d = 0.0;\n")));
    let first = lagged("").replace("      d = inClose[i];\n", "").replace("   {\n      if(", "   {\n      d = inClose[i];\n      if(");
    assert!(folds(&first), "written first in the pass");
}

#[test]
fn only_a_non_negative_max_credits_the_lookback_constant() {
    let base = std::path::Path::new(env!("CARGO_MANIFEST_DIR")).join("../input/cdl3outside");
    let fixed = |ret: &str| {
        let mut f = crate::parser::yaml::parse_yaml(&base.join("cdl3outside.yaml"));
        let text = std::fs::read_to_string(base.join("cdl3outside.c")).expect("the source").replacen("return 3;", &format!("return {ret};"), 1);
        crate::parser::c_source::wire_parsed_source(&mut f, &crate::parser::c_source::parse_c_source_str(&text));
        Declared::of(&f).fixed
    };
    assert_eq!(fixed("max( BodyShort_avgPeriod, BodyLong_avgPeriod ) + 2"), 2);
    assert_eq!(fixed("max( max( optInA, BodyLong_avgPeriod ), optInB ) + 3"), 3);
    assert_eq!(fixed("max( 1, optInA ) + 5"), 5);
    assert_eq!(fixed("max( optInA - 5, optInB - 5 ) + 5"), 0);
    assert_eq!(fixed("max( -1, optInA ) + 5"), 0);
}

/// Every candlestick function whose decision the pass rewrites keeps it, so an
/// input edit that silently stops a fold fails here, by name.
#[test]
fn every_candlestick_fold_in_the_corpus_holds() {
    // Left as branches today.
    const NOT_FOLDED: &[&str] = &["cdldoji", "cdlhikkake", "cdlhikkakemod", "cdltristar"];
    let base = std::path::Path::new(env!("CARGO_MANIFEST_DIR")).join("../input");
    let helpers = HelperRegistry::from_dir(&base);
    let mut names: Vec<String> = std::fs::read_dir(&base)
        .expect("the corpus")
        .filter_map(|e| e.ok()?.file_name().into_string().ok())
        .filter(|n| n.starts_with("cdl"))
        .collect();
    names.sort();
    assert!(names.len() > 50, "the candlestick corpus is missing");
    let mut lost = Vec::new();
    for n in &names {
        let mut f = crate::parser::yaml::parse_yaml(&base.join(format!("{n}/{n}.yaml")));
        let parsed = crate::parser::c_source::parse_c_source(&base.join(format!("{n}/{n}.c")));
        crate::parser::c_source::wire_parsed_source(&mut f, &parsed);
        let t = format!("{:?}", rewrite(&f.body, &Declared::of(&f), &f.outputs, &helpers));
        let folded = t.contains(SELECT) || t.contains(GUARD) || t.contains(EAGER_ALL);
        if folded == NOT_FOLDED.contains(&n.as_str()) {
            lost.push(n.clone());
        }
    }
    assert!(lost.is_empty(), "fold state changed: {lost:?}");
}
