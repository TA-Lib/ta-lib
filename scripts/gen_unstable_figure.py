#!/usr/bin/env python3
"""Redraw the figure in website/src/api/unstable-period/README.md.

    python3 scripts/gen_unstable_figure.py --inject

The three lines are real TA_EMA output over the ta_regtest reference closes: same
period, each fed a history starting `--gap` bars later, so the only thing that
separates them is how much of the seed still shows. `--start` must land in a
splice-free stretch of that corpus -- it is several series concatenated, and a
window straddling a joint invents a fan that no continuous data produces.

The dashed rule sits where the three agree to within one part in a thousand of
price. Colours and neutrals live in website/src/.vuepress/styles/index.scss.

`--inject` rewrites the <svg> element and nothing else: the caption is the page's
own prose, and it is also the figure's accessible name (`aria-labelledby="uf-cap"`),
so the description exists once and is edited in the page.
"""
import re, sys, math, argparse
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
BASE = str(ROOT / 'src/tools/ta_regtest') + '/'
PAGE = ROOT / 'website/src/api/unstable-period/README.md'

def load(p):
    b = open(p).read().split('{', 1)[1].rsplit('}', 1)[0]
    return [float(x) for x in re.findall(r'-?\d+(?:\.\d+)?', b)]

C = load(BASE + 'ta_gDataClose.c')
H = load(BASE + 'ta_gDataHigh.c')
L = load(BASE + 'ta_gDataLow.c')

def ema(vals, period):
    """TA-Lib EMA, unstable period 0: seed = SMA of the first `period` values."""
    k = 2.0 / (period + 1)
    seed = sum(vals[:period]) / period
    out = [None] * (period - 1) + [seed]
    prev = seed
    for v in vals[period:]:
        prev = (v - prev) * k + prev
        out.append(prev)
    return out

ap = argparse.ArgumentParser()
ap.add_argument('--start', type=int, default=5350)
ap.add_argument('--period', type=int, default=20)
ap.add_argument('--gap', type=int, default=10)
ap.add_argument('--tail', type=int, default=70)
ap.add_argument('--context', choices=('none', 'close', 'bars'), default='bars')
ap.add_argument('--pad', type=float, default=0.55)
ap.add_argument('--zoom', choices=('lines','all'), default='lines')
ap.add_argument('--inject', action='store_true', help='splice into the page instead of a file')
ap.add_argument('--out', default='fig.svg')
A = ap.parse_args()

S, PERIOD, GAP = A.start, A.period, A.gap
OFFS  = (0, GAP, 2 * GAP)
first = 2 * GAP + PERIOD - 1
N     = first + A.tail

w  = C[S:S+N]; hh = H[S:S+N]; ll = L[S:S+N]
lines = [[None] * o + ema(w[o:], PERIOD) for o in OFFS]

px = sum(w) / len(w)
spread = [None] * N
for i in range(first, N):
    v = [l[i] for l in lines]
    spread[i] = max(v) - min(v)
THRESH   = px * 0.001                          # "agree" = within one part in a thousand
boundary = next(i for i in range(first, N) if all(spread[j] <= THRESH for j in range(i, N)))

# ---------------------------------------------------------------- geometry
W, HGT   = 760, 344
PX0, PX1 = 54, 736
PY0, PY1 = 16, 262
PW, PH   = PX1 - PX0, PY1 - PY0
AXIS_Y   = PY1 + 22
ARROW_Y  = PY1 + 64

emavals = [v for l in lines for v in l if v is not None]
lo, hi = min(emavals), max(emavals)
if A.zoom == 'all':
    if A.context == 'bars':
        lo, hi = min(min(ll), lo), max(max(hh), hi)
    elif A.context == 'close':
        lo, hi = min(min(w), lo), max(max(w), hi)
pad = (hi - lo) * A.pad
lo, hi = lo - pad, hi + pad

def x(i): return PX0 + i * PW / (N - 1)
def y(v): return PY1 - (v - lo) / (hi - lo) * PH
def f(v): return f'{v:.1f}'.rstrip('0').rstrip('.')
def pts(vals):
    return ' '.join(f'{f(x(i))},{f(y(v))}' for i, v in enumerate(vals) if v is not None)

span = hi - lo
base = 10 ** (math.floor(math.log10(span)) - 1)
for m in (1, 2, 2.5, 5, 10, 20, 25, 50, 100):
    step = base * m
    if span / step <= 4.2:
        break
ticks = []
t = math.ceil(lo / step) * step
while t < hi:
    ticks.append(t); t += step

o = []; a = o.append
a(f'<svg class="uf" viewBox="0 0 {W} {HGT}" role="img" aria-labelledby="uf-cap" '
  f'xmlns="http://www.w3.org/2000/svg">')

a(f'<rect class="uf-wash" x="{PX0}" y="{PY0}" width="{f(x(boundary)-PX0)}" height="{PH}"/>')
for t in ticks:
    a(f'<line class="uf-grid" x1="{PX0}" x2="{PX1}" y1="{f(y(t))}" y2="{f(y(t))}"/>')
    a(f'<text class="uf-tick uf-tick-y" x="{PX0-9}" y="{f(y(t)+3.5)}">{f(t)}</text>')

