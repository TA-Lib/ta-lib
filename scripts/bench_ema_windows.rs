use std::{env, fs::File, hint::black_box, io::{BufWriter, Write}, time::Instant};
use ta_lib::{Core, FuncUnstId, OutRange};

const CANARY: u64 = 0x7ff8_dead_beef_1234;

fn capture(file: &mut Option<BufWriter<File>>, name: &str, range: OutRange, outputs: &[&[f64]]) {
    for output in outputs {
        assert!(output[range.count..].iter().all(|value| value.to_bits() == CANARY));
    }
    if let Some(file) = file {
        writeln!(file, "{name} {} {} {}", range.beg_idx, range.count, outputs.len()).unwrap();
        for output in outputs {
            for value in *output { file.write_all(&value.to_bits().to_le_bytes()).unwrap(); }
        }
    }
}

fn parity(file: &mut Option<BufWriter<File>>) {
    let mut cases = 0;
    let mut written = 0;
    for shape in 0..8 {
        let mut input: Vec<f64> = (0..512).map(|i| match shape {
            0 => 100.0 + (i as f64 * 0.17).sin() * 4.0,
            1 => -0.0,
            2 => if i % 2 == 0 { -0.0 } else { 0.0 },
            3 => 3.3,
            4 => f64::from_bits(1 + i as u64 % 7),
            _ => (i as f64 * 0.1).sin(),
        }).collect();
        if shape == 5 { input[93] = f64::NAN; }
        if shape == 6 { input[93] = f64::INFINITY; }
        if shape == 7 { input[93] = f64::NEG_INFINITY; }
        for unstable in [0, 7, Core::UNSTABLE_AUTO_PREC_4, Core::UNSTABLE_AUTO_PREC_8] {
            let core = Core::builder().unstable_period(FuncUnstId::EMA, unstable).build().unwrap();
            for start in [0, 1, 14, 29, 93, 255, 511] {
                for end in [start, 511] {
                    for period in [Core::INTEGER_DEFAULT, 1, 2, 3, 14, 30, 50, 200, 512, 100000] {
                        let mut out = vec![f64::from_bits(CANARY); input.len()];
                        let range = core.ema(start, end, &input, period, &mut out).unwrap();
                        capture(file, &format!("parity/EMA/{shape}/{unstable}/{period}/{start}/{end}"), range, &[&out]);
                        cases += 1;
                        written += range.count;
                    }
                    for (fast, slow, signal) in [(12, 26, 9), (26, 12, 9), (2, 3, 1), (2, 3, 2), (50, 200, 14), (Core::INTEGER_DEFAULT, Core::INTEGER_DEFAULT, Core::INTEGER_DEFAULT)] {
                        let mut a = vec![f64::from_bits(CANARY); input.len()];
                        let mut b = a.clone();
                        let mut c = a.clone();
                        let range = core.macd(start, end, &input, fast, slow, signal, &mut a, &mut b, &mut c).unwrap();
                        capture(file, &format!("parity/MACD/{shape}/{unstable}/{fast}/{slow}/{signal}/{start}/{end}"), range, &[&a, &b, &c]);
                        cases += 1;
                        written += 3 * range.count;
                    }
                    for signal in [1, 9, Core::INTEGER_DEFAULT] {
                        let mut a = vec![f64::from_bits(CANARY); input.len()];
                        let mut b = a.clone();
                        let mut c = a.clone();
                        let range = core.macdfix(start, end, &input, signal, &mut a, &mut b, &mut c).unwrap();
                        capture(file, &format!("parity/MACDFIX/{shape}/{unstable}/{signal}/{start}/{end}"), range, &[&a, &b, &c]);
                        cases += 1;
                        written += 3 * range.count;
                    }
                }
            }
        }
    }
    assert!(written > 500000);
    println!("{{\"kind\":\"parity\",\"cases\":{cases},\"written_values\":{written}}}");
}

fn call(core: &Core, mode: &str, input: &[f64], high: &[f64], low: &[f64], a: &mut [f64], b: &mut [f64], c: &mut [f64]) -> OutRange {
    let end = input.len() - 1;
    match mode {
        "EMA" => core.ema(0, end, input, black_box(14), a),
        "MACD" => core.macd(0, end, input, black_box(12), black_box(26), black_box(9), a, b, c),
        "MACDFIX" => core.macdfix(0, end, input, black_box(9), a, b, c),
        "ATR_control" => core.atr(0, end, high, low, input, black_box(14), a),
        _ => core.sma(0, end, input, black_box(14), a),
    }.unwrap()
}

fn main() {
    let args: Vec<_> = env::args().collect();
    let repeats: usize = args.get(1).expect("repeats").parse().unwrap();
    assert!(repeats >= 5);
    let capture_only = args.get(3).is_some_and(|value| value == "--capture-only");
    let mut file = args.get(2).map(|path| BufWriter::new(File::create(path).unwrap()));
    println!("{{\"kind\":\"metadata\",\"arch\":\"{}\",\"os\":\"{}\",\"repeats\":{repeats},\"capture_only\":{capture_only}}}", env::consts::ARCH, env::consts::OS);
    parity(&mut file);
    let core = Core::new();
    for n in [20usize, 100, 512, 10_000, 1_000_000] {
        let input: Vec<_> = (0..n).map(|i| 100.0 + ((i * 17) % 107) as f64 / 100.0).collect();
        let high: Vec<_> = input.iter().map(|value| value + 0.7).collect();
        let low: Vec<_> = input.iter().map(|value| value - 0.4).collect();
        let iterations = (2_000_000 / n).max(3);
        for mode in ["EMA", "MACD", "MACDFIX", "ATR_control", "SMA_control"] {
            if mode.starts_with("MACD") && n == 20 { continue; }
            let mut a = vec![f64::from_bits(CANARY); n];
            let mut b = a.clone();
            let mut c = a.clone();
            let range = call(&core, mode, &input, &high, &low, &mut a, &mut b, &mut c);
            assert!(range.count > 0);
            let outputs = if mode.starts_with("MACD") { vec![a.as_slice(), b.as_slice(), c.as_slice()] } else { vec![a.as_slice()] };
            capture(&mut file, &format!("benchmark/{mode}/{n}"), range, &outputs);
            if capture_only { continue; }
            for _ in 0..5 { black_box(call(&core, mode, &input, &high, &low, &mut a, &mut b, &mut c)); }
            let mut samples = Vec::new();
            for _ in 0..repeats {
                let start = Instant::now();
                for _ in 0..iterations {
                    black_box(call(black_box(&core), black_box(mode), black_box(&input), black_box(&high), black_box(&low), black_box(&mut a), black_box(&mut b), black_box(&mut c)));
                }
                samples.push(start.elapsed().as_nanos() as f64 / iterations as f64);
            }
            println!("{{\"case\":\"{mode}/{n}\",\"iterations\":{iterations},\"samples_ns\":{samples:?}}}");
        }
    }
}
