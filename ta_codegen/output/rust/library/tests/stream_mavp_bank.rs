//! The MAVP stream against the batch, bit for bit, for every MAType (#445).
//!
//! Every slot of the bank reads its price history from one tape the bank owns,
//! so what can go wrong is a wrong lag in one slot, a tape one slot too short, a
//! peek that sees a bar the commit would not, a fork sharing the tape, or the
//! tape drifting from the bars after an `advance`. Each leg exists to see one:
//! - bands whose deepest lag is exactly a power of two (`[2,2]`, `[2,32]`,
//!   `[2,33]`, `[2,65]`), the only place an undersized tape shows;
//! - a ramp that selects every slot every `nBank + 2` bars, and, for the two
//!   re-anchoring MAs, a staircase that holds each period for `8p + 1` bars so a
//!   re-anchor bar is selected in every slot;
//! - `peek` before every `update`;
//! - a fork driven to the end BEFORE its original, so a shared tape shows;
//! - two handles fed alike, one of them advanced once.

use ta_lib::{Core, FuncUnstId, MAType};

const BANDS: [(i32, i32); 6] = [(1, 1), (2, 2), (1, 13), (2, 32), (2, 33), (2, 65)];
const RAMP_BARS: usize = 600;

fn same(a: f64, b: f64) -> bool {
    a.to_bits() == b.to_bits()
}

/// Two-decimal prices are not dyadic, so a wrong lag cannot cancel out; the flat
/// runs and the rare large prints reach KAMA's and WMA's edge paths.
fn prices(n: usize) -> Vec<f64> {
    let mut seed: u32 = 445;
    let mut x = 100.0_f64;
    (0..n)
        .map(|i| {
            seed = seed.wrapping_mul(1_103_515_245).wrapping_add(12_345);
            if i % 1500 < 1460 {
                if i % 997 == 0 {
                    x *= if (i / 997) % 2 == 1 { 3.7 } else { 0.29 };
                } else {
                    x += (f64::from((seed >> 8) % 201) - 100.0) / 100.0;
                }
            }
            if x < 1.0 {
                x = 1.0 + f64::from((seed >> 8) % 97) / 100.0;
            }
            x = ((x * 100.0 + 0.5) as i64) as f64 / 100.0;
            x
        })
        .collect()
}

fn ramp(n: usize, (min, max): (i32, i32)) -> Vec<f64> {
    let cycle = (max - min + 1) as usize + 2;
    (0..n)
        .map(|i| f64::from(min - 1) + (i % cycle) as f64 + if i % 3 == 1 { 0.37 } else { 0.0 })
        .collect()
}

/// Period `p` held for `8p + 1` bars from the first output on, `min` up to `max`.
fn staircase(lb: usize, (min, max): (i32, i32)) -> Vec<f64> {
    let len: usize = (min..=max).map(|p| 8 * p as usize + 1).sum();
    let n = lb + 16 + len;
    let (mut p, mut held) = (min, 0);
    (0..n)
        .map(|i| {
            let v = f64::from(p);
            if i > lb {
                held += 1;
                if held == 8 * p + 1 && p < max {
                    p += 1;
                    held = 0;
                }
            }
            v
        })
        .collect()
}

struct Case<'a> {
    core: &'a Core,
    ty: MAType,
    band: (i32, i32),
    series: &'static str,
    price: &'a [f64],
    per: &'a [f64],
    batch: &'a [f64],
    lb: usize,
}

