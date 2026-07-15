// fuzz_data.rs — plain-Rust bit-exact port of src/tools/ta_regtest/fuzz_data.h.
// Deterministic input generator + output hasher. Every f64/integer result is
// bit-for-bit identical to the C on little-endian x86-64 (FP_CONTRACT OFF: no FMA
// fusion; Rust never auto-fuses a*b+c and f64::mul_add is deliberately unused).

// ---- Data shapes (keep FUZZ_NSHAPES last). ----
const FUZZ_RANDWALK: i32 = 0; // geometric random walk around 100 (typical prices)
const FUZZ_MONO_UP: i32 = 1; // strictly increasing
const FUZZ_MONO_DOWN: i32 = 2; // strictly decreasing
const FUZZ_CONSTANT: i32 = 3; // flat O=H=L=C (degenerate: hh==ll, zero variance)
const FUZZ_TIE_HEAVY: i32 = 4; // small integer set — many equal values / ties
const FUZZ_EXTREME: i32 = 5; // alternating huge (1e9) / tiny (1e-7) magnitudes
const FUZZ_WITH_ZEROS: i32 = 6; // sprinkled 0.0 / -0.0 and small signed values
const FUZZ_CANDLE: i32 = 7; // inside-bar cascades + breakouts + confirmations
const FUZZ_ZEROSUM: i32 = 8; // symmetric high=-low bars: high+low == 0 EXACTLY
const FUZZ_NSHAPES: i32 = 9;

// ---- splitmix64 PRNG (deterministic, self-contained) ----
fn fuzz_sm_next(s: &mut u64) -> u64 {
    *s = s.wrapping_add(0x9E3779B97F4A7C15);
    let mut z = *s;
    z = (z ^ (z >> 30)).wrapping_mul(0xBF58476D1CE4E5B9);
    z = (z ^ (z >> 27)).wrapping_mul(0x94D049BB133111EB);
    z ^ (z >> 31)
}

// Uniform double in [0,1) from the top 53 bits.
fn fuzz_sm_unit(s: &mut u64) -> f64 {
    (fuzz_sm_next(s) >> 11) as f64 * (1.0 / 9007199254740992.0)
}

// ---- FUZZ_CANDLE: deterministic, pattern-rich inside-bar OHLC ----
// One candle bar (clamped to a valid candle); writes at index p only when p < n.
fn fuzz_cdl_bar(
    o: &mut [f64], h: &mut [f64], l: &mut [f64], c: &mut [f64], v: &mut [f64], oi: &mut [f64],
    mut p: i32, n: i32, cap_o: f64, cap_h: f64, cap_l: f64, cap_cl: f64,
) -> i32 {
    let mut hi = cap_h;
    let mut lo = cap_l;
    if hi < cap_o { hi = cap_o; }
    if hi < cap_cl { hi = cap_cl; } // clamp to a valid candle
    if lo > cap_o { lo = cap_o; }
    if lo > cap_cl { lo = cap_cl; }
    if p < n {
        o[p as usize] = cap_o;
        h[p as usize] = hi;
        l[p as usize] = lo;
        c[p as usize] = cap_cl;
        v[p as usize] = 1000.0;
        oi[p as usize] = 100.0;
        p += 1;
    }
    p
}

fn fuzz_cdl_flat(
    o: &mut [f64], h: &mut [f64], l: &mut [f64], c: &mut [f64], v: &mut [f64], oi: &mut [f64],
    mut p: i32, n: i32, mut k: i32, base: f64,
) -> i32 {
    while k > 0 {
        k -= 1;
        p = fuzz_cdl_bar(o, h, l, c, v, oi, p, n, base, base + 1.0, base - 1.0, base);
    }
    p
}

// Hikkake window: 3 bars + optional confirm. dir +1 bull/-1 bear;
// brk 0=intact,1=break P1,2=break P2,3/4=break the breakout; conf
// 0=none,1=confirm next bar,2=confirm one bar too late (expired).
fn fuzz_cdl_hikkake(
    o: &mut [f64], h: &mut [f64], l: &mut [f64], c: &mut [f64], v: &mut [f64], oi: &mut [f64],
    mut p: i32, n: i32, base: f64, w: f64, dir: i32, brk: i32, conf: i32,
) -> i32 {
    let h1 = base + w;
    let l1 = base - w;
    let h2 = if brk == 1 { base + w + 0.5 } else { base + 0.6 * w };
    let l2 = if brk == 2 { base - w - 0.5 } else { base - 0.6 * w };
    let h3;
    let l3;
    if dir > 0 {
        h3 = if brk == 3 { h2 + 0.4 * w } else { h2 - 0.4 * w };
        l3 = l2 - 0.4 * w;
    } else {
        h3 = if brk == 4 { h2 - 0.4 * w } else { h2 + 0.4 * w };
        l3 = l2 + 0.4 * w;
    }
    p = fuzz_cdl_bar(o, h, l, c, v, oi, p, n, base, h1, l1, base);
    p = fuzz_cdl_bar(o, h, l, c, v, oi, p, n, base, h2, l2, base);
    p = fuzz_cdl_bar(o, h, l, c, v, oi, p, n, base, h3, l3, base);
    if conf == 2 {
        p = fuzz_cdl_flat(o, h, l, c, v, oi, p, n, 3, base);
    }
    if conf != 0 {
        let cc = if dir > 0 { h2 + w } else { l2 - w };
        p = fuzz_cdl_bar(o, h, l, c, v, oi, p, n, cc, cc + 0.5, cc - 0.5, cc);
    }
    p
}

