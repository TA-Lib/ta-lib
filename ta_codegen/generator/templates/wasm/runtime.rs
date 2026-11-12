//! The module's state and the checks every `<name>_batch` export shares.
//!
//! Hand-written, not generated: this file lives in
//! `ta_codegen/generator/templates/wasm/runtime.rs` and is copied verbatim into
//! the crate by `generate`.

use std::sync::atomic::{AtomicU32, Ordering};
use std::sync::{LazyLock, Mutex, PoisonError};
use ta_lib::{Core, OutRange, RetCode};

pub(crate) static CORE: LazyLock<Core> = LazyLock::new(Core::new);

/// What an export returns when it refuses the call itself, before the crate
/// sees it. No `RetCode` negates to it.
pub(crate) const DECLINED: i32 = i32::MIN;

static OUT_RANGE: [AtomicU32; 2] = [AtomicU32::new(0), AtomicU32::new(0)];

/// Address of two `u32`: the begin index and the count of the last call that
/// succeeded. Fixed for the life of the instance.
#[no_mangle]
pub extern "C" fn out_range() -> *const u32 {
    OUT_RANGE.as_ptr().cast()
}

static BATCH_CALLS: AtomicU32 = AtomicU32::new(0);

/// The `<name>_batch` calls this instance has entered. Only the exports
/// advance it, so a caller that sees it move by one knows the call ran here.
#[no_mangle]
pub extern "C" fn batch_calls() -> u32 {
    BATCH_CALLS.load(Ordering::Relaxed)
}

pub(crate) fn entered() {
    BATCH_CALLS.fetch_add(1, Ordering::Relaxed);
}

static SCRATCH: Mutex<Vec<f64>> = Mutex::new(Vec::new());

/// At least `bytes` bytes aligned for `f64`, kept across calls. A call that has
/// to grow it moves it and may grow linear memory, so take the pointer, then
/// every view on memory, after the last `scratch` call.
#[no_mangle]
pub extern "C" fn scratch(bytes: usize) -> *mut f64 {
    let mut buf = SCRATCH.lock().unwrap_or_else(PoisonError::into_inner);
    let words = bytes.div_ceil(size_of::<f64>());
    if buf.len() < words {
        buf.resize(words, 0.0);
    }
    buf.as_mut_ptr()
}

/// `a * b + c` on run-time operands, through the instruction the crate's fused
/// sites use. Both arms must stay those of `fmadd!` in
/// `templates/rust/c_math.rs`, or the load check vouches for another one.
#[cfg(all(target_arch = "wasm32", target_feature = "relaxed-simd"))]
#[no_mangle]
pub extern "C" fn fma_probe(a: f64, b: f64, c: f64) -> f64 {
    use core::arch::wasm32::{f64x2_extract_lane, f64x2_relaxed_madd, f64x2_splat};
    f64x2_extract_lane::<0>(f64x2_relaxed_madd(f64x2_splat(a), f64x2_splat(b), f64x2_splat(c)))
}
#[cfg(not(all(target_arch = "wasm32", target_feature = "relaxed-simd")))]
#[no_mangle]
pub extern "C" fn fma_probe(a: f64, b: f64, c: f64) -> f64 {
    a.mul_add(b, c)
}

pub(crate) fn answer(result: Result<OutRange, RetCode>) -> i32 {
    match result {
        Ok(range) => {
            OUT_RANGE[0].store(range.beg_idx as u32, Ordering::Relaxed);
            OUT_RANGE[1].store(range.count as u32, Ordering::Relaxed);
            range.count as i32
        }
        Err(code) => -code.as_c_int(),
    }
}

pub(crate) fn enum_param<E: TryFrom<i32>>(value: i32, declined: &[i32]) -> Option<E> {
    if declined.contains(&value) {
        return None;
    }
    E::try_from(value).ok()
}

pub(crate) fn span<T>(ptr: *const T, len: usize) -> (usize, usize) {
    (ptr as usize, len.saturating_mul(size_of::<T>()))
}

/// Every output is disjoint from every other output and from every input:
/// a `&mut` slice overlapping any other slice is undefined behaviour that the
/// compiler optimises on, with no trap to show for it.
pub(crate) fn disjoint(outs: &[(usize, usize)], ins: &[(usize, usize)]) -> bool {
    let apart = |a: (usize, usize), b: (usize, usize)| {
        a.1 == 0 || b.1 == 0 || a.0.saturating_add(a.1) <= b.0 || b.0.saturating_add(b.1) <= a.0
    };
    outs.iter().enumerate().all(|(i, &o)| {
        outs[i + 1..].iter().all(|&p| apart(o, p)) && ins.iter().all(|&p| apart(o, p))
    })
}

fn well_formed<T>(ptr: *const T, len: usize) -> bool {
    len == 0
        || (!ptr.is_null()
            && ptr.is_aligned()
            && len <= isize::MAX as usize / size_of::<T>()
            && (ptr as usize).checked_add(len * size_of::<T>()).is_some())
}

/// # Safety
/// `ptr` must address `len` initialised elements in linear memory that no
/// `&mut` slice formed for the same call overlaps.
pub(crate) unsafe fn input<'a, T>(ptr: *const T, len: usize) -> Option<&'a [T]> {
    if !well_formed(ptr, len) {
        return None;
    }
    Some(if len == 0 { &[] } else { std::slice::from_raw_parts(ptr, len) })
}

/// # Safety
/// As [`input`], and no other slice formed for the same call may overlap it.
pub(crate) unsafe fn output<'a, T>(ptr: *mut T, len: usize) -> Option<&'a mut [T]> {
    if !well_formed(ptr, len) {
        return None;
    }
    Some(if len == 0 { &mut [] } else { std::slice::from_raw_parts_mut(ptr, len) })
}
