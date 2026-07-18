//! Generate the `ta_abstract` C layer from YAML-derived IR.
//!
//! Produces all files under `ta_codegen/output/c/ta_abstract/`:
//! - `ta_def_ui.h` / `ta_def_ui.c`
//! - `ta_frame_priv.h`
//! - `frames/ta_frame.h` / `frames/ta_frame.c`
//! - `tables/table_a.c` .. `tables/table_z.c`
//! - `ta_group_idx.c`
//! - `ta_abstract.c`
//! - `ta_func_api.c`

use std::collections::HashMap;
use std::fmt::Write as _;
use std::path::Path;

use super::write_if_changed_silent;
use crate::ir::{EnumDef, FuncDef, Input, OptInput, Output, ParamType};

// ---------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------

const LICENSE: &str = "\
/* TA-LIB Copyright (c) 1999-2025, Mario Fortier
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the following
 * conditions are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in
 *   the documentation and/or other materials provided with the
 *   distribution.
 *
 * - Neither name of author nor the names of its contributors
 *   may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * REGENTS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */\n\n";

/// Canonical group order (matches `TA_GroupId` enum values 0..9).
const GROUPS: &[(&str, &str)] = &[
    ("Math Operators", "MathOperators"),
    ("Math Transform", "MathTransform"),
    ("Overlap Studies", "OverlapStudies"),
    ("Volatility Indicators", "VolatilityIndicators"),
    ("Momentum Indicators", "MomentumIndicators"),
    ("Cycle Indicators", "CycleIndicators"),
    ("Volume Indicators", "VolumeIndicators"),
    ("Pattern Recognition", "PatternRecognition"),
    ("Statistic Functions", "Statistic"),
    ("Price Transform", "PriceTransform"),
];

// ---------------------------------------------------------------------------
// Public entry point
// ---------------------------------------------------------------------------

/// Generate all `ta_abstract` C files into `out_base/c/ta_abstract/`.
#[allow(clippy::implicit_hasher)]
pub fn generate(
    funcs: &[FuncDef],
    enums: &HashMap<String, EnumDef>,
    out_base: &Path,
) {
    // out_base is `<root>/ta_codegen/output`, so the repo root is two levels up.
    // Canonical cutover option B: the ta_abstract introspection layer is generated
    // in place under `src/ta_abstract` (the shipped library), replacing gen_code.
    // gen_code and its hand-written-only files (ta_java_defs.h, templates/) were
    // removed in the Stage 7 cutover; only the generated tables/frames live here now.
    let repo_root = out_base.parent().unwrap().parent().unwrap();
    let base = repo_root.join("src/ta_abstract");
    std::fs::create_dir_all(base.join("tables")).unwrap();
    std::fs::create_dir_all(base.join("frames")).unwrap();

    // Sort functions alphabetically for deterministic output.
    let mut sorted: Vec<&FuncDef> = funcs.iter().collect();
    sorted.sort_by(|a, b| a.name.cmp(&b.name));

    // Build per-group lists (sorted alphabetically within group).
    let mut groups: Vec<Vec<&FuncDef>> = vec![Vec::new(); GROUPS.len()];
    for func in &sorted {
        if let Some(idx) = group_index(&func.group) {
            groups[idx].push(func);
        }
    }

    write_if_changed_silent(&base.join("ta_def_ui.h"), &gen_def_ui_h());
    write_if_changed_silent(&base.join("ta_def_ui.c"), &gen_def_ui_c(enums));
    write_if_changed_silent(&base.join("ta_frame_priv.h"), &gen_frame_priv_h());
    write_if_changed_silent(
        &base.join("frames").join("ta_frame.h"),
        &gen_frame_h(&sorted),
    );
    write_if_changed_silent(
        &base.join("frames").join("ta_frame.c"),
        &gen_frame_c(&sorted),
    );

    // Per-letter table files.
    for letter in b'a'..=b'z' {
        let ch = letter as char;
        let letter_funcs: Vec<&&FuncDef> = sorted
            .iter()
            .filter(|f| {
                f.name
                    .chars()
                    .next()
                    .is_some_and(|c| c.to_ascii_lowercase() == ch)
            })
            .collect();
        let filename = format!("table_{ch}.c");
        write_if_changed_silent(
            &base.join("tables").join(&filename),
            &gen_table_file(ch, &letter_funcs),
        );
    }

    write_if_changed_silent(
        &base.join("ta_group_idx.c"),
        &gen_group_idx(&sorted, &groups),
    );
    write_if_changed_silent(&base.join("ta_abstract.c"), &gen_ta_abstract_c());

    write_if_changed_silent(&base.join("ta_func_api.c"), &gen_ta_func_api_c(repo_root));

    // Generate include/ta_func.h — this replaces gen_code's role.
    // Batch prototypes MUST stay byte-identical to the pre-cutover original
    // (backward compatibility); the streaming declarations are additive.
    write_if_changed_silent(
        &repo_root.join("include").join("ta_func.h"),
        &gen_ta_func_h(&sorted),
    );

    println!(
        "  ta_abstract: generated {} files ({} functions)",
        8 + 26 + 1,
        sorted.len()
    );
}

// ---------------------------------------------------------------------------
// File 1: ta_def_ui.h
// ---------------------------------------------------------------------------

#[allow(clippy::too_many_lines)]
fn gen_def_ui_h() -> String {
    let mut o = String::new();
    o.push_str(LICENSE);
    o.push_str("#ifndef TA_DEF_UI_H\n#define TA_DEF_UI_H\n\n");
    o.push_str("#ifndef TA_ABSTRACT_H\n   #include \"ta_abstract.h\"\n#endif\n\n");
    o.push_str("#ifndef TA_FRAME_PRIV_H\n   #include \"ta_frame_priv.h\"\n#endif\n\n");
    o.push_str(
        "#if !defined(TA_GEN_CODE) && !defined( TA_FRAME_H )\n   \
         #include \"ta_frame.h\"\n#endif\n\n",
    );

    // GroupId enum
    o.push_str("typedef enum\n{\n");
    for (_, id_suffix) in GROUPS {
        let _ = writeln!(o, "  TA_GroupId_{id_suffix},");
    }
    o.push_str("  TA_NB_GROUP_ID\n} TA_GroupId;\n\n");

    // Group string externs
    for (_, id_suffix) in GROUPS {
        let _ = writeln!(o, "extern const char TA_GroupId_{id_suffix}String[];");
    }
    o.push('\n');
    o.push_str("extern const char *TA_GroupString[TA_NB_GROUP_ID];\n\n");

    // Input externs
    o.push_str("/* Inputs */\n");
    for name in &[
        "Input_Real",
        "Input_Real0",
        "Input_Real1",
        "Input_Integer",
        "Input_Price_OHLCV",
        "Input_Price_HLCV",
        "Input_Price_OHLC",
        "Input_Price_HLC",
        "Input_Price_HL",
        "Input_Price_OC",
        "Input_Price_CV",
        "Input_Price_V",
        "Input_Periods",
    ] {
        let _ = writeln!(
            o,
            "extern const TA_InputParameterInfo TA_DEF_UI_{name};"
        );
    }
    o.push('\n');

    // Output externs
    o.push_str("/* Outputs. */\n");
    o.push_str("extern const TA_OutputParameterInfo TA_DEF_UI_Output_Real;\n");
    o.push_str("extern const TA_OutputParameterInfo TA_DEF_UI_Output_Integer;\n");
    o.push_str("extern const TA_OutputParameterInfo TA_DEF_UI_Output_Lines;\n\n");

    // OptInput externs
    o.push_str("/* Optional Inputs. */\n");
    for name in &[
        "TimePeriod_30",
        "TimePeriod_14",
        "TimePeriod_10",
        "TimePeriod_5",
        "TimePeriod_30_MINIMUM2",
        "TimePeriod_20_MINIMUM2",
        "TimePeriod_21_MINIMUM2",
        "TimePeriod_14_MINIMUM2",
        "TimePeriod_14_MINIMUM5",
        "TimePeriod_10_MINIMUM2",
        "TimePeriod_5_MINIMUM2",
        "VerticalShift",
        "HorizontalShift",
        "MA_Method",
        "Fast_Period",
        "Slow_Period",
        "NbDeviation",
        "Penetration_30",
        "Penetration_50",
        "MinPeriod",
        "MaxPeriod",
    ] {
        let _ = writeln!(
            o,
            "extern const TA_OptInputParameterInfo TA_DEF_UI_{name};"
        );
    }
    o.push('\n');

    // Range externs
    o.push_str("/* Re-usable ranges. */\n");
    o.push_str("extern const TA_IntegerRange TA_DEF_TimePeriod_Positive;\n");
    o.push_str("extern const TA_IntegerRange TA_DEF_TimePeriod_Positive_Minimum2;\n");
    o.push_str("extern const TA_IntegerRange TA_DEF_TimePeriod_Positive_Minimum5;\n\n");
    o.push_str("extern const TA_RealRange    TA_DEF_VerticalShiftPercent;\n");
    o.push_str("extern const TA_IntegerRange TA_DEF_HorizontalShiftPeriod;\n");
    o.push_str("extern const TA_RealRange    TA_DEF_NbDeviation;\n");
    o.push_str("extern const TA_RealRange    TA_DEF_ZeroToOne;\n");
    o.push_str("extern const TA_RealRange    TA_DEF_RealPositive;\n\n");

    // MA type list
    o.push_str("extern const TA_IntegerList TA_MA_TypeList;\n\n");

    // TA_FuncDef struct
    o.push_str(
        "typedef struct\n\
         {\n\
         \x20  const unsigned int magicNumber;\n\
         \x20  const TA_GroupId groupId;\n\
         \x20  const TA_FuncInfo * const funcInfo;\n\
         \x20  const TA_InputParameterInfo    * const input;\n\
         \x20  const TA_OptInputParameterInfo * const optInput;\n\
         \x20  const TA_OutputParameterInfo   * const output;\n\
         \x20  const TA_FrameFunction function;\n\
         \x20  const TA_FrameLookback lookback;\n\
         } TA_FuncDef;\n\n",
    );

    // DEF_FUNCTION macro
    o.push_str(
        "#if !defined( TA_GEN_CODE )\n\
         \x20  #define DEF_FUNCTION( name, \\\n\
         \x20                        groupId, \\\n\
         \x20                        hint, \\\n\
         \x20                        camelCaseName, \\\n\
         \x20                        flags ) \\\n\
         \x20  \\\n\
         \x20  TA_FuncInfo TA_INFO_##name; \\\n\
         \x20  \\\n\
         \x20  const TA_FuncDef TA_DEF_##name = \\\n\
         \x20  { \\\n\
         \x20     TA_FUNC_DEF_MAGIC_NB, \\\n\
         \x20     groupId, \\\n\
         \x20     &TA_INFO_##name, \\\n\
         \x20     (const TA_InputParameterInfo    * const)&TA_##name##_Inputs[0],    \\\n\
         \x20     (const TA_OptInputParameterInfo * const)&TA_##name##_OptInputs[0], \\\n\
         \x20     (const TA_OutputParameterInfo   * const)&TA_##name##_Outputs[0],   \\\n\
         \x20     TA_##name##_FramePP, \\\n\
         \x20     TA_##name##_FramePPLB \\\n\
         \x20  }; \\\n\
         \x20  TA_FuncInfo TA_INFO_##name = \\\n\
         \x20  { \\\n\
         \x20     (const char * const)#name, \\\n\
         \x20     (const char * const)groupId##String, \\\n\
         \x20     (const char * const)hint, \\\n\
         \x20     (const char * const)camelCaseName, \\\n\
         \x20     (const int)flags, \\\n\
         \x20     (sizeof(TA_##name##_Inputs)   / sizeof(TA_InputParameterInfo *))   - 1, \\\n\
         \x20     (sizeof(TA_##name##_OptInputs)/ sizeof(TA_OptInputParameterInfo *))- 1, \\\n\
         \x20     (sizeof(TA_##name##_Outputs)  / sizeof(TA_OutputParameterInfo *))  - 1, \\\n\
         \x20     (const TA_FuncHandle * const)&TA_DEF_##name \\\n\
         \x20  };\n\
         #else\n\
         \x20  #define DEF_FUNCTION( name, \\\n\
         \x20                        groupId, \\\n\
         \x20                        hint, \\\n\
         \x20                        camelCaseName, \\\n\
         \x20                        flags ) \\\n\
         \x20  \\\n\
         \x20  TA_FuncInfo TA_INFO_##name; \\\n\
         \x20  \\\n\
         \x20  const TA_FuncDef TA_DEF_##name = \\\n\
         \x20  { \\\n\
         \x20     TA_FUNC_DEF_MAGIC_NB, \\\n\
         \x20     groupId, \\\n\
         \x20     &TA_INFO_##name, \\\n\
         \x20     (const TA_InputParameterInfo    * const)&TA_##name##_Inputs[0],    \\\n\
         \x20     (const TA_OptInputParameterInfo * const)&TA_##name##_OptInputs[0], \\\n\
         \x20     (const TA_OutputParameterInfo   * const)&TA_##name##_Outputs[0],   \\\n\
         \x20     NULL, \\\n\
         \x20     NULL \\\n\
         \x20  }; \\\n\
         \x20  TA_FuncInfo TA_INFO_##name = \\\n\
         \x20  { \\\n\
         \x20     (const char * const)#name, \\\n\
         \x20     (const char * const)groupId##String, \\\n\
         \x20     (const char * const)hint, \\\n\
         \x20     (const char * const)camelCaseName, \\\n\
         \x20     (const int)flags, \\\n\
         \x20     (sizeof(TA_##name##_Inputs)   / sizeof(TA_InputParameterInfo *))   - 1, \\\n\
         \x20     (sizeof(TA_##name##_OptInputs)/ sizeof(TA_OptInputParameterInfo *))- 1, \\\n\
         \x20     (sizeof(TA_##name##_Outputs)  / sizeof(TA_OutputParameterInfo *))  - 1, \\\n\
         \x20     (const TA_FuncHandle * const)&TA_DEF_##name \\\n\
         \x20  };\n\
         #endif\n\n",
    );

    o.push_str("#define ADD_TO_TABLE(name) &TA_DEF_##name\n\n");

    // Utility macros for math unary/binary operators
    o.push_str(
        "#define DEF_MATH_UNARY_OPERATOR(NAME,HINT,CAMELCASENAME) \\\n\
         \tstatic const TA_InputParameterInfo    *TA_##NAME##_Inputs[]    = \\\n\
         { \\\n\
         \x20 &TA_DEF_UI_Input_Real, \\\n\
         \x20 NULL \\\n\
         }; \\\n\
         static const TA_OutputParameterInfo   *TA_##NAME##_Outputs[]   = \\\n\
         { \\\n\
         \x20 &TA_DEF_UI_Output_Real, \\\n\
         \x20 NULL \\\n\
         }; \\\n\
         \tstatic const TA_OptInputParameterInfo *TA_##NAME##_OptInputs[] = { NULL }; \\\n\
         DEF_FUNCTION( NAME, \\\n\
         \x20             TA_GroupId_MathTransform, \\\n\
         \x20             HINT, \\\n\
         \x20             CAMELCASENAME, \\\n\
         \x20             0 \\\n\
         \x20            );\n\n",
    );

    o.push_str(
        "#define DEF_MATH_BINARY_OPERATOR(NAME,HINT,CAMELCASENAME) \\\n\
         \tstatic const TA_InputParameterInfo    *TA_##NAME##_Inputs[]    = \\\n\
         { \\\n\
         \x20 &TA_DEF_UI_Input_Real0, \\\n\
         \x20 &TA_DEF_UI_Input_Real1, \\\n\
         \x20 NULL \\\n\
         }; \\\n\
         static const TA_OutputParameterInfo   *TA_##NAME##_Outputs[]   = \\\n\
         { \\\n\
         \x20 &TA_DEF_UI_Output_Real, \\\n\
         \x20 NULL \\\n\
         }; \\\n\
         \tstatic const TA_OptInputParameterInfo *TA_##NAME##_OptInputs[] = { NULL }; \\\n\
         DEF_FUNCTION( NAME, \\\n\
         \x20             TA_GroupId_MathOperators, \\\n\
         \x20             HINT, \\\n\
         \x20             CAMELCASENAME, \\\n\
         \x20             0 \\\n\
         \x20            );\n\n",
    );

    o.push_str("#endif\n");
    o
}

// ---------------------------------------------------------------------------
// File 2: ta_def_ui.c
// ---------------------------------------------------------------------------

