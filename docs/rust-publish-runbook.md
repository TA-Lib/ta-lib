# Rust Publish Runbook — releasing the `ta-lib` crate to crates.io

**Status:** `ta-lib-dispatch` 0.1.2 published 2026-08-12. `ta-lib` not yet published; the
remaining blockers are tracked in issue #179.

This is a **per-release** procedure, not a one-off. The crate version is locked to the repo
`VERSION`, and a release in practice changes something every backend shares, so crates.io
publication is part of cutting a release the same way the source tarball is.

---

## 1. What ships

The Rust workspace (`ta_codegen/output/rust/`) has three members; **two of them publish.**

| Member | Crate | Published | Version |
|---|---|---|---|
| `library/` | `ta-lib` | yes | locked to repo `VERSION` |
| `dispatch/` | `ta-lib-dispatch` | yes | independent (`DISPATCH_VERSION`) |
| `tools/` | `ta-lib-tools` | **no** — `publish = false` | — |

`dispatch` is one macro: the runtime FMA CPU-feature check, which is where the workspace's
only `unsafe` lives so the library can keep `#![forbid(unsafe_code)]`. It is versioned
independently because it changes when that macro changes, which is close to never — it is
**not** part of the lockstep in §2.

Both crate manifests, both READMEs and both `LICENSE` files are **generated**
(`generate_rust_crate_scaffolding`, `ta_codegen/generator/src/main.rs`). Never hand-edit
them; the regen-check gate fails if you do.

## 2. Version policy

**One number, every backend.** C, the Rust crate and the Java artifact all carry the repo
`VERSION` — they are one generated library, not four that ship together.

**The obligation that buys: bump `MINOR` whenever any backend breaks its API**, even when
the change that motivated the release was a C patch. Cargo treats the minor as the breaking
position below 1.0, so `ta-lib = "0.8"` accepts every 0.8.x — a break shipped in a patch
upgrades itself into consumers' builds with no signal.

`cargo-semver-checks` in the dev nightly is the mechanical guard (#179 E7). It needs a
published version to compare against, so it can only be added *after* the first release.

`ta-lib-dispatch` is exempt: it carries its own version and is not bound to a release.

## 3. Publish order, and why it is not optional

**`ta-lib-dispatch` first, then `ta-lib`** — and only when dispatch actually changed.

`library/Cargo.toml` pins the support crate exactly (`version = "=<DISPATCH_VERSION>"`, the
companion-crate pattern), so a published `ta-lib` can never float onto a newer macro. The
cost is that **while the pinned dispatch version is not yet on crates.io, `cargo package -p
ta-lib` and `cargo publish -p ta-lib` cannot resolve it at all.** Package the pair instead
and cargo verifies `ta-lib` against a temporary registry built from the sibling:

```bash
cargo package -p ta-lib-dispatch -p ta-lib
```

A published `.crate` is immutable. Changing *anything* inside one — the LICENSE, a README
badge, a keyword — is only expressible as a new version. That is what turned dispatch 0.1.1
into 0.1.2. Get the contents right before publishing, not after.

## 4. Pre-flight

From the repo root:

```bash
scripts/build.py generate        # output/rust must be regenerated, not stale
git status --porcelain           # must be empty — publish from a clean, committed tree
```

Then the crate's own gates, from `ta_codegen/output/rust/`:

```bash
cargo clippy --all-targets -- -D warnings
cargo test --lib -p ta-lib
cargo test --doc -p ta-lib
RUSTDOCFLAGS="-D warnings" cargo doc --no-deps -p ta-lib
cargo package -p ta-lib-dispatch -p ta-lib      # packages AND verify-builds both
```

And the cross-language gate, from `bin/`:

```bash
./ta_regtest --codegen --language=c,rust
./ta_regtest --xlang-hash --language=c,rust
```

Sanity-check the tarball contents once — the file list is easy to break and impossible to
fix after the fact:

```bash
tar tzf target/package/ta-lib-<VERSION>.crate | grep -Ev '^ta-lib-[^/]+/src/ta_func/'
# expect: Cargo.toml, Cargo.lock, LICENSE, README.md, src/lib.rs,
#         src/abstract_api.rs, src/ta_func_api.xml
```

## 5. Credentials

Not yet decided (#179 A6). Today: a maintainer's personal crates.io token, stored by
`cargo login`, publishing from a local machine. That is what published dispatch 0.1.2.

The alternative is crates.io **Trusted Publishing** — OIDC from a named GitHub Actions
workflow, no long-lived secret anywhere. The repo currently has zero Actions secrets and
both crates report `trustpub_only: false`. Worth revisiting now that publication is
per-release rather than one-off: a personal token on one machine is a bus factor.

## 6. Publish

```bash
cd ta_codegen/output/rust

cargo publish -p ta-lib-dispatch --dry-run     # only if dispatch changed
cargo publish -p ta-lib-dispatch               # permanent

cargo publish -p ta-lib --dry-run              # resolves only after the above is live
cargo publish -p ta-lib                        # permanent
```

If the second publish cannot resolve the pin immediately after the first, that is index
lag — retry in a minute. It is not a manifest problem.

## 7. After

- **Check docs.rs actually built** — `https://docs.rs/ta-lib/<VERSION>`. docs.rs compiles
  independently of `cargo publish`, so a rustdoc failure there is invisible from a
  successful publish.
- **Badges lag by minutes.** The README badges are live shields.io / docs.rs renders keyed
  on crate *name*; they report the registry's newest version and know nothing about the
  tree. Nothing to update.
- **Flip the website** (#179 B4) — `website/src/api/rust/README.md` and
  `website/src/install/README.md` still carry pre-release banners. The site deploys from
  `main` on push.
- **`cargo owner --list`** on both crates if the maintainer set changed.

## 8. Known, not covered here

- `ta-lib` 0.1.0–0.1.2 on crates.io are an **unrelated third-party FFI wrapper**, not ours:
  5097 downloads, zero reverse dependencies. Until the first real publish, the `ta-lib`
  README badges advertise that crate and its failing docs build. Whether to yank is #179 B2
  — it breaks no published crate and no existing lockfile, and only blocks new `^0.1`
  resolutions.
- Nothing has ever run `cargo publish --dry-run` in CI (#179 E1). A packaging break is
  currently discovered with the version already burned.