// Modified-hikkake window: 4 nested/breakout bars + optional confirm.
fn fuzz_cdl_hikkakemod(
    o: &mut [f64], h: &mut [f64], l: &mut [f64], c: &mut [f64], v: &mut [f64], oi: &mut [f64],
    mut p: i32, n: i32, base: f64, w: f64, dir: i32, brk: i32, conf: i32,
) -> i32 {
    let h1 = base + w;
    let l1 = base - w;
    let h2 = if brk == 1 { base + w + 0.5 } else { base + 0.7 * w };
    let l2 = if brk == 1 { base - w - 0.5 } else { base - 0.7 * w };
    let h3 = if brk == 2 { h2 + 0.3 * w } else { base + 0.45 * w };
    let l3 = if brk == 2 { l2 - 0.3 * w } else { base - 0.45 * w };
    let h4;
    let l4;
    if dir > 0 {
        h4 = if brk == 3 { h3 + 0.3 * w } else { h3 - 0.3 * w };
        l4 = l3 - 0.3 * w;
    } else {
        h4 = if brk == 3 { h3 - 0.3 * w } else { h3 + 0.3 * w };
        l4 = l3 + 0.3 * w;
    }
    let c2 = if brk == 4 { base } else if dir > 0 { l2 } else { h2 }; // 2nd close near low/high
    p = fuzz_cdl_bar(o, h, l, c, v, oi, p, n, base, h1, l1, base);
    p = fuzz_cdl_bar(o, h, l, c, v, oi, p, n, base, h2, l2, c2);
    p = fuzz_cdl_bar(o, h, l, c, v, oi, p, n, base, h3, l3, base);
    p = fuzz_cdl_bar(o, h, l, c, v, oi, p, n, base, h4, l4, base);
    if conf != 0 {
        let cc = if dir > 0 { h3 + w } else { l3 - w };
        p = fuzz_cdl_bar(o, h, l, c, v, oi, p, n, cc, cc + 0.5, cc - 0.5, cc);
    }
    p
}

// ---- FUZZ_CANDLE deterministic pattern catalog (issue #109) ----
// Neutral primer: k alternating small-body bars around base.
fn fuzz_cdl_primer(
    o: &mut [f64], h: &mut [f64], l: &mut [f64], c: &mut [f64], v: &mut [f64], oi: &mut [f64],
    mut p: i32, n: i32, k: i32, base: f64, bd: f64, hr: f64,
) -> i32 {
    let mut i = 0;
    while i < k {
        let cap_o = if (i & 1) != 0 { base } else { base + bd };
        let cap_c = if (i & 1) != 0 { base + bd } else { base };
        p = fuzz_cdl_bar(o, h, l, c, v, oi, p, n, cap_o, base + bd + hr, base - hr, cap_c);
        i += 1;
    }
    p
}

// CDL2CROWS (bearish, -100 on the 3rd candle).
fn fuzz_cdl_2crows(
    o: &mut [f64], h: &mut [f64], l: &mut [f64], c: &mut [f64], v: &mut [f64], oi: &mut [f64],
    mut p: i32, n: i32, base: f64,
) -> i32 {
    p = fuzz_cdl_primer(o, h, l, c, v, oi, p, n, 6, base, 2.0, 1.0);
    p = fuzz_cdl_bar(o, h, l, c, v, oi, p, n, base, base + 12.5, base - 0.5, base + 12.0); // 1st white long
    p = fuzz_cdl_bar(o, h, l, c, v, oi, p, n, base + 18.0, base + 18.5, base + 13.5, base + 14.0); // 2nd black gap up
    p = fuzz_cdl_bar(o, h, l, c, v, oi, p, n, base + 16.0, base + 16.5, base + 5.5, base + 6.0); // 3rd black inside
    p
}

// CDL3BLACKCROWS (bearish, -100 on the 3rd crow).
fn fuzz_cdl_3blackcrows(
    o: &mut [f64], h: &mut [f64], l: &mut [f64], c: &mut [f64], v: &mut [f64], oi: &mut [f64],
    mut p: i32, n: i32, base: f64,
) -> i32 {
    p = fuzz_cdl_primer(o, h, l, c, v, oi, p, n, 6, base, 2.0, 1.0);
    p = fuzz_cdl_bar(o, h, l, c, v, oi, p, n, base, base + 11.0, base - 1.0, base + 10.0); // i-3: white long body
    p = fuzz_cdl_bar(o, h, l, c, v, oi, p, n, base + 8.0, base + 8.0, base, base); // i-2: 1st black
    p = fuzz_cdl_bar(o, h, l, c, v, oi, p, n, base + 5.0, base + 5.0, base - 5.0, base - 5.0); // i-1: 2nd black inside
    p = fuzz_cdl_bar(o, h, l, c, v, oi, p, n, base + 2.0, base + 2.0, base - 8.0, base - 8.0); // i:   3rd black inside
    p
}

// CDL3WHITESOLDIERS (bullish, +100 on the 3rd soldier).
fn fuzz_cdl_3whitesoldiers(
    o: &mut [f64], h: &mut [f64], l: &mut [f64], c: &mut [f64], v: &mut [f64], oi: &mut [f64],
    mut p: i32, n: i32, base: f64,
) -> i32 {
    p = fuzz_cdl_primer(o, h, l, c, v, oi, p, n, 6, base, 2.0, 1.0);
    p = fuzz_cdl_bar(o, h, l, c, v, oi, p, n, base, base + 10.2, base - 0.2, base + 10.0); // 1st white long
    p = fuzz_cdl_bar(o, h, l, c, v, oi, p, n, base + 5.0, base + 15.2, base + 4.8, base + 15.0); // 2nd white
    p = fuzz_cdl_bar(o, h, l, c, v, oi, p, n, base + 10.0, base + 20.2, base + 9.8, base + 20.0); // 3rd white
    p
}

