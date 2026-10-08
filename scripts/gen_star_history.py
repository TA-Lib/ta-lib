#!/usr/bin/env python3
"""Redraw the star-history figure at the bottom of the top-level README.md.

    GH_TOKEN=$(gh auth token) python3 scripts/gen_star_history.py

Reads the repository's stargazers from the GitHub API and rewrites the SVG.
Without a token the API allows 60 requests an hour, one per hundred stars.

The x axis ends at the newest star, not at today, so a run with no new star
rewrites the same bytes.
"""
import json, os, sys, urllib.request
from datetime import datetime, timezone
from pathlib import Path

REPO = 'TA-Lib/ta-lib'
SINCE = 2022
OUT = Path(__file__).resolve().parent.parent / 'website/src/.vuepress/public/assets/images/star-history.svg'

W, H = 800, 400
L, R, T, B = 62, 772, 56, 356


def fetch():
    token = os.environ.get('GH_TOKEN') or os.environ.get('GITHUB_TOKEN')
    stamps, page = [], 1
    while True:
        req = urllib.request.Request(
            f'https://api.github.com/repos/{REPO}/stargazers?per_page=100&page={page}',
            headers={'Accept': 'application/vnd.github.star+json', 'User-Agent': 'ta-lib-star-history'})
        if token:
            req.add_header('Authorization', f'Bearer {token}')
        with urllib.request.urlopen(req, timeout=60) as resp:
            rows = json.load(resp)
        if not rows:
            return sorted(stamps)
        stamps += [datetime.strptime(r['starred_at'], '%Y-%m-%dT%H:%M:%SZ')
                   .replace(tzinfo=timezone.utc).timestamp() for r in rows]
        page += 1


def render(stamps):
    total = len(stamps)
    t0 = datetime(SINCE, 1, 1, tzinfo=timezone.utc).timestamp()
    t1 = stamps[-1]
    before = sum(1 for t in stamps if t < t0)

    step = next(s for s in (10, 20, 50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000, 50000)
                if total / s <= 5)
    ymax = (total // step + 1) * step

    def x(t):
        return L + (t - t0) / (t1 - t0) * (R - L)

    def y(v):
        return B - v / ymax * (B - T)

    # At most one point per tenth of a pixel: the last count seen there.
    pts = [(float(L), y(before))]
    for count, t in enumerate(stamps[before:], before + 1):
        px = round(x(t), 1)
        if pts[-1][0] == px:
            pts[-1] = (px, y(count))
        else:
            pts.append((px, y(count)))
    line = ' '.join(f"{'L' if i else 'M'}{px:.1f} {py:.1f}" for i, (px, py) in enumerate(pts))

    label = f'{REPO} GitHub stars since {SINCE}: {total:,}'
    out = [
        f'<svg xmlns="http://www.w3.org/2000/svg" width="{W}" height="{H}" viewBox="0 0 {W} {H}" role="img" aria-label="{label}">',
        f'<title>{label}</title>',
        '<style>',
        '.bg{fill:#ffffff;stroke:#d0d7de}.ink{fill:#1f2328}.mute{fill:#59636e}.grid{stroke:#e6e9ed}.axis{stroke:#8c959f}',
        '.line{stroke:#0969da}.area{fill:#0969da;fill-opacity:.12}.dot{fill:#0969da;stroke:#ffffff}',
        '@media (prefers-color-scheme:dark){.bg{fill:#0d1117;stroke:#30363d}.ink{fill:#e6edf3}.mute{fill:#9198a1}',
        '.grid{stroke:#21262d}.axis{stroke:#484f58}.line{stroke:#4493f8}.area{fill:#4493f8;fill-opacity:.16}.dot{fill:#4493f8;stroke:#0d1117}}',
        'text{font-family:system-ui,-apple-system,Segoe UI,Helvetica,Arial,sans-serif}',
        '</style>',
        f'<rect class="bg" x=".5" y=".5" width="{W - 1}" height="{H - 1}" rx="12"/>',
        f'<text class="ink" x="{L}" y="32" font-size="18" font-weight="600">{REPO}</text>',
        f'<text class="mute" x="{R}" y="32" font-size="14" text-anchor="end">GitHub stars</text>',
    ]
    for v in range(0, ymax + 1, step):
        out.append(f'<line class="{"grid" if v else "axis"}" x1="{L}" y1="{y(v):.1f}" x2="{R}" y2="{y(v):.1f}"/>')
        out.append(f'<text class="mute" x="{L - 8}" y="{y(v) + 4:.1f}" font-size="12" text-anchor="end">{v:,}</text>')
    for year in range(SINCE, datetime.fromtimestamp(t1, timezone.utc).year + 1):
        px = x(datetime(year, 1, 1, tzinfo=timezone.utc).timestamp())
        out.append(f'<line class="axis" x1="{px:.1f}" y1="{B}" x2="{px:.1f}" y2="{B + 5}"/>')
        if px < R - 20:
            out.append(f'<text class="mute" x="{px:.1f}" y="{B + 20}" font-size="12" text-anchor="middle">{year}</text>')
    out += [
        f'<path class="area" d="{line} L{R} {B} L{L} {B} Z"/>',
        f'<path class="line" d="{line}" fill="none" stroke-width="2" stroke-linejoin="round" stroke-linecap="round"/>',
        f'<circle class="dot" cx="{R}" cy="{y(total):.1f}" r="4" stroke-width="2"/>',
        f'<text class="ink" x="{R - 10}" y="{y(total) - 8:.1f}" font-size="14" font-weight="600" text-anchor="end">{total:,}</text>',
        '</svg>',
    ]
    return '\n'.join(out) + '\n'


def main():
    stamps = fetch()
    if not stamps:
        sys.exit('no stargazers returned')
    OUT.write_text(render(stamps))
    print(f'{OUT}: {len(stamps):,} stars')


if __name__ == '__main__':
    main()
