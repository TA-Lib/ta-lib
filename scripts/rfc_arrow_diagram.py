#!/usr/bin/env python3

# Regenerates docs/RFC-Arrow-API-terms.svg, the terms diagram of docs/RFC-Arrow-API.md.
# The outputs are computed from the inputs below, so an edit to the data redraws them;
# keep the notes, segment brackets and footer text in step with the data by hand.

import sys
from pathlib import Path

rows = 13
X_ = "X"
# value, validity bit; "X" = don't care
high  = [(v,1) for v in [101.2,102.0,101.6,102.4,103.1,103.8, 0.0, 251.0,254.3,252.9,255.6,257.2,256.4]]
low   = [(v,1) for v in [ 99.8,100.6,100.1, 0.0, 101.5,101.0, 247.3, 246.2,249.8, 0.0, 250.1,252.7,251.9]]
low[3] = (X_, 0)            # null by its validity bit: value is don't care
high[6] = ("NaN", X_)       # leading NaN row of TSLA
low[9] = ("NaN", 1)         # NaN with its bit set: still null; its previous output is NaN
high[11] = (X_, 0)          # a second null, by bit, right after an output
cols = [("inHigh", high, [(0,4),(4,13)]), ("inLow", low, [(0,8),(8,13)])]
def is_null(c): return c[1] == 0 or c[0] == "NaN"
null_rows = [r for r in range(rows) if any(is_null(c[1][r]) for c in cols)]
P = 3   # MIDPRICE(3): lookback 2
out = []
for r0, r1 in ((0,6),(6,13)):
    win, prev = [], None
    for r in range(r0, r1):
        if r in null_rows:
            out.append(("NaN", 0)); continue   # keep-null
        win.append((high[r][0], low[r][0]))
        win = win[-P:]
        prev = ((max(h for h, _ in win) + min(l for _, l in win)) / 2, 1) if len(win) == P else None
        out.append(prev if prev else ("NaN", 0))
out2, filled = [], []
for r0, r1 in ((0,6),(6,13)):
    last = None
    for r in range(r0, r1):
        if r in null_rows and last is not None and last[1] == 1:
            out2.append(last); filled.append(r)
        else:
            out2.append(out[r])
        last = out2[-1]
