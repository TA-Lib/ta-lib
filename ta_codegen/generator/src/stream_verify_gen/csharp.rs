use super::{
    collect_pin_ids, sv_guard_enum_ints, sv_input_suffix, sv_range_bit, sv_reject_condition,
    SvRangeSite, SV_RANGE_MASK_CSHARP,
};
use crate::ir::{EnumDef, FuncDef};
use std::collections::HashMap;

/// The per-input expanded fuzz array variable in the generated C# handler.
///
/// Same mapping as the C, Rust and Java twins -- price components to their
/// OHLCV series, generic reals to close then volume -- routed through the shared
/// [`sv_input_suffix`] so it cannot drift from what the driver seeds.
fn sv_csharp_input_array(name: &str, generic_idx: &mut usize) -> &'static str {
    match sv_input_suffix(name, generic_idx) {
        "o" => "fz_o",
        "h" => "fz_h",
        "l" => "fz_l",
        "c" => "fz_c",
        "v" => "fz_v",
        _ => "fz_oi",
    }
}

/// One `Sv_<NAME>` verify method for a function with an emitted C# stream.
// Integer optional-param defaults are integer-valued `f64` in the IR; the
// `as i64` casts for literal emission are exact, not truncating.
#[allow(clippy::too_many_lines, clippy::cast_possible_truncation, clippy::cognitive_complexity)]
fn emit_csharp_sv_func(
    func: &FuncDef,
    funcs: &[FuncDef],
    enums: &HashMap<String, EnumDef>,
) -> String {
    use std::fmt::Write as _;
    let base = crate::backends::common::pascal_words(&func.name);
    let base_pascal = crate::backends::common::pascal_words(&func.name);
    let class = crate::backends::csharp_stream::stream_class_name(func);
    let valty = crate::backends::csharp_stream::value_type_name(func);
    let candle = func.name.starts_with("CDL");
    let inputs = crate::streaming::input_array_names(func);
    let mut gi = 0usize;
    let arrays: Vec<&'static str> = inputs
        .iter()
        .map(|i| sv_csharp_input_array(i, &mut gi))
        .collect();
    let n_out = func.outputs.len();
    let multi = n_out > 1;
    let out_is_int: Vec<bool> = func
        .outputs
        .iter()
        .map(|o| o.param_type == crate::ir::ParamType::Integer)
        .collect();
    // `<NAME>_Value` member names: the output name with a leading `out` stripped,
    // PascalCase kept (`outSlowK` -> `SlowK`, `outMinIdx` -> `MinIdx`). Unlike
    // Java these are PROPERTIES on a readonly record struct, not accessor calls,
    // so no `()` and no null concerns.
    let vmem: Vec<String> = func
        .outputs
        .iter()
        .map(|o| crate::backends::csharp_stream::value_member_name(&o.name))
        .collect();

    // Read output `i` off an Update/Peek/Value expression. A single-output
    // function returns the scalar itself, so the read IS the expression.
    let rd_out = |v: &str, i: usize| -> String {
        if multi {
            format!("{v}.{}", vmem[i])
        } else {
            v.to_string()
        }
    };

    // RULE 1 -- THE COMPARATOR IS SELECTED PER OUTPUT TYPE, NOT PER LEG.
    //
    // INTEGER outputs compare with plain `!=` on BOTH tiers. Never cast one to
    // double: MINMAXINDEX's `<NAME>_Value` members are `int`,
    // `BitConverter.DoubleToInt64Bits` does not accept one, and a `(double)`
    // cast would compile and silently weaken a strict leg into a numeric one.
    //
    // REAL outputs pick by tier. SAME-TIER legs -- peek vs update, Value vs
    // update, cloneA vs cloneB, the int.MinValue sentinel pair -- run ONE code
    // path twice and have no licence to differ in a single bit, so they use the
    // strict `SvBne`. CROSS-TIER legs -- stream vs batch, fill vs batch -- reach
    // a zero by different but equally correct routes, so they use the
    // +/-0-tolerant `SvXtierNe`, which counts those cases as benign (#147).
    //
    // And: NEVER `==` / `Equals` on a `<NAME>_Value`. Record-struct equality
    // says `+0.0 == -0.0` and `NaN == NaN`, which makes every strict leg
    // vacuous. Java gets away with a reference-identity check on its cached
    // `Value` object; a returned record struct is COPIED into the caller's
    // frame, so there is no identity to check and C# must compare per
    // component. That is what `rd_out` above exists for.
    let same_tier_ne = |a: &str, b: &str, i: usize| -> String {
        if out_is_int[i] {
            format!("{a} != {b}")
        } else {
            format!("SvBne({a}, {b})")
        }
    };
    let xtier_ne = |a: &str, b: &str, i: usize, z: &str| -> String {
        if out_is_int[i] {
            format!("{a} != {b}")
        } else {
            format!("SvXtierNe({a}, {b}, ref {z})")
        }
    };
    // Diagnostic spelling: reals as their raw IEEE bits (a decimal rendering of
    // a 1-ULP miss is unreadable), ints as themselves.
    let diag_val = |e: &str, i: usize| -> String {
        if out_is_int[i] {
            e.to_string()
        } else {
            format!("BitConverter.DoubleToInt64Bits({e}).ToString(\"x16\")")
        }
    };

    let mut s = String::new();
    // The JsonElement parameter is deliberately NOT named `p`: the prefix sweep
    // below declares a local `int p`, and C# -- unlike Java -- rejects a local
    // that shadows an enclosing parameter (CS0136).
    let _ = writeln!(s, "    static string Sv_{}(JsonElement req) {{", func.name);
    s.push_str("        int svShape = GetInt(req, \"gen_shape\", 0);\n");
    s.push_str("        int svSeed = GetInt(req, \"gen_seed\", 0);\n");
    s.push_str("        int svN = GetInt(req, \"gen_n\", 0);\n");
    s.push_str("        if (svN < 2) svN = 2;\n        if (svN > 256) svN = 256;\n");
    s.push_str("        int svK = GetInt(req, \"unstablePeriod\", 0);\n");
    if candle {
        s.push_str("        int candleLegs = GetInt(req, \"candleLegs\", 0);\n");
    }

    // Optional params. `GetInt`/`GetDouble` already fall back to the YAML
    // default when the key is absent, so C# needs none of Java's
    // `json.contains("\"name\"")` dance. Enum params are decoded TWICE: as a raw
    // int (for R5's out-of-list probe and for `sv_reject_condition`'s guard,
    // which compares raw enum ints) and as the typed enum the API takes.
    let mut has_int_default = false;
    let mut enum_param_names: Vec<String> = Vec::new();
    for p in &func.optional_inputs {
        let name = &p.name;
        match &p.param_type {
            crate::ir::ParamType::Real => {
                let d = p.default.unwrap_or(0.0);
                // `{d:e}` renders `0.3` as `3e-1`, `2.0` as `2e0`, `-4e37`
                // verbatim -- all valid C# real literals, and the same spelling
                // the Java emitter uses.
                let _ = writeln!(
                    s,
                    "        double {name} = GetDouble(req, \"{name}\", {d:e});"
                );
            }
            crate::ir::ParamType::Enum(en) => {
                let d = p.default.unwrap_or(0.0) as i64;
                let _ = writeln!(s, "        int _raw_{name} = GetInt(req, \"{name}\", {d});");
                let _ = writeln!(s, "        {en} {name} = ({en})_raw_{name};");
                enum_param_names.push(name.clone());
            }
            _ => {
                let d = p.default.unwrap_or(0.0) as i64;
                if p.default.is_some() {
                    has_int_default = true;
                }
                let _ = writeln!(s, "        int {name} = GetInt(req, \"{name}\", {d});");
            }
        }
    }

    // Seeded inputs.
    s.push_str("        double[] fz_o = new double[svN];\n        double[] fz_h = new double[svN];\n        double[] fz_l = new double[svN];\n        double[] fz_c = new double[svN];\n        double[] fz_v = new double[svN];\n        double[] fz_oi = new double[svN];\n");
    // INTEGRATION NOTE 1: `FuzzData.FuzzGen` is the C# port of `fuzz_data.h`
    // that S2 adds as `templates/csharp/FuzzData.cs`. It does not exist yet, so
    // the class name, namespace (global, mirroring the Java port's default
    // package) and argument order are ASSUMED to mirror the Java template's
    // `FuzzData.fuzzGen(shape, seed, n, o, h, l, c, v, oi)`. Reconcile when the
    // template lands -- `HandleFuzzInHash` below makes the same call.
    s.push_str("        FuzzData.FuzzGen(svShape, svSeed, svN, fz_o, fz_h, fz_l, fz_c, fz_v, fz_oi);\n");

    // Period-bank ramp (see the C/Rust/Java emitters): the fuzz period-selector
    // series would clamp to `maxPeriod` at every bar, so every bank slot but one
    // would be vacuous. Span [min-1, max+1], fed identically to both arms.
    {
        let lookup = crate::streaming::FuncsLookup(funcs);
        if let Ok(crate::streaming::StreamPlan::PeriodBank(pb)) =
            crate::streaming::validate_streamable(func, &lookup)
        {
            if let Some(idx) = inputs.iter().position(|i| *i == pb.period_input) {
                let arr = arrays[idx];
                let _ = writeln!(
                    s,
                    "        for (int _pi = 0; _pi < svN; _pi++) {{ {arr}[_pi] = {min} + (_pi % ({max} - {min} + 3)) - 1; }}",
                    min = pb.min_param,
                    max = pb.max_param
                );
            }
        }
    }

    let full_ins = arrays.join(", ");
    // C# range slicing on an array (`a[..p]`) lowers to
    // `RuntimeHelpers.GetSubArray` and returns a FRESH `double[]` -- Java's
    // `Arrays.copyOf`, spelled shorter. Fresh is what the aliasing guards need:
    // two prefix opens never share a buffer by accident.
    let pfx_ins = |p: &str| -> String {
        arrays
            .iter()
            .map(|a| format!("{a}[..{p}]"))
            .collect::<Vec<_>>()
            .join(", ")
    };
    let opts = func
        .optional_inputs
        .iter()
        .map(|p| p.name.clone())
        .collect::<Vec<_>>()
        .join(", ");
    let opts_lead = if opts.is_empty() { String::new() } else { format!("{opts}, ") };
    let opts_tail = if opts.is_empty() { String::new() } else { format!(", {opts}") };
    let bar_args = |t: &str| -> String {
        arrays
            .iter()
            .map(|a| format!("{a}[{t}]"))
            .collect::<Vec<_>>()
            .join(", ")
    };
    let bars_t = bar_args("t");

    // Batch, fill and mutated-batch output buffers.
    let mut bdecls = String::new();
    let mut bargs = String::new();
    let mut fdecls = String::new();
    let mut fargs = String::new();
    let mut mdecls = String::new();
    let mut margs = String::new();
    for (i, is_int) in out_is_int.iter().enumerate() {
        let ty = if *is_int { "int" } else { "double" };
        let _ = writeln!(bdecls, "        {ty}[] b{i} = new {ty}[svN];");
        let _ = write!(bargs, ", b{i}");
        // Canary-filled, not zero-filled: the slack above the produced range is
        // asserted untouched after the call (#205's write bound), so a write
        // past `nb` fails instead of landing in unread space.
        let canary = if *is_int { "-987654321" } else { "-1.2345678901234e300" };
        let _ = writeln!(fdecls, "            {ty}[] f{i} = new {ty}[svN];");
        let _ = writeln!(fdecls, "            Array.Fill(f{i}, ({ty}){canary});");
        let _ = write!(fargs, ", f{i}");
        let _ = writeln!(mdecls, "                    {ty}[] m{i} = new {ty}[svN];");
        let _ = write!(margs, ", m{i}");
    }
    s.push_str(&bdecls);

    s.push_str("        long legs = 0;\n        bool allOk = true;\n        bool peekAll = true;\n        long peekReps = 0;\n        long peekRejects = 0;\n        bool peekRepAll = true;\n        int fillChecked = 0;\n        bool fillOk = true;\n        int beg = 0, nb = 0;\n        string diag = \"\";\n");
    // The range leg (#241): a handle's OutRange against what batch reported for
    // the same bars. Public API in every backend, so unlike the state leg this
    // one is not C-only.
    s.push_str("        int rangeChecked = 0;\n        bool rangeOk = true;\n        long rangeLegs = 0;\n        int rangeSites = 0;\n");
    // RULE 7 -- the benign +/-0 accumulator is a REQUEST-SCOPED LOCAL, passed by
    // `ref`. One process answers many requests and a `static` would carry one
    // function's count into the next -- and the plan's Java<->C# `benign`
    // equality check would then compare a running total against a per-request
    // one. (Java passes a one-element array only because Java has no `ref`.)
    s.push_str("        long zsign = 0;\n");
    // R4's result, reported as `updAlloc`; max over the rounds.
    s.push_str("        long updAlloc = 0;\n");
    if candle {
        // R3 bookkeeping. `zsignMut` is SEPARATE from `zsign` on purpose: the
        // mid-stream leg is a C#-only leg, and folding its benign cases into
        // `zsign` would make the reported count differ from Java's by
        // construction, breaking the cross-language equality check.
        s.push_str("        int candleMutRan = 0;\n        int candleMutMoved = 0;\n        long zsignMut = 0;\n");
    }

    // RULE 5 -- OUT-OF-LIST ENUM NEEDS A REAL REJECT CHECK.
    //
    // Java short-circuits here with "type safety IS the rejection": a value
    // outside the enum's list cannot be built, so batch and stream both reject
    // at the type level and the leg is a constant. Rust does the same through
    // `TryFrom`. Neither ports. A C# enum is int-backed and open, so
    // `(MAType)int.MinValue` is a perfectly representable value that the
    // driver's `maxList + 91` vector delivers intact to the library. The
    // rejection has to be OBSERVED, the way the C gate observes it.
    //
    // BOTH openers are probed, not just `Open`: `OpenAndFill` validates through
    // its own wrapper and has its own reject path.
    //
    // `c0` is a plain default Core on purpose -- a parameter-domain rejection
    // does not depend on unstable periods or candle settings, and the round
    // loop's `c2` does not exist yet at this point.
    if !enum_param_names.is_empty() {
        let cond = enum_param_names
            .iter()
            .map(|n| format!("!Enum.IsDefined({n})"))
            .collect::<Vec<_>>()
            .join(" || ");
        s.push_str("        Core c0 = new Core();\n");
        // `Enum.IsDefined<TEnum>(TEnum)` (the generic overload) is inferred from
        // the argument -- non-boxing and trim/AOT-safe, unlike the legacy
        // `Enum.IsDefined(typeof(T), (object)v)`.
        let _ = writeln!(s, "        if ({cond}) {{");
        s.push_str("            bool eOpen, eFill;\n");
        let _ = writeln!(
            s,
            "            try {{ _ = c0.{base_pascal}Open({full_ins}{opts_tail}); eOpen = false; }}"
        );
        s.push_str("            catch (ArgumentException) { eOpen = true; }\n");
        s.push_str(&fdecls);
        let _ = writeln!(
            s,
            "            try {{ _ = c0.{base_pascal}OpenAndFill({full_ins}{opts_tail}{fargs}); eFill = false; }}"
        );
        s.push_str("            catch (ArgumentException) { eFill = true; }\n");
        s.push_str("            bool eOk = eOpen && eFill;\n");
        // legs:0, exactly like Java's short-circuit answer, so the
        // cross-language `legs` equality holds for this vector too. `ok` is
        // COMPUTED, never the literal 1 Java can afford.
        s.push_str("            return \"{\\\"retCode\\\":2,\\\"legs\\\":0,\\\"nb\\\":0,\\\"openRejects\\\":\" + (eOk ? 1 : 0) + \",\\\"enumRejects\\\":\" + ((eOpen ? 1 : 0) + (eFill ? 1 : 0)) + \",\\\"ok\\\":\" + (eOk ? 1 : 0) + \",\\\"peek_ok\\\":1}\";\n");
        s.push_str("        }\n");
    }

    if candle {
        s.push_str("        int rounds = (candleLegs != 0) ? 4 : 1;\n");
    } else {
        s.push_str("        int rounds = 1;\n");
    }
    s.push_str("        for (int rd = 0; rd < rounds; rd++) {\n");

    // Fresh, pinned, configured Core for this round. Built through the SHIPPED
    // CoreBuilder rather than by reaching into the instance's fields, so the
    // validation exercised is the library's own; `Build()` snapshots, so the
    // Core never aliases the builder.
    s.push_str("            CoreBuilder cb = Core.Builder();\n");
    for id in collect_pin_ids(func, funcs, enums) {
        let _ = writeln!(s, "            cb = cb.UnstablePeriod((FuncUnstId){id}, svK);");
    }
    if candle {
        s.push_str("            cb = SvApplyCandleRound(cb, rd);\n");
    }
    // Reported, never thrown: this runs in a subprocess ta_regtest drives over a
    // pipe, so an escaping exception surfaces as a dead pipe instead of a
    // diagnosable answer.
    s.push_str("            Core c2;\n");
    s.push_str("            try { c2 = cb.Build(); }\n");
    s.push_str("            catch (ArgumentOutOfRangeException) {\n");
    s.push_str("                return \"{\\\"error\\\":\\\"unstablePeriod out of range\\\"}\";\n");
    s.push_str("            }\n");

    // Expected-reject precheck (dispatch/period-bank arms with no stream) --
    // live since #139: HMA has no stream yet, so every MAType-dispatching
    // function (MA, BBANDS, APO/PPO/PVO, STOCH*, MACDEXT, MAVP) generates a
    // guard for the HMA arm. The guard compares raw enum ints, so C enum
    // constants are substituted with their values and enum params rewritten to
    // their `_raw_` locals -- the same two rewrites the Java emitter does.
    if let Some(guard) = sv_reject_condition(func, funcs, None) {
        let mut guard_cs = sv_guard_enum_ints(&guard, enums);
        for p in &func.optional_inputs {
            if matches!(p.param_type, crate::ir::ParamType::Enum(_)) {
                guard_cs = guard_cs.replace(&p.name, &format!("_raw_{}", p.name));
            }
        }
        let _ = writeln!(s, "            if ({guard_cs}) {{");
        s.push_str("                bool r1, r2;\n");
        let _ = writeln!(
            s,
            "                try {{ _ = c2.{base_pascal}Open({full_ins}{opts_tail}); r1 = false; }}"
        );
        s.push_str("                catch (ArgumentException) { r1 = true; }\n");
        s.push_str(&fdecls.replace("            ", "                "));
        let _ = writeln!(
            s,
            "                try {{ _ = c2.{base_pascal}OpenAndFill({full_ins}{opts_tail}{fargs}); r2 = false; }}"
        );
        s.push_str("                catch (ArgumentException) { r2 = true; }\n");
        s.push_str("                bool okr = r1 && r2;\n");
        s.push_str("                return \"{\\\"retCode\\\":0,\\\"legs\\\":0,\\\"unsupportedArm\\\":1,\\\"ok\\\":\" + (okr ? 1 : 0) + \",\\\"peek_ok\\\":1}\";\n");
        s.push_str("            }\n");
    }

    // ---- batch leg ----
    // `c2.<NAME>(...)` with `out` args and output arrays binds the INTERNAL
    // RetCode overload, not the public `OutRange` one: the gate needs the return
    // code, including the ones the public surface converts into throws.
    let _ = writeln!(
        s,
        "            RetCode rc;\n            try {{ rc = c2.{base}Impl(0, svN - 1, {full_ins}, {opts_lead}out beg, out nb{bargs}); }}\n            catch (Exception _sve) when (_sve is ITALibFailure) {{ rc = ((ITALibFailure)_sve).RetCode; beg = 0; nb = 0; }}"
    );
    let _ = writeln!(s, "            int lb = c2.{base}Lookback({opts});");
    s.push_str("            if (rc != RetCode.Success || nb == 0) {\n");
    // Reject parity: whenever the batch produced nothing -- an error (bad
    // params, an out-of-list enum reaching a dispatch default arm) or an empty
    // range -- Open must reject too. Open mirrors the batch validation and the
    // min-history rule by construction, so a stream that opens where batch fails
    // is always a contract break.
    s.push_str("                bool openRejects;\n");
    let _ = writeln!(
        s,
        "                try {{ _ = c2.{base_pascal}Open({full_ins}{opts_tail}); openRejects = false; }}"
    );
    s.push_str("                catch (ArgumentException) { openRejects = true; }\n");
    if candle {
        s.push_str("                if (!openRejects) allOk = false;\n");
        // A failed round must not truncate the sweep.
        s.push_str("                if (rd + 1 < rounds) continue;\n");
        s.push_str("                return \"{\\\"retCode\\\":\" + (int)rc + \",\\\"legs\\\":\" + legs + \",\\\"nb\\\":\" + nb + \",\\\"openRejects\\\":\" + (openRejects ? 1 : 0) + \",\\\"ok\\\":\" + (allOk ? 1 : 0) + \",\\\"peek_ok\\\":\" + (peekAll ? 1 : 0) + \",\\\"peek_reps\\\":\" + peekReps + \",\\\"peek_rep_ok\\\":\" + (peekRepAll ? 1 : 0) + \",\\\"peek_rejects\\\":\" + peekRejects + \",\\\"benign\\\":\" + zsign + \"}\";\n");
    } else {
        s.push_str("                return \"{\\\"retCode\\\":\" + (int)rc + \",\\\"legs\\\":0,\\\"nb\\\":\" + nb + \",\\\"openRejects\\\":\" + (openRejects ? 1 : 0) + \",\\\"ok\\\":\" + (openRejects ? 1 : 0) + \",\\\"peek_ok\\\":1}\";\n");
    }
    s.push_str("            }\n");

    // ---- OpenAndFill leg: the filled array == batch(0, n-1), bitwise ----
    s.push_str("            fillChecked = 1;\n            try {\n");
    s.push_str(&fdecls.replace("            ", "                "));
    let _ = writeln!(
        s,
        "                Core.{class} _fh = c2.{base_pascal}OpenAndFill({full_ins}{opts_tail}{fargs});"
    );
    // `OutRange` is a PROPERTY on the C# handle (Java spells it `outRange()`),
    // returning the shipped `OutRange` with `BegIdx` / `Count`.
    s.push_str("                OutRange _fr = _fh.OutRange;\n");
    let _ = writeln!(s, "                rangeChecked = 1; rangeLegs++; rangeSites |= {};", sv_range_bit(SvRangeSite::Fill, SV_RANGE_MASK_CSHARP));
    s.push_str("                if (_fr.BegIdx != beg || _fr.Count != nb) rangeOk = false;\n");
    s.push_str("                if (_fr.BegIdx != beg || _fr.Count != nb) fillOk = false;\n                else {\n");
    for i in 0..n_out {
        let cmp = xtier_ne(&format!("f{i}[bi]"), &format!("b{i}[bi]"), i, "zsign");
        let _ = writeln!(
            s,
            "                    for (int bi = 0; bi < nb; bi++) if ({cmp}) fillOk = false;"
        );
    }
    // Slack canary: everything above the produced range must be untouched.
    for (i, is_int) in out_is_int.iter().enumerate() {
        let ty = if *is_int { "int" } else { "double" };
        let canary = if *is_int { "-987654321" } else { "-1.2345678901234e300" };
        let _ = writeln!(
            s,
            "                    for (int bi = nb; bi < svN; bi++) if (f{i}[bi] != ({ty}){canary}) fillOk = false;"
        );
    }
    s.push_str("                }\n");

    // RULE 2 -- FULL ALIASING CROSS PRODUCT.
    //
    // Java probes exactly two pairs: output0 == input0 and output1 == output0.
    // That leaves most of the surface untested, and it matters most in the
    // composed tier, where `fill_scratch_may_alias_output` deliberately makes
    // `sc_<out>` alias the CALLER'S array for eight functions -- and where the
    // C# failure mode is a wrong VALUE, not an exception, so nothing else would
    // notice.
    //
    // So: every real output i x every distinct input array, plus every
    // same-typed output pair i<j. Each must throw ArgumentException and mint no
    // handle (a throw is the only way not to return one). O(9) probes for the
    // widest function.
    //
    // The input arrays are passed BY REFERENCE, deliberately -- a defensive copy
    // would make `ReferenceEquals` false and the probe vacuous. A missing guard
    // therefore writes into the fuzz series and corrupts the legs after it; that
    // is acceptable because a missing guard already sets `fillOk = false` right
    // here, so the run reds either way, and it reds louder.
    s.push_str("                /* R2: aliasing cross product -- every real output x every input,\n");
    s.push_str("                   then every same-typed output pair. Each must throw. */\n");
    for (i, i_is_int) in out_is_int.iter().enumerate() {
        if *i_is_int {
            continue; // an int[] output slot cannot take a double[] input
        }
        // Several input positions can map to the same fuzz array (generic
        // real1/real2 both land on fz_v), and two identical probes prove nothing
        // twice.
        let mut seen: std::collections::BTreeSet<&str> = std::collections::BTreeSet::new();
        for (j, arr) in arrays.iter().enumerate() {
            if !seen.insert(*arr) {
                continue;
            }
            let mut aargs = String::new();
            for k in 0..n_out {
                if k == i {
                    let _ = write!(aargs, ", {arr}");
                } else {
                    let _ = write!(aargs, ", f{k}");
                }
            }
            let _ = writeln!(
                s,
                "                try {{ _ = c2.{base_pascal}OpenAndFill({full_ins}{opts_tail}{aargs}); fillOk = false; }}"
            );
            let _ = writeln!(
                s,
                "                catch (ArgumentException) {{ /* expected: output {i} aliases input {} */ }}",
                inputs[j]
            );
        }
    }
    for (i, i_is_int) in out_is_int.iter().enumerate() {
        for (j, j_is_int) in out_is_int.iter().enumerate().skip(i + 1) {
            if i_is_int != j_is_int {
                // Expressible — `MemoryMarshal.Cast` lays a `Span<int>` over a
                // `Span<double>` — but not probed here: SUPERTREND is the
                // corpus's only mixed-type pair and its own suite covers it.
                continue;
            }
            let mut aargs = String::new();
            for k in 0..n_out {
                if k == j {
                    let _ = write!(aargs, ", f{i}");
                } else {
                    let _ = write!(aargs, ", f{k}");
                }
            }
            let _ = writeln!(
                s,
                "                try {{ _ = c2.{base_pascal}OpenAndFill({full_ins}{opts_tail}{aargs}); fillOk = false; }}"
            );
            let _ = writeln!(
                s,
                "                catch (ArgumentException) {{ /* expected: output {j} aliases output {i} */ }}"
            );
        }
    }
    // R2b: PARTIAL overlap, the case only spans can express and the one that
    // distinguishes `Overlaps` from reference identity.
    //
    // Every probe above passes a WHOLE, identical buffer, where `==`,
    // `ReferenceEquals` and `Overlaps` all fire alike — so those probes cannot
    // tell a correct guard from the pre-span one. Reverting `alias_reject` to
    // identity left the whole gate green while multi-output functions silently
    // returned Success with every value wrong. These probes are what make the
    // overlap guard a checked property rather than a claim.
    //
    // Two shapes per pair, because they fail differently:
    //   - offset:  a window and the same window shifted one element in
    //   - same start, different length: identical memory and start, which span
    //     `==` reads as NOT equal, so an `==`-based guard waves it through
    //
    // EVERY window is `svN` long or longer, cut from a buffer one element wider
    // than the series. Slicing `f{i}` itself cannot do that: the widest window
    // inside it that still leaves room to shift is `svN - 1`, which rule S5
    // rejects for capacity the moment the lookback is 0 — so for the 28
    // unconditional-zero-lookback functions, and every period-taking one at
    // `period = 1`, the probe caught a capacity fault and never reached the
    // overlap guard it is named for (issue #271 item 2).
    if n_out >= 1 {
        let pair = |ints: bool| {
            out_is_int.iter().enumerate().any(|(i, a)| {
                *a == ints
                    && out_is_int.iter().skip(i + 1).any(|b| *b == ints)
            })
        };
        if pair(false) {
            s.push_str("                double[] ovD = new double[svN + 1];\n");
        }
        if pair(true) {
            s.push_str("                int[] ovI = new int[svN + 1];\n");
        }
        if out_is_int.iter().any(|b| !*b) {
            if let Some(arr) = arrays.first() {
                // The input leg needs the OUTPUT to overlap an INPUT, so the
                // input has to come out of the wide buffer too — same values,
                // one element of headroom.
                let _ = writeln!(s, "                double[] ovIn = new double[svN + 1];");
                let _ = writeln!(s, "                Array.Copy({arr}, ovIn, svN);");
            }
        }
        s.push_str("                /* R2b: PARTIAL overlap -- only spans can express it, and it is
");
        s.push_str("                   the only shape that separates Overlaps from identity. */
");
        for (i, i_is_int) in out_is_int.iter().enumerate() {
            for (j, j_is_int) in out_is_int.iter().enumerate().skip(i + 1) {
                if i_is_int != j_is_int {
                    continue;
                }
                let ov = if *i_is_int { "ovI" } else { "ovD" };
                for (shape, expr) in [
                    ("offset", format!("{ov}.AsSpan(1, svN)")),
                    ("same start, longer", format!("{ov}.AsSpan(0, svN + 1)")),
                ] {
                    let mut aargs = String::new();
                    for k in 0..n_out {
                        if k == j {
                            let _ = write!(aargs, ", {expr}");
                        } else if k == i {
                            let _ = write!(aargs, ", {ov}.AsSpan(0, svN)");
                        } else {
                            let _ = write!(aargs, ", f{k}");
                        }
                    }
                    let _ = writeln!(
                        s,
                        "                try {{ _ = c2.{base_pascal}OpenAndFill({full_ins}{opts_tail}{aargs}); fillOk = false; }}"
                    );
                    let _ = writeln!(
                        s,
                        "                catch (ArgumentException) {{ /* expected: outputs {i}/{j} partially overlap ({shape}) */ }}"
                    );
                }
            }
        }
        // Output partially overlapping an INPUT. Whole-buffer in-place is
        // legitimate and stays accepted; only the partial case is a reject, so
        // this probe is the one that pins that distinction.
        for (i, i_is_int) in out_is_int.iter().enumerate() {
            if *i_is_int {
                continue;
            }
            if !arrays.is_empty() {
                let mut aargs = String::new();
                for k in 0..n_out {
                    if k == i {
                        let _ = write!(aargs, ", ovIn.AsSpan(1, svN)");
                    } else {
                        let _ = write!(aargs, ", f{k}");
                    }
                }
                // The history comes out of `ovIn` so the output window above
                // overlaps it; every other input stays the fuzz array, and the
                // two agree in length (rule S5's input half).
                let ov_ins = arrays
                    .iter()
                    .enumerate()
                    .map(|(k, a)| {
                        if k == 0 { "ovIn.AsSpan(0, svN)".to_string() } else { (*a).to_string() }
                    })
                    .collect::<Vec<_>>()
                    .join(", ");
                let _ = writeln!(
                    s,
                    "                try {{ _ = c2.{base_pascal}OpenAndFill({ov_ins}{opts_tail}{aargs}); fillOk = false; }}"
                );
                let _ = writeln!(
                    s,
                    "                catch (ArgumentException) {{ /* expected: output {i} partially overlaps an input */ }}"
                );
            }
        }
    }
    s.push_str("            } catch (ArgumentException) { fillOk = false; }\n");

    // ---- prefix sweep: the trajectory, bit-exact against batch ----
    s.push_str("            int[] pcs = { lb + 1, lb + 13, svN / 2, svN - 1 };\n");
    s.push_str("            Array.Sort(pcs);\n");
    s.push_str("            int prevP = -1;\n");
    s.push_str("            for (int pi = 0; pi < pcs.Length; pi++) {\n");
    s.push_str("                int p = pcs[pi];\n");
    s.push_str("                if (p < lb + 1 || p > svN - 1 || p == prevP) continue;\n");
    s.push_str("                prevP = p;\n");
    let _ = writeln!(s, "                Core.{class} st;");
    let _ = writeln!(
        s,
        "                try {{ st = c2.{base_pascal}Open({}{opts_tail}); }}",
        pfx_ins("p")
    );
    s.push_str("                catch (ArgumentException) { allOk = false; if (diag.Length == 0) diag = \",\\\"openRejectP\\\":\" + p; continue; }\n");
    // THE ONLY `legs++` IN THIS FILE. See the header note: the plan compares
    // this count against the Java row for equality, so every C#-only leg below
    // reports through its own counter instead.
    s.push_str("                legs++;\n");
    // Open-value compare, through `Value`. Load-bearing: Open returns only the
    // handle, so this anchor compare IS the `Value` verification at open.
    let up_ty = if multi {
        format!("Core.{valty}")
    } else if out_is_int[0] {
        "int".to_string()
    } else {
        "double".to_string()
    };
    let _ = writeln!(s, "                {up_ty} v0 = st.Value;");
    for i in 0..n_out {
        let cmp = xtier_ne(&rd_out("v0", i), &format!("b{i}[p - 1 - beg]"), i, "zsign");
        let _ = writeln!(
            s,
            "                if ({cmp}) {{ allOk = false; if (diag.Length == 0) diag = \",\\\"badBar\\\":\" + (p - 1) + \",\\\"badOut\\\":{i},\\\"where\\\":\\\"open\\\"\"; }}"
        );
    }

    // Update loop: peek-every-7 (same-tier) + Value == update (same-tier, per
    // component) + update vs batch (cross-tier).
    let emit_up_compares = |s: &mut String, pad: &str| {
        for i in 0..n_out {
            let a = rd_out("up", i);
            let b = format!("b{i}[t - beg]");
            let cmp = xtier_ne(&a, &b, i, "zsign");
            let bv = diag_val(&b, i);
            let sv = diag_val(&a, i);
            let _ = writeln!(
                s,
                "{pad}if ({cmp}) {{ allOk = false; if (diag.Length == 0) diag = \",\\\"badBar\\\":\" + t + \",\\\"badOut\\\":{i},\\\"batchv\\\":\\\"\" + {bv} + \"\\\",\\\"streamv\\\":\\\"\" + {sv} + \"\\\"\"; }}"
            );
        }
    };
    s.push_str("                for (int t = p; t < svN; t++) {\n");
    // A Peek may refuse the bar -- see `emit_sv_peek_repeat_probe`. Refusals are
    // counted, and every comparison that would read an unassigned result is
    // guarded, because a refused Peek returns nothing.
    s.push_str("                    bool pkTook = true;\n");
    let _ = writeln!(s, "                    {up_ty} pk = default;");
    let _ = writeln!(s, "                    try {{ pk = st.Peek({bars_t}); }} catch (ArgumentException) {{ pkTook = false; peekRejects++; }}");
    // The repeat probe — see `emit_sv_peek_repeat_probe` for why the decoy in
    // the middle is what makes it see anything.
    s.push_str("                    if (t % 7 == 0) {\n");
    s.push_str("                        bool rpTook = pkTook;\n");
    let _ = writeln!(s, "                        try {{ _ = st.Peek({}); }} catch (ArgumentException) {{ peekRejects++; }}", bar_args("t - 1"));
    let _ = writeln!(s, "                        {up_ty} rp = default;");
    let _ = writeln!(s, "                        try {{ rp = st.Peek({bars_t}); }} catch (ArgumentException) {{ rpTook = false; }}");
    s.push_str("                        if (rpTook) {\n");
    s.push_str("                            peekReps++;\n");
    for i in 0..n_out {
        let cmp = same_tier_ne(&rd_out("rp", i), &rd_out("pk", i), i);
        let _ = writeln!(s, "                            if ({cmp}) peekRepAll = false;");
    }
    s.push_str("                        } else { peekRejects++; }\n");
    s.push_str("                    }\n");
    let _ = writeln!(s, "                    {up_ty} up = st.Update({bars_t});");
    for i in 0..n_out {
        let cmp = same_tier_ne(&rd_out("pk", i), &rd_out("up", i), i);
        let _ = writeln!(s, "                    if (pkTook && ({cmp})) peekAll = false;");
    }
    // `Value` == the value just returned, read AFTER an intervening `Peek`.
    //
    // The intervening peek is the whole leg. Without it this compares
    // `Update`'s return against a `Value` read with nothing in between, and
    // both render from the same generator expression over the same fields --
    // literally `new X_Value(cur_a, cur_b) != new X_Value(cur_a, cur_b)`. That
    // cannot fail, and it did not: it passed unchanged while the guard it was
    // meant to protect was reverted. The Java leg carries the same intervening
    // peek for the same reason.
    //
    // Peeking a DIFFERENT bar (t-1, always in range since t >= lb+1 >= 1) makes
    // it a real check of the documented contract: `Value` is a pure read that
    // `Peek` does not disturb. A `Peek` that commits advances the handle, and
    // `Value` then reports the peeked bar instead of the committed one. On
    // FUZZ_CONSTANT the two bars carry the same number, so there the leg leans
    // on state advancement rather than on a value difference -- which is why it
    // is a complement to `peek_ok`, not a replacement for it.
    //
    // Comparison is per component and strict: record-struct `==` would call
    // +0.0 equal to -0.0 and NaN equal to NaN, i.e. would pass on exactly the
    // corruption this leg exists to find.
    let _ = writeln!(s, "                    try {{ _ = st.Peek({}); }} catch (ArgumentException) {{ peekRejects++; }}", bar_args("t - 1"));
    let _ = writeln!(s, "                    {up_ty} vc = st.Value;");
    for i in 0..n_out {
        let cmp = same_tier_ne(&rd_out("vc", i), &rd_out("up", i), i);
        let _ = writeln!(
            s,
            "                    if ({cmp}) {{ allOk = false; if (diag.Length == 0) diag = \",\\\"valueNeUpdate\\\":\" + t; }}"
        );
    }
    emit_up_compares(&mut s, "                    ");
    s.push_str("                }\n");
    // Open(p) + (svN - p) updates: whatever p was, the handle has consumed svN
    // bars and must report exactly what batch(0, svN-1) did. Only when the value
    // leg passed — otherwise the handle is short of the bars it was to consume.
    s.push_str("                if (allOk) {\n");
    let _ = writeln!(s, "                    rangeChecked = 1; rangeLegs++; rangeSites |= {};", sv_range_bit(SvRangeSite::Prefix, SV_RANGE_MASK_CSHARP));
    s.push_str("                    if (st.OutRange.BegIdx != beg || st.OutRange.Count != nb) rangeOk = false;\n");
    // One Advance(), last on this handle -- see the C server for why.
    let _ = writeln!(s, "                    rangeLegs++; rangeSites |= {};", sv_range_bit(SvRangeSite::Advance, SV_RANGE_MASK_CSHARP));
    s.push_str("                    st.Advance();\n");
    s.push_str("                    if (st.OutRange.BegIdx != beg || st.OutRange.Count != nb + 1) rangeOk = false;\n");
    s.push_str("                }\n");
    s.push_str("            }\n");


    // ---- Clone() independence: open at the earliest prefix, advance to mid,
    // clone, drive both to the end. Both must match batch (cross-tier) and each
    // other (same-tier), and both must report the batch range (#287: a clone
    // that carries every numeric field but drops the range pair produced
    // identical values and was invisible here).
    // `Clone()` is C#'s spelling of Java's `copy()`.
    s.push_str("            {\n");
    s.push_str("                int p0 = lb + 1;\n");
    s.push_str("                if (p0 <= svN - 1) {\n");
    s.push_str("                    try {\n");
    let _ = writeln!(
        s,
        "                        Core.{class} sA = c2.{base_pascal}Open({}{opts_tail});",
        pfx_ins("p0")
    );
    s.push_str("                        int mid = (p0 + svN) / 2;\n");
    let _ = writeln!(
        s,
        "                        for (int t = p0; t < mid; t++) sA.Update({bars_t});"
    );
    let _ = writeln!(s, "                        Core.{class} sB = sA.Clone();");
    s.push_str("                        for (int t = mid; t < svN; t++) {\n");
    let _ = writeln!(s, "                            {up_ty} uA = sA.Update({bars_t});");
    let _ = writeln!(s, "                            {up_ty} uB = sB.Update({bars_t});");
    for i in 0..n_out {
        let same = same_tier_ne(&rd_out("uA", i), &rd_out("uB", i), i);
        let cross = xtier_ne(&rd_out("uA", i), &format!("b{i}[t - beg]"), i, "zsign");
        let _ = writeln!(
            s,
            "                            if ({same} || {cross}) {{ allOk = false; if (diag.Length == 0) diag = \",\\\"copyDiverged\\\":\" + t; }}"
        );
    }
    s.push_str("                        }\n");
    // Both handles have now consumed bars [p0-1, svN-1] — the fork's own
    // updates carried it over exactly the bars the original took — so each
    // must report what batch(0, svN-1) did, the same claim the prefix leg
    // makes about the handle it never cloned. The original is the control: it
    // is the prefix leg's shape, so a failure on sA alone says the leg's own
    // bookkeeping broke rather than Clone().
    // Only when the value leg passed: a diverged handle is not one whose range
    // is worth reading.
    s.push_str("                        if (allOk) {\n");
    let _ = writeln!(
        s,
        "                            rangeChecked = 1; rangeLegs++; rangeSites |= {};",
        sv_range_bit(SvRangeSite::Copy, SV_RANGE_MASK_CSHARP)
    );
    s.push_str("                            if (sA.OutRange.BegIdx != beg || sA.OutRange.Count != nb) { rangeOk = false; if (diag.Length == 0) diag = \",\\\"copyRangeSrc\\\":1\"; }\n");
    s.push_str("                            if (sB.OutRange.BegIdx != beg || sB.OutRange.Count != nb) { rangeOk = false; if (diag.Length == 0) diag = \",\\\"copyRange\\\":1\"; }\n");
    s.push_str("                        }\n");
    s.push_str("                    } catch (ArgumentException) { allOk = false; if (diag.Length == 0) diag = \",\\\"copyOpenReject\\\":1\"; }\n");
    s.push_str("                }\n");
    s.push_str("            }\n");

    // RULE 4 -- THE ALLOCATION PROBE, RESTRUCTURED.
    //
    // A separate loop, containing ONLY Update. Deliberately NOT folded into the
    // trajectory sweep: that loop Peeks every 7th bar, and Peek allocates a
    // scratch handle on the ~83 functions where the [ThreadStatic] scratch is
    // not elected, so a folded probe would red for a reason that is not a
    // defect.
    //
    // No warm-up pass is emitted: the sweep above has already JIT-compiled every
    // step and its callees on this thread. The measured handle is FRESH on
    // purpose -- a handle that allocates lazily on its first Update is a real
    // defect and this probe is meant to see it.
    //
    // `sink` accumulates ONE component and is consumed into a static field AFTER
    // the measured region, so the JIT cannot prove the Update results dead and
    // delete the calls. Nothing inside the region boxes the returned value (32
    // bytes per bar, measured).
    let sink_ty = if out_is_int[0] { "long" } else { "double" };
    let sink_zero = if out_is_int[0] { "0L" } else { "0.0" };
    s.push_str("            {\n");
    s.push_str("                int pa = lb + 1;\n");
    s.push_str("                if (pa <= svN - 1) {\n");
    s.push_str("                    try {\n");
    let _ = writeln!(
        s,
        "                        Core.{class} sQ = c2.{base_pascal}Open({}{opts_tail});",
        pfx_ins("pa")
    );
    let _ = writeln!(s, "                        {sink_ty} sink = {sink_zero};");
    s.push_str("                        long a0 = GC.GetAllocatedBytesForCurrentThread();\n");
    s.push_str("                        for (int t = pa; t < svN; t++) {\n");
    let _ = writeln!(s, "                            {up_ty} uq = sQ.Update({bars_t});");
    let _ = writeln!(s, "                            sink += {};", rd_out("uq", 0));
    s.push_str("                        }\n");
    s.push_str("                        long ad = GC.GetAllocatedBytesForCurrentThread() - a0;\n");
    s.push_str("                        svUpdSink += sink;\n");
    s.push_str("                        if (ad > updAlloc) updAlloc = ad;\n");
    s.push_str("                        if (ad != 0) { allOk = false; if (diag.Length == 0) diag = \",\\\"updAllocBytes\\\":\" + ad; }\n");
    s.push_str("                    } catch (ArgumentException) { /* open rejects here -- nothing to measure */ }\n");
    s.push_str("                }\n");
    s.push_str("            }\n");

    // RULE 3 -- A REAL MID-STREAM CANDLE-SETTINGS LEG.
    //
    // The four rounds above do NOT test the open-time snapshot at all: each
    // builds a fresh Core and opens on it, so the step's snapshot and the Core's
    // live settings hold the same value and a step reading the wrong one is
    // invisible. Keep the rounds as the open-time check; do not claim more.
    //
    // This leg opens under the round's settings, then OVERWRITES the settings
    // the step reads, then drives the remaining Update bars and requires them to
    // still match the PRE-mutation batch. It reds if and only if the step reads
    // live settings instead of its open-time snapshot.
    //
    // The settings are saved before the leg and restored in a `finally`, so
    // every later leg in the round still runs under the round's settings
    // whatever happens here -- which is what keeps this leg order-independent.
    if candle {
        s.push_str("            {\n");
        s.push_str("                int pc = lb + 1;\n");
        s.push_str("                if (pc <= svN - 1) {\n");
        s.push_str("                    CandleSetting[] svSaved = (CandleSetting[])c2.candleSettings.Clone();\n");
        s.push_str(&mdecls);
        s.push_str("                    try {\n");
        let _ = writeln!(
            s,
            "                        Core.{class} sC = c2.{base_pascal}Open({}{opts_tail});",
            pfx_ins("pc")
        );
        s.push_str("                        SvMutateLiveCandles(c2);\n");
        s.push_str("                        candleMutRan = 1;\n");
        // Non-vacuity witness. Re-run the BATCH under the mutated settings: if it
        // answers the same as the unmutated batch then a step reading live
        // settings would match too, and this leg proves nothing for this vector.
        // Reported, never failed -- a pattern that never fires on this shape
        // legitimately answers all-zero either way, so the S3 gate asserts
        // `candleMutMoved` rather than this leg guessing.
        s.push_str("                        int mBeg = 0, mNb = 0;\n");
        let _ = writeln!(
            s,
            "                        RetCode mrc;\n                        try {{ mrc = c2.{base}Impl(0, svN - 1, {full_ins}, {opts_lead}out mBeg, out mNb{margs}); }}\n                        catch (Exception _mve) when (_mve is ITALibFailure) {{ mrc = ((ITALibFailure)_mve).RetCode; mBeg = 0; mNb = 0; }}"
        );
        s.push_str("                        if (mrc != RetCode.Success || mNb != nb || mBeg != beg) candleMutMoved = 1;\n");
        s.push_str("                        else {\n");
        for i in 0..n_out {
            let _ = writeln!(
                s,
                "                            for (int bi = 0; bi < nb; bi++) if (m{i}[bi] != b{i}[bi]) candleMutMoved = 1;"
            );
        }
        s.push_str("                        }\n");
        s.push_str("                        for (int t = pc; t < svN; t++) {\n");
        let _ = writeln!(s, "                            {up_ty} uC = sC.Update({bars_t});");
        for i in 0..n_out {
            let cmp = xtier_ne(&rd_out("uC", i), &format!("b{i}[t - beg]"), i, "zsignMut");
            let _ = writeln!(
                s,
                "                            if ({cmp}) {{ allOk = false; if (diag.Length == 0) diag = \",\\\"candleSnapshotLeaked\\\":\" + t + \",\\\"badOut\\\":{i}\"; }}"
            );
        }
        s.push_str("                        }\n");
        s.push_str("                    } catch (ArgumentException) { allOk = false; if (diag.Length == 0) diag = \",\\\"candleMutOpenReject\\\":1\"; }\n");
        // The ONE place a broader catch is right, and only because of what this
        // leg does: it deliberately drives the library into a state the contract
        // says the step must ignore. A step that does NOT ignore it can index a
        // ring sized from the open-time avgPeriod with a live-settings index and
        // throw IndexOutOfRange -- which, uncaught, kills the process and the
        // driver reports a pipe failure naming nothing. Converting it into a
        // named red is strictly more informative and cannot mask anything else:
        // no other leg runs inside this try.
        s.push_str("                    catch (IndexOutOfRangeException) { allOk = false; if (diag.Length == 0) diag = \",\\\"candleSnapshotLeakedThrow\\\":1\"; }\n");
        s.push_str("                    finally { SvRestoreLiveCandles(c2, svSaved); }\n");
        s.push_str("                }\n");
        s.push_str("            }\n");
    }

    // ---- short-history reject: at exactly `lb` bars no output is defined for
    // ANY configuration, so Open must reject -- and with the TYPED exception,
    // because `InsufficientHistoryException` is the one routine, data-dependent
    // failure a caller is meant to catch separately from a programming error.
    // The derived catch must come first (C# rejects the other order, CS0160),
    // which is also what makes the "wrong type" arm reachable.
    s.push_str("            if (lb >= 1 && lb < svN) {\n");
    let _ = writeln!(
        s,
        "                try {{ _ = c2.{base_pascal}Open({}{opts_tail}); allOk = false; if (diag.Length == 0) diag = \",\\\"shortHistoryAccepted\\\":1\"; }}",
        pfx_ins("lb")
    );
    s.push_str("                catch (InsufficientHistoryException) { /* expected, typed */ }\n");
    s.push_str("                catch (ArgumentException) { allOk = false; if (diag.Length == 0) diag = \",\\\"shortHistoryWrongType\\\":1\"; }\n");
    s.push_str("            }\n");

    // ---- int.MinValue default-sentinel pair: Open(int.MinValue) must equal
    // Open(the explicit YAML default) BITWISE. The batch guard transcribes into
    // the stream open, so defaulting can never silently diverge. SAME-TIER: one
    // code path twice, no licence to differ.
    if has_int_default {
        let mut sent_args: Vec<String> = Vec::new();
        let mut expl_args: Vec<String> = Vec::new();
        for p in &func.optional_inputs {
            match &p.param_type {
                crate::ir::ParamType::Integer if p.default.is_some() => {
                    let d = p.default.unwrap_or(0.0) as i64;
                    sent_args.push("int.MinValue".to_string());
                    expl_args.push(format!("{d}"));
                }
                _ => {
                    sent_args.push(p.name.clone());
                    expl_args.push(p.name.clone());
                }
            }
        }
        s.push_str("            try {\n");
        let _ = writeln!(
            s,
            "                Core.{class} sD = c2.{base_pascal}Open({full_ins}, {});",
            sent_args.join(", ")
        );
        let _ = writeln!(
            s,
            "                Core.{class} sE = c2.{base_pascal}Open({full_ins}, {});",
            expl_args.join(", ")
        );
        let _ = writeln!(s, "                {up_ty} vD = sD.Value;");
        let _ = writeln!(s, "                {up_ty} vE = sE.Value;");
        for i in 0..n_out {
            let cmp = same_tier_ne(&rd_out("vD", i), &rd_out("vE", i), i);
            let _ = writeln!(
                s,
                "                if ({cmp}) {{ allOk = false; if (diag.Length == 0) diag = \",\\\"minValueDefault\\\":1\"; }}"
            );
        }
        s.push_str("            } catch (ArgumentException) { /* defaults need more history than svN -- skip */ }\n");
    }

    // startIdx-anchored range site (#241) — the C leg's twin. `_OpenInternal`'s
    // range is max(startIdx, lookback), resolved by a different emitter branch
    // from the two sites above, and it was gated in C alone.
    s.push_str("            {\n");
    let anchored_bit = sv_range_bit(SvRangeSite::Anchored, SV_RANGE_MASK_CSHARP);
    s.push_str("                int Sidx = lb + (svN - lb) / 3;\n");
    s.push_str("                if (Sidx > lb && Sidx < svN - 1) {\n");
    s.push_str("                    int begS = 0, nbS = 0;\n");
    s.push_str("                    RetCode rcS;\n");
    let _ = writeln!(
        s,
        "                    try {{ rcS = c2.{base}Impl(Sidx, svN - 1, {full_ins}, {opts_lead}out begS, out nbS{bargs}); }}\n\
         \x20                   catch (Exception _sve) when (_sve is ITALibFailure) {{ rcS = ((ITALibFailure)_sve).RetCode; }}"
    );
    s.push_str("                    if (rcS == RetCode.Success && nbS > 0) {\n");
    let _ = writeln!(
        s,
        "                        try {{\n\
         \x20                           Core.{class} stA = c2.{base_pascal}OpenInternal({}, Sidx{opts_tail});\n\
         \x20                           rangeChecked = 1; rangeLegs++; rangeSites |= {anchored_bit};\n\
         \x20                           if (stA.OutRange.BegIdx != begS || stA.OutRange.Count != nbS) rangeOk = false;\n\
         \x20                       }} catch (ArgumentException) {{ rangeOk = false; if (diag.Length == 0) diag = \",\\\"anchoredOpenRejected\\\":1\"; }}",
        pfx_ins("svN")
    );
    s.push_str("                    }\n");
    s.push_str("                }\n");

    s.push_str("            }\n");

    s.push_str("        }\n");
    // `fill_ok` folds into `ok` as a safety net (mirrors the C/Rust/Java gates),
    // and so does the allocation probe -- `updAlloc` is a diagnostic AND a
    // failure (R4). `benign` is the Java-comparable count; `benignMut` is the
    // C#-only mid-stream leg's, reported separately so the cross-language
    // equality check compares like with like.
    s.push_str("        string extra = \",\\\"updAlloc\\\":\" + updAlloc;\n");
    if candle {
        s.push_str("        extra += \",\\\"candleMut\\\":\" + candleMutRan + \",\\\"candleMutMoved\\\":\" + candleMutMoved + \",\\\"benignMut\\\":\" + zsignMut;\n");
    }

    s.push_str("        return \"{\\\"retCode\\\":0,\\\"beg\\\":\" + beg + \",\\\"nb\\\":\" + nb + \",\\\"legs\\\":\" + legs + \",\\\"fill_checked\\\":\" + fillChecked + \",\\\"fill_ok\\\":\" + (fillOk ? 1 : 0) + \",\\\"range_checked\\\":\" + rangeChecked + \",\\\"range_legs\\\":\" + rangeLegs + \",\\\"range_sites\\\":\" + rangeSites + \",\\\"range_sites_all\\\":"); s.push_str(&SV_RANGE_MASK_CSHARP.to_string()); s.push_str(",\\\"range_ok\\\":\" + (rangeOk ? 1 : 0) + \",\\\"step_ok\\\":\" + (allOk ? 1 : 0) + \",\\\"ok\\\":\" + ((allOk && fillOk && rangeOk) ? 1 : 0) + \",\\\"peek_ok\\\":\" + (peekAll ? 1 : 0) + \",\\\"peek_reps\\\":\" + peekReps + \",\\\"peek_rep_ok\\\":\" + (peekRepAll ? 1 : 0) + \",\\\"peek_rejects\\\":\" + peekRejects + \",\\\"benign\\\":\" + zsign + extra + diag + \"}\";\n");
    s.push_str("    }\n\n");
    s
}

/// The whole C# `stream_verify` section: the two comparators, the candle-round
/// and live-mutation helpers, the allocation sink, one `Sv_<NAME>` per function
/// with an emitted C# stream, the `fuzz_in_hash` self-check, and the dispatcher.
///
/// The dispatcher's unknown-method answer is DELIBERATELY NOT Java's
/// `not_streamable` -- see the block comment at the emit site.
#[allow(clippy::too_many_lines)]
pub(crate) fn generate_csharp_stream_verify(
    funcs: &[FuncDef],
    enums: &HashMap<String, EnumDef>,
) -> String {
    use std::fmt::Write as _;
    let mut s = String::new();
    s.push_str("    // ---- stream_verify: C# stream vs C# batch, bitwise ----\n\n");

    // Strict, same-tier comparator.
    s.push_str("    /* Strict comparator for SAME-TIER legs: peek vs update, Value vs update,\n");
    s.push_str("       clone vs clone, and the int.MinValue sentinel pair all run ONE code path\n");
    s.push_str("       twice, so they have no licence to differ in a single bit. -0.0 != +0.0\n");
    s.push_str("       here and NaN == NaN -- which is exactly why a <NAME>_Value's own `==`\n");
    s.push_str("       must never be used for these: record-struct equality says the opposite\n");
    s.push_str("       of both, so it would pass on precisely what these legs look for. */\n");
    s.push_str("    static bool SvBne(double a, double b) =>\n");
    s.push_str("        BitConverter.DoubleToInt64Bits(a) != BitConverter.DoubleToInt64Bits(b);\n\n");

    // Cross-tier comparator.
    s.push_str("    /* Cross-tier comparator: stream vs batch, fill vs batch. Bits that differ\n");
    s.push_str("       but compare equal are +0.0 vs -0.0 (issue #147) -- counted as benign,\n");
    s.push_str("       never a mismatch, because the two tiers reach a zero by different but\n");
    s.push_str("       equally correct routes.\n");
    s.push_str("       `zsign` is a `ref` to a REQUEST-SCOPED local, never a static: one process\n");
    s.push_str("       answers many requests and a static would carry one function's count into\n");
    s.push_str("       the next. (Java passes a one-element array here only because Java has no\n");
    s.push_str("       `ref`.) */\n");
    s.push_str("    static bool SvXtierNe(double a, double b, ref long zsign) {\n");
    s.push_str("        if (!SvBne(a, b)) return false;\n");
    s.push_str("        if (a == b) { zsign++; return false; }\n");
    s.push_str("        return true;\n");
    s.push_str("    }\n\n");

    // Elision barrier for the allocation probe.
    s.push_str("    /* The allocation probe accumulates one output component into a local and\n");
    s.push_str("       consumes it here, AFTER the measured region. Without a consumer the JIT\n");
    s.push_str("       can prove the Update results dead and delete the calls, and the probe\n");
    s.push_str("       then reads 0 bytes for a loop that never ran. Never read. */\n");
    s.push_str("    static double svUpdSink;\n\n");

    // Candle rounds, through the shipped builder.
    s.push_str("    /* Candle-settings rounds (mirror the C/Rust/Java sweep): defaults /\n");
    s.push_str("       avgPeriod+3 / avgPeriod=0 (instant candle) / rangeType=Shadows.\n");
    s.push_str("       Goes through the SHIPPED CoreBuilder, so the validation exercised is the\n");
    s.push_str("       library's own and not a second copy living in the server; Build() takes a\n");
    s.push_str("       snapshot, so a built Core never aliases the builder.\n");
    s.push_str("       Each round is derived from Core.DefaultCandleSettings, never from the\n");
    s.push_str("       previous round -- a fresh builder is seeded with exactly those defaults,\n");
    s.push_str("       so the rounds cannot compound. (C# needs none of Java's care about\n");
    s.push_str("       editing the shared defaults in place: Core clones the array and\n");
    s.push_str("       CandleSetting is immutable, so replacing a slot reaches nothing else.)\n");
    s.push_str("       WHAT THESE FOUR ROUNDS DO NOT TEST: the open-time snapshot. Each round\n");
    s.push_str("       builds its Core and opens on it, so the step's snapshot and the Core's\n");
    s.push_str("       live settings hold the same value and a step reading the wrong one is\n");
    s.push_str("       invisible here. SvMutateLiveCandles below is what tests that. */\n");
    s.push_str("    static CoreBuilder SvApplyCandleRound(CoreBuilder b, int rd) {\n");
    s.push_str("        if (rd == 0) return b;\n");
    s.push_str("        for (int ci = 0; ci < Core.DefaultCandleSettings.Length; ci++) {\n");
    s.push_str("            CandleSetting cs = Core.DefaultCandleSettings[ci];\n");
    s.push_str("            if (rd == 1)\n");
    s.push_str("                b = b.CandleSetting((CandleSettingType)ci, cs.RangeType, cs.AvgPeriod + 3, cs.Factor);\n");
    s.push_str("            else if (rd == 2)\n");
    s.push_str("                b = b.CandleSetting((CandleSettingType)ci, cs.RangeType, 0, cs.Factor);\n");
    s.push_str("            else if (rd == 3)\n");
    s.push_str("                b = b.CandleSetting((CandleSettingType)ci, TALib.RangeType.Shadows, cs.AvgPeriod, cs.Factor);\n");
    s.push_str("        }\n");
    s.push_str("        return b;\n");
    s.push_str("    }\n\n");

    // Live mutation of a BUILT Core -- the one thing the builder cannot express.
    s.push_str("    /* Overwrite a BUILT Core's live candle settings, in place.\n");
    s.push_str("       `Core.candleSettings` is `internal readonly CandleSetting[]` -- the\n");
    s.push_str("       REFERENCE is readonly, the ELEMENTS are not -- and CandleSetting's\n");
    s.push_str("       constructor is internal. Both are reachable because the server csproj\n");
    s.push_str("       compiles the library sources into its own assembly.\n");
    s.push_str("       This is the one thing CoreBuilder deliberately cannot express: a\n");
    s.push_str("       settings change AFTER the Core exists, which is precisely what the\n");
    s.push_str("       streaming open-time snapshot contract is about.\n");
    s.push_str("       All three fields move, and to values no round uses, so a step reading\n");
    s.push_str("       live settings must produce a DIFFERENT number rather than a\n");
    s.push_str("       differently-spelled same one.\n");
    s.push_str("       The caller saves and restores: every later leg in the round runs against\n");
    s.push_str("       the round's settings, not these. */\n");
    s.push_str("    static void SvMutateLiveCandles(Core c) {\n");
    s.push_str("        for (int ci = 0; ci < c.candleSettings.Length; ci++) {\n");
    s.push_str("            CandleSetting cs = c.candleSettings[ci];\n");
    s.push_str("            c.candleSettings[ci] = new CandleSetting(\n");
    s.push_str("                cs.RangeType == TALib.RangeType.Shadows ? TALib.RangeType.HighLow\n");
    s.push_str("                                                       : TALib.RangeType.Shadows,\n");
    s.push_str("                (cs.AvgPeriod + 7) % 13,\n");
    s.push_str("                cs.Factor * 2.0 + 0.5);\n");
    s.push_str("        }\n");
    s.push_str("    }\n\n");
    s.push_str("    static void SvRestoreLiveCandles(Core c, CandleSetting[] saved) {\n");
    s.push_str("        Array.Copy(saved, c.candleSettings, saved.Length);\n");
    s.push_str("    }\n\n");

    let lookup = crate::streaming::FuncsLookup(funcs);
    let emitted: Vec<&FuncDef> = funcs
        .iter()
        .filter(|f| crate::backends::csharp_stream::emits_stream(f, &lookup))
        .collect();
    for f in &emitted {
        s.push_str(&emit_csharp_sv_func(f, funcs, enums));
    }

    // fuzz_in_hash -- the same input-port self-check the Rust and Java servers
    // answer (issue #113): proves the FuzzData port reproduces C's fuzz_gen
    // bytes. The stream pass probes it, so the port cannot silently rot.
    //
    // INTEGRATION NOTE 2: the driver's existing self-check runs at exactly one
    // seed (gen_seed = 7, n = 240) while the vector loop uses others, and it
    // fails OPEN -- an absent `in_hash` just breaks the loop. Neither is fixable
    // from this side (both live in test_codegen.c), which is why S2's gate is on
    // the literal printed line "Fuzz-port self-check: 9/9 shapes bit-identical"
    // and not on the absence of a failure message.
    s.push_str("    static string HandleFuzzInHash(JsonElement req) {\n");
    s.push_str("        int shape = GetInt(req, \"gen_shape\", 0);\n");
    s.push_str("        int seed = GetInt(req, \"gen_seed\", 0);\n");
    s.push_str("        int n = GetInt(req, \"gen_n\", 0);\n");
    s.push_str("        if (n < 1) n = 1;\n");
    s.push_str("        if (n > MAX_ARRAY_SIZE) n = MAX_ARRAY_SIZE;\n");
    s.push_str("        double[] fo = new double[n]; double[] fh = new double[n]; double[] fl = new double[n];\n");
    s.push_str("        double[] fc = new double[n]; double[] fv = new double[n]; double[] foi = new double[n];\n");
    s.push_str("        FuzzData.FuzzGen(shape, seed, n, fo, fh, fl, fc, fv, foi);\n");
    s.push_str("        ulong hh = SvHashInit();\n");
    s.push_str("        hh = SvHashF64(hh, fo, n);\n");
    s.push_str("        hh = SvHashF64(hh, fh, n);\n");
    s.push_str("        hh = SvHashF64(hh, fl, n);\n");
    s.push_str("        hh = SvHashF64(hh, fc, n);\n");
    s.push_str("        hh = SvHashF64(hh, fv, n);\n");
    s.push_str("        hh = SvHashF64(hh, foi, n);\n");
    s.push_str("        hh = SvHashFin(hh);\n");
    s.push_str("        return \"{\\\"in_hash\\\":\\\"\" + hh.ToString(\"x16\") + \"\\\"}\";\n");
    s.push_str("    }\n\n");

    s.push_str("    static string HandleStreamVerify(JsonElement req) {\n");
    s.push_str("        string fn = req.GetProperty(\"funcName\").GetString()!;\n");
    s.push_str("        switch (fn) {\n");
    for f in &emitted {
        let _ = writeln!(
            s,
            "        case \"TA_{}\": return Sv_{}(req);",
            f.name.to_uppercase(),
            f.name
        );
    }
    // ============ THE DARK RESPONSE -- READ BEFORE CHANGING THIS LINE ========
    //
    // The driver's capability probe is a SUBSTRING test:
    //     test_codegen.c:3686   strstr(responseBuf, "not_streamable")
    // and it is all-or-nothing: the moment the C# server answers with that
    // token, ta_regtest starts requiring a stream handler for every function
    // carrying TA_FUNC_FLG_STREAM and prints STREAM SET MISMATCH for each one
    // that is missing. The C# metadata catalogue already publishes that flag on
    // all 172 functions, so copying Java's literal now would redden every
    // regtest.py run for the remaining stages, for a reason unrelated to the
    // work in flight.
    //
    // So the unknown-method answer -- including the TA_STREAM_PROBE the driver
    // sends -- is pinned to the string below, which must NOT contain the token
    // `not_streamable` anywhere in the emitted file. S2 gates on
    // `grep -c not_streamable TaCodegenServe.cs == 0` until S9.
    //
    // The flip happened once all 172 functions had a handler, which is the
    // precondition the all-or-nothing check needs. It is a capability
    // ANNOUNCEMENT, not a description of this method: the driver reads the
    // token off the unknown-name path (it probes with TA_STREAM_PROBE, which is
    // not a function), so answering it is how the server says "ask me about
    // streams". A real function name that fell through to here would be a
    // missing handler, and the driver reports that as STREAM SET MISMATCH.
    // ========================================================================
    s.push_str("        default: return \"{\\\"error\\\":\\\"not_streamable\\\"}\";\n");
    s.push_str("        }\n");
    s.push_str("    }\n\n");
    s
}
