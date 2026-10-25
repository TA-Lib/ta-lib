# C# Publish Runbook

Releasing the `TA-Lib` package to nuget.org.

**`c-publish-runbook.md` must be completed first.** `README.md` in this directory holds the
order.

This is a **per-release** procedure. The package version is locked to the repo `VERSION`.

---

## What ships

One project publishes: `ta_codegen/output/csharp/library/TALib.csproj`. The hand-written
suites in `library/test/` are a sibling project and are excluded from the package by
`DefaultItemExcludes`: without that exclusion the SDK's `**/*.cs` glob pulls the suites and
their `obj/` AssemblyInfo into the shipped DLL.

A pack produces two files:

| `TA-Lib.<version>.nupkg` | `TA-Lib.<version>.snupkg` |
|---|---|
| `TA-Lib.nuspec` | `TA-Lib.nuspec` |
| `lib/net10.0/TALib.dll` | `lib/net10.0/TALib.pdb` |
| `lib/net10.0/TALib.xml` | |
| `LICENSE`, `README.md`, `icon.png` | |

The `.snupkg` is the symbol package and is pushed separately, after the `.nupkg`.

**The package id is `TA-Lib`; the assembly and the namespace are `TALib`.** The id also holds
0.5.0 to 0.5.0.3, from before this repository published it.

## Version policy

**One number, every backend.** C, the Rust crate, the Java artifact and this package all carry
the repo `VERSION`. `scripts/sync.py` owns the csproj
`<Version>`; never edit it by hand.

A rehearsal version is `-p:PackageVersion=...`. Not `VersionSuffix`, which is a silent no-op
against an explicit `<Version>`, and not `-p:Version`, which changes the DLL as well.
Rehearse the id and the README on the nuget.org Upload page, where nothing publishes until
Submit. Not on int.nugettest.org: the id has no owner there, so it answers a different
question.

## Credentials

The nuget.org account `mario4tier` owns the id.

**Publish by web upload** (NP3) until a publish workflow exists, signed in as an owner, with
no API key. nuget.org has no dry run, so a workflow's first run is itself a publish.

**Then: Trusted Publishing** (GitHub OIDC, a one-hour key). Its policy names the publish
workflow by **file name**, so that workflow is written first and this runbook then names it.

No signing key is owed: nuget.org repository-signs, and an author certificate is a one-way
switch.

## Publishing

(NP1) Pre-flight.

The nightly runs the C# jobs, including the ones a local machine cannot. Confirm its latest
run is green on the commit being tagged, then run these anyway: the nightly can predate that
commit.

From a clean worktree detached at the release tag `v<version>`:

```bash
python3 scripts/build.py generate          # output/csharp must be regenerated, not stale
git status --porcelain                     # must be empty
python3 scripts/build.py libraries --language=csharp
```

Then the cross-language gates. Build the servers first: `bin/` holds whatever the last build
left there, and the gates then compare some other commit's C# with this one's C.

```bash
python3 scripts/build.py servers --language=c,csharp
(cd bin && ./ta_regtest --codegen    --language=c,csharp)
(cd bin && ./ta_regtest --xlang-hash --language=c,csharp)
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
are the files to push**, not a later, separate pack.

(NP2) **Decide here.** nuget.org has no delete. It has unlist, which still serves the package
to anyone asking for that exact version, and deprecate, which leaves it installable with a
warning. The pre-push steps above are the last point at which anything is reversible.

(NP3) Publish the package, then the symbols.

Upload the `.nupkg` at <https://www.nuget.org/packages/manage/upload>. Check the id, the
version and the README preview on the Verify section, then Submit. Then the `.snupkg`: its
documentation describes only a command-line push, so if the Upload page refuses it, create an
API key scoped to this id with the shortest expiry, run
`dotnet nuget push <file>.snupkg --source https://api.nuget.org/v3/index.json --api-key <key>`,
and delete the key.

(NP4) **An accepted upload is not the end.** Validation and indexing are asynchronous
and report by email. The step ends when the package reads **listed** on nuget.org, not when
the upload is accepted.

(NP5) Verify from a clean package cache, which is a different thing from verifying the file
you pushed:

```bash
python3 scripts/csharp_package_check.py --from-source https://api.nuget.org/v3/index.json
```

(NP6) Record the public API, in the worktree at the published commit:

```bash
python3 scripts/csharp_public_api.py
```

It **rewrites** `ta_codegen/output/csharp/library/PublicAPI.Shipped.txt`, moves
`PublicAPI.Unshipped.txt`'s entries into it, and prints how many it added and moved: together,
every member added since the previous publish. Without this, those members ship unguarded.

Bring the change to dev as a patch, not a copy, so whatever dev changed in the two files since
the release survives: `git diff -- 'ta_codegen/output/csharp/library/PublicAPI.*.txt'` in the
worktree, then `git apply --3way` on dev, and commit there. Run on dev instead, the script
would also mark members added since the release as shipped.

(NP7) Tag the published commit `csharp-v<version>` and push the tag.

## Known, not covered here

- **Another package on nuget.org declares a type named `TALib.Core`.** A consumer referencing
  both gets CS0433 and needs an extern alias.
- **What the recorded API surface cannot see** (#444 E13): a changed base type, interface list,
  enum underlying type or sealedness. E13 detects removed or changed members only.
