# Security Policy

## Reporting a Vulnerability

Report it privately through GitHub:
[Report a vulnerability](https://github.com/TA-Lib/ta-lib/security/advisories/new).
Only you and the TA-Lib maintainers can see the report, and the discussion and
fix stay private until an advisory is published.

Please do not open a public issue for a vulnerability.

If you cannot use GitHub, message one of the maintainers listed at
https://ta-lib.org/about/ privately, for example by
[Discord](https://discord.gg/Erb6SwsVbH) DM.

## What to expect

- A first reply within 14 days.
- If the report is confirmed, the fix ships as a patch release, and a GitHub
  security advisory is published with it. The advisory credits you unless you ask
  not to be named.
- The target is a released fix within 90 days of the report. If that cannot be
  met, the disclosure date is agreed with you before anything is made public.

## Scope

In scope: memory-safety faults (an out-of-bounds read or write, a crash, a hang)
reachable through the public API of the C library or of the Rust, Java, C#,
TypeScript and Go packages built from this repository, with arguments the API
documents as valid.

A wrong value from an indicator is a bug, not a vulnerability: open a normal
[issue](https://github.com/TA-Lib/ta-lib/issues).

## Supported versions

Fixes are made on the latest release. Earlier releases are not patched.
