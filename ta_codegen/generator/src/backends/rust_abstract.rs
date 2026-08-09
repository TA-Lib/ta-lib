//! Generate `abstract_.rs` — the Rust function metadata registry (introspection layer).
//!
//! This is the Rust analog of C's `ta_abstract` (`TA_GetFuncInfo`,
//! `TA_Get*ParameterInfo`, `TA_ForEachFunc`). Instead of mirroring C's runtime
//! (linear strcmp name scan, opaque `void* dataSet`, heap-allocated string tables,
//! fn-pointer callbacks), it emits a **zero-cost, link-time-const registry**:
//! everything lives in `&'static`/`const` tables, the opaque dataSet becomes a
//! type-safe `OptInputType` enum, `FuncId` is a fieldless enum that doubles as the
//! dense index, enumeration is an iterator, and name lookup is a generated `match`.
//!
//! Renders [`abstract_rows`](super::abstract_rows) — the backend-neutral row model
//! Java and C# render too — so the three registries agree by construction rather
//! than by three parallel derivations.
#![allow(clippy::cast_possible_truncation, clippy::cast_sign_loss)]

use super::abstract_rows::{rows, FuncRow, InputKind, InputRow, OptDomain, OptRow, OutputKind, OutputRow};
use crate::ir::{EnumDef, FuncDef};
use std::collections::HashMap;
use std::fmt::Write as _;
use std::path::Path;

/// Generate `ta_codegen/output/rust/src/abstract_.rs` from the function defs.
///
/// [`rows`] sorts alphabetically by name (so `FuncId` discriminants and the name
/// `match` are deterministic); this emits the registry and writes only if changed.
#[allow(clippy::implicit_hasher)]
pub fn generate(funcs: &[FuncDef], enums: &HashMap<String, EnumDef>, out_base: &Path) {
    let sorted = rows(funcs, enums);
    let n = sorted.len();

    let mut o = String::new();
    o.push_str(HEADER);

    // --- FuncId enum (fieldless; doubles as the dense index into FUNCS) ---
    o.push_str("#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, PartialOrd, Ord)]\n");
    o.push_str("#[repr(u16)]\n#[non_exhaustive]\n#[allow(non_camel_case_types)]\npub enum FuncId {\n");
    for f in &sorted {
        let _ = writeln!(o, "    {},", f.name);
    }
    o.push_str("}\n\n");

    let _ = writeln!(o, "impl FuncId {{");
    let _ = writeln!(o, "    /// Number of functions in the registry.");
    let _ = writeln!(o, "    pub const COUNT: usize = {n};");
    o.push_str("    /// Metadata for this function (O(1) index into the const table).\n");
    o.push_str("    #[inline] pub fn info(self) -> &'static FuncInfo { &FUNCS[self as usize] }\n");
    o.push_str("    /// Upper-case TA name, e.g. \"RSI\".\n");
    o.push_str("    #[inline] pub fn name(self) -> &'static str { FUNCS[self as usize].name }\n");
    o.push_str("}\n\n");

    // --- model types (fixed) ---
    o.push_str(MODEL);

    // --- the one flat master table ---
    let _ = writeln!(o, "/// All function metadata, indexed by [`FuncId`]. Link-time const, in `.rodata`.");
    let _ = writeln!(o, "pub static FUNCS: [FuncInfo; {n}] = [");
    for f in &sorted {
        emit_func(&mut o, f);
    }
    o.push_str("];\n\n");

    // --- API surface (C-recognizable names, idiomatic shapes) ---
    emit_api(&mut o, &sorted);

    // --- TA_FunctionDescriptionXML analog (embeds the XML data file below) ---
    o.push_str(XML_FN);

    // --- committed regression tests for the registry's structural invariants ---
    o.push_str(REGISTRY_TESTS);

    // Write the XML data file embedded by function_description_xml() via include_str!.
    // Byte-identical to the repo-root ta_func_api.xml (same generator + same input),
    // which C's TA_FunctionDescriptionXML also bakes — so the two are equal.
    let xml = super::func_api_xml::generate_string(funcs);
    let xml_path = out_base.join("rust/library/src/ta_func_api.xml");
    super::write_if_changed(&xml_path, &xml, "ta_func_api.xml (rust embed)", n);

    let out_path = out_base.join("rust/library/src/abstract_api.rs");
    super::write_if_changed(&out_path, &o, "abstract_api.rs", n);
}

fn emit_func(o: &mut String, f: &FuncRow) {
    o.push_str("    FuncInfo {\n");
    let _ = writeln!(o, "        id: FuncId::{},", f.name);
    let _ = writeln!(o, "        name: {:?},", f.name);
    let _ = writeln!(o, "        group: Group::{},", f.group.ident());
    let _ = writeln!(o, "        hint: {:?},", f.hint);
    let _ = writeln!(o, "        flags: FuncFlags({:#010x}),", f.flags);

    o.push_str("        inputs: &[");
    for inp in &f.inputs {
        emit_input(o, inp);
    }
    o.push_str("],\n");

    o.push_str("        opt_inputs: &[");
    for opt in &f.opt_inputs {
        emit_opt(o, opt);
    }
    o.push_str("],\n");

    o.push_str("        outputs: &[");
    for out in &f.outputs {
        emit_output(o, out);
    }
    o.push_str("],\n");

    match &f.unst {
        Some(u) => {
            let _ = writeln!(o, "        unst_id: Some(FuncUnstId::{}),", u.name);
        }
        None => o.push_str("        unst_id: None,\n"),
    }
    o.push_str("    },\n");
}

/// One `InputInfo`. Price components were folded back into a single
/// `InputType::Price` (carrying the OHLCV bitmask and the canonical
/// `inPriceXXX` name) by [`abstract_rows`](super::abstract_rows), via the same
/// `price_bundle` fold the C abstract backend uses.
fn emit_input(o: &mut String, inp: &InputRow) {
    let kind = match inp.kind {
        super::abstract_rows::InputKind::Price => "Price",
        super::abstract_rows::InputKind::Real => "Real",
        super::abstract_rows::InputKind::Integer => "Integer",
    };
    let _ = write!(
        o,
        "InputInfo {{ param_name: {:?}, kind: InputType::{kind}, flags: InputFlags({:#010x}) }}, ",
        inp.param_name, inp.flags
    );
}

