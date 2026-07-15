---
title: About Us
# This page is a dense list of contributor/wrapper GitHub links; the external-link
# arrow on every one is distracting. Disable it here only — other pages keep it.
externalLinkIcon: false
---

# About Us

TA-Lib has been built and refined by an open community of traders, engineers and researchers for more than two decades.

This page credits the people whose work is recorded across the source tree and the wider ecosystem.

Join us on [Discord](https://discord.com/invite/Erb6SwsVbH)!

<div class="credits">

## TA-Lib Administrators

| Administrator | Role |
| --- | --- |
| **Mario Fortier** — [@mario4tier](https://github.com/mario4tier) | Founder and lead maintainer. Original author of the C/C++ core. |
| **John Benediktsson** — [@mrjbq7](https://github.com/mrjbq7) | Author and lead maintainer. Created the Python ([ta-lib-python](https://github.com/TA-Lib/ta-lib-python)) and Zig ([ta-lib-zig](https://github.com/TA-Lib/ta-lib-zig)) wrappers; `ta-lib-python` is by far the most widely used TA-Lib binding. |

## Feature Contributors

The people behind some of TA-Lib's major features.

| Contributor | Contribution |
| --- | --- |
| **Angelo Ciceri** | Candlestick pattern recognition — the complete family of `CDL*` functions. |
| **Chad Furman** — [@chadfurman](https://github.com/chadfurman) | Native Rust port and co-designer of `ta_codegen`, which now automates maintenance of every backend (C, Rust, Java, .NET, docs, streaming API) from a single source of truth. |
| **Barry Tsung** | Early Java `Core`: candle-settings initialization, compatibility and unstable-period APIs. |
| **Richard Gomes** | Java abstract/reflection layer — the `meta` package: annotation-based RTTI and late-bound (dynamic) TA-function invocation. |
| **Paweł Konieczny** | Early contributor to `ta_regtest`, the build system and the now-defunct `ta_data` feature. |
| **Alexander Trufanov** — [@trufanov-nok](https://github.com/trufanov-nok) | Numerical-robustness improvements and streaming-API inspiration. |

### Language wrappers {#wrappers}

TA-Lib reaches many languages thanks to the maintainers of these wrappers:

| Maintainer | Wrapper |
| --- | --- |
| **John Benediktsson** — [@mrjbq7](https://github.com/mrjbq7) | Python — [ta-lib-python](https://github.com/TA-Lib/ta-lib-python) · Zig — [ta-lib-zig](https://github.com/TA-Lib/ta-lib-zig) |
| **Brad Peabody** — [@bradleypeabody](https://github.com/bradleypeabody) | Go — [ta-lib-cgo](https://github.com/TA-Lib/ta-lib-cgo) |
| **Rafael Ernesto Espinosa Santiesteban** — [@rernesto](https://github.com/rernesto) | PHP — [ext-ta-lib](https://github.com/TA-Lib/ext-ta-lib) |
| **Marcelo Teixeira Monteiro** — [@tuxmonteiro](https://github.com/tuxmonteiro) | PostgreSQL — [ta_pg](https://github.com/TA-Lib/ta_pg) |
| **Victor Yang** — [@Youngv](https://github.com/Youngv) | Ruby — [ta-lib-ruby](https://github.com/TA-Lib/ta-lib-ruby) |
| **Serkan Korkmaz** — [@serkor1](https://github.com/serkor1) | R — [ta-lib-R](https://github.com/serkor1/ta-lib-R) |
| **Kevin Johnson** — [@twopirllc](https://github.com/twopirllc) (original author) · **[@xgboosted](https://github.com/xgboosted)** (current maintainer) | pandas — [pandas-ta-classic](https://github.com/xgboosted/pandas-ta-classic) |

## Other Code Contributors

Everyone else whose fixes, optimizations and new functions are recorded in the TA-Lib
source tree.

| Contributor | Contribution |
| --- | --- |
| **Adrian Michel** — [amichel.com](http://amichel.com) | Fixed the missing `atan()` in `LINEARREG_ANGLE`. |
| **Anatoliy Belsky** | Initial implementations of `IMI` and `AVGDEV`. |
| **Anatoliy Siryi** — [@hmG3](https://github.com/hmG3) | Visual Studio 2022 build support ([#34](https://github.com/TA-Lib/ta-lib/pull/34)). |
| **Andrew Atkinson** | Lookback, out-of-bounds and null-pointer fixes in `APO`, `PPO`, `STOCHRSI` and `TRIX`. |
| **[@CaptainTrunky](https://github.com/CaptainTrunky)** | Conan package-manager support ([#86](https://github.com/TA-Lib/ta-lib/issues/86)). |
| **Chris** — crokusek@hotmail.com | Reported the `TRIMA` range-handling bug. |
| **Christo Fogelberg** | `period = 1` correctness fixes across `SAR`, `PLUS_DI`/`PLUS_DM` and `MINUS_DI`. |
| **Christopher Barnhouse** | Fixed an out-of-bounds write in `CDLTRISTAR`. |
| **Drew McCormack** — [trade-strategist.com](http://www.trade-strategist.com) | Initial implementation of `ULTOSC`. |
| **echo999** — echo999@ifrance.com | Found and fixed a `STOCHF` `outFastD` bug. |
| **Fernando José** — [@iglesias](https://github.com/iglesias) | Fixed a floating-point multiplication overflow ([#33](https://github.com/TA-Lib/ta-lib/pull/33)). |
| **[@greenTableWork](https://github.com/greenTableWork)** | Helper script for the vcpkg post-release PR workflow ([#82](https://github.com/TA-Lib/ta-lib/pull/82)). |
| **guycom** | Reported the `ADX` divide-by-zero bug. |
| **[@halohsu](https://github.com/halohsu)** | macOS libtool build support ([#39](https://github.com/TA-Lib/ta-lib/pull/39)). |
| **jdoyle** | Fixed start/end range handling in `AD`. |
| **Jesus Viver** | Speed optimizations for `STDDEV`, `VAR`, `MIN` and `MAX`. |
| **John Price** — jp_talib@gcfl.net | Initial implementation of the `LINEARREG` family. |
| **JP Pienaar** | Fixed the `MACD` period-swap logic. |
| **Kenneth Jorgensen** — [@kennethjor](https://github.com/kennethjor) | Documentation index (`index.md`) updates ([#70](https://github.com/TA-Lib/ta-lib/pull/70)). |
| **[@Lqingyu](https://github.com/Lqingyu)** | Out-of-bounds fix in the regression-test tooling ([#62](https://github.com/TA-Lib/ta-lib/pull/62)). |
| **Major Hayden** — [@major](https://github.com/major) | Added the BSD 3-Clause LICENSE file ([#38](https://github.com/TA-Lib/ta-lib/pull/38)). |
| **Matthew Lindblom** | Implementation of `T3`. |
| **Michael Williamson** | Initial implementation of `BETA`. |
| **Mikhail Smirnov** — [@cpp4ever](https://github.com/cpp4ever) | CMake build-script fix ([#9](https://github.com/TA-Lib/ta-lib/pull/9)). |
| **Mirek Fontan** — [fontan.cz](http://fontan.cz) | Reported a divide-by-zero bug. |
| **[@mw66](https://github.com/mw66)** | Removed the spurious unstable period from `MFI`. |
| **[@nehemiah888](https://github.com/nehemiah888)** | Added function documentation, including Chinese translations ([#75](https://github.com/TA-Lib/ta-lib/pull/75)). |
| **Peter Pudaite** | Initial `STOCHRSI`; reworked the `SAREXT` parameters. |
| **Robert Meier** | Initial implementation of `ACCBANDS`. |
| **Rowe Wilson Frederisk Holme** — [@Frederisk](https://github.com/Frederisk) | Fixed the `CdlStickSandwich` name typos ([#28](https://github.com/TA-Lib/ta-lib/pull/28)). |
| **Thorsten Alteholz** — [@alteholz](https://github.com/alteholz) | Spelling fixes across the library, including the `TA_LIB_NOT_INITIALIZE` return-code message ([#68](https://github.com/TA-Lib/ta-lib/pull/68)). |
| **Vikas N Kumar** — [@vikasnkumar](https://github.com/vikasnkumar) | Debian 11 / Ubuntu 22.04 support with a lower minimum CMake version ([#51](https://github.com/TA-Lib/ta-lib/pull/51)). |
| **Vincent Bernardoff** — [@vbmithr](https://github.com/vbmithr) | Linux / autotools build fixes ([#1](https://github.com/TA-Lib/ta-lib/pull/1)). |
| **wony** — [@wony-zheng](https://github.com/wony-zheng) | Removed the spurious unstable period from `IMI`. |

## Issue Reporters

Community members whose bug reports and requests led to a committed change — with our thanks.

| Reporter | Issue |
| --- | --- |
| **[@731315163](https://github.com/731315163)** | `HT_TRENDLINE` internal-buffer investigation ([#88](https://github.com/TA-Lib/ta-lib/issues/88)). |
| **Anton Danylchenko** — [@1st](https://github.com/1st) | Proposed the GitHub Action, now [setup-ta-lib](https://github.com/TA-Lib/setup-ta-lib) ([#79](https://github.com/TA-Lib/ta-lib/issues/79)). |
| **Benjamin Leff** — [@BwL1289](https://github.com/BwL1289) | CMake `libm` linkage and shared/static build options ([#77](https://github.com/TA-Lib/ta-lib/issues/77), [#78](https://github.com/TA-Lib/ta-lib/issues/78)). |
| **Cole Richardson** — [@SeeRich](https://github.com/SeeRich) | Requested the repository LICENSE file ([#35](https://github.com/TA-Lib/ta-lib/issues/35)). |
| **[@hotston](https://github.com/hotston)** | Reported MACD signal-line repainting, leading to the `period = 1` fix ([#59](https://github.com/TA-Lib/ta-lib/issues/59)). |
| **Ilia Pozdnyakov** — [@iliazeus](https://github.com/iliazeus) | Prompted a release including the small-values fix ([#31](https://github.com/TA-Lib/ta-lib/issues/31)). |
| **Jake Arkinstall** — SourceForge | `HT_TRENDLINE` internal-buffer investigation ([#88](https://github.com/TA-Lib/ta-lib/issues/88)). |

</div>

---

Want to help? See [How to get support](/faq/) or open an issue on [GitHub](https://github.com/TA-Lib/ta-lib/issues).
