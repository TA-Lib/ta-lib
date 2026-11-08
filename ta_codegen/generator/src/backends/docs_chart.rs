//! One SVG chart per function for its ta-lib.org page, drawn from the
//! function's metadata and a reference run over a fixed daily series.
//!
//! The metadata decides the drawing: `overlap` puts real outputs on the price
//! pane, otherwise they get an indicator pane; each integer output that is not a
//! pattern gets a step pane of its own; `pattern_*` outputs, and a `candlestick`
//! function's plain integer output (its sign is the candle's colour), become
//! markers on the candles; the inputs' price components pick candles or a close
//! line and add a volume pane. Output flags pick the stroke (`line`, `dash_line`,
//! `dot`, `histogram`, `upper_limit`/`lower_limit` band), and `display_shift`
//! moves the series by the shift the reference run reports.
//!
//! `STYLE` is the chart's whole stylesheet and ships inside the SVG; the site
//! adds layout only.

use crate::ir::{FuncDef, Output, ParamType, PriceComponent};
use std::collections::HashMap;
use std::fmt::Write as _;

const W: f64 = 720.0;
const PAD_L: f64 = 10.0;
const PAD_R: f64 = 58.0;
const TOP: f64 = 6.0;
const GAP: f64 = 12.0;
const H_PRICE: f64 = 230.0;
const H_IND: f64 = 130.0;
const H_INT_SMALL: f64 = 70.0;
const H_VOL: f64 = 56.0;
const H_AXIS: f64 = 20.0;
const H_CAPTION: f64 = 18.0;
const LEG: f64 = 16.0;
const SLOTS: [&str; 5] = ["s1", "s2", "s3", "s4", "s5"];
const MONTHS: [&str; 12] = [
    "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec",
];
const BAR_TICK: usize = 50;
const SERIES_CAPTION: &str = "IBM, daily, 1999";

/// The chart's whole look, shipped inside every SVG: the `var(--vp-c-*, fallback)` hooks
/// take the site's theme when the component inlines the file, and the fallbacks make the
/// bare file render on its own (noscript image, direct link, a crawler).
pub const STYLE: &str = ".ta-chart{font-family:ui-sans-serif,system-ui,sans-serif;font-size:11px;--ta-text:var(--vp-c-text,#3c3c43);--ta-mute:var(--vp-c-text-mute,#6b6b74);--ta-grid:var(--vp-c-divider,#e2e2e3);--ta-up:#1f9d55;--ta-down:#d6454a;--ta-s1:#2a78d6;--ta-s2:#eb6834;--ta-s3:#1baf7a;--ta-s4:#6250d6;--ta-s5:#eda100;--ta-vol:var(--vp-c-border,#c2c2c4)}[data-theme=\"dark\"] .ta-chart{--ta-up:#3cc47a;--ta-down:#ef6b6b;--ta-s1:#3987e5;--ta-s2:#e2693a;--ta-s3:#199e70;--ta-s4:#9085e9;--ta-s5:#c98500}.ta-chart text{fill:var(--ta-mute)}.ta-chart .grid{stroke:var(--ta-grid);stroke-width:1}.ta-chart .zero{stroke:var(--ta-mute);stroke-width:1;stroke-dasharray:3 3}.ta-chart .close{stroke:var(--ta-text);stroke-width:1.3;fill:none;opacity:.75}.ta-chart .up{stroke:var(--ta-up);fill:var(--ta-up)}.ta-chart .down{stroke:var(--ta-down);fill:var(--ta-down)}.ta-chart .wick{stroke-width:.8}.ta-chart .line{fill:none;stroke-width:1.6;stroke-linejoin:round}.ta-chart .dash{stroke-dasharray:5 3}.ta-chart .edge{fill:none;stroke-width:1.1}.ta-chart .fill{opacity:.1}.ta-chart .s1{stroke:var(--ta-s1)}.ta-chart .s2{stroke:var(--ta-s2)}.ta-chart .s3{stroke:var(--ta-s3)}.ta-chart .s4{stroke:var(--ta-s4)}.ta-chart .s5{stroke:var(--ta-s5)}.ta-chart .f1{fill:var(--ta-s1)}.ta-chart .f2{fill:var(--ta-s2)}.ta-chart .f3{fill:var(--ta-s3)}.ta-chart .f4{fill:var(--ta-s4)}.ta-chart .f5{fill:var(--ta-s5)}.ta-chart .vol{fill:var(--ta-vol)}.ta-chart .hollow{fill:none;stroke-width:1.2}.ta-chart .legend{fill:var(--ta-text);font-family:var(--vp-font-mono,ui-monospace,monospace);font-size:10.5px}.ta-chart .mono{font-family:var(--vp-font-mono,ui-monospace,monospace)}";


/// The reference run's CSV, as `ta_refdata` lays it out: `#` header lines, then
/// a header row `date,open,high,low,close,volume,<inputs>,<outputs>` and one row
/// per bar where an output cell is empty before its first value. The input
/// columns are the real inputs not fed a plain price column, named after the
/// argument (`inReal` for a synthetic series, `inPeriods`), and a column
/// between `volume` and the outputs is one when `# inputs:` names it.
#[derive(Debug, Clone, Default)]
pub struct RefData {
    pub dates: Vec<String>,
    pub open: Vec<f64>,
    pub high: Vec<f64>,
    pub low: Vec<f64>,
    pub close: Vec<f64>,
    pub volume: Vec<f64>,
    pub inputs: Vec<(String, Vec<Option<f64>>)>,
    pub outputs: Vec<(String, Vec<Option<f64>>)>,
    pub shifts: HashMap<String, i32>,
    /// `# inputs:` items, `(argument, source)`: `("inReal", "close")`,
    /// `("inPriceHLC", "HLC")`, `("inReal", "ramp[-1,1]")`.
    pub input_desc: Vec<(String, String)>,
    /// `# parameters:` items, `(name, value)`.
    pub parameters: Vec<(String, String)>,
    /// `# series:`, else the data file named by `# input:`.
    pub series: Option<String>,
    /// `# input: ...; flagged bars: N`, written for pattern functions.
    pub flagged: Option<u32>,
}

impl RefData {
    pub fn parse(text: &str) -> Result<RefData, String> {
        let mut data = RefData::default();
        let mut header: Option<Vec<String>> = None;
        for (lineno, raw) in text.lines().enumerate() {
            let line = raw.trim_end();
            if line.is_empty() {
                continue;
            }
            if let Some(meta) = line.strip_prefix('#') {
                let (key, value) = meta.split_once(':').unwrap_or((meta, ""));
                data.parse_meta(key.trim(), value.trim())?;
                continue;
            }
            let cells: Vec<&str> = line.split(',').collect();
            let Some(hdr) = &header else {
                let hdr: Vec<String> = cells.iter().map(|s| s.trim().to_string()).collect();
                if hdr.len() < 6 || hdr[..6] != ["date", "open", "high", "low", "close", "volume"] {
                    return Err(format!("line {}: unexpected column header", lineno + 1));
                }
                let n_in = hdr[6..]
                    .iter()
                    .take_while(|n| data.input_desc.iter().any(|(k, _)| k == *n))
                    .count();
                data.inputs = hdr[6..6 + n_in].iter().map(|n| (n.clone(), Vec::new())).collect();
                data.outputs = hdr[6 + n_in..].iter().map(|n| (n.clone(), Vec::new())).collect();
                header = Some(hdr);
                continue;
            };
            if cells.len() != hdr.len() {
                return Err(format!(
                    "line {}: {} cells, header has {}",
                    lineno + 1,
                    cells.len(),
                    hdr.len()
                ));
            }
            data.dates.push(cells[0].to_string());
            let num = |k: usize| -> Result<f64, String> {
                cells[k]
                    .trim()
                    .parse::<f64>()
                    .map_err(|e| format!("line {}: column {}: {e}", lineno + 1, hdr[k]))
            };
            data.open.push(num(1)?);
            data.high.push(num(2)?);
            data.low.push(num(3)?);
            data.close.push(num(4)?);
            data.volume.push(num(5)?);
            for (k, (_, col)) in data.inputs.iter_mut().chain(data.outputs.iter_mut()).enumerate() {
                let cell = cells[k + 6].trim();
                col.push(if cell.is_empty() {
                    None
                } else {
                    Some(num(k + 6)?).filter(|v| v.is_finite())
                });
            }
        }
        if header.is_none() {
            return Err("no column header".to_string());
        }
        if let Some((arg, src)) = data
            .input_desc
            .iter()
            .find(|(k, v)| !RefData::is_price_column(v) && data.input(k).is_none())
        {
            return Err(format!("input {arg}={src} has no `{arg}` column"));
        }
        Ok(data)
    }

    /// A source the price columns already hold: component letters or one column's name.
    fn is_price_column(src: &str) -> bool {
        matches!(src, "open" | "high" | "low" | "close" | "volume")
            || (!src.is_empty() && src.chars().all(|c| "OHLCV".contains(c)))
    }

    fn parse_meta(&mut self, key: &str, value: &str) -> Result<(), String> {
        match key {
            "inputs" => {
                for item in value.split_whitespace() {
                    let (k, v) = item.split_once('=').unwrap_or((item, ""));
                    self.input_desc.push((k.to_string(), v.to_string()));
                }
            }
            "series" => {
                if !value.is_empty() {
                    self.series = Some(value.to_string());
                }
            }
            "input" => {
                let file = value.split(';').next().unwrap_or("").trim();
                if self.series.is_none() && !file.is_empty() {
                    self.series = Some(file.to_string());
                }
                if let Some(n) = value.split("flagged bars:").nth(1) {
                    self.flagged = Some(
                        n.trim()
                            .parse::<u32>()
                            .map_err(|e| format!("input header `flagged bars: {}`: {e}", n.trim()))?,
                    );
                }
            }
            "parameters" => {
                for item in value.split_whitespace().filter(|t| *t != "(defaults)") {
                    let (k, v) = item.split_once('=').unwrap_or((item, ""));
                    self.parameters.push((k.to_string(), v.to_string()));
                }
            }
            "outputs" => {
                // `name (shift N) name (shift N) ...`
                let mut tokens = value.split_whitespace();
                while let Some(name) = tokens.next() {
                    let (tag, n) = (tokens.next(), tokens.next());
                    let shift = match (tag, n.and_then(|n| n.strip_suffix(')'))) {
                        (Some("(shift"), Some(n)) => n
                            .parse::<i32>()
                            .map_err(|e| format!("outputs header `{name}`: {e}"))?,
                        _ => return Err(format!("outputs header `{name}`: expected `(shift N)`")),
                    };
                    self.shifts.insert(name.to_string(), shift);
                }
            }
            _ => {}
        }
        Ok(())
    }

    fn output(&self, name: &str) -> &[Option<f64>] {
        self.outputs
            .iter()
            .find(|(n, _)| n == name)
            .map_or(&[][..], |(_, v)| v.as_slice())
    }

    /// The column of input argument `name`, when the file carries one.
    fn input(&self, name: &str) -> Option<&[Option<f64>]> {
        self.inputs.iter().find(|(n, _)| n == name).map(|(_, v)| v.as_slice())
    }

    fn shift(&self, name: &str) -> i32 {
        self.shifts.get(name).copied().unwrap_or(0)
    }

    fn series_caption(&self) -> &str {
        self.series.as_deref().unwrap_or(SERIES_CAPTION)
    }

    /// Every input the function read was synthesised: the price columns are in
    /// the file but the chart shows none of them, so it must not claim the series.
    fn all_inputs_synthetic(&self) -> bool {
        !self.input_desc.is_empty() && self.input_desc.iter().all(|(_, v)| synthetic_source(v).is_some())
    }

    /// The caption's lead: the series, or the bar count when no price was read.
    fn caption_lead(&self) -> String {
        if self.all_inputs_synthetic() {
            plural(u32::try_from(self.dates.len()).unwrap_or(u32::MAX), "bar")
        } else {
            self.series_caption().to_string()
        }
    }