#[allow(clippy::implicit_hasher, clippy::too_many_lines)]
fn gen_def_ui_c(enums: &HashMap<String, EnumDef>) -> String {
    let mut o = String::new();
    o.push_str(LICENSE);
    o.push_str("#include <stdlib.h>\n#include \"ta_abstract.h\"\n#include \"ta_def_ui.h\"\n\n");

    // Group strings
    for (display, id_suffix) in GROUPS {
        let _ = writeln!(
            o,
            "const char TA_GroupId_{id_suffix}String[] = \"{display}\";"
        );
    }
    o.push('\n');

    o.push_str("const char *TA_GroupString[TA_NB_GROUP_ID] =\n{\n");
    for (i, (_, id_suffix)) in GROUPS.iter().enumerate() {
        let comma = if i + 1 < GROUPS.len() { "," } else { "" };
        let _ = writeln!(o, "   &TA_GroupId_{id_suffix}String[0]{comma}");
    }
    o.push_str("};\n\n");

    // Input constants
    emit_price_input(&mut o, "OHLCV", "inPriceOHLCV",
        &["TA_IN_PRICE_OPEN", "TA_IN_PRICE_HIGH", "TA_IN_PRICE_LOW", "TA_IN_PRICE_CLOSE", "TA_IN_PRICE_VOLUME"]);
    emit_price_input(&mut o, "HLCV", "inPriceHLCV",
        &["TA_IN_PRICE_HIGH", "TA_IN_PRICE_LOW", "TA_IN_PRICE_CLOSE", "TA_IN_PRICE_VOLUME"]);
    emit_price_input(&mut o, "OHLC", "inPriceOHLC",
        &["TA_IN_PRICE_OPEN", "TA_IN_PRICE_HIGH", "TA_IN_PRICE_LOW", "TA_IN_PRICE_CLOSE"]);
    emit_price_input(&mut o, "HLC", "inPriceHLC",
        &["TA_IN_PRICE_HIGH", "TA_IN_PRICE_LOW", "TA_IN_PRICE_CLOSE"]);
    emit_price_input(&mut o, "HL", "inPriceHL",
        &["TA_IN_PRICE_HIGH", "TA_IN_PRICE_LOW"]);
    emit_price_input(&mut o, "OC", "inPriceOC",
        &["TA_IN_PRICE_OPEN", "TA_IN_PRICE_CLOSE"]);
    emit_price_input(&mut o, "CV", "inPriceCV",
        &["TA_IN_PRICE_CLOSE", "TA_IN_PRICE_VOLUME"]);
    emit_price_input(&mut o, "V", "inPriceV",
        &["TA_IN_PRICE_VOLUME"]);

    o.push_str("const TA_InputParameterInfo TA_DEF_UI_Input_Real =\n");
    o.push_str("                                  { TA_Input_Real, \"inReal\", 0 };\n\n");
    o.push_str("const TA_InputParameterInfo TA_DEF_UI_Input_Periods =\n");
    o.push_str("                                  { TA_Input_Real, \"inPeriods\", 0 };\n\n");
    o.push_str("const TA_InputParameterInfo TA_DEF_UI_Input_Real0 =\n");
    o.push_str("                                  { TA_Input_Real, \"inReal0\", 0 };\n\n");
    o.push_str("const TA_InputParameterInfo TA_DEF_UI_Input_Real1 =\n");
    o.push_str("                                  { TA_Input_Real, \"inReal1\", 0 };\n\n");
    o.push_str("const TA_InputParameterInfo TA_DEF_UI_Input_Integer =\n");
    o.push_str("                                  { TA_Input_Integer, \"inInteger\", 0 };\n\n");

    // Output constants
    o.push_str("const TA_OutputParameterInfo TA_DEF_UI_Output_Real =\n");
    o.push_str("                                  { TA_Output_Real, \"outReal\", TA_OUT_LINE };\n\n");
    o.push_str("const TA_OutputParameterInfo TA_DEF_UI_Output_Integer =\n");
    o.push_str("                                  { TA_Output_Integer, \"outInteger\", TA_OUT_LINE };\n\n");

    // Integer ranges
    emit_int_range(&mut o, "TA_DEF_TimePeriod_Positive", 1, 100_000, 1, 200, 1);
    emit_int_range(&mut o, "TA_DEF_TimePeriod_Positive_Minimum5", 5, 100_000, 5, 200, 1);
    emit_int_range(&mut o, "TA_DEF_TimePeriod_Positive_Minimum2", 2, 100_000, 4, 200, 1);
    emit_int_range(&mut o, "TA_DEF_HorizontalShiftPeriod", -200, 200, 0, 8, 1);

    // Real ranges
    emit_real_range(&mut o, "TA_DEF_VerticalShiftPercent", "-99.0", "99.0", 1, "-10.0", "10.0", "0.5");
    emit_real_range(&mut o, "TA_DEF_NbDeviation", "TA_REAL_MIN", "TA_REAL_MAX", 2, "-2.0", "2.0", "0.2");
    emit_real_range(&mut o, "TA_DEF_ZeroToOne", "0.00", "1.00", 2, "0.01", "1.00", "0.05");
    emit_real_range(&mut o, "TA_DEF_RealPositive", "0.00", "TA_REAL_MAX", 0, "0.0", "0.0", "0.0");

    // Pre-defined OptInput constants
    emit_opt_input_int_const(&mut o, "TA_DEF_UI_MinPeriod", "optInMinPeriod",
        "Minimum Period", "TA_DEF_TimePeriod_Positive_Minimum2", 2,
        "Value less than minimum will be changed to Minimum period");
    emit_opt_input_int_const(&mut o, "TA_DEF_UI_MaxPeriod", "optInMaxPeriod",
        "Maximum Period", "TA_DEF_TimePeriod_Positive_Minimum2", 30,
        "Value higher than maximum will be changed to Maximum period");

    for (name, default) in &[
        ("TimePeriod_30_MINIMUM2", 30),
        ("TimePeriod_20_MINIMUM2", 20),
        ("TimePeriod_21_MINIMUM2", 21),
        ("TimePeriod_14_MINIMUM2", 14),
        ("TimePeriod_10_MINIMUM2", 10),
        ("TimePeriod_5_MINIMUM2", 5),
    ] {
        emit_opt_input_int_const(
            &mut o,
            &format!("TA_DEF_UI_{name}"),
            "optInTimePeriod",
            "Time Period",
            "TA_DEF_TimePeriod_Positive_Minimum2",
            *default,
            "Time period",
        );
    }

    emit_opt_input_int_const(
        &mut o,
        "TA_DEF_UI_TimePeriod_14_MINIMUM5",
        "optInTimePeriod",
        "Time Period",
        "TA_DEF_TimePeriod_Positive_Minimum5",
        14,
        "Time period",
    );

    for (name, default) in &[
        ("TimePeriod_30", 30),
        ("TimePeriod_14", 14),
        ("TimePeriod_10", 10),
        ("TimePeriod_5", 5),
    ] {
        emit_opt_input_int_const(
            &mut o,
            &format!("TA_DEF_UI_{name}"),
            "optInTimePeriod",
            "Time Period",
            "TA_DEF_TimePeriod_Positive",
            *default,
            "Time period",
        );
    }

    // NbDeviation
    o.push_str("const TA_OptInputParameterInfo TA_DEF_UI_NbDeviation =\n{\n");
    o.push_str("   TA_OptInput_RealRange,\n");
    o.push_str("   \"optInNbDev\",\n   0,\n\n");
    o.push_str("   \"Deviations\",\n");
    o.push_str("   (const void *)&TA_DEF_NbDeviation,\n");
    o.push_str("   1.0,\n   \"Nb of deviations\",\n\n   NULL\n};\n\n");

    // Penetration
    emit_opt_input_real_const(
        &mut o, "TA_DEF_UI_Penetration_30", "optInPenetration", "Penetration",
        "TA_DEF_RealPositive", "0.3",
        "Percentage of penetration of a candle within another candle",
    );
    emit_opt_input_real_const(
        &mut o, "TA_DEF_UI_Penetration_50", "optInPenetration", "Penetration",
        "TA_DEF_RealPositive", "0.5",
        "Percentage of penetration of a candle within another candle",
    );

    // Vertical/Horizontal shift
    o.push_str("const TA_OptInputParameterInfo TA_DEF_UI_VerticalShift =\n{\n");
    o.push_str("   TA_OptInput_RealRange,\n   \"optInVertShift\",\n");
    o.push_str("   TA_OPTIN_IS_PERCENT,\n\n");
    o.push_str("   \"Vertical Shift\",\n");
    o.push_str("   (const void *)&TA_DEF_VerticalShiftPercent,\n");
    o.push_str("   0,\n   \"Positive number shift upwards, negative downwards\",\n\n   NULL\n};\n\n");

    emit_opt_input_int_const(
        &mut o, "TA_DEF_UI_HorizontalShift", "optInHorizShift",
        "Horizontal Shift", "TA_DEF_HorizontalShiftPeriod", 0,
        "Positive number shift 'n' period to the right, negative shift to the left",
    );

    // MA Type list
    let ma = enums.get("MAType").expect("MAType enum required");
    o.push_str("static const TA_IntegerDataPair TA_MA_TypeDataPair[] =\n{\n");
    for (i, v) in ma.variants.iter().enumerate() {
        let comma = if i + 1 < ma.variants.len() { "," } else { "" };
        let _ = writeln!(o, "   {{{},\"{}\"}}{comma}", v.value, v.short_name);
    }
    o.push_str("};\n\n");

    o.push_str("const TA_IntegerList TA_MA_TypeList =\n{\n");
    o.push_str("   &TA_MA_TypeDataPair[0],\n");
    o.push_str("   sizeof(TA_MA_TypeDataPair)/sizeof(TA_IntegerDataPair)\n};\n\n");

    // MA Method
    o.push_str("const TA_OptInputParameterInfo TA_DEF_UI_MA_Method =\n{\n");
    o.push_str("   TA_OptInput_IntegerList,\n   \"optInMAType\",\n   0,\n\n");
    o.push_str("   \"MA Type\",\n   (const void *)&TA_MA_TypeList,\n");
    o.push_str("   0,\n   \"Type of Moving Average\",\n\n   NULL\n};\n\n");

    // Fast/Slow period
    emit_opt_input_int_const(
        &mut o, "TA_DEF_UI_Fast_Period", "optInFastPeriod",
        "Fast Period", "TA_DEF_TimePeriod_Positive_Minimum2", 12,
        "Period of the fast MA",
    );
    emit_opt_input_int_const(
        &mut o, "TA_DEF_UI_Slow_Period", "optInSlowPeriod",
        "Slow Period", "TA_DEF_TimePeriod_Positive_Minimum2", 26,
        "Period of the slow MA",
    );

    o
}

fn emit_price_input(o: &mut String, suffix: &str, param_name: &str, flags: &[&str]) {
    let _ = writeln!(
        o,
        "const TA_InputParameterInfo TA_DEF_UI_Input_Price_{suffix} ="
    );
    o.push_str("                                  { TA_Input_Price, \"");
    o.push_str(param_name);
    o.push_str("\",\n                                    ");
    for (i, flag) in flags.iter().enumerate() {
        if i > 0 {
            o.push_str(" |\n                                    ");
        }
        o.push_str(flag);
    }
    o.push_str(" };\n\n");
}

fn emit_int_range(o: &mut String, name: &str, min: i32, max: i32, sstart: i32, send: i32, sinc: i32) {
    let _ = writeln!(o, "const TA_IntegerRange {name} =\n{{");
    let _ = writeln!(o, "   {min},");
    let _ = writeln!(o, "   {max},");
    let _ = writeln!(o, "   {sstart},");
    let _ = writeln!(o, "   {send},");
    let _ = writeln!(o, "   {sinc}");
    o.push_str("};\n\n");
}

fn emit_real_range(
    o: &mut String, name: &str,
    min: &str, max: &str, precision: i32,
    sstart: &str, send: &str, sinc: &str,
) {
    let _ = writeln!(o, "const TA_RealRange {name} =\n{{");
    let _ = writeln!(o, "   {min},");
    let _ = writeln!(o, "   {max},");
    let _ = writeln!(o, "   {precision},");
    let _ = writeln!(o, "   {sstart},");
    let _ = writeln!(o, "   {send},");
    let _ = writeln!(o, "   {sinc}");
    o.push_str("};\n\n");
}

fn emit_opt_input_int_const(
    o: &mut String, c_name: &str, param_name: &str,
    display_name: &str, data_set: &str, default: i32, hint: &str,
) {
    let _ = writeln!(o, "const TA_OptInputParameterInfo {c_name} =\n{{");
    o.push_str("   TA_OptInput_IntegerRange,\n");
    let _ = writeln!(o, "   \"{param_name}\",");
    o.push_str("   0,\n\n");
    let _ = writeln!(o, "   \"{display_name}\",");
    let _ = writeln!(o, "   (const void *)&{data_set},");
    let _ = writeln!(o, "   {default},");
    let _ = writeln!(o, "   \"{hint}\",\n");
    o.push_str("   NULL\n};\n\n");
}

fn emit_opt_input_real_const(
    o: &mut String, c_name: &str, param_name: &str,
    display_name: &str, data_set: &str, default: &str, hint: &str,
) {
    let _ = writeln!(o, "const TA_OptInputParameterInfo {c_name} =\n{{");
    o.push_str("   TA_OptInput_RealRange,\n");
    let _ = writeln!(o, "   \"{param_name}\",");
    o.push_str("   0,\n\n");
    let _ = writeln!(o, "   \"{display_name}\",");
    let _ = writeln!(o, "   (const void *)&{data_set},");
    let _ = writeln!(o, "   {default},");
    let _ = writeln!(o, "   \"{hint}\",\n");
    o.push_str("   NULL\n};\n\n");
}

// ---------------------------------------------------------------------------
// File 3: ta_frame_priv.h
// ---------------------------------------------------------------------------

fn gen_frame_priv_h() -> String {
    let mut o = String::new();
    o.push_str(LICENSE);
    o.push_str("#ifndef TA_FRAME_PRIV_H\n#define TA_FRAME_PRIV_H\n\n");
    o.push_str("#ifndef TA_ABSTRACT_H\n   #include \"ta_abstract.h\"\n#endif\n\n");
    o.push_str("#ifndef TA_MAGIC_NB_H\n   #include \"ta_magic_nb.h\"\n#endif\n\n");

    o.push_str(
        "typedef struct\n\
         {\n\
         \x20  const TA_Real      *open;\n\
         \x20  const TA_Real      *high;\n\
         \x20  const TA_Real      *low;\n\
         \x20  const TA_Real      *close;\n\
         \x20  const TA_Real      *volume;\n\
         \x20  const TA_Real      *openInterest;\n\
         } TA_PricePtrs;\n\n",
    );

    o.push_str(
        "typedef struct\n\
         {\n\
         \x20  union TA_ParamHolderInputData\n\
         \x20  {\n\
         \x20     const TA_Real      *inReal;\n\
         \x20     const TA_Integer   *inInteger;\n\
         \x20     TA_PricePtrs        inPrice;\n\
         \x20  } data;\n\
         \n\
         \x20  const TA_InputParameterInfo *inputInfo;\n\
         \n\
         } TA_ParamHolderInput;\n\n",
    );

    o.push_str(
        "typedef struct\n\
         {\n\
         \x20  union TA_ParamHolderOptInData\n\
         \x20  {\n\
         \x20     TA_Integer optInInteger;\n\
         \x20     TA_Real    optInReal;\n\
         \x20  } data;\n\
         \n\
         \x20  const TA_OptInputParameterInfo *optInputInfo;\n\
         \n\
         } TA_ParamHolderOptInput;\n\n",
    );

    o.push_str(
        "typedef struct\n\
         {\n\
         \x20  union TA_ParamHolderOutputData\n\
         \x20  {\n\
         \x20     TA_Real        *outReal;\n\
         \x20     TA_Integer     *outInteger;\n\
         \x20  } data;\n\
         \n\
         \x20  const TA_OutputParameterInfo *outputInfo;\n\
         } TA_ParamHolderOutput;\n\n",
    );

    o.push_str(
        "typedef struct\n\
         {\n\
         \x20  unsigned int magicNumber;\n\
         \n\
         \x20  TA_ParamHolderInput    *in;\n\
         \x20  TA_ParamHolderOptInput *optIn;\n\
         \x20  TA_ParamHolderOutput   *out;\n\
         \n\
         \x20  unsigned int inBitmap;\n\
         \x20  unsigned int outBitmap;\n\
         \n\
         \x20  const TA_FuncInfo *funcInfo;\n\
         } TA_ParamHolderPriv;\n\n",
    );

    o.push_str(
        "typedef TA_RetCode (*TA_FrameFunction)( const TA_ParamHolderPriv *params,\n\
         \x20                                       TA_Integer  startIdx,\n\
         \x20                                       TA_Integer  endIdx,\n\
         \x20                                       TA_Integer *outBegIdx,\n\
         \x20                                       TA_Integer *outNbElement );\n\n",
    );

    o.push_str(
        "typedef unsigned int (*TA_FrameLookback)( const TA_ParamHolderPriv *params );\n\n",
    );

    o.push_str("#endif\n");
    o
}

// ---------------------------------------------------------------------------
// File 4: frames/ta_frame.h
// ---------------------------------------------------------------------------

fn gen_frame_h(funcs: &[&FuncDef]) -> String {
    let mut o = String::new();
    o.push_str(LICENSE);
    o.push_str("#ifndef TA_FRAME_H\n#define TA_FRAME_H\n\n");
    o.push_str(
        "/* Note: This file is generated by ta_codegen, do not\n\
         \x20*       modify directly.\n */\n\n",
    );
    o.push_str("#ifndef TA_COMMON_H\n   #include \"ta_common.h\"\n#endif\n\n");
    o.push_str("#ifndef TA_FRAME_PRIV_H\n   #include \"ta_frame_priv.h\"\n#endif\n\n\n");

    for func in funcs {
        let name = &func.name;
        let _ = writeln!(
            o,
            "/* Generated */ TA_RetCode TA_{name}_FramePP( const TA_ParamHolderPriv *params,\n\
             /* Generated */                           int            startIdx,\n\
             /* Generated */                           int            endIdx,\n\
             /* Generated */                           int           *outBegIdx,\n\
             /* Generated */                           int           *outNBElement )\n;\n\
             /* Generated */ unsigned int TA_{name}_FramePPLB( const TA_ParamHolderPriv *params )\n;\n"
        );
    }

    o.push_str("\n#endif\n");
    o
}

// ---------------------------------------------------------------------------
// File 5: frames/ta_frame.c
// ---------------------------------------------------------------------------