// CDL3STARSINSOUTH (bullish, +100 on the 3rd candle).
fn fuzz_cdl_3starsinsouth(
    o: &mut [f64], h: &mut [f64], l: &mut [f64], c: &mut [f64], v: &mut [f64], oi: &mut [f64],
    mut p: i32, n: i32, base: f64,
) -> i32 {
    p = fuzz_cdl_primer(o, h, l, c, v, oi, p, n, 6, base, 2.0, 1.0);
    p = fuzz_cdl_bar(o, h, l, c, v, oi, p, n, base + 8.0, base + 8.0, base - 12.0, base); // 1st black long + long lower shadow
    p = fuzz_cdl_bar(o, h, l, c, v, oi, p, n, base + 6.0, base + 6.0, base - 4.0, base + 2.0); // 2nd black smaller body
    p = fuzz_cdl_bar(o, h, l, c, v, oi, p, n, base + 1.0, base + 1.0, base, base); // 3rd black tiny marubozu
    p
}

// CDL3LINESTRIKE (three-white branch, +100 on the strike).
fn fuzz_cdl_3linestrike(
    o: &mut [f64], h: &mut [f64], l: &mut [f64], c: &mut [f64], v: &mut [f64], oi: &mut [f64],
    mut p: i32, n: i32, base: f64,
) -> i32 {
    p = fuzz_cdl_primer(o, h, l, c, v, oi, p, n, 6, base, 2.0, 1.0);
    p = fuzz_cdl_bar(o, h, l, c, v, oi, p, n, base, base + 5.0, base - 1.0, base + 4.0); // i-3: 1st white soldier
    p = fuzz_cdl_bar(o, h, l, c, v, oi, p, n, base + 2.0, base + 7.0, base + 1.0, base + 6.0); // i-2: 2nd white
    p = fuzz_cdl_bar(o, h, l, c, v, oi, p, n, base + 4.0, base + 9.0, base + 3.0, base + 8.0); // i-1: 3rd white
    p = fuzz_cdl_bar(o, h, l, c, v, oi, p, n, base + 10.0, base + 11.0, base - 3.0, base - 2.0); // i:   4th black strike
    p
}

// CDLCONCEALBABYSWALL (bullish, +100 on the 4th candle).
fn fuzz_cdl_concealbabyswall(
    o: &mut [f64], h: &mut [f64], l: &mut [f64], c: &mut [f64], v: &mut [f64], oi: &mut [f64],
    mut p: i32, n: i32, base: f64,
) -> i32 {
    p = fuzz_cdl_primer(o, h, l, c, v, oi, p, n, 6, base, 2.0, 1.0);
    p = fuzz_cdl_bar(o, h, l, c, v, oi, p, n, base, base, base - 3.0, base - 3.0); // 1st black marubozu
    p = fuzz_cdl_bar(o, h, l, c, v, oi, p, n, base - 4.0, base - 4.0, base - 7.0, base - 7.0); // 2nd black marubozu
    p = fuzz_cdl_bar(o, h, l, c, v, oi, p, n, base - 9.0, base - 6.0, base - 12.0, base - 11.0); // 3rd black gapdown+shadow
    p = fuzz_cdl_bar(o, h, l, c, v, oi, p, n, base - 5.0, base - 4.0, base - 16.0, base - 15.0); // 4th black engulfs 3rd
    p
}

// CDLMATHOLD (bullish, +100 on the 5th candle).
fn fuzz_cdl_mathold(
    o: &mut [f64], h: &mut [f64], l: &mut [f64], c: &mut [f64], v: &mut [f64], oi: &mut [f64],
    mut p: i32, n: i32, base: f64,
) -> i32 {
    p = fuzz_cdl_primer(o, h, l, c, v, oi, p, n, 6, base, 2.0, 1.0);
    p = fuzz_cdl_bar(o, h, l, c, v, oi, p, n, base, base + 21.0, base - 1.0, base + 20.0); // c1 white long
    p = fuzz_cdl_bar(o, h, l, c, v, oi, p, n, base + 31.0, base + 32.0, base + 29.0, base + 30.0); // c2 short black gap up
    p = fuzz_cdl_bar(o, h, l, c, v, oi, p, n, base + 16.0, base + 17.0, base + 14.0, base + 15.0); // c3 short, in 1st range
    p = fuzz_cdl_bar(o, h, l, c, v, oi, p, n, base + 14.0, base + 15.0, base + 12.0, base + 13.0); // c4 short, falling
    p = fuzz_cdl_bar(o, h, l, c, v, oi, p, n, base + 35.0, base + 41.0, base + 34.0, base + 40.0); // c5 white breakout
    p
}

// CDLRISEFALL3METHODS (rising branch, +100 on the 5th candle).
fn fuzz_cdl_risefall3methods(
    o: &mut [f64], h: &mut [f64], l: &mut [f64], c: &mut [f64], v: &mut [f64], oi: &mut [f64],
    mut p: i32, n: i32, base: f64,
) -> i32 {
    p = fuzz_cdl_primer(o, h, l, c, v, oi, p, n, 6, base, 2.0, 1.0);
    p = fuzz_cdl_bar(o, h, l, c, v, oi, p, n, base, base + 12.5, base - 0.5, base + 12.0); // 1st long white
    p = fuzz_cdl_bar(o, h, l, c, v, oi, p, n, base + 10.0, base + 10.5, base + 8.5, base + 9.0); // 2nd small black
    p = fuzz_cdl_bar(o, h, l, c, v, oi, p, n, base + 8.0, base + 8.5, base + 6.5, base + 7.0); // 3rd small black falling
    p = fuzz_cdl_bar(o, h, l, c, v, oi, p, n, base + 6.0, base + 6.5, base + 4.5, base + 5.0); // 4th small black falling
    p = fuzz_cdl_bar(o, h, l, c, v, oi, p, n, base + 6.0, base + 20.5, base + 5.5, base + 20.0); // 5th long white breakout
    p
}

