use ta_lib::{Core, FuncUnstId, MAType, OutRange, RetCode};

const POISON: f64 = 123_456.75;

#[test]
fn normalized_equal_bounds_match_ma_without_touching_output_padding() {
    let types: Vec<_> = (0..).map_while(|i| MAType::try_from(i).ok()).collect();
    assert!(types.len() >= 14);
    let mut compared = 0;
    for unstable in [0, 7] {
        let core = Core::builder().unstable_period(FuncUnstId::ALL, unstable).build().unwrap();
        for &ty in &types {
            for (min, max, period) in [(1, 1, 1), (2, 2, 2), (15, 15, 15), (10000, 10000, 10000),
                                       (Core::INTEGER_DEFAULT, 2, 2), (30, Core::INTEGER_DEFAULT, 30)] {
                let lb = core.ma_lookback(period, ty).unwrap();
                let n = lb + 67;
                let data: Vec<_> = (0..n).map(|i| 100.0 + (i % 97) as f64 * 0.01).collect();
                let choices = [f64::NAN, f64::INFINITY, f64::NEG_INFINITY, f64::MAX, -f64::MAX, -0.0, 0.0, 2.9, 10001.5];
                let periods: Vec<_> = (0..n).map(|i| choices[i % choices.len()]).collect();
                for start in [0, lb + 5, n - 1] {
                    let mut expected = vec![POISON; n];
                    let want = core.ma(start, n - 1, &data, period, ty, &mut expected).unwrap();
                    let mut actual = vec![POISON; n + 2];
                    let got = core.mavp(start, n - 1, &data, &periods, min, max, ty, &mut actual[1..n+1]).unwrap();
                    assert_eq!(got, want, "{ty:?} bounds={min}/{max}, start={start}, unstable={unstable}");
                    assert!(got.count > 0);
                    for (&a, &b) in actual[1..got.count+1].iter().zip(&expected[..got.count]) {
                        assert_eq!(a.to_bits(), b.to_bits());
                        compared += 1;
                    }
                    assert!(actual[..1].iter().chain(&actual[got.count+1..]).all(|x| x.to_bits() == POISON.to_bits()));
                }
                if lb > 0 {
                    let mut out = [POISON];
                    assert_eq!(core.mavp(0, lb-1, &data, &periods, min, max, ty, &mut out), Ok(OutRange { beg_idx: 0, count: 0 }));
                    assert_eq!(out, [POISON]);
                }
            }
        }
    }
    assert!(compared > 20_000, "only {compared} values compared");
}

#[test]
fn unequal_normalized_defaults_remain_variable_period() {
    let core = Core::new();
    let data: Vec<_> = (0..192).map(|i| 10.0 + (i % 37) as f64 / 100.0).collect();
    let periods: Vec<_> = (0..192).map(|i| (2 + i % 29) as f64).collect();
    for raw in [(Core::INTEGER_DEFAULT, Core::INTEGER_DEFAULT), (2, 30)] {
        let mut got = vec![POISON; 192];
        let r = core.mavp(0, 191, &data, &periods, raw.0, raw.1, MAType::SMA, &mut got).unwrap();
        assert_eq!(r, OutRange { beg_idx: 29, count: 163 });
        for (i, &value) in got[..r.count].iter().enumerate() {
            let row = r.beg_idx + i;
            let mut reference = vec![POISON; 192];
            let expected = core.ma(r.beg_idx, row, &data, periods[row] as i32, MAType::SMA, &mut reference).unwrap();
            assert_eq!(expected.count, row - r.beg_idx + 1);
            assert_eq!(value.to_bits(), reference[expected.count-1].to_bits());
        }
    }
}

#[test]
fn fixed_bounds_preserve_period_input_and_output_validation() {
    let core = Core::new();
    let data = vec![100.0; 96];
    let periods = vec![f64::NAN; 96];
    for (start, end, period) in [(0, 95, 2), (37, 95, 30), (0, 5, 30)] {
        let mut out = vec![POISON; 96];
        assert_eq!(core.mavp(start, end, &data, &periods[..end], period, period, MAType::SMA, &mut out), Err(RetCode::BadParam));
        assert!(out.iter().all(|x| *x == POISON));
        assert_eq!(core.mavp(start, end, &data, &periods, period, period, MAType::SMA, &mut []), Err(RetCode::BadParam));
    }
    let mut short = [POISON; 3];
    assert_eq!(core.mavp(0, 95, &data, &periods, 2, 2, MAType::SMA, &mut short), Err(RetCode::BadParam));
    assert_eq!(short, [POISON; 3]);
    let mut out = [POISON; 96];
    assert_eq!(core.mavp(0, 95, &data, &periods, 0, 0, MAType::SMA, &mut out), Err(RetCode::BadParam));
    assert_eq!(core.mavp(0, 95, &data, &periods, 10001, 10001, MAType::SMA, &mut out), Err(RetCode::BadParam));
    assert_eq!(out, [POISON; 96]);
}