fn gen_frame_c(funcs: &[&FuncDef]) -> String {
    let mut o = String::new();
    o.push_str(LICENSE);
    o.push_str(
        "/* Note: This file is generated by ta_codegen, do not\n\
         \x20*       modify directly.\n */\n\n",
    );
    o.push_str("#ifndef TA_FUNC_H\n   #include \"ta_func.h\"\n#endif\n\n");
    o.push_str("#ifndef TA_FRAME_PRIV_H\n   #include \"ta_frame_priv.h\"\n#endif\n\n");
    o.push_str("#ifndef TA_FRAME_H\n   #include \"ta_frame.h\"\n#endif\n\n");
    o.push_str("/* NEVER CALL directly these functions! Use TA_CallFunc. */\n\n");

    for func in funcs {
        emit_frame_pp(&mut o, func);
        emit_frame_pp_lb(&mut o, func);
    }

    o
}

/// Emit the `TA_XXX_FramePP` function.
fn emit_frame_pp(o: &mut String, func: &FuncDef) {
    let name = &func.name;
    let _ = writeln!(
        o,
        "/* Generated */ TA_RetCode TA_{name}_FramePP( const TA_ParamHolderPriv *params,\n\
         /* Generated */                           int            startIdx,\n\
         /* Generated */                           int            endIdx,\n\
         /* Generated */                           int           *outBegIdx,\n\
         /* Generated */                           int           *outNBElement )\n\
         /* Generated */ {{"
    );
    let _ = writeln!(o, "/* Generated */    return TA_{name}(");
    let _ = writeln!(o, "/* Generated */                startIdx,");
    let _ = writeln!(o, "/* Generated */                endIdx,");

    // Input params — reconstruct Price groups from expanded Real inputs.
    let abstract_inputs = reconstruct_abstract_inputs(&func.inputs);
    for (abstract_idx, ai) in abstract_inputs.iter().enumerate() {
        match ai {
            AbstractInput::Price(components) => {
                for comp in components {
                    let _ = writeln!(
                        o,
                        "/* Generated */                params->in[{abstract_idx}].data.inPrice.{comp}, \
                         /* in{} */",
                        capitalize_first(comp)
                    );
                }
            }
            AbstractInput::Single(inp) => {
                let accessor = match &inp.param_type {
                    ParamType::Real => "inReal",
                    ParamType::Integer | ParamType::Enum(_) => "inInteger",
                    ParamType::Price(_) => unreachable!(),
                };
                let _ = writeln!(
                    o,
                    "/* Generated */                params->in[{abstract_idx}].data.{accessor}, /* {} */",
                    inp.name
                );
            }
        }
    }

    // Opt input params
    for (i, opt) in func.optional_inputs.iter().enumerate() {
        let accessor = opt_input_accessor(opt, i);
        let _ = writeln!(
            o,
            "/* Generated */                {accessor}, /* {}*/",
            opt.name
        );
    }

    // outBegIdx, outNBElement
    let _ = writeln!(o, "/* Generated */                outBegIdx, ");
    let _ = writeln!(o, "/* Generated */                outNBElement, ");

    // Output params
    for (i, out) in func.outputs.iter().enumerate() {
        let accessor = match &out.param_type {
            ParamType::Integer => format!("params->out[{i}].data.outInteger"),
            _ => format!("params->out[{i}].data.outReal"),
        };
        let is_last = i + 1 == func.outputs.len();
        let comma = if is_last { "" } else { "," };
        let _ = writeln!(
            o,
            "/* Generated */                {accessor}{comma} /*  {} */",
            out.name
        );
    }

    o.push_str("/* Generated */                );\n");
    o.push_str("/* Generated */ }\n");
}

/// Emit the `TA_XXX_FramePPLB` function.
fn emit_frame_pp_lb(o: &mut String, func: &FuncDef) {
    let name = &func.name;
    let _ = writeln!(
        o,
        "/* Generated */ unsigned int TA_{name}_FramePPLB( const TA_ParamHolderPriv *params )\n\
         /* Generated */ {{"
    );

    if func.optional_inputs.is_empty() {
        let _ = writeln!(o, "/* Generated */    (void)params;");
        let _ = writeln!(o, "/* Generated */    return TA_{name}_Lookback( );");
    } else {
        let _ = write!(o, "/* Generated */    return TA_{name}_Lookback(");
        for (i, opt) in func.optional_inputs.iter().enumerate() {
            let accessor = opt_input_accessor(opt, i);
            let is_last = i + 1 == func.optional_inputs.len();
            let comma = if is_last { "" } else { "," };
            let _ = write!(o, "{accessor}{comma} /* {}*/", opt.name);
            if !is_last {
                let _ = write!(o, "\n/* Generated */                     ");
            }
        }
        let _ = writeln!(o, " );");
    }

    o.push_str("/* Generated */ }\n");
}

/// Build the C accessor expression for an optional input parameter.
fn opt_input_accessor(opt: &OptInput, idx: usize) -> String {
    match &opt.param_type {
        ParamType::Real => format!("params->optIn[{idx}].data.optInReal"),
        ParamType::Enum(_) => {
            format!("(TA_MAType)params->optIn[{idx}].data.optInInteger")
        }
        _ => format!("params->optIn[{idx}].data.optInInteger"),
    }
}

// ---------------------------------------------------------------------------
// Price input reconstruction
// ---------------------------------------------------------------------------

/// Known price component names that the YAML parser expands.
const PRICE_COMPONENT_NAMES: &[&str] = &[
    "inOpen",
    "inHigh",
    "inLow",
    "inClose",
    "inVolume",
    "inOpenInterest",
];

/// Represents either a single abstract input or a group of price components.
enum AbstractInput {
    /// A single non-price input (Real, Integer, etc.)
    Single(Input),
    /// A group of price component names, e.g. `["high", "low", "close"]`.
    Price(Vec<String>),
}

/// Reconstruct abstract (ta_abstract-style) inputs from the expanded IR inputs.
///
/// The YAML parser expands `type: price, price_components: [high, low, close]`
/// into three separate `Real` inputs: `inHigh`, `inLow`, `inClose`.
/// For ta_abstract, we need to group these back into a single `Price` input.
fn reconstruct_abstract_inputs(inputs: &[Input]) -> Vec<AbstractInput> {
    let mut result = Vec::new();
    let mut i = 0;
    while i < inputs.len() {
        // Check if current input is a price component.
        if is_price_component(&inputs[i].name) {
            // Consume consecutive price components.
            let mut components = Vec::new();
            while i < inputs.len() && is_price_component(&inputs[i].name) {
                let comp = input_name_to_component(&inputs[i].name);
                components.push(comp);
                i += 1;
            }
            result.push(AbstractInput::Price(components));
        } else {
            result.push(AbstractInput::Single(inputs[i].clone()));
            i += 1;
        }
    }
    result
}

fn is_price_component(name: &str) -> bool {
    PRICE_COMPONENT_NAMES.contains(&name)
}

/// Convert expanded input name back to component: "inHigh" -> "high", "inClose" -> "close".
fn input_name_to_component(name: &str) -> String {
    match name {
        "inOpen" => "open".to_string(),
        "inHigh" => "high".to_string(),
        "inLow" => "low".to_string(),
        "inClose" => "close".to_string(),
        "inVolume" => "volume".to_string(),
        "inOpenInterest" => "openInterest".to_string(),
        _ => name.to_string(),
    }
}

// ---------------------------------------------------------------------------
// File 6: tables/table_X.c
// ---------------------------------------------------------------------------

fn gen_table_file(letter: char, funcs: &[&&FuncDef]) -> String {
    let upper = letter.to_ascii_uppercase();
    let mut o = String::new();
    o.push_str(LICENSE);
    let _ = writeln!(
        o,
        "/*********************************************************************\n\
         \x20* This file contains only TA functions starting with the letter '{upper}' *\n\
         \x20*********************************************************************/\n\
         #include <stddef.h>\n\
         #include \"ta_abstract.h\"\n\
         #include \"ta_def_ui.h\"\n"
    );

    // Emit each function's metadata.
    for func in funcs {
        emit_table_function(&mut o, func);
    }

    // Table array.
    let _ = writeln!(
        o,
        "/****************************************************************************\n\
         \x20* Step 2 - Add your TA function to the table.\n\
         \x20*          Keep in alphabetical order. Must be NULL terminated.\n\
         \x20****************************************************************************/\n\
         const TA_FuncDef *TA_DEF_Table{upper}[] =\n{{"
    );
    for func in funcs {
        let _ = writeln!(o, "   ADD_TO_TABLE({}),", func.name);
    }
    o.push_str("   NULL\n};\n\n\n");

    let _ = writeln!(
        o,
        "/* Do not modify the following line. */\n\
         const unsigned int TA_DEF_Table{upper}Size =\n\
         \x20             ((sizeof(TA_DEF_Table{upper})/sizeof(TA_FuncDef *))-1);\n"
    );

    o
}

/// Emit a single function's metadata block inside a table file.
#[allow(clippy::too_many_lines)]
fn emit_table_function(o: &mut String, func: &FuncDef) {
    let name = &func.name;
    let _ = writeln!(o, "/* {name} BEGIN */");

    // Determine which custom items we need to emit before the arrays.
    // 1. Custom opt-input parameters (those that don't match a pre-defined constant).
    let mut opt_input_refs: Vec<String> = Vec::new();
    for opt in &func.optional_inputs {
        if let Some(predef) = match_predefined_opt_input(opt) {
            opt_input_refs.push(predef);
        } else {
            // Need a custom one — emit range + optinput structs.
            let custom_name = emit_custom_opt_input(o, func, opt);
            opt_input_refs.push(custom_name);
        }
    }

    // 2. Custom output parameters.
    let mut output_refs: Vec<String> = Vec::new();
    for out in &func.outputs {
        if let Some(predef) = match_predefined_output(out) {
            output_refs.push(predef);
        } else {
            let custom_name = emit_custom_output(o, func, out);
            output_refs.push(custom_name);
        }
    }

    // Inputs array — reconstruct Price groups from expanded Real inputs.
    let abstract_inputs = reconstruct_abstract_inputs(&func.inputs);
    let _ = writeln!(
        o,
        "static const TA_InputParameterInfo    *TA_{name}_Inputs[]    =\n{{"
    );
    for ai in &abstract_inputs {
        let ref_name = match ai {
            AbstractInput::Price(components) => {
                let key = price_components_to_suffix(components);
                format!("TA_DEF_UI_Input_Price_{key}")
            }
            AbstractInput::Single(inp) => input_ref_name(inp),
        };
        let _ = writeln!(o, "  &{ref_name},");
    }
    o.push_str("  NULL\n};\n\n");

    // Outputs array.
    let _ = writeln!(
        o,
        "static const TA_OutputParameterInfo   *TA_{name}_Outputs[]   =\n{{"
    );
    for r in &output_refs {
        let _ = writeln!(o, "  &{r},");
    }
    o.push_str("  NULL\n};\n\n");

    // OptInputs array.
    let _ = writeln!(
        o,
        "static const TA_OptInputParameterInfo *TA_{name}_OptInputs[] ="
    );
    if opt_input_refs.is_empty() {
        o.push_str("{ NULL };\n\n");
    } else {
        o.push_str("{ ");
        for (i, r) in opt_input_refs.iter().enumerate() {
            if i > 0 {
                o.push_str("  ");
            }
            let _ = writeln!(o, "&{r},");
        }
        o.push_str("  NULL\n};\n\n");
    }

    // DEF_FUNCTION macro invocation.
    let group_id = group_id_string(&func.group);
    let hint = func.hint.as_deref().unwrap_or("");
    let camel = func.camel_case.as_deref().unwrap_or(name);
    let flags_str = func_flags_string(&func.flags);

    let _ = writeln!(
        o,
        "DEF_FUNCTION( {name},\n\
         \x20             {group_id},\n\
         \x20             \"{hint}\",\n\
         \x20             \"{camel}\",\n\
         \x20             {flags_str}\n\
         \x20            );"
    );
    let _ = writeln!(o, "/* {name} END */\n");
}

/// Return the pre-defined C extern reference for an input, e.g. `TA_DEF_UI_Input_Real`.
fn input_ref_name(inp: &crate::ir::Input) -> String {
    match &inp.param_type {
        ParamType::Price(_) => unreachable!("price inputs are expanded by the parser"),
        ParamType::Real => {
            // Distinguish inReal, inReal0, inReal1, inPeriods
            match inp.name.as_str() {
                "inReal0" => "TA_DEF_UI_Input_Real0".to_string(),
                "inReal1" => "TA_DEF_UI_Input_Real1".to_string(),
                "inPeriods" => "TA_DEF_UI_Input_Periods".to_string(),
                _ => "TA_DEF_UI_Input_Real".to_string(),
            }
        }
        ParamType::Integer | ParamType::Enum(_) => "TA_DEF_UI_Input_Integer".to_string(),
    }
}

/// Map price component list to the suffix like "OHLCV", "HLC", etc.
fn price_components_to_suffix(components: &[String]) -> String {
    let normalized: Vec<&str> = components.iter().map(String::as_str).collect();
    match normalized.as_slice() {
        ["open", "high", "low", "close", "volume"] => "OHLCV".to_string(),
        ["high", "low", "close", "volume"] => "HLCV".to_string(),
        ["open", "high", "low", "close"] => "OHLC".to_string(),
        ["high", "low", "close"] => "HLC".to_string(),
        ["high", "low"] => "HL".to_string(),
        ["open", "close"] => "OC".to_string(),
        ["close", "volume"] => "CV".to_string(),
        ["volume"] => "V".to_string(),
        _ => {
            // Fallback: uppercase first letter of each component.
            components
                .iter()
                .map(|c| c.chars().next().unwrap_or('x').to_ascii_uppercase())
                .collect()
        }
    }
}

/// Check if an output matches a pre-defined constant.
/// Returns the C variable name if so.
fn match_predefined_output(out: &Output) -> Option<String> {
    match &out.param_type {
        ParamType::Real => {
            if out.name == "outReal"
                && out.flags.len() == 1
                && out.flags[0] == "line"
            {
                return Some("TA_DEF_UI_Output_Real".to_string());
            }
        }
        ParamType::Integer => {
            if out.name == "outInteger"
                && out.flags.len() == 1
                && out.flags[0] == "line"
            {
                return Some("TA_DEF_UI_Output_Integer".to_string());
            }
        }
        ParamType::Price(_) | ParamType::Enum(_) => {}
    }
    None
}

/// Emit a custom `TA_OutputParameterInfo` and return its C name.
fn emit_custom_output(o: &mut String, func: &FuncDef, out: &Output) -> String {
    let out_type = match &out.param_type {
        ParamType::Integer => "TA_Output_Integer",
        _ => "TA_Output_Real",
    };
    let flags = output_flags_string(&out.flags);
    // Choose a stable C name: TA_DEF_UI_Output_Real_FUNCNAME_OutName
    // To match legacy: TA_DEF_UI_Output_Real_BBANDS_Upper etc.
    let suffix = output_short_suffix(&out.name);
    let type_word = match &out.param_type {
        ParamType::Integer => "Integer",
        _ => "Real",
    };
    let c_name = format!(
        "TA_DEF_UI_Output_{type_word}_{}_{suffix}",
        func.name
    );

    let _ = writeln!(
        o,
        "const TA_OutputParameterInfo {c_name} =\n\
         \x20                              {{ {out_type}, \"{}\", {flags} }};\n",
        out.name
    );
    c_name
}

/// Derive a short suffix from an output name like "outRealUpperBand" -> "Upper".
fn output_short_suffix(name: &str) -> String {
    // Strip "outReal" or "outInteger" prefix, then use what remains.
    let stripped = name
        .strip_prefix("outReal")
        .or_else(|| name.strip_prefix("outInteger"))
        .unwrap_or(name);
    if stripped.is_empty() {
        "Default".to_string()
    } else {
        stripped.to_string()
    }
}

/// Build the C flags expression for output (e.g. "TA_OUT_LINE | TA_OUT_UPPER_LIMIT").
fn output_flags_string(flags: &[String]) -> String {
    if flags.is_empty() {
        return "0".to_string();
    }
    let mapped: Vec<&str> = flags
        .iter()
        .filter_map(|f| output_flag_to_c(f))
        .collect();
    if mapped.is_empty() {
        "0".to_string()
    } else {
        mapped.join(" | ")
    }
}

pub(crate) fn output_flag_to_c(flag: &str) -> Option<&'static str> {
    match flag {
        "line" => Some("TA_OUT_LINE"),
        "dot_line" => Some("TA_OUT_DOT_LINE"),
        "dash_line" => Some("TA_OUT_DASH_LINE"),
        "dot" => Some("TA_OUT_DOT"),
        "histogram" => Some("TA_OUT_HISTO"),
        "pattern_bool" => Some("TA_OUT_PATTERN_BOOL"),
        "pattern_bull_bear" => Some("TA_OUT_PATTERN_BULL_BEAR"),
        "pattern_strength" => Some("TA_OUT_PATTERN_STRENGTH"),
        "positive" => Some("TA_OUT_POSITIVE"),
        "negative" => Some("TA_OUT_NEGATIVE"),
        "zero" => Some("TA_OUT_ZERO"),
        "upper_limit" => Some("TA_OUT_UPPER_LIMIT"),
        "lower_limit" => Some("TA_OUT_LOWER_LIMIT"),
        "nullable" => Some("TA_OUT_NULLABLE"),
        _ => None,
    }
}

// ---------------------------------------------------------------------------
// Pre-defined opt-input matching
// ---------------------------------------------------------------------------