// CDLADVANCEBLOCK (bearish, -100 on the 3rd candle).
fn fuzz_cdl_advanceblock(
    o: &mut [f64], h: &mut [f64], l: &mut [f64], c: &mut [f64], v: &mut [f64], oi: &mut [f64],
    mut p: i32, n: i32, base: f64,
) -> i32 {
    p = fuzz_cdl_primer(o, h, l, c, v, oi, p, n, 6, base, 2.0, 1.0);
    p = fuzz_cdl_bar(o, h, l, c, v, oi, p, n, base, base + 6.0, base, base + 6.0); // 1st white long body
    p = fuzz_cdl_bar(o, h, l, c, v, oi, p, n, base + 5.0, base + 7.0, base + 5.0, base + 7.0); // 2nd white, shorter
    p = fuzz_cdl_bar(o, h, l, c, v, oi, p, n, base + 7.0, base + 8.0, base + 7.0, base + 8.0); // 3rd white, shortest
    p
}

// CDLINNECK (bearish, -100 on the 2nd candle). Wider primer (hr=6).
fn fuzz_cdl_inneck(
    o: &mut [f64], h: &mut [f64], l: &mut [f64], c: &mut [f64], v: &mut [f64], oi: &mut [f64],
    mut p: i32, n: i32, base: f64,
) -> i32 {
    p = fuzz_cdl_primer(o, h, l, c, v, oi, p, n, 6, base, 2.0, 6.0);
    p = fuzz_cdl_bar(o, h, l, c, v, oi, p, n, base + 10.0, base + 12.0, base - 1.0, base); // 1st long black
    p = fuzz_cdl_bar(o, h, l, c, v, oi, p, n, base - 5.0, base + 1.0, base - 6.0, base + 0.35); // 2nd white into neck
    p
}

// CDLUNIQUE3RIVER (bullish, +100 on the 3rd candle).
fn fuzz_cdl_unique3river(
    o: &mut [f64], h: &mut [f64], l: &mut [f64], c: &mut [f64], v: &mut [f64], oi: &mut [f64],
    mut p: i32, n: i32, base: f64,
) -> i32 {
    p = fuzz_cdl_primer(o, h, l, c, v, oi, p, n, 6, base, 2.0, 1.0);
    p = fuzz_cdl_bar(o, h, l, c, v, oi, p, n, base + 10.0, base + 11.0, base - 12.0, base - 10.0); // 1st black long body
    p = fuzz_cdl_bar(o, h, l, c, v, oi, p, n, base + 5.0, base + 6.0, base - 15.0, base - 5.0); // 2nd black harami+low
    p = fuzz_cdl_bar(o, h, l, c, v, oi, p, n, base - 4.0, base - 2.0, base - 5.0, base - 3.0); // 3rd white short body
    p
}

// CDLKICKING: 2nd pattern bar (index 13) = +100 (bullish).
fn fuzz_cdl_kicking(
    o: &mut [f64], h: &mut [f64], l: &mut [f64], c: &mut [f64], v: &mut [f64], oi: &mut [f64],
    mut p: i32, n: i32, base: f64,
) -> i32 {
    p = fuzz_cdl_primer(o, h, l, c, v, oi, p, n, 6, base, 2.0, 1.0);
    p = fuzz_cdl_bar(o, h, l, c, v, oi, p, n, base + 20.0, base + 20.0, base, base); // idx12: BLACK long marubozu
    p = fuzz_cdl_bar(o, h, l, c, v, oi, p, n, base + 30.0, base + 50.0, base + 30.0, base + 50.0); // idx13: WHITE long marubozu, gap up
    p
}

// CDLKICKINGBYLENGTH: 2nd pattern bar (index 13) = +100 (bullish).
fn fuzz_cdl_kickingbylength(
    o: &mut [f64], h: &mut [f64], l: &mut [f64], c: &mut [f64], v: &mut [f64], oi: &mut [f64],
    mut p: i32, n: i32, base: f64,
) -> i32 {
    p = fuzz_cdl_primer(o, h, l, c, v, oi, p, n, 6, base, 2.0, 1.0);
    p = fuzz_cdl_bar(o, h, l, c, v, oi, p, n, base + 40.0, base + 40.0, base + 20.0, base + 20.0); // bar12: BLACK long marubozu
    p = fuzz_cdl_bar(o, h, l, c, v, oi, p, n, base + 45.0, base + 70.0, base + 45.0, base + 70.0); // bar13: WHITE long marubozu, gap up
    p
}

// CDLDARKCLOUDCOVER: 2nd pattern bar (index 13) = -100.
fn fuzz_cdl_darkcloudcover(
    o: &mut [f64], h: &mut [f64], l: &mut [f64], c: &mut [f64], v: &mut [f64], oi: &mut [f64],
    mut p: i32, n: i32, base: f64,
) -> i32 {
    p = fuzz_cdl_primer(o, h, l, c, v, oi, p, n, 6, base, 2.0, 1.0);
    p = fuzz_cdl_bar(o, h, l, c, v, oi, p, n, base, base + 21.0, base - 1.0, base + 20.0); // idx12: white LONG body
    p = fuzz_cdl_bar(o, h, l, c, v, oi, p, n, base + 30.0, base + 31.0, base + 4.0, base + 5.0); // idx13: black, closes below midpoint
    p
}

