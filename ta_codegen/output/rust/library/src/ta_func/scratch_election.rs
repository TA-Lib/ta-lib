//! Value gate for the batch bodies' scratch-buffer election (issue #146).
//!
//! Hand-written, not generated: this file lives in
//! `ta_codegen/generator/templates/rust/scratch_election.rs` and is copied
//! verbatim into the crate by `generate` (the Rust backend's `clean_keep` holds
//! it, so `generate` never deletes it). It is declared `#[cfg(test)]` in
//! `mod.rs`, so nothing here ships in a release build.
//!
//! `BBANDS` elects two of its own output slices as the scratch buffers the
//! calculation runs in, so it allocates nothing. The Rust backend
//! implements that election as a *rename* — the standard deviation is written
//! into `outRealUpperBand` and read back out of it by the band loop, and the
//! moving average is written straight into `outRealMiddleBand`. The whole point
//! of the transform is that it changes **no value**, and that is what these
//! tests pin: they pass identically before and after it, because they check
//! numeric invariance, not the absence of an allocation.
//!
//! Three independent properties, each of which a mis-election would break:
//!
//! 1. **Composition.** `BBANDS` must be bit-identical to `MA` + `STDDEV`
//!    recomposed through the public API — a genuinely separate code path. This
//!    covers the SMA fast path (`optInMAType == 0`, where the election happens)
//!    and the general MA path (every other type, which must stay untouched).
//! 2. **No dependence on prior output contents.** Electing an output as scratch
//!    means the calculation reads a buffer the caller owned. Running the same
//!    call over slices pre-filled with different garbage must give the same bits.
//! 3. **No dependence on output slice capacity.** The pre-#146 shape sized its
//!    scratch from the caller's slice length rather than the data range, so a
//!    caller who allocates once at capacity and calls per window paid for the
//!    capacity every call. Results must not vary with the capacity either.

use super::*;

/// Every optional-parameter combination the gate sweeps.
const PERIODS: [i32; 6] = [2, 3, 5, 20, 33, 100];
/// `(optInNbDevUp, optInNbDevDn)`. Equal and unequal pairs take different band
/// loops in the source (the unequal one fuses with `mul_add`), and the zero and
/// negative multipliers are in range but unusual.
const DEVS: [(f64, f64); 4] = [(2.0, 2.0), (0.0, 3.0), (-1.0, 2.0), (1.5, 2.5)];
/// `SMA` (the fast path that elects) plus `EMA`/`WMA`/`DEMA`/`T3` (the general
/// path, which keeps its own allocations and must be unaffected).
const MATYPES: [i32; 5] = [0, 1, 2, 3, 8];

/// Bar counts from 1 to 5000: every small length (so the lookback clamps, the
/// empty-output returns and the single-bar cases are all hit) plus a spread of
/// larger ones. Above ~32*period the fast path's variance recurrence reseeds, so
/// the large lengths are not redundant.
fn bar_counts() -> Vec<usize> {
    let mut ns: Vec<usize> = (1..=40).collect();
    ns.extend_from_slice(&[100, 250, 500, 999, 1000, 2000, 5000]);
    ns
}

/// Deterministic input, no dependencies: a drifting series with enough spread to
/// keep the standard deviation away from the fast path's degenerate branches.
fn series(n: usize, seed: u64) -> Vec<f64> {
    let mut s = seed
        .wrapping_mul(6_364_136_223_846_793_005)
        .wrapping_add(1_442_695_040_888_963_407);
    (0..n)
        .map(|i| {
            s = s
                .wrapping_mul(6_364_136_223_846_793_005)
                .wrapping_add(1_442_695_040_888_963_407);
            let u = ((s >> 11) as f64) / ((1u64 << 53) as f64);
            100.0 + (i as f64) * 0.01 + u * 5.0
        })
        .collect()
}

struct Bands {
    rc: RetCode,
    beg: usize,
    nb: usize,
    upper: Vec<f64>,
    middle: Vec<f64>,
    lower: Vec<f64>,
}