/// Match an `OptInput` against the pre-defined constants.
/// Returns the C extern name (e.g. `"TA_DEF_UI_TimePeriod_30"`) if matched.
#[allow(clippy::float_cmp, clippy::cast_possible_truncation)]
fn match_predefined_opt_input(opt: &OptInput) -> Option<String> {
    let (range_min, range_max) = opt.range.unwrap_or((0.0, 0.0));
    let default = opt.default.unwrap_or(0.0);
    let min_i = range_min as i32;
    let max_i = range_max as i32;
    let def_i = default as i32;

    match &opt.param_type {
        ParamType::Integer => {
            if opt.name == "optInTimePeriod" && max_i == 100_000 {
                // Min=1 variants
                if min_i == 1 {
                    return match def_i {
                        30 => Some("TA_DEF_UI_TimePeriod_30".into()),
                        14 => Some("TA_DEF_UI_TimePeriod_14".into()),
                        10 => Some("TA_DEF_UI_TimePeriod_10".into()),
                        5 => Some("TA_DEF_UI_TimePeriod_5".into()),
                        _ => None,
                    };
                }
                // Min=2 variants
                if min_i == 2 {
                    return match def_i {
                        30 => Some("TA_DEF_UI_TimePeriod_30_MINIMUM2".into()),
                        20 => Some("TA_DEF_UI_TimePeriod_20_MINIMUM2".into()),
                        21 => Some("TA_DEF_UI_TimePeriod_21_MINIMUM2".into()),
                        14 => Some("TA_DEF_UI_TimePeriod_14_MINIMUM2".into()),
                        10 => Some("TA_DEF_UI_TimePeriod_10_MINIMUM2".into()),
                        5 => Some("TA_DEF_UI_TimePeriod_5_MINIMUM2".into()),
                        _ => None,
                    };
                }
                // Min=5 variants
                if min_i == 5 && def_i == 14 {
                    return Some("TA_DEF_UI_TimePeriod_14_MINIMUM5".into());
                }
            }
            if opt.name == "optInFastPeriod" && min_i == 2 && max_i == 100_000 && def_i == 12 {
                return Some("TA_DEF_UI_Fast_Period".into());
            }
            if opt.name == "optInSlowPeriod" && min_i == 2 && max_i == 100_000 && def_i == 26 {
                return Some("TA_DEF_UI_Slow_Period".into());
            }
            if opt.name == "optInHorizShift" && def_i == 0 {
                return Some("TA_DEF_UI_HorizontalShift".into());
            }
            if opt.name == "optInMinPeriod" && min_i == 2 && max_i == 100_000 && def_i == 2 {
                return Some("TA_DEF_UI_MinPeriod".into());
            }
            if opt.name == "optInMaxPeriod" && min_i == 2 && max_i == 100_000 && def_i == 30 {
                return Some("TA_DEF_UI_MaxPeriod".into());
            }
        }
        ParamType::Enum(_) => {
            if opt.name == "optInMAType" && def_i == 0 {
                return Some("TA_DEF_UI_MA_Method".into());
            }
        }
        ParamType::Real => {
            let min_is_ta_real_min = is_ta_real_min(range_min);
            let max_is_ta_real_max = is_ta_real_max(range_max);

            if opt.name == "optInNbDev"
                && min_is_ta_real_min
                && max_is_ta_real_max
                && (default - 1.0).abs() < 1e-9
            {
                return Some("TA_DEF_UI_NbDeviation".into());
            }
            if opt.name == "optInPenetration" && range_min == 0.0 && max_is_ta_real_max {
                if (default - 0.3).abs() < 1e-9 {
                    return Some("TA_DEF_UI_Penetration_30".into());
                }
                if (default - 0.5).abs() < 1e-9 {
                    return Some("TA_DEF_UI_Penetration_50".into());
                }
            }
            if opt.name == "optInVertShift" && (default).abs() < 1e-9 {
                return Some("TA_DEF_UI_VerticalShift".into());
            }
        }
        ParamType::Price(_) => {}
    }
    None
}

/// Emit a custom range + `TA_OptInputParameterInfo` and return the C name.
#[allow(clippy::cast_possible_truncation)]
fn emit_custom_opt_input(o: &mut String, func: &FuncDef, opt: &OptInput) -> String {
    match &opt.param_type {
        ParamType::Real => emit_custom_real_opt_input(o, func, opt),
        ParamType::Integer => emit_custom_int_opt_input(o, func, opt),
        ParamType::Enum(_) => emit_custom_enum_opt_input(o, func, opt),
        ParamType::Price(_) => String::new(),
    }
}

#[allow(clippy::cast_possible_truncation)]
fn emit_custom_real_opt_input(o: &mut String, func: &FuncDef, opt: &OptInput) -> String {
    let (range_min, range_max) = opt.range.unwrap_or((0.0, f64::MAX));
    let (sstart, send, sinc) = opt.suggested.unwrap_or((0.0, 0.0, 0.0));
    let precision = get_precision(opt);

    // Emit range struct.
    let range_c_name = custom_range_name(func, opt);
    let _ = writeln!(o, "static const TA_RealRange {range_c_name} =\n{{");
    let _ = writeln!(o, "   {},", format_real_sentinel(range_min));
    let _ = writeln!(o, "   {},", format_real_sentinel(range_max));
    let _ = writeln!(o, "   {precision},");
    let _ = writeln!(o, "   {},", format_real_value(sstart));
    let _ = writeln!(o, "   {},", format_real_value(send));
    let _ = writeln!(o, "   {}", format_real_value(sinc));
    o.push_str("};\n\n");

    // Emit opt-input struct.
    let oi_name = custom_opt_input_name(func, opt);
    let display = opt.display_name.as_deref().unwrap_or(&opt.name);
    let hint = opt.hint.as_deref().unwrap_or("");
    let default = opt.default.unwrap_or(0.0);

    let _ = writeln!(o, "static const TA_OptInputParameterInfo {oi_name} =\n{{");
    o.push_str("   TA_OptInput_RealRange,\n");
    let _ = writeln!(o, "   \"{}\",", opt.name);
    let _ = writeln!(o, "   {},\n", opt_input_flags_c(&opt.flags));
    let _ = writeln!(o, "   \"{display}\",");
    let _ = writeln!(o, "   (const void *)&{range_c_name},");
    let _ = writeln!(o, "   {},", format_real_value(default));
    let _ = writeln!(o, "   \"{hint}\",\n");
    o.push_str("   NULL\n};\n\n");

    oi_name
}

#[allow(clippy::cast_possible_truncation)]
fn emit_custom_int_opt_input(o: &mut String, func: &FuncDef, opt: &OptInput) -> String {
    let (range_min, range_max) = opt.range.unwrap_or((1.0, 100_000.0));
    let min_i = range_min as i32;
    let max_i = range_max as i32;

    // Check if there is a matching predefined integer range.
    let range_ref = match (min_i, max_i) {
        (1, 100_000) => Some("TA_DEF_TimePeriod_Positive"),
        (2, 100_000) => Some("TA_DEF_TimePeriod_Positive_Minimum2"),
        (5, 100_000) => Some("TA_DEF_TimePeriod_Positive_Minimum5"),
        _ => None,
    };

    let range_c_name;
    if let Some(existing) = range_ref {
        range_c_name = existing.to_string();
    } else {
        // Emit a custom integer range.
        range_c_name = format!("TA_DEF_{}_Range", sanitize_param_name(&opt.name));
        let (sstart, send, sinc) = opt
            .suggested
            .map_or((min_i, max_i.min(200), 1), |(a, b, c)| (a as i32, b as i32, c as i32));
        let _ = writeln!(o, "static const TA_IntegerRange {range_c_name} =\n{{");
        let _ = writeln!(o, "   {min_i},");
        let _ = writeln!(o, "   {max_i},");
        let _ = writeln!(o, "   {sstart},");
        let _ = writeln!(o, "   {send},");
        let _ = writeln!(o, "   {sinc}");
        o.push_str("};\n\n");
    }

    let oi_name = custom_opt_input_name(func, opt);
    let display = opt.display_name.as_deref().unwrap_or(&opt.name);
    let hint = opt.hint.as_deref().unwrap_or("");
    let default = opt.default.unwrap_or(0.0) as i32;

    let _ = writeln!(o, "static const TA_OptInputParameterInfo {oi_name} =\n{{");
    o.push_str("   TA_OptInput_IntegerRange,\n");
    let _ = writeln!(o, "   \"{}\",", opt.name);
    let _ = writeln!(o, "   {},\n", opt_input_flags_c(&opt.flags));
    let _ = writeln!(o, "   \"{display}\",");
    let _ = writeln!(o, "   (const void *)&{range_c_name},");
    let _ = writeln!(o, "   {default},");
    let _ = writeln!(o, "   \"{hint}\",\n");
    o.push_str("   NULL\n};\n\n");

    oi_name
}

fn emit_custom_enum_opt_input(o: &mut String, func: &FuncDef, opt: &OptInput) -> String {
    let oi_name = custom_opt_input_name(func, opt);
    let display = opt.display_name.as_deref().unwrap_or(&opt.name);
    let hint = opt.hint.as_deref().unwrap_or("");
    #[allow(clippy::cast_possible_truncation)]
    let default = opt.default.unwrap_or(0.0) as i32;

    let _ = writeln!(o, "const TA_OptInputParameterInfo {oi_name} =\n{{");
    o.push_str("   TA_OptInput_IntegerList,\n");
    let _ = writeln!(o, "   \"{}\",", opt.name);
    let _ = writeln!(o, "   {},\n", opt_input_flags_c(&opt.flags));
    let _ = writeln!(o, "   \"{display}\",");
    o.push_str("   (const void *)&TA_MA_TypeList,\n");
    let _ = writeln!(o, "   {default},");
    let _ = writeln!(o, "   \"{hint}\",\n");
    o.push_str("   NULL\n};\n\n");

    oi_name
}

fn custom_range_name(func: &FuncDef, opt: &OptInput) -> String {
    let param = sanitize_param_name(&opt.name);
    format!("TA_DEF_{}_{param}", func.name)
}

fn custom_opt_input_name(func: &FuncDef, opt: &OptInput) -> String {
    let param = sanitize_param_name(&opt.name);
    format!("TA_DEF_UI_D_{}_{param}", func.name)
}

/// Remove the "optIn" prefix from a param name for struct naming.
fn sanitize_param_name(name: &str) -> String {
    name.strip_prefix("optIn").unwrap_or(name).to_string()
}


/// Map a YAML opt-input flag to its C constant (one entry per
/// `TA_OPTIN_*` in include/ta_abstract.h — kept in sync by flag_sync tests).
pub(crate) fn opt_flag_to_c(flag: &str) -> Option<&'static str> {
    match flag {
        "percent" => Some("TA_OPTIN_IS_PERCENT"),
        "degree" => Some("TA_OPTIN_IS_DEGREE"),
        "currency" => Some("TA_OPTIN_IS_CURRENCY"),
        "advanced" => Some("TA_OPTIN_ADVANCED"),
        _ => None,
    }
}
/// Build the C flags constant for opt-input flags.
fn opt_input_flags_c(flags: &[String]) -> String {
    let mapped: Vec<&str> = flags.iter().filter_map(|f| opt_flag_to_c(f)).collect();
    if mapped.is_empty() {
        "0".to_string()
    } else {
        mapped.join(" | ")
    }
}


/// Map a YAML function flag to its C constant (one entry per
/// `TA_FUNC_FLG_*` in include/ta_abstract.h — kept in sync by flag_sync tests).
pub(crate) fn func_flag_to_c(flag: &str) -> Option<&'static str> {
    match flag {
        "overlap" => Some("TA_FUNC_FLG_OVERLAP"),
        "stream" => Some("TA_FUNC_FLG_STREAM"),
        "volume" => Some("TA_FUNC_FLG_VOLUME"),
        "unstable_period" => Some("TA_FUNC_FLG_UNST_PER"),
        "candlestick" => Some("TA_FUNC_FLG_CANDLESTICK"),
        "start_dependent" => Some("TA_FUNC_FLG_START_DEP"),
        _ => None,
    }
}
/// Build the C flags string for function flags. `stream` marks functions
/// with a generated streaming API (TA_FUNC_FLG_STREAM) so wrappers can
/// discover the stream surface through ta_abstract.
fn func_flags_string(flags: &[String]) -> String {
    let mapped: Vec<&str> = flags
        .iter()
        .filter_map(|f| func_flag_to_c(f))
        .collect();
    if mapped.is_empty() {
        "0".to_string()
    } else {
        mapped.join(" | ")
    }
}

/// Map a group display name to the `TA_GroupId_XXX` C identifier.
fn group_id_string(group: &str) -> String {
    for (display, id_suffix) in GROUPS {
        if *display == group {
            return format!("TA_GroupId_{id_suffix}");
        }
    }
    // Fallback.
    format!(
        "TA_GroupId_{}",
        group.replace([' ', '/'], "")
    )
}

fn group_index(group: &str) -> Option<usize> {
    GROUPS.iter().position(|(display, _)| *display == group)
}

// ---------------------------------------------------------------------------
// Sentinel / formatting helpers
// ---------------------------------------------------------------------------

#[allow(clippy::float_cmp)]
fn is_ta_real_min(v: f64) -> bool {
    v == f64::MIN || v <= -3e37 + 1e30
}

#[allow(clippy::float_cmp)]
fn is_ta_real_max(v: f64) -> bool {
    v == f64::MAX || v >= 3e37 - 1e30
}

fn format_real_sentinel(v: f64) -> String {
    if is_ta_real_min(v) {
        "TA_REAL_MIN".to_string()
    } else if is_ta_real_max(v) {
        "TA_REAL_MAX".to_string()
    } else {
        format_real_value(v)
    }
}

fn format_real_value(v: f64) -> String {
    // Try integer-like representation first.
    #[allow(clippy::cast_possible_truncation)]
    if v.fract() == 0.0 && v.abs() < 1e15 {
        let i = v as i64;
        // Use ".0" suffix if the integer form is short enough.
        if v == 0.0 {
            return "0.0".to_string();
        }
        return format!("{i}.0");
    }
    // Use enough precision.
    let s = format!("{v}");
    if s.contains('.') {
        s
    } else {
        format!("{s}.0")
    }
}

/// Get precision for a real optional input.
/// Uses the explicit `precision` field from YAML if present,
/// otherwise falls back to deriving from `suggested_increment`.
#[allow(clippy::cast_possible_truncation, clippy::cast_possible_wrap)]
fn get_precision(opt: &OptInput) -> i32 {
    if let Some(p) = opt.precision {
        return p;
    }
    // Fallback: derive from suggested_increment
    if let Some((_, _, increment)) = opt.suggested {
        derive_precision_from_increment(increment)
    } else {
        2 // default
    }
}

/// Derive precision from `suggested_increment`.
/// Count decimal places of the increment.
#[allow(clippy::cast_possible_truncation, clippy::cast_possible_wrap)]
fn derive_precision_from_increment(increment: f64) -> i32 {
    if increment == 0.0 {
        return 0;
    }
    let s = format!("{increment}");
    if s.contains('.') {
        let trimmed = s.trim_end_matches('0');
        if let Some(d) = trimmed.find('.') {
            (trimmed.len() - d - 1) as i32
        } else {
            0
        }
    } else {
        0
    }
}

// ---------------------------------------------------------------------------
// File 7: ta_group_idx.c
// ---------------------------------------------------------------------------

fn gen_group_idx(sorted: &[&FuncDef], groups: &[Vec<&FuncDef>]) -> String {
    let mut o = String::new();
    o.push_str(LICENSE);
    o.push_str(
        "/* Important: This file is automatically generated by ta_codegen.\n\
         \x20*            Any modification will be lost on next execution\n\
         \x20*            of ta_codegen.\n */\n\n",
    );
    o.push_str("#include <stddef.h>\n#include \"ta_def_ui.h\"\n#include \"ta_abstract.h\"\n\n");

    // Extern declarations for all function defs.
    for func in sorted {
        let _ = writeln!(o, "extern const TA_FuncDef TA_DEF_{};", func.name);
    }
    o.push('\n');

    // Per-group arrays.
    for (i, grp_funcs) in groups.iter().enumerate() {
        let _ = writeln!(o, "const TA_FuncDef *TA_PerGroupFunc_{i}[] = {{");
        for func in grp_funcs {
            let _ = writeln!(o, "&TA_DEF_{},", func.name);
        }
        o.push_str("NULL };\n");
        let _ = writeln!(
            o,
            "#define SIZE_GROUP_{i} ((sizeof(TA_PerGroupFunc_{i})/sizeof(const TA_FuncDef *))-1)\n"
        );
    }

    // TA_PerGroupFuncDef
    let nb_groups = groups.len();
    let _ = writeln!(
        o,
        "/* Generated */ const TA_FuncDef **TA_PerGroupFuncDef[{nb_groups}] = {{"
    );
    for (i, _) in groups.iter().enumerate() {
        let comma = if i + 1 < nb_groups { "," } else { "" };
        let _ = writeln!(o, "&TA_PerGroupFunc_{i}[0]{comma}");
    }
    o.push_str("/* Generated */ };\n\n");

    // TA_PerGroupSize
    let _ = writeln!(
        o,
        "/* Generated */ const unsigned int TA_PerGroupSize[{nb_groups}] = {{"
    );
    for (i, _) in groups.iter().enumerate() {
        let comma = if i + 1 < nb_groups { "," } else { "" };
        let _ = writeln!(o, "SIZE_GROUP_{i}{comma}");
    }
    o.push_str("/* Generated */ };\n\n");

    // TA_TotalNbFunction
    o.push_str("/* Generated */ const unsigned int TA_TotalNbFunction =\n");
    for (i, _) in groups.iter().enumerate() {
        let sep = if i + 1 < nb_groups { "+" } else { ";" };
        let _ = writeln!(o, "SIZE_GROUP_{i}{sep}");
    }
    o.push('\n');

    o
}

// ---------------------------------------------------------------------------
// File 8: ta_abstract.c
// ---------------------------------------------------------------------------