    /// The price components the header says the function read, as lower-cased
    /// tokens: a bundle's letters (`inPriceHLC=HLC`) or a real input's column
    /// (`inReal0=high`), so a function fed high and low draws candles.
    fn header_components(&self) -> Vec<&'static str> {
        let mut comps = Vec::new();
        for (_, v) in &self.input_desc {
            let names: Vec<&'static str> = match v.as_str() {
                "open" => vec!["open"],
                "high" => vec!["high"],
                "low" => vec!["low"],
                "close" => vec!["close"],
                "volume" => vec!["volume"],
                letters if !letters.is_empty() && letters.chars().all(|c| "OHLCV".contains(c)) => letters
                    .chars()
                    .map(|c| match c {
                        'O' => "open",
                        'H' => "high",
                        'L' => "low",
                        'C' => "close",
                        _ => "volume",
                    })
                    .collect(),
                _ => Vec::new(),
            };
            for name in names {
                if !comps.contains(&name) {
                    comps.push(name);
                }
            }
        }
        comps
    }

    /// `inReal=ramp[lo,hi]` or `sine[lo,hi]`: the inputs the reference run synthesised,
    /// whose columns the top pane plots in place of the close; `(argument, source)`.
    fn synthetic_inputs(&self) -> Vec<(&str, &str)> {
        self.input_desc
            .iter()
            .filter(|(_, v)| synthetic_source(v).is_some())
            .map(|(k, v)| (k.as_str(), v.as_str()))
            .collect()
    }
}

/// `ramp[lo,hi]` or `sine[lo,hi]` as `(kind, lo, hi)`; `None` for any other source.
fn synthetic_source(src: &str) -> Option<(&str, &str, &str)> {
    let (kind, rest) = src.split_once('[')?;
    if !matches!(kind, "ramp" | "sine") {
        return None;
    }
    let (lo, hi) = rest.strip_suffix(']')?.split_once(',')?;
    Some((kind, lo, hi))
}

/// A synthetic source in words, for prose: "a ramp from -1 to 1", "a sine between 90 and 110".
fn synthetic_words(src: &str) -> String {
    match synthetic_source(src) {
        Some(("ramp", lo, hi)) => format!("a ramp from {lo} to {hi}"),
        Some((_, lo, hi)) => format!("a sine between {lo} and {hi}"),
        None => src.to_string(),
    }
}

/// A synthetic source for the caption: `ramp [-1, 1]`.
fn synthetic_caption(src: &str) -> String {
    match synthetic_source(src) {
        Some((kind, lo, hi)) => format!("{kind} [{lo}, {hi}]"),
        None => src.to_string(),
    }
}

#[derive(Debug, Clone)]
pub struct Chart {
    pub svg: String,
    /// The SVG's own accessible name, `<N> over <series>`.
    pub label: String,
    /// One plain-English sentence stating how the chart is drawn.
    pub sentence: String,
    /// The metadata that decided the drawing, as the site lists it: the function
    /// flags, then the inputs that shaped the panes, then each output with its flags.
    pub flags_used: Vec<String>,
    pub width: u32,
    pub height: u32,
}

/// At most one decimal, no trailing `.0`, never a negative zero.
pub fn fmt(v: f64) -> String {
    if !v.is_finite() {
        return "0".to_string();
    }
    let s = format!("{v:.1}");
    let s = s.strip_suffix(".0").unwrap_or(&s);
    if s == "-0" {
        "0".to_string()
    } else {
        s.to_string()
    }
}

/// Round tick positions over `[lo, hi]`, about `n` of them, at a 1/2/2.5/5 step.
#[allow(clippy::cast_possible_truncation)]
pub fn nice_ticks(lo: f64, hi: f64, n: u32) -> Vec<f64> {
    let hi = if hi <= lo { lo + 1.0 } else { hi };
    let raw = (hi - lo) / f64::from(n.max(1));
    let mag = 10f64.powi(raw.log10().floor() as i32);
    let mut step = 10.0 * mag;
    for m in [1.0, 2.0, 2.5, 5.0, 10.0] {
        if m * mag >= raw {
            step = m * mag;
            break;
        }
    }
    let mut ticks = Vec::new();
    let mut t = (lo / step).ceil() * step;
    while t <= hi + 1e-9 {
        ticks.push((t * 1e10).round() / 1e10);
        t += step;
    }
    ticks
}

/// C's `%.<p>g`: `p` significant digits, trailing zeros dropped.
fn fmt_g(v: f64, p: usize) -> String {
    if v == 0.0 {
        return "0".to_string();
    }
    let digits = i32::try_from(p).unwrap_or(i32::MAX);
    let sci = format!("{:.*e}", p - 1, v);
    let (mant, exp) = sci.split_once('e').unwrap_or((&sci, "0"));
    let exp: i32 = exp.parse().unwrap_or(0);
    let strip = |s: &str| -> String {
        if s.contains('.') {
            s.trim_end_matches('0').trim_end_matches('.').to_string()
        } else {
            s.to_string()
        }
    };
    if exp < -4 || exp >= digits {
        let sign = if exp < 0 { '-' } else { '+' };
        format!("{}e{}{:02}", strip(mant), sign, exp.abs())
    } else {
        let decimals = usize::try_from(digits - 1 - exp).unwrap_or(0);
        strip(&format!("{v:.decimals$}"))
    }
}

fn label(v: f64) -> String {
    let a = v.abs();
    if a >= 1e6 {
        format!("{}M", fmt_g(v / 1e6, 6))
    } else if a >= 1e4 {
        format!("{}k", fmt_g(v / 1e3, 6))
    } else if a >= 100.0 {
        format!("{v:.0}")
    } else if a >= 1.0 {
        fmt_g(v, 4)
    } else {
        fmt_g(v, 3)
    }
}

fn esc(s: &str) -> String {
    s.replace('&', "&amp;")
        .replace('<', "&lt;")
        .replace('>', "&gt;")
        .replace('"', "&quot;")
}

struct Pane {
    y: f64,
    h: f64,
    lo: Option<f64>,
    hi: Option<f64>,
}

impl Pane {
    fn new(y: f64, h: f64) -> Pane {
        Pane {
            y: y + LEG,
            h: h - LEG,
            lo: None,
            hi: None,
        }
    }

    fn extend<'a>(&mut self, vals: impl IntoIterator<Item = &'a Option<f64>>) {
        for v in vals.into_iter().flatten().filter(|v| v.is_finite()) {
            self.lo = Some(self.lo.map_or(*v, |lo| lo.min(*v)));
            self.hi = Some(self.hi.map_or(*v, |hi| hi.max(*v)));
        }
    }

    fn extend_plain(&mut self, vals: &[f64]) {
        for v in vals.iter().filter(|v| v.is_finite()) {
            self.lo = Some(self.lo.map_or(*v, |lo| lo.min(*v)));
            self.hi = Some(self.hi.map_or(*v, |hi| hi.max(*v)));
        }
    }

    fn finish(&mut self, pad: f64, include_zero: bool) -> (f64, f64) {
        let (mut lo, mut hi) = match (self.lo, self.hi) {
            (Some(lo), Some(hi)) => (lo, hi),
            _ => (0.0, 1.0),
        };
        if include_zero {
            lo = lo.min(0.0);
            hi = hi.max(0.0);
        }
        let mut span = hi - lo;
        if span == 0.0 {
            span = hi.abs();
        }
        if span == 0.0 {
            span = 1.0;
        }
        lo -= span * pad;
        hi += span * pad;
        if hi <= lo {
            hi = lo + 1.0;
        }
        self.lo = Some(lo);
        self.hi = Some(hi);
        (lo, hi)
    }

    fn lo(&self) -> f64 {
        self.lo.unwrap_or(0.0)
    }

    fn hi(&self) -> f64 {
        self.hi.unwrap_or(1.0)
    }

    fn sy(&self, v: f64) -> f64 {
        let (lo, hi) = (self.lo(), self.hi());
        self.y + self.h - (v - lo) / (hi - lo) * self.h
    }
}

