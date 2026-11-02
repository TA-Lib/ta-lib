use ta_lib::{Core, OutRange, RetCode};

fn interval_pattern(open: &[f64], close: &[f64], i: usize) -> i32 {
    let white = close[i] >= open[i];
    if white == (close[i - 1] >= open[i - 1]) { return 0; }
    let (outer_low, outer_high, inner_low, inner_high, sign) = if white {
        (open[i], close[i], close[i - 1], open[i - 1], 1)
    } else {
        (close[i], open[i], open[i - 1], close[i - 1], -1)
    };
    if !(outer_low <= inner_low && outer_high >= inner_high
        && (outer_low < inner_low || outer_high > inner_high)) { return 0; }
    sign * if outer_low == inner_low || outer_high == inner_high { 80 } else { 100 }
}

#[test]
fn all_ieee_two_bar_combinations_keep_signed_pattern_values_and_tails() {
    let values = [f64::NEG_INFINITY, -f64::MAX, -2.0, -1.0, -0.0, 0.0,
        f64::from_bits(1), 1.0, 2.0, f64::MAX, f64::INFINITY,
        f64::from_bits(0x7ff8_0000_0000_0042), f64::from_bits(0xfff0_0000_0000_0001)];
    let core = Core::new();
    let mut hits = [0usize; 5];
    for &previous_open in &values { for &previous_close in &values {
        for &current_open in &values { for &current_close in &values {
            let open = [0.0, previous_open, current_open];
            let close = [0.0, previous_close, current_close];
            let mut out = [12345; 4];
            assert_eq!(core.cdlengulfing(0, 2, &open, &[3.0; 3], &[-3.0; 3], &close, &mut out), Ok(OutRange { beg_idx: 2, count: 1 }));
            let expected = interval_pattern(&open, &close, 2);
            assert_eq!(out, [expected, 12345, 12345, 12345]);
            hits[[-100, -80, 0, 80, 100].iter().position(|v| *v == expected).unwrap()] += 1;
        }}
    }}
    assert!(hits.iter().all(|n| *n > 0), "every signed magnitude must be exercised: {hits:?}");
}

#[test]
fn partial_ranges_and_offset_slices_keep_the_output_origin() {
    let open: Vec<f64> = (0..137).map(|i| [3.0, 1.0, -0.0, 2.0, f64::INFINITY][i % 5]).collect();
    let close: Vec<f64> = (0..137).map(|i| [1.0, 4.0, 0.0, f64::NAN, -1.0, 2.0][i % 6]).collect();
    let open = &open[3..]; let close = &close[3..];
    let core = Core::new();
    for start in [0, 1, 2, 3, 17, 99] {
        for end in [start, start + 1, 130] {
            let beg = start.max(2);
            let expected: Vec<i32> = (beg..=end).map(|i| interval_pattern(open, close, i)).collect();
            let mut output = vec![54321; expected.len() + 7];
            let range = core.cdlengulfing(start, end, open, &[8.0; 134], &[-8.0; 134], close, &mut output[3..]).unwrap();
            assert_eq!(range, if expected.is_empty() { OutRange { beg_idx: 0, count: 0 } } else { OutRange { beg_idx: beg, count: expected.len() } });
            assert_eq!(&output[..3], &[54321; 3]);
            assert_eq!(&output[3..3 + expected.len()], expected);
            assert!(output[3 + expected.len()..].iter().all(|v| *v == 54321));
        }
    }
}

#[test]
fn unused_ohlc_and_empty_output_validation_still_precede_computation() {
    let core = Core::new(); let data = [1.0; 5];
    let mut out = [1122; 4];
    for missing in 0..4 {
        let mut inputs = [&data[..]; 4]; inputs[missing] = &data[..4];
        assert_eq!(core.cdlengulfing(0, 4, inputs[0], inputs[1], inputs[2], inputs[3], &mut out), Err(RetCode::BadParam));
        assert_eq!(out, [1122; 4]);
    }
    assert_eq!(core.cdlengulfing(0, 4, &data, &data, &data, &data, &mut out[..2]), Err(RetCode::BadParam));
    assert_eq!(core.cdlengulfing(0, 1, &data, &data, &data, &data, &mut []), Err(RetCode::BadParam));
    assert_eq!(core.cdlengulfing(3, 2, &[], &[], &[], &[], &mut out), Err(RetCode::OutOfRangeEndIndex));
    assert_eq!(core.cdlengulfing(usize::MAX, usize::MAX, &[], &[], &[], &[], &mut out), Err(RetCode::OutOfRangeStartIndex));
    assert_eq!(core.cdlengulfing(0, usize::MAX, &data, &data, &data, &data, &mut out), Err(RetCode::OutOfRangeEndIndex));
    assert_eq!(out, [1122; 4]);
}
