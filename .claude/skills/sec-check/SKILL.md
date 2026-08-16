---
name: sec-check
description: On-demand semantic security review of one PR before merge — reads the diff and judges intent, rather than just pattern-matching. Blocks and asks for human review on any suspicion instead of trying to resolve it. Use when asked to security-check, vet, or judge whether a PR is safe to merge. Triggers on "sec-check", "security check this PR", "vet this PR", "is this PR safe to merge".
---

# /sec-check — Pre-merge Security Review

## Usage

- `/sec-check 123` — PR number
- `/sec-check https://github.com/TA-Lib/ta-lib/pull/123` — PR link

`<PR>` accepts either form; extract the number if a link is given.

## Why this exists, and what it is not

`.github/workflows/pr-security-gate.yml` already runs two free, deterministic
checks on **every** PR automatically: a binary-file check and a Betterleaks
secret scan. Those catch "does this diff contain a flagged pattern." They
cannot catch "does this code do something malicious that reads perfectly
innocent." That second question needs judgment, and judgment costs API
tokens — so unlike the free gates, this skill runs **on demand, per PR, at
your discretion**, not automatically on every PR. That split is deliberate:
see the CI workflow's own header comment for why an always-on paid reviewer
was rejected (unbounded per-PR cost, no natural rate limit on a public repo).

This skill **reports and recommends** — it does not merge, approve, or
request changes on GitHub itself, and it does not approve any pending
deployment/environment review for other workflows. If a future refinement
wires those actions in, they must stay conditioned on this skill's own
verdict, never on "a run happened" — an agent that auto-approves whatever it
runs has quietly rebuilt the always-on gate this was built to avoid.

## Core rule

**Never clear a PR that raises any suspicion.** On doubt, stop and hand it to
a human — do not try to resolve, explain away, or auto-fix the concern
yourself. Silence/uncertainty defaults to BLOCK, not PASS. This is the
opposite autonomy stance from a perf-iteration loop: there, keep going and
log open questions; here, stopping IS the correct output.

## Steps

1. **Resolve the target.** Parse `<PR>` to a bare number.

2. **Gather context** — don't skip this, a diff without context is easy to
   misjudge:
   ```bash
   gh pr view <PR> --json number,title,body,author,baseRefName,headRefName,isCrossRepository,files,additions,deletions,changedFiles
   gh pr diff <PR>
   gh pr checks <PR>
   ```
   - `isCrossRepository: true` → fork PR, unknown/untrusted author → default
     to more scrutiny, not less.
   - If `binary-check` or `secret-scan` in `gh pr checks` is failing or
     hasn't completed, **stop here** — that is already a hard signal, and
     re-deriving what those gates already check is wasted work, not extra
     rigor.

3. **Read the diff against these lenses** (tailored to this repo — generic
   "look for eval()" checklists miss what's actually sensitive here):
   - `.github/workflows/**` — new/changed secret usage, any drift toward
     `pull_request_target`, permission escalation, new or re-pinned
     third-party actions/Docker images (especially anything moving *away*
     from a pinned digest/tag).
   - `ta_codegen/generator/**` — this is the **single** generator for every
     backend (C/Rust/Java/C#). A subtle change here poisons all four at
     once; scrutinize it harder than an equivalent change to one backend's
     generated output.
   - Build-system changes (`CMakeLists.txt`, `Makefile.am`, `Cargo.toml`,
     `pom.xml`, `*.csproj`, `package.json`) — new dependencies, changed
     source lists, altered compiler flags. `CMakeLists.txt` carries specific
     load-bearing flags (`-ffp-contract=off`, `-fno-math-errno`) — a change
     to either is a correctness/perf question for a human, not a rubber
     stamp either way.
   - Anything high-entropy or binary-shaped that the free gates wouldn't
     catch: a base64/hex blob embedded in otherwise-plain source, code no
     one could read and understand at a glance, unexplained obfuscation.
   - New outbound network calls, new filesystem writes outside expected
     paths, new subprocess/exec calls — anywhere in the diff, not just in
     workflow files.
   - Whether the diff actually matches the PR's stated title/description.
     A mismatch is itself a signal, independent of what the mismatched code
     does.

4. **Form a verdict.**
   - **CLEAR** — state plainly what was checked and why nothing raised
     concern. Still frame this as informative, not a merge authorization —
     this skill is one input, not a replacement for the human's own read.
   - **SUSPICIOUS** — stop. List exactly what raised the flag with
     `file:line` references and *why* it's suspicious (not just "this looks
     unusual"). State explicitly: **do not merge, do not approve any
     pending workflow run for this PR**, until a human has looked. Do not
     attempt to resolve it in the same turn.

## Where the verdict goes

Report the verdict in the conversation only — that's the "agent console" the
person driving this session is watching, and that's where a pre-merge check
belongs. Never write that a sec-check happened, or its result, into a commit
message, PR description/review, or a GitHub issue comment. It isn't part of
the durable record of *what changed* or *why* — it's a gate someone watched
pass before merging, and restating that in text other people read later is
noise, not signal. If the same turn also produces a commit or a posted
comment (for unrelated reasons), that commit/comment must stand on its own,
with no mention that a sec-check ran or what it found.

## Refining this skill

First version — expect to tune it against real PRs rather than treat this
list as fixed:

- No persistent memory of past verdicts yet (e.g., "this contributor's prior
  3 PRs were clean") — every run starts cold.
- The lens list above is a starting set, not exhaustive — add to it when a
  real PR exposes a category it missed.
- Whether a SUSPICIOUS verdict should also post a `gh pr review
  --request-changes` (a real, visible GitHub action, not just a local
  report) is an open decision — deliberately left out of v1 so this skill
  stays purely advisory until that tradeoff is made on purpose.
