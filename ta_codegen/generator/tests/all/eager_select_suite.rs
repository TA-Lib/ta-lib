//! A select arm stores what the plain store would, converted to the
//! destination's type.

use crate::common::{generate_all, load_indicator_with_source};

const REAL: &str = r"
int mom_lookback(int optInTimePeriod)
{
   return optInTimePeriod;
}

TA_RetCode mom(int startIdx, int endIdx,
   const double inReal[],
   int optInTimePeriod,
   int *outBegIdx, int *outNBElement,
   double outReal[])
{
   int inIdx, outIdx, trailingIdx;
   if( startIdx < optInTimePeriod )
      startIdx = optInTimePeriod;
   if( startIdx > endIdx )
   {
      *outBegIdx = 0;
      *outNBElement = 0;
      return TA_SUCCESS;
   }
   outIdx      = 0;
   inIdx       = startIdx;
   trailingIdx = startIdx - optInTimePeriod;
   while( inIdx <= endIdx ) {
      if( inReal[inIdx] > inReal[trailingIdx] )
         outReal[outIdx] = inReal[inIdx] - inReal[trailingIdx];
      else
         outReal[outIdx] = 0;
      outIdx++;
      inIdx++;
      trailingIdx++;
   }
   *outNBElement = outIdx;
   *outBegIdx    = startIdx;
   return TA_SUCCESS;
}
";

/// `LOOP` is replaced by the pattern loop's body.
const INTEGER: &str = r"
int cdl3outside_lookback( void )
{
   return 3;
}

TA_RetCode cdl3outside(int startIdx, int endIdx,
   const double inOpen[],
   const double inHigh[],
   const double inLow[],
   const double inClose[],
   int *outBegIdx, int *outNBElement,
   int outInteger[])
{
   int i, outIdx, lookbackTotal;
   lookbackTotal = cdl3outside_lookback();
   if( startIdx < lookbackTotal ) {
      startIdx = lookbackTotal;
   }
   if( startIdx > endIdx )
   {
      *outBegIdx = 0;
      *outNBElement = 0;
      return TA_SUCCESS;
   }
   i = startIdx;
   outIdx = 0;
   do
   {
      LOOP
      outIdx++;
      i++;
   } while( i <= endIdx );
   *outNBElement = outIdx;
   *outBegIdx    = startIdx;
   return TA_SUCCESS;
}
";

fn rust_of(name: &str, src: &str) -> String {
    let (func, enums) = load_indicator_with_source(name, src);
    generate_all(&func, &enums).rust
}

#[test]
fn an_integer_literal_arm_into_a_real_output_is_a_real() {
    let rust = rust_of("mom", REAL);
    let line = rust.lines().find(|l| l.contains("select_unpredictable(")).expect("the store folds");
    assert!(line.trim_end().ends_with(", 0.0);"), "{line}");
}

/// Only names declared real may run eagerly; an integer one stays behind its test.
#[test]
fn an_integer_name_arm_stays_a_branch() {
    let src = INTEGER.replace(
        "LOOP",
        "if( inClose[i] > inOpen[i] && inHigh[i] > inLow[i] ) outInteger[outIdx] = lookbackTotal; else outInteger[outIdx] = 0;",
    );
    let rust = rust_of("cdl3outside", &src);
    assert!(!rust.contains("select_unpredictable("), "{rust}");
    let folds = rust_of("cdl3outside", &INTEGER.replace("LOOP", "if( inClose[i] > inOpen[i] && inHigh[i] > inLow[i] ) outInteger[outIdx] = 100; else outInteger[outIdx] = 0;"));
    assert!(folds.contains("select_unpredictable("), "the control folds");
}

#[test]
fn an_index_difference_arm_stays_a_branch() {
    let src = INTEGER.replace("LOOP", "if( i > 2 ) outInteger[outIdx] = i - 3; else outInteger[outIdx] = 0;");
    let (func, enums) = load_indicator_with_source("cdl3outside", &src);
    let all = generate_all(&func, &enums);
    assert!(!all.rust.contains("select_unpredictable("));
    assert!(all.rust.contains("if i > 2"), "the branch is gone:\n{}", all.rust);
}

fn c_of(name: &str, src: &str) -> String {
    let (func, enums) = load_indicator_with_source(name, src);
    generate_all(&func, &enums).c
}

fn store_line<'a>(text: &'a str, needle: &str) -> &'a str {
    text.lines().find(|l| l.contains(needle)).unwrap_or_else(|| panic!("no `{needle}` in:\n{text}"))
}

#[test]
fn c_picks_an_integer_store_by_mask_and_a_real_one_by_ternary() {
    let src = INTEGER.replace(
        "LOOP",
        "if( inClose[i] > inOpen[i] && inHigh[i] > inLow[i] ) outInteger[outIdx] = 100; else outInteger[outIdx] = 0;",
    );
    let c = c_of("cdl3outside", &src);
    assert!(store_line(&c, "outInteger[outIdx] =").contains("((100) & -(int)(((inClose[i] > inOpen[i]) & (inHigh[i] > inLow[i]))))"));
    let src = INTEGER.replace(
        "LOOP",
        "if( inClose[i] > inOpen[i] ) outInteger[outIdx] = 100; else outInteger[outIdx] = -100;",
    );
    let c = c_of("cdl3outside", &src);
    let line = store_line(&c, "outInteger[outIdx] =");
    assert!(line.contains("((-100) ^ (((100) ^ (-100)) & -(int)(inClose[i] > inOpen[i])))"), "{line}");
    let c = c_of("mom", REAL);
    let line = store_line(&c, "outReal[outIdx] =");
    assert!(line.contains(" ? (") && !line.contains("-(int)"), "{line}");
}