#[allow(clippy::too_many_lines)]
fn gen_ta_abstract_c() -> String {
    let mut o = String::new();
    o.push_str(LICENSE);
    o.push_str(
        "/* Description:\n\
         \x20*   Provide a way to abstract the call of the TA functions.\n\
         \x20*/\n\n",
    );
    o.push_str(
        "#include <stddef.h>\n\
         #include <string.h>\n\
         #include <ctype.h>\n\
         #include \"ta_common.h\"\n\
         #include \"ta_memory.h\"\n\
         #include \"ta_abstract.h\"\n\
         #include \"ta_def_ui.h\"\n\
         #include \"ta_frame_priv.h\"\n\n\
         #include <limits.h>\n\n",
    );

    // External table declarations — use array syntax so it compiles
    // in both single-TU and multi-TU builds.
    for letter in b'A'..=b'Z' {
        let ch = letter as char;
        let _ = writeln!(o, "extern const TA_FuncDef *TA_DEF_Table{ch}[];");
    }
    o.push('\n');

    o.push_str("extern const unsigned int TA_DEF_TableASize, TA_DEF_TableBSize,\n");
    o.push_str("                          TA_DEF_TableCSize, TA_DEF_TableDSize,\n");
    o.push_str("                          TA_DEF_TableESize, TA_DEF_TableFSize,\n");
    o.push_str("                          TA_DEF_TableGSize, TA_DEF_TableHSize,\n");
    o.push_str("                          TA_DEF_TableISize, TA_DEF_TableJSize,\n");
    o.push_str("                          TA_DEF_TableKSize, TA_DEF_TableLSize,\n");
    o.push_str("                          TA_DEF_TableMSize, TA_DEF_TableNSize,\n");
    o.push_str("                          TA_DEF_TableOSize, TA_DEF_TablePSize,\n");
    o.push_str("                          TA_DEF_TableQSize, TA_DEF_TableRSize,\n");
    o.push_str("                          TA_DEF_TableSSize, TA_DEF_TableTSize,\n");
    o.push_str("                          TA_DEF_TableUSize, TA_DEF_TableVSize,\n");
    o.push_str("                          TA_DEF_TableWSize, TA_DEF_TableXSize,\n");
    o.push_str("                          TA_DEF_TableYSize, TA_DEF_TableZSize;\n\n");

    // Declared unconditionally: the generated ta_group_idx.c always defines these,
    // so the decls must be visible to every consumer — otherwise getGroupSize/
    // getFuncNameByIdx would reference undeclared symbols.
    o.push_str(
        "extern const TA_FuncDef **TA_PerGroupFuncDef[];\n\
         extern const unsigned int TA_PerGroupSize[];\n\n",
    );

    // Local declarations.
    o.push_str(
        "#ifndef min\n\
         \x20  #define min(a, b)  (((a) < (b)) ? (a) : (b))\n\
         #endif\n\n\
         typedef struct\n{\n   unsigned int magicNumber;\n} TA_StringTablePriv;\n\n",
    );

    // Local function forward declarations.
    o.push_str(
        "static TA_RetCode getGroupId( const char *groupString, unsigned int *groupId );\n\
         static TA_RetCode getGroupSize( TA_GroupId groupId, unsigned int *groupSize );\n\
         static TA_RetCode getFuncNameByIdx( TA_GroupId groupId,\n\
         \x20                                    unsigned int idx,\n\
         \x20                                    const char **stringPtr );\n\n",
    );

    // Static table arrays — use array name directly (decays to const TA_FuncDef **).
    o.push_str("static const TA_FuncDef **TA_DEF_Tables[26] =\n{\n");
    for (i, ch) in (b'A'..=b'Z').enumerate() {
        let letter = ch as char;
        let comma = if i < 25 { "," } else { "" };
        let _ = write!(o, "   (const TA_FuncDef **)TA_DEF_Table{letter}{comma}");
        if (i + 1) % 5 == 0 {
            o.push('\n');
        } else {
            o.push(' ');
        }
    }
    o.push_str("};\n\n");

    o.push_str("static const unsigned int *TA_DEF_TablesSize[26] =\n{\n");
    for (i, ch) in (b'A'..=b'Z').enumerate() {
        let letter = ch as char;
        let comma = if i < 25 { "," } else { "" };
        let _ = write!(o, "   &TA_DEF_Table{letter}Size{comma}");
        if (i + 1) % 3 == 0 {
            o.push('\n');
        } else {
            o.push(' ');
        }
    }
    o.push_str("};\n\n");

    // --- TA_GroupTableAlloc ---
    o.push_str(
        "TA_RetCode TA_GroupTableAlloc( TA_StringTable **table )\n\
         {\n\
         \x20  TA_StringTable *stringTable;\n\
         \x20  TA_StringTablePriv *stringTablePriv;\n\n\
         \x20  if( table == NULL )\n\
         \x20  {\n\
         \x20     return TA_BAD_PARAM;\n\
         \x20  }\n\n\
         \x20  stringTable = (TA_StringTable *)TA_Malloc( sizeof(TA_StringTable) + sizeof(TA_StringTablePriv) );\n\
         \x20  if( !stringTable )\n\
         \x20  {\n\
         \x20     *table = NULL;\n\
         \x20     return TA_ALLOC_ERR;\n\
         \x20  }\n\n\
         \x20  memset( stringTable, 0, sizeof(TA_StringTable) + sizeof(TA_StringTablePriv) );\n\
         \x20  stringTablePriv = (TA_StringTablePriv *)(((char *)stringTable)+sizeof(TA_StringTable));\n\
         \x20  stringTablePriv->magicNumber = TA_STRING_TABLE_GROUP_MAGIC_NB;\n\n\
         \x20  stringTable->size = TA_NB_GROUP_ID;\n\
         \x20  stringTable->string = &TA_GroupString[0];\n\
         \x20  stringTable->hiddenData = stringTablePriv;\n\n\
         \x20  *table = stringTable;\n\n\
         \x20  return TA_SUCCESS;\n\
         }\n\n",
    );

    // --- TA_GroupTableFree ---
    o.push_str(
        "TA_RetCode TA_GroupTableFree( TA_StringTable *table )\n\
         {\n\
         \x20  TA_StringTablePriv *stringTablePriv;\n\n\
         \x20  if( table )\n\
         \x20  {\n\
         \x20     stringTablePriv = (TA_StringTablePriv *)table->hiddenData;\n\
         \x20     if( !stringTablePriv )\n\
         \x20     {\n\
         \x20        return TA_INTERNAL_ERROR(1);\n\
         \x20     }\n\n\
         \x20     if( stringTablePriv->magicNumber != TA_STRING_TABLE_GROUP_MAGIC_NB )\n\
         \x20     {\n\
         \x20        return TA_BAD_OBJECT;\n\
         \x20     }\n\n\
         \x20     TA_Free( table );\n\
         \x20  }\n\n\
         \x20  return TA_SUCCESS;\n\
         }\n\n",
    );

    // --- TA_ForEachFunc ---
    o.push_str(
        "TA_RetCode TA_ForEachFunc( TA_CallForEachFunc functionToCall, void *opaqueData )\n\
         {\n\
         \x20  const TA_FuncDef **funcDefTable;\n\
         \x20  const TA_FuncDef *funcDef;\n\
         \x20  const TA_FuncInfo *funcInfo;\n\
         \x20  unsigned int i, j, funcDefTableSize;\n\n\
         \x20  if( functionToCall == NULL )\n\
         \x20  {\n\
         \x20     return TA_BAD_PARAM;\n\
         \x20  }\n\n\
         \x20  for( i=0; i < 26; i++ )\n\
         \x20  {\n\
         \x20     funcDefTable = TA_DEF_Tables[i];\n\
         \x20     funcDefTableSize = *TA_DEF_TablesSize[i];\n\
         \x20     for( j=0; j < funcDefTableSize; j++ )\n\
         \x20     {\n\
         \x20        funcDef = funcDefTable[j];\n\
         \x20        if( !funcDef || !funcDef->funcInfo )\n\
         \x20           return TA_INTERNAL_ERROR(3);\n\n\
         \x20        funcInfo = funcDef->funcInfo;\n\
         \x20        if( !funcInfo )\n\
         \x20           return TA_INTERNAL_ERROR(4);\n\
         \x20        (*functionToCall)( funcInfo, opaqueData );\n\
         \x20     }\n\
         \x20  }\n\n\
         \x20  return TA_SUCCESS;\n\
         }\n\n",
    );

    // --- TA_FuncTableAlloc ---
    o.push_str(
        "TA_RetCode TA_FuncTableAlloc( const char *group, TA_StringTable **table )\n\
         {\n\
         \x20  TA_RetCode retCode;\n\
         \x20  unsigned int i;\n\
         \x20  TA_StringTable *stringTable;\n\
         \x20  unsigned int groupId;\n\
         \x20  unsigned int groupSize;\n\
         \x20  const char *stringPtr;\n\
         \x20  TA_StringTablePriv *stringTablePriv;\n\n\
         \x20  if( (group == NULL) || (table == NULL ) )\n\
         \x20  {\n\
         \x20     return TA_BAD_PARAM;\n\
         \x20  }\n\n\
         \x20  *table = NULL;\n\
         \x20  stringPtr = NULL;\n\n\
         \x20  retCode = getGroupId( group, &groupId );\n\
         \x20  if( retCode != TA_SUCCESS )\n\
         \x20  {\n\
         \x20     return retCode;\n\
         \x20  }\n\n\
         \x20  retCode = getGroupSize( (TA_GroupId)groupId, &groupSize );\n\
         \x20  if( retCode != TA_SUCCESS )\n\
         \x20  {\n\
         \x20     return retCode;\n\
         \x20  }\n\n\
         \x20  stringTable = (TA_StringTable *)TA_Malloc( sizeof(TA_StringTable) + sizeof(TA_StringTablePriv) );\n\
         \x20  if( !stringTable )\n\
         \x20  {\n\
         \x20     *table = NULL;\n\
         \x20     return TA_ALLOC_ERR;\n\
         \x20  }\n\n\
         \x20  memset( stringTable, 0, sizeof(TA_StringTable) + sizeof(TA_StringTablePriv) );\n\
         \x20  stringTablePriv = (TA_StringTablePriv *)(((char *)stringTable)+sizeof(TA_StringTable));\n\
         \x20  stringTablePriv->magicNumber = TA_STRING_TABLE_FUNC_MAGIC_NB;\n\
         \x20  stringTable->hiddenData = stringTablePriv;\n\n\
         \x20  stringTable->size = groupSize;\n\
         \x20  if( groupSize != 0 )\n\
         \x20  {\n\
         \x20     stringTable->string = (const char **)TA_Malloc( (stringTable->size) *\n\
         \x20                                                     sizeof(const char *) );\n\n\
         \x20     if( stringTable->string == NULL )\n\
         \x20     {\n\
         \x20        *table = NULL;\n\
         \x20        TA_FuncTableFree( stringTable );\n\
         \x20        return TA_ALLOC_ERR;\n\
         \x20     }\n\n\
         \x20     memset( (void *)stringTable->string, 0,\n\
         \x20             (stringTable->size) * sizeof(const char *) );\n\n\
         \x20     for( i=0; i < stringTable->size; i++ )\n\
         \x20     {\n\
         \x20        retCode = getFuncNameByIdx( (TA_GroupId)groupId, i, &stringPtr );\n\n\
         \x20        if( retCode != TA_SUCCESS )\n\
         \x20        {\n\
         \x20           *table = NULL;\n\
         \x20           TA_FuncTableFree( stringTable );\n\
         \x20           return TA_ALLOC_ERR;\n\
         \x20        }\n\n\
         \x20        (stringTable->string)[i] = stringPtr;\n\
         \x20     }\n\
         \x20  }\n\n\
         \x20  *table = stringTable;\n\n\
         \x20  return TA_SUCCESS;\n\
         }\n\n",
    );

    // --- TA_FuncTableFree ---
    o.push_str(
        "TA_RetCode TA_FuncTableFree( TA_StringTable *table )\n\
         {\n\
         \x20  TA_StringTablePriv *stringTablePriv;\n\n\
         \x20  if( table )\n\
         \x20  {\n\
         \x20     stringTablePriv = (TA_StringTablePriv *)table->hiddenData;\n\
         \x20     if( !stringTablePriv )\n\
         \x20     {\n\
         \x20        return TA_INTERNAL_ERROR(3);\n\
         \x20     }\n\n\
         \x20     if( stringTablePriv->magicNumber != TA_STRING_TABLE_FUNC_MAGIC_NB )\n\
         \x20     {\n\
         \x20        return TA_BAD_OBJECT;\n\
         \x20     }\n\n\
         \x20     if( table->string )\n\
         \x20        TA_Free( (void *)table->string );\n\n\
         \x20     TA_Free( table );\n\
         \x20  }\n\n\
         \x20  return TA_SUCCESS;\n\
         }\n\n",
    );

    // --- TA_GetFuncHandle ---
    o.push_str(
        "TA_RetCode TA_GetFuncHandle( const char *name, const TA_FuncHandle **handle )\n\
         {\n\
         \x20  char firstChar, tmp;\n\
         \x20  const TA_FuncDef **funcDefTable;\n\
         \x20  const TA_FuncDef *funcDef;\n\
         \x20  const TA_FuncInfo *funcInfo;\n\
         \x20  unsigned int i, funcDefTableSize;\n\n\
         \x20  if( (name == NULL) || (handle == NULL) )\n\
         \x20  {\n\
         \x20     return TA_BAD_PARAM;\n\
         \x20  }\n\n\
         \x20  *handle = NULL;\n\n\
         \x20  firstChar = name[0];\n\n\
         \x20  if( firstChar == '\\0' )\n\
         \x20  {\n\
         \x20     return TA_BAD_PARAM;\n\
         \x20  }\n\n\
         \x20  tmp = (char)tolower( firstChar );\n\n\
         \x20  if( (tmp < 'a') || (tmp > 'z') )\n\
         \x20  {\n\
         \x20     return TA_FUNC_NOT_FOUND;\n\
         \x20  }\n\n\
         \x20  tmp -= (char)'a';\n\
         \x20  funcDefTable = TA_DEF_Tables[(int)tmp];\n\n\
         \x20  funcDefTableSize = *TA_DEF_TablesSize[(int)tmp];\n\
         \x20  if( funcDefTableSize < 1 )\n\
         \x20  {\n\
         \x20     return TA_FUNC_NOT_FOUND;\n\
         \x20  }\n\n\
         \x20  for( i=0; i < funcDefTableSize; i++ )\n\
         \x20  {\n\
         \x20     funcDef = funcDefTable[i];\n\
         \x20     if( !funcDef || !funcDef->funcInfo )\n\
         \x20        return TA_INTERNAL_ERROR(3);\n\n\
         \x20     funcInfo = funcDef->funcInfo;\n\
         \x20     if( !funcInfo )\n\
         \x20        return TA_INTERNAL_ERROR(4);\n\n\
         \x20     if( strcmp( funcInfo->name, name ) == 0 )\n\
         \x20     {\n\
         \x20        *handle = (TA_FuncHandle *)funcDef;\n\
         \x20        return TA_SUCCESS;\n\
         \x20     }\n\
         \x20  }\n\n\
         \x20  return TA_FUNC_NOT_FOUND;\n\
         }\n\n",
    );

    // --- TA_GetFuncInfo ---
    o.push_str(
        "TA_RetCode TA_GetFuncInfo(  const TA_FuncHandle *handle,\n\
         \x20                           const TA_FuncInfo **funcInfo )\n\
         {\n\
         \x20  const TA_FuncDef *funcDef;\n\n\
         \x20  if( !funcInfo || !handle )\n\
         \x20  {\n\
         \x20     return TA_BAD_PARAM;\n\
         \x20  }\n\n\
         \x20  funcDef = (const TA_FuncDef *)handle;\n\
         \x20  if( funcDef->magicNumber != TA_FUNC_DEF_MAGIC_NB )\n\
         \x20  {\n\
         \x20     return TA_INVALID_HANDLE;\n\
         \x20  }\n\n\
         \x20  *funcInfo = funcDef->funcInfo;\n\
         \x20  if( !funcDef->funcInfo )\n\
         \x20     return TA_INVALID_HANDLE;\n\n\
         \x20  return TA_SUCCESS;\n\
         }\n\n",
    );

    // --- TA_GetInputParameterInfo ---
    o.push_str(
        "TA_RetCode TA_GetInputParameterInfo( const TA_FuncHandle *handle,\n\
         \x20                                    unsigned int paramIndex,\n\
         \x20                                    const TA_InputParameterInfo **info )\n\
         {\n\
         \x20  const TA_FuncDef  *funcDef;\n\
         \x20  const TA_FuncInfo *funcInfo;\n\
         \x20  const TA_InputParameterInfo **inputTable;\n\n\
         \x20  if( (handle == NULL) || (info == NULL) )\n\
         \x20  {\n\
         \x20     return TA_BAD_PARAM;\n\
         \x20  }\n\n\
         \x20  *info = NULL;\n\n\
         \x20  funcDef = (const TA_FuncDef *)handle;\n\
         \x20  if( funcDef->magicNumber != TA_FUNC_DEF_MAGIC_NB )\n\
         \x20  {\n\
         \x20     return TA_INVALID_HANDLE;\n\
         \x20  }\n\
         \x20  funcInfo = funcDef->funcInfo;\n\n\
         \x20  if( !funcInfo ) return TA_INVALID_HANDLE;\n\n\
         \x20  if( paramIndex >= funcInfo->nbInput )\n\
         \x20  {\n\
         \x20     return TA_BAD_PARAM;\n\
         \x20  }\n\n\
         \x20  inputTable = (const TA_InputParameterInfo **)funcDef->input;\n\n\
         \x20  if( !inputTable )\n\
         \x20     return TA_INTERNAL_ERROR(2);\n\n\
         \x20  *info = inputTable[paramIndex];\n\n\
         \x20  if( !(*info) )\n\
         \x20     return TA_INTERNAL_ERROR(3);\n\n\
         \x20  return TA_SUCCESS;\n\
         }\n\n",
    );

    // --- TA_GetOptInputParameterInfo ---
    o.push_str(
        "TA_RetCode TA_GetOptInputParameterInfo( const TA_FuncHandle *handle,\n\
         \x20                                       unsigned int paramIndex,\n\
         \x20                                       const TA_OptInputParameterInfo **info )\n\
         {\n\
         \x20  const TA_FuncDef  *funcDef;\n\
         \x20  const TA_FuncInfo *funcInfo;\n\
         \x20  const TA_OptInputParameterInfo **inputTable;\n\n\
         \x20  if( (handle == NULL) || (info == NULL) )\n\
         \x20  {\n\
         \x20     return TA_BAD_PARAM;\n\
         \x20  }\n\n\
         \x20  *info = NULL;\n\n\
         \x20  funcDef = (const TA_FuncDef *)handle;\n\
         \x20  if( funcDef->magicNumber != TA_FUNC_DEF_MAGIC_NB )\n\
         \x20  {\n\
         \x20     return TA_INVALID_HANDLE;\n\
         \x20  }\n\n\
         \x20  funcInfo = funcDef->funcInfo;\n\n\
         \x20  if( !funcInfo )\n\
         \x20     return TA_INVALID_HANDLE;\n\n\
         \x20  if( paramIndex >= funcInfo->nbOptInput )\n\
         \x20  {\n\
         \x20     return TA_BAD_PARAM;\n\
         \x20  }\n\n\
         \x20  inputTable = (const TA_OptInputParameterInfo **)funcDef->optInput;\n\n\
         \x20  if( !inputTable )\n\
         \x20     return TA_INTERNAL_ERROR(3);\n\n\
         \x20  *info = inputTable[paramIndex];\n\n\
         \x20  if( !(*info) )\n\
         \x20     return TA_INTERNAL_ERROR(4);\n\n\
         \x20  return TA_SUCCESS;\n\
         }\n\n",
    );

    // --- TA_GetOutputParameterInfo ---
    o.push_str(
        "TA_RetCode TA_GetOutputParameterInfo( const TA_FuncHandle *handle,\n\
         \x20                                     unsigned int paramIndex,\n\
         \x20                                     const TA_OutputParameterInfo **info )\n\
         {\n\n\
         \x20  const TA_FuncDef  *funcDef;\n\
         \x20  const TA_FuncInfo *funcInfo;\n\
         \x20  const TA_OutputParameterInfo **outputTable;\n\n\
         \x20  if( (handle == NULL) || (info == NULL) )\n\
         \x20  {\n\
         \x20     return TA_BAD_PARAM;\n\
         \x20  }\n\n\
         \x20  *info = NULL;\n\n\
         \x20  funcDef = (const TA_FuncDef *)handle;\n\
         \x20  if( funcDef->magicNumber != TA_FUNC_DEF_MAGIC_NB )\n\
         \x20  {\n\
         \x20     return TA_INVALID_HANDLE;\n\
         \x20  }\n\n\
         \x20  funcInfo = funcDef->funcInfo;\n\n\
         \x20  if( !funcInfo )\n\
         \x20  {\n\
         \x20     return TA_INVALID_HANDLE;\n\
         \x20  }\n\n\
         \x20  if( paramIndex >= funcInfo->nbOutput )\n\
         \x20  {\n\
         \x20     return TA_BAD_PARAM;\n\
         \x20  }\n\n\
         \x20  outputTable = (const TA_OutputParameterInfo **)funcDef->output;\n\n\
         \x20  if( !outputTable )\n\
         \x20  {\n\
         \x20     return TA_INTERNAL_ERROR(4);\n\
         \x20  }\n\n\
         \x20  *info = outputTable[paramIndex];\n\n\
         \x20  if( !(*info) )\n\
         \x20  {\n\
         \x20     return TA_INTERNAL_ERROR(5);\n\
         \x20  }\n\n\
         \x20  return TA_SUCCESS;\n\
         }\n\n",
    );

    // --- TA_ParamHolderAlloc ---
    o.push_str(
        "TA_RetCode TA_ParamHolderAlloc( const TA_FuncHandle *handle,\n\
         \x20                               TA_ParamHolder **allocatedParams )\n\
         {\n\n\
         \x20  TA_FuncDef *funcDef;\n\
         \x20  unsigned int allocSize, i;\n\
         \x20  TA_ParamHolderInput    *input;\n\
         \x20  TA_ParamHolderOptInput *optInput;\n\
         \x20  TA_ParamHolderOutput   *output;\n\n\
         \x20  const TA_FuncInfo *funcInfo;\n\
         \x20  TA_ParamHolder *newParams;\n\
         \x20  TA_ParamHolderPriv *newParamsPriv;\n\n\
         \x20  const TA_InputParameterInfo    **inputInfo;\n\
         \x20  const TA_OptInputParameterInfo **optInputInfo;\n\
         \x20  const TA_OutputParameterInfo   **outputInfo;\n\n\
         \x20  if( !handle || !allocatedParams)\n\
         \x20  {\n\
         \x20     return TA_BAD_PARAM;\n\
         \x20  }\n\n\
         \x20  funcDef = (TA_FuncDef *)handle;\n\
         \x20  if( funcDef->magicNumber != TA_FUNC_DEF_MAGIC_NB )\n\
         \x20  {\n\
         \x20     *allocatedParams = NULL;\n\
         \x20     return TA_INVALID_HANDLE;\n\
         \x20  }\n\n\
         \x20  funcInfo = funcDef->funcInfo;\n\
         \x20  if( !funcInfo ) return TA_INVALID_HANDLE;\n\n\
         \x20  newParams = (TA_ParamHolder *)TA_Malloc( sizeof(TA_ParamHolder) + sizeof(TA_ParamHolderPriv));\n\
         \x20  if( !newParams )\n\
         \x20  {\n\
         \x20     *allocatedParams = NULL;\n\
         \x20     return TA_ALLOC_ERR;\n\
         \x20  }\n\n\
         \x20  memset( newParams, 0, sizeof(TA_ParamHolder) + sizeof(TA_ParamHolderPriv) );\n\
         \x20  newParamsPriv = (TA_ParamHolderPriv *)(((char *)newParams)+sizeof(TA_ParamHolder));\n\
         \x20  newParamsPriv->magicNumber = TA_PARAM_HOLDER_PRIV_MAGIC_NB;\n\
         \x20  newParams->hiddenData = newParamsPriv;\n\n\
         \x20  if( funcInfo->nbInput == 0 ) return TA_INTERNAL_ERROR(2);\n\n\
         \x20  allocSize = (funcInfo->nbInput) * sizeof(TA_ParamHolderInput);\n\
         \x20  input = (TA_ParamHolderInput *)TA_Malloc( allocSize );\n\n\
         \x20  if( !input )\n\
         \x20  {\n\
         \x20     TA_ParamHolderFree( newParams );\n\
         \x20     *allocatedParams = NULL;\n\
         \x20     return TA_ALLOC_ERR;\n\
         \x20  }\n\
         \x20  memset( input, 0, allocSize );\n\
         \x20  newParamsPriv->in = input;\n\n\
         \x20  if( funcInfo->nbOptInput == 0 )\n\
         \x20     optInput = NULL;\n\
         \x20  else\n\
         \x20  {\n\
         \x20     allocSize = (funcInfo->nbOptInput) * sizeof(TA_ParamHolderOptInput);\n\
         \x20     optInput = (TA_ParamHolderOptInput *)TA_Malloc( allocSize );\n\n\
         \x20     if( !optInput )\n\
         \x20     {\n\
         \x20        TA_ParamHolderFree( newParams );\n\
         \x20        *allocatedParams = NULL;\n\
         \x20        return TA_ALLOC_ERR;\n\
         \x20     }\n\
         \x20     memset( optInput, 0, allocSize );\n\
         \x20  }\n\
         \x20  newParamsPriv->optIn = optInput;\n\n\
         \x20  allocSize = (funcInfo->nbOutput) * sizeof(TA_ParamHolderOutput);\n\
         \x20  output = (TA_ParamHolderOutput *)TA_Malloc( allocSize );\n\
         \x20  if( !output )\n\
         \x20  {\n\
         \x20     TA_ParamHolderFree( newParams );\n\
         \x20     *allocatedParams = NULL;\n\
         \x20     return TA_ALLOC_ERR;\n\
         \x20  }\n\
         \x20  memset( output, 0, allocSize );\n\
         \x20  newParamsPriv->out = output;\n\n\
         \x20  newParamsPriv->funcInfo = funcInfo;\n\n\
         \x20  inputInfo    = (const TA_InputParameterInfo **)funcDef->input;\n\
         \x20  optInputInfo = (const TA_OptInputParameterInfo **)funcDef->optInput;\n\
         \x20  outputInfo   = (const TA_OutputParameterInfo   **)funcDef->output;\n\n\
         \x20  for( i=0; i < funcInfo->nbInput; i++ )\n\
         \x20  {\n\
         \x20     input[i].inputInfo = inputInfo[i];\n\
         \x20     newParamsPriv->inBitmap <<= 1;\n\
         \x20     newParamsPriv->inBitmap |= 1;\n\
         \x20  }\n\n\
         \x20  for( i=0; i < funcInfo->nbOptInput; i++ )\n\
         \x20  {\n\
         \x20     optInput[i].optInputInfo = optInputInfo[i];\n\
         \x20     if( optInput[i].optInputInfo->type == TA_OptInput_RealRange )\n\
         \x20        optInput[i].data.optInReal = optInputInfo[i]->defaultValue;\n\
         \x20     else\n\
         \x20        optInput[i].data.optInInteger = (TA_Integer)optInputInfo[i]->defaultValue;\n\
         \x20  }\n\n\
         \x20  for( i=0; i < funcInfo->nbOutput; i++ )\n\
         \x20  {\n\
         \x20     output[i].outputInfo = outputInfo[i];\n\
         \x20     newParamsPriv->outBitmap <<= 1;\n\
         \x20     newParamsPriv->outBitmap |= 1;\n\
         \x20  }\n\n\
         \x20  *allocatedParams = newParams;\n\n\
         \x20  return TA_SUCCESS;\n\
         }\n\n",
    );

    // --- TA_ParamHolderFree ---
    o.push_str(
        "TA_RetCode TA_ParamHolderFree( TA_ParamHolder *paramsToFree )\n\
         {\n\
         \x20  TA_ParamHolderPriv     *paramPriv;\n\n\
         \x20  TA_ParamHolderInput    *input;\n\
         \x20  TA_ParamHolderOptInput *optInput;\n\
         \x20  TA_ParamHolderOutput   *output;\n\n\
         \x20  if( !paramsToFree )\n\
         \x20  {\n\
         \x20     return TA_SUCCESS;\n\
         \x20  }\n\n\
         \x20  paramPriv = paramsToFree->hiddenData;\n\n\
         \x20  if( !paramPriv )\n\
         \x20  {\n\
         \x20     return TA_INVALID_PARAM_HOLDER;\n\
         \x20  }\n\n\
         \x20  if( paramPriv->magicNumber != TA_PARAM_HOLDER_PRIV_MAGIC_NB )\n\
         \x20  {\n\
         \x20     return TA_INVALID_PARAM_HOLDER;\n\
         \x20  }\n\n\
         \x20  optInput = paramPriv->optIn;\n\
         \x20  if( optInput )\n\
         \x20     TA_Free( optInput );\n\n\
         \x20  input = paramPriv->in;\n\
         \x20  if( input )\n\
         \x20     TA_Free( input );\n\n\
         \x20  output = paramPriv->out;\n\
         \x20  if( output )\n\
         \x20     TA_Free( output );\n\n\
         \x20  TA_Free( paramsToFree );\n\n\
         \x20  return TA_SUCCESS;\n\
         }\n\n",
    );

    // --- TA_SetInputParamIntegerPtr ---
    o.push_str(
        "TA_RetCode TA_SetInputParamIntegerPtr( TA_ParamHolder *param,\n\
         \x20                                      unsigned int paramIndex,\n\
         \x20                                      const TA_Integer *value )\n\
         {\n\n\
         \x20  TA_ParamHolderPriv *paramHolderPriv;\n\
         \x20  const TA_InputParameterInfo *paramInfo;\n\
         \x20  const TA_FuncInfo *funcInfo;\n\n\
         \x20  if( (param == NULL) || (value == NULL) )\n\
         \x20  {\n\
         \x20     return TA_BAD_PARAM;\n\
         \x20  }\n\n\
         \x20  paramHolderPriv = (TA_ParamHolderPriv *)(param->hiddenData);\n\
         \x20  if( paramHolderPriv->magicNumber != TA_PARAM_HOLDER_PRIV_MAGIC_NB )\n\
         \x20  {\n\
         \x20     return TA_INVALID_PARAM_HOLDER;\n\
         \x20  }\n\n\
         \x20  funcInfo = paramHolderPriv->funcInfo;\n\
         \x20  if( !funcInfo ) return TA_INVALID_HANDLE;\n\n\
         \x20  if( paramIndex >= funcInfo->nbInput )\n\
         \x20  {\n\
         \x20     return TA_BAD_PARAM;\n\
         \x20  }\n\n\
         \x20  paramInfo = paramHolderPriv->in[paramIndex].inputInfo;\n\
         \x20  if( !paramInfo ) return TA_INTERNAL_ERROR(2);\n\
         \x20  if( paramInfo->type != TA_Input_Integer )\n\
         \x20  {\n\
         \x20     return TA_INVALID_PARAM_HOLDER_TYPE;\n\
         \x20  }\n\n\
         \x20  paramHolderPriv->in[paramIndex].data.inInteger = value;\n\n\
         \x20  paramHolderPriv->inBitmap &= ~(1<<paramIndex);\n\n\
         \x20  return TA_SUCCESS;\n\
         }\n\n",
    );

    // --- TA_SetInputParamRealPtr ---
    o.push_str(
        "TA_RetCode TA_SetInputParamRealPtr( TA_ParamHolder *param,\n\
         \x20                                   unsigned int paramIndex,\n\
         \x20                                   const TA_Real *value )\n\
         {\n\
         \x20  TA_ParamHolderPriv *paramHolderPriv;\n\
         \x20  const TA_InputParameterInfo *paramInfo;\n\
         \x20  const TA_FuncInfo *funcInfo;\n\n\
         \x20  if( (param == NULL) || (value == NULL) )\n\
         \x20  {\n\
         \x20     return TA_BAD_PARAM;\n\
         \x20  }\n\n\
         \x20  paramHolderPriv = (TA_ParamHolderPriv *)(param->hiddenData);\n\
         \x20  if( paramHolderPriv->magicNumber != TA_PARAM_HOLDER_PRIV_MAGIC_NB )\n\
         \x20  {\n\
         \x20     return TA_INVALID_PARAM_HOLDER;\n\
         \x20  }\n\n\
         \x20  funcInfo = paramHolderPriv->funcInfo;\n\
         \x20  if( !funcInfo ) return TA_INVALID_HANDLE;\n\n\
         \x20  if( paramIndex >= funcInfo->nbInput )\n\
         \x20  {\n\
         \x20     return TA_BAD_PARAM;\n\
         \x20  }\n\n\
         \x20  paramInfo = paramHolderPriv->in[paramIndex].inputInfo;\n\
         \x20  if( !paramInfo ) return TA_INTERNAL_ERROR(2);\n\
         \x20  if( paramInfo->type != TA_Input_Real )\n\
         \x20  {\n\
         \x20     return TA_INVALID_PARAM_HOLDER_TYPE;\n\
         \x20  }\n\n\
         \x20  paramHolderPriv->in[paramIndex].data.inReal = value;\n\n\
         \x20  paramHolderPriv->inBitmap &= ~(1<<paramIndex);\n\n\
         \x20  return TA_SUCCESS;\n\
         }\n\n",
    );

    // --- TA_SetInputParamPricePtr ---
    o.push_str(
        "TA_RetCode TA_SetInputParamPricePtr( TA_ParamHolder     *param,\n\
         \x20                                    unsigned int        paramIndex,\n\
         \x20                                    const TA_Real      *open,\n\
         \x20                                    const TA_Real      *high,\n\
         \x20                                    const TA_Real      *low,\n\
         \x20                                    const TA_Real      *close,\n\
         \x20                                    const TA_Real      *volume,\n\
         \x20                                    const TA_Real      *openInterest )\n\
         {\n\n\
         \x20  TA_ParamHolderPriv *paramHolderPriv;\n\
         \x20  const TA_InputParameterInfo *paramInfo;\n\
         \x20  const TA_FuncInfo *funcInfo;\n\n\
         \x20  if( param == NULL )\n\
         \x20  {\n\
         \x20     return TA_BAD_PARAM;\n\
         \x20  }\n\n\
         \x20  paramHolderPriv = (TA_ParamHolderPriv *)(param->hiddenData);\n\
         \x20  if( paramHolderPriv->magicNumber != TA_PARAM_HOLDER_PRIV_MAGIC_NB )\n\
         \x20  {\n\
         \x20     return TA_INVALID_PARAM_HOLDER;\n\
         \x20  }\n\n\
         \x20  funcInfo = paramHolderPriv->funcInfo;\n\
         \x20  if( !funcInfo ) return TA_INVALID_HANDLE;\n\
         \x20  if( paramIndex >= funcInfo->nbInput )\n\
         \x20  {\n\
         \x20     return TA_BAD_PARAM;\n\
         \x20  }\n\n\
         \x20  paramInfo = paramHolderPriv->in[paramIndex].inputInfo;\n\
         \x20  if( !paramInfo ) return TA_INTERNAL_ERROR(2);\n\
         \x20  if( paramInfo->type != TA_Input_Price )\n\
         \x20  {\n\
         \x20     return TA_INVALID_PARAM_HOLDER_TYPE;\n\
         \x20  }\n\n\
         \x20  #define SET_PARAM_INFO(lowerParam,upperParam) \\\n\
         \x20  { \\\n\
         \x20     if( paramInfo->flags & TA_IN_PRICE_##upperParam ) \\\n\
         \x20     { \\\n\
         \x20        if( lowerParam == NULL ) \\\n\
         \x20        { \\\n\
         \x20           return TA_BAD_PARAM; \\\n\
         \x20        } \\\n\
         \x20        paramHolderPriv->in[paramIndex].data.inPrice.lowerParam = lowerParam; \\\n\
         \x20     } \\\n\
         \x20  }\n\n\
         \x20  SET_PARAM_INFO(open, OPEN );\n\
         \x20  SET_PARAM_INFO(high, HIGH );\n\
         \x20  SET_PARAM_INFO(low, LOW );\n\
         \x20  SET_PARAM_INFO(close, CLOSE );\n\
         \x20  SET_PARAM_INFO(volume, VOLUME );\n\
         \x20  SET_PARAM_INFO(openInterest, OPENINTEREST );\n\n\
         \x20  #undef SET_PARAM_INFO\n\n\
         \x20  paramHolderPriv->inBitmap &= ~(1<<paramIndex);\n\n\
         \x20  return TA_SUCCESS;\n\
         }\n\n",
    );

    // --- TA_SetOptInputParamInteger ---
    o.push_str(
        "TA_RetCode TA_SetOptInputParamInteger( TA_ParamHolder *param,\n\
         \x20                                      unsigned int paramIndex,\n\
         \x20                                      TA_Integer value )\n\
         {\n\n\
         \x20  TA_ParamHolderPriv *paramHolderPriv;\n\
         \x20  const TA_OptInputParameterInfo *paramInfo;\n\
         \x20  const TA_FuncInfo *funcInfo;\n\n\
         \x20  if( param == NULL )\n\
         \x20  {\n\
         \x20     return TA_BAD_PARAM;\n\
         \x20  }\n\n\
         \x20  paramHolderPriv = (TA_ParamHolderPriv *)(param->hiddenData);\n\
         \x20  if( paramHolderPriv->magicNumber != TA_PARAM_HOLDER_PRIV_MAGIC_NB )\n\
         \x20  {\n\
         \x20     return TA_INVALID_PARAM_HOLDER;\n\
         \x20  }\n\n\
         \x20  funcInfo = paramHolderPriv->funcInfo;\n\
         \x20  if( !funcInfo ) return TA_INVALID_HANDLE;\n\
         \x20  if( paramIndex >= funcInfo->nbOptInput )\n\
         \x20  {\n\
         \x20     return TA_BAD_PARAM;\n\
         \x20  }\n\n\
         \x20  paramInfo = paramHolderPriv->optIn[paramIndex].optInputInfo;\n\
         \x20  if( !paramInfo ) return TA_INTERNAL_ERROR(2);\n\
         \x20  if( (paramInfo->type != TA_OptInput_IntegerRange) &&\n\
         \x20      (paramInfo->type != TA_OptInput_IntegerList) )\n\
         \x20  {\n\
         \x20     return TA_INVALID_PARAM_HOLDER_TYPE;\n\
         \x20  }\n\n\
         \x20  paramHolderPriv->optIn[paramIndex].data.optInInteger = value;\n\n\
         \x20  return TA_SUCCESS;\n\
         }\n\n",
    );

    // --- TA_SetOptInputParamReal ---
    o.push_str(
        "TA_RetCode TA_SetOptInputParamReal( TA_ParamHolder *param,\n\
         \x20                                   unsigned int paramIndex,\n\
         \x20                                   TA_Real value )\n\
         {\n\
         \x20  TA_ParamHolderPriv *paramHolderPriv;\n\
         \x20  const TA_OptInputParameterInfo *paramInfo;\n\
         \x20  const TA_FuncInfo *funcInfo;\n\n\
         \x20  if( param == NULL )\n\
         \x20  {\n\
         \x20     return TA_BAD_PARAM;\n\
         \x20  }\n\n\
         \x20  paramHolderPriv = (TA_ParamHolderPriv *)(param->hiddenData);\n\
         \x20  if( paramHolderPriv->magicNumber != TA_PARAM_HOLDER_PRIV_MAGIC_NB )\n\
         \x20  {\n\
         \x20     return TA_INVALID_PARAM_HOLDER;\n\
         \x20  }\n\n\
         \x20  funcInfo = paramHolderPriv->funcInfo;\n\
         \x20  if( !funcInfo ) return TA_INVALID_HANDLE;\n\n\
         \x20  if( paramIndex >= funcInfo->nbOptInput )\n\
         \x20  {\n\
         \x20     return TA_BAD_PARAM;\n\
         \x20  }\n\n\
         \x20  paramInfo = paramHolderPriv->optIn[paramIndex].optInputInfo;\n\
         \x20  if( !paramInfo ) return TA_INTERNAL_ERROR(2);\n\
         \x20  if( (paramInfo->type != TA_OptInput_RealRange) &&\n\
         \x20      (paramInfo->type != TA_OptInput_RealList) )\n\
         \x20  {\n\
         \x20     return TA_INVALID_PARAM_HOLDER_TYPE;\n\
         \x20  }\n\n\
         \x20  paramHolderPriv->optIn[paramIndex].data.optInReal = value;\n\n\
         \x20  return TA_SUCCESS;\n\
         }\n\n",
    );

    // --- TA_SetOutputParamIntegerPtr ---
    o.push_str(
        "TA_RetCode TA_SetOutputParamIntegerPtr( TA_ParamHolder *param,\n\
         \x20                                       unsigned int paramIndex,\n\
         \x20                                       TA_Integer     *out )\n\
         {\n\
         \x20  TA_ParamHolderPriv *paramHolderPriv;\n\
         \x20  const TA_OutputParameterInfo *paramInfo;\n\
         \x20  const TA_FuncInfo *funcInfo;\n\n\
         \x20  if( (param == NULL) || (out == NULL) )\n\
         \x20  {\n\
         \x20     return TA_BAD_PARAM;\n\
         \x20  }\n\n\
         \x20  paramHolderPriv = (TA_ParamHolderPriv *)(param->hiddenData);\n\
         \x20  if( paramHolderPriv->magicNumber != TA_PARAM_HOLDER_PRIV_MAGIC_NB )\n\
         \x20  {\n\
         \x20     return TA_INVALID_PARAM_HOLDER;\n\
         \x20  }\n\n\
         \x20  funcInfo = paramHolderPriv->funcInfo;\n\
         \x20  if( !funcInfo ) return TA_INVALID_HANDLE;\n\n\
         \x20  if( paramIndex >= funcInfo->nbOutput )\n\
         \x20  {\n\
         \x20     return TA_BAD_PARAM;\n\
         \x20  }\n\n\
         \x20  paramInfo = paramHolderPriv->out[paramIndex].outputInfo;\n\
         \x20  if( !paramInfo ) return TA_INTERNAL_ERROR(2);\n\
         \x20  if( paramInfo->type != TA_Output_Integer )\n\
         \x20  {\n\
         \x20     return TA_INVALID_PARAM_HOLDER_TYPE;\n\
         \x20  }\n\n\
         \x20  paramHolderPriv->out[paramIndex].data.outInteger = out;\n\n\
         \x20  paramHolderPriv->outBitmap &= ~(1<<paramIndex);\n\n\
         \x20  return TA_SUCCESS;\n\
         }\n\n",
    );

    // --- TA_SetOutputParamRealPtr ---
    o.push_str(
        "TA_RetCode TA_SetOutputParamRealPtr( TA_ParamHolder *param,\n\
         \x20                                    unsigned int paramIndex,\n\
         \x20                                    TA_Real        *out )\n\
         {\n\
         \x20  TA_ParamHolderPriv *paramHolderPriv;\n\
         \x20  const TA_OutputParameterInfo *paramInfo;\n\
         \x20  const TA_FuncInfo *funcInfo;\n\n\
         \x20  if( (param == NULL) || (out == NULL) )\n\
         \x20  {\n\
         \x20     return TA_BAD_PARAM;\n\
         \x20  }\n\n\
         \x20  paramHolderPriv = (TA_ParamHolderPriv *)(param->hiddenData);\n\
         \x20  if( paramHolderPriv->magicNumber != TA_PARAM_HOLDER_PRIV_MAGIC_NB )\n\
         \x20  {\n\
         \x20     return TA_INVALID_PARAM_HOLDER;\n\
         \x20  }\n\n\
         \x20  funcInfo = paramHolderPriv->funcInfo;\n\
         \x20  if( !funcInfo ) return TA_INVALID_HANDLE;\n\n\
         \x20  if( paramIndex >= funcInfo->nbOutput )\n\
         \x20  {\n\
         \x20     return TA_BAD_PARAM;\n\
         \x20  }\n\n\
         \x20  paramInfo = paramHolderPriv->out[paramIndex].outputInfo;\n\
         \x20  if( !paramInfo ) return TA_INTERNAL_ERROR(2);\n\
         \x20  if( paramInfo->type != TA_Output_Real )\n\
         \x20  {\n\
         \x20     return TA_INVALID_PARAM_HOLDER_TYPE;\n\
         \x20  }\n\n\
         \x20  paramHolderPriv->out[paramIndex].data.outReal = out;\n\n\
         \x20  paramHolderPriv->outBitmap &= ~(1<<paramIndex);\n\n\
         \x20  return TA_SUCCESS;\n\
         }\n\n",
    );

    // --- TA_GetLookback ---
    o.push_str(
        "TA_RetCode TA_GetLookback( const TA_ParamHolder *param, TA_Integer *lookback )\n\
         {\n\
         \x20  const TA_ParamHolderPriv *paramHolderPriv;\n\n\
         \x20  const TA_FuncDef *funcDef;\n\
         \x20  const TA_FuncInfo *funcInfo;\n\
         \x20  TA_FrameLookback lookbackFunction;\n\n\
         \x20  if( (param == NULL) || (lookback == NULL))\n\
         \x20  {\n\
         \x20     return TA_BAD_PARAM;\n\
         \x20  }\n\n\
         \x20  paramHolderPriv = (TA_ParamHolderPriv *)(param->hiddenData);\n\
         \x20  if( paramHolderPriv->magicNumber != TA_PARAM_HOLDER_PRIV_MAGIC_NB )\n\
         \x20  {\n\
         \x20     return TA_INVALID_PARAM_HOLDER;\n\
         \x20  }\n\n\
         \x20  funcInfo = paramHolderPriv->funcInfo;\n\
         \x20  if( !funcInfo ) return TA_INVALID_HANDLE;\n\n\
         \x20  funcDef = (const TA_FuncDef *)funcInfo->handle;\n\
         \x20  if( !funcDef ) return TA_INTERNAL_ERROR(2);\n\
         \x20  lookbackFunction = funcDef->lookback;\n\
         \x20  if( !lookbackFunction ) return TA_INTERNAL_ERROR(2);\n\n\
         \x20  *lookback = (*lookbackFunction)( paramHolderPriv );\n\n\
         \x20  return TA_SUCCESS;\n\
         }\n\n",
    );

    // --- TA_CallFunc ---
    o.push_str(
        "TA_RetCode TA_CallFunc( const TA_ParamHolder *param,\n\
         \x20                       TA_Integer            startIdx,\n\
         \x20                       TA_Integer            endIdx,\n\
         \x20                       TA_Integer           *outBegIdx,\n\
         \x20                       TA_Integer           *outNbElement )\n\
         {\n\
         \x20  TA_RetCode retCode;\n\
         \x20  const TA_ParamHolderPriv *paramHolderPriv;\n\n\
         \x20  const TA_FuncDef *funcDef;\n\
         \x20  const TA_FuncInfo *funcInfo;\n\
         \x20  TA_FrameFunction function;\n\n\
         \x20  if( (param == NULL) ||\n\
         \x20      (outBegIdx == NULL) ||\n\
         \x20      (outNbElement == NULL) )\n\
         \x20  {\n\
         \x20     return TA_BAD_PARAM;\n\
         \x20  }\n\n\
         \x20  paramHolderPriv = (TA_ParamHolderPriv *)(param->hiddenData);\n\
         \x20  if( paramHolderPriv->magicNumber != TA_PARAM_HOLDER_PRIV_MAGIC_NB )\n\
         \x20  {\n\
         \x20     return TA_INVALID_PARAM_HOLDER;\n\
         \x20  }\n\n\
         \x20  if( paramHolderPriv->inBitmap != 0 )\n\
         \x20  {\n\
         \x20     return TA_INPUT_NOT_ALL_INITIALIZE;\n\
         \x20  }\n\n\
         \x20  if( paramHolderPriv->outBitmap != 0 )\n\
         \x20  {\n\
         \x20     return TA_OUTPUT_NOT_ALL_INITIALIZE;\n\
         \x20  }\n\n\
         \x20  funcInfo = paramHolderPriv->funcInfo;\n\
         \x20  if( !funcInfo ) return TA_INVALID_HANDLE;\n\
         \x20  funcDef = (const TA_FuncDef *)funcInfo->handle;\n\
         \x20  if( !funcDef ) return TA_INTERNAL_ERROR(2);\n\
         \x20  function = funcDef->function;\n\
         \x20  if( !function ) return TA_INTERNAL_ERROR(2);\n\n\
         \x20  retCode = (*function)( paramHolderPriv, startIdx, endIdx,\n\
         \x20                         outBegIdx, outNbElement );\n\
         \x20  return retCode;\n\
         }\n\n",
    );

    // --- Local functions ---
    o.push_str(
        "static TA_RetCode getGroupId( const char *groupString, unsigned int *groupId )\n\
         {\n\
         \x20  unsigned int i;\n\n\
         \x20  for( i=0; i < TA_NB_GROUP_ID; i++ )\n\
         \x20  {\n\
         \x20     if( strcmp( TA_GroupString[i], groupString ) == 0 )\n\
         \x20     {\n\
         \x20        *groupId = i;\n\
         \x20        return TA_SUCCESS;\n\
         \x20     }\n\
         \x20  }\n\n\
         \x20  return TA_GROUP_NOT_FOUND;\n\
         }\n\n",
    );

    o.push_str(
        "static TA_RetCode getGroupSize( TA_GroupId groupId, unsigned int *groupSize )\n\
         {\n\
         \x20  *groupSize = TA_PerGroupSize[groupId];\n\n\
         \x20  return TA_SUCCESS;\n\
         }\n\n",
    );

    o.push_str(
        "static TA_RetCode getFuncNameByIdx( TA_GroupId groupId,\n\
         \x20                                    unsigned int idx,\n\
         \x20                                    const char **stringPtr )\n\
         {\n\
         \x20  const TA_FuncDef **funcDefTable;\n\
         \x20  const TA_FuncInfo *funcInfo;\n\n\
         \x20  funcDefTable = TA_PerGroupFuncDef[groupId];\n\
         \x20  funcInfo = funcDefTable[idx]->funcInfo;\n\
         \x20  *stringPtr = funcInfo->name;\n\n\
         \x20  return TA_SUCCESS;\n\
         }\n",
    );

    o
}

