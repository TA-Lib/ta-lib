# Canonical Cutover Runbook — making `ta_codegen` output the shipped library

**Status:** ✅ **CUTOVER COMPLETE (2026-06-30) — gen_code is removed; ta_codegen is the single
generator; the C build (CMake + autotools) is C-only.** #91 (Java) + #92 (packaging) merged to `dev`;
**Stage 7 done directly on `dev`** (commit `5a0d7a48`): deleted `src/tools/gen_code/` + templates +
`ta_java_defs.h` + the dead .NET/MSVC IDE surface; removed gen_code from CMake/autotools (incl. the
`-DTA_GEN_CODE` `libta_abstract_gc` variant + `gen_rust` dist); **moved all Rust/ta_codegen building
out of CMake into `scripts/build.py` (cargo-direct) — CMake never invokes cargo.** Verified: C-only
CMake build (0 cargo, gen_code absent) + autotools build; `regtest.py` exit 0 + `--codegen` 161/0 all
4 langs + absolute oracle + bench; ABI gate green; cargo test 445 + clippy clean; regen deterministic.
Remaining: only optional polish (drop the dead `gen_code_pass` digest field; Stage 8 retire the
reference tag, much later). · **History below.**

**Status (historical):** IN PROGRESS — **generate-into-`src` (option B)**. DONE: Stage 0 (tag), 1
(reference-as-server #7), 2 (ABI gate #2), **Stage 4-B** (C indicators → `src/ta_func`),
**Stage 5-B / #3** (the `ta_abstract` layer → `src/ta_abstract`), and **Stage 5-C** (the last two
gen_code C-side scalar generators — `include/ta_defs.h` FuncUnstId enum + `src/ta_common/ta_retcode.c`
— now ta_codegen-generated, **byte-identical**) (2026-06-29). **`output/c` holds ONLY codegen-specific
artifacts.** The whole shipped C library (indicators + abstract + ta_defs.h FuncUnstId + ta_retcode +
hand-written ta_common) is ta_codegen-owned. **gen_code's entire C side is now retired.** Verified:
CMake **and** autotools build the shipped lib from `src/`; **gen_code still compiles**; absolute
oracle passes; `--codegen` 161/0 all 4 langs vs frozen `ta_ref_serve`; ABI gate green; regen
deterministic; generator `cargo test` (445) + clippy clean. Nothing committed.
**#4 (Java backend) DONE 2026-06-29 (not committed)** — `ta_codegen` now generates the shipped Java
(`Core.java` GENCODE + `CoreAnnotated.java` + `FuncUnstId.java`); gen_code's last unique role is gone.
Verified: cargo test 444 + clippy clean; regen oracle = only `java/src` + generator + `output/java`
(C lib + `include/` ABI untouched); javac whole `java/src` + sanity call; `--codegen` **161/0 all 4
langs**; gen_code still compiles. See the **#4** prerequisite-gate entry below for the full detail.
**Remaining: Stage 6-B (package.py) + Stage 7 (remove gen_code).** · **Branch:** `dev`

> This is a *careful, reversible, one-step-at-a-time* runbook for retiring the legacy
> `gen_code` generator and making `ta_codegen`'s generated C the canonical/shipped
> library. Every stage has a **green/red checkpoint** and an explicit **rollback**.
> Do not proceed past a red checkpoint. Coordinate each stage with the maintainer.

---

## Sandbox proof-of-concept results (2026-06-27)

Ran Stage 3 in a throwaway detached `git worktree` (real tree untouched): symlinked
`src/{ta_func,ta_common,ta_abstract}` → `ta_codegen/output/c/*`, built with CMake, ran `ta_regtest`.

**✅ The build mechanism works.** CMake compiled `libta-lib` entirely from the symlinked
generated sources and linked `ta_regtest`, once three layout gaps were handled.

**Three file-layout gaps — now RESOLVED & VALIDATED in the Stage 3 throwaway (2026-06-28):**
1. **`ta_utility.c` relocation.** Generated lives in `output/c/ta_common/` (and the generated
   `ta_utility.h` *strips the `TA_INT_*` decls* the reference header had — correct, since the
   generated lib uses `_Unguarded`/`_Private`). Fix that worked: CMake → repoint `LIB_SOURCES`
   `ta_utility.c` to the generated `ta_common` path **and** add `ta_codegen/output/c/ta_common`
   to `include_directories` (so the generated `ta_utility.h` is found); autotools → build
   `ta_utility.c` from `ta_common`'s `Makefile.am`.
2. **`ta_NVI.c` / `ta_PVI.c`.** Stage 2 proved these are **dead exported symbols** (absent from
   `include/`, unregistered in `ta_abstract`, not in the 161). Resolution used: **dropped them**
   from `LIB_SOURCES` (ABI-safe). *(Maintainer decision for the real tree: drop — recommended —
   vs port to `ta_codegen/input`. Default: drop.)*
