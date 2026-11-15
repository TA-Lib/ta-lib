# Go Publish Runbook

Tagging the module `ta-lib.github.io/go`: release candidates, then the official release.

**An official release waits for `c-publish-runbook.md`.** A release candidate waits for nothing.
`README.md` in this directory holds the order.

This is a **per-release** procedure. Nothing uploads: the "Release Go module" workflow
(`.github/workflows/release-go.yml`) pushes the tag `ta_codegen/output/go/v<version>`, and Go's
module proxy serves the module from the repository at that tag. The proxy keeps a version's
content forever and the checksum database records it publicly, so the content at a Go tag must
never change.

## IMPORTANT: first release only

Before the first Go tag of any kind, release candidates included:

1. The import-path page must serve the go-import tag. This prints one line, which must contain
   `ta-lib.github.io/go git https://github.com/TA-Lib/ta-lib ta_codegen/output/go`:
   `curl -s 'https://ta-lib.github.io/go?go-get=1' | grep go-import`
2. After the first tag, GP2 is the first fetch of this subdirectory module through
   proxy.golang.org. Do not announce the version until it passes.

## Release candidate

(GP1) Pick a commit main carries, with both main nightlies green on it
(`scripts/nightly_status.py --branch main --sha <commit>`). Run "Release Go module" with
version `vX.Y.Z-rc.N` and that commit. N starts at 1 for each X.Y.Z.

(GP2) Fetch the version through the proxy, from an empty module outside the repository:

```bash
cd <empty dir> && go mod init example.com/check
GOPROXY=https://proxy.golang.org GOFLAGS=-mod=mod go list -m ta-lib.github.io/go@<version>
```

It prints `ta-lib.github.io/go <version>`. A "not found" right after the tag is proxy lag:
retry in a minute.

(GP3) Give the version to the integrators the release candidate is coordinated with. Until the
official release no public page tells how to install the module.

## Official release

(GP4) On dev, before CP5 of `c-publish-runbook.md`, remove the "planned for official release"
line from `ta_codegen/output/go/README.md` and the `::: warning Not yet released` block from
`website/src/api/go/README.md` and `website/src/api/go/stream/README.md`, if still there. The
workflow refuses a final version while any is. In the same change add the install line,
`go get ta-lib.github.io/go`, to that README, to section 2.0 of `website/src/api/go/README.md`
and as a Go section of `website/src/install/README.md`, and put the pkg.go.dev link
(`https://pkg.go.dev/ta-lib.github.io/go`) back in section 5.0 of `website/src/api/go/README.md`.
If the C release commit still carries the line or a banner, remove them on dev, bring that to
main, and pass the main commit as `commit` in GP5.

(GP5) After `c-publish-runbook.md`, run "Release Go module" with version `vX.Y.Z`. Without
`commit` it tags the commit of the C tag `vX.Y.Z`; a `commit` given must descend from that tag.

(GP6) Verify:

```bash
git fetch --tags origin
git rev-parse "vX.Y.Z^{commit}" "ta_codegen/output/go/vX.Y.Z^{commit}"   # equal, unless GP5 passed commit
```

Then GP2 with `vX.Y.Z`, and check that https://pkg.go.dev/ta-lib.github.io/go lists it. GP2's
fetch is what makes pkg.go.dev index the version, which can take a few minutes.

## Withdrawing a version

Never move or delete a Go tag. A bad version is withdrawn by a `retract` directive in
`ta_codegen/output/go/go.mod` of a later version:

```
// <why>
retract vX.Y.Z
```

`go.mod` is generated: add the directive to `GO_MOD` in `ta_codegen/generator/src/backends/go.rs`
and regenerate. It takes effect when that later version is tagged.

## If the history is rewritten

A version fetched untagged stops resolving: its pseudo-version names a commit the rewrite
drops. Re-point each `ta_codegen/output/go/v*` tag to its rewritten commit, after checking the
module's content did not change:

```bash
git diff <old commit>:ta_codegen/output/go <new commit>:ta_codegen/output/go   # must print nothing
git tag -f ta_codegen/output/go/<version> <new commit>
git push --force origin refs/tags/ta_codegen/output/go/<version>
```
