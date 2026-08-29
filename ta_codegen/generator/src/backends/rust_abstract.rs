//! Generate `abstract_api.rs` — the Rust function metadata registry (introspection layer).
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

/// Generate `ta_codegen/output/rust/library/src/abstract_api.rs` from the function defs.
///
/// [`rows`] sorts alphabetically by name (so `FuncId` discriminants and the name
/// `match` are deterministic); this emits the registry and writes only if changed.
#[allow(clippy::implicit_hasher)]
pub fn generate(funcs: &[FuncDef], enums: &HashMap<String, EnumDef>, out_base: &Path) {
    let n = rows(funcs, enums).len();
    let o = render(funcs, enums);

    // Write the XML data file embedded by function_description_xml() via include_str!.
    // Byte-identical to the repo-root ta_func_api.xml (same generator + same input),
    // which C's TA_FunctionDescriptionXML also bakes — so the two are equal.
    let xml = super::func_api_xml::generate_string(funcs);
    let xml_path = out_base.join("rust/library/src/ta_func_api.xml");
    super::write_if_changed(&xml_path, &xml, "ta_func_api.xml (rust embed)", n);

    let out_path = out_base.join("rust/library/src/abstract_api.rs");
    super::write_if_changed(&out_path, &o, "abstract_api.rs", n);
}