// CDLPIERCING: 2nd pattern candle (buffer index 13) fires +100.
fn fuzz_cdl_piercing(
    o: &mut [f64], h: &mut [f64], l: &mut [f64], c: &mut [f64], v: &mut [f64], oi: &mut [f64],
    mut p: i32, n: i32, base: f64,
) -> i32 {
    p = fuzz_cdl_primer(o, h, l, c, v, oi, p, n, 6, base, 2.0, 1.0);
    p = fuzz_cdl_bar(o, h, l, c, v, oi, p, n, base + 20.0, base + 21.0, base - 1.0, base); // 1st: black LONG body
    p = fuzz_cdl_bar(o, h, l, c, v, oi, p, n, base - 10.0, base + 16.0, base - 11.0, base + 15.0); // 2nd: white LONG body, pierces
    p
}

// CDLTHRUSTING: 2nd pattern bar (index 13) = -100 (always bearish).
fn fuzz_cdl_thrusting(
    o: &mut [f64], h: &mut [f64], l: &mut [f64], c: &mut [f64], v: &mut [f64], oi: &mut [f64],
    mut p: i32, n: i32, base: f64,
) -> i32 {
    p = fuzz_cdl_primer(o, h, l, c, v, oi, p, n, 6, base, 2.0, 1.0);
    p = fuzz_cdl_bar(o, h, l, c, v, oi, p, n, base + 10.0, base + 11.0, base - 1.0, base); // 1st: BLACK LONG body
    p = fuzz_cdl_bar(o, h, l, c, v, oi, p, n, base - 10.0, base + 4.0, base - 12.0, base + 2.5); // 2nd: WHITE thrusting
    p
}

// CDLHOMINGPIGEON: 2nd pattern bar (index 13) = +100 (always bullish).
fn fuzz_cdl_homingpigeon(
    o: &mut [f64], h: &mut [f64], l: &mut [f64], c: &mut [f64], v: &mut [f64], oi: &mut [f64],
    mut p: i32, n: i32, base: f64,
) -> i32 {
    p = fuzz_cdl_primer(o, h, l, c, v, oi, p, n, 6, base, 2.0, 1.0);
    p = fuzz_cdl_bar(o, h, l, c, v, oi, p, n, base + 20.0, base + 21.0, base - 1.0, base); // idx12: black LONG body
    p = fuzz_cdl_bar(o, h, l, c, v, oi, p, n, base + 7.0, base + 8.0, base + 4.0, base + 5.0); // idx13: black SHORT body, engulfed
    p
}

// CDL3INSIDE: 3rd pattern bar (index 14) = -100 (bearish three-inside-down).
fn fuzz_cdl_3inside(
    o: &mut [f64], h: &mut [f64], l: &mut [f64], c: &mut [f64], v: &mut [f64], oi: &mut [f64],
    mut p: i32, n: i32, base: f64,
) -> i32 {
    p = fuzz_cdl_primer(o, h, l, c, v, oi, p, n, 6, base, 2.0, 1.0);
    p = fuzz_cdl_bar(o, h, l, c, v, oi, p, n, base, base + 12.5, base - 0.5, base + 12.0); // 1st: long WHITE body
    p = fuzz_cdl_bar(o, h, l, c, v, oi, p, n, base + 6.0, base + 7.5, base + 5.5, base + 7.0); // 2nd: SHORT body, engulfed
    p = fuzz_cdl_bar(o, h, l, c, v, oi, p, n, base + 8.0, base + 8.5, base - 2.5, base - 2.0); // 3rd: BLACK, closes below 1st open
    p
}

// CDLIDENTICAL3CROWS: 3rd pattern bar (index 14) = -100.
fn fuzz_cdl_identical3crows(
    o: &mut [f64], h: &mut [f64], l: &mut [f64], c: &mut [f64], v: &mut [f64], oi: &mut [f64],
    mut p: i32, n: i32, base: f64,
) -> i32 {
    p = fuzz_cdl_primer(o, h, l, c, v, oi, p, n, 6, base, 2.0, 1.0);
    p = fuzz_cdl_bar(o, h, l, c, v, oi, p, n, base, base, base - 3.0, base - 3.0); // 1st crow: black, lower shadow 0
    p = fuzz_cdl_bar(o, h, l, c, v, oi, p, n, base - 3.0, base - 3.0, base - 6.0, base - 6.0); // 2nd crow: opens at 1st close
    p = fuzz_cdl_bar(o, h, l, c, v, oi, p, n, base - 6.0, base - 6.0, base - 9.0, base - 9.0); // 3rd crow: opens at 2nd close
    p
}

// CDLSTALLEDPATTERN: 3rd pattern bar (index 14) = -100.
fn fuzz_cdl_stalledpattern(
    o: &mut [f64], h: &mut [f64], l: &mut [f64], c: &mut [f64], v: &mut [f64], oi: &mut [f64],
    mut p: i32, n: i32, base: f64,
) -> i32 {
    p = fuzz_cdl_primer(o, h, l, c, v, oi, p, n, 6, base, 2.0, 1.0);
    p = fuzz_cdl_bar(o, h, l, c, v, oi, p, n, base, base + 12.0, base, base + 12.0); // 1st: white LONG body
    p = fuzz_cdl_bar(o, h, l, c, v, oi, p, n, base + 8.0, base + 25.0, base + 8.0, base + 25.0); // 2nd: white LONG body, no upper shadow
    p = fuzz_cdl_bar(o, h, l, c, v, oi, p, n, base + 25.0, base + 26.0, base + 25.0, base + 26.0); // 3rd: small white on 2nd's shoulder
    p
}

