//! One SVG chart per function for its ta-lib.org page, drawn from the
//! function's metadata and a reference run over a fixed daily series.
//!
//! The metadata decides the drawing: `overlap` puts real outputs on the price
//! pane, otherwise they get an indicator pane; an integer output that is not a
//! pattern gets a step pane of its own; `pattern_*` outputs become markers on
//! the candles; the inputs' price components pick candles or a close line and
//! add a volume pane. Output flags pick the stroke (`line`, `dash_line`, `dot`,
//! `histogram`, `upper_limit`/`lower_limit` band), and `display_shift` moves the
//! series by the shift the reference run reports.
//!
//! The SVG carries class names only; the site stylesheet styles them.

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
const SERIES_CAPTION: &str = "IBM, daily, 1999";

/// The chart's whole look, shipped inside every SVG: the `var(--vp-c-*, fallback)` hooks
/// take the site's theme when the component inlines the file, and the fallbacks make the
/// bare file render on its own (noscript image, direct link, a crawler).
pub const STYLE: &str = ".ta-chart{font-family:ui-sans-serif,system-ui,sans-serif;font-size:11px;--ta-text:var(--vp-c-text,#3c3c43);--ta-mute:var(--vp-c-text-mute,#6b6b74);--ta-grid:var(--vp-c-divider,#e2e2e3);--ta-up:#1f9d55;--ta-down:#d6454a;--ta-s1:#2a78d6;--ta-s2:#eb6834;--ta-s3:#1baf7a;--ta-s4:#6250d6;--ta-s5:#eda100;--ta-vol:var(--vp-c-border,#c2c2c4)}[data-theme=\"dark\"] .ta-chart{--ta-up:#3cc47a;--ta-down:#ef6b6b;--ta-s1:#3987e5;--ta-s2:#e2693a;--ta-s3:#199e70;--ta-s4:#9085e9;--ta-s5:#c98500}.ta-chart text{fill:var(--ta-mute)}.ta-chart .grid{stroke:var(--ta-grid);stroke-width:1}.ta-chart .zero{stroke:var(--ta-mute);stroke-width:1;stroke-dasharray:3 3}.ta-chart .close{stroke:var(--ta-text);stroke-width:1.3;fill:none;opacity:.75}.ta-chart .up{stroke:var(--ta-up);fill:var(--ta-up)}.ta-chart .down{stroke:var(--ta-down);fill:var(--ta-down)}.ta-chart .wick{stroke-width:.8}.ta-chart .line{fill:none;stroke-width:1.6;stroke-linejoin:round}.ta-chart .dash{stroke-dasharray:5 3}.ta-chart .edge{fill:none;stroke-width:1.1}.ta-chart .fill{opacity:.1}.ta-chart .s1{stroke:var(--ta-s1)}.ta-chart .s2{stroke:var(--ta-s2)}.ta-chart .s3{stroke:var(--ta-s3)}.ta-chart .s4{stroke:var(--ta-s4)}.ta-chart .s5{stroke:var(--ta-s5)}.ta-chart .f1{fill:var(--ta-s1)}.ta-chart .f2{fill:var(--ta-s2)}.ta-chart .f3{fill:var(--ta-s3)}.ta-chart .f4{fill:var(--ta-s4)}.ta-chart .f5{fill:var(--ta-s5)}.ta-chart .vol{fill:var(--ta-vol)}.ta-chart .hollow{fill:none;stroke-width:1.2}.ta-chart .legend{fill:var(--ta-text);font-family:var(--vp-font-mono,ui-monospace,monospace);font-size:10.5px}.ta-chart .mono{font-family:var(--vp-font-mono,ui-monospace,monospace)}";


