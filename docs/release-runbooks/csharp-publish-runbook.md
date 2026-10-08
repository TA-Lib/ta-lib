# C# Publish Runbook

Releasing the `TALib` package to nuget.org.

**`c-publish-runbook.md` must be completed first.** `README.md` in this directory holds the
order.

**Status:** not yet published. The package itself is ready — `IsPackable` is on, the nuspec is
filled in, the LICENSE, README and icon ship inside the `.nupkg`, and `scripts/sync.py` owns
the csproj `<Version>` (issue #444, A1-A3). What is not ready is the identity: the package id
(#444 B1), the nuget.org account (B2) and the credential (B3) are all open, and **none of this
runbook's push steps can be executed until they are decided.** Everything before the push can
be, and is, run today.

This is a **per-release** procedure. The package version is locked to the repo `VERSION`.

---

## What ships

One project publishes: `ta_codegen/output/csharp/library/TALib.csproj`. The hand-written
suites in `library/test/` are a sibling project and are excluded from the package by
`DefaultItemExcludes` — without that exclusion the SDK's `**/*.cs` glob pulls the suites and
their `obj/` AssemblyInfo into the shipped DLL.

A pack produces two files. Their contents, verified from a pack of 0.8.2:

| `TALib.<version>.nupkg` | `TALib.<version>.snupkg` |
|---|---|
| `TALib.nuspec` | `TALib.nuspec` |
| `lib/net10.0/TALib.dll` | `lib/net10.0/TALib.pdb` |
| `lib/net10.0/TALib.xml` | |
| `LICENSE`, `README.md`, `icon.png` | |

The `.snupkg` is the symbol package and is pushed separately, after the `.nupkg`.

**The id today is not a decision.** `PackageId` is deliberately unset, so it falls back to
`AssemblyName` and a pack is `TALib.<version>.nupkg`. The csproj says why:

> No PackageId yet. The default is AssemblyName, an id the project does not own, and pushing
> to an id someone else owns answers 403 with text that blames the API key.

So the filename a pack produces today is **not** the id that will ship. B1 decides that, and
it is permanent.

## Version policy

**One number, every backend.** C, the Rust crate, the Java artifact and this package all carry
the repo `VERSION` (`0.8.2` at the time of writing). `scripts/sync.py` owns the csproj
`<Version>`; never edit it by hand.

A rehearsal version is `-p:PackageVersion=...`. Not `VersionSuffix`, which is a silent no-op
against an explicit `<Version>`, and not `-p:Version`, which changes the DLL as well.

## Credentials

**Open — B2 and B3 in #444.** This section cannot be written correctly until they are
decided, and guessing it would be worse than leaving it visibly open:

- the account is a Microsoft account with 2FA, not a GitHub sign-in, and its username can
  never be renamed. Ownership goes to whoever pushes first, so the organization account has
  to exist **before** the first push and the first key has to name it as package owner;
- a new API key expires after at most 30 days, which makes "create a key" a per-release step;
- Trusted Publishing (GitHub OIDC, 1-hour key) keys its policy on the **workflow file name**,
  so choosing it means writing the publish workflow first and this runbook referencing it by
  name.

The first push needs the "push new packages" scope. No signing key is owed: nuget.org
repository-signs, and an author certificate is a one-way switch.

## Publishing

(NP1) Pre-flight.

The nightly runs the three C# jobs plus the ones a local machine cannot. Confirm its latest
run is green on the commit being tagged, then run these anyway — the nightly can predate that
commit.

From the repo root:

```bash
python3 scripts/build.py generate          # output/csharp must be regenerated, not stale
git status --porcelain                     # must be empty
git clean -xdn -- ta_codegen/output/csharp # must be silent (F2)
python3 scripts/build.py libraries --language=csharp
```

Then the cross-language gates, from `bin/`:

```bash
./ta_regtest --codegen   --language=c,csharp
./ta_regtest --xlang-hash --language=c,csharp
```

Then the package gate, which is the one that matters here:

```bash
python3 scripts/csharp_package_check.py --output <empty dir>
```

It packs from the clean tree, checks that the two packages hold exactly the expected entries
and that the packed LICENSE, README and icon are byte-identical to their sources, checks the
nuspec's version against `VERSION` and its repository against `TA-Lib/ta-lib` at HEAD, and
then consumes the package twice from a throwaway cache: once under the JIT and once published
with NativeAOT and `TrimMode=full`. Both runs must print the same thing. It ends at
`=== PACKAGE GATE PASSED ===`, and **the `.nupkg` and `.snupkg` it leaves in that directory
are the files to push** — not a later, separate pack.

The script takes no positional arguments and has no `--help`; an unrecognised argument is
passed through to the build, which then fails with `the build failed with no diagnostic this
script reads`. That message is about the stray argument, not about the tree.

(NP2) **Decide here.** nuget.org has no delete. It has unlist, which still serves the package
to anyone asking for that exact version, and deprecate, which leaves it installable with a
warning. The pre-push steps above are the last point at which anything is reversible.

(NP3) Push the package, then the symbols.

Blocked on B1-B3: the id, the account and the credential are what the push names, and none is
decided. When they are, this step is `dotnet nuget push` for the `.nupkg` and then the
`.snupkg`, against the key B3 chooses.

(NP4) **The push's success response is not the end.** Validation and indexing are asynchronous
and report by email. The step ends when the package reads **listed** on nuget.org, not when
the push returns.

(NP5) Verify from a clean package cache, which is a different thing from verifying the file
you pushed:

```bash
python3 scripts/csharp_package_check.py --from-source https://api.nuget.org/v3/index.json
```

(NP6) On the published commit, record the public API:

```bash
python3 scripts/csharp_public_api.py
```

It **rewrites** `ta_codegen/output/csharp/library/PublicAPI.Shipped.txt` and prints what it
did (`N member(s) added, M listed`). Commit the result. Without this, members added since the
last publish ship unguarded. The file is 3196 lines as committed today and the script brings
it to 3804, so the first run after this card lands will report a large addition; that is the
backlog, not a fault.

(NP7) Tag `csharp-v<version>`.

(NP8) **Flip the website** (#444 D1) and update `README.md` in this directory: C# is listed
there as "not published yet".

(NP9) **First publish only:** apply for the id-prefix reservation (#444 B4). It needs the
license element and the icon, both of which already ship.

## Known, not covered here

- **The id situation (#444 B1).** `TALib`, `TA-Lib`, `TALib.NET` and `TALib.NETCore` are taken.
  A `TALib.*` id has a cost of its own: `TALib.NETCore` also declares `TALib.Core`, so a
  consumer holding both gets CS0433. Recovering `TA-Lib` is tracked outside #444. Shipping
  under two ids later is permanent — nuget.org cannot merge them.
- **What the recorded API surface cannot see** (#444 E13): a changed base type, interface list,
  enum underlying type or sealedness. E13 detects removed or changed members only.
- **Where the package is built** (#444 F2) is undecided. A laptop pack compiles any untracked
  `.cs` into the DLL while the nuspec's `commit=` names HEAD, which is why NP1 requires both
  `git status --porcelain` empty and `git clean -xdn` silent. A byte-identical DLL also needs
  the same .NET runtime and the same line endings; the repo has no `.gitattributes`.

## What has been run, and where

Every step above up to NP2 was executed on macOS 25.5 (x64) with .NET SDK 10.0.112 against
`0a4c36e79`:

| step | result |
|---|---|
| `build.py libraries --language=csharp` | rc=0, all 11 C# suites passed |
| `ta_regtest --codegen --language=c,csharp` | C 231/0, C# 231/0 |
| `ta_regtest --xlang-hash --language=c,csharp` | 232 functions, 0 mismatches, bit-identical |
| `csharp_package_check.py --output <dir>` | `=== PACKAGE GATE PASSED ===`, NativeAOT included |
| `csharp_public_api.py` | 609 member(s) added, 3804 listed |

One caution learned while running them, because it costs an hour to diagnose: `bin/` holds
the servers from whatever branch built them last. A `--codegen` run after building another
branch reports a metadata mismatch that is about the stale binary, not about the tree —
`TA_GetFuncInfo flags C=<x> server=<y>` differing by exactly one flag bit is that, not a
regression. Rebuild the library and the servers from the commit being released before
reading either gate.

NP3 onwards cannot be run from any machine until B1-B3 are decided, and nothing in this
runbook should be read as having been rehearsed against nuget.org.
