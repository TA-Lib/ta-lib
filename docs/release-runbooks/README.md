# Release Runbooks

One release round, in this order. Any dev with permission to merge to main can
run it.

1. `c-publish-runbook.md`
2. The language generated backends, each to its own registry:
   - `java-publish-runbook.md`
   - `rust-publish-runbook.md`
   - `csharp-publish-runbook.md`
   - `typescript-publish-runbook.md`
   - `go-publish-runbook.md`
3. `post-publish-runbook.md`

About backends at rank 2: each waits for the C publish of the same version,
none waits for another. A Go release candidate waits for nothing.

**The order is load-bearing, in one direction.** Every backend takes its version
from `VERSION`, except Go, whose version is the release workflow's input and, for a
final version, names the published C release; the last runbook bumps `VERSION`.