/// The reference run's CSV, as `ta_regtest`'s refdata writer lays it out: `#`
/// header lines, then a header row `date,open,high,low,close,volume,<outputs>`
/// and one row per bar where an output cell is empty before its first value.
#[derive(Debug, Clone, Default)]
pub struct RefData {
    pub dates: Vec<String>,
    pub open: Vec<f64>,
    pub high: Vec<f64>,
    pub low: Vec<f64>,
    pub close: Vec<f64>,
    pub volume: Vec<f64>,
    pub outputs: Vec<(String, Vec<Option<f64>>)>,
    pub shifts: HashMap<String, i32>,
    /// `# inputs:` items, `(argument, source)`: `("inReal", "close")`,
    /// `("inPriceHLC", "HLC")`, `("inReal", "unit(close)")`.
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
                data.outputs = hdr[6..].iter().map(|n| (n.clone(), Vec::new())).collect();
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
            for (k, (_, col)) in data.outputs.iter_mut().enumerate() {
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
        Ok(data)
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

    fn shift(&self, name: &str) -> i32 {
        self.shifts.get(name).copied().unwrap_or(0)
    }

    fn series_caption(&self) -> &str {
        self.series.as_deref().unwrap_or(SERIES_CAPTION)
    }

    /// Component letters the header names (`inPriceHLC=HLC`), lower-cased tokens.
    fn header_components(&self) -> Vec<&'static str> {
        let mut comps = Vec::new();
        for (_, v) in &self.input_desc {
            if !v.is_empty() && v.chars().all(|c| "OHLCV".contains(c)) {
                for c in v.chars() {
                    let name = match c {
                        'O' => "open",
                        'H' => "high",
                        'L' => "low",
                        'C' => "close",
                        _ => "volume",
                    };
                    if !comps.contains(&name) {
                        comps.push(name);
                    }
                }
            }
        }
        comps
    }

    /// `inReal=close->[lo,hi]`: the function read the close mapped affinely into
    /// `[lo, hi]`, and the chart plots that series as its input.
    fn mapped_input(&self) -> Option<(&str, (f64, f64))> {
        self.input_desc.iter().find_map(|(_, v)| {
            let range = v.strip_prefix("close->[")?.strip_suffix(']')?;
            let (lo, hi) = range.split_once(',')?;
            Some((v.as_str(), (lo.trim().parse().ok()?, hi.trim().parse().ok()?)))
        })
    }
}