// ---------------------------------------------------------------------------
// File 9: ta_func_api.c
// ---------------------------------------------------------------------------

/// Generate `ta_func_api.c` — binary-encoded XML of all function metadata.
///
/// Reads the already-generated `ta_func_api.xml` from the repo root and
/// encodes it as a C byte array.
fn gen_ta_func_api_c(repo_root: &Path) -> String {
    let xml_path = repo_root.join("ta_func_api.xml");
    let xml = std::fs::read_to_string(&xml_path)
        .unwrap_or_else(|e| panic!("Cannot read {}: {}", xml_path.display(), e));

    let mut o = String::with_capacity(xml.len() * 6);
    o.push_str(LICENSE);
    o.push_str(
        "/* Important: This file is automatically generated by ta_codegen.\n\
         \x20*            Any modification will be lost on next execution\n\
         \x20*            of ta_codegen.\n\
         \x20*\n\
         \x20* This file is a binary representation of the func_api.xml file.\n\
         \x20*/\n\n\
         #include \"ta_abstract.h\"\n\n\
         static const char TA_FunctionDescriptionXMLArray[] =\n{\n",
    );

    // Encode XML as hex bytes, 21 per line (matching reference format).
    let bytes = xml.as_bytes();
    for (i, &b) in bytes.iter().enumerate() {
        if i == 0 {
            let _ = write!(o, "0x{b:02X}");
        } else if i % 21 == 0 {
            let _ = write!(o, "\n,0x{b:02X}");
        } else {
            let _ = write!(o, ",0x{b:02X}");
        }
    }
    // NUL-terminate so TA_FunctionDescriptionXML() returns a valid C string.
    let _ = write!(o, ",0x00");
    o.push_str("};\n\n");

    o.push_str(
        "const char *TA_FunctionDescriptionXML()\n\
         {\n\
         \x20  return TA_FunctionDescriptionXMLArray;\n\
         }\n\n\
         /***************/\n\
         /* End of File */\n\
         /***************/\n",
    );

    o
}

