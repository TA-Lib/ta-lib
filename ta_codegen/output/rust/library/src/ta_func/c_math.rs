//! C's `max`, `min`, `floor` and `ceil`, bit for bit, inline where gcc inlines
//! them.
//!
//! Hand-written, not generated: this file lives in
//! `ta_codegen/generator/templates/rust/c_math.rs` and is copied verbatim into
//! the crate by `generate`.

/// C's `max(a, b)` macro, `a > b ? a : b`: on a tie or a NaN it answers `b`,
/// where `f64::max` answers the non-NaN operand. One MAXSD.
#[inline(always)]
pub(crate) fn c_max(a: f64, b: f64) -> f64 {
    // An if-expression is lowered as control flow, and LLVM leaves it a branch
    // when its join is shared with an enclosing `if`.
    core::hint::select_unpredictable(a > b, a, b)
}

/// C's `min(a, b)` macro, `a < b ? a : b`. One MINSD.
#[inline(always)]
pub(crate) fn c_min(a: f64, b: f64) -> f64 {
    core::hint::select_unpredictable(a < b, a, b)
}

/// Without SSE4.1 `f64::floor` is a libm call per element; gcc inlines it.
#[inline(always)]
#[allow(clippy::disallowed_methods)]
pub(crate) fn c_floor(x: f64) -> f64 {
    #[cfg(all(target_arch = "x86_64", not(target_feature = "sse4.1")))]
    {
        const TWO52: f64 = 4_503_599_627_370_496.0;
        // Zero at |x| >= 2^52, inf and NaN, where x is already integral.
        let c = (if x.abs() < TWO52 { TWO52 } else { 0.0 }).copysign(x);
        let r = (x + c) - c;
        let f = r - if r > x { 1.0 } else { 0.0 };
        // Restores -0.0 and returns a NaN with its own bits.
        if f < x { f } else { x }
    }
    #[cfg(not(all(target_arch = "x86_64", not(target_feature = "sse4.1"))))]
    {
        x.floor()
    }
}

/// `-floor(-x)`, spelled so its last select is one MAXSD.
#[inline(always)]
#[allow(clippy::disallowed_methods)]
pub(crate) fn c_ceil(x: f64) -> f64 {
    #[cfg(all(target_arch = "x86_64", not(target_feature = "sse4.1")))]
    {
        const TWO52: f64 = 4_503_599_627_370_496.0;
        let y = -x;
        let c = (if y.abs() < TWO52 { TWO52 } else { 0.0 }).copysign(y);
        let r = (y + c) - c;
        let f = -(r - if r > y { 1.0 } else { 0.0 });
        if f > x { f } else { x }
    }
    #[cfg(not(all(target_arch = "x86_64", not(target_feature = "sse4.1"))))]
    {
        x.ceil()
    }
}

#[cfg(test)]
// `expect`, not `allow`: an emptied clippy.toml fails the lint run here.
#[expect(clippy::disallowed_methods)]
mod tests {
    use super::{c_ceil, c_floor, c_max, c_min};

    const TWO52: f64 = 4_503_599_627_370_496.0;

    /// A quiet NaN comes back with its own bits (payload and sign), as from
    /// gcc's inline floor. A signalling one may come back quieted, as from
    /// ROUNDSD on an SSE4.1 build.
    fn check(x: f64) {
        for (name, got, want) in [("c_floor", c_floor(x), x.floor()), ("c_ceil", c_ceil(x), x.ceil())] {
            if x.is_nan() {
                assert!(got.is_nan(), "{name}({:#018x}) must be NaN", x.to_bits());
                if x.to_bits() & (1 << 51) != 0 {
                    assert_eq!(got.to_bits(), x.to_bits(), "{name}({:#018x}) must keep the NaN's bits", x.to_bits());
                }
            } else {
                assert_eq!(got.to_bits(), want.to_bits(), "{name}({x:e} = {:#018x})", x.to_bits());
            }
        }
    }

