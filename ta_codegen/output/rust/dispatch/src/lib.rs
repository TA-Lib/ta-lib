//! Runtime CPU-feature dispatch for the `ta-lib` crate (issue #156).
//!
//! Support crate: it exists so the library crate can stay `#![forbid(unsafe_code)]`
//! while selecting hardware-FMA clones at runtime — the single `unsafe` in the
//! ta-lib workspace lives here, adjacent to the CPU-feature check that justifies
//! it. This is the Rust analogue of the C library's
//! `__attribute__((target_clones("default","fma")))` ifunc dispatch.
//! Internal contract; no semver promises outside the ta-lib workspace.

/// Dispatch one indicator call to its hardware-FMA clone when the CPU supports
/// FMA, else to the portable implementation.
///
/// Both paths compute IEEE-754 correctly-rounded fused multiply-adds (`vfmadd`
/// vs libm `fma()`), so which one runs can change speed, never bits.
///
/// # Contract (enforced by ta_codegen, not checkable by a macro)
///
/// `$fma` must name a method whose only `#[target_feature]` requirement is
/// `fma`; the generator emits the clone and this dispatch call as a pair.
#[macro_export]
macro_rules! dispatch_fma {
    ($core:expr, $fma:ident, $plain:ident, ( $($arg:expr),* $(,)? )) => {
        if std::arch::is_x86_feature_detected!("fma") {
            // SAFETY: $fma's only target_feature requirement is "fma", proven
            // present on this CPU by the guard above.
            unsafe { $core.$fma($($arg),*) }
        } else {
            $core.$plain($($arg),*)
        }
    };
}