#[test]
fn colour_flags_compare_as_booleans_in_both_backends() {
    let src = INTEGER.replace(
        "LOOP",
        "if( ((inClose[i] >= inOpen[i]) ? 1 : -1) == -((inClose[i-1] >= inOpen[i-1]) ? 1 : -1) && \
         ((inClose[i-2] >= inOpen[i-2]) ? 1 : -1) == -1 ) outInteger[outIdx] = 100; else outInteger[outIdx] = 0;",
    );
    let (func, enums) = load_indicator_with_source("cdl3outside", &src);
    let all = generate_all(&func, &enums);
    let c = store_line(&all.c, "outInteger[outIdx] =");
    assert!(c.contains("((inClose[i] >= inOpen[i]) != (inClose[i - 1] >= inOpen[i - 1])) & (!(inClose[i - 2] >= inOpen[i - 2]))"), "{c}");
    let rust = store_line(&all.rust, "select_unpredictable(");
    assert!(rust.contains("((_w0[_wk + 2] >= _w1[_wk + 2]) != (_w0[_wk + 1] >= _w1[_wk + 1])) & (!(_w0[_wk] >= _w1[_wk]))"), "{rust}");
}

#[test]
fn a_nested_integer_tree_writes_each_condition_once() {
    let src = INTEGER.replace(
        "LOOP",
        "if( inClose[i] > inOpen[i] ) outInteger[outIdx] = 100; \
         else if( inHigh[i] > inLow[i] ) outInteger[outIdx] = -100; \
         else outInteger[outIdx] = 0;",
    );
    let c = c_of("cdl3outside", &src);
    let line = store_line(&c, "outInteger[outIdx] =");
    assert!(
        line.contains("((100) ^ (((((-100) & -(int)(inHigh[i] > inLow[i]))) ^ (100)) & ((int)(inClose[i] > inOpen[i]) - 1)))"),
        "{line}"
    );
}

const PRE_CHECK: &str = "if( LEAD ) { if( inClose[i] > inOpen[i] ) outInteger[outIdx] = 100; else outInteger[outIdx] = 0; } else outInteger[outIdx] = 0;";

/// The C and Rust lines that test `lead`, with the inner tree folded below.
fn pre_check_lines(lead: &str) -> [String; 2] {
    let src = INTEGER.replace("LOOP", &PRE_CHECK.replace("LEAD", lead));
    let (func, enums) = load_indicator_with_source("cdl3outside", &src);
    let all = generate_all(&func, &enums);
    assert!(all.rust.contains("select_unpredictable(") && all.c.contains("& -(int)"), "the inner tree folds:\n{}", all.rust);
    let test = |t: &str| t.lines().find(|l| l.trim_start().starts_with("if") && l.contains("inOpen[i")).map(str::to_string).unwrap_or_else(|| panic!("no pre-check in:\n{t}"));
    [test(&all.c), test(&all.rust)]
}

#[test]
fn a_pre_check_term_that_may_trap_or_act_keeps_the_short_circuit() {
    for lead in [
        "i > 3 && inClose[i-4] > inOpen[i]",
        "lookbackTotal != 0 && i / lookbackTotal > 1 && inOpen[i] > 0",
        "inClose[i] > inOpen[i-1] && lookbackTotal++ > 0",
        "inClose[i] > inOpen[i-1] && sqrt(inHigh[i] - inLow[i]) > 1.0",
        "inClose[i] > inOpen[i-1] && 100 / (inClose[i] > inOpen[i]) > 0",
    ] {
        for line in pre_check_lines(lead) {
            assert!(line.contains(" && ") && !line.contains("== 2") && !line.contains("== 3"), "{lead}: {line}");
        }
    }
    for line in pre_check_lines("inClose[i] > inOpen[i-1] && inHigh[i] > inLow[i]") {
        assert!(line.contains("== 2") && !line.contains(" && "), "{line}");
    }
}

#[test]
fn a_for_init_below_the_lookback_keeps_the_read_behind_its_guard() {
    let src = INTEGER
        .replace("int i, outIdx, lookbackTotal;", "int i, outIdx, lookbackTotal;\n   double armed;")
        .replace(
            "   do\n   {\n      LOOP\n      outIdx++;\n      i++;\n   } while( i <= endIdx );\n",
            "   armed = 0.0;\n   for( i = startIdx - 3; i <= endIdx; i++ )\n   {\n      if( armed > 0.5 && inClose[i-3] > inOpen[i] ) outInteger[outIdx] = 100; else outInteger[outIdx] = 0;\n      outIdx++;\n      if( i >= 3 ) armed = 1.0;\n   }\n",
        );
    assert!(src.contains("for( i = startIdx - 3"));
    let (func, enums) = load_indicator_with_source("cdl3outside", &src);
    let all = generate_all(&func, &enums);
    assert!(!all.rust.contains("select_unpredictable(") && !all.c.contains("& -(int)"), "{}", store_line(&all.c, "outInteger[outIdx] ="));
}