enum LegendMark<'a> {
    None,
    Line { cls: &'a str, dashed: bool },
    Dot(&'a str),
    Diamond(&'a str),
    /// A sign marker pair in a slot colour.
    Sign(&'a str),
    Hist,
}

struct LegendItem<'a> {
    mark: LegendMark<'a>,
    text: String,
}

fn slot_digit(cls: &str) -> char {
    cls.chars().nth(1).unwrap_or('1')
}

fn has(flags: &[String], flag: &str) -> bool {
    flags.iter().any(|f| f == flag)
}

fn plural(n: u32, word: &str) -> String {
    if n == 1 {
        format!("{n} {word}")
    } else {
        format!("{n} {word}s")
    }
}

/// Joins `a, b and c`.
fn join_and(parts: &[String]) -> String {
    match parts.len() {
        0 => String::new(),
        1 => parts[0].clone(),
        n => format!("{} and {}", parts[..n - 1].join(", "), parts[n - 1]),
    }
}

struct Canvas<'a> {
    out: String,
    data: &'a RefData,
    n: usize,
    bw: f64,
}

impl<'a> Canvas<'a> {
    /// `max_shift` widens the x range so a shifted series stays on the pane.
    #[allow(clippy::cast_precision_loss)]
    fn new(data: &'a RefData, bars: usize, max_shift: i32) -> Canvas<'a> {
        let slots = (bars as f64 + f64::from(max_shift)).max(1.0);
        Canvas {
            out: String::with_capacity(16 * 1024),
            data,
            n: bars,
            bw: (W - PAD_L - PAD_R) / slots,
        }
    }

    /// Centre x of bar `i` drawn `shift` bars later; `None` when that bar lies
    /// before the first one, off the plot.
    #[allow(clippy::cast_precision_loss)]
    fn bar_x(&self, i: usize, shift: i32) -> Option<f64> {
        let slot = i as f64 + f64::from(shift);
        (slot >= 0.0).then_some(PAD_L + (slot + 0.5) * self.bw)
    }

    fn x_of(&self, i: usize) -> f64 {
        self.bar_x(i, 0).unwrap_or(PAD_L)
    }

    /// Where a marker for bar `i` drawn `shift` bars later sits: its x and the
    /// high and low of the bar it lands on; `None` off the plot.
    fn marker_bar(&self, i: usize, shift: i32) -> Option<(f64, f64, f64)> {
        let x = self.bar_x(i, shift)?;
        let at = usize::try_from(i64::try_from(i).ok()? + i64::from(shift)).ok()?;
        Some((x, *self.data.high.get(at)?, *self.data.low.get(at)?))
    }

    fn grid(&mut self, p: &Pane, show_zero: bool) {
        for t in nice_ticks(p.lo(), p.hi(), 4) {
            let yy = p.sy(t);
            if yy < p.y - 1.0 || yy > p.y + p.h + 1.0 {
                continue;
            }
            let _ = write!(
                self.out,
                "<line class=\"grid\" x1=\"{}\" x2=\"{}\" y1=\"{}\" y2=\"{}\"/>\n<text class=\"mono\" x=\"{}\" y=\"{}\">{}</text>\n",
                fmt(PAD_L),
                fmt(W - PAD_R),
                fmt(yy),
                fmt(yy),
                fmt(W - PAD_R + 6.0),
                fmt(yy + 3.5),
                label(t)
            );
        }
        if show_zero && p.lo() < 0.0 && 0.0 < p.hi() {
            let y0 = fmt(p.sy(0.0));
            let _ = writeln!(
                self.out,
                "<line class=\"zero\" x1=\"{}\" x2=\"{}\" y1=\"{y0}\" y2=\"{y0}\"/>",
                fmt(PAD_L),
                fmt(W - PAD_R)
            );
        }
    }

    /// A polyline with a gap wherever a value is missing.
    fn polyline(&mut self, p: &Pane, vals: &[Option<f64>], cls: &str, shift: i32) {
        let mut segs: Vec<Vec<(String, String)>> = Vec::new();
        let mut pts: Vec<(String, String)> = Vec::new();
        for (i, v) in vals.iter().enumerate() {
            let Some(v) = v.filter(|v| v.is_finite()) else {
                if !pts.is_empty() {
                    segs.push(std::mem::take(&mut pts));
                }
                continue;
            };
            let Some(x) = self.bar_x(i, shift) else { continue };
            pts.push((fmt(x), fmt(p.sy(v))));
        }
        if !pts.is_empty() {
            segs.push(pts);
        }
        for s in &segs {
            if s.len() == 1 {
                let _ = writeln!(
                    self.out,
                    "<circle class=\"{cls}\" r=\"1.5\" cx=\"{}\" cy=\"{}\"/>",
                    s[0].0, s[0].1
                );
            } else {
                let pts: Vec<String> = s.iter().map(|(x, y)| format!("{x},{y}")).collect();
                let _ = writeln!(
                    self.out,
                    "<polyline class=\"{cls}\" points=\"{}\"/>",
                    pts.join(" ")
                );
            }
        }
    }

    fn step_line(&mut self, p: &Pane, vals: &[Option<f64>], cls: &str, shift: i32) {
        let mut pts: Vec<String> = Vec::new();
        let mut last_y: Option<String> = None;
        for (i, v) in vals.iter().enumerate() {
            let Some(v) = v.filter(|v| v.is_finite()) else { continue };
            let Some(x) = self.bar_x(i, shift) else { continue };
            let x = fmt(x);
            if let Some(ly) = &last_y {
                pts.push(format!("{x},{ly}"));
            }
            let y = fmt(p.sy(v));
            pts.push(format!("{x},{y}"));
            last_y = Some(y);
        }
        if !pts.is_empty() {
            let _ = writeln!(
                self.out,
                "<polyline class=\"line {cls}\" points=\"{}\"/>",
                pts.join(" ")
            );
        }
    }

    fn histogram(&mut self, p: &Pane, vals: &[Option<f64>], shift: i32) {
        let y0 = p.sy(0.0);
        let hw = (self.bw * 0.33).max(0.5);
        let (mut pos, mut neg) = (String::new(), String::new());
        for (i, v) in vals.iter().enumerate() {
            let Some(v) = v.filter(|v| v.is_finite()) else { continue };
            let Some(x) = self.bar_x(i, shift) else { continue };
            let yv = p.sy(v);
            let d = if v >= 0.0 { &mut pos } else { &mut neg };
            let _ = write!(
                d,
                "M{} {}h{}V{}h{}Z",
                fmt(x - hw),
                fmt(y0.min(yv)),
                fmt(2.0 * hw),
                fmt(y0.max(yv)),
                fmt(-2.0 * hw)
            );
        }
        if !pos.is_empty() {
            let _ = writeln!(self.out, "<path class=\"up\" d=\"{pos}\"/>");
        }
        if !neg.is_empty() {
            let _ = writeln!(self.out, "<path class=\"down\" d=\"{neg}\"/>");
        }
    }

    fn candles(&mut self, p: &Pane) {
        let half = (self.bw * 0.36).max(0.6);
        let (mut ups, mut downs) = (String::new(), String::new());
        let data = self.data;
        for i in 0..self.n {
            let (open, close) = (data.open[i], data.close[i]);
            let x = self.x_of(i);
            let top = p.sy(open.max(close));
            let mut bot = p.sy(open.min(close));
            if bot - top < 0.8 {
                bot = top + 0.8;
            }
            let path = if close >= open { &mut ups } else { &mut downs };
            let _ = write!(
                path,
                "M{} {}V{}M{} {}h{}V{}h{}Z",
                fmt(x),
                fmt(p.sy(data.high[i])),
                fmt(p.sy(data.low[i])),
                fmt(x - half),
                fmt(top),
                fmt(2.0 * half),
                fmt(bot),
                fmt(-2.0 * half)
            );
        }
        if !ups.is_empty() {
            let _ = writeln!(self.out, "<path class=\"up wick\" d=\"{ups}\"/>");
        }
        if !downs.is_empty() {
            let _ = writeln!(self.out, "<path class=\"down wick\" d=\"{downs}\"/>");
        }
    }

    fn legend(&mut self, p: &Pane, items: &[LegendItem<'_>]) {
        let mut x = PAD_L + 2.0;
        let y = p.y - 5.0;
        for item in items {
            match &item.mark {
                LegendMark::Dot(cls) => {
                    let f = slot_digit(cls);
                    let _ = writeln!(
                        self.out,
                        "<circle class=\"f{f}\" r=\"2\" cx=\"{}\" cy=\"{}\"/><circle class=\"f{f}\" r=\"2\" cx=\"{}\" cy=\"{}\"/>",
                        fmt(x + 3.0),
                        fmt(y - 4.0),
                        fmt(x + 10.0),
                        fmt(y - 4.0)
                    );
                    x += 18.0;
                }
                LegendMark::Diamond(cls) => {
                    let _ = writeln!(
                        self.out,
                        "<path class=\"f{}\" d=\"M{} {}l4 4l-4 4l-4 -4z\"/>",
                        slot_digit(cls),
                        fmt(x + 5.0),
                        fmt(y - 8.0)
                    );
                    x += 14.0;
                }
                LegendMark::Sign(cls) => {
                    let _ = writeln!(
                        self.out,
                        "<path class=\"f{}\" d=\"M{} {}l-4 6h8zM{} {}l-4 -6h8z\"/>",
                        slot_digit(cls),
                        fmt(x + 4.0),
                        fmt(y - 9.0),
                        fmt(x + 13.0),
                        fmt(y - 1.0)
                    );
                    x += 20.0;
                }
                LegendMark::Hist => {
                    let _ = writeln!(
                        self.out,
                        "<rect class=\"up\" x=\"{}\" y=\"{}\" width=\"5\" height=\"7\"/><rect class=\"down\" x=\"{}\" y=\"{}\" width=\"5\" height=\"7\"/>",
                        fmt(x),
                        fmt(y - 7.0),
                        fmt(x + 6.0),
                        fmt(y - 7.0)
                    );
                    x += 16.0;
                }
                LegendMark::Line { cls, dashed } => {
                    let _ = writeln!(
                        self.out,
                        "<line class=\"line {cls}{}\" x1=\"{}\" x2=\"{}\" y1=\"{}\" y2=\"{}\"/>",
                        if *dashed { " dash" } else { "" },
                        fmt(x),
                        fmt(x + 14.0),
                        fmt(y - 3.5),
                        fmt(y - 3.5)
                    );
                    x += 19.0;
                }
                LegendMark::None => {}
            }
            let _ = writeln!(
                self.out,
                "<text class=\"legend\" x=\"{}\" y=\"{}\">{}</text>",
                fmt(x),
                fmt(y),
                esc(&item.text)
            );
            x += f64::from(u32::try_from(item.text.chars().count()).unwrap_or(u32::MAX)) * 6.3 + 14.0;
        }
    }
}

/// Where each output is drawn; derived from the metadata alone.
struct Plan<'a> {
    candles: bool,
    has_vol: bool,
    price_outs: Vec<&'a Output>,
    ind_outs: Vec<&'a Output>,
    int_outs: Vec<&'a Output>,
    pattern_outs: Vec<&'a Output>,
    /// A candlestick function's plain integer output: its sign is the candle's colour.
    sign_outs: Vec<&'a Output>,
}

fn plan<'a>(func: &'a FuncDef, data: &RefData) -> Plan<'a> {
    let is_candle = has(&func.flags, "candlestick");
    let overlap = has(&func.flags, "overlap");
    let mut comps: Vec<&str> = func
        .inputs
        .iter()
        .filter_map(|i| i.price.map(|p| p.component.token()))
        .collect();
    comps.extend(data.header_components());
    let reads = |c: PriceComponent| comps.contains(&c.token());
    let candles = is_candle
        || reads(PriceComponent::Open)
        || reads(PriceComponent::High)
        || reads(PriceComponent::Low);
    let has_vol = reads(PriceComponent::Volume);
    let mut p = Plan {
        candles,
        has_vol,
        price_outs: Vec::new(),
        ind_outs: Vec::new(),
        int_outs: Vec::new(),
        pattern_outs: Vec::new(),
        sign_outs: Vec::new(),
    };
    for o in &func.outputs {
        if o.flags.iter().any(|f| f.starts_with("pattern_")) {
            p.pattern_outs.push(o);
        } else if o.param_type == ParamType::Integer && is_candle {
            p.sign_outs.push(o);
        } else if o.param_type == ParamType::Integer {
            p.int_outs.push(o);
        } else if overlap {
            p.price_outs.push(o);
        } else {
            p.ind_outs.push(o);
        }
    }
    p
}

fn stroke_word(flags: &[String]) -> &'static str {
    if has(flags, "dot") {
        "dots"
    } else if has(flags, "dash_line") {
        "a dashed line"
    } else {
        "a solid line"
    }
}

fn shift_phrase(shift: i32) -> String {
    match shift.signum() {
        0 => String::new(),
        1 => format!(" moved {} forward", plural(shift.unsigned_abs(), "bar")),
        _ => format!(" moved {} back", plural(shift.unsigned_abs(), "bar")),
    }
}

fn line_class(slot: &str, dashed: bool) -> String {
    format!("line {slot}{}", if dashed { " dash" } else { "" })
}

struct Layout {
    price: Pane,
    ind: Option<Pane>,
    steps: Vec<Pane>,
    vol: Option<Pane>,
    total_h: f64,
}

fn layout(plan: &Plan<'_>) -> Layout {
    let mut cursor = TOP;
    let mut take = |h: f64| {
        let p = Pane::new(cursor, h);
        cursor += h + GAP;
        p
    };
    let price = take(H_PRICE);
    let ind = (!plan.ind_outs.is_empty()).then(|| take(H_IND));
    let step_h = if plan.ind_outs.is_empty() && plan.int_outs.len() == 1 { H_IND } else { H_INT_SMALL };
    let steps = plan.int_outs.iter().map(|_| take(step_h)).collect();
    let vol = plan.has_vol.then(|| take(H_VOL));
    Layout {
        price,
        ind,
        steps,
        vol,
        total_h: cursor - GAP + H_AXIS + H_CAPTION,
    }
}