/// Run `BBANDS` over `0..=n-1` with output slices of `cap` elements pre-filled
/// with `fill`, and return the first `outNBElement` of each band.
fn bbands(
    core: &Core,
    input: &[f64],
    period: i32,
    dev_up: f64,
    dev_dn: f64,
    matype: i32,
    cap: usize,
    fill: f64,
) -> Bands {
    let mut upper = vec![fill; cap];
    let mut middle = vec![fill; cap];
    let mut lower = vec![fill; cap];
    let mut beg = 0usize;
    let mut nb = 0usize;
    let rc = core.BBANDS(
        0,
        input.len() - 1,
        input,
        period,
        dev_up,
        dev_dn,
        matype,
        &mut beg,
        &mut nb,
        &mut upper,
        &mut middle,
        &mut lower,
    );
    upper.truncate(nb);
    middle.truncate(nb);
    lower.truncate(nb);
    Bands { rc, beg, nb, upper, middle, lower }
}

/// `BBANDS` recomposed from `MA` and `STDDEV` through the public API, following
/// the source's own realignment and band arithmetic. `None` when either leg
/// declines the parameters.
fn bbands_from_ma_and_stddev(
    core: &Core,
    input: &[f64],
    period: i32,
    dev_up: f64,
    dev_dn: f64,
    matype: i32,
) -> Option<Bands> {
    let n = input.len();
    let mut ma = vec![f64::NAN; n];
    let mut ma_beg = 0usize;
    let mut ma_nb = 0usize;
    if core.MA(0, n - 1, input, period, matype, &mut ma_beg, &mut ma_nb, &mut ma) != RetCode::Success
    {
        return None;
    }
    if ma_nb == 0 {
        return None;
    }
    let mut sd = vec![f64::NAN; n];
    let mut sd_beg = 0usize;
    let mut sd_nb = 0usize;
    if core.STDDEV(ma_beg, n - 1, input, period, 1.0, &mut sd_beg, &mut sd_nb, &mut sd)
        != RetCode::Success
    {
        return None;
    }
    // The standard deviation can clamp to a later bar than the moving average
    // did; the source shifts the MA forward so each band pairs the two at the
    // same bar.
    let shift = sd_beg.saturating_sub(ma_beg);
    let nb = sd_nb;
    let mut upper = Vec::with_capacity(nb);
    let mut middle = Vec::with_capacity(nb);
    let mut lower = Vec::with_capacity(nb);
    for i in 0..nb {
        let m = ma[i + shift];
        let s = sd[i];
        // Mirrors the two band loops: the equal-multiplier one shares a single
        // product, the unequal one fuses the upper band's multiply-add.
        let (up, lo) = if dev_up == dev_dn {
            let t = s * dev_up;
            (m + t, m - t)
        } else {
            (s.mul_add(dev_up, m), m - s * dev_dn)
        };
        upper.push(up);
        middle.push(m);
        lower.push(lo);
    }
    Some(Bands { rc: RetCode::Success, beg: sd_beg, nb, upper, middle, lower })
}

/// Bit-for-bit, not approximate: this is an identity gate, so `NaN` must match
/// `NaN` and `-0.0` must not match `0.0`.
fn assert_same_bits(what: &str, got: &[f64], want: &[f64], ctx: &str) {
    assert_eq!(got.len(), want.len(), "{what} length differs ({ctx})");
    for (i, (g, w)) in got.iter().zip(want.iter()).enumerate() {
        assert_eq!(
            g.to_bits(),
            w.to_bits(),
            "{what}[{i}] differs ({ctx}): {g:?} ({:#018x}) vs {w:?} ({:#018x})",
            g.to_bits(),
            w.to_bits()
        );
    }
}

#[test]
fn bbands_is_bit_identical_to_ma_plus_stddev() {
    let core = Core::new();
    let mut compared = 0usize;
    for &matype in &MATYPES {
        for &period in &PERIODS {
            for &(dev_up, dev_dn) in &DEVS {
                for n in bar_counts() {
                    let input = series(n, (period as u64) * 1_000_003 + n as u64);
                    let got = bbands(&core, &input, period, dev_up, dev_dn, matype, n, f64::NAN);
                    let Some(want) =
                        bbands_from_ma_and_stddev(&core, &input, period, dev_up, dev_dn, matype)
                    else {
                        continue;
                    };
                    if got.rc != RetCode::Success || got.nb == 0 {
                        continue;
                    }
                    let ctx =
                        format!("matype={matype} period={period} dev=({dev_up},{dev_dn}) n={n}");
                    assert_eq!(got.beg, want.beg, "outBegIdx differs ({ctx})");
                    assert_eq!(got.nb, want.nb, "outNBElement differs ({ctx})");
                    assert_same_bits("middle", &got.middle, &want.middle, &ctx);
                    assert_same_bits("upper", &got.upper, &want.upper, &ctx);
                    assert_same_bits("lower", &got.lower, &want.lower, &ctx);
                    compared += 1;
                }
            }
        }
    }
    // A silently-empty sweep would make this test vacuous.
    eprintln!("BBANDS vs MA+STDDEV: {compared} bit-exact comparisons");
    assert!(compared > 500, "only {compared} comparisons ran");
}