3. **Autotools — the whole-dir symlink breaks per-dir recursion.** Confirmed: emitting a
   `Makefile.am` into the symlink target is **NOT enough** — `make` enters the symlink's
   *physical* path so the Makefile's relative `../../libtool` resolves wrong (`Error 127`,
   "../../libtool: No such file or directory"). **The working fix is option 2: a top-level flat
   list** — drop `ta_func` from `SUBDIRS` and from `configure.ac`'s `AC_CONFIG_FILES`, and list
   `ta_func/ta_*.c` directly in `src/Makefile.am`'s `libta_lib_la_SOURCES` (with `subdir-objects`
   already enabled). Then `make` stays in `src/`, `../libtool` resolves correctly. (CMake is
   unaffected — it references source paths directly, no per-dir recursion.) Also: the explicit
   list correctly ignores the extra `ta_codegen_funcs.c`; a glob would wrongly include it.

**✅ FIXED (task #8) — MFI/CCI fixed-array buffer overflow (flattened CIRCBUF).**
ROOT CAUSE was in the **input sources** (not the generator): `CIRCBUF` was fully removed during
conversion and flattened to too-small fixed arrays. MFI (`[50]`) and CCI (`[30]`) used the
reference's *dynamic* `CIRCBUF_INIT(…, optInTimePeriod)` (mallocs when period exceeds the static
size); the flattening dropped the malloc → overflow for period > 50 / > 30, in C, Rust, and Java
alike. A comprehensive audit (all 6 CIRCBUF users + all 24 fixed-array input files) found **only
MFI/CCI** affected (HT_*/MAMA use `CIRCBUF_INIT_LOCAL_ONLY` = always-static, safe; cdl*/ultosc/sar
are fixed by structure). Fixed `ta_codegen/input/{mfi,cci}/*.c` to allocate sized to
`optInTimePeriod`. Verified: regen oracle clean (only MFI/CCI changed); the crash scenario now
passes + full 161 in-process; `--codegen` MFI/CCI PASS in all 4 languages. Test-coverage
follow-up tracked separately (large-period cases).

