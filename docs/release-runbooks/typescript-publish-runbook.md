# TypeScript Publish Runbook

Releasing the `ta-lib` package to npm.

**`c-publish-runbook.md` must be completed first.** `README.md` in this directory holds the
order.

This is a **per-release** procedure. The package version is locked to the repo `VERSION`.

---

## What ships

One package publishes: `ta_codegen/output/ts/library/`, with two entry points, `ta-lib` and
`ta-lib/metadata`. The tarball holds `package.json`, `README.md`, `LICENSE` and the compiled
`dist/`; sources and tests stay out. The tooling root, `ta_codegen/output/ts/package.json`, is
private and never publishes.

`generate` writes `library/package.json`, its version included, and copies the root `LICENSE`
beside it. `library/README.md` is hand-written and ships frozen inside each version.

## Version policy

**One number, every backend.** C, the Rust crate, the Java artifact, the NuGet package and
this one all carry the repo `VERSION`. Never edit the version in `library/package.json` by
hand.

**Never above the Core.** No backend publishes a version the C library has not released.

**Bump `MINOR` whenever the API breaks.** Below 1.0 npm treats the minor as the breaking
position: `^0.8.4` accepts every later 0.8.x and nothing else, so a break shipped in a patch
reaches every consumer's next install.

**A published version is immutable.** npm never accepts a second publish of a name and
version, not even after `npm unpublish`. `npm deprecate` leaves it installable with a warning.
Anything wrong in a published tarball is fixed only by a new version, so every step below that
can still be undone comes before the one that cannot.

## Credentials

Publish as an owner of the package (`npm owner ls ta-lib`), from your own machine, signed in
with `npm login`, with two-factor authentication on: `npm publish` asks for the one-time
password. No CI secret and no token in a file.

**Later: trusted publishing** (GitHub OIDC, with `--provenance`). Its policy names the publish
workflow by **file name**, so that workflow is written first and this runbook then names it.

## Publishing

(TP1) Pre-flight.

The nightly's `typescript` job runs the package gate on every Node major CI supports. Confirm
its latest run is green on the commit being tagged, then run these anyway: the nightly can
predate that commit.

From a clean worktree detached at the release tag `v<version>`:

```bash
python3 scripts/build.py generate          # output/ts must be regenerated, not stale
git status --porcelain                     # must be empty
python3 scripts/build.py libraries --language=ts
```

Then the cross-language gates. Build the servers first: `bin/` holds whatever the last build
left there, and the gates then compare some other commit's TypeScript with this one's C.

```bash
python3 scripts/build.py servers --language=c,ts
(cd bin && ./ta_regtest --codegen    --language=c,ts)
(cd bin && ./ta_regtest --xlang-hash --language=c,ts)
```

Then check that npm can still take this version (see [Versions 0.9.0, 0.10.0 and
0.11.0](#versions-090-0100-and-0110)):

```bash
npm view ta-lib versions --json            # must not list VERSION
```

Then the package gate:

```bash
python3 scripts/ts_package_check.py --output <empty dir>
```

It ends at `=== PACKAGE GATE PASSED ===`, and **the `ta-lib-<version>.tgz` it leaves in that
directory is the file to publish**, not a later, separate pack.

(TP2) Rehearse, with the same file:

```bash
npm publish <dir>/ta-lib-<version>.tgz --tag latest --dry-run
```

Check the name, the version and the file list it prints. Keep `--tag latest`: npm 11 refuses
to publish below the current `latest` without an explicit tag, and `latest` can name the earlier
package's 0.11.0 ([below](#versions-090-0100-and-0110)). **This is the last reversible step.**

(TP3) Publish:

```bash
npm publish <dir>/ta-lib-<version>.tgz --tag latest
```

(TP4) Verify from a clean package cache, which is a different thing from verifying the file
you published:

```bash
npm view ta-lib dist-tags.latest           # must print VERSION
cd <empty dir> && npm init -y
npm_config_cache=<another empty dir> npm install ta-lib@<version>
node --input-type=module -e "import { sma } from 'ta-lib'; console.log(sma(0, 2, [1, 2, 3], 2, new Float64Array(2)))"
node -e "console.log(require('ta-lib').smaLookback(30))"
```

The two `node` lines print `OutRange { begIdx: 1, count: 2 }` and `29`. A `latest` other than
`VERSION` sends a bare `npm install ta-lib` to another version: fix it with
`npm dist-tag add ta-lib@<version> latest`. A `404` right after the
publish is registry lag: retry in a minute.

(TP5) Record the public API, in the worktree at the published commit:

```bash
python3 scripts/ts_package_check.py --write-api-snapshot
```

It **rewrites** `ta_codegen/output/ts/PublicAPI.Shipped.txt` from the packed declarations.
Without this, a declaration added since the previous publish can later be dropped with no gate
failing.

Bring the change to dev as a patch, not a copy, so whatever dev changed in the file since the
release survives: in the worktree, `git add -N` the file and
`git diff -- ta_codegen/output/ts/PublicAPI.Shipped.txt`, then `git apply --3way` on dev, and
commit there. Run on dev instead, the script would also record declarations added since the
release as shipped.

(TP6) Tag the published commit `ts-v<version>` and push the tag.

## Versions 0.9.0, 0.10.0 and 0.11.0

`ta-lib` 0.9.0, 0.10.0 and 0.11.0 on npm belong to an unrelated earlier package that used the
same name. npm never accepts a version it has already seen, so this package can never publish
those three: TP1's version check catches a `VERSION` that lands on one. They are deprecated; leave
them on the registry: the package has public dependents, so npm refuses to unpublish them.
