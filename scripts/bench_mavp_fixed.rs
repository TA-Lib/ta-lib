use std::{env, fs::File, hint::black_box, io::{BufWriter, Write}, time::Instant};
use ta_lib::{Core, MAType, OutRange};

fn call(core: &Core, mode: &str, ty: MAType, data: &[f64], periods: &[f64], out: &mut [f64]) -> OutRange {
    let end = data.len() - 1;
    match mode {
        "fixed" => core.mavp(0, end, data, periods, black_box(14), black_box(14), ty, out),
        "variable_control" => core.mavp(0, end, data, periods, black_box(2), black_box(30), ty, out),
        _ => core.ma(0, end, data, black_box(14), ty, out),
    }.unwrap()
}

fn main() {
    let args: Vec<_> = env::args().collect();
    let repeats: usize = args.get(1).expect("repeats").parse().unwrap();
    assert!(repeats >= 5);
    let capture_only = args.get(3).is_some_and(|v| v == "--capture-only");
    let mut capture = args.get(2).map(|path| BufWriter::new(File::create(path).unwrap()));
    let core = Core::new();
    for n in [100usize, 10_000, 1_000_000] {
        let data: Vec<_> = (0..n).map(|i| 100.0 + ((i * 17) % 107) as f64 / 100.0).collect();
        let choices = [f64::NAN, f64::INFINITY, f64::NEG_INFINITY, -0.0, 3.9, 17.3, 10001.0];
        let periods: Vec<_> = (0..n).map(|i| choices[i % choices.len()]).collect();
        let iterations = if n == 100 { 2000 } else if n == 10_000 { 100 } else { 3 };
        for (mode, ty) in [("fixed", MAType::SMA), ("fixed", MAType::EMA), ("fixed", MAType::WMA), ("fixed", MAType::DISABLED),
                           ("variable_control", MAType::SMA), ("variable_control", MAType::EMA),
                           ("ma_control", MAType::SMA), ("ma_control", MAType::EMA)] {
            let mut out = vec![123_456.75; n];
            let range = call(&core, mode, ty, &data, &periods, &mut out);
            let name = format!("{mode}/{ty:?}/{n}");
            if let Some(file) = &mut capture {
                writeln!(file, "{name} {} {}", range.beg_idx, range.count).unwrap();
                for value in &out { file.write_all(&value.to_bits().to_le_bytes()).unwrap(); }
            }
            if capture_only { continue; }
            for _ in 0..5 { black_box(call(&core, mode, ty, &data, &periods, &mut out)); }
            let mut samples = Vec::new();
            for _ in 0..repeats {
                let start = Instant::now();
                for _ in 0..iterations {
                    black_box(call(black_box(&core), black_box(mode), black_box(ty), black_box(&data), black_box(&periods), black_box(&mut out)));
                }
                samples.push(start.elapsed().as_nanos() as f64 / iterations as f64);
            }
            println!("{{\"case\":\"{name}\",\"iterations\":{iterations},\"samples_ns\":{samples:?}}}");
        }
    }
}