**🟡 Pre-existing, NOT a blocker (task #9) — `test_abstract` XML over-read.** ASAN flags a
1-byte global over-read of `TA_FunctionDescriptionXMLArray` in `test_abstract.c:957`, but it is
**identical in the reference** (same array, same size) — `ta_func_api.c`'s array isn't
NUL-terminated in either gen_code or ta_codegen. Latent; fix opportunistically.

**Verification lesson (refines task #7):** the cutover oracle must run the in-process
hand-written `ta_regtest` tests **linked against the generated lib** (ideally + ASAN), not just
the JSON-RPC servers — the server value-comparison missed a memory-corrupting bug. The current
suite also **under-covers large optInTimePeriod values** (>static CIRCBUF size) — add cases.

---

## Guiding principles (read first — this is why it's safe)

1. **Nothing is destructive.** All work happens on a branch. The current `src/`
   reference is preserved *twice*: in git history, and in an immutable **pinned tag**
   (`reference-pre-cutover`). Any bad step is undone with one `git checkout` / `git reset`.
2. **The reference stays usable throughout.** It is built from the pinned tag (via a
   `git worktree`) and exposed as `ta_ref_serve`, so at *every* checkpoint we can prove
   the new build still matches the old one numerically (`ta_regtest`) and at the ABI
   level (symbol/enum/struct diff).
3. **The build stays green at every checkpoint.** Both CMake and autotools must build,
   and `ta_regtest` must report 161/0, before a stage is accepted.
4. **One step at a time.** Each stage is small, independently verifiable, and reversible.

---

## ⚠️ APPROACH CHANGED 2026-06-29 — generate-into-`src` (option B), NOT symlinks

**Maintainer decision (2026-06-29):** drop the symlink model. `ta_codegen` will generate the
C library **directly into `src/`** (in place, where `gen_code` always wrote), and
`ta_codegen/output/c` keeps **only codegen-specific artifacts** (the JSON-RPC server, the
`.so`/bench unity files). The Rust/Java/.NET backends are unaffected — they remain
fully-generated under `ta_codegen/output/{rust,java,dotnet}` (generated *bindings* with no
canonical home), which is exactly why C, the *product* (with `include/` + downstream
`src/ta_func/*.c` globbing), is treated differently. Rationale: the symlink bought nothing and
*cost* real complexity we hit in Stages 3–4 — autotools/libtool physical-path breakage, a
packaging dereference step, and circular copy-from-`src` ownership transfers. The Stage-4
symlink was implemented, verified, then **reverted** per this decision (2026-06-29); the C
verification spine (tag, reference-as-server #7, ABI gate #2) was kept.

## End state we are building toward (option B)

- `include/` — **stays exactly where it is**, ABI-stable. Public contract for every downstream
  wrapper (Python/Cython, Ruby FFI, Julia, Zig, Perl, PHP, …).
- `src/ta_func`, `src/ta_common`, `src/ta_abstract` — **real directories** holding the
  `ta_codegen`-generated C **in place**, co-located with the hand-written helpers that are
  *not* generated (`ta_utility.{c,h}`, `ta_common/ta_global.c`, `ta_retcode.c`, `ta_version.c`,
  the internal `.h` headers). Generated files carry a `DO NOT EDIT — generated by ta_codegen;
  source: ta_codegen/input/<name>` banner. The build lists (`CMakeLists.txt` `LIB_SOURCES`,
  `src/ta_func/Makefile.am`) keep their current shape — they already reference `src/...` paths,
  now pointing at generated-in-place files. **No symlinks; no autotools option-2; no packaging
  deref; no `ta_utility`/`ta_common` move into `ta_codegen/input`.**
- `ta_codegen/output/c` — **only codegen-specific build artifacts**: `ta_codegen_serve.c`
  (server unity), `ta_codegen_funcs.c` (.so unity), `ta_bench_cg.c` (bench), `ta_abstract_all.c`,
  `ta_lib_types.h`. These `#include` from `src/...` via `-I`.
- `src/tools/` — **stays** (the `ta_regtest` / `ta_bench` harness).
- The old hand/gen_code reference C — preserved at the pinned tag `reference-pre-cutover` + in
  history; reachable via `git worktree` (already stood up at `../ta-lib-ref`).
- `gen_code` — removed last: `ta_codegen` *takes over* writing `src/ta_func` (etc.) from
  gen_code's GENCODE sections, then gen_code is deleted.
- Release tarball — ships `src/` real files as today (no deref needed; the PHP `trader`
  extension's `src/ta_func/*.c` glob just works).

### Generated-vs-hand-written inventory (must be nailed before generate-into-src)
The generator must know exactly which files it owns per directory (the rest are hand-written
and left untouched). Rough cut — **confirm precisely during Stage 4**:
- `src/ta_func/`: generated = the 161 `ta_<NAME>.c` indicators (+ `ta_func_private.h`);
  hand-written = `ta_utility.{c,h}`. (NVI/PVI: dead — delete; not generated, not in `include/`.)
- `src/ta_common/`: hand-written = `ta_global.c`, `ta_retcode.c`, `ta_version.c`, `ta_memory.h`,
  `ta_global.h`, `ta_magic_nb.h`, `ta_pragma.h`. (Mostly/entirely hand-written — likely
  little/none generated; `ta_codegen` currently just *copies* these into output.)
- `src/ta_abstract/`: generated = the introspection layer (`tables/×26`, `ta_func_api.c`,
  `ta_group_idx.c`, `frames/ta_frame.c`, parts of `ta_def_ui.c`); hand-written = `ta_abstract.c`,
  headers (`ta_def_ui.h`, `ta_frame_priv.h`), `ta_retcode.csv`, gen_code-only files to delete.

---

## Prerequisite gates (must be green BEFORE the C generate-into-`src` cutover)

These are the tracked Phase-0 tasks. The symlink cutover is Phase 2; do not start it until:

- [ ] **#1 Perf parity** — confirmed (sample green; ACCBANDS +3.8%, the "1.5×" was noise).
- [x] **#3 Ownership transfer** — DONE 2026-06-29. ta_codegen is the canonical writer of the
      ta_abstract layer, generated from the IR directly into `src/ta_abstract` (Stage 5-B). The
      shipped C library (indicators + abstract) is fully generated in `src/`; `output/c` holds
      only codegen artifacts. (`ta_common`/`ta_retcode.c` confirmed hand-written, stay in `src/`.)
- [x] **#7 Reference-as-server** — DONE 2026-06-28. `ta_regtest`'s codegen comparison no
      longer computes its baseline in-process; it diffs every language server (incl. the
      generated C server) against `ta_ref_serve` built from the pinned-tag worktree. Verified
      161/0 all 4 languages. *(This is what makes the comparison non-circular post-cutover.)*
      See "Stage 1" below for what changed and the scope decision (baseline only).
- [x] **#2 ABI gate** — DONE 2026-06-28. Wired as `scripts/abi_gate.py` (reusable; exit 0/1/2).
      Green: the generated lib exports a **superset** of the reference public ABI (all 522
      public function symbols present, verified bidirectionally), `include/` unchanged (type ABI
      frozen). See "Stage 2" below for the symbol deltas + the `_Unguarded` visibility decision.
- [x] **#4 Java backend** — DONE 2026-06-29 (not yet committed). `ta_codegen` now generates the
      shipped Java (`java/.../Core.java` GENCODE + `CoreAnnotated.java` + `FuncUnstId.java`), retiring
      gen_code's last unique role. Four increments: (A) generator foundation — `to_java_method_name`
      uses the YAML `camel_case` field (irregular/typo names), `Registry` threads a `java_names` map so
      cross-calls match, `Logic`→`Unguarded` (matches C's public `_Unguarded`), `emit_java_unpacking`
      → canonical `candleSettings[CandleSettingType.X.ordinal()].rangeType.ordinal()` (server candle
      model switched to a `CandleSetting[]`+`RangeType` enum to match); (B) `FuncUnstId.java` from
      `enums.yaml` (adds the missing `Imi`, sentinels `All`/`None`); (C) `Core.java` GENCODE assembly
      (the verified per-indicator method text + the new public `xxxUnguarded` variants); (D)
      `CoreAnnotated.java` — a faithful gen_code port, **byte-identical except the `avgPrice`/`avgDev`
      canonical reorder** (matches `ta_func_list.txt`/`table_a.c`). Verified: cargo test 444 + forced
      clippy clean; regen deterministic; full regen oracle = only `java/src` + generator + `output/java`
      changed (**nothing in `src/`/`include/`/`output/{c,rust,dotnet}`** — C lib + ABI untouched); javac
      whole `java/src` (40 classes, 0 errors) + a sanity call; `--codegen` Java 161/0 (all-4-lang green
      from increment A, servers unchanged by B–D); gen_code still compiles. Generator: new
      `backends/java_enums.rs` + `backends/java_shipped.rs`, wired in `main.rs`'s `"java"` block (Core/
      CoreAnnotated only on a full, unfiltered `generate`). `ta_java_defs.h` stays gen_code-owned and
      dies WITH gen_code in Stage 7. Deferred to the PR (maintainer: "good enough, fix later"): JUnit
      suite not run (compile + 1 sanity call only), no dedicated unit tests for the new modules.

---

## The reference mechanism (Phase 1 — stand this up first)

```
git tag reference-pre-cutover <last gen_code-built commit>     # immutable snapshot
git worktree add ../ta-lib-ref reference-pre-cutover           # lightweight, shares .git
  └─ build ta_ref_serve there  ── copy binary into bin/
main worktree:
  └─ build ta_codegen_serve_c/_rust/java/dotnet + the ta_regtest runner
bin/ ends up holding BOTH ta_ref_serve and ta_codegen_serve_* ; ta_regtest spawns them
  side by side over JSON-RPC and diffs outputs. The runner links NEITHER implementation,
  so which worktree built which binary is irrelevant.
```

The hardcoded `test_*.c` expected values remain the generator-independent **absolute** oracle.

---

## Staged execution plan (each stage = checkpoint + rollback)

### Stage 0 — Safety net (zero risk) ✅ DONE 2026-06-28
- **Did:** `git tag -a reference-pre-cutover` at HEAD `0d6b0e64` (the current, correct
  gen_code-built reference — `src/ta_func` there never had the CIRCBUF-flattening bug,
  which lived only in `ta_codegen/input` and is fixed). Stayed on branch `dev` (where the
  cutover work lives). Working tree was clean apart from this runbook.
- **Checkpoint:** ✅ tag `reference-pre-cutover` → `0d6b0e64`.
- **Rollback:** `git tag -d reference-pre-cutover` (pure addition).

### Stage 1 — Reference-as-server (task #7) ✅ DONE 2026-06-28
- **Scope decision (maintainer):** *baseline only.* Only the cross-language comparison
  **baseline** moved off the in-process call; `doRangeTest` self-coherency and the
  hand-written `test_*.c` absolute-value tests still run in-process against the linked lib
  (post-cutover = the generated lib — exactly the in-process/ASAN net the runbook asked for).
  So "runner links nothing" means *the comparison baseline doesn't come from the linked lib*,
  not that ta_regtest stops linking it.
- **Phase A — `test_codegen.c`:** spawn `ta_ref_serve` once (`argv_cref`), thread a `refCp`
  pipe through the context; replaced the in-process `TA_CallFunc` baseline (default + large-
  period passes) with a `ta_ref_serve` JSON-RPC call parsed by a new `parse_ref_baseline()`
  into the same `lastRetCode/lastBegIdx/lastNbElement` + `outRealBufs/outIntBufs` that
  `compare_codegen_output_generic()` already diffs against — so the comparator, request
  builder, and struct are **unchanged**. `c_ref` timing now comes from the server's
  `timing_ns` (apples-to-apples). Removed the now-dead in-process `get_nanotime()`.
  `--codegen` now *requires* `ta_ref_serve` in `bin/` and aborts with a clear message if
  missing (it is no longer optional).
- **Phase B — `scripts/regtest.py`:** new `ensure_reference_serve()` builds `bin/ta_ref_serve`
  from the pinned-tag worktree: auto-creates `../ta-lib-ref` @ `reference-pre-cutover`,
  builds the frozen `libta-lib.a` (`ta-lib-static`) there once, then strips the generated
  indicator/ta_common `#include`s from that tree's `ta_codegen_serve.c` and links the frozen
  lib (`-DTA_REF_SERVE`). Rebuilds only when missing/stale (the tag is immutable). Falls back
  to the current tree with a **loud warning** if the tag/worktree is unavailable (valid only
  pre-cutover). The protocol is byte-for-byte the existing server protocol (probed: emits
  `retCode/outBegIdx/outNBElement/timing_ns/outReal[N]`; `list_functions` = 161).
- **Checkpoint:** ✅ `ta_regtest --codegen` = **161/0 for C, Rust, Java, .NET** with
  `ta_ref_serve` (worktree-built) as the oracle. In-process C absolute tests still pass;
  `ta_codegen/output` untouched (regen oracle clean); both CMake **and** autotools compile
  `test_codegen.c`. Confirmed the nightly/dist CI (`scripts/test-dist.py`) does **not** run
  `--codegen`, so requiring `ta_ref_serve` does not affect CI.
- **Rollback:** `git checkout -- src/tools/ta_regtest/test_codegen.c scripts/regtest.py`;
  `git worktree remove ../ta-lib-ref` (+ `git tag -d reference-pre-cutover` to undo Stage 0).
- **Deferred (not needed for non-circularity):** `server_verify.c` (the hand-written tests'
  server bridge) still compares against the in-process C. Post-cutover that becomes
  redundant (its absolute hardcoded values remain the real oracle), not circular — optional
  cleanup later. **CI/tag portability:** the tag is local; CI that runs `--codegen` would need
  the tag pushed (or `regtest.py`'s fallback) — revisit when/if CI gains a codegen step.

### Stage 2 — ABI gate (task #2) ✅ DONE 2026-06-28
- **Wired:** `scripts/abi_gate.py` — compiles the generated lib sources (`ta_codegen/output/c`,
  excluding the unity/server/bench root files), `nm`s the objects, and diffs the exported
  symbol set against the frozen reference `libta-lib.a` (pinned-tag worktree). Because the
  build uses **no `-fvisibility=hidden` and no version script**, every extern symbol is an
  exported ABI symbol, so `nm` on the archives is a faithful, flag-independent ABI surface.
  Pass rule: every reference *public* symbol (exported AND named in `include/`) must still be
  exported by the generated lib; additions are allowed. Verified bidirectionally (symbol-side
  and from `include/` function declarations).
- **Result (green):**
  - reference exported = 1348; generated exported = 1653; **public = 522, all present in
    generated (0 removed)** → no ABI break.
  - `include/` **unchanged vs the tag** → enums/structs/typedefs (the type ABI) are frozen by
    construction; nothing to diff there.
  - **Additions** (ABI-compatible — adding symbols never breaks a consumer): 322 `*_Unguarded`
    variants (declared in the generated `include/ta_func_unguarded.h`) + 40 per-function
    `TA_DEF_UI_*` abstract descriptors (a naming restructure) + `TA_EMA_Private`/`TA_S_EMA_Private`.
  - **Internal-only removals** (59, none in `include/`, none ABI-relevant): 41 *shared*
    `TA_DEF_UI_*` descriptors (replaced by the per-function ones above); 12 `TA_INT_*`/
    `TA_S_INT_*` shared helpers (EMA/SMA/MACD/PO/VAR/stddev — replaced by `_Unguarded`/`_Private`);
    6 NVI/PVI symbols.
  - **NVI/PVI finding (refines gap #2):** `TA_NVI`/`TA_PVI` are **dead exported symbols** in the
    reference — present as `.c` files but **absent from `include/` and unregistered in
    `ta_abstract`** (not in the 161-func set). So their absence from the generated tree is **not
    an ABI break**; porting them is needed only to satisfy the `LIB_SOURCES` *build* list when
    `src/ta_func` is symlinked (a Stage 3/4 build gap), not for ABI.
- **`_Unguarded` visibility decision (recommended, pending maintainer confirmation):** *accept
  the additions; do NOT add visibility hardening for the cutover.* Rationale: the reference
  itself exported all its internals (no visibility control), so export-everything is the
  status quo — matching it is the minimal, lowest-risk choice and additions can't break
  existing consumers. Hardening (`-fvisibility=hidden` + `TA_LIB_API` export annotations or a
  version script over 522 symbols) is a worthwhile *future* cleanup but is out of scope and
  adds risk to the cutover.
- **Checkpoint:** ✅ ABI diff = additions + internal churn only; 0 public breaks; type ABI frozen.
- **Rollback:** the gate is a check; nothing to roll back. (`scripts/abi_gate.py` is a new file.)

### Stage 3 — ~~PROVE the `src/ta_func` symlink~~  ❌ SUPERSEDED by option B (2026-06-29)
The symlink approach was dropped, so a symlink prove is moot. It WAS done in a throwaway on
2026-06-28 and is kept here only for the **lesson that justified option B**: a whole-dir symlink
**breaks autotools' per-dir libtool recursion** (`make` enters the symlink's *physical* path, so
the Makefile's relative `../../libtool` → `Error 127`), forcing an awkward "flat top-level list"
restructure; plus it needs a packaging dereference step and circular copy-from-`src` ownership
transfers. Generate-into-`src` (B) has none of these. (Also learned, still relevant: a
system-installed `/usr/local/lib/libta-lib.so.0` shadows *dynamically*-linked test binaries —
prefer static-linked `ta_regtest`, which the build already does.)

### Stage 4-B — Generate the C library into `src/ta_func` (in place)  ✅ DONE 2026-06-29
`ta_codegen` now writes the generated C **into `src/ta_func`**, taking over gen_code's
GENCODE-section role there. No symlink, no autotools restructure, no `ta_utility` move.
- **Done:**
  1. **C backend repointed to `src/`** via a new `LanguageBackend::lib_output_dir` hook
     (`backends/mod.rs`); `CBackend` returns `<root>/src/ta_func` and `clean_keep = ["ta_utility.c"]`
     so the hand-written helper survives cleanup. `generate_backend` + `clean_generated_files` use
     it. `ta_func_private.h` now written to `src/ta_func`; the `ta_common`/`ta_utility` copies into
     `output/c` were removed (they stay hand-written in `src/`). Dead `strip_ta_int_declarations`
     deleted (the generated `ta_utility.h` keeps its now-harmless `TA_INT_*` decls — nothing
     references them).
  2. **`output/c` trimmed:** `git rm -r output/c/{ta_func,ta_common}` (171 files). `output/c` now
     holds only `ta_abstract` (generated, pending #3) + the unity/server/bench artifacts.
  3. **Servers/bench/.so re-sourced from `src/`:** the C server, bench, and `libta_codegen_funcs.so`
     unity builds now `-I src src/ta_func src/ta_common` (+ `output/c` first, so `ta_abstract`
     resolves to the generated copy); unity `#include "ta_func/ta_utility.c"` (was `ta_common/...`);
     the `.so` unity scans `src/ta_func` and is written to `output/c/ta_codegen_funcs.c`.
  4. **Dead NVI/PVI deleted** — the `clean` step removes them automatically (not regenerated); the
     `extras` dir-scan in `sorted_source_stems` was removed.
  5. **Banner** on every generated indicator (`gen_header` in `c.rs`): `AUTO-GENERATED by
     ta_codegen — DO NOT EDIT`.
  6. **Build lists** kept per-dir (generator-owned): `CMakeLists.txt` `LIB_SOURCES` +
     `src/ta_func/Makefile.am` (161 + `ta_utility.c`, no NVI/PVI). No option-2/configure.ac change.
  7. **`abi_gate.py`** now nm-diffs the CMake `cmake-build/libta-lib.a` (built from `src/`) vs the
     frozen worktree archive — no source recompile.
- **Checkpoint:** ✅ ALL PASSED — `generate` regen deterministic (stable change set; `src/ta_func`
  diff is the generated content, reviewable via `git diff`); CMake **and** autotools both build the
  shipped lib from `src/` (per-dir recursion works — real dir); plain `ta_regtest` absolute oracle
  passes (static-linked); `--codegen` **161/0 all 4 langs** vs the frozen `ta_ref_serve` (servers
  rebuilt from the cutover sources, 0 stderr errors); ABI gate green (0 public breaks, `include/`
  frozen); generator `cargo test` (445) + clippy clean. Extractor tests read the frozen reference
  (`../ta-lib-ref/src/ta_func`), skip if absent.
- **Rollback:** `git checkout HEAD -- src/ta_func CMakeLists.txt` + restore `output/c/{ta_func,
  ta_common}` + revert the generator edits (frozen reference preserved at the tag).

### Stage 5-B / #3 — Generate `ta_abstract` into `src/ta_abstract`  ✅ DONE 2026-06-29
The abstract generator (`backends/ta_abstract_c.rs`) **already builds everything from the IR**
(`gen_ta_func_api_c` reads the IR-generated `ta_func_api.xml`, not the reference) — so it was
*not* circular, and the untangle was a clean repoint, not an ownership rewrite.
- **Done:**
  1. `ta_abstract_c::generate` now writes its `base` to `<root>/src/ta_abstract` (was
     `output/c/ta_abstract`). It overwrites the generated-equivalent files (`ta_abstract.c`,
     `ta_def_ui.{c,h}`, `ta_frame_priv.h`, `frames/ta_frame.{c,h}`, `tables/table_a-z.c`,
     `ta_group_idx.c`, `ta_func_api.c`) in place and leaves the gen_code-only files
     (`ta_java_defs.h`, `templates/`) untouched (they go with gen_code in Stage 7).
  2. `output/c/ta_abstract` removed — **`output/c` is now purely codegen artifacts.**
  3. The C server build's `-I` points `ta_abstract`/`frames` at `src/ta_abstract`.
  4. **gen_code coexistence fix:** the autotools `-DTA_GEN_CODE` "gc" abstract lib
     (`libta_abstract_gc.la`, linked by gen_code) compiles the same sources with `-DTA_GEN_CODE`.
     The generated `ta_abstract.c` was guarding the `TA_PerGroupFuncDef`/`TA_PerGroupSize` extern
     **decls** under `#ifndef TA_GEN_CODE` while its `ta_group_idx.c` *defines* them
     unconditionally (and the gc lib compiles `ta_group_idx.c`) → "undeclared" under
     `-DTA_GEN_CODE`. Fixed `gen_ta_abstract_c` to declare them unconditionally.
- **Checkpoint:** ✅ CMake + autotools both build the shipped lib (with the generated abstract);
  **gen_code still compiles** (gc variant green); absolute oracle passes; `--codegen` 161/0 all 4
  langs (servers use the generated abstract); ABI gate green (0 public breaks); regen deterministic;
  `cargo test` (445) + clippy clean. `ta_common` confirmed all hand-written — stays in `src/`.
- **Rollback:** `git checkout HEAD -- src/ta_abstract` + restore `output/c/ta_abstract` + revert
  the generator edits.
- **Note:** `ta_codegen` is now the canonical writer of the whole C library + the abstract layer
  (`include/ta_func.h`, `ta_func_api.xml`, `ta_func_list.txt` were already its outputs) — i.e.
  prerequisite **#3 (ta_abstract ownership) is effectively complete.** Remaining gen_code roles
  after this stage: `ta_defs.h`/`ta_retcode.c` (→ Stage 5-C, since DONE) and the Java wrappers
  (→ #4); gen_code itself goes in Stage 7.

### Stage 5-C — Take over gen_code's last two C-side scalar generators  ✅ DONE 2026-06-29
With the indicators (4-B) and `ta_abstract` (5-B) cut over, gen_code still uniquely generated two
small committed C-side files. `ta_codegen` now generates both, reproduced **byte-identical** to the
committed gen_code output (the cleanest possible oracle: regenerate → `git diff` is empty).
- **`include/ta_defs.h` — the `FuncUnstId` enum (GENCODE SECTION 1).** New `backends/ta_defs.rs`
  marker-splices just SECTION 1 (the rest of the hand-written header is untouched). The
  unstable-period ID list is a new `FuncUnstId:` entry in `ta_codegen/input/enums.yaml` (the YAML
  enum parser was already generic — no parser change); the `TA_FUNC_UNST_ALL`/`_NONE` sentinels are
  structural and appended by the generator.
- **`src/ta_common/ta_retcode.c` — the `TA_SetRetCodeInfo` table.** New `backends/retcode.rs` builds
  `retCodeInfoTable[]` from the shipped source `src/ta_common/ta_retcode.csv` (the csv is INPUT, not
  generated — only gen_code ever read it) + a static-scaffolding template
  `ta_codegen/generator/templates/c/ta_retcode.c.template`; the `0xFFFF`/`TA_UNKNOWN_ERR` sentinel is appended
  as the final comma-less entry. **Corrects an earlier runbook claim:** `ta_common` was NOT "all
  hand-written" — `ta_retcode.c` was gen_code-generated (only `ta_retcode.csv` is the hand-source).
- Both wired into `main.rs`'s `if backends_to_run.contains(&"c")` block.
- **Checkpoint:** ✅ regen → `ta_defs.h` + `ta_retcode.c` byte-identical (`write_if_changed` reports
  "up to date"); generator `cargo test` (445) + clippy clean; full pipeline (build + `--codegen`
  161/0 all 4 langs + ABI gate green) — trivially so, since the shipped artifacts are byte-for-byte
  unchanged. Regen deterministic (the CMake-fix below survives regen; `ta_defs.h`/`ta_retcode.c` stay
  byte-identical).
- **Also fixed — a pre-existing 5-B CMake-gen_code regression (NOT caused by 5-C).** `build.py
  gen_code` (the CMake path) had been failing since the abstract/indicator source lists moved into
  the ta_codegen-generated `LIB_SOURCES` block: CMake's gen_code target recompiles sources with
  `-DTA_GEN_CODE` from `COMMON_SOURCES + IDL_SOURCES`, which no longer included `ta_group_idx.c`,
  `ta_func_api.c`, `frames/ta_frame.c`, or the indicators → undefined `TA_PerGroupSize` /
  `TA_PerGroupFuncDef` / `TA_*`. (Autotools was unaffected — it links a separate `libta_abstract_gc`
  + `libta_func`.) Fix: `GEN_CODE_SOURCES = gen_code.c + ${LIB_SOURCES}` (the whole lib, compiled
  `-DTA_GEN_CODE` — safe: only `ta_abstract` carries `TA_GEN_CODE` guards) + link `m` (libm). gen_code
  now builds via CMake **and** autotools. (The whole gen_code CMake block is removed in Stage 7.)
- **Rollback:** `git checkout HEAD -- include/ta_defs.h src/ta_common/ta_retcode.c` + revert the
  generator edits (`backends/{ta_defs,retcode}.rs`, `mod.rs`, `main.rs`, `enums.yaml`, the template).
- **Note:** with this, **gen_code's entire C side is retired** — its only remaining unique role is
  the Java wrappers (#4). The `.NET`/MSVC generation is dead surface (shipped .NET deleted
  2026-03-19), deleted *with* gen_code in Stage 7, not a separate cutover.

### Stage 6-B — Packaging (no deref needed)
- **Do:** `src/` already ships real files, so **no symlink dereference**. Just ensure the
  tarball includes the generated `src/` files + `ta_retcode.csv` + internal headers the PHP
  `trader` glob needs, and disable the `gen_code_pass` zero-diff guard in `scripts/package.py`.
  **Also push the `reference-pre-cutover` tag** if CI is to run `--codegen` (it needs the tag to
  build `ta_ref_serve`); else the `regtest.py` fallback covers local dev.
- **Checkpoint:** built tarball builds (CMake + autotools) and `trader`'s `src/ta_func/*.c` glob resolves.
- **Rollback:** revert the `package.py` edits.

### Stage 7 — Remove gen_code  ✅ DONE 2026-06-30 (commit `5a0d7a48` on `dev`)
Also made the C build C-only: removed the `ta_codegen_bin/generate/servers` cargo targets from CMake
(CMake never invokes cargo); `scripts/build.py` now builds ta_codegen via cargo directly. See the
status header at the top for the full summary + verification.

- **Do:** with `ta_codegen` now writing `src/ta_func`/`src/ta_abstract` directly, delete
  `src/tools/gen_code/`, `src/ta_abstract/templates/`, `ta_java_defs.h`, and the dead .NET/MSVC
  surface. Update `CLAUDE.md` to the single-generator architecture.
- **Checkpoint:** full build + `ta_regtest` 161/0; packaging works.
- **Rollback:** `git revert` (gen_code remains in history + the pinned tag).

### Stage 8 — Retire the reference (much later, optional)
- Keep the pinned tag + worktree as the diff baseline for a few release cycles. The
  reference can later be archived; the **tag** preserves it permanently, so this is never a
  one-way door.

---

## Master rollback

At any point before merge: the branch can be abandoned entirely (`git checkout dev`), and
the `reference-pre-cutover` tag + history fully preserve the pre-cutover state. After merge:
each stage was a separate commit, individually `git revert`-able.

---

## Decisions locked in

- `include/` stays in place (public ABI contract). — maintainer, 2026-06-27
- ~~`src/{ta_func,ta_common,ta_abstract}` become symlinks to `ta_codegen/output/c/*`.~~
  **SUPERSEDED 2026-06-29:** `ta_codegen` generates the C library **directly into `src/`**
  (option B); `ta_codegen/output/c` keeps only codegen-specific artifacts. No symlinks — they
  cost real complexity (autotools libtool breakage, packaging deref, circular copy-from-`src`)
  and bought nothing. The C lib is the *product* (lives in `src/`, like gen_code wrote it); the
  Rust/Java/.NET *bindings* stay fully under `ta_codegen/output/<lang>`. — maintainer, 2026-06-29
- Reference lives at a **pinned git tag** (+ on-demand worktree), not a separate fork repo. — 2026-06-27
- `ta_regtest` uses **reference-as-server** (no in-process reference) to stay non-circular. — 2026-06-27
- Java remains a shipped deliverable. — 2026-06-27
