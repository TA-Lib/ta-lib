# Governance

## Who decides

TA-Lib has two lead maintainers:

- **Mario Fortier** ([@mario4tier](https://github.com/mario4tier)), founder and original author of the C core.
- **John Benediktsson** ([@mrjbq7](https://github.com/mrjbq7)), author of [ta-lib-python](https://github.com/TA-Lib/ta-lib-python) and [ta-lib-zig](https://github.com/TA-Lib/ta-lib-zig).

They review and merge pull requests, decide what goes into a release, and publish
releases. There is no committee and no vote: a decision is made in the open, on the
issue or pull request it concerns, with its reasons written there. What a new
function computes (name, formula, inputs, defaults) is agreed on the
[New TA Functions board](https://github.com/orgs/TA-Lib/projects/1) before code is
written.

Everyone credited for the project is listed at <https://ta-lib.org/about/>.

## How a change lands

Anyone can open an issue or a pull request; [CONTRIBUTING.md](CONTRIBUTING.md) says
how. Pull requests target `dev`. Every pull request runs the regeneration and
security gates, and `dev` and `main` are each built and tested every night on
Linux, Windows and macOS, in every supported language.

`main` only moves forward to a `dev` commit that both nightly runs passed.

## Releases

Any maintainer who can merge to `main` can release. The steps are in
[docs/release-runbooks/](docs/release-runbooks/README.md): the C library first,
from the packages the nightly built and tested, then each language binding to
its own registry.

Versions follow `MAJOR.MINOR.PATCH`. The shared library keeps its soname until
public API that shipped is removed or changed.

Security fixes follow [SECURITY.md](SECURITY.md) and ship as a patch release.

## Continuity

- Everything needed to build, test and release is in this repository: the sources,
  the code generator, the CI workflows and the release runbooks.
- The repository and its packages belong to the [TA-Lib organization](https://github.com/TA-Lib),
  not to a personal account, and more than one person can merge to `main` and publish.
- The code is BSD licensed: anyone may fork it and continue.
- A contributor who has shown sustained, careful work may be invited to become a
  maintainer by the lead maintainers.
- If the lead maintainers step back, the organization is handed to a willing
  maintainer and the change is announced in the README. If nobody takes it over,
  the repository is archived with a note saying so.

## Changes to this document

By pull request, like any other change.