// ---------------------------------------------------------------------------
// File 10: include/ta_func.h
// ---------------------------------------------------------------------------

/// Generate `include/ta_func.h` — the public C header with all function prototypes.
#[allow(clippy::too_many_lines)]
fn gen_ta_func_h(funcs: &[&FuncDef]) -> String {
    let mut o = String::with_capacity(256 * 1024);
    // LICENSE ends with "\n\n" but ta_func.h has no blank line before #ifndef.
    o.push_str(LICENSE.trim_end_matches('\n'));
    o.push('\n');

    // Header guard + includes
    o.push_str(
        "#ifndef TA_FUNC_H\n\
         #define TA_FUNC_H\n\
         \n\
         #ifndef TA_COMMON_H\n\
         \x20  #include \"ta_common.h\"\n\
         #endif\n\
         \n\
         /* This header contains the prototype of all the Technical Analysis\n\
         \x20* function provided by TA-LIB.\n\
         \x20*/\n\
         \n\
         /* TA-LIB Developer Note: Do not modify this file, it is automaticaly\n\
         \x20*                        generated by ta_codegen.\n\
         \x20*/\n\
         #ifdef __cplusplus\n\
         extern \"C\" {\n\
         #endif\n\
         \n\
         #ifndef TA_DEFS_H\n\
         \x20  #include \"ta_defs.h\"\n\
         #endif\n\n\n",
    );

    // Emit all function prototypes.
    let ref_lookup = RefFuncsLookup(funcs);
    for func in funcs {
        emit_func_h_block(&mut o, func, &ref_lookup);
    }

    // Utility function section (unstable period, compatibility, candle settings).
    o.push_str(
        "/* Some TA functions takes a certain amount of input data\n\
         \x20* before stabilizing and outputing meaningful data. This is\n\
         \x20* a behavior pertaining to the algo of some TA functions and\n\
         \x20* is not particular to the TA-Lib implementation.\n\
         \x20* TA-Lib allows you to automatically strip off these unstabl\n\
         \x20* data from your output and from any internal processing.\n\
         \x20* (See documentation for more info)\n\
         \x20*\n\
         \x20* Examples:\n\
         \x20*      TA_SetUnstablePeriod( TA_FUNC_UNST_EMA, 30 );\n\
         \x20*           Always strip off 30 price bar for the TA_EMA function.\n\
         \x20*\n\
         \x20*      TA_SetUnstablePeriod( TA_FUNC_UNST_ALL, 30 );\n\
         \x20*           Always strip off 30 price bar from ALL functions\n\
         \x20*           having an unstable period.\n\
         \x20*\n\
         \x20* See ta_defs.h for the enumeration TA_FuncUnstId\n\
         \x20*/\n\
         \n\
         TA_LIB_API TA_RetCode TA_SetUnstablePeriod( TA_FuncUnstId id,\n\
         \x20                                unsigned int  unstablePeriod );\n\
         \n\
         TA_LIB_API unsigned int TA_GetUnstablePeriod( TA_FuncUnstId id );\n\
         \n\
         /* You can change slightly the behavior of the TA functions\n\
         \x20* by requesting compatibiliy with some existing software.\n\
         \x20*\n\
         \x20* By default, the behavior is as close as the original \n\
         \x20* author of the TA functions intend it to be.\n\
         \x20*\n\
         \x20* See ta_defs.h for the enumeration TA_Compatibility.\n\
         \x20*/\n\
         TA_LIB_API TA_RetCode TA_SetCompatibility( TA_Compatibility value );\n\
         TA_LIB_API TA_Compatibility TA_GetCompatibility( void );\n\
         \n\
         /* Candlesticks struct and functions\n\
         \x20* Because candlestick patterns are subjective, it is necessary \n\
         \x20* to allow the user to specify what should be the meaning of \n\
         \x20* 'long body', 'short shadows', etc.\n\
         \x20*/\n\
         \n\
         /* Call TA_SetCandleSettings to set that when comparing a candle \n\
         \x20* basing on settingType it must be compared with the average \n\
         \x20* of the last avgPeriod candles' rangeType multiplied by factor.\n\
         \x20* This setting is valid until TA_RestoreCandleDefaultSettings is called\n\
         \x20*/\n\
         TA_LIB_API TA_RetCode TA_SetCandleSettings( TA_CandleSettingType settingType,\n\
         \x20                                TA_RangeType rangeType, \n\
         \x20                                int avgPeriod, \n\
         \x20                                double factor );\n\
         \n\
         /* Call TA_RestoreCandleDefaultSettings after using custom settings \n\
         \x20* to restore the default settings for the specified settingType\n\
         \x20*/\n\
         TA_LIB_API TA_RetCode TA_RestoreCandleDefaultSettings( TA_CandleSettingType settingType );\n\
         \n\
         #ifdef __cplusplus\n\
         }\n\
         #endif\n\
         \n\
         #endif\n\
         \n\
         /***************/\n\
         /* End of File */\n\
         /***************/\n\n",
    );

    o
}