impl Case<'_> {
    fn at(&self, what: &str, bar: usize) -> String {
        format!(
            "MAVP stream {what}: {:?} band {:?} {} bar {bar}",
            self.ty, self.band, self.series
        )
    }

    /// Open at `prefix`, then peek and update every remaining bar. With
    /// `fork_at`, a clone taken there runs to the end first. With `twin`, a
    /// second handle takes the same bars and one `advance` at the midpoint.
    fn drive(&self, prefix: usize, fork_at: Option<usize>, twin: bool) -> u64 {
        let (n, lb, (min, max)) = (self.per.len(), self.lb, self.band);
        let open = || {
            self.core
                .mavp_open(&self.price[..prefix], &self.per[..prefix], min, max, self.ty)
                .unwrap_or_else(|e| panic!("{}: {e:?}", self.at("open", prefix - 1)))
        };
        let (mut h, v) = open();
        assert!(same(v, self.batch[prefix - 1 - lb]), "{}", self.at("open", prefix - 1));
        let mut h2 = twin.then(|| open().0);
        let mut fork: Vec<f64> = Vec::new();
        let mut compared = 0;
        for t in prefix..n {
            if Some(t) == fork_at {
                let mut f = h.clone();
                fork = (t..n).map(|u| f.update(self.price[u], self.per[u]).unwrap()).collect();
            }
            if t == (prefix + n) / 2 {
                if let Some(h2) = h2.as_mut() {
                    h2.advance().unwrap();
                }
            }
            let pk = h.peek(self.price[t], self.per[t]).unwrap();
            let v = h.update(self.price[t], self.per[t]).unwrap();
            assert!(same(v, self.batch[t - lb]), "{}: {v} vs batch {}", self.at("update", t), self.batch[t - lb]);
            assert!(same(pk, v), "{}: peek {pk} vs update {v}", self.at("peek", t));
            if let Some(at) = fork_at.filter(|at| t >= *at) {
                assert!(same(fork[t - at], v), "{}: fork {} vs original {v}", self.at("clone", t), fork[t - at]);
            }
            if let Some(h2) = h2.as_mut() {
                let v2 = h2.update(self.price[t], self.per[t]).unwrap();
                assert!(same(v2, v), "{}: twin {v2} vs {v}", self.at("advance twin", t));
            }
            compared += 1;
        }
        if let Some(h2) = &h2 {
            let (a, b) = (h.out_range(), h2.out_range());
            assert!(b.beg_idx == a.beg_idx && b.count == a.count + 1, "advance twin range {b:?} vs {a:?}");
        }
        compared
    }
}

fn stream_bank(core: &Core) -> u64 {
    let types: Vec<MAType> = (0..).map_while(|i| MAType::try_from(i).ok()).collect();
    // A floor, not an equality: a newly appended MAType is picked up with no edit.
    assert!(types.len() >= 14, "only {} MATypes", types.len());
    let mut compared = 0;
    for &ty in &types {
        for band in BANDS {
            let lb = core.mavp_lookback(band.0, band.1, ty).unwrap();
            let mut shapes = vec![("ramp", ramp(lb + RAMP_BARS, band))];
            if matches!(ty, MAType::WMA | MAType::HMA) {
                shapes.push(("staircase", staircase(lb, band)));
            }
            for (series, per) in shapes {
                let n = per.len();
                let price = prices(n);
                let mut batch = vec![0.0; n];
                let r = core.mavp(0, n - 1, &price, &per, band.0, band.1, ty, &mut batch).unwrap();
                assert_eq!((r.beg_idx, r.count), (lb, n - lb));
                let case = Case { core, ty, band, series, price: &price, per: &per, batch: &batch, lb };

                let mut out = vec![0.0; n];
                let (_, r) = core.mavp_open_and_fill(&price, &per, band.0, band.1, ty, &mut out).unwrap();
                assert_eq!((r.beg_idx, r.count), (lb, n - lb));
                for (i, (o, b)) in out.iter().zip(&batch).take(r.count).enumerate() {
                    assert!(same(*o, *b), "{}", case.at("open_and_fill", lb + i));
                }
                compared += r.count as u64;

                compared += case.drive(lb + 1, None, false);
                compared += case.drive(lb + 2, None, true);
                compared += case.drive(lb + (n - lb) / 3, Some(lb + (n - lb) / 2), false);
            }
        }
    }
    compared
}

#[test]
fn the_bank_streams_the_batch() {
    let compared = stream_bank(&Core::new());
    assert!(compared > 200_000, "compared only {compared} values");
}

#[test]
fn the_bank_streams_the_batch_with_an_unstable_period() {
    let core = Core::new().to_builder().unstable_period(FuncUnstId::ALL, 7).build().unwrap();
    let compared = stream_bank(&core);
    assert!(compared > 200_000, "compared only {compared} values");
}