if A.context != 'none':
    a(f'<mask id="uf-fade"><linearGradient id="uf-fadeg" x1="0" y1="{PY0}" x2="0" y2="{PY1}" '
      f'gradientUnits="userSpaceOnUse">'
      '<stop offset="0" stop-color="#000"/><stop offset="0.16" stop-color="#fff"/>'
      '<stop offset="0.84" stop-color="#fff"/><stop offset="1" stop-color="#000"/>'
      f'</linearGradient><rect x="{PX0}" y="{PY0}" width="{PW}" height="{PH}" fill="url(#uf-fadeg)"/></mask>')
    a('<g mask="url(#uf-fade)">')
    if A.context == 'bars':
        a('<path class="uf-ctx" d="%s"/>' % ''.join(
            f'M{f(x(i))},{f(y(hh[i]))}V{f(y(ll[i]))}' for i in range(N)))
    else:
        a(f'<polyline class="uf-ctx" points="{pts(w)}"/>')
    a('</g>')

a(f'<line class="uf-axis" x1="{PX0}" x2="{PX1}" y1="{PY1}" y2="{PY1}"/>')
for i in range(0, N, 20):
    a(f'<line class="uf-axis" x1="{f(x(i))}" x2="{f(x(i))}" y1="{PY1}" y2="{PY1+5}"/>')
    a(f'<text class="uf-tick" x="{f(x(i))}" y="{AXIS_Y}" text-anchor="middle">{i}</text>')
a(f'<text class="uf-tick" x="{PX1}" y="{AXIS_Y}" text-anchor="end">bar</text>')

a(f'<line class="uf-boundary" x1="{f(x(boundary))}" x2="{f(x(boundary))}" y1="{PY0}" y2="{ARROW_Y+9}"/>')

for n, vals in enumerate(lines, start=1):
    a(f'<polyline class="uf-ema uf-ema{n}" points="{pts(vals)}"/>')
for n, (off, vals) in enumerate(zip(OFFS, lines), start=1):
    i0 = off + PERIOD - 1
    a(f'<circle class="uf-dot uf-dot{n}" cx="{f(x(i0))}" cy="{f(y(vals[i0]))}" r="4.5"/>')

# legend, in whichever plot corner the lines leave emptiest
corners = {}
for cx0, cy0, name in ((PX0, PY0, 'tl'), (PX1 - 150, PY0, 'tr'),
                       (PX0, PY1 - 78, 'bl'), (PX1 - 150, PY1 - 78, 'br')):
    d = min((abs(x(i) - (cx0 + 75)) / 75) ** 2 + (abs(y(v) - (cy0 + 39)) / 39) ** 2
            for l in lines for i, v in enumerate(l) if v is not None)
    corners[name] = (d, cx0, cy0)
_, lx, ly = max(corners.values())
a(f'<rect class="uf-legend-bg" x="{f(lx)}" y="{f(ly)}" width="150" height="72" rx="5"/>')
for n, off in enumerate(OFFS, start=1):
    ry = ly + 20 + (n - 1) * 21
    a(f'<line class="uf-ema uf-ema{n}" x1="{f(lx+12)}" x2="{f(lx+34)}" y1="{f(ry)}" y2="{f(ry)}"/>')
    a(f'<text class="uf-legend" x="{f(lx+42)}" y="{f(ry+4)}">fed from bar {off}</text>')

a('<defs><marker id="uf-ah" viewBox="0 0 10 10" refX="9.5" refY="5" markerWidth="7" '
  'markerHeight="7" orient="auto"><path d="M0,0.7 L10,5 L0,9.3 z"/></marker></defs>')
bx = x(boundary)
a(f'<line class="uf-arrow" x1="{f(bx-9)}" x2="{PX0+2}" y1="{ARROW_Y}" y2="{ARROW_Y}" marker-end="url(#uf-ah)"/>')
a(f'<line class="uf-arrow" x1="{f(bx+9)}" x2="{PX1-2}" y1="{ARROW_Y}" y2="{ARROW_Y}" marker-end="url(#uf-ah)"/>')
a(f'<text class="uf-zone" x="{f((PX0+bx)/2)}" y="{ARROW_Y-11}" text-anchor="middle">unstable</text>')
a(f'<text class="uf-zone" x="{f((PX1+bx)/2)}" y="{ARROW_Y-11}" text-anchor="middle">stable</text>')
a('</svg>')

svg = '\n'.join(o)
if A.inject:
    src = PAGE.read_text()
    i, j = src.index('<svg class="uf"'), src.index('</svg>') + len('</svg>')
    PAGE.write_text(src[:i] + svg + src[j:])
    where = PAGE
else:
    Path(A.out).write_text(svg)
    where = A.out
print(f'start={S} period={PERIOD} offs={OFFS} N={N} first={first} boundary={boundary} '
      f'plot {lo:.2f}-{hi:.2f} ({A.context}) -> {where}')