// CDLUPSIDEGAP2CROWS: 3rd pattern bar (index 14) = -100.
fn fuzz_cdl_upsidegap2crows(
    o: &mut [f64], h: &mut [f64], l: &mut [f64], c: &mut [f64], v: &mut [f64], oi: &mut [f64],
    mut p: i32, n: i32, base: f64,
) -> i32 {
    p = fuzz_cdl_primer(o, h, l, c, v, oi, p, n, 6, base, 2.0, 1.0);
    p = fuzz_cdl_bar(o, h, l, c, v, oi, p, n, base, base + 10.5, base - 0.5, base + 10.0); // bar12: white LONG body
    p = fuzz_cdl_bar(o, h, l, c, v, oi, p, n, base + 16.0, base + 16.5, base + 14.5, base + 15.0); // bar13: black SHORT body, gap up
    p = fuzz_cdl_bar(o, h, l, c, v, oi, p, n, base + 18.0, base + 18.5, base + 11.5, base + 12.0); // bar14: black
    p
}

// CDLBREAKAWAY: 5th pattern bar (index 16) = +100 (1st-candle-black branch).
fn fuzz_cdl_breakaway(
    o: &mut [f64], h: &mut [f64], l: &mut [f64], c: &mut [f64], v: &mut [f64], oi: &mut [f64],
    mut p: i32, n: i32, base: f64,
) -> i32 {
    p = fuzz_cdl_primer(o, h, l, c, v, oi, p, n, 6, base, 2.0, 1.0);
    p = fuzz_cdl_bar(o, h, l, c, v, oi, p, n, base + 20.0, base + 21.0, base + 7.0, base + 8.0); // idx12: black LONG body
    p = fuzz_cdl_bar(o, h, l, c, v, oi, p, n, base + 4.0, base + 5.0, base - 1.0, base); // idx13: black, body gaps DOWN
    p = fuzz_cdl_bar(o, h, l, c, v, oi, p, n, base - 2.0, base, base - 5.0, base - 4.0); // idx14: lower high & low than 2nd
    p = fuzz_cdl_bar(o, h, l, c, v, oi, p, n, base - 7.0, base - 6.0, base - 11.0, base - 9.0); // idx15: black, lower high & low than 3rd
    p = fuzz_cdl_bar(o, h, l, c, v, oi, p, n, base + 5.0, base + 7.0, base + 4.0, base + 6.0); // idx16: white, close in gap
    p
}

// CDLLADDERBOTTOM: 5th pattern bar (index 16) = +100 (always bullish).
fn fuzz_cdl_ladderbottom(
    o: &mut [f64], h: &mut [f64], l: &mut [f64], c: &mut [f64], v: &mut [f64], oi: &mut [f64],
    mut p: i32, n: i32, base: f64,
) -> i32 {
    p = fuzz_cdl_primer(o, h, l, c, v, oi, p, n, 6, base, 2.0, 1.0);
    p = fuzz_cdl_bar(o, h, l, c, v, oi, p, n, base + 20.0, base + 21.0, base + 14.0, base + 15.0); // i-4: black, highest open & close
    p = fuzz_cdl_bar(o, h, l, c, v, oi, p, n, base + 15.0, base + 16.0, base + 9.0, base + 10.0); // i-3: black, lower
    p = fuzz_cdl_bar(o, h, l, c, v, oi, p, n, base + 10.0, base + 11.0, base + 4.0, base + 5.0); // i-2: black, lower
    p = fuzz_cdl_bar(o, h, l, c, v, oi, p, n, base + 5.0, base + 12.0, base - 2.0, base); // i-1: black, big upper shadow
    p = fuzz_cdl_bar(o, h, l, c, v, oi, p, n, base + 6.0, base + 15.0, base + 6.0, base + 15.0); // i:   white breakout
    p
}

// CDLXSIDEGAP3METHODS: 3rd pattern bar (index 14) = +100 (bullish).
fn fuzz_cdl_xsidegap3methods(
    o: &mut [f64], h: &mut [f64], l: &mut [f64], c: &mut [f64], v: &mut [f64], oi: &mut [f64],
    mut p: i32, n: i32, base: f64,
) -> i32 {
    p = fuzz_cdl_primer(o, h, l, c, v, oi, p, n, 6, base, 2.0, 1.0);
    p = fuzz_cdl_bar(o, h, l, c, v, oi, p, n, base, base + 6.0, base - 1.0, base + 5.0); // 1st: white body [100,105]
    p = fuzz_cdl_bar(o, h, l, c, v, oi, p, n, base + 8.0, base + 14.0, base + 7.0, base + 13.0); // 2nd: white body, gap up
    p = fuzz_cdl_bar(o, h, l, c, v, oi, p, n, base + 10.0, base + 11.0, base + 1.0, base + 2.0); // 3rd: black, fills the gap
    p
}