/// The lines the top pane shows as the input, `(legend name, values)`: the close,
/// or every synthetic input column the reference run fed the function.
fn base_series<'d>(data: &'d RefData, synthetic: &[(&'d str, &str)]) -> Vec<(&'d str, Vec<Option<f64>>)> {
    if synthetic.is_empty() {
        return vec![("close", data.close.iter().map(|v| Some(*v)).collect())];
    }
    synthetic
        .iter()
        .map(|(arg, _)| (*arg, data.input(arg).expect("parse requires the column").to_vec()))
        .collect()
}

/// The k-th input line's `(path class, legend slot)`: the close's neutral stroke,
/// then the slot the overlap outputs reach last.
fn input_line_class(k: usize) -> (&'static str, &'static str) {
    if k == 0 { ("close", "close") } else { ("line s5", "s5") }
}

/// Per output class, the flags its drawing reads; an integer output's deciding fact
/// is its type, which is no flag.
const REAL_FLAGS_READ: [&str; 9] =
    ["line", "dot", "dash_line", "histogram", "upper_limit", "lower_limit", "zero", "negative", "display_shift"];
const PATTERN_FLAGS_READ: [&str; 4] = ["positive", "negative", "zero", "display_shift"];
const SIGN_FLAGS_READ: [&str; 3] = ["positive", "negative", "display_shift"];
const STEP_FLAGS_READ: [&str; 1] = ["display_shift"];

/// What decided the drawing, in the site's order: the function's `overlap` or
/// `candlestick` flag, the price bundles and synthetic inputs, then every output's
/// flags with its display shift when it has one.
fn flags_used(func: &FuncDef, data: &RefData, plan: &Plan<'_>) -> Vec<String> {
    let mut used: Vec<String> = Vec::new();
    for flag in ["overlap", "candlestick"] {
        if has(&func.flags, flag) {
            used.push(flag.to_string());
        }
    }
    let mut groups_seen: Vec<usize> = Vec::new();
    for input in &func.inputs {
        if let Some(p) = input.price {
            if groups_seen.contains(&p.group) {
                continue;
            }
            groups_seen.push(p.group);
            let comps: Vec<PriceComponent> = func
                .inputs
                .iter()
                .filter_map(|i| i.price.filter(|q| q.group == p.group).map(|q| q.component))
                .collect();
            let comps = super::price_bundle::canonical(&comps);
            let names: Vec<&str> = comps.iter().map(|c| c.token()).collect();
            used.push(format!("{}: {}", super::price_bundle::canonical_name(&comps), names.join(", ")));
        } else if let Some((lo, hi)) = input.domain {
            used.push(format!("{}: series {} [{}, {}]", input.name, input.series.token(), fmt(lo), fmt(hi)));
        }
    }
    let named = |outs: &[&Output], o: &Output| outs.iter().any(|p| p.name == o.name);
    for o in &func.outputs {
        let shift = data.shift(&o.name);
        let pattern = named(&plan.pattern_outs, o);
        let (lead, keep): (Option<&str>, &[&str]) = if pattern {
            (None, &PATTERN_FLAGS_READ)
        } else if named(&plan.sign_outs, o) {
            (Some("integer"), &SIGN_FLAGS_READ)
        } else if named(&plan.int_outs, o) {
            (Some("integer"), &STEP_FLAGS_READ)
        } else {
            (None, &REAL_FLAGS_READ)
        };
        let mut flags: Vec<String> = lead.map(String::from).into_iter().collect();
        if pattern {
            flags.extend(o.flags.iter().filter(|f| f.starts_with("pattern_")).cloned());
        }
        flags.extend(
            o.flags
                .iter()
                .filter(|f| keep.contains(&f.as_str()))
                .map(|f| if f == "display_shift" && shift != 0 { format!("{f} ({shift:+})") } else { f.clone() }),
        );
        used.push(if flags.is_empty() { o.name.clone() } else { format!("{}: {}", o.name, flags.join(", ")) });
    }
    used
}

/// The overlap outputs: strokes, the band fill, and each one's legend row.
fn draw_price_outputs<'p>(
    cv: &mut Canvas<'_>,
    plan: &Plan<'p>,
    price: &Pane,
    legend: &mut Vec<LegendItem<'p>>,
) -> Vec<String> {
    let data = cv.data;
    let find = |flag: &str| -> Vec<usize> {
        (0..plan.price_outs.len())
            .filter(|&k| has(&plan.price_outs[k].flags, flag))
            .collect()
    };
    let (upper, lower) = (find("upper_limit"), find("lower_limit"));
    let band = (upper.len() == 1 && lower.len() == 1).then(|| (upper[0], lower[0]));

    let mut phrases: Vec<String> = Vec::new();
    for (slot, o) in plan.price_outs.iter().enumerate() {
        let sh = data.shift(&o.name);
        let cls = SLOTS[slot % SLOTS.len()];
        let vals = data.output(&o.name);
        let is_edge = has(&o.flags, "upper_limit") || has(&o.flags, "lower_limit");
        if has(&o.flags, "dot") {
            let mut d = String::new();
            for (i, v) in vals.iter().enumerate() {
                let (Some(v), Some(x)) = (v, cv.bar_x(i, sh)) else { continue };
                let _ = write!(
                    d,
                    "M{} {}m-1.3 0a1.3 1.3 0 1 0 2.6 0a1.3 1.3 0 1 0 -2.6 0",
                    fmt(x),
                    fmt(price.sy(*v))
                );
            }
            let _ = writeln!(cv.out, "<path class=\"f{}\" d=\"{d}\"/>", slot_digit(cls));
            legend.push(LegendItem { mark: LegendMark::Dot(cls), text: o.name.clone() });
        } else if is_edge {
            cv.polyline(price, vals, &format!("edge {cls}"), sh);
            legend.push(LegendItem { mark: LegendMark::Line { cls, dashed: false }, text: o.name.clone() });
        } else {
            let dashed = has(&o.flags, "dash_line");
            cv.polyline(price, vals, &line_class(cls, dashed), sh);
            legend.push(LegendItem { mark: LegendMark::Line { cls, dashed }, text: o.name.clone() });
        }
        if band.is_some_and(|(u, l)| u == slot || l == slot) {
            continue;
        }
        let mut ph = if is_edge {
            format!("{} as a band edge", o.name)
        } else {
            format!("{} as {}", o.name, stroke_word(&o.flags))
        };
        ph.push_str(&shift_phrase(sh));
        phrases.push(ph);
    }

    if let Some((u, l)) = band {
        let (uo, lo) = (plan.price_outs[u], plan.price_outs[l]);
        let (uv, lv) = (data.output(&uo.name), data.output(&lo.name));
        let (ush, lsh) = (data.shift(&uo.name), data.shift(&lo.name));
        let point = |i: usize, v: Option<f64>, sh: i32| -> Option<String> {
            let v = v.filter(|v| v.is_finite())?;
            Some(format!("{},{}", fmt(cv.bar_x(i, sh)?), fmt(price.sy(v))))
        };
        let mut pts: Vec<String> =
            uv.iter().enumerate().filter_map(|(i, v)| point(i, *v, ush)).collect();
        let n_upper = pts.len();
        pts.extend(lv.iter().enumerate().rev().filter_map(|(i, v)| point(i, *v, lsh)));
        if n_upper > 0 && pts.len() > n_upper {
            let _ = writeln!(
                cv.out,
                "<polygon class=\"fill f{}\" points=\"{}\"/>",
                slot_digit(SLOTS[u % SLOTS.len()]),
                pts.join(" ")
            );
        }
        phrases.push(format!("a band between {} and {} with a soft fill", uo.name, lo.name));
    }
    phrases
}

/// Pattern outputs as markers on the bars they flag. Values are the pattern
/// levels (0, +-80, +-100, +-200), so comparing them exactly is sound.
#[allow(clippy::float_cmp)]
fn draw_pattern_markers<'p>(
    cv: &mut Canvas<'_>,
    plan: &Plan<'p>,
    price: &Pane,
    legend: &mut Vec<LegendItem<'p>>,
) -> Vec<String> {
    let data = cv.data;
    let mut phrases: Vec<String> = Vec::new();
    for (k, o) in plan.pattern_outs.iter().enumerate() {
        let vals = data.output(&o.name);
        let sh = data.shift(&o.name);
        let is_bool = has(&o.flags, "pattern_bool");
        let bcls = SLOTS[k % SLOTS.len()];
        let mut hits = 0u32;
        let (mut saw_weak, mut saw_confirm) = (false, false);
        for (i, v) in vals.iter().enumerate() {
            let Some(v) = *v else { continue };
            if v == 0.0 {
                continue;
            }
            hits += 1;
            let hollow = v.abs() == 80.0;
            let ring = v.abs() == 200.0;
            saw_weak |= hollow;
            saw_confirm |= ring;
            let Some((x, bar_high, bar_low)) = cv.marker_bar(i, sh) else { continue };
            if is_bool {
                let yy = price.sy(bar_high) - 6.0;
                let _ = writeln!(
                    cv.out,
                    "<path class=\"f{}\" d=\"M{} {}l4 4l-4 4l-4 -4z\"/>",
                    slot_digit(bcls),
                    fmt(x),
                    fmt(yy - 4.0)
                );
                continue;
            }
            let (yy, cls, d) = if v > 0.0 {
                let yy = price.sy(bar_low) + 5.0;
                (yy, "up", format!("M{} {}l-4 6h8z", fmt(x), fmt(yy)))
            } else {
                let yy = price.sy(bar_high) - 5.0;
                (yy, "down", format!("M{} {}l-4 -6h8z", fmt(x), fmt(yy)))
            };
            let _ = writeln!(
                cv.out,
                "<path class=\"{cls}{}\" d=\"{d}\"/>",
                if hollow { " hollow" } else { "" }
            );
            if ring {
                let _ = writeln!(
                    cv.out,
                    "<circle class=\"{cls} hollow\" r=\"6.5\" cx=\"{}\" cy=\"{}\"/>",
                    fmt(x),
                    fmt(yy + if v > 0.0 { 4.0 } else { -4.0 })
                );
            }
        }
        let none = hits == 0 || data.flagged == Some(0);
        legend.push(LegendItem {
            mark: if is_bool { LegendMark::Diamond(bcls) } else { LegendMark::None },
            text: occurrence_text(&o.name, hits, none),
        });
        let mut ph = if is_bool {
            format!("{} as a diamond above the high of every bar it flags", o.name)
        } else {
            let mut calls: Vec<String> = Vec::new();
            if has(&o.flags, "positive") {
                calls.push("\u{25B2} below the low for a bullish call".to_string());
            }
            if has(&o.flags, "negative") {
                calls.push(format!(
                    "\u{25BC} above the high for a bearish {}",
                    if calls.is_empty() { "call" } else { "one" }
                ));
            }
            let mut ph = format!("{} as markers on the candles", o.name);
            if !calls.is_empty() {
                let _ = write!(ph, ", {}", join_and(&calls));
            }
            ph
        };
        if !is_bool {
            let mut grades = Vec::new();
            if saw_weak || has(&o.flags, "pattern_weak") {
                grades.push("a weak call (80) hollow".to_string());
            }
            if saw_confirm || has(&o.flags, "pattern_confirm") {
                grades.push("a confirmed call (200) ringed".to_string());
            }
            if !grades.is_empty() {
                let _ = write!(ph, ", {}", join_and(&grades));
            }
        }
        ph.push_str(&shift_phrase(sh));
        if none {
            ph.push_str(" (no occurrence in the reference data)");
        }
        phrases.push(ph);
    }
    phrases
}

fn occurrence_text(name: &str, hits: u32, none: bool) -> String {
    if none {
        format!("{name}: no occurrence in the reference data")
    } else {
        format!("{name}: {} flagged", plural(hits, "bar"))
    }
}

/// A candlestick function's plain integer output as markers in its slot colour:
/// the sign is the candle's colour, not a call, so the marker says only which.
fn draw_sign_markers<'p>(
    cv: &mut Canvas<'_>,
    plan: &Plan<'p>,
    price: &Pane,
    legend: &mut Vec<LegendItem<'p>>,
) -> Vec<String> {
    let data = cv.data;
    let mut phrases: Vec<String> = Vec::new();
    for (k, o) in plan.sign_outs.iter().enumerate() {
        let vals = data.output(&o.name);
        let sh = data.shift(&o.name);
        let cls = SLOTS[(plan.pattern_outs.len() + k) % SLOTS.len()];
        let mut hits = 0u32;
        let mut d = String::new();
        for (i, v) in vals.iter().enumerate() {
            let Some(v) = *v else { continue };
            if v == 0.0 {
                continue;
            }
            hits += 1;
            let Some((x, bar_high, bar_low)) = cv.marker_bar(i, sh) else { continue };
            if v > 0.0 {
                let _ = write!(d, "M{} {}l-4 -6h8z", fmt(x), fmt(price.sy(bar_high) - 5.0));
            } else {
                let _ = write!(d, "M{} {}l-4 6h8z", fmt(x), fmt(price.sy(bar_low) + 5.0));
            }
        }
        if !d.is_empty() {
            let _ = writeln!(cv.out, "<path class=\"f{}\" d=\"{d}\"/>", slot_digit(cls));
        }
        let none = hits == 0 || data.flagged == Some(0);
        legend.push(LegendItem { mark: LegendMark::Sign(cls), text: occurrence_text(&o.name, hits, none) });
        let mut ph = format!(
            "{} as markers on the candles by sign (the candle's colour), \u{25B2} above the high for + and \u{25BC} below the low for \u{2212}",
            o.name
        );
        ph.push_str(&shift_phrase(sh));
        if none {
            ph.push_str(" (no occurrence in the reference data)");
        }
        phrases.push(ph);
    }
    phrases
}