/// `abstract_api.rs`'s text, without writing it — so a test can assert on the
/// generated binder without a repo checkout to read from.
#[allow(clippy::implicit_hasher)]
pub fn render(funcs: &[FuncDef], enums: &HashMap<String, EnumDef>) -> String {
    let sorted = rows(funcs, enums);
    let enum_params = enum_param_types(funcs);
    let n = sorted.len();

    let mut o = String::new();
    o.push_str(HEADER);
    // The enum types the generated thunks convert into, taken from what is
    // actually used rather than a name spelled here.
    let mut used: Vec<&String> = enum_params.values().flat_map(HashMap::values).collect();
    used.sort_unstable();
    used.dedup();
    for ty in used {
        let _ = writeln!(o, "use crate::{ty};");
    }

    // --- FuncId enum (fieldless; doubles as the dense index into FUNCS) ---
    o.push_str(
        "/// Every function in the registry, in the canonical order — and the dense\n\
         /// index into [`FUNCS`].\n\
         ///\n\
         /// Fieldless, so it is the whole handle: there is no opaque pointer to free\n\
         /// and no magic number to validate. [`FuncId::info`] is the metadata.\n",
    );
    o.push_str("#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, PartialOrd, Ord)]\n");
    o.push_str("#[repr(u16)]\n#[non_exhaustive]\n#[allow(non_camel_case_types)]\npub enum FuncId {\n");
    for f in &sorted {
        // The variant's doc is the function's own `hint` — the same string the
        // row carries into `FuncInfo::hint` and that C's `TA_FuncInfo.hint`
        // reports — so the two can never say different things about a function.
        let _ = writeln!(
            o,
            "    /// {} — [`Core::{}`](crate::Core::{}).",
            f.hint, f.name, f.name
        );
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
    emit_api(&mut o, &sorted, &enum_params);

    // --- TA_FunctionDescriptionXML analog (embeds the XML data file below) ---
    o.push_str(XML_FN);

    // --- committed regression tests for the registry's structural invariants ---
    o.push_str(REGISTRY_TESTS);

    o
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

fn emit_api(
    o: &mut String,
    sorted: &[FuncRow],
    enum_params: &HashMap<String, HashMap<String, String>>,
) {
    o.push_str(
        "/// Resolve a function name (e.g. \"RSI\") to its [`FuncId`], exact-case first.\n\
         ///\n\
         /// A generated `match` — see the module-level docs for why this is O(1) and\n\
         /// faster than C's linear scan, with zero allocation/dependencies. Private:\n\
         /// [`get_func_handle`] is the public entry, and falls back to a case-insensitive\n\
         /// scan this fast path cannot express.\n",
    );
    o.push_str("fn get_func_handle_exact(name: &str) -> Option<FuncId> {\n    Some(match name {\n");
    for f in sorted {
        let _ = writeln!(o, "        {:?} => FuncId::{},", f.name, f.name);
    }
    o.push_str("        _ => return None,\n    })\n}\n\n");

    o.push_str(
        "/// Resolve a function name (e.g. \"RSI\", \"rsi\") to its [`FuncId`].\n\
         ///\n\
         /// Every name is invariant ASCII, so an exact match (`get_func_handle_exact`,\n\
         /// O(1), zero allocation) is tried first; a caller spelling the name in any\n\
         /// other case falls back to an ASCII-only case-insensitive linear scan over\n\
         /// [`FUNCS`] (`eq_ignore_ascii_case` — not a locale-aware `to_uppercase`, which\n\
         /// has the classic Turkish-locale bug). Either way the returned [`FuncId`] and\n\
         /// its [`FuncInfo::name`] stay the canonical upper-case spelling.\n",
    );
    o.push_str("pub fn get_func_handle(name: &str) -> Option<FuncId> {\n");
    o.push_str("    if let Some(id) = get_func_handle_exact(name) {\n        return Some(id);\n    }\n");
    o.push_str("    FUNCS.iter().find(|f| f.name.eq_ignore_ascii_case(name)).map(|f| f.id)\n}\n\n");

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

    emit_binder(o, sorted, enum_params);
}


/// The hand-written half of the binder: the holder, its setters and the sealed
/// `OptValue` trait. The two dispatch matches are generated after it.
const BINDER_SCAFFOLDING: &str = r#"
use crate::{Core, OutRange, RetCode};

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
#[derive(Debug)]
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
    /// Validates every consumed component before writing any of them, so a
    /// rejection leaves the holder exactly as it found it (#266). Interleaved,
    /// this committed the components ahead of the offending one, and a caller
    /// re-binding an already-good bundle then computed over a mixture of the two
    /// -- the holder-reusability rule the `call` tier states for itself.
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
        for (i, series) in given.iter().enumerate() {
            if flags.0 & (1u32 << i) != 0 && series.is_none() {
                return Err(RetCode::BadParam);
            }
        }
        self.price[slot] = given;
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
                        assert_eq!(ru[k][..r.count], re[k][..r.count],
                                   "{} output {k}", f.name);
                    } else {
                        assert_eq!(iu[k][..r.count], ie[k][..r.count],
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

    /// The check C cannot perform: its setters take a bare pointer, so an
    /// undersized output is an out-of-bounds write there. A slice carries its length.
    ///
    /// Since #265 the bound is the public entry point's, not this tier's own, so
    /// what is asserted here is that binding through the catalogue answers what
    /// calling `Core::SMA` directly answers — which is also what Java's binder
    /// and C's frames have always done.
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

    /// An INPUT shorter than the requested range, which nothing reached before
    /// #265: this tier checked outputs only, so a short leg went straight into
    /// the numerics and tripped their assert — a panic out of a `Result`-typed
    /// method. Java answers `BadParam` (its binder calls the public tier) and so
    /// does C# since #265; C cannot express the case at all, its setters taking
    /// a bare pointer.
    #[test]
    fn an_undersized_input_is_rejected_not_read_past() {
        let core = Core::new();
        let close = series(0.0);
        let mut out = vec![0.0; N];
        let mut h = FuncId::SMA.new_call(&core);
        h.set_input(0, &close[..N / 2]).unwrap();
        h.set_opt(0, 30_i32).unwrap();
        h.set_output(0, &mut out).unwrap();
        assert_eq!(h.call(0, N - 1), Err(RetCode::BadParam));

        // Control: the same call over a range the leg does cover succeeds, so
        // the rejection above is the length and not the binding.
        let mut h = FuncId::SMA.new_call(&core);
        h.set_input(0, &close[..N / 2]).unwrap();
        h.set_opt(0, 30_i32).unwrap();
        h.set_output(0, &mut out).unwrap();
        assert!(h.call(0, N / 2 - 1).is_ok());
    }

    /// An output sized to the count the call PRODUCES is enough — the bound is
    /// B5's, `endIdx - max(startIdx, lookback) + 1`, not the width of the
    /// requested range. Demanding the latter rejects a caller who allocated by
    /// the published formula (#265).
    #[test]
    fn an_output_sized_to_the_produced_count_is_enough() {
        let core = Core::new();
        let close = series(0.0);
        let lookback = core.SMA_Lookback(30).unwrap();
        let mut exact = vec![0.0; N - lookback];
        let mut h = FuncId::SMA.new_call(&core);
        h.set_input(0, &close).unwrap();
        h.set_opt(0, 30_i32).unwrap();
        h.set_output(0, &mut exact).unwrap();
        assert_eq!(h.call(0, N - 1), Ok(OutRange { beg_idx: lookback, count: N - lookback }));
    }

    /// A rejected call leaves the holder as it found it. The outputs are moved
    /// out of it for the duration of one call, and every exit has to put them
    /// back — otherwise a caller who corrects the mistake and calls again is told
    /// their bound output is missing, forever.
    #[test]
    fn a_rejected_call_leaves_the_holder_reusable() {
        let core = Core::new();
        let close = series(0.0);
        let mut a = vec![0.0; N];
        let mut b = vec![0.0; N];
        let mut c = vec![0.0; N];

        // Multi-output, so the arm takes more than one buffer: rejected on the
        // second, the first must not stay out of the holder.
        let mut h = FuncId::ACCBANDS.new_call(&core);
        h.set_price_input(0, Some(&close), Some(&close), Some(&close),
                          Some(&close), Some(&close), Some(&close)).unwrap();
        h.set_output(0, &mut a).unwrap();
        h.set_output(2, &mut c).unwrap();
        assert_eq!(h.call(0, N - 1), Err(RetCode::BadParam), "output 1 is unbound");
        h.set_output(1, &mut b).unwrap();
        assert!(h.call(0, N - 1).is_ok(), "the corrected holder still works");
    }

    /// A rejected SETTER leaves the holder as it found it, which is the other
    /// half of the rule the test above pins for a rejected call (#266).
    ///
    /// The sharp case is a RE-bind, not a first bind: on a fresh holder the
    /// partial write is masked, because the arm's `.ok_or(BadParam)?` reports the
    /// component that was never set. Over a bundle that already works, an
    /// interleaved check-and-write committed the components ahead of the
    /// offending one and left the rest holding the previous bundle, so the next
    /// `call` succeeded over a mixture of the two.
    ///
    /// Each holder is scoped because it borrows its output buffer for its whole
    /// life; the buffers are compared after the scopes close.
    #[test]
    fn a_rejected_setter_leaves_the_holder_as_it_found_it() {
        let core = Core::new();
        // WILLR consumes High|Low|Close (InputFlags 0x0e), so `close` is the last
        // required component and the natural place to trip the setter.
        let high = series(0.0);
        let low: Vec<f64> = high.iter().map(|v| v - 4.0).collect();
        let close: Vec<f64> = high.iter().map(|v| v - 2.0).collect();
        // A different PHASE, not a shift: WILLR is (hh - c) / (hh - ll), which a
        // uniform offset leaves unchanged -- the control below would then pass on
        // a setter that did nothing at all.
        let high2 = series(1.3);
        let low2: Vec<f64> = high2.iter().map(|v| v - 4.0).collect();
        let close2: Vec<f64> = high2.iter().map(|v| v - 2.0).collect();

        let bind = |h: &mut ParamHolder<'_>| {
            h.set_opt(0, 14_i32).unwrap();
        };

        let mut reference = vec![0.0; N];
        let mut after_reject = vec![0.0; N];
        let mut after_rebind = vec![0.0; N];

        // What a correctly bound holder produces.
        let want = {
            let mut h = FuncId::WILLR.new_call(&core);
            h.set_price_input(0, None, Some(&high), Some(&low), Some(&close), None, None).unwrap();
            bind(&mut h);
            h.set_output(0, &mut reference).unwrap();
            h.call(0, N - 1).expect("the reference bind computes")
        };
        assert!(want.count > 0, "the reference call produced values");

        // The same bind, then a REJECTED rebind that supplies high and low and
        // forgets close, then the same call again.
        let got = {
            let mut h = FuncId::WILLR.new_call(&core);
            h.set_price_input(0, None, Some(&high), Some(&low), Some(&close), None, None).unwrap();
            bind(&mut h);
            h.set_output(0, &mut after_reject).unwrap();
            h.call(0, N - 1).unwrap();
            assert_eq!(
                h.set_price_input(0, None, Some(&high2), Some(&low2), None, None, None).err(),
                Some(RetCode::BadParam),
                "close is consumed and was not supplied"
            );
            h.call(0, N - 1).expect("the holder still computes")
        };
        assert_eq!((got.beg_idx, got.count), (want.beg_idx, want.count));
        for i in 0..want.count {
            assert_eq!(
                after_reject[i].to_bits(),
                reference[i].to_bits(),
                "a rejected setter changed what the holder computes, at output[{i}]"
            );
        }

        // Control: a CORRECT rebind must reach the output, or the assertion above
        // passes for a setter that stopped working altogether.
        {
            let mut h = FuncId::WILLR.new_call(&core);
            h.set_price_input(0, None, Some(&high), Some(&low), Some(&close), None, None).unwrap();
            bind(&mut h);
            h.set_output(0, &mut after_rebind).unwrap();
            h.call(0, N - 1).unwrap();
            h.set_price_input(0, None, Some(&high2), Some(&low2), Some(&close2), None, None)
                .unwrap();
            h.call(0, N - 1).unwrap();
        }
        assert!(
            (0..want.count).any(|i| after_rebind[i].to_bits() != reference[i].to_bits()),
            "a correct rebind must reach the output"
        );
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
fn emit_binder(
    o: &mut String,
    sorted: &[FuncRow],
    enum_params: &HashMap<String, HashMap<String, String>>,
) {
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
         \x20       match self.func {\n",
    );
    for f in sorted {
        let snake = f.name.clone();
        let args = opt_args(f, enum_params);
        let _ = writeln!(
            o,
            "            FuncId::{} => self.core.{snake}_Lookback({args}),",
            f.name
        );
    }
    o.push_str(
        "        }\n\
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
         \x20   /// [`Core::MAX_INDEX`], [`RetCode::OutOfRangeEndIndex`] if `end_idx` exceeds\n\
         \x20   /// it or is below `start_idx`, and [`RetCode::BadParam`] if a required\n\
         \x20   /// input or output was never bound, if the function rejects its\n\
         \x20   /// parameters, or if a bound buffer is too short: every input must\n\
         \x20   /// cover `end_idx`, and every output must hold the count actually\n\
         \x20   /// produced, `end_idx - max(start_idx, lookback) + 1`.\n\
         \x20   pub fn call(&mut self, start_idx: usize, end_idx: usize) -> Result<OutRange, RetCode> {\n\
         \x20       if start_idx > Core::MAX_INDEX { return Err(RetCode::OutOfRangeStartIndex); }\n\
         \x20       if end_idx > Core::MAX_INDEX || end_idx < start_idx {\n\
         \x20           return Err(RetCode::OutOfRangeEndIndex);\n\
         \x20       }\n\
         \x20       // The buffer bounds are the PUBLIC entry point's, which every arm\n\
         \x20       // below calls (#265). This tier used to hand `_Impl` a hand-rolled\n\
         \x20       // output check of its own, `end_idx - start_idx + 1` -- the width of\n\
         \x20       // the REQUESTED range, where B5 says the count actually PRODUCED, so\n\
         \x20       // it rejected a caller who sized by the published formula on a range\n\
         \x20       // starting below the lookback. It also checked no input at all, which\n\
         \x20       // is what let a short leg reach the numerics and panic. One bound, in\n\
         \x20       // the tier that states it, and Java's binder and C's frames have\n\
         \x20       // always reached the same one.\n\
         \x20       let mut beg: usize = 0;\n\
         \x20       let mut nb: usize = 0;\n\
         \x20       let rc = match self.func {\n",
    );
    for f in sorted {
        emit_call_arm(o, f, enum_params);
    }
    o.push_str(
        "        };\n\
         \x20       if rc == RetCode::Success { Ok(OutRange { beg_idx: beg, count: nb }) } else { Err(rc) }\n\
         \x20   }\n}\n",
    );

    o.push_str(BINDER_TESTS);
}

/// `function -> parameter -> enum type`, for the optional parameters the Rust
/// API types as an enum.
///
/// The row model describes the C metadata, where a choice list is just a list of
/// ints, so it carries no enum name — but the generated call needs the Rust
/// type. Taking it from the `FuncDef` keeps that a codegen concern instead of
/// widening a model three backends share.
fn enum_param_types(funcs: &[FuncDef]) -> HashMap<String, HashMap<String, String>> {
    funcs
        .iter()
        .map(|f| {
            let params = f
                .optional_inputs
                .iter()
                .filter_map(|o| match &o.param_type {
                    crate::ir::ParamType::Enum(n) => Some((o.name.clone(), n.clone())),
                    _ => None,
                })
                .collect();
            (f.name.clone(), params)
        })
        .collect()
}

/// The optional-parameter argument list for one function, in declaration order.
fn opt_args(f: &FuncRow, enum_params: &HashMap<String, HashMap<String, String>>) -> String {
    f.opt_inputs
        .iter()
        .enumerate()
        .map(|(i, opt)| opt_arg(f, opt, i, enum_params))
        .collect::<Vec<_>>()
        .join(", ")
}

/// One optional-parameter argument.
///
/// An enum-typed parameter converts here, and the conversion is fallible on
/// purpose: the holder stores whatever integer was bound (as C's does), so an
/// out-of-domain value must be rejected rather than coerced into a member.
/// `?` carries that out as `BadParam`.
///
/// This diverges from C at the lookback tier, unavoidably: `TA_GetLookback`
/// computes a real number for an out-of-domain MAType, which a typed parameter
/// cannot express. Java rejects such a value even earlier, at bind.
fn opt_arg(
    f: &FuncRow,
    opt: &OptRow,
    i: usize,
    enum_params: &HashMap<String, HashMap<String, String>>,
) -> String {
    match &opt.domain {
        OptDomain::RealRange { .. } | OptDomain::RealList { .. } => format!("self.real_opt[{i}]"),
        _ => match enum_params.get(&f.name).and_then(|m| m.get(&opt.param_name)) {
            Some(ty) => format!("{ty}::try_from(self.int_opt[{i}])?"),
            None => format!("self.int_opt[{i}]"),
        },
    }
}

/// One `call` match arm. Inputs are read by value (the slices are `Copy` out of
/// their `Option`), outputs are `take`n so the borrow checker sees no overlap
/// with `&self`, reborrowed into the call, and put straight back — so a holder
/// stays reusable, as C's does.
///
/// The callee is the **public** entry point (#265), so the arm converts its
/// `Result` back into the `RetCode` `call` accumulates. The restore has to
/// happen on both sides of that conversion: an `Err` still owes the holder its
/// output bindings.
fn emit_call_arm(
    o: &mut String,
    f: &FuncRow,
    enum_params: &HashMap<String, HashMap<String, String>>,
) {
    let snake = f.name.clone();
    let _ = writeln!(o, "            FuncId::{} => {{", f.name);

    // Enum conversions happen FIRST, before any output is `take`n. A `?` after
    // the take would leave the holder without its output bindings, so a caller
    // that bound a bad value and then corrected it would get BadParam forever --
    // C keeps the holder reusable, and so must this.
    let mut enum_binds: HashMap<String, String> = HashMap::new();
    for (i, opt) in f.opt_inputs.iter().enumerate() {
        if let Some(ty) = enum_params.get(&f.name).and_then(|m| m.get(&opt.param_name)) {
            let bind = format!("e{i}");
            let _ = writeln!(
                o,
                "                let {bind} = {ty}::try_from(self.int_opt[{i}])?;"
            );
            enum_binds.insert(opt.param_name.clone(), bind);
        }
    }

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
        args.push(match enum_binds.get(&opt.param_name) {
            Some(bind) => bind.clone(),
            None => opt_arg(f, opt, i, enum_params),
        });
    }
    // Every output's presence, decided before the first `take`. With more than
    // one output a `?` between the takes would return with the earlier ones
    // already out of the holder and never put back, and the next `call` would
    // then answer `BadParam` forever on a binding the caller can see is there.
    // Same reasoning as the enum conversions above; C keeps the holder reusable.
    if f.outputs.len() > 1 {
        let bound: Vec<String> = f
            .outputs
            .iter()
            .enumerate()
            .map(|(k, out)| {
                let arr = match out.kind {
                    OutputKind::Integer => "int_out",
                    OutputKind::Real => "real_out",
                };
                format!("self.{arr}[{k}].is_none()")
            })
            .collect();
        let _ = writeln!(
            o,
            "                if {} {{ return Err(RetCode::BadParam); }}",
            bound.join(" || ")
        );
    }

    for (k, out) in f.outputs.iter().enumerate() {
        let arr = match out.kind {
            OutputKind::Integer => "int_out",
            OutputKind::Real => "real_out",
        };
        let _ = writeln!(
            o,
            "                let mut o{k} = self.{arr}[{k}].take().ok_or(RetCode::BadParam)?;"
        );
        // The abstract tier always supplies every declared output, so a
        // `nullable` one (rule B6a, `TA_OUT_NULLABLE`) is handed `Some(..)`
        // rather than declined: the catalogue's job is to reproduce the direct
        // call, not to choose for the caller.
        args.push(if out.flags & super::abstract_rows::OUT_NULLABLE == 0 {
            format!("&mut *o{k}")
        } else {
            format!("Some(&mut *o{k})")
        });
    }

    let _ = writeln!(o, "                let res = self.core.{snake}({});", args.join(", "));
    for (k, out) in f.outputs.iter().enumerate() {
        let arr = match out.kind {
            OutputKind::Integer => "int_out",
            OutputKind::Real => "real_out",
        };
        let _ = writeln!(o, "                self.{arr}[{k}] = Some(o{k});");
    }
    o.push_str(
        "                match res {\n\
         \x20                   Ok(r) => { beg = r.beg_idx; nb = r.count; RetCode::Success }\n\
         \x20                   Err(e) => e,\n\
         \x20               }\n            }\n",
    );
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

    /// #278: the lookup folds ASCII case, but every name it hands back
    /// (`FuncId`, `FuncInfo::name`) stays the canonical upper-case spelling.
    #[test]
    fn lookup_is_ascii_case_insensitive() {
        for f in FUNCS.iter() {
            assert_eq!(get_func_handle(&f.name.to_ascii_lowercase()), Some(f.id));
        }
        assert_eq!(get_func_handle("sma"), Some(FuncId::SMA));
        assert_eq!(get_func_handle("Sma"), Some(FuncId::SMA));
        assert_eq!(get_func_handle("sMa"), Some(FuncId::SMA));
        assert_eq!(get_func_info(get_func_handle("sma").unwrap()).name, "SMA");

        // Alternating case, swept over the corpus: between this spelling and
        // the all-lower one above, every letter position of every name is
        // presented in both cases, so a fold applied to only part of the name
        // fails here. The long names and the ones carrying a digit or an
        // underscore (`CDL3STARSINSOUTH`, `CDL2CROWS`, `HT_DCPERIOD`) are the
        // ones a partial fold gets wrong, and no single probe stands in for
        // them.
        for f in FUNCS.iter() {
            let mixed: String = f
                .name
                .chars()
                .enumerate()
                .map(|(i, c)| {
                    if i % 2 == 0 { c.to_ascii_lowercase() } else { c.to_ascii_uppercase() }
                })
                .collect();
            assert_eq!(get_func_handle(&mixed), Some(f.id), "mixed-case lookup of {}", f.name);
            // The lookup answers a `FuncId`, so the spelling asked for cannot
            // reach the name reported back: comparing that name to `f.name`
            // would restate the table's index alignment, not the fold. What is
            // worth pinning is the half that makes "canonical" mean something --
            // the stored spelling is the upper-case one the fold folds onto.
            assert!(
                !f.name.chars().any(|c| c.is_ascii_lowercase()),
                "{} is not stored in its canonical upper case",
                f.name
            );
        }
    }

    /// #278: the fold is ASCII-only, and it is only a fold — not a
    /// normalisation that would start resolving names no function has.
    #[test]
    fn the_fold_does_not_widen_what_resolves() {
        // The two traps a `to_uppercase`-based fold falls into, in either
        // direction: `U+0130`/`U+0131` are the Turkish dotted/dotless `I`, and
        // `U+017F` is the long `s`. Unicode uppercases the latter two onto
        // ASCII `I` and `S`, so a locale- or Unicode-aware fold resolves
        // `"s\u{131}n"` to SIN and `"\u{17f}ma"` to SMA. An ASCII fold does not.
        assert_eq!(get_func_handle("S\u{130}N"), None);
        assert_eq!(get_func_handle("s\u{131}n"), None);
        assert_eq!(get_func_handle("\u{17f}ma"), None);
        // Length, padding and separators are still part of the name.
        assert_eq!(get_func_handle("sma "), None);
        assert_eq!(get_func_handle(" sma"), None);
        assert_eq!(get_func_handle("ht-dcperiod"), None);
        assert_eq!(get_func_handle(""), None);
        // Longer than any name: no truncating match.
        assert_eq!(get_func_handle(&"s".repeat(512)), None);
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
//!  * `get_func_handle` tries this exact match first, and only falls back to an
//!    O(n) ASCII case-insensitive scan over [`FUNCS`] on a miss (#278) — a cold
//!    path within a cold path, so the fallback's linearity doesn't matter.
#![allow(clippy::all)]
#![allow(non_camel_case_types)]

use crate::FuncUnstId;

";

const MODEL: &str = r#"/// Function group (closed set — replaces C's runtime group-string table + linear `getGroupId`).
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[repr(u8)]
#[non_exhaustive]
pub enum Group {
    /// `Cycle Indicators` — the Hilbert Transform family.
    CycleIndicators,
    /// `Math Operators` — arithmetic and rolling aggregates over a series.
    MathOperators,
    /// `Math Transform` — element-wise transcendental and rounding functions.
    MathTransform,
    /// `Momentum Indicators` — rate-of-change and oscillator studies.
    MomentumIndicators,
    /// `Overlap Studies` — studies drawn on the price scale itself.
    OverlapStudies,
    /// `Pattern Recognition` — the `CDL*` candlestick recognizers.
    PatternRecognition,
    /// `Price Transform` — a single bar's OHLC reduced to one price.
    PriceTransform,
    /// `Statistic Functions` — regression and distribution measures.
    StatisticFunctions,
    /// `Volatility Indicators` — true-range derived measures.
    VolatilityIndicators,
    /// `Volume Indicators` — studies that read the volume series.
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
pub enum InputType {
    /// One or more OHLCV components of the same bar series; which ones is in
    /// [`InputInfo::flags`].
    Price,
    /// A single `&[f64]` series.
    Real,
    /// A single integer series.
    Integer,
}

/// Output data kind (C: `TA_Output_Real`/`Integer`).
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[repr(u8)]
pub enum OutputType {
    /// Written into an `&mut [f64]`.
    Real,
    /// Written into an `&mut [i32]` — the `CDL*` patterns and the `*INDEX` studies.
    Integer,
}

macro_rules! flag_newtype {
    ($(#[$sm:meta])* $name:ident { $($(#[$cm:meta])* $cn:ident = $cv:expr),* $(,)? }) => {
        $(#[$sm])*
        #[derive(Debug, Clone, Copy, PartialEq, Eq)]
        pub struct $name(
            /// The raw flag word, with the same bit values as the matching C
            /// `#define`s in `ta_abstract.h`.
            pub u32
        );
        impl $name {
            $($(#[$cm])* pub const $cn: Self = Self($cv);)*
            /// Raw bits.
            #[inline] pub const fn bits(self) -> u32 { self.0 }
            /// True if all bits of `other` are set.
            #[inline] pub const fn contains(self, other: Self) -> bool { self.0 & other.0 == other.0 }
        }
    };
}

flag_newtype!(
    /// What a function is, for a caller deciding how to call or plot it
    /// (C: `TA_FuncFlags`).
    FuncFlags {
    /// Output is on the same scale as the input data, so it can be drawn over
    /// the price series.
    OVERLAP = 0x0100_0000,
    /// The function also has a streaming tier — `<F>_Open` / `Update` /
    /// `Peek` / `Close` — bit-identical to the batch function.
    STREAM = 0x0200_0000,
    /// Output is over the volume data rather than the price scale.
    VOLUME = 0x0400_0000,
    /// The function has an unstable initial period, settable through
    /// [`CoreBuilder::unstable_period`](crate::CoreBuilder::unstable_period).
    UNSTABLE_PERIOD = 0x0800_0000,
    /// Output is a candlestick-pattern verdict.
    CANDLESTICK = 0x1000_0000,
    /// Output is path-dependent: built up from the first bar, so it depends on
    /// the requested `startIdx` and never converges across ranges — the same bar
    /// computed from a different `startIdx` can differ. E.g. AD, ADOSC, OBV,
    /// NVI, PVI, SAR, SAREXT.
    PATH_DEPENDENT = 0x2000_0000,
    /// Inputs of ordinary magnitude can have no finite result, so a successful
    /// call may write NaN or ±Inf (e.g. ACOS outside `[-1, 1]`, LN of zero,
    /// `0/0`). Not set where a non-finite value needs magnitudes large enough to
    /// overflow the intermediate arithmetic. Set on ACOS, ASIN, DIV, LN, LOG10,
    /// SQRT and VWMA, and on no others.
    NAN_INF_OUTPUT = 0x4000_0000,
    /// A period of 1 performs no smoothing: the lookback is 0 and every output
    /// value is a bit-exact copy of its input value.
    PERIOD1_IDENTITY = 0x0000_0001,
});
flag_newtype!(
    /// Which OHLCV components an [`InputType::Price`] input reads
    /// (C: the `TA_IN_PRICE_*` bits).
    InputFlags {
    /// Reads the open price.
    PRICE_OPEN = 0x0000_0001,
    /// Reads the high price.
    PRICE_HIGH = 0x0000_0002,
    /// Reads the low price.
    PRICE_LOW = 0x0000_0004,
    /// Reads the close price.
    PRICE_CLOSE = 0x0000_0008,
    /// Reads the volume.
    PRICE_VOLUME = 0x0000_0010,
    /// Reads the open interest. No shipped function sets this.
    PRICE_OPENINTEREST = 0x0000_0020,
    /// Reads the timestamp. No shipped function sets this.
    PRICE_TIMESTAMP = 0x0000_0040,
});
flag_newtype!(
    /// How a UI should present an optional input (C: the `TA_OPTIN_*` bits).
    OptInputFlags {
    /// The value is a percentage.
    IS_PERCENT = 0x0010_0000,
    /// The value is a degree, in `0..=360`.
    IS_DEGREE = 0x0020_0000,
    /// The value is a currency amount.
    IS_CURRENCY = 0x0040_0000,
    /// The parameter is for advanced users; a UI may hide it by default.
    ADVANCED = 0x0100_0000,
});
flag_newtype!(
    /// How an output should be drawn, and what its values can be
    /// (C: the `TA_OUT_*` bits).
    OutputFlags {
    /// Suggest displaying as a connected line.
    LINE = 0x0000_0001,
    /// Suggest displaying as a dotted line.
    DOT_LINE = 0x0000_0002,
    /// Suggest displaying as a dashed line.
    DASH_LINE = 0x0000_0004,
    /// Suggest displaying with dots only.
    DOT = 0x0000_0008,
    /// Suggest displaying as a histogram.
    HISTO = 0x0000_0010,
    /// The value says whether the pattern exists: non-zero yes, zero no.
    PATTERN_BOOL = 0x0000_0020,
    /// Zero is no pattern, positive is bullish, negative is bearish.
    PATTERN_BULL_BEAR = 0x0000_0040,
    /// Zero is neutral; `]0..100]` getting bullish, `]100..200]` bullish,
    /// `[-100..0[` getting bearish, `[-200..-100[` bearish.
    PATTERN_STRENGTH = 0x0000_0080,
    /// The output can be positive.
    POSITIVE = 0x0000_0100,
    /// The output can be negative.
    NEGATIVE = 0x0000_0200,
    /// The output can be zero.
    ZERO = 0x0000_0400,
    /// The values represent an upper limit.
    UPPER_LIMIT = 0x0000_0800,
    /// The values represent a lower limit.
    LOWER_LIMIT = 0x0000_1000,
    /// The caller may discard this output — it is computed but need not be
    /// kept. E.g. MAMA's FAMA line when only the MAMA line is wanted.
    NULLABLE = 0x0000_2000,
});

/// A required input parameter.
#[derive(Debug, Clone, Copy)]
pub struct InputInfo {
    /// The parameter's name in the generated signature, e.g. `inReal`.
    pub param_name: &'static str,
    /// Which shape the input has.
    pub kind: InputType,
    /// For an [`InputType::Price`], the OHLCV components it reads; empty otherwise.
    pub flags: InputFlags,
}

/// An output parameter.
#[derive(Debug, Clone, Copy)]
pub struct OutputInfo {
    /// The parameter's name in the generated signature, e.g. `outReal`.
    pub param_name: &'static str,
    /// Which element type the output is written as.
    pub kind: OutputType,
    /// Plotting and value-domain hints for this output.
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
    /// A real parameter, valid anywhere in `min..=max`.
    RealRange {
        /// Smallest accepted value.
        min: f64,
        /// Largest accepted value.
        max: f64,
        /// Digits after the decimal point a UI should display.
        precision: u8,
        /// The value [`Core::REAL_DEFAULT`](crate::Core::REAL_DEFAULT) selects.
        default: f64,
        /// Three values worth offering, from low to high.
        suggested: (f64, f64, f64),
    },
    /// An integer parameter, valid anywhere in `min..=max`.
    IntegerRange {
        /// Smallest accepted value.
        min: i32,
        /// Largest accepted value.
        max: i32,
        /// The value [`Core::INTEGER_DEFAULT`](crate::Core::INTEGER_DEFAULT) selects.
        default: i32,
        /// Three values worth offering, from low to high.
        suggested: (i32, i32, i32),
    },
    /// A real parameter drawn from a closed list.
    RealList {
        /// The accepted values, each with its display string.
        values: &'static [(f64, &'static str)],
        /// The value [`Core::REAL_DEFAULT`](crate::Core::REAL_DEFAULT) selects.
        default: f64,
    },
    /// An integer parameter drawn from a closed list — a moving-average type, say.
    IntegerList {
        /// The accepted values, each with its display string.
        values: &'static [(i64, &'static str)],
        /// The value [`Core::INTEGER_DEFAULT`](crate::Core::INTEGER_DEFAULT) selects.
        default: i64,
    },
}

/// An optional input parameter.
#[derive(Debug, Clone, Copy)]
pub struct OptInputInfo {
    /// The parameter's name in the generated signature, e.g. `optInTimePeriod`.
    pub param_name: &'static str,
    /// A short label for a UI, e.g. `Time Period`.
    pub display_name: &'static str,
    /// One line describing what the parameter does.
    pub hint: &'static str,
    /// How a UI should present the value.
    pub flags: OptInputFlags,
    /// The parameter's shape and its domain.
    pub kind: OptInputType,
}

/// Metadata for one TA-Lib function (C: `TA_FuncInfo` + its parameter tables).
#[derive(Debug, Clone, Copy)]
pub struct FuncInfo {
    /// This function's id — also its index into [`FUNCS`].
    pub id: FuncId,
    /// Upper-case TA name, e.g. `"RSI"`.
    pub name: &'static str,
    /// The group the function is filed under.
    pub group: Group,
    /// One line describing what the function computes.
    pub hint: &'static str,
    /// What the function is, for a caller deciding how to call or plot it.
    pub flags: FuncFlags,
    /// Required inputs, in call order.
    pub inputs: &'static [InputInfo],
    /// Optional inputs, in call order.
    pub opt_inputs: &'static [OptInputInfo],
    /// Outputs, in call order.
    pub outputs: &'static [OutputInfo],
    /// Stable unstable-period id (the one metadata kept aligned to C); `None` if N/A.
    pub unst_id: Option<FuncUnstId>,
}

impl FuncInfo {
    /// Number of required inputs.
    #[inline] pub const fn nb_input(&self) -> usize { self.inputs.len() }
    /// Number of optional inputs.
    #[inline] pub const fn nb_opt_input(&self) -> usize { self.opt_inputs.len() }
    /// Number of outputs.
    #[inline] pub const fn nb_output(&self) -> usize { self.outputs.len() }
}

"#;