fn emit_output(o: &mut String, out: &OutputRow) {
    let kind = match out.kind {
        super::abstract_rows::OutputKind::Real => "Real",
        super::abstract_rows::OutputKind::Integer => "Integer",
    };
    let _ = write!(
        o,
        "OutputInfo {{ param_name: {:?}, kind: OutputType::{kind}, flags: OutputFlags({:#010x}) }}, ",
        out.param_name, out.flags
    );
}

fn emit_opt(o: &mut String, opt: &OptRow) {
    // The row carries `precision` as `i32` (C's `TA_RealRange.precision` is an
    // int); the generated `OptInputType::RealRange` narrows it to `u8`. Nothing in
    // the shipped input comes close, but an out-of-range YAML value would emit a
    // crate that does not compile — fail here instead, naming the parameter.
    if let OptDomain::RealRange { precision, .. } = &opt.domain {
        assert!(
            (0..=255).contains(precision),
            "{}: precision {precision} does not fit the generated `u8` field",
            opt.param_name
        );
    }
    let _ = write!(
        o,
        "OptInputInfo {{ param_name: {:?}, display_name: {:?}, hint: {:?}, flags: OptInputFlags({:#010x}), kind: ",
        opt.param_name, opt.display_name, opt.hint, opt.flags
    );
    emit_domain(o, &opt.domain);
    o.push_str(" }, ");
}

fn emit_domain(o: &mut String, domain: &OptDomain) {
    match domain {
        OptDomain::RealRange { min, max, precision, default, suggested } => {
            let (s, e, i) = *suggested;
            let _ = write!(
                o,
                "OptInputType::RealRange {{ min: {}, max: {}, precision: {}, default: {}, suggested: ({}, {}, {}) }}",
                fl(*min),
                fl(*max),
                precision,
                fl(*default),
                fl(s),
                fl(e),
                fl(i),
            );
        }
        OptDomain::IntegerRange { min, max, default, suggested } => {
            let (s, e, i) = *suggested;
            let _ = write!(
                o,
                "OptInputType::IntegerRange {{ min: {min}, max: {max}, default: {default}, suggested: ({s}, {e}, {i}) }}"
            );
        }
        OptDomain::IntegerList { values, default } => {
            o.push_str("OptInputType::IntegerList { values: &[");
            for (v, name) in values {
                let _ = write!(o, "({v}, {name:?}), ");
            }
            let _ = write!(o, "], default: {default} }}");
        }
        OptDomain::RealList { values, default } => {
            o.push_str("OptInputType::RealList { values: &[");
            for (v, name) in values {
                let _ = write!(o, "({}, {name:?}), ", fl(*v));
            }
            let _ = write!(o, "], default: {} }}", fl(*default));
        }
    }
}

fn emit_api(o: &mut String, sorted: &[FuncRow]) {
    o.push_str(
        "/// Resolve a function name (e.g. \"RSI\") to its [`FuncId`].\n\
         ///\n\
         /// Uses a generated `match` — see the module-level docs for why this is O(1)\n\
         /// and faster than C's linear `strcmp` scan, with zero allocation/dependencies.\n",
    );
    o.push_str("pub fn get_func_handle(name: &str) -> Option<FuncId> {\n    Some(match name {\n");
    for f in sorted {
        let _ = writeln!(o, "        {:?} => FuncId::{},", f.name, f.name);
    }
    o.push_str("        _ => return None,\n    })\n}\n\n");

    o.push_str("/// C-style variant returning the familiar `RetCode` error channel.\n");
    o.push_str("pub fn get_func_handle_rc(name: &str) -> Result<FuncId, crate::RetCode> {\n");
    o.push_str("    get_func_handle(name).ok_or(crate::RetCode::BadParam)\n}\n\n");

    o.push_str("/// Function metadata for a handle (infallible — `FuncId` cannot be invalid).\n");
    o.push_str("#[inline] pub fn get_func_info(handle: FuncId) -> &'static FuncInfo { handle.info() }\n");
    o.push_str("/// Required-input metadata by index (`None` if out of range).\n");
    o.push_str("#[inline] pub fn get_input_parameter_info(handle: FuncId, index: usize) -> Option<&'static InputInfo> { handle.info().inputs.get(index) }\n");
    o.push_str("/// Optional-input metadata by index (`None` if out of range).\n");
    o.push_str("#[inline] pub fn get_opt_input_parameter_info(handle: FuncId, index: usize) -> Option<&'static OptInputInfo> { handle.info().opt_inputs.get(index) }\n");
    o.push_str("/// Output metadata by index (`None` if out of range).\n");
    o.push_str("#[inline] pub fn get_output_parameter_info(handle: FuncId, index: usize) -> Option<&'static OutputInfo> { handle.info().outputs.get(index) }\n\n");

    o.push_str("/// Iterate metadata for every function (idiomatic replacement for C's `TA_ForEachFunc`).\n");
    o.push_str("#[inline] pub fn funcs() -> impl Iterator<Item = &'static FuncInfo> + Clone { FUNCS.iter() }\n");
    o.push_str("/// Iterate functions in a given group.\n");
    o.push_str("#[inline] pub fn funcs_in_group(group: Group) -> impl Iterator<Item = &'static FuncInfo> + Clone { FUNCS.iter().filter(move |f| f.group == group) }\n");
    o.push_str("/// C-style enumeration wrapper, for porters who prefer the callback shape.\n");
    o.push_str("pub fn for_each_func<F: FnMut(&'static FuncInfo)>(mut f: F) { for fi in FUNCS.iter() { f(fi); } }\n\n");
    o.push_str("/// All function groups (no allocation, unlike C's `TA_GroupTableAlloc`).\n");
    o.push_str("#[inline] pub fn groups() -> &'static [Group] { Group::ALL }\n");

    emit_binder(o, sorted);
}


/// The hand-written half of the binder: the holder, its setters and the sealed
/// `OptValue` trait. The two dispatch matches are generated after it.
const BINDER_SCAFFOLDING: &str = r#"
use crate::{Core, RetCode, MAX_INDEX};