/// Emit the comment block + double/single/lookback prototypes for one function.
/// [`crate::streaming::CalleeLookup`] over the borrowed FuncDef list this
/// backend already carries (dispatch capability notes in the header).
struct RefFuncsLookup<'a>(&'a [&'a FuncDef]);

impl crate::streaming::CalleeLookup for RefFuncsLookup<'_> {
    fn callee(&self, name: &str) -> Option<crate::streaming::CalleeSig> {
        self.0
            .iter()
            .find(|f| f.name.eq_ignore_ascii_case(name))
            .map(|f| crate::streaming::callee_sig_of(f))
    }
}

fn emit_func_h_block(o: &mut String, func: &FuncDef, lookup: &dyn crate::streaming::CalleeLookup) {
    let name = &func.name;
    let hint = func.hint.as_deref().unwrap_or(name);

    // --- Comment block ---
    let _ = writeln!(o, "/*");
    let _ = writeln!(o, " * TA_{name} - {hint}");
    o.push_str(" * \n");

    // Input line
    let abstract_inputs = reconstruct_abstract_inputs(&func.inputs);
    let input_desc: Vec<String> = abstract_inputs
        .iter()
        .map(|ai| match ai {
            AbstractInput::Price(components) => components
                .iter()
                .map(|c| capitalize_first(c))
                .collect::<Vec<_>>()
                .join(", "),
            AbstractInput::Single(input) => match input.param_type {
                ParamType::Integer => "integer".to_string(),
                ParamType::Real | ParamType::Enum(_) | ParamType::Price(_) => {
                    "double".to_string()
                }
            },
        })
        .collect();
    let _ = writeln!(o, " * Input  = {}", input_desc.join(", "));

    // Output line
    let output_desc: Vec<&str> = func
        .outputs
        .iter()
        .map(|out| match out.param_type {
            ParamType::Integer => "int",
            ParamType::Real | ParamType::Enum(_) | ParamType::Price(_) => "double",
        })
        .collect();
    let _ = writeln!(o, " * Output = {}", output_desc.join(", "));
    o.push_str(" * \n");

    // Optional parameters section
    if !func.optional_inputs.is_empty() {
        o.push_str(" * Optional Parameters\n");
        o.push_str(" * -------------------\n");
        for opt in &func.optional_inputs {
            let range_str = opt_input_range_str(opt);
            if range_str.is_empty() {
                let _ = writeln!(o, " * {}:", opt.name);
            } else {
                let _ = writeln!(o, " * {}:{}", opt.name, range_str);
            }
            if let Some(hint) = &opt.hint {
                let _ = writeln!(o, " *    {hint}");
            }
            o.push_str(" * \n");
        }
        // Extra blank line before closing
        o.push_str(" * \n");
    }

    o.push_str(" */\n");

    // --- Double-precision prototype: TA_XXX ---
    emit_func_prototype(o, name, func);
    o.push('\n');

    // --- Single-precision prototype: TA_S_XXX ---
    emit_func_s_prototype(o, name, func);
    o.push('\n');

    // --- Lookback prototype: TA_XXX_Lookback ---
    emit_lookback_prototype(o, name, func);
    // Match gen_code spacing: 2 blank lines after lookback, except when
    // the last opt param is TA_MAType (Enum) which gets only 1 blank line.
    let last_is_enum = func
        .optional_inputs
        .last()
        .is_some_and(|o| matches!(&o.param_type, ParamType::Enum(_)));
    if last_is_enum {
        o.push('\n');
    } else {
        o.push_str("\n\n");
    }

    // --- Streaming API declarations (only for YAML-declared functions) ---
    if func.streaming {
        o.push_str(&crate::backends::c_stream::header_decls(func, lookup));
        o.push('\n');
    }
}

/// Format the range string for a comment block, e.g. "(From 2 to 100000)".
fn opt_input_range_str(opt: &OptInput) -> String {
    match &opt.param_type {
        ParamType::Enum(_) | ParamType::Price(_) => String::new(),
        ParamType::Integer => {
            if let Some((min, max)) = opt.range {
                let min_s = format_range_val_int(min);
                let max_s = format_range_val_int(max);
                format!("(From {min_s} to {max_s})")
            } else {
                String::new()
            }
        }
        ParamType::Real => {
            if let Some((min, max)) = opt.range {
                let min_s = format_range_val_real(min);
                let max_s = format_range_val_real(max);
                format!("(From {min_s} to {max_s})")
            } else {
                String::new()
            }
        }
    }
}

/// Format a range boundary value for integer opt params.
fn format_range_val_int(v: f64) -> String {
    // i32 sentinels stored as f64
    if v <= f64::from(i32::MIN) {
        "TA_INTEGER_MIN".to_string()
    } else if v >= f64::from(i32::MAX) {
        "TA_INTEGER_MAX".to_string()
    } else {
        #[allow(clippy::cast_possible_truncation)]
        let i = v as i64;
        format!("{i}")
    }
}

/// Format a range boundary value for real opt params.
fn format_range_val_real(v: f64) -> String {
    if v <= f64::MIN + f64::EPSILON.abs() {
        "TA_REAL_MIN".to_string()
    } else if v >= f64::MAX - f64::MAX * f64::EPSILON {
        "TA_REAL_MAX".to_string()
    } else {
        format!("{v}")
    }
}

/// Format the inline range comment for a prototype param, e.g. `/* From 2 to 100000 */`.
fn opt_input_inline_comment(opt: &OptInput) -> String {
    let r = opt_input_range_str(opt);
    if r.is_empty() {
        return String::new();
    }
    // Strip the parentheses: "(From X to Y)" -> "From X to Y"
    let inner = &r[1..r.len() - 1];
    format!("/* {inner} */")
}

/// Emit the double-precision prototype: `TA_LIB_API TA_RetCode TA_XXX(...)`.
fn emit_func_prototype(o: &mut String, name: &str, func: &FuncDef) {
    let prefix = format!("TA_LIB_API TA_RetCode TA_{name}( ");
    let end_idx_indent = prefix.len();
    let param_indent = end_idx_indent + 11;

    // First line: startIdx
    let _ = writeln!(o, "{prefix}int    startIdx,");
    // Second line: endIdx
    let _ = writeln!(o, "{}int    endIdx,", " ".repeat(end_idx_indent));

    let pad = " ".repeat(param_indent);

    // Input parameters
    for input in &func.inputs {
        let _ = writeln!(o, "{pad}const double {}[],", input.name);
    }

    // Optional input parameters
    for (i, opt) in func.optional_inputs.iter().enumerate() {
        let is_last_opt =
            i == func.optional_inputs.len() - 1 && func.outputs.is_empty();
        let comment = opt_input_inline_comment(opt);
        let (type_str, name_str) = opt_param_type_and_name(opt);
        let comma = if is_last_opt { "" } else { "," };
        if comment.is_empty() {
            let _ = writeln!(o, "{pad}{type_str}{name_str}{comma}");
        } else {
            let _ = writeln!(
                o,
                "{pad}{type_str}{name_str}{comma} {comment}"
            );
        }
    }

    // outBegIdx and outNBElement
    let _ = writeln!(o, "{pad}int          *outBegIdx,");
    let _ = writeln!(o, "{pad}int          *outNBElement,");

    // Output parameters
    for (i, out) in func.outputs.iter().enumerate() {
        let is_last = i == func.outputs.len() - 1;
        let (type_str, arr_suffix) = output_type_and_suffix(out);
        if is_last {
            let _ = writeln!(o, "{pad}{type_str}{}{arr_suffix} );", out.name);
        } else {
            let _ = writeln!(o, "{pad}{type_str}{}[],", out.name);
        }
    }
}

/// Emit the single-precision prototype: `TA_LIB_API TA_RetCode TA_S_XXX(...)`.
fn emit_func_s_prototype(o: &mut String, name: &str, func: &FuncDef) {
    let prefix = format!("TA_LIB_API TA_RetCode TA_S_{name}( ");
    let end_idx_indent = prefix.len();
    let param_indent = end_idx_indent + 11;

    // First line: startIdx
    let _ = writeln!(o, "{prefix}int    startIdx,");
    // Second line: endIdx
    let _ = writeln!(o, "{}int    endIdx,", " ".repeat(end_idx_indent));

    let pad = " ".repeat(param_indent);

    // Input parameters (float for S_ variant)
    for input in &func.inputs {
        let _ = writeln!(o, "{pad}const float  {}[],", input.name);
    }

    // Optional input parameters (same types as double variant)
    for (i, opt) in func.optional_inputs.iter().enumerate() {
        let is_last_opt =
            i == func.optional_inputs.len() - 1 && func.outputs.is_empty();
        let comment = opt_input_inline_comment(opt);
        let (type_str, name_str) = opt_param_type_and_name(opt);
        let comma = if is_last_opt { "" } else { "," };
        if comment.is_empty() {
            let _ = writeln!(o, "{pad}{type_str}{name_str}{comma}");
        } else {
            let _ = writeln!(
                o,
                "{pad}{type_str}{name_str}{comma} {comment}"
            );
        }
    }

    // outBegIdx and outNBElement
    let _ = writeln!(o, "{pad}int          *outBegIdx,");
    let _ = writeln!(o, "{pad}int          *outNBElement,");

    // Output parameters (always double for S_ variant too)
    for (i, out) in func.outputs.iter().enumerate() {
        let is_last = i == func.outputs.len() - 1;
        let (type_str, arr_suffix) = output_type_and_suffix(out);
        if is_last {
            let _ = writeln!(o, "{pad}{type_str}{}{arr_suffix} );", out.name);
        } else {
            let _ = writeln!(o, "{pad}{type_str}{}[],", out.name);
        }
    }
}

/// Emit the lookback prototype: `TA_LIB_API int TA_XXX_Lookback(...)`.
fn emit_lookback_prototype(o: &mut String, name: &str, func: &FuncDef) {
    let prefix = format!("TA_LIB_API int TA_{name}_Lookback( ");

    if func.optional_inputs.is_empty() {
        let _ = writeln!(o, "{prefix}void );");
        return;
    }

    // Single opt param — all on one line
    if func.optional_inputs.len() == 1 {
        let opt = &func.optional_inputs[0];
        let comment = opt_input_inline_comment(opt);
        let (type_str, name_str) = opt_param_type_and_name(opt);
        if comment.is_empty() {
            let _ = writeln!(o, "{prefix}{type_str}{name_str} );");
        } else {
            let _ = writeln!(
                o,
                "{prefix}{type_str}{name_str} );  {comment}"
            );
        }
        return;
    }

    // Multiple opt params — first on same line, rest indented
    let continuation_indent = prefix.len() + 9;
    let cont_pad = " ".repeat(continuation_indent);

    for (i, opt) in func.optional_inputs.iter().enumerate() {
        let is_last = i == func.optional_inputs.len() - 1;
        let comment = opt_input_inline_comment(opt);
        let (type_str, name_str) = opt_param_type_and_name(opt);

        if i == 0 {
            // First param on same line as function name
            if comment.is_empty() {
                let _ = writeln!(o, "{prefix}{type_str}{name_str},");
            } else {
                let _ = writeln!(
                    o,
                    "{prefix}{type_str}{name_str}, {comment}"
                );
            }
        } else if is_last {
            // Last param — close with );
            if comment.is_empty() {
                let _ =
                    writeln!(o, "{cont_pad}{type_str}{name_str} );");
            } else {
                let _ = writeln!(
                    o,
                    "{cont_pad}{type_str}{name_str} );  {comment}"
                );
            }
        } else {
            // Middle params
            if comment.is_empty() {
                let _ =
                    writeln!(o, "{cont_pad}{type_str}{name_str},");
            } else {
                let _ = writeln!(
                    o,
                    "{cont_pad}{type_str}{name_str}, {comment}"
                );
            }
        }
    }
}

/// Return the C type string and parameter name for an optional input.
///
/// The type string is padded to 14 characters.
fn opt_param_type_and_name(opt: &OptInput) -> (String, String) {
    match &opt.param_type {
        ParamType::Integer | ParamType::Price(_) => {
            ("int           ".to_string(), opt.name.clone())
        }
        ParamType::Real => {
            ("double        ".to_string(), opt.name.clone())
        }
        ParamType::Enum(enum_name) => {
            let c_type = format!("TA_{enum_name}");
            let padded = format!("{c_type:<14}");
            (padded, opt.name.clone())
        }
    }
}

/// Return the C type prefix and array suffix for an output parameter.
///
/// For double outputs: `("double        ", "[]")`
/// For integer outputs: `("int           ", "[]")`
fn output_type_and_suffix(out: &Output) -> (String, &'static str) {
    match out.param_type {
        ParamType::Integer => ("int           ".to_string(), "[]"),
        ParamType::Real | ParamType::Enum(_) | ParamType::Price(_) => {
            ("double        ".to_string(), "[]")
        }
    }
}

/// Capitalize the first letter of a string (e.g., "high" -> "High").
fn capitalize_first(s: &str) -> String {
    let mut chars = s.chars();
    match chars.next() {
        None => String::new(),
        Some(c) => {
            let upper: String = c.to_uppercase().collect();
            upper + chars.as_str()
        }
    }
}

// ---------------------------------------------------------------------------
// Utilities
// ---------------------------------------------------------------------------