    struct SplitMix(u64);
    impl SplitMix {
        fn next(&mut self) -> u64 {
            self.0 = self.0.wrapping_add(0x9E37_79B9_7F4A_7C15);
            let mut z = self.0;
            z = (z ^ (z >> 30)).wrapping_mul(0xBF58_476D_1CE4_E5B9);
            z = (z ^ (z >> 27)).wrapping_mul(0x94D0_49BB_1331_11EB);
            z ^ (z >> 31)
        }
    }

    #[test]
    fn floor_and_ceil_match_libm_on_special_values() {
        let bits = [
            0x0000_0000_0000_0000_u64, 0x8000_0000_0000_0000, // +-0
            0x7FF0_0000_0000_0000, 0xFFF0_0000_0000_0000, // +-inf
            0x7FF8_0000_0000_0000, 0xFFF8_0000_0000_0001, // quiet NaN
            0x7FF0_0000_0000_0001, 0xFFF4_0000_DEAD_BEEF, // signalling NaN
            0x0000_0000_0000_0001, 0x8000_0000_0000_0001, // +-min subnormal
            0x000F_FFFF_FFFF_FFFF, 0x800F_FFFF_FFFF_FFFF, // +-max subnormal
        ];
        for b in bits {
            check(f64::from_bits(b));
        }
        for v in [
            f64::MIN_POSITIVE, f64::MAX, f64::EPSILON, 0.5, 1.0, 1.5, 2.5, 0.499_999_999_999_999_94,
            0.999_999_999_999_999_9, TWO52 / 2.0 - 0.5, TWO52 - 0.5, TWO52 - 1.0, TWO52, TWO52 + 1.0,
            TWO52 * 2.0 - 1.0, TWO52 * 2.0, TWO52 * 2.0 + 2.0,
        ] {
            check(v);
            check(-v);
        }
    }

    #[test]
    fn floor_and_ceil_match_libm_across_every_exponent() {
        let mut rng = SplitMix(0x5EED);
        for sign in [0_u64, 1] {
            for exp in 0..2048_u64 {
                for m in 0..64_u64 {
                    for mant in [m, (1_u64 << 52) - 1 - m, rng.next() >> 12] {
                        check(f64::from_bits(sign << 63 | exp << 52 | mant));
                    }
                }
            }
        }
    }

    #[test]
    fn floor_and_ceil_match_libm_next_to_integers_and_two_to_the_52() {
        let mut centers: Vec<f64> = (0..4096).flat_map(|k| [f64::from(k), f64::from(k) + 0.5]).collect();
        centers.extend([TWO52 / 2.0, TWO52, TWO52 * 2.0]);
        for c in centers {
            for c in [c, -c] {
                let b = c.to_bits();
                for j in 0..64_u64 {
                    check(f64::from_bits(b + j));
                    if b & !(1 << 63) >= j {
                        check(f64::from_bits(b - j));
                    }
                }
            }
        }
    }

    #[test]
    fn floor_and_ceil_match_libm_on_random_bit_patterns() {
        let mut rng = SplitMix(0xF100_0C);
        for _ in 0..1_000_000 {
            check(f64::from_bits(rng.next()));
        }
    }

    #[test]
    fn max_and_min_answer_like_the_c_macros() {
        let nan = f64::NAN;
        assert!(c_max(1.0, nan).is_nan());
        assert!(c_min(1.0, nan).is_nan());
        assert_eq!(c_max(nan, 1.0), 1.0);
        assert_eq!(c_min(nan, 1.0), 1.0);
        assert_eq!(c_max(0.0, -0.0).to_bits(), (-0.0_f64).to_bits());
        assert_eq!(c_max(-0.0, 0.0).to_bits(), 0.0_f64.to_bits());
        assert_eq!(c_min(0.0, -0.0).to_bits(), (-0.0_f64).to_bits());
        assert_eq!(c_min(-0.0, 0.0).to_bits(), 0.0_f64.to_bits());
        assert_eq!(c_max(-2.0, 3.0), 3.0);
        assert_eq!(c_min(-2.0, 3.0), -2.0);
    }
}