/// Where a call's output starts and how much of it there is.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct OutRange {
    /// Index of the first computed value, relative to the input series.
    pub beg_idx: usize,
    /// How many values were written.
    pub nb_element: usize,
}

mod sealed {
    pub trait Sealed {}
    impl Sealed for i32 {}
    impl Sealed for f64 {}
}

/// A value bindable to an optional parameter.
///
/// Rust has no overloading, so this is how `set_opt` accepts either an `i32` or an
/// `f64` under one name — resolved at compile time, and sealed so the set of
/// bindable types stays the generator's to decide.
pub trait OptValue: sealed::Sealed {
    /// Bind `self` to optional parameter `index` of `holder`.
    ///
    /// # Errors
    /// [`RetCode::BadParam`] if the index is out of range or the parameter's
    /// domain does not take this type.
    fn bind(self, holder: &mut ParamHolder<'_>, index: usize) -> Result<(), RetCode>;
}

impl OptValue for i32 {
    fn bind(self, holder: &mut ParamHolder<'_>, index: usize) -> Result<(), RetCode> {
        let info = holder.func.info().opt_inputs.get(index).ok_or(RetCode::BadParam)?;
        match info.kind {
            OptInputType::IntegerRange { .. } | OptInputType::IntegerList { .. } => {
                holder.int_opt[index] = self;
                Ok(())
            }
            _ => Err(RetCode::BadParam),
        }
    }
}

impl OptValue for f64 {
    fn bind(self, holder: &mut ParamHolder<'_>, index: usize) -> Result<(), RetCode> {
        let info = holder.func.info().opt_inputs.get(index).ok_or(RetCode::BadParam)?;
        match info.kind {
            OptInputType::RealRange { .. } | OptInputType::RealList { .. } => {
                holder.real_opt[index] = self;
                Ok(())
            }
            _ => Err(RetCode::BadParam),
        }
    }
}

/// Binds a function's arguments at run time, then calls it — the counterpart of
/// C's `TA_ParamHolder` + `TA_CallFunc`, of Java's `ParamHolder` and of C#'s
/// `FunctionCall`.
///
/// Borrows rather than owns, so binding costs nothing and the caller keeps its
/// buffers. That is what makes two outputs sharing one buffer — the aliasing every
/// other backend rejects at run time (issue #108) — a compile error here.
///
/// ```no_run
/// use ta_lib::{Core, abstract_api::{self, FuncId}};
/// let core = Core::new();
/// let close = vec![1.0f64; 64];
/// let mut out = vec![0.0f64; 64];
/// let mut call = FuncId::SMA.new_call(&core);
/// call.set_input(0, &close)?;
/// call.set_opt(0, 30_i32)?;
/// call.set_output(0, &mut out)?;
/// let range = call.call(0, close.len() - 1)?;
/// # Ok::<(), ta_lib::RetCode>(())
/// ```
pub struct ParamHolder<'a> {
    func: FuncId,
    core: &'a Core,
    real_in: [Option<&'a [f64]>; MAX_INPUTS],
    int_in: [Option<&'a [i32]>; MAX_INPUTS],
    price: [[Option<&'a [f64]>; 6]; MAX_INPUTS],
    real_opt: [f64; MAX_OPT_INPUTS],
    int_opt: [i32; MAX_OPT_INPUTS],
    real_out: [Option<&'a mut [f64]>; MAX_OUTPUTS],
    int_out: [Option<&'a mut [i32]>; MAX_OUTPUTS],
}

impl FuncId {
    /// Begin a call to this function with arguments bound at run time.
    #[must_use]
    pub fn new_call(self, core: &Core) -> ParamHolder<'_> {
        ParamHolder {
            func: self,
            core,
            real_in: [None; MAX_INPUTS],
            int_in: [None; MAX_INPUTS],
            price: [[None; 6]; MAX_INPUTS],
            // Unset optional parameters carry the cross-language default
            // sentinel, exactly as an omitted argument does in C. Every generated
            // function maps it to that parameter's documented default (#162), so
            // "left unset" and "explicitly the default" are one code path.
            real_opt: [-4e37; MAX_OPT_INPUTS],
            int_opt: [i32::MIN; MAX_OPT_INPUTS],
            real_out: [const { None }; MAX_OUTPUTS],
            int_out: [const { None }; MAX_OUTPUTS],
        }
    }
}

impl<'a> ParamHolder<'a> {
    /// The function this call runs.
    #[must_use]
    pub fn info(&self) -> &'static FuncInfo { self.func.info() }

    fn check_input(&self, slot: usize, want: InputType) -> Result<(), RetCode> {
        let info = self.func.info().inputs.get(slot).ok_or(RetCode::BadParam)?;
        if info.kind == want { Ok(()) } else { Err(RetCode::BadParam) }
    }

    /// Bind a real input series.
    ///
    /// # Errors
    /// [`RetCode::BadParam`] if the slot is out of range or is not a real input.
    pub fn set_input(&mut self, slot: usize, series: &'a [f64]) -> Result<&mut Self, RetCode> {
        self.check_input(slot, InputType::Real)?;
        self.real_in[slot] = Some(series);
        Ok(self)
    }

    /// Bind an integer input series.
    ///
    /// # Errors
    /// [`RetCode::BadParam`] if the slot is out of range or is not an integer input.
    pub fn set_int_input(&mut self, slot: usize, series: &'a [i32]) -> Result<&mut Self, RetCode> {
        self.check_input(slot, InputType::Integer)?;
        self.int_in[slot] = Some(series);
        Ok(self)
    }

    /// Bind a price bundle. Components the function does not consume are accepted
    /// and ignored, matching C's `SET_PARAM_INFO` and the Java and C# binders.
    ///
    /// # Errors
    /// [`RetCode::BadParam`] if the slot is out of range, is not a price input, or
    /// a component the function *does* consume was left `None`.
    pub fn set_price_input(
        &mut self,
        slot: usize,
        open: Option<&'a [f64]>,
        high: Option<&'a [f64]>,
        low: Option<&'a [f64]>,
        close: Option<&'a [f64]>,
        volume: Option<&'a [f64]>,
        open_interest: Option<&'a [f64]>,
    ) -> Result<&mut Self, RetCode> {
        self.check_input(slot, InputType::Price)?;
        let flags = self.func.info().inputs[slot].flags;
        let given = [open, high, low, close, volume, open_interest];
        for (i, series) in given.into_iter().enumerate() {
            if flags.0 & (1u32 << i) != 0 && series.is_none() {
                return Err(RetCode::BadParam);
            }
            self.price[slot][i] = series;
        }
        Ok(self)
    }

    /// Bind an optional parameter. Takes an `i32` or an `f64`; see [`OptValue`].
    ///
    /// # Errors
    /// [`RetCode::BadParam`] if the index is out of range or the value's type does
    /// not match the parameter's domain.
    pub fn set_opt<V: OptValue>(&mut self, index: usize, value: V) -> Result<&mut Self, RetCode> {
        value.bind(self, index)?;
        Ok(self)
    }

    /// Bind a real output buffer.
    ///
    /// # Errors
    /// [`RetCode::BadParam`] if the index is out of range or is not a real output.
    pub fn set_output(&mut self, index: usize, out: &'a mut [f64]) -> Result<&mut Self, RetCode> {
        let info = self.func.info().outputs.get(index).ok_or(RetCode::BadParam)?;
        if info.kind != OutputType::Real { return Err(RetCode::BadParam); }
        self.real_out[index] = Some(out);
        Ok(self)
    }

    /// Bind an integer output buffer.
    ///
    /// # Errors
    /// [`RetCode::BadParam`] if the index is out of range or is not an integer output.
    pub fn set_int_output(&mut self, index: usize, out: &'a mut [i32]) -> Result<&mut Self, RetCode> {
        let info = self.func.info().outputs.get(index).ok_or(RetCode::BadParam)?;
        if info.kind != OutputType::Integer { return Err(RetCode::BadParam); }
        self.int_out[index] = Some(out);
        Ok(self)
    }
"#;

/// A `#[cfg(test)]` module exercising the binder over every function.
///
/// Written generically against the registry rather than per function, so it
/// cannot drift from the corpus: it enumerates `FUNCS`, binds whatever each row
/// declares, and asserts the contract. Run with `cargo test --lib -p ta-lib`.
const BINDER_TESTS: &str = r#"
#[cfg(test)]
mod binder_tests {
    use super::*;
    use crate::Core;

    const N: usize = 160;

    fn series(phase: f64) -> Vec<f64> {
        (0..N).map(|i| {
            let x = i as f64;
            100.0 + 8.0 * (x / 9.0 + phase).sin() + 0.4 * (x / 3.0 + phase).cos()
        }).collect()
    }

    /// Binds every declared input and output of `f`, leaving optional parameters
    /// unset unless `opts` says otherwise.
    fn drive(core: &Core, f: &'static FuncInfo,
             opts: &dyn Fn(&mut ParamHolder<'_>),
             close: &[f64], high: &[f64], low: &[f64], vol: &[f64],
             ints: &[i32],
             rout: &mut [Vec<f64>], iout: &mut [Vec<i32>]) -> Result<OutRange, RetCode> {
        let mut h = f.id.new_call(core);
        for (slot, inp) in f.inputs.iter().enumerate() {
            match inp.kind {
                InputType::Price => {
                    h.set_price_input(slot, Some(close), Some(high), Some(low),
                                      Some(close), Some(vol), Some(vol)).unwrap();
                }
                InputType::Real => { h.set_input(slot, close).unwrap(); }
                InputType::Integer => { h.set_int_input(slot, ints).unwrap(); }
            }
        }
        opts(&mut h);
        for (k, buf) in rout.iter_mut().enumerate() {
            if f.outputs[k].kind == OutputType::Real { h.set_output(k, buf).unwrap(); }
        }
        for (k, buf) in iout.iter_mut().enumerate() {
            if f.outputs[k].kind == OutputType::Integer { h.set_int_output(k, buf).unwrap(); }
        }
        h.call(0, N - 1)
    }

    fn bufs(f: &'static FuncInfo) -> (Vec<Vec<f64>>, Vec<Vec<i32>>) {
        (vec![vec![0.0; N]; f.outputs.len()], vec![vec![0; N]; f.outputs.len()])
    }

    /// Every function is reachable through the binder, and the range it reports
    /// starts exactly at the lookback the binder computes. `lookback()` is a
    /// SECOND mapping from opt slots to arguments, so pitting it against the one
    /// `call()` uses is what catches a transposed slot.
    #[test]
    fn every_function_binds_calls_and_agrees_with_its_lookback() {
        let core = Core::new();
        let close = series(0.0);
        let high: Vec<f64> = close.iter().map(|v| v + 2.0).collect();
        let low: Vec<f64> = close.iter().map(|v| v - 2.0).collect();
        let vol: Vec<f64> = (0..N).map(|i| 1.0e6 + i as f64).collect();
        let ints: Vec<i32> = (0..N as i32).collect();

        let mut covered = 0;
        for f in FUNCS.iter() {
            let (mut r, mut i) = bufs(f);
            // Distinct, in-range periods: with every slot carrying the same
            // number a transposition is undetectable.
            let set = |h: &mut ParamHolder<'_>| {
                for (k, o) in f.opt_inputs.iter().enumerate() {
                    if let OptInputType::IntegerRange { min, max, .. } = o.kind {
                        let v = (min + 2 + k as i32).min(max);
                        h.set_opt(k, v).unwrap();
                    }
                }
            };
            let lb = {
                let mut probe = f.id.new_call(&core);
                set(&mut probe);
                probe.lookback().expect("lookback")
            };
            let range = drive(&core, f, &set, &close, &high, &low, &vol, &ints, &mut r, &mut i)
                .unwrap_or_else(|e| panic!("{} failed: {e:?}", f.name));
            assert_eq!(range.beg_idx, lb, "{}: outBegIdx must be the lookback", f.name);
            covered += 1;
        }
        assert_eq!(covered, FUNCS.len());
    }

    /// An unset optional parameter and its explicitly-bound documented default are
    /// the same call — the sentinel contract, from the binder's side (issue #162).
    #[test]
    fn unset_matches_the_documented_default() {
        let core = Core::new();
        let close = series(0.0);
        let high: Vec<f64> = close.iter().map(|v| v + 2.0).collect();
        let low: Vec<f64> = close.iter().map(|v| v - 2.0).collect();
        let vol: Vec<f64> = (0..N).map(|i| 1.0e6 + i as f64).collect();
        let ints: Vec<i32> = (0..N as i32).collect();

        let mut covered = 0;
        for f in FUNCS.iter() {
            if f.opt_inputs.is_empty() { continue; }
            let (mut ru, mut iu) = bufs(f);
            let (mut re, mut ie) = bufs(f);
            let unset = |_: &mut ParamHolder<'_>| {};
            let explicit = |h: &mut ParamHolder<'_>| {
                for (k, o) in f.opt_inputs.iter().enumerate() {
                    match o.kind {
                        OptInputType::IntegerRange { default, .. } => { h.set_opt(k, default).unwrap(); }
                        OptInputType::IntegerList { default, .. } => {
                            h.set_opt(k, i32::try_from(default).unwrap()).unwrap();
                        }
                        OptInputType::RealRange { default, .. }
                        | OptInputType::RealList { default, .. } => { h.set_opt(k, default).unwrap(); }
                    }
                }
            };
            let a = drive(&core, f, &unset, &close, &high, &low, &vol, &ints, &mut ru, &mut iu);
            let b = drive(&core, f, &explicit, &close, &high, &low, &vol, &ints, &mut re, &mut ie);
            assert_eq!(a, b, "{}: unset must equal the explicit default", f.name);
            if let Ok(r) = a {
                for k in 0..f.outputs.len() {
                    if f.outputs[k].kind == OutputType::Real {
                        assert_eq!(ru[k][..r.nb_element], re[k][..r.nb_element],
                                   "{} output {k}", f.name);
                    } else {
                        assert_eq!(iu[k][..r.nb_element], ie[k][..r.nb_element],
                                   "{} output {k}", f.name);
                    }
                }
            }
            covered += 1;
        }
        // 79 of the 168 declare an optional parameter; floor it so the
        // discriminating half cannot quietly shrink.
        assert!(covered >= 75, "covered {covered} functions with optional parameters");
    }

    /// The bind-time check C cannot perform: its setters take a bare pointer, so an
    /// undersized output is an out-of-bounds write there. A slice carries its length.
    #[test]
    fn an_undersized_output_is_rejected_not_written_past() {
        let core = Core::new();
        let close = series(0.0);
        let mut tiny = vec![0.0; 4];
        let mut h = FuncId::SMA.new_call(&core);
        h.set_input(0, &close).unwrap();
        h.set_opt(0, 30_i32).unwrap();
        h.set_output(0, &mut tiny).unwrap();
        assert_eq!(h.call(0, N - 1), Err(RetCode::BadParam));
    }

    /// Type mismatches and out-of-range indices are refused rather than silently bound.
    #[test]
    fn the_setters_refuse_what_does_not_belong() {
        let core = Core::new();
        let close = series(0.0);
        let mut out = vec![0.0; N];
        let mut h = FuncId::SMA.new_call(&core);
        assert_eq!(h.set_input(9, &close).err(), Some(RetCode::BadParam));
        assert_eq!(h.set_opt(0, 1.5_f64).err(), Some(RetCode::BadParam));
        assert_eq!(h.set_opt(9, 30_i32).err(), Some(RetCode::BadParam));
        let mut wrong_kind = [0i32; 4];
        assert_eq!(h.set_int_output(0, &mut wrong_kind).err(), Some(RetCode::BadParam));
        h.set_output(0, &mut out).unwrap();
        assert_eq!(h.call(0, N - 1).err(), Some(RetCode::BadParam)); // input still unbound
    }
}
"#;

/// Emit the dynamic-binding tier: `ParamHolder`, the sealed `OptValue` trait, and
/// the generated `call` / `lookback` dispatch.
///
/// Rust shipped the introspection half of `ta_abstract` and none of the
/// invocation half, so a caller could discover that MA takes an `optInMAType` and
/// then had to hand-write a 168-arm `match` to actually call it — which is the
/// work this layer exists to remove (issue #164). C, Java and C# all have it.
///
/// The holder is the right shape here even though it is C's: the use case is
/// inherently runtime-dynamic, and typestate or const generics — the tempting
/// Rust answers — would demand compile-time knowledge of exactly what this layer
/// exists to discover at run time. What Rust does buy over C:
///
/// * **Slices, not pointers.** `TA_SetInputParamRealPtr` takes a bare
///   `const double*` with no length, which is why no backend can check output
///   capacity at bind time. Here the length rides along, so an undersized output
///   is a `RetCode::BadParam` instead of an out-of-bounds write.
/// * **A sealed `OptValue` trait instead of overloads.** One `set_opt` accepting
///   `i32` or `f64`, resolved at compile time — the Rust spelling of the
///   overloading C could not afford.
/// * **`Result`, not out-params.**
/// * **Output aliasing is a compile error.** Binding one buffer to two outputs is
///   the #108 rejection every other backend implements as a runtime check; two
///   `&mut` to the same slice simply do not coexist.
fn emit_binder(o: &mut String, sorted: &[FuncRow]) {
    let widest_input = sorted.iter().map(|f| f.inputs.len()).max().unwrap_or(1).max(1);
    let widest_opt = sorted.iter().map(|f| f.opt_inputs.len()).max().unwrap_or(1).max(1);
    let widest_output = sorted.iter().map(|f| f.outputs.len()).max().unwrap_or(1).max(1);

    let _ = writeln!(
        o,
        "\n/// Widest input arity in the corpus — the holder's slots are sized from it.\npub const MAX_INPUTS: usize = {widest_input};\n\
         /// Widest optional-parameter arity in the corpus.\npub const MAX_OPT_INPUTS: usize = {widest_opt};\n\
         /// Widest output arity in the corpus.\npub const MAX_OUTPUTS: usize = {widest_output};\n"
    );

    o.push_str(BINDER_SCAFFOLDING);

    // ---- lookback dispatch -------------------------------------------------
    o.push_str(
        "    /// The first index at which this function produces output, for the\n\
         \x20   /// optional parameters bound so far. Inputs and outputs need not be\n\
         \x20   /// bound — which is what makes it useful for sizing them.\n\
         \x20   ///\n\
         \x20   /// # Errors\n\
         \x20   /// [`RetCode::BadParam`] if a bound optional parameter is out of range.\n\
         \x20   pub fn lookback(&self) -> Result<usize, RetCode> {\n\
         \x20       let lb = match self.func {\n",
    );
    for f in sorted {
        let snake = f.name.clone();
        let args = opt_args(f);
        let _ = writeln!(
            o,
            "            FuncId::{} => self.core.{snake}_Lookback({args}),",
            f.name
        );
    }
    o.push_str(
        "        };\n\
         \x20       // The generated lookbacks report a rejected parameter as usize::MAX.\n\
         \x20       if lb == usize::MAX { Err(RetCode::BadParam) } else { Ok(lb) }\n\
         \x20   }\n\n",
    );

    // ---- call dispatch -----------------------------------------------------
    o.push_str(
        "    /// Run the function over `[start_idx, end_idx]`.\n\
         \x20   ///\n\
         \x20   /// Unbound optional parameters carry the cross-language default\n\
         \x20   /// sentinel, which every generated function maps to its documented\n\
         \x20   /// default — so leaving one unset and passing its default explicitly\n\
         \x20   /// are the same call (issue #162).\n\
         \x20   ///\n\
         \x20   /// # Errors\n\
         \x20   /// [`RetCode::OutOfRangeStartIndex`] if `start_idx` exceeds\n\
         \x20   /// [`MAX_INDEX`], [`RetCode::OutOfRangeEndIndex`] if `end_idx` exceeds\n\
         \x20   /// it or is below `start_idx`, and [`RetCode::BadParam`] if a required\n\
         \x20   /// input or output was never bound, if an output is too short for the\n\
         \x20   /// range, or if the function rejects its parameters.\n\
         \x20   pub fn call(&mut self, start_idx: usize, end_idx: usize) -> Result<OutRange, RetCode> {\n\
         \x20       if start_idx > MAX_INDEX { return Err(RetCode::OutOfRangeStartIndex); }\n\
         \x20       if end_idx > MAX_INDEX || end_idx < start_idx {\n\
         \x20           return Err(RetCode::OutOfRangeEndIndex);\n\
         \x20       }\n\
         \x20       // C cannot do this: TA_SetOutputParamRealPtr takes a bare pointer, so\n\
         \x20       // no backend checks capacity and an undersized buffer is an\n\
         \x20       // out-of-bounds write. A slice carries its length.\n\
         \x20       let need = end_idx - start_idx + 1;\n\
         \x20       let mut beg: usize = 0;\n\
         \x20       let mut nb: usize = 0;\n\
         \x20       let rc = match self.func {\n",
    );
    for f in sorted {
        emit_call_arm(o, f);
    }
    o.push_str(
        "        };\n\
         \x20       if rc == RetCode::Success { Ok(OutRange { beg_idx: beg, nb_element: nb }) } else { Err(rc) }\n\
         \x20   }\n}\n",
    );

    o.push_str(BINDER_TESTS);
}

/// The optional-parameter argument list for one function, in declaration order.
fn opt_args(f: &FuncRow) -> String {
    f.opt_inputs
        .iter()
        .enumerate()
        .map(|(i, opt)| match &opt.domain {
            OptDomain::RealRange { .. } | OptDomain::RealList { .. } => {
                format!("self.real_opt[{i}]")
            }
            _ => format!("self.int_opt[{i}]"),
        })
        .collect::<Vec<_>>()
        .join(", ")
}

/// One `call` match arm. Inputs are read by value (the slices are `Copy` out of
/// their `Option`), outputs are `take`n so the borrow checker sees no overlap
/// with `&self`, reborrowed into the call, and put straight back — so a holder
/// stays reusable, as C's does.
fn emit_call_arm(o: &mut String, f: &FuncRow) {
    let snake = f.name.clone();
    let _ = writeln!(o, "            FuncId::{} => {{", f.name);

    let mut args: Vec<String> = vec!["start_idx".into(), "end_idx".into()];
    for (slot, inp) in f.inputs.iter().enumerate() {
        match inp.kind {
            InputKind::Price => {
                for c in &inp.signature_components {
                    let idx = *c as usize;
                    let _ = writeln!(
                        o,
                        "                let i{slot}_{idx} = self.price[{slot}][{idx}].ok_or(RetCode::BadParam)?;"
                    );
                    args.push(format!("i{slot}_{idx}"));
                }
            }
            InputKind::Real => {
                let _ = writeln!(
                    o,
                    "                let i{slot} = self.real_in[{slot}].ok_or(RetCode::BadParam)?;"
                );
                args.push(format!("i{slot}"));
            }
            InputKind::Integer => {
                let _ = writeln!(
                    o,
                    "                let i{slot} = self.int_in[{slot}].ok_or(RetCode::BadParam)?;"
                );
                args.push(format!("i{slot}"));
            }
        }
    }

    for (i, opt) in f.opt_inputs.iter().enumerate() {
        args.push(match &opt.domain {
            OptDomain::RealRange { .. } | OptDomain::RealList { .. } => {
                format!("self.real_opt[{i}]")
            }
            _ => format!("self.int_opt[{i}]"),
        });
    }
    args.push("&mut beg".into());
    args.push("&mut nb".into());

    for (k, out) in f.outputs.iter().enumerate() {
        let (arr, ty) = match out.kind {
            OutputKind::Integer => ("int_out", "i32"),
            OutputKind::Real => ("real_out", "f64"),
        };
        let _ = writeln!(
            o,
            "                let mut o{k} = self.{arr}[{k}].take().ok_or(RetCode::BadParam)?;"
        );
        let _ = writeln!(
            o,
            "                if o{k}.len() < need {{ self.{arr}[{k}] = Some(o{k}); return Err(RetCode::BadParam); }} // {ty}"
        );
        args.push(format!("&mut *o{k}"));
    }

    let _ = writeln!(o, "                let rc = self.core.{snake}({});", args.join(", "));
    for (k, out) in f.outputs.iter().enumerate() {
        let arr = match out.kind {
            OutputKind::Integer => "int_out",
            OutputKind::Real => "real_out",
        };
        let _ = writeln!(o, "                self.{arr}[{k}] = Some(o{k});");
    }
    o.push_str("                rc\n            }\n");
}

// --- name → identifier / variant helpers ---

/// Format an f64 as a valid Rust literal (Debug yields e.g. `2.0`, `0.1`, `3e37`).
fn fl(v: f64) -> String {
    format!("{v:?}")
}

/// `function_description_xml()` — the Rust analog of C's `TA_FunctionDescriptionXML()`.
/// Embeds the generated `ta_func_api.xml` data file at compile time.
const XML_FN: &str = r#"
/// Rust analog of C's `TA_FunctionDescriptionXML()` — the full machine-readable XML
/// description of every function. Byte-identical to the generated `ta_func_api.xml`
/// (embedded via `include_str!`), which C's `TA_FunctionDescriptionXML` also bakes.
pub fn function_description_xml() -> &'static str {
    include_str!("ta_func_api.xml")
}
"#;

/// Generated `#[cfg(test)]` invariant tests for the registry. These guard the
/// crate's public introspection API and the internal consistency of the tables
/// (FuncId<->index<->name, group coverage, param-index bounds, negative paths) —
/// the surface the metadata-parity RPC test does NOT exercise. Run with
/// `cargo test` in the generated crate.
const REGISTRY_TESTS: &str = r#"
#[cfg(test)]
mod registry_tests {
    use super::*;

    #[test]
    fn count_matches_table_and_indices_align() {
        assert_eq!(FuncId::COUNT, FUNCS.len());
        for (i, f) in FUNCS.iter().enumerate() {
            assert_eq!(f.id as usize, i, "FuncId discriminant must equal its FUNCS index");
        }
    }

    #[test]
    fn name_handle_roundtrip() {
        for f in FUNCS.iter() {
            assert_eq!(get_func_handle(f.name), Some(f.id), "handle lookup for {}", f.name);
            assert_eq!(f.id.name(), f.name);
            assert_eq!(get_func_info(f.id).name, f.name);
        }
    }

    #[test]
    fn unknown_name_is_none() {
        assert_eq!(get_func_handle("definitely_not_a_ta_func"), None);
        assert!(get_func_handle_rc("definitely_not_a_ta_func").is_err());
    }

    #[test]
    fn groups_cover_every_func() {
        assert_eq!(groups(), Group::ALL);
        for f in FUNCS.iter() {
            assert!(Group::ALL.contains(&f.group), "{} group not in Group::ALL", f.name);
            assert!(!f.group.as_str().is_empty());
        }
    }

    #[test]
    fn funcs_in_group_partitions_all() {
        let total: usize = Group::ALL.iter().map(|g| funcs_in_group(*g).count()).sum();
        assert_eq!(total, FuncId::COUNT);
        assert_eq!(funcs().count(), FuncId::COUNT);
    }

    #[test]
    fn param_index_bounds() {
        for f in FUNCS.iter() {
            assert!(get_input_parameter_info(f.id, f.nb_input()).is_none());
            assert!(get_opt_input_parameter_info(f.id, f.nb_opt_input()).is_none());
            assert!(get_output_parameter_info(f.id, f.nb_output()).is_none());
            if f.nb_input() > 0 { assert!(get_input_parameter_info(f.id, 0).is_some()); }
            if f.nb_output() > 0 { assert!(get_output_parameter_info(f.id, 0).is_some()); }
        }
    }

    #[test]
    fn for_each_func_visits_all() {
        let mut n = 0;
        for_each_func(|_| n += 1);
        assert_eq!(n, FuncId::COUNT);
    }

    #[test]
    fn function_description_xml_is_sane() {
        let xml = function_description_xml();
        assert!(xml.starts_with("<?xml"));
        assert!(xml.contains("<FinancialFunctions>"));
        assert!(xml.len() > 500);
    }
}
"#;

const HEADER: &str = r"//! TA-Lib function metadata registry — the Rust abstract / introspection layer.
//!
//! GENERATED by ta_codegen (`backends/rust_abstract.rs`) — do not edit by hand.
//!
//! Rust analog of C's `ta_abstract` (`TA_GetFuncInfo`, `TA_Get*ParameterInfo`,
//! `TA_ForEachFunc`), implemented as a **zero-cost, link-time-const registry**:
//!   * all metadata is `&'static`/`const` in `.rodata` — zero heap, zero runtime init;
//!   * the opaque C `void* dataSet` + type tag becomes a type-safe [`OptInputType`] enum
//!     (illegal states unrepresentable, no unchecked cast);
//!   * [`FuncId`] is a fieldless enum that doubles as the dense index into [`FUNCS`]
//!     (no opaque handle, no magic-number validity check);
//!   * enumeration returns an iterator instead of C's fn-pointer + `void*` callback;
//!   * no heap allocation and no `*Alloc`/`*Free` pairs anywhere.
//!
//! Public names mirror C (`get_func_info`, `get_*_parameter_info`, `for_each_func`,
//! `groups`) so C porters are at home; the shapes are idiomatic Rust.
//!
//! ## Why name lookup uses a generated `match` (and not a hash map / `phf` / strcmp)
//!  * Lookup is a **cold path** — function discovery at setup, never per-bar compute.
//!  * rustc/LLVM lower a 161-arm `&str` `match` to a length-bucketed + leading-byte
//!    dispatch (not a comparison chain): effectively O(1), entirely in `.rodata`,
//!    zero allocation, zero dependencies.
//!  * For reference, C's `TA_GetFuncHandle` is an O(n) linear `strcmp` within a
//!    26-way first-letter bucket (up to 67 compares for the `CDL*` bucket) plus
//!    several pointer hops per entry. The generated `match` is strictly less work.
#![allow(clippy::all)]
#![allow(non_camel_case_types)]

use crate::FuncUnstId;

";

const MODEL: &str = r#"/// Function group (closed set — replaces C's runtime group-string table + linear `getGroupId`).
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[repr(u8)]
#[non_exhaustive]
pub enum Group {
    CycleIndicators,
    MathOperators,
    MathTransform,
    MomentumIndicators,
    OverlapStudies,
    PatternRecognition,
    PriceTransform,
    StatisticFunctions,
    VolatilityIndicators,
    VolumeIndicators,
}

impl Group {
    /// All groups, in canonical order.
    pub const ALL: &'static [Group] = &[
        Group::CycleIndicators,
        Group::MathOperators,
        Group::MathTransform,
        Group::MomentumIndicators,
        Group::OverlapStudies,
        Group::PatternRecognition,
        Group::PriceTransform,
        Group::StatisticFunctions,
        Group::VolatilityIndicators,
        Group::VolumeIndicators,
    ];
    /// Canonical display string (matches C's `TA_GroupString` / the YAML `group`).
    pub const fn as_str(self) -> &'static str {
        match self {
            Group::CycleIndicators => "Cycle Indicators",
            Group::MathOperators => "Math Operators",
            Group::MathTransform => "Math Transform",
            Group::MomentumIndicators => "Momentum Indicators",
            Group::OverlapStudies => "Overlap Studies",
            Group::PatternRecognition => "Pattern Recognition",
            Group::PriceTransform => "Price Transform",
            Group::StatisticFunctions => "Statistic Functions",
            Group::VolatilityIndicators => "Volatility Indicators",
            Group::VolumeIndicators => "Volume Indicators",
        }
    }
}

/// Required-input data kind (C: `TA_Input_Price`/`Real`/`Integer`).
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[repr(u8)]
pub enum InputType { Price, Real, Integer }

/// Output data kind (C: `TA_Output_Real`/`Integer`).
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[repr(u8)]
pub enum OutputType { Real, Integer }

macro_rules! flag_newtype {
    ($name:ident { $($cn:ident = $cv:expr),* $(,)? }) => {
        #[derive(Debug, Clone, Copy, PartialEq, Eq)]
        pub struct $name(pub u32);
        impl $name {
            $(pub const $cn: Self = Self($cv);)*
            /// Raw bits.
            #[inline] pub const fn bits(self) -> u32 { self.0 }
            /// True if all bits of `other` are set.
            #[inline] pub const fn contains(self, other: Self) -> bool { self.0 & other.0 == other.0 }
        }
    };
}

flag_newtype!(FuncFlags {
    OVERLAP = 0x0100_0000,
    STREAM = 0x0200_0000,
    VOLUME = 0x0400_0000,
    UNSTABLE_PERIOD = 0x0800_0000,
    CANDLESTICK = 0x1000_0000,
    PATH_DEPENDENT = 0x2000_0000,
});
flag_newtype!(InputFlags {
    PRICE_OPEN = 0x0000_0001,
    PRICE_HIGH = 0x0000_0002,
    PRICE_LOW = 0x0000_0004,
    PRICE_CLOSE = 0x0000_0008,
    PRICE_VOLUME = 0x0000_0010,
    PRICE_OPENINTEREST = 0x0000_0020,
    PRICE_TIMESTAMP = 0x0000_0040,
});
flag_newtype!(OptInputFlags {
    IS_PERCENT = 0x0010_0000,
    IS_DEGREE = 0x0020_0000,
    IS_CURRENCY = 0x0040_0000,
    ADVANCED = 0x0100_0000,
});
flag_newtype!(OutputFlags {
    LINE = 0x0000_0001,
    DOT_LINE = 0x0000_0002,
    DASH_LINE = 0x0000_0004,
    DOT = 0x0000_0008,
    HISTO = 0x0000_0010,
    PATTERN_BOOL = 0x0000_0020,
    PATTERN_BULL_BEAR = 0x0000_0040,
    PATTERN_STRENGTH = 0x0000_0080,
    POSITIVE = 0x0000_0100,
    NEGATIVE = 0x0000_0200,
    ZERO = 0x0000_0400,
    UPPER_LIMIT = 0x0000_0800,
    LOWER_LIMIT = 0x0000_1000,
    NULLABLE = 0x0000_2000,
});

/// A required input parameter.
#[derive(Debug, Clone, Copy)]
pub struct InputInfo {
    pub param_name: &'static str,
    pub kind: InputType,
    pub flags: InputFlags,
}

/// An output parameter.
#[derive(Debug, Clone, Copy)]
pub struct OutputInfo {
    pub param_name: &'static str,
    pub kind: OutputType,
    pub flags: OutputFlags,
}

/// What an optional input may be: the tag *and* its domain.
///
/// C keeps these apart — `TA_OptInputParameterType` names the shape and
/// `const void *dataSet` carries the values, cast by hand. Here they are fused, so the
/// pairing is checked by the compiler. Named for the C tag it replaces, matching Java's
/// `OptInputType`; C# ships this same fused shape under `OptInputDomain`.
#[derive(Debug, Clone, Copy)]
pub enum OptInputType {
    RealRange { min: f64, max: f64, precision: u8, default: f64, suggested: (f64, f64, f64) },
    IntegerRange { min: i32, max: i32, default: i32, suggested: (i32, i32, i32) },
    RealList { values: &'static [(f64, &'static str)], default: f64 },
    IntegerList { values: &'static [(i64, &'static str)], default: i64 },
}

/// An optional input parameter.
#[derive(Debug, Clone, Copy)]
pub struct OptInputInfo {
    pub param_name: &'static str,
    pub display_name: &'static str,
    pub hint: &'static str,
    pub flags: OptInputFlags,
    pub kind: OptInputType,
}

/// Metadata for one TA-Lib function (C: `TA_FuncInfo` + its parameter tables).
#[derive(Debug, Clone, Copy)]
pub struct FuncInfo {
    pub id: FuncId,
    pub name: &'static str,
    pub group: Group,
    pub hint: &'static str,
    pub flags: FuncFlags,
    pub inputs: &'static [InputInfo],
    pub opt_inputs: &'static [OptInputInfo],
    pub outputs: &'static [OutputInfo],
    /// Stable unstable-period id (the one metadata kept aligned to C); `None` if N/A.
    pub unst_id: Option<FuncUnstId>,
}

impl FuncInfo {
    #[inline] pub const fn nb_input(&self) -> usize { self.inputs.len() }
    #[inline] pub const fn nb_opt_input(&self) -> usize { self.opt_inputs.len() }
    #[inline] pub const fn nb_output(&self) -> usize { self.outputs.len() }
}

"#;