// Lay the deterministic per-family catalog (issue #109); one entry per pattern.
fn fuzz_cdl_catalog(
    o: &mut [f64], h: &mut [f64], l: &mut [f64], c: &mut [f64], v: &mut [f64], oi: &mut [f64],
    mut p: i32, n: i32,
) -> i32 {
    p = fuzz_cdl_2crows(o, h, l, c, v, oi, p, n, 100.0);
    p = fuzz_cdl_3blackcrows(o, h, l, c, v, oi, p, n, 100.0);
    p = fuzz_cdl_3whitesoldiers(o, h, l, c, v, oi, p, n, 100.0);
    p = fuzz_cdl_3starsinsouth(o, h, l, c, v, oi, p, n, 100.0);
    p = fuzz_cdl_3linestrike(o, h, l, c, v, oi, p, n, 100.0);
    p = fuzz_cdl_concealbabyswall(o, h, l, c, v, oi, p, n, 100.0);
    p = fuzz_cdl_mathold(o, h, l, c, v, oi, p, n, 100.0);
    p = fuzz_cdl_risefall3methods(o, h, l, c, v, oi, p, n, 100.0);
    p = fuzz_cdl_advanceblock(o, h, l, c, v, oi, p, n, 100.0);
    p = fuzz_cdl_inneck(o, h, l, c, v, oi, p, n, 100.0);
    p = fuzz_cdl_unique3river(o, h, l, c, v, oi, p, n, 100.0);
    p = fuzz_cdl_kicking(o, h, l, c, v, oi, p, n, 100.0);
    p = fuzz_cdl_kickingbylength(o, h, l, c, v, oi, p, n, 100.0);
    p = fuzz_cdl_darkcloudcover(o, h, l, c, v, oi, p, n, 100.0);
    p = fuzz_cdl_piercing(o, h, l, c, v, oi, p, n, 100.0);
    p = fuzz_cdl_thrusting(o, h, l, c, v, oi, p, n, 100.0);
    p = fuzz_cdl_homingpigeon(o, h, l, c, v, oi, p, n, 100.0);
    p = fuzz_cdl_3inside(o, h, l, c, v, oi, p, n, 100.0);
    p = fuzz_cdl_identical3crows(o, h, l, c, v, oi, p, n, 100.0);
    p = fuzz_cdl_stalledpattern(o, h, l, c, v, oi, p, n, 100.0);
    p = fuzz_cdl_upsidegap2crows(o, h, l, c, v, oi, p, n, 100.0);
    p = fuzz_cdl_breakaway(o, h, l, c, v, oi, p, n, 100.0);
    p = fuzz_cdl_ladderbottom(o, h, l, c, v, oi, p, n, 100.0);
    p = fuzz_cdl_xsidegap3methods(o, h, l, c, v, oi, p, n, 100.0);
    p
}

fn fuzz_candle_gen(
    seed: i32, n: i32,
    o: &mut [f64], h: &mut [f64], l: &mut [f64], c: &mut [f64], v: &mut [f64], oi: &mut [f64],
) {
    let mut s: u64 =
        0x243F6A8885A308D3 ^ (seed as u32 as u64).wrapping_mul(0xD1B54A32D192ED03);
    let mut p = fuzz_cdl_flat(o, h, l, c, v, oi, 0, n, 6, 100.0);
    p = fuzz_cdl_catalog(o, h, l, c, v, oi, p, n); // deterministic per-family windows (#109)
    while p < n - 16 {
        let base = 100.0 + (fuzz_sm_unit(&mut s) - 0.5) * 40.0;
        let w = 8.0 + fuzz_sm_unit(&mut s) * 20.0;
        let variant = (fuzz_sm_unit(&mut s) * 10.0) as i32;
        let dir = if (fuzz_sm_next(&mut s) & 1) != 0 { 1 } else { -1 };
        match variant {
            0 => p = fuzz_cdl_hikkake(o, h, l, c, v, oi, p, n, base, w, dir, 0, 1),
            1 => p = fuzz_cdl_hikkake(o, h, l, c, v, oi, p, n, base, w, dir, 0, 2),
            2 => p = fuzz_cdl_hikkake(o, h, l, c, v, oi, p, n, base, w, dir, 1, 0),
            3 => p = fuzz_cdl_hikkake(o, h, l, c, v, oi, p, n, base, w, dir, 2, 0),
            4 => p = fuzz_cdl_hikkake(o, h, l, c, v, oi, p, n, base, w, dir, if dir > 0 { 3 } else { 4 }, 0),
            5 => p = fuzz_cdl_hikkakemod(o, h, l, c, v, oi, p, n, base, w, dir, 0, 1),
            6 => p = fuzz_cdl_hikkakemod(o, h, l, c, v, oi, p, n, base, w, dir, 0, 0),
            7 => p = fuzz_cdl_hikkakemod(o, h, l, c, v, oi, p, n, base, w, dir, 1, 0),
            8 => p = fuzz_cdl_hikkakemod(o, h, l, c, v, oi, p, n, base, w, dir, 3, 0),
            _ => p = fuzz_cdl_hikkakemod(o, h, l, c, v, oi, p, n, base, w, dir, 4, 1),
        }
        p = fuzz_cdl_flat(o, h, l, c, v, oi, p, n, 6, base);
    }
    while p < n {
        p = fuzz_cdl_flat(o, h, l, c, v, oi, p, n, 1, 100.0);
    }
}

// ---- FUZZ_ZEROSUM: bars whose high+low is exactly zero ----
fn fuzz_zerosum_gen(
    seed: i32, n: i32,
    o: &mut [f64], h: &mut [f64], l: &mut [f64], c: &mut [f64], v: &mut [f64], oi: &mut [f64],
) {
    let mut s: u64 =
        0x243F6A8885A308D3
            ^ (seed as u32 as u64).wrapping_mul(0xD1B54A32D192ED03)
            ^ ((FUZZ_ZEROSUM as u64) << 32);
    let mut i = 0i32;
    while i < n {
        let r = fuzz_sm_unit(&mut s);
        let open: f64;
        let mut hi: f64;
        let mut lo: f64;
        let close: f64;
        let mut t: f64;
        if r < 0.34 {
            // symmetric zero-sum bar: hi + lo == +0.0 exactly (else branch).
            let a = 1.0 + fuzz_sm_unit(&mut s) * 50.0;
            hi = a;
            lo = -a;
            open = 0.0;
            close = 0.0;
        } else if r < 0.50 {
            // all-zero degenerate bar: hi + lo == 0 with hi == lo == 0.
            hi = 0.0;
            lo = 0.0;
            open = 0.0;
            close = 0.0;
        } else {
            // ordinary positive bar around 100 (hi + lo != 0 -> then branch).
            let base = 90.0 + fuzz_sm_unit(&mut s) * 20.0;
            let w = 0.5 + fuzz_sm_unit(&mut s) * 5.0;
            close = base;
            t = fuzz_sm_unit(&mut s) - 0.5;
            open = base + t;
            hi = if open > close { open } else { close };
            hi = hi + w;
            lo = if open < close { open } else { close };
            lo = lo - w;
        }
        o[i as usize] = open;
        h[i as usize] = hi;
        l[i as usize] = lo;
        c[i as usize] = close;
        t = fuzz_sm_unit(&mut s) * 1000.0;
        v[i as usize] = 1000.0 + t;
        t = fuzz_sm_unit(&mut s) * 100.0;
        oi[i as usize] = 100.0 + t;
        i += 1;
    }
}

