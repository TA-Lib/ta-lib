# Release Runbooks

One release round, in this order. Any dev with permission to merge to main can
run it.

1. `c-publish-runbook.md`
2. The language generated backends, each to its own registry:
   - `java-publish-runbook.md`
   - `rust-publish-runbook.md`
   - C#, not published yet
3. `post-publish-runbook.md`

About backends at rank 2: each waits for the C publish of the same version,
none waits for another.

**The order is load-bearing, in one direction.** Every backend takes its version
from `VERSION`, and the last runbook bumps it.
