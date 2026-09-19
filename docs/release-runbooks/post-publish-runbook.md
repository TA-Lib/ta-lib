# Post-Publish Runbook

Closing a release round: open the next development version so `dev` and `main`
stop advertising an already-released one.

**Every other runbook in this directory must be completed first**: the C publish,
and each language publish shipping this version. This one runs last, and the
reason it is separate rather than the C runbook's last section is below.

## Why this is not the C runbook's last section

`VERSION` is one number for every backend, so bumping it retires the version the
bindings are still publishing from. A binding built after the bump carries the
next version number, and every registry it publishes to is immutable.

This is not hypothetical. `79a013707` had to set `VERSION` back to 0.8.1 by hand
so that ta-lib-python could finish its own 0.8.1 release against `dev`.

**So do not start here until every backend shipping this version is public**: the
C release, and each language publish in `README.md`'s order.

## Steps

(PP1) On dev, bump the VERSION file to the next patch (e.g. `0.7.2` -> `0.7.3`). The exact number can be adjusted later; it only has to be higher.

(PP2) Add a `## [0.7.3] Not Released Yet` entry at the top of CHANGELOG.md.

(PP3) Run `./scripts/sync.py`. Besides the version, it points the website install page at the release just published and records that release in `ABI.released` (needs gcc and network). Commit, push dev, then `./scripts/merge.py`; the push to main deploys the website. Confirm with:

```bash
./scripts/sync-website.py --check   # non-zero if the page is behind, or the release could not be looked up
```