fn draw_indicator_pane(cv: &mut Canvas<'_>, plan: &Plan<'_>, ind: &mut Pane) -> String {
    let data = cv.data;
    for o in &plan.ind_outs {
        ind.extend(data.output(&o.name));
    }
    let want_zero = plan.ind_outs.iter().any(|o| {
        has(&o.flags, "zero") || has(&o.flags, "negative") || has(&o.flags, "histogram")
    });
    ind.finish(0.08, want_zero);
    cv.grid(ind, true);
    let mut legend: Vec<LegendItem<'_>> = Vec::new();
    let mut slot = 0usize;
    let mut phrases: Vec<String> = Vec::new();
    for o in &plan.ind_outs {
        let sh = data.shift(&o.name);
        let cls = SLOTS[slot % SLOTS.len()];
        let vals = data.output(&o.name);
        if has(&o.flags, "histogram") {
            cv.histogram(ind, vals, sh);
            legend.push(LegendItem { mark: LegendMark::Hist, text: o.name.clone() });
            phrases.push(format!("{} as a sign-coloured histogram{}", o.name, shift_phrase(sh)));
        } else {
            let dashed = has(&o.flags, "dash_line");
            cv.polyline(ind, vals, &line_class(cls, dashed), sh);
            legend.push(LegendItem { mark: LegendMark::Line { cls, dashed }, text: o.name.clone() });
            phrases.push(format!("{} as {}{}", o.name, stroke_word(&o.flags), shift_phrase(sh)));
            slot += 1;
        }
    }
    cv.legend(ind, &legend);
    let zero_note = if want_zero || (ind.lo() < 0.0 && 0.0 < ind.hi()) {
        " with a zero line"
    } else {
        ""
    };
    format!("a separate indicator pane{zero_note} showing {}", join_and(&phrases))
}

/// One pane per integer output: two never share a scale.
fn draw_step_panes(cv: &mut Canvas<'_>, plan: &Plan<'_>, steps: &mut [Pane]) -> String {
    let data = cv.data;
    for (k, (o, pane)) in plan.int_outs.iter().zip(steps.iter_mut()).enumerate() {
        let vals = data.output(&o.name);
        pane.extend(vals);
        pane.finish(0.15, true);
        cv.grid(pane, false);
        let cls = SLOTS[k % SLOTS.len()];
        cv.step_line(pane, vals, cls, data.shift(&o.name));
        cv.legend(pane, &[LegendItem { mark: LegendMark::Line { cls, dashed: false }, text: o.name.clone() }]);
    }
    let names: Vec<String> = plan.int_outs.iter().map(|o| o.name.clone()).collect();
    if names.len() == 1 {
        format!("a step pane of its own for the integer output {}", names[0])
    } else {
        format!("a step pane of its own for each of {}", join_and(&names))
    }
}

fn draw_volume_pane(cv: &mut Canvas<'_>, vol: &mut Pane) {
    let data = cv.data;
    vol.extend_plain(&data.volume);
    vol.lo = Some(0.0);
    let (lo, hi) = vol.finish(0.0, false);
    vol.hi = Some(if hi > lo { hi * 1.05 } else { lo + 1.0 });
    let hw = (cv.bw * 0.36).max(0.5);
    let mut d = String::new();
    for (i, v) in data.volume.iter().enumerate() {
        let _ = write!(
            d,
            "M{} {}h{}V{}h{}Z",
            fmt(cv.x_of(i) - hw),
            fmt(vol.sy(*v)),
            fmt(2.0 * hw),
            fmt(vol.sy(0.0)),
            fmt(-2.0 * hw)
        );
    }
    let _ = writeln!(cv.out, "<path class=\"vol\" d=\"{d}\"/>");
    for t in nice_ticks(0.0, vol.hi(), 2).into_iter().skip(1) {
        let yy = vol.sy(t);
        if yy >= vol.y {
            let _ = writeln!(
                cv.out,
                "<text class=\"mono\" x=\"{}\" y=\"{}\">{}</text>",
                fmt(W - PAD_R + 6.0),
                fmt(yy + 3.5),
                label(t)
            );
        }
    }
    let _ = writeln!(
        cv.out,
        "<text class=\"legend\" x=\"{}\" y=\"{}\">volume</text>",
        fmt(PAD_L + 2.0),
        fmt(vol.y - 5.0)
    );
}

fn draw_axis(cv: &mut Canvas<'_>, total_h: f64, max_shift: i32) {
    let ax_y = total_h - H_CAPTION - H_AXIS + 4.0;
    let _ = writeln!(
        cv.out,
        "<line class=\"grid\" x1=\"{}\" x2=\"{}\" y1=\"{}\" y2=\"{}\"/>",
        fmt(PAD_L),
        fmt(W - PAD_R),
        fmt(ax_y),
        fmt(ax_y)
    );
    let ticks: Vec<(usize, String)> = if cv.data.all_inputs_synthetic() {
        (0..cv.data.dates.len()).step_by(BAR_TICK).map(|i| (i, i.to_string())).collect()
    } else {
        month_ticks(&cv.data.dates)
    };
    for (i, text) in ticks {
        let x = cv.x_of(i) - cv.bw / 2.0;
        let _ = writeln!(
            cv.out,
            "<line class=\"grid\" x1=\"{}\" x2=\"{}\" y1=\"{}\" y2=\"{}\"/>\n<text x=\"{}\" y=\"{}\">{}</text>",
            fmt(x),
            fmt(x),
            fmt(ax_y),
            fmt(ax_y + 4.0),
            fmt(x + 3.0),
            fmt(ax_y + 14.0),
            text
        );
    }
    if max_shift > 0 {
        let x = cv.x_of(cv.data.dates.len()) - cv.bw / 2.0;
        let _ = writeln!(
            cv.out,
            "<line class=\"zero\" x1=\"{}\" x2=\"{}\" y1=\"{}\" y2=\"{}\"/>",
            fmt(x),
            fmt(x),
            fmt(TOP + LEG),
            fmt(ax_y)
        );
    }
}

fn month_ticks(dates: &[String]) -> Vec<(usize, String)> {
    let mut ticks = Vec::new();
    let mut prev: Option<usize> = None;
    for (i, d) in dates.iter().enumerate() {
        let mo = d
            .get(5..7)
            .and_then(|m| m.parse::<usize>().ok())
            .filter(|m| (1..=12).contains(m));
        let Some(mo) = mo else { continue };
        if prev != Some(mo) {
            ticks.push((i, MONTHS[mo - 1].to_string()));
            prev = Some(mo);
        }
    }
    ticks
}

fn component_names(letters: &str) -> Option<String> {
    if letters.is_empty() || !letters.chars().all(|c| "OHLCV".contains(c)) {
        return None;
    }
    let names: Vec<&str> = letters
        .chars()
        .map(|c| match c {
            'O' => "open",
            'H' => "high",
            'L' => "low",
            'C' => "close",
            _ => "volume",
        })
        .collect();
    Some(names.join(", "))
}

fn draw_caption(cv: &mut Canvas<'_>, total_h: f64) {
    let ins: Vec<String> = cv
        .data
        .input_desc
        .iter()
        .map(|(k, v)| component_names(v).unwrap_or_else(|| format!("{k} = {}", synthetic_caption(v))))
        .collect();
    let mut cap = format!("{} \u{B7} input: {}", cv.data.caption_lead(), ins.join(" \u{B7} "));
    if !cv.data.parameters.is_empty() {
        cap.push_str(" \u{B7} default parameters");
    }
    let _ = writeln!(
        cv.out,
        "<text x=\"{}\" y=\"{}\">{}</text>",
        fmt(PAD_L),
        fmt(total_h - 4.0),
        esc(&cap)
    );
}

#[allow(clippy::cast_possible_truncation, clippy::cast_sign_loss)]
fn px(v: f64) -> u32 {
    v.round().max(0.0) as u32
}

/// The SVG's accessible name: `<N> over <series>`, or over the synthetic inputs in
/// words when the function read no price.
fn chart_label(func: &FuncDef, data: &RefData, synthetic: &[(&str, &str)]) -> String {
    if data.all_inputs_synthetic() {
        let words: Vec<String> = synthetic.iter().map(|(_, src)| synthetic_words(src)).collect();
        format!("{} over {}", func.name, join_and(&words))
    } else {
        format!("{} over {}", func.name, data.series_caption())
    }
}

