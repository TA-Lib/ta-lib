//! C's TA_SetEMAVariation on one Core: each Core answers by its own setting,
//! and a stream it opens equals its batch. An unknown value is not
//! representable on the builder; `EMAVariation::try_from` refuses it.

use ta_lib::{Core, EMAVariation, RetCode};

fn series(n: usize) -> Vec<f64> {
    (0..n)
        .map(|i| 100.0 + 10.0 * (i as f64 / 7.0).sin() + 3.0 * (i as f64 / 3.0).cos())
        .collect()
}

#[test]
fn each_core_answers_by_its_own_ema_variation_and_its_stream_equals_its_batch() {
    assert_eq!(Core::new().ema_variation(), EMAVariation::SMA_SEED);
    assert_eq!(EMAVariation::try_from(2), Err(RetCode::BadParam));
    assert_eq!(EMAVariation::try_from(-1), Err(RetCode::BadParam));

    let builder = Core::builder().ema_variation(EMAVariation::FIRST_BAR_SEED);
    let first = builder.clone().build().unwrap();
    assert_eq!(first.ema_variation(), EMAVariation::FIRST_BAR_SEED);

    let input = series(120);
    let mut dflt = vec![0.0; input.len()];
    let mut fb = vec![0.0; input.len()];
    let rd = Core::new().ema(0, input.len() - 1, &input, 10, &mut dflt).unwrap();
    let rf = first.ema(0, input.len() - 1, &input, 10, &mut fb).unwrap();
    assert_eq!(rd, rf, "the EMA variation moves no range");
    assert_ne!(dflt[0], fb[0], "two Cores answer by their own EMA variation");
    let (k, mut e) = (2.0 / 11.0, input[0]);
    for x in &input[1..=rf.beg_idx] {
        e = k * x + (1.0 - k) * e;
    }
    assert!((fb[0] - e).abs() <= 1e-12 * e.abs(), "seeded on bar 0: {} vs {}", fb[0], e);

    let (mut stream, _) = first.ema_open(&input[..60], 10).unwrap();
    let reset = builder.ema_variation(EMAVariation::SMA_SEED).build().unwrap();
    assert_eq!(reset.ema_variation(), EMAVariation::SMA_SEED);
    for (t, x) in input.iter().enumerate().skip(60) {
        let v = stream.update(*x).unwrap();
        assert_eq!(v.to_bits(), fb[t - rf.beg_idx].to_bits(), "bar {t}");
    }
}