#[test]
fn bbands_ignores_the_prior_contents_of_its_output_slices() {
    let core = Core::new();
    // The election writes the standard deviation into outRealUpperBand and reads
    // it back, so whatever the caller left in the output buffers must not leak
    // into the result.
    let fills = [0.0, f64::NAN, -1.0, 1e300, f64::INFINITY];
    let mut compared = 0usize;
    for &matype in &MATYPES {
        for &period in &PERIODS {
            for &(dev_up, dev_dn) in &DEVS {
                for n in [1usize, 2, 3, 5, 34, 101, 500, 5000] {
                    let input = series(n, (period as u64) * 7 + n as u64);
                    let base = bbands(&core, &input, period, dev_up, dev_dn, matype, n, fills[0]);
                    if base.rc != RetCode::Success || base.nb == 0 {
                        continue;
                    }
                    for &fill in &fills[1..] {
                        let other =
                            bbands(&core, &input, period, dev_up, dev_dn, matype, n, fill);
                        let ctx = format!(
                            "matype={matype} period={period} dev=({dev_up},{dev_dn}) n={n} fill={fill:?}"
                        );
                        assert_eq!(other.rc, base.rc, "retCode differs ({ctx})");
                        assert_eq!(other.beg, base.beg, "outBegIdx differs ({ctx})");
                        assert_eq!(other.nb, base.nb, "outNBElement differs ({ctx})");
                        assert_same_bits("middle", &other.middle, &base.middle, &ctx);
                        assert_same_bits("upper", &other.upper, &base.upper, &ctx);
                        assert_same_bits("lower", &other.lower, &base.lower, &ctx);
                        compared += 1;
                    }
                }
            }
        }
    }
    eprintln!("BBANDS vs prior output contents: {compared} bit-exact comparisons");
    assert!(compared > 500, "only {compared} comparisons ran");
}

#[test]
fn bbands_is_independent_of_output_slice_capacity() {
    let core = Core::new();
    // The caller pattern the pre-#146 shape penalised: allocate the outputs once
    // at capacity, then call per window. The values must not move with the
    // capacity, in either direction.
    let mut compared = 0usize;
    for &matype in &MATYPES {
        for &period in &PERIODS {
            for &(dev_up, dev_dn) in &DEVS {
                for n in [3usize, 34, 500] {
                    let input = series(n, (period as u64) * 13 + n as u64);
                    let base = bbands(&core, &input, period, dev_up, dev_dn, matype, n, f64::NAN);
                    if base.rc != RetCode::Success || base.nb == 0 {
                        continue;
                    }
                    for mult in [10usize, 100] {
                        let wide = bbands(
                            &core,
                            &input,
                            period,
                            dev_up,
                            dev_dn,
                            matype,
                            n * mult,
                            f64::NAN,
                        );
                        let ctx = format!(
                            "matype={matype} period={period} dev=({dev_up},{dev_dn}) n={n} cap={}",
                            n * mult
                        );
                        assert_eq!(wide.rc, base.rc, "retCode differs ({ctx})");
                        assert_eq!(wide.beg, base.beg, "outBegIdx differs ({ctx})");
                        assert_eq!(wide.nb, base.nb, "outNBElement differs ({ctx})");
                        assert_same_bits("middle", &wide.middle, &base.middle, &ctx);
                        assert_same_bits("upper", &wide.upper, &base.upper, &ctx);
                        assert_same_bits("lower", &wide.lower, &base.lower, &ctx);
                        compared += 1;
                    }
                }
            }
        }
    }
    eprintln!("BBANDS vs output slice capacity: {compared} bit-exact comparisons");
    assert!(compared > 200, "only {compared} comparisons ran");
}