// Fill OHLCV+OI arrays (length n) from (shape,seed). high>=max(o,c), low<=min(o,c).
// Mul/add split into statements so nothing contracts.
fn fuzz_gen(
    shape: i32, seed: i32, n: i32,
    o: &mut [f64], h: &mut [f64], l: &mut [f64], c: &mut [f64], v: &mut [f64], oi: &mut [f64],
) {
    if shape == FUZZ_CANDLE {
        fuzz_candle_gen(seed, n, o, h, l, c, v, oi);
        return;
    }
    if shape == FUZZ_ZEROSUM {
        fuzz_zerosum_gen(seed, n, o, h, l, c, v, oi);
        return;
    }
    let mut s: u64 =
        0x243F6A8885A308D3
            ^ (seed as u32 as u64).wrapping_mul(0xD1B54A32D192ED03)
            ^ ((shape as u32 as u64) << 32);
    let mut walk = 100.0;
    let mut i = 0i32;
    while i < n {
        if shape == FUZZ_CONSTANT {
            o[i as usize] = 42.0;
            h[i as usize] = 42.0;
            l[i as usize] = 42.0;
            c[i as usize] = 42.0;
            v[i as usize] = 1000000.0;
            oi[i as usize] = 10000.0;
            i += 1;
            continue;
        }
        if shape == FUZZ_TIE_HEAVY {
            let close = (3 + (fuzz_sm_unit(&mut s) * 5.0) as i32) as f64; // {3..7}
            o[i as usize] = close;
            c[i as usize] = close;
            h[i as usize] = close + (fuzz_sm_next(&mut s) & 1) as f64;
            l[i as usize] = close - (fuzz_sm_next(&mut s) & 1) as f64;
            v[i as usize] = (1 + (fuzz_sm_unit(&mut s) * 4.0) as i32) as f64 * 1000.0;
            oi[i as usize] = 1000.0;
            i += 1;
            continue;
        }

        let close: f64;
        let mut t: f64;
        match shape {
            FUZZ_MONO_UP => {
                t = i as f64 * 0.5;
                close = 10.0 + t;
            }
            FUZZ_MONO_DOWN => {
                t = i as f64 * 0.25;
                close = 500.0 - t;
            }
            FUZZ_EXTREME => {
                t = fuzz_sm_unit(&mut s);
                close = if (fuzz_sm_next(&mut s) & 1) != 0 {
                    (1.0 + t) * 1.0e9
                } else {
                    (1.0 + t) * 1.0e-7
                };
            }
            FUZZ_WITH_ZEROS => {
                let r = fuzz_sm_unit(&mut s);
                if r < 0.15 {
                    close = 0.0;
                } else if r < 0.30 {
                    close = -0.0;
                } else {
                    t = r - 0.5;
                    close = t * 8.0;
                }
            }
            // FUZZ_RANDWALK and default
            _ => {
                t = fuzz_sm_unit(&mut s) - 0.5;
                t = t * 0.04;
                walk = walk * (1.0 + t);
                close = walk;
            }
        }

        let mag = close.abs() * 0.01 + 0.001;
        t = fuzz_sm_unit(&mut s) - 0.5;
        t = t * mag;
        let open = close + t;
        let mut hi = if open > close { open } else { close };
        t = fuzz_sm_unit(&mut s) * mag;
        hi = hi + t;
        let mut lo = if open < close { open } else { close };
        t = fuzz_sm_unit(&mut s) * mag;
        lo = lo - t;

        o[i as usize] = open;
        h[i as usize] = hi;
        l[i as usize] = lo;
        c[i as usize] = close;
        t = fuzz_sm_unit(&mut s) * 1.0e6;
        v[i as usize] = 1000.0 + t;
        t = fuzz_sm_unit(&mut s) * 1.0e4;
        oi[i as usize] = 100.0 + t;
        i += 1;
    }
}

// ---- 64-bit output hash (FNV-1a over raw bytes + murmur finalizer). ----
fn fuzz_hash_init() -> u64 {
    1469598103934665603 // FNV-1a 64-bit offset basis
}

// Hash the raw little-endian bytes of each f64 (preserves -0.0 and NaN payloads).
fn fuzz_hash_bytes_f64(mut h: u64, data: &[f64]) -> u64 {
    for x in data {
        for &b in x.to_le_bytes().iter() {
            h ^= b as u64;
            h = h.wrapping_mul(1099511628211); // FNV-1a 64-bit prime
        }
    }
    h
}

// Hash the raw little-endian bytes of each i32.
fn fuzz_hash_bytes_i32(mut h: u64, data: &[i32]) -> u64 {
    for x in data {
        for &b in x.to_le_bytes().iter() {
            h ^= b as u64;
            h = h.wrapping_mul(1099511628211); // FNV-1a 64-bit prime
        }
    }
    h
}

fn fuzz_hash_fin(mut h: u64) -> u64 {
    h ^= h >> 33;
    h = h.wrapping_mul(0xFF51AFD7ED558CCD);
    h ^= h >> 33;
    h = h.wrapping_mul(0xC4CEB9FE1A85EC53);
    h ^= h >> 33;
    h
}
