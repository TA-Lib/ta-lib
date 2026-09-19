# C Publish Runbook

Tagging a version, publishing the GitHub release and its packages, and updating
the downstream ports.

The C release goes first. Every language binding then publishes the same version
from its own runbook, and `post-publish-runbook.md` closes the round. `README.md`
in this directory holds the order.

## Cutting the release

Any dev with permission to merge to main branch can do a release.

(CP1) On the dev branch, edit the VERSION file in the root of the repos.

(CP2) Run "./scripts/sync.py". This ensures your dev branch is up-to-date (among other things).

(CP3) Push to the dev branch.

(CP4) Manually trig the "nightly dev" Github action. This will regenerate and test for **all** platforms. If you do not trig it, it will get run anyway once per day.

(CP5) Merge dev into main with "./scripts/merge.py". At this point, the main branch is the release candidate with all the assets under "dist" folder.

(CP6) Manually trig the "nightly main" Github action. This will perform a last round of check prior to alloweing for the release. If you do not trig it, it will get run anyway once per day.

(CP7) Manually trig the "Release (step-1)" Github action on main branch. This will tag, generate a draft release and attach all assets from the dist/ directory.

(CP8) Optionally edit the draft "Release notes" on the Github website. A good time to add thank you to contributors. You can still edit after the official release.

(CP9) Manually trig "Release (step 2)" Github action. This will make the release official/public.

(CP10) Verify the Github release page shows the new version with all assets attached and downloadable. The website (https://ta-lib.org/install) still shows the previous release until `post-publish-runbook.md` is done.

(CP11) Update the vcpkg port with "./scripts/post-release-vcpkg.py" from Linux or WSL (needs `gh auth login`), one stage at a time:
- `plan` (optional) prints the version, asset URL and SHA512. Read-only.
- `prepare` branches `ta-lib-<ver>` from microsoft/vcpkg master in a local checkout (`--vcpkg-root`, default `temp/post-release-vcpkg/vcpkg`) and bumps the version + SHA512. Nothing is pushed. Now review the port: delete any patch that no longer applies, and fix anything a version bump cannot.
- Run the local gate `prepare` prints. Each run must print "Building talib:<triplet>@<ver>" and "All feature tests passed", and exit 0. `./vcpkg install talib` is not a gate: its post-build lint only warns.
- `submit` runs x-add-version and commits. After a confirmation it syncs your fork's master (create the fork once with `gh repo fork microsoft/vcpkg --clone=false`), pushes to it, opens a draft microsoft/vcpkg PR, and opens a "[monitor] VCPkg release <ver>" issue here.
- Mark the PR ready only once CI is green and the Azure "file lists for <triplet>" artifacts show talib was actually built. Review can still send it back, and each round costs days. Close the monitor issue once "vcpkg install talib" installs the new version.

(CP12) Monitor homebrew-core. The formula is updated within about an hour:
https://github.com/Homebrew/homebrew-core/blob/HEAD/Formula/t/ta-lib.rb

(CP13) If FMA_TRANSITION_TOLERANCE is still 1 in "src/tools/ta_regtest/test_codegen.c", re-freeze the bit-exact oracle onto the tag just published; the RE-FREEZE note beside that macro has the steps. Until it is done, "--fuzz-064" compares at 1e-9 instead of hash-exact.