pub fn render(func: &FuncDef, data: &RefData) -> Chart {
    let plan = plan(func, data);
    let bars = data.dates.len();
    let max_shift = func
        .outputs
        .iter()
        .map(|o| data.shift(&o.name))
        .max()
        .unwrap_or(0)
        .max(0);
    let Layout { mut price, mut ind, mut steps, mut vol, total_h } = layout(&plan);

    let synthetic = if plan.candles { Vec::new() } else { data.synthetic_inputs() };
    let base = base_series(data, &synthetic);
    if plan.candles {
        price.extend_plain(&data.high);
        price.extend_plain(&data.low);
    } else {
        for (_, vals) in &base {
            price.extend(vals);
        }
    }
    for o in &plan.price_outs {
        price.extend(data.output(&o.name));
    }
    price.finish(0.05, false);

    let mut cv = Canvas::new(data, bars, max_shift);
    let label = chart_label(func, data, &synthetic);
    let _ = writeln!(
        cv.out,
        "<svg class=\"ta-chart\" xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 {} {}\" role=\"img\" aria-label=\"{}\">",
        fmt(W),
        fmt(total_h),
        esc(&label)
    );
    cv.out.push_str(&format!("<style>{STYLE}</style>"));

    cv.grid(&price, false);
    let mut legend: Vec<LegendItem<'_>> = Vec::new();
    let mut sentence = if plan.candles {
        cv.candles(&price);
        "Drawn on the price pane over candles".to_string()
    } else {
        for (k, (name, vals)) in base.iter().enumerate() {
            let (path_cls, cls) = input_line_class(k);
            cv.polyline(&price, vals, path_cls, 0);
            if !synthetic.is_empty() {
                legend.push(LegendItem { mark: LegendMark::Line { cls, dashed: false }, text: (*name).to_string() });
            }
        }
        match synthetic.as_slice() {
            [] => "Drawn on the price pane over the close as a line".to_string(),
            [(_, src)] => format!("Drawn on the price pane over {} as a line", synthetic_words(src)),
            many => {
                let names: Vec<String> = many.iter().map(|(arg, _)| (*arg).to_string()).collect();
                let words: Vec<String> = many.iter().map(|(_, src)| synthetic_words(src)).collect();
                format!(
                    "Drawn on the price pane over the inputs {}, {} as lines",
                    join_and(&names),
                    join_and(&words)
                )
            }
        }
    };
    let price_phrases = draw_price_outputs(&mut cv, &plan, &price, &mut legend);
    let mut pattern_phrases = draw_pattern_markers(&mut cv, &plan, &price, &mut legend);
    pattern_phrases.extend(draw_sign_markers(&mut cv, &plan, &price, &mut legend));
    cv.legend(&price, &legend);

    let mut below: Vec<String> = Vec::new();
    if let Some(ind) = ind.as_mut() {
        below.push(draw_indicator_pane(&mut cv, &plan, ind));
    }
    if !steps.is_empty() {
        below.push(draw_step_panes(&mut cv, &plan, &mut steps));
    }
    if let Some(vol) = vol.as_mut() {
        draw_volume_pane(&mut cv, vol);
        below.push("a volume pane".to_string());
    }
    draw_axis(&mut cv, total_h, max_shift);
    draw_caption(&mut cv, total_h);
    cv.out.push_str("</svg>");

    for phrases in [&price_phrases, &pattern_phrases] {
        if !phrases.is_empty() {
            let _ = write!(sentence, ", with {}", join_and(phrases));
        }
    }
    if !below.is_empty() {
        let _ = write!(sentence, "; below it, {}", below.join(", then "));
    }
    sentence.push('.');

    Chart {
        svg: cv.out,
        label,
        sentence,
        flags_used: flags_used(func, data, &plan),
        width: px(W),
        height: px(total_h),
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::ir::{Input, Output};
    use std::path::{Path, PathBuf};

    fn input_dir() -> PathBuf {
        Path::new(env!("CARGO_MANIFEST_DIR")).join("../input")
    }

    /// The committed reference CSVs, or the directory `TA_CHART_PROTO_CSV` names.
    fn csv_dir() -> PathBuf {
        let dir = std::env::var_os("TA_CHART_PROTO_CSV").map_or_else(
            || Path::new(env!("CARGO_MANIFEST_DIR")).join("../..").join(super::super::docs_site::PUBLIC_FUNCTIONS),
            PathBuf::from,
        );
        assert!(dir.is_dir(), "{}: no reference CSV directory", dir.display());
        dir
    }

    /// Where the ignored writer puts its SVGs: `TA_CHART_SVG_OUT`, else next to
    /// an explicitly named CSV directory; never inside the repository by default.
    fn svg_out_dir() -> Option<PathBuf> {
        if let Some(dir) = std::env::var_os("TA_CHART_SVG_OUT") {
            return Some(PathBuf::from(dir));
        }
        Some(PathBuf::from(std::env::var_os("TA_CHART_PROTO_CSV")?).join("../rust_svg"))
    }

    fn corpus() -> Vec<(FuncDef, RefData)> {
        let csv_dir = csv_dir();
        let mut names: Vec<String> = std::fs::read_dir(&csv_dir)
            .expect("csv dir")
            .filter_map(Result::ok)
            .filter_map(|e| {
                let f = e.file_name().to_string_lossy().to_string();
                f.strip_suffix(".csv").map(str::to_string)
            })
            .collect();
        names.sort();
        let mut out = Vec::new();
        for name in names {
            let yaml = input_dir().join(&name).join(format!("{name}.yaml"));
            if !yaml.exists() {
                continue;
            }
            let func = crate::parser::yaml::parse_yaml(&yaml);
            let text = std::fs::read_to_string(csv_dir.join(format!("{name}.csv"))).expect("csv");
            let data = RefData::parse(&text).unwrap_or_else(|e| panic!("{name}.csv: {e}"));
            out.push((func, data));
        }
        out
    }

    fn func(name: &str, flags: &[&str], inputs: Vec<Input>, outputs: Vec<Output>) -> FuncDef {
        FuncDef {
            name: name.to_string(),
            group: String::new(),
            description: None,
            hint: None,
            flags: flags.iter().map(|s| (*s).to_string()).collect(),
            inputs,
            optional_inputs: vec![],
            outputs,
            lookback: None,
            display_shift: None,
            body: vec![],
            private_body: vec![],
            private_extra_params: vec![],
            private_param_init: vec![],
            has_explicit_private: false,
            opt_outs: vec![],
            header_comments: vec![],
            doc: None,
            streaming: false,
            alternates: vec![],
            resolved_stream_body: None,
        }
    }

    fn out(name: &str, ty: ParamType, flags: &[&str]) -> Output {
        Output {
            name: name.to_string(),
            param_type: ty,
            flags: flags.iter().map(|s| (*s).to_string()).collect(),
        }
    }

    #[test]
    fn fmt_keeps_one_decimal_and_drops_point_zero() {
        assert_eq!(fmt(12.0), "12");
        assert_eq!(fmt(12.04), "12");
        assert_eq!(fmt(12.06), "12.1");
        assert_eq!(fmt(-0.04), "0");
        assert_eq!(fmt(-3.25), "-3.2");
        assert_eq!(fmt(f64::NAN), "0");
    }

    #[test]
    fn nice_ticks_pick_a_round_step_inside_the_range() {
        assert_eq!(nice_ticks(0.0, 100.0, 4), vec![0.0, 25.0, 50.0, 75.0, 100.0]);
        assert_eq!(nice_ticks(91.3, 139.7, 4), vec![100.0, 120.0]);
        assert_eq!(nice_ticks(91.3, 129.7, 4), vec![100.0, 110.0, 120.0]);
        assert_eq!(nice_ticks(-1.1, 1.1, 4), vec![-1.0, 0.0, 1.0]);
        assert_eq!(nice_ticks(-1.1, 0.9, 4), vec![-1.0, -0.5, 0.0, 0.5]);
        assert_eq!(nice_ticks(5.0, 5.0, 4), vec![5.0, 5.25, 5.5, 5.75, 6.0]);
        let t = nice_ticks(0.0, 12_345_678.0, 2);
        assert_eq!(t, vec![0.0, 10_000_000.0]);
    }

    #[test]
    fn labels_follow_the_prototype() {
        assert_eq!(label(4_077_500.0), "4.0775M");
        assert_eq!(label(12_500.0), "12.5k");
        assert_eq!(label(123.4), "123");
        assert_eq!(label(92.25), "92.25");
        assert_eq!(label(2.5), "2.5");
        assert_eq!(label(0.5), "0.5");
        assert_eq!(label(0.0), "0");
        assert_eq!(label(-0.25), "-0.25");
    }

    /// A negative display shift moves a series back; bars that would land
    /// before the first one are not drawn.
    #[test]
    fn a_negative_shift_drops_bars_before_the_plot() {
        let f = func("DPO", &[], vec![Input::new("inReal", ParamType::Real)], vec![out("outReal", ParamType::Real, &["line", "display_shift"])]);
        let mut csv = String::from("# input: test.csv; window 1999-01-04..1999-01-08 (5 bars)\n# parameters: optInTimePeriod=2 (defaults)\n# inputs: inReal=close\n# outputs: outReal (shift -2)\ndate,open,high,low,close,volume,outReal\n");
        for (i, c) in [10.0, 11.0, 12.0, 11.0, 10.0].iter().enumerate() {
            let _ = writeln!(csv, "1999-01-0{},{c},{c},{c},{c},100,{}", i + 4, c + 1.0);
        }
        let data = RefData::parse(&csv).unwrap();
        assert_eq!(data.series.as_deref(), Some("test.csv"));
        assert_eq!(data.parameters, vec![("optInTimePeriod".to_string(), "2".to_string())]);
        let chart = render(&f, &data);
        let line = chart.svg.lines().find(|l| l.starts_with("<polyline class=\"line s1\"")).unwrap();
        let xs: Vec<f64> = line.split("points=\"").nth(1).unwrap().trim_end_matches("\"/>").split(' ').map(|pt| pt.split(',').next().unwrap().parse().unwrap()).collect();
        assert_eq!(xs.len(), 3, "{line}");
        assert!(xs.iter().all(|x| *x >= PAD_L), "{line}");
        assert!(chart.svg.contains("aria-label=\"DPO over test.csv\""));
        assert!(chart.svg.contains(">test.csv \u{B7} input: inReal = close \u{B7} default parameters</text>"), "{}", chart.svg);
        assert!(chart.sentence.contains("moved 2 bars back"), "{}", chart.sentence);
    }

    /// Five candles with distinct highs and lows, `outputs` columns appended per bar.
    fn candle_csv(header: &str, outputs: &str, rows: &[&str]) -> RefData {
        let mut csv = format!("# input: test.csv; window 1999-01-04..1999-01-08 (5 bars){header}\n# parameters: (defaults)\n# inputs: inPriceOHLC=OHLC\n# outputs: {outputs}\ndate,open,high,low,close,volume");
        for name in outputs.split_whitespace().filter(|t| !t.starts_with('(') && !t.ends_with(')')) {
            let _ = write!(csv, ",{name}");
        }
        csv.push('\n');
        for (i, cells) in rows.iter().enumerate() {
            let hi = 20.0 + f64::from(u8::try_from(i).unwrap()) * 10.0;
            let _ = writeln!(csv, "1999-01-0{},{},{hi},{},{},100,{cells}", i + 4, hi - 5.0, hi - 10.0, hi - 4.0);
        }
        RefData::parse(&csv).unwrap()
    }

    /// The price pane as `render` scales it over candles.
    fn price_pane(d: &RefData) -> Pane {
        let mut p = Pane::new(TOP, H_PRICE);
        p.extend_plain(&d.high);
        p.extend_plain(&d.low);
        p.finish(0.05, false);
        p
    }

    /// The `d` of the first path line starting with `needle`: the data path, which
    /// precedes the legend's.
    fn first_path(svg: &str, needle: &str) -> Option<String> {
        let l = svg.lines().find(|l| l.starts_with(needle))?;
        Some(l.split("d=\"").nth(1)?.split('"').next()?.to_string())
    }

    /// A shifted pattern marker sits on the bar it is drawn on, not the bar that flagged it.
    #[test]
    fn a_shifted_pattern_marker_takes_the_drawn_bars_high_and_low() {
        let f = func(
            "P",
            &["candlestick"],
            vec![],
            vec![
                out("outA", ParamType::Integer, &["pattern_bool", "display_shift"]),
                out("outB", ParamType::Integer, &["pattern_bull_bear", "display_shift"]),
            ],
        );
        // Bar 2 flags both; drawn 2 bars back it lands on bar 0 (high 20, low 10).
        let d = candle_csv("; flagged bars: 1", "outA (shift -2) outB (shift -2)", &["0,0", "0,0", "100,-100", "0,0", "0,0"]);
        let chart = render(&f, &d);
        let p = price_pane(&d);
        let (x0, high0) = (fmt(PAD_L + 0.5 * (W - PAD_L - PAD_R) / 5.0), p.sy(d.high[0]));
        assert_eq!(
            first_path(&chart.svg, "<path class=\"f1\"").as_deref(),
            Some(format!("M{x0} {}l4 4l-4 4l-4 -4z", fmt(high0 - 10.0)).as_str()),
            "{}",
            chart.svg
        );
        assert_eq!(
            first_path(&chart.svg, "<path class=\"down\"").as_deref(),
            Some(format!("M{x0} {}l-4 -6h8z", fmt(high0 - 5.0)).as_str()),
            "{}",
            chart.svg
        );
        // Shifted off the plot: no marker, the hit still counts.
        let d = candle_csv("; flagged bars: 1", "outA (shift -2) outB (shift -2)", &["0,0", "100,100", "0,0", "0,0", "0,0"]);
        let chart = render(&f, &d);
        assert!(first_path(&chart.svg, "<path class=\"f1\" d=\"M").unwrap().starts_with("M17 9l4 4"), "{}", chart.svg);
        assert!(first_path(&chart.svg, "<path class=\"up\" d=\"M").is_none(), "{}", chart.svg);
        assert!(chart.svg.contains(">outA: 1 bar flagged<") && chart.svg.contains(">outB: 1 bar flagged<"), "{}", chart.svg);
        // Shifted past the last bar: the same.
        let d = candle_csv("; flagged bars: 1", "outA (shift 2) outB (shift 2)", &["0,0", "0,0", "0,0", "0,0", "100,100"]);
        let chart = render(&f, &d);
        assert!(first_path(&chart.svg, "<path class=\"f1\" d=\"M").unwrap().starts_with("M17 9l4 4"), "{}", chart.svg);
        assert!(first_path(&chart.svg, "<path class=\"up\" d=\"M").is_none(), "{}", chart.svg);
    }

    /// `flags_used` names only what each output class's drawing read: no `line` on
    /// a pattern output, no `positive`/`negative` on a plain line, an integer
    /// output's type in place of flags.
    #[test]
    fn flags_used_lists_what_each_output_class_read() {
        let macd = func(
            "MACD",
            &[],
            vec![Input::new("inReal", ParamType::Real)],
            vec![
                out("outMACD", ParamType::Real, &["line", "zero", "positive", "negative"]),
                out("outMACDSignal", ParamType::Real, &["dash_line", "negative"]),
                out("outMACDHist", ParamType::Real, &["histogram", "zero", "negative", "positive"]),
            ],
        );
        let mut csv = String::from("# input: test.csv; window 1999-01-04..1999-01-05 (2 bars)\n# parameters: optInFastPeriod=12 (defaults)\n# inputs: inReal=close\n# outputs: outMACD (shift 0) outMACDSignal (shift 0) outMACDHist (shift 0)\ndate,open,high,low,close,volume,outMACD,outMACDSignal,outMACDHist\n");
        csv.push_str("1999-01-04,1,2,0.5,1.5,100,1,2,-1\n1999-01-05,1,2,0.5,1.6,100,2,1,1\n");
        let chart = render(&macd, &RefData::parse(&csv).unwrap());
        assert_eq!(
            chart.flags_used,
            vec!["outMACD: line, zero, negative", "outMACDSignal: dash_line, negative", "outMACDHist: histogram, zero, negative"]
        );

        let ohlc = [PriceComponent::Open, PriceComponent::High, PriceComponent::Low, PriceComponent::Close]
            .into_iter()
            .map(|component| Input {
                price: Some(crate::ir::PriceRef { group: 0, component }),
                ..Input::new(component.input_name(), ParamType::Real)
            })
            .collect();
        let cdl = func(
            "CDLX",
            &["candlestick"],
            ohlc,
            vec![
                out("outInteger", ParamType::Integer, &["line", "pattern_bull_bear", "zero", "negative", "display_shift", "pattern_weak"]),
                out("outSign", ParamType::Integer, &["line", "zero", "positive", "negative"]),
                out("outFlag", ParamType::Integer, &["line", "pattern_bool", "zero", "positive"]),
            ],
        );
        let d = candle_csv("; flagged bars: 1", "outInteger (shift -1) outSign (shift 0) outFlag (shift 0)", &["0,0,0", "-100,100,0", "0,0,100", "0,-100,0", "0,0,0"]);
        let chart = render(&cdl, &d);
        assert_eq!(
            chart.flags_used,
            vec![
                "candlestick",
                "inPriceOHLC: open, high, low, close",
                "outInteger: pattern_bull_bear, pattern_weak, zero, negative, display_shift (-1)",
                "outSign: integer, positive, negative",
                "outFlag: pattern_bool, zero, positive",
            ]
        );

        let step = func("HT_TRENDMODE", &[], vec![Input::new("inReal", ParamType::Real)], vec![out("outTrend", ParamType::Integer, &["line", "zero", "positive"])]);
        let csv = "# input: test.csv; window 1999-01-04..1999-01-05 (2 bars)\n# parameters: (defaults)\n# inputs: inReal=close\n# outputs: outTrend (shift 0)\ndate,open,high,low,close,volume,outTrend\n1999-01-04,1,2,0.5,1.5,100,1\n1999-01-05,1,2,0.5,1.6,100,0\n";
        let chart = render(&step, &RefData::parse(csv).unwrap());
        assert_eq!(chart.flags_used, vec!["outTrend: integer"]);
    }

    /// The call markers a pattern sentence promises are the ones its sign flags
    /// declare: a bearish-only pattern never mentions a bullish marker.
    #[test]
    fn the_pattern_sentence_follows_the_outputs_sign_flags() {
        let both = out("outInteger", ParamType::Integer, &["line", "pattern_bull_bear", "zero", "positive", "negative"]);
        let bear = out("outInteger", ParamType::Integer, &["line", "pattern_bull_bear", "zero", "negative"]);
        let bull = out("outInteger", ParamType::Integer, &["line", "pattern_bull_bear", "zero", "positive"]);
        let d = candle_csv("; flagged bars: 1", "outInteger (shift 0)", &["0", "0", "-100", "0", "0"]);
        let s = render(&func("CDLX", &["candlestick"], vec![], vec![both]), &d).sentence;
        assert!(s.contains("outInteger as markers on the candles, \u{25B2} below the low for a bullish call and \u{25BC} above the high for a bearish one"), "{s}");
        let s = render(&func("CDL3BLACKCROWS", &["candlestick"], vec![], vec![bear]), &d).sentence;
        assert!(s.contains("outInteger as markers on the candles, \u{25BC} above the high for a bearish call"), "{s}");
        assert!(!s.contains('\u{25B2}') && !s.contains("bullish"), "{s}");
        let s = render(&func("CDLX", &["candlestick"], vec![], vec![bull]), &d).sentence;
        assert!(s.contains("outInteger as markers on the candles, \u{25B2} below the low for a bullish call"), "{s}");
        assert!(!s.contains('\u{25BC}') && !s.contains("bearish"), "{s}");
    }

    /// A candlestick function's plain integer output is a slot-coloured marker by
    /// sign, above the high for + and below the low for -, never a step pane.
    #[test]
    fn a_candle_sign_output_marks_the_candles() {
        let f = func("CDLX", &["candlestick"], vec![], vec![out("outInteger", ParamType::Integer, &["line", "zero", "positive", "negative"])]);
        let d = candle_csv("; flagged bars: 2", "outInteger (shift 0)", &["0", "100", "0", "-100", "0"]);
        let chart = render(&f, &d);
        assert!(!chart.svg.contains("<polyline class=\"line s1\""), "{}", chart.svg);
        let p = price_pane(&d);
        let x = |i: f64| fmt(PAD_L + (i + 0.5) * (W - PAD_L - PAD_R) / 5.0);
        assert_eq!(
            first_path(&chart.svg, "<path class=\"f1\"").as_deref(),
            Some(format!("M{} {}l-4 -6h8zM{} {}l-4 6h8z", x(1.0), fmt(p.sy(d.high[1]) - 5.0), x(3.0), fmt(p.sy(d.low[3]) + 5.0)).as_str()),
            "{}",
            chart.svg
        );
        assert!(chart.svg.contains(">outInteger: 2 bars flagged<"), "{}", chart.svg);
        assert_eq!(chart.height, px(TOP + H_PRICE + H_AXIS + H_CAPTION));
        assert!(chart.sentence.contains("by sign (the candle's colour), \u{25B2} above the high for + and \u{25BC} below the low for \u{2212}"), "{}", chart.sentence);
        assert!(!chart.sentence.contains("bullish"), "{}", chart.sentence);
        let d = candle_csv("; flagged bars: 0", "outInteger (shift 0)", &["0", "0", "0", "0", "0"]);
        let chart = render(&f, &d);
        assert!(chart.svg.contains(">outInteger: no occurrence in the reference data<"), "{}", chart.svg);
    }

    /// Two integer outputs never share a scale: one step pane each.
    #[test]
    fn each_integer_output_gets_its_own_step_pane() {
        let f = func(
            "ZZ",
            &["overlap"],
            vec![],
            vec![
                out("outZigZag", ParamType::Real, &["line"]),
                out("outTrend", ParamType::Integer, &["line"]),
                out("outPivotIdx", ParamType::Integer, &["line"]),
            ],
        );
        let d = candle_csv("", "outZigZag (shift 0) outTrend (shift 0) outPivotIdx (shift 0)", &["20,1,0", "30,1,0", "40,-1,2", "50,-1,2", "60,1,4"]);
        let chart = render(&f, &d);
        assert_eq!(chart.height, px(TOP + H_PRICE + GAP + 2.0 * (H_INT_SMALL + GAP) - GAP + H_AXIS + H_CAPTION));
        assert_eq!(chart.svg.matches("<text class=\"legend\"").count(), 3, "{}", chart.svg);
        assert!(chart.sentence.ends_with("a step pane of its own for each of outTrend and outPivotIdx."), "{}", chart.sentence);
        let f1 = func("ST", &["overlap"], vec![], vec![out("outSupertrend", ParamType::Real, &["line"]), out("outTrend", ParamType::Integer, &["line"])]);
        let d = candle_csv("", "outSupertrend (shift 0) outTrend (shift 0)", &["20,1", "30,1", "40,-1", "50,-1", "60,1"]);
        let chart = render(&f1, &d);
        assert_eq!(chart.height, px(TOP + H_PRICE + GAP + H_IND + H_AXIS + H_CAPTION));
        assert!(chart.sentence.ends_with("a step pane of its own for the integer output outTrend."), "{}", chart.sentence);
    }

    /// The band fill follows the edges' display shift.
    #[test]
    fn the_band_fill_follows_the_edges_shift() {
        let f = func(
            "B",
            &["overlap"],
            vec![Input::new("inReal", ParamType::Real)],
            vec![out("outUp", ParamType::Real, &["upper_limit", "display_shift"]), out("outLo", ParamType::Real, &["lower_limit", "display_shift"])],
        );
        let mut csv = String::from("# input: test.csv; window 1999-01-04..1999-01-08 (5 bars)\n# parameters: (defaults)\n# inputs: inReal=close\n# outputs: outUp (shift -1) outLo (shift -1)\ndate,open,high,low,close,volume,outUp,outLo\n");
        for i in 0..5u8 {
            let c = 10.0 + f64::from(i);
            let _ = writeln!(csv, "1999-01-0{},{c},{c},{c},{c},100,{},{}", i + 4, c + 1.0, c - 1.0);
        }
        let d = RefData::parse(&csv).unwrap();
        let chart = render(&f, &d);
        let edge = chart.svg.lines().find(|l| l.starts_with("<polyline class=\"edge s1\"")).unwrap();
        let fill = chart.svg.lines().find(|l| l.starts_with("<polygon class=\"fill f1\"")).unwrap();
        let pts = |l: &str| l.split("points=\"").nth(1).unwrap().split('"').next().unwrap().to_string();
        let (edge_pts, fill_pts) = (pts(edge), pts(fill));
        assert!(fill_pts.starts_with(&edge_pts), "edge {edge_pts}\nfill {fill_pts}");
        assert_eq!(fill_pts.split(' ').count(), 8, "{fill_pts}");
    }

    /// The x-axis tick labels: unclassed `<text>` other than the caption.
    fn axis_labels(svg: &str) -> Vec<&str> {
        svg.lines()
            .filter(|l| l.starts_with("<text x=") && !l.contains("input:"))
            .filter_map(|l| l.split('>').nth(1)?.strip_suffix("</text"))
            .collect()
    }

    /// A column named after an input argument is the input the function read; the
    /// top pane plots it, and a non-price input without one is a parse error.
    #[test]
    fn an_input_column_is_plotted_as_the_input() {
        let f = func("ACOS", &[], vec![Input::new("inReal", ParamType::Real)], vec![out("outReal", ParamType::Real, &["line"])]);
        let csv = "# input: test.csv; window 1999-01-04..1999-01-06 (3 bars)\n# parameters: (defaults)\n# inputs: inReal=ramp[-1,1]\n# outputs: outReal (shift 0)\ndate,open,high,low,close,volume,inReal,outReal\n1999-01-04,1,2,0.5,1,100,-0.5,2\n1999-01-05,1,2,0.5,1.5,100,0.25,1\n1999-01-06,1,2,0.5,2,100,0.5,0\n";
        let d = RefData::parse(csv).unwrap();
        assert_eq!(d.inputs.len(), 1);
        assert_eq!(d.input("inReal"), Some(&[Some(-0.5), Some(0.25), Some(0.5)][..]));
        assert_eq!(d.output("outReal"), &[Some(2.0), Some(1.0), Some(0.0)]);
        assert_eq!(d.synthetic_inputs(), vec![("inReal", "ramp[-1,1]")]);
        assert_eq!(base_series(&d, &d.synthetic_inputs()), vec![("inReal", vec![Some(-0.5), Some(0.25), Some(0.5)])]);
        let chart = render(&f, &d);
        assert!(chart.sentence.starts_with("Drawn on the price pane over a ramp from -1 to 1 as a line;"), "{}", chart.sentence);
        assert!(!chart.sentence.contains('['), "{}", chart.sentence);
        assert!(chart.svg.contains("input: inReal = ramp [-1, 1]"), "{}", chart.svg);
        assert!(chart.svg.contains(">inReal</text>"), "{}", chart.svg);
        assert_eq!(chart.svg.matches("<polyline class=\"close\"").count(), 1, "{}", chart.svg);
        assert_eq!(chart.label, "ACOS over a ramp from -1 to 1");
        assert!(chart.svg.contains(">3 bars \u{B7} input: inReal = ramp [-1, 1]<"), "{}", chart.svg);
        assert!(!chart.svg.contains("test.csv"), "{}", chart.svg);
        assert!(chart.svg.contains("aria-label=\"ACOS over a ramp from -1 to 1\""), "{}", chart.svg);
        let without = csv.replace(",inReal,", ",").replace(",-0.5,2", ",2").replace(",0.25,1", ",1").replace(",0.5,0", ",0");
        assert_eq!(RefData::parse(&without).unwrap_err(), "input inReal=ramp[-1,1] has no `inReal` column");
        assert_eq!(axis_labels(&chart.svg), vec!["0"], "{}", chart.svg);
        let on_close = render(&f, &RefData::parse(&without.replace("ramp[-1,1]", "close")).unwrap());
        assert_eq!(axis_labels(&on_close.svg), vec!["Jan"], "{}", on_close.svg);
        assert!(RefData::parse(&without.replace("inReal=ramp[-1,1]", "inPeriods=5(first-half)|20(second-half)")).is_err());
        let sine = RefData::parse(&csv.replace("ramp[-1,1]", "sine[90,110]")).unwrap();
        assert_eq!(sine.synthetic_inputs(), vec![("inReal", "sine[90,110]")]);
        assert!(render(&f, &sine).sentence.starts_with("Drawn on the price pane over a sine between 90 and 110 as a line;"));
        assert_eq!(synthetic_source("close"), None);
        assert_eq!(synthetic_source("5(first-half)|20(second-half)"), None);
        assert_eq!(synthetic_source("ramp[0.1,10]"), Some(("ramp", "0.1", "10")));
    }

    /// Two synthetic inputs share the top pane: one line each, named in the legend,
    /// and the sentence names both.
    #[test]
    fn two_synthetic_inputs_are_both_plotted_on_the_top_pane() {
        let mut in0 = Input::new("inReal0", ParamType::Real);
        in0.series = crate::ir::Series::Sine;
        in0.domain = Some((90.0, 110.0));
        let mut in1 = Input::new("inReal1", ParamType::Real);
        in1.series = crate::ir::Series::Ramp;
        in1.domain = Some((95.0, 105.0));
        let f = func("ADD", &[], vec![in0, in1], vec![out("outReal", ParamType::Real, &["line"])]);
        let csv = "# input: test.csv; window 1999-01-04..1999-01-06 (3 bars)\n# parameters: (defaults)\n# inputs: inReal0=sine[90,110] inReal1=ramp[95,105]\n# outputs: outReal (shift 0)\ndate,open,high,low,close,volume,inReal0,inReal1,outReal\n1999-01-04,1,2,0.5,1,100,100,95,195\n1999-01-05,1,2,0.5,1.5,100,101,100,201\n1999-01-06,1,2,0.5,2,100,102,105,207\n";
        let d = RefData::parse(csv).unwrap();
        assert_eq!(d.inputs.len(), 2);
        assert_eq!(d.synthetic_inputs(), vec![("inReal0", "sine[90,110]"), ("inReal1", "ramp[95,105]")]);
        let chart = render(&f, &d);
        assert!(
            chart.sentence.starts_with("Drawn on the price pane over the inputs inReal0 and inReal1, a sine between 90 and 110 and a ramp from 95 to 105 as lines;"),
            "{}",
            chart.sentence
        );
        assert_eq!(chart.svg.matches("<polyline class=\"close\"").count(), 1, "{}", chart.svg);
        assert_eq!(chart.svg.matches("<polyline class=\"line s5\"").count(), 1, "{}", chart.svg);
        let legend_in0 = chart.svg.find(">inReal0</text>").expect("inReal0 in the legend");
        let legend_in1 = chart.svg.find(">inReal1</text>").expect("inReal1 in the legend");
        assert!(legend_in0 < legend_in1);
        assert!(chart.svg.contains(">3 bars \u{B7} input: inReal0 = sine [90, 110] \u{B7} inReal1 = ramp [95, 105]<"), "{}", chart.svg);
        assert_eq!(chart.label, "ADD over a sine between 90 and 110 and a ramp from 95 to 105");
        // The top pane's range spans both lines, not the close.
        assert!(!chart.svg.contains(">1.5<"), "{}", chart.svg);
        assert_eq!(
            chart.flags_used,
            vec!["inReal0: series sine [90, 110]", "inReal1: series ramp [95, 105]", "outReal: line"]
        );
    }

    #[test]
    fn ref_data_header_is_parsed_once() {
        let text = "# TA-Lib reference data: X\n# series: IBM, daily, 1999\n# input: ibm_1999.csv; window 1999-01-04..1999-01-05 (2 bars); flagged bars: 1\n# parameters: (defaults)\n# inputs: inReal=ramp[-1,1] inPriceV=V\n# outputs: outA (shift 8) outB (shift 0)\n# lookback: 0\ndate,open,high,low,close,volume,inReal,outA,outB\n1999-01-04,1,2,0.5,1.5,100,-1,,\n1999-01-05,1,2,0.5,1.6,100,1,nan,3\n";
        let d = RefData::parse(text).unwrap();
        assert_eq!(d.shift("outA"), 8);
        assert_eq!(d.shift("outB"), 0);
        assert_eq!(d.input_desc, vec![("inReal".to_string(), "ramp[-1,1]".to_string()), ("inPriceV".to_string(), "V".to_string())]);
        assert!(d.parameters.is_empty());
        assert_eq!(d.series.as_deref(), Some("IBM, daily, 1999"));
        assert_eq!(d.flagged, Some(1));
        assert_eq!(d.output("outA"), &[None, None]);
        assert_eq!(d.output("outB"), &[None, Some(3.0)]);
        assert_eq!(d.header_components(), vec!["volume"]);
        let hl = RefData::parse(&text.replace("inReal=ramp[-1,1] inPriceV=V", "inReal0=high inReal1=low").replace(",inReal,", ",").replace(",-1,,", ",,").replace(",1,nan,3", ",nan,3")).unwrap();
        assert_eq!(hl.header_components(), vec!["high", "low"]);
        let f = func("CORREL", &[], vec![Input::new("inReal0", ParamType::Real), Input::new("inReal1", ParamType::Real)], vec![out("outA", ParamType::Real, &["line"]), out("outB", ParamType::Real, &["line"])]);
        let chart = render(&f, &hl);
        assert!(chart.sentence.starts_with("Drawn on the price pane over candles;"), "{}", chart.sentence);
        assert!(chart.svg.contains("class=\"up wick\"") || chart.svg.contains("class=\"down wick\""), "{}", chart.svg);
        let c = RefData::parse(&text.replace("inReal=ramp[-1,1] inPriceV=V", "inReal=close").replace(",inReal,", ",").replace(",-1,,", ",,").replace(",1,nan,3", ",nan,3")).unwrap();
        assert_eq!(c.header_components(), vec!["close"]);
        assert!(render(&f, &c).sentence.starts_with("Drawn on the price pane over the close as a line;"));
        assert_eq!(d.synthetic_inputs(), vec![("inReal", "ramp[-1,1]")]);
        assert_eq!(base_series(&d, &d.synthetic_inputs()), vec![("inReal", vec![Some(-1.0), Some(1.0)])]);
        assert!(!d.all_inputs_synthetic(), "a price input keeps the series");
        assert_eq!(d.caption_lead(), "IBM, daily, 1999");
        let f = func("X", &[], vec![Input::new("inReal", ParamType::Real)], vec![out("outA", ParamType::Real, &["line"]), out("outB", ParamType::Real, &["line"])]);
        let chart = render(&f, &d);
        assert_eq!(chart.label, "X over IBM, daily, 1999");
        assert!(chart.svg.contains(">IBM, daily, 1999 \u{B7} input: inReal = ramp [-1, 1] \u{B7} volume<"), "{}", chart.svg);
        assert!(RefData::parse("# outputs: outA (shift x)\ndate,open,high,low,close,volume,outA\n").is_err());
        assert!(RefData::parse("# outputs: outA\ndate,open,high,low,close,volume,outA\n").is_err());
        assert!(RefData::parse("date,x\n").is_err());
        assert!(RefData::parse("date,open,high,low,close,volume\n1999-01-04,1,2\n").is_err());
    }

    #[test]
    fn every_reference_csv_renders() {
        let corpus = corpus();
        assert!(!corpus.is_empty(), "{}: no function CSV with a YAML", csv_dir().display());
        for (f, d) in &corpus {
            let chart = render(f, d);
            assert!(chart.svg.ends_with("</svg>"), "{}", f.name);
            assert!(chart.svg.len() < 40 * 1024, "{}: {} bytes", f.name, chart.svg.len());
            assert!(!chart.svg.contains("NaN") && !chart.svg.contains("inf"), "{}", f.name);
            assert_eq!(chart.svg.matches("<style").count(), 1, "{}", f.name);
            let view_h = chart.svg.split("viewBox=\"0 0 720 ").nth(1).unwrap().split('"').next().unwrap();
            assert_eq!(chart.height.to_string(), view_h, "{}", f.name);
            assert_eq!(chart.width, 720);
            assert!(chart.svg.contains(&format!("aria-label=\"{}\"", esc(&chart.label))), "{}", f.name);
            for o in &f.outputs {
                let needle = format!(">{}", o.name);
                assert!(chart.svg.contains(&needle), "{}: {} not in legend", f.name, o.name);
                assert!(chart.sentence.contains(&o.name), "{}: sentence lacks {}: {}", f.name, o.name, chart.sentence);
                let row = chart.flags_used.iter().filter(|r| *r == &o.name || r.starts_with(&format!("{}: ", o.name))).count();
                assert_eq!(row, 1, "{}: flags_used names {} once: {:?}", f.name, o.name, chart.flags_used);
            }
            assert!(!chart.flags_used.is_empty(), "{}", f.name);
            assert!(chart.flags_used.iter().all(|r| !r.contains("independent")), "{}: {:?}", f.name, chart.flags_used);
            for (k, (name, _)) in f.inputs.iter().filter_map(|i| i.domain.map(|d| (i.name.as_str(), d))).enumerate() {
                assert!(chart.flags_used.iter().any(|r| r.starts_with(&format!("{name}: series "))), "{}: {:?}", f.name, chart.flags_used);
                assert!(chart.svg.contains(&format!(">{name}</text>")), "{}: input {k} not in the legend", f.name);
            }
            assert!(chart.sentence.ends_with('.') && chart.sentence.matches(". ").count() == 0, "{}: {}", f.name, chart.sentence);
            assert!(!chart.sentence.contains('[') && !chart.sentence.contains("->"), "{}: {}", f.name, chart.sentence);
            for o in &f.outputs {
                if o.flags.iter().any(|fl| fl == "pattern_bull_bear") {
                    assert_eq!(chart.sentence.contains('\u{25B2}'), has(&o.flags, "positive"), "{}: {}", f.name, chart.sentence);
                    assert_eq!(chart.sentence.contains('\u{25BC}'), has(&o.flags, "negative"), "{}: {}", f.name, chart.sentence);
                }
            }
        }
    }

    /// Writes every chart next to the prototype's CSV directory for a visual diff.
    #[test]
    #[ignore = "writes files next to the prototype CSVs for a visual diff"]
    fn write_all_svgs() {
        let Some(out_dir) = svg_out_dir() else { return };
        std::fs::create_dir_all(&out_dir).unwrap();
        let mut sizes = Vec::new();
        for (f, d) in &corpus() {
            let chart = render(f, d);
            let name = f.name.to_lowercase();
            std::fs::write(out_dir.join(format!("{name}.svg")), &chart.svg).unwrap();
            sizes.push(format!("{name}.svg {} bytes | {}", chart.svg.len(), chart.sentence));
        }
        std::fs::write(out_dir.join("sizes.txt"), sizes.join("\n") + "\n").unwrap();
    }
}