#[derive(Debug, Clone)]
pub struct Chart {
    pub svg: String,
    /// One plain-English sentence stating how the chart is drawn.
    pub sentence: String,
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
    };
    for o in &func.outputs {
        if o.flags.iter().any(|f| f.starts_with("pattern_")) {
            p.pattern_outs.push(o);
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
    intp: Option<Pane>,
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
    let intp = (!plan.int_outs.is_empty())
        .then(|| take(if plan.ind_outs.is_empty() { H_IND } else { H_INT_SMALL }));
    let vol = plan.has_vol.then(|| take(H_VOL));
    Layout {
        price,
        ind,
        intp,
        vol,
        total_h: cursor - GAP + H_AXIS + H_CAPTION,
    }
}

/// The series the top pane shows as the input: the close, or its mapped form
/// when the reference run fed the function a transformed close.
fn base_series(data: &RefData, mapped: Option<(&str, (f64, f64))>) -> Vec<Option<f64>> {
    match mapped {
        Some((_, (lo, hi))) => {
            let mn = data.close.iter().copied().fold(f64::INFINITY, f64::min);
            let mx = data.close.iter().copied().fold(f64::NEG_INFINITY, f64::max);
            let span = if mx > mn { mx - mn } else { 1.0 };
            data.close
                .iter()
                .map(|v| Some(lo + (v - mn) / span * (hi - lo)))
                .collect()
        }
        None => data.close.iter().map(|v| Some(*v)).collect(),
    }
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
        let point = |i: usize, v: f64| format!("{},{}", fmt(cv.x_of(i)), fmt(price.sy(v)));
        let mut pts: Vec<String> =
            uv.iter().enumerate().filter_map(|(i, v)| v.map(|v| point(i, v))).collect();
        let n_upper = pts.len();
        pts.extend(lv.iter().enumerate().rev().filter_map(|(i, v)| v.map(|v| point(i, v))));
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
            let Some(x) = cv.bar_x(i, sh) else { continue };
            let hollow = v.abs() == 80.0;
            let ring = v.abs() == 200.0;
            saw_weak |= hollow;
            saw_confirm |= ring;
            let bar_high = data.high.get(i).copied().unwrap_or(0.0);
            let bar_low = data.low.get(i).copied().unwrap_or(0.0);
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
            text: if none {
                format!("{}: no occurrence in the reference data", o.name)
            } else {
                format!("{}: {} flagged", o.name, plural(hits, "bar"))
            },
        });
        let mut ph = if is_bool {
            format!("{} as a diamond above the high of every bar it flags", o.name)
        } else {
            format!(
                "{} as markers on the candles, \u{25B2} below the low for a bullish call and \u{25BC} above the high for a bearish one",
                o.name
            )
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

fn draw_step_pane(cv: &mut Canvas<'_>, plan: &Plan<'_>, intp: &mut Pane) -> String {
    let data = cv.data;
    for o in &plan.int_outs {
        intp.extend(data.output(&o.name));
    }
    intp.finish(0.15, true);
    cv.grid(intp, false);
    let mut legend: Vec<LegendItem<'_>> = Vec::new();
    for (k, o) in plan.int_outs.iter().enumerate() {
        let cls = SLOTS[k % SLOTS.len()];
        cv.step_line(intp, data.output(&o.name), cls, data.shift(&o.name));
        legend.push(LegendItem { mark: LegendMark::Line { cls, dashed: false }, text: o.name.clone() });
    }
    cv.legend(intp, &legend);
    let names: Vec<String> = plan.int_outs.iter().map(|o| o.name.clone()).collect();
    format!(
        "a step pane of its own for the integer {} {}",
        if names.len() == 1 { "output" } else { "outputs" },
        join_and(&names)
    )
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
    let mut prev: Option<usize> = None;
    for (i, d) in cv.data.dates.iter().enumerate() {
        let mo = d
            .get(5..7)
            .and_then(|m| m.parse::<usize>().ok())
            .filter(|m| (1..=12).contains(m));
        let Some(mo) = mo else { continue };
        if prev != Some(mo) {
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
                MONTHS[mo - 1]
            );
            prev = Some(mo);
        }
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
        .map(|(k, v)| component_names(v).unwrap_or_else(|| format!("{k} = {v}")))
        .collect();
    let mut cap = format!("{} \u{B7} input: {}", cv.data.series_caption(), ins.join(" \u{B7} "));
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
    let Layout { mut price, mut ind, mut intp, mut vol, total_h } = layout(&plan);

    let mapped = if plan.candles { None } else { data.mapped_input() };
    let base = base_series(data, mapped);
    if plan.candles {
        price.extend_plain(&data.high);
        price.extend_plain(&data.low);
    } else {
        price.extend(&base);
    }
    for o in &plan.price_outs {
        price.extend(data.output(&o.name));
    }
    price.finish(0.05, false);

    let mut cv = Canvas::new(data, bars, max_shift);
    let _ = writeln!(
        cv.out,
        "<svg class=\"ta-chart\" xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 {} {}\" role=\"img\" aria-label=\"{}\">",
        fmt(W),
        fmt(total_h),
        esc(&format!("{} over {}", func.name, data.series_caption()))
    );
    cv.out.push_str(&format!("<style>{STYLE}</style>"));

    cv.grid(&price, false);
    let mut sentence = if plan.candles {
        cv.candles(&price);
        "Drawn on the price pane over candles".to_string()
    } else {
        cv.polyline(&price, &base, "close", 0);
        match mapped {
            Some((label, _)) => format!("Drawn on the price pane over the input, {label}, as a line"),
            None => "Drawn on the price pane over the close as a line".to_string(),
        }
    };
    let mut legend: Vec<LegendItem<'_>> = Vec::new();
    let price_phrases = draw_price_outputs(&mut cv, &plan, &price, &mut legend);
    let pattern_phrases = draw_pattern_markers(&mut cv, &plan, &price, &mut legend);
    cv.legend(&price, &legend);

    let mut below: Vec<String> = Vec::new();
    if let Some(ind) = ind.as_mut() {
        below.push(draw_indicator_pane(&mut cv, &plan, ind));
    }
    if let Some(intp) = intp.as_mut() {
        below.push(draw_step_pane(&mut cv, &plan, intp));
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
        sentence,
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
    fn csv_dir() -> Option<PathBuf> {
        let dir = std::env::var_os("TA_CHART_PROTO_CSV").map_or_else(
            || Path::new(env!("CARGO_MANIFEST_DIR")).join("../../website/src/.vuepress/public/functions"),
            PathBuf::from,
        );
        dir.is_dir().then_some(dir)
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
        let Some(csv_dir) = csv_dir() else { return Vec::new() };
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

    #[test]
    fn ref_data_header_is_parsed_once() {
        let text = "# TA-Lib reference data: X\n# series: IBM, daily, 1999\n# input: ibm_1999.csv; window 1999-01-04..1999-01-05 (2 bars); flagged bars: 1\n# parameters: (defaults)\n# inputs: inReal=close->[-1,1] inPriceV=V\n# outputs: outA (shift 8) outB (shift 0)\n# lookback: 0\ndate,open,high,low,close,volume,outA,outB\n1999-01-04,1,2,0.5,1.5,100,,\n1999-01-05,1,2,0.5,1.6,100,nan,3\n";
        let d = RefData::parse(text).unwrap();
        assert_eq!(d.shift("outA"), 8);
        assert_eq!(d.shift("outB"), 0);
        assert_eq!(d.input_desc, vec![("inReal".to_string(), "close->[-1,1]".to_string()), ("inPriceV".to_string(), "V".to_string())]);
        assert!(d.parameters.is_empty());
        assert_eq!(d.series.as_deref(), Some("IBM, daily, 1999"));
        assert_eq!(d.flagged, Some(1));
        assert_eq!(d.output("outA"), &[None, None]);
        assert_eq!(d.output("outB"), &[None, Some(3.0)]);
        assert_eq!(d.header_components(), vec!["volume"]);
        assert_eq!(d.mapped_input(), Some(("close->[-1,1]", (-1.0, 1.0))));
        assert_eq!(base_series(&d, d.mapped_input()), vec![Some(-1.0), Some(1.0)]);
        assert!(RefData::parse("# outputs: outA (shift x)\ndate,open,high,low,close,volume,outA\n").is_err());
        assert!(RefData::parse("# outputs: outA\ndate,open,high,low,close,volume,outA\n").is_err());
        assert!(RefData::parse("date,x\n").is_err());
        assert!(RefData::parse("date,open,high,low,close,volume\n1999-01-04,1,2\n").is_err());
    }

    #[test]
    fn every_reference_csv_renders() {
        let Some(dir) = csv_dir() else {
            eprintln!("no reference CSV directory: skipped");
            return;
        };
        let corpus = corpus();
        assert!(!corpus.is_empty(), "{}: no function CSV with a YAML", dir.display());
        for (f, d) in &corpus {
            let chart = render(f, d);
            assert!(chart.svg.ends_with("</svg>"), "{}", f.name);
            assert!(chart.svg.len() < 40 * 1024, "{}: {} bytes", f.name, chart.svg.len());
            assert!(!chart.svg.contains("NaN") && !chart.svg.contains("inf"), "{}", f.name);
            assert_eq!(chart.svg.matches("<style").count(), 1, "{}", f.name);
            let view_h = chart.svg.split("viewBox=\"0 0 720 ").nth(1).unwrap().split('"').next().unwrap();
            assert_eq!(chart.height.to_string(), view_h, "{}", f.name);
            assert_eq!(chart.width, 720);
            for o in &f.outputs {
                let needle = format!(">{}", o.name);
                assert!(chart.svg.contains(&needle), "{}: {} not in legend", f.name, o.name);
                assert!(chart.sentence.contains(&o.name), "{}: sentence lacks {}: {}", f.name, o.name, chart.sentence);
            }
            assert!(chart.sentence.ends_with('.') && chart.sentence.matches(". ").count() == 0, "{}: {}", f.name, chart.sentence);
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
