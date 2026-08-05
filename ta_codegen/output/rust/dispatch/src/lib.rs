// Crate docs = README.md verbatim (single source for crates.io + docs.rs;
// see README.md for the actual text).
#![doc = include_str!("../README.md")]

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