X0, Y0, CW, CH, VW, GAP = 132, 128, 72, 28, 18, 12
xs = [X0 + i*(CW+VW+GAP) for i in range(len(cols))]
in_right = xs[-1] + CW + VW
OX = in_right + 70; OCW = 80; out_right = OX + OCW + VW
NOTES_END = OX + OCW + VW + 16 + 14 + 130
OX2 = NOTES_END + 50; out2_right = OX2 + OCW + VW
W, H = out2_right + 24, Y0 + rows*CH + 34 + 122 + 14
ink, muted, line = "#1f2933", "#5f6b76", "#9aa5b1"
part = {"IBM": "#dbe8f6", "TSLA": "#f6f1c7"}
null_bg = "#fad7c3"
seg_c, chunk_c, null_c, ok_c, dc_c = "#2f7d4f", "#24425f", "#b4462f", "#2f7d4f", "#b4462f"
fill_c = "#1f6fb2"
warm_c = "#7a4fa3"
o = []; a = o.append
a(f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {W} {H}" width="{W}" height="{H}" font-family="Helvetica, Arial, sans-serif">')
a(f'<rect width="{W}" height="{H}" fill="#ffffff"/>')
a(f'<text x="{W/2}" y="34" font-size="19" font-weight="bold" fill="{ink}" text-anchor="middle">Example: MIDPRICE(3)</text>')
a('<defs><pattern id="hatch" width="7" height="7" patternUnits="userSpaceOnUse" patternTransform="rotate(45)">'
  f'<line x1="0" y1="0" x2="0" y2="7" stroke="{null_c}" stroke-width="1.2" opacity="0.4"/></pattern>'
  f'<marker id="arr" viewBox="0 0 10 10" refX="9" refY="5" markerWidth="7" markerHeight="7" orient="auto"><path d="M0 0 L10 5 L0 10 z" fill="{muted}"/></marker>'
  f'<marker id="arrf" viewBox="0 0 10 10" refX="7" refY="5" markerWidth="3.6" markerHeight="3.6" orient="auto"><path d="M0 0 L10 5 L0 10 z" fill="{fill_c}"/></marker></defs>')
for name, r0, r1 in (("IBM",0,6),("TSLA",6,13)):
    y0, y1 = Y0 + r0*CH, Y0 + r1*CH
    a(f'<rect x="{X0-56}" y="{y0}" width="{out_right-X0+56+8}" height="{y1-y0}" fill="{part[name]}"/>')
    a(f'<text x="18" y="{(y0+y1)/2-6}" font-size="13" font-weight="bold" fill="{ink}">partition</text>')
    a(f'<text x="18" y="{(y0+y1)/2+11}" font-size="13" fill="{ink}">{name}</text>')
a(f'<line x1="{X0-56}" y1="{Y0+6*CH}" x2="{out_right+8}" y2="{Y0+6*CH}" stroke="{ink}" stroke-width="2"/>')
for name, r0, r1 in (("IBM",0,6),("TSLA",6,13)):
    a(f'<rect x="{OX2-30}" y="{Y0+r0*CH}" width="{out2_right-OX2+38}" height="{(r1-r0)*CH}" fill="{part[name]}"/>')
a(f'<line x1="{OX2-30}" y1="{Y0+6*CH}" x2="{out2_right+8}" y2="{Y0+6*CH}" stroke="{ink}" stroke-width="2"/>')
a(f'<text x="{out2_right+4}" y="{Y0-58}" font-size="13" fill="{muted}" text-anchor="end">Output (<tspan font-weight="bold" fill="{fill_c}">fill-previous option</tspan>)</text>')
a(f'<line x1="{NOTES_END}" y1="{Y0-27}" x2="{OX2-12}" y2="{Y0-27}" stroke="{muted}" stroke-width="1.5" marker-end="url(#arr)"/>')
a(f'<text x="{(X0+in_right)/2}" y="{Y0-58}" font-size="13" fill="{muted}" text-anchor="middle">Multi-input: each column with a validity bitmap</text>')
a(f'<text x="{OX-12}" y="{Y0-58}" font-size="13" fill="{muted}" text-anchor="start">Output (default option)</text>')
a(f'<line x1="{in_right+14}" y1="{Y0-27}" x2="{OX-12}" y2="{Y0-27}" stroke="{muted}" stroke-width="1.5" marker-end="url(#arr)"/>')
def column(x, w, name, cells, chunks):
    a(f'<text x="{x+w/2}" y="{Y0-22}" font-size="15" font-weight="bold" fill="{ink}" text-anchor="middle">{name}</text>')
    a(f'<text x="{x+w+VW/2}" y="{Y0-8}" font-size="9" fill="{muted}" text-anchor="middle">validity</text>')
    for r, (v, b) in enumerate(cells):
        y = Y0 + r*CH
        null = b == 0 or v == "NaN"
        a(f'<rect x="{x}" y="{y}" width="{w}" height="{CH}" fill="{null_bg if null else "#ffffff"}" stroke="{line}" stroke-width="0.8"/>')
        if null:
            a(f'<rect x="{x}" y="{y}" width="{w}" height="{CH}" fill="url(#hatch)"/>')
        if v == X_:   txt, col, wt = "X", dc_c, "bold"
        elif v == "NaN": txt, col, wt = "NaN", null_c, "bold"
        else:         txt, col, wt = (f"{v:.1f}" if name != "outReal" else f"{v:.2f}"), ink, "normal"
        a(f'<text x="{x+w/2}" y="{y+CH/2+6}" font-size="12" font-weight="{wt}" fill="{col}" text-anchor="middle">{txt}</text>')
        bf = "#eef1f4" if b == X_ else ("#e3f1e8" if b else "#f8e1dc")
        bc = dc_c if b == X_ else (ok_c if b else null_c)
        a(f'<rect x="{x+w}" y="{y}" width="{VW}" height="{CH}" fill="{bf}" stroke="{line}" stroke-width="0.8"/>')
        a(f'<text x="{x+w+VW/2}" y="{y+CH/2+6}" font-size="11" font-weight="bold" fill="{bc}" text-anchor="middle">{b}</text>')
    for k, (r0, r1) in enumerate(chunks):
        y0, y1 = Y0 + r0*CH, Y0 + r1*CH
        a(f'<rect x="{x+1.5}" y="{y0+1.5}" width="{w+VW-3}" height="{y1-y0-3}" fill="none" stroke="{chunk_c}" stroke-width="2.5" rx="4"/>')
        a(f'<text x="{x+w-5}" y="{y0+8}" font-size="8" fill="{chunk_c}" text-anchor="end">chunk {k+1}</text>')
for r in range(rows):
    a(f'<text x="{X0-14}" y="{Y0+r*CH+CH/2+6}" font-size="11" fill="{muted}" text-anchor="end">{r}</text>')
a(f'<text x="{X0-14}" y="{Y0-22}" font-size="11" fill="{muted}" text-anchor="end">row</text>')
for r in null_rows:
    a(f'<rect x="{X0-32}" y="{Y0+r*CH+1}" width="{in_right-X0+36}" height="{CH-2}" fill="none" stroke="{null_c}" stroke-width="1.5" stroke-dasharray="4 3" rx="3"/>')
for x, (name, cells, chunks) in zip(xs, cols):
    column(x, CW, name, cells, chunks)
column(OX, OCW, "outReal", out, [(0,13)])
column(OX2, OCW, "outReal", out2, [(0,13)])
for r in filled:
    y0, y1, x = Y0 + (r-1)*CH + CH/2, Y0 + r*CH + CH/2, OX2 + 12
    a(f'<line x1="{x}" y1="{y0}" x2="{x}" y2="{y1}" stroke="{fill_c}" stroke-width="2.6" marker-end="url(#arrf)"/>')
# segments: between inputs and output
bx = in_right + 14
segs = [(0,3),(4,6),(7,9),(10,11),(12,13)]
for k, (r0, r1) in enumerate(segs):
    y0, y1 = Y0 + r0*CH + 4, Y0 + r1*CH - 4
    a(f'<path d="M{bx} {y0} h7 V{y1} h-7" fill="none" stroke="{seg_c}" stroke-width="2"/>')
    a(f'<text x="{bx+12}" y="{(y0+y1)/2+4}" font-size="11" font-weight="bold" fill="{seg_c}">s{k+1}</text>')
# output notes
nx = out_right + 16
def note(y, label, txt, col):
    a(f'<text x="{nx+14}" y="{y}" font-size="11" fill="{col}">{txt}</text>')
def bracket(r0, r1, label, txt, col):
    y0, y1 = Y0 + r0*CH + 4, Y0 + r1*CH - 4
    m, w, r = (y0 + y1) / 2, 5, 5
    a(f'<path d="M{nx} {y0} q{w} 0 {w} {r} V{m-r} q0 {r} {w} {r} q-{w} 0 -{w} {r} V{y1-r} q0 {r} -{w} {r}" fill="none" stroke="{col}" stroke-width="1.8"/>')
    note((y0+y1)/2+4, label, txt, col)
rowy = lambda r: Y0 + r*CH + CH/2 + 6
bracket(0, 2, "Rows 0-1:", "warm-up (lookback 2)", warm_c)
note(rowy(3), "Row 3:", "skip and keep-null", null_c)
note(rowy(4), "Row 4:", "uses rows 1, 2 and 4", muted)
note(rowy(6), "Row 6:", "Leading NaN", null_c)
bracket(7, 9, "Rows 7-8:", "warm-up (lookback 2)", warm_c)
note(rowy(9), "Row 9:", "skip and keep-null", null_c)
note(rowy(10), "Row 10:", "uses rows 7, 8 and 10", muted)
note(rowy(11), "Row 11:", "skip and keep-null", null_c)
note(rowy(12), "Row 12:", "uses rows 8, 10 and 12", muted)
ly = Y0 + rows*CH + 34
items = [("Arrow chunk", f'<rect x="0" y="-11" width="22" height="14" fill="none" stroke="{chunk_c}" stroke-width="2.5" rx="3"/>'),
         ("null", f'<rect x="0" y="-11" width="22" height="14" fill="{null_bg}" stroke="{line}"/><rect x="0" y="-11" width="22" height="14" fill="url(#hatch)"/>'),
         ("don\'t care", f'<text x="11" y="0" font-size="12" font-weight="bold" fill="{dc_c}" text-anchor="middle">X</text>'),
         ("s1-s5: segments; the state carries across them, per partition", f'<path d="M2 -11 h6 V3 h-6" fill="none" stroke="{seg_c}" stroke-width="2"/>'),]
lx = 18
for label, sym in items:
    a(f'<g transform="translate({lx},{ly})">{sym}<text x="30" y="0" font-size="12" fill="{ink}">{label}</text></g>')
    lx += 30 + len(label)*6.6 + 24
a(f'<text x="18" y="{ly+32}" font-size="12" fill="{muted}">MIDPRICE(3): (highest inHigh + lowest inLow) / 2 over the last 3 non-null rows.</text>')
a(f'<text x="18" y="{ly+50}" font-size="12" fill="{muted}"><tspan font-weight="bold">Row 3:</tspan> if any validity bit is 0, the row is null.</text>')
a(f'<text x="18" y="{ly+68}" font-size="12" fill="{muted}"><tspan font-weight="bold">Row 6:</tspan> if any value is NaN, the row is null.</text>')
a(f'<text x="18" y="{ly+86}" font-size="12" fill="{muted}"><tspan font-weight="bold">Row 9:</tspan> NaN with validity bit 1 is still null.</text>')
a(f'<text x="18" y="{ly+104}" font-size="12" fill="{muted}">Chunks may cross partitions; segments may cross chunks.</text>')
a(f'<text x="18" y="{ly+122}" font-size="12" fill="{muted}">Inputs chunked alike may share one validity bitmap.</text>')
a('</svg>')
dest = sys.argv[1] if len(sys.argv) > 1 else Path(__file__).resolve().parent.parent / "docs" / "RFC-Arrow-API-terms.svg"
open(dest, "w").write("\n".join(o))
