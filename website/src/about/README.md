---
title: About Us
---

# About Us

TA-Lib has been built and refined by an open community of traders, engineers and
researchers for more than two decades. This page credits the people whose work is
recorded across the source tree and the wider ecosystem.

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
| **Paweł Konieczny** | Early contributor to `ta_regtest`, the build system and the now-defunct `ta_data` feature. |
| **Alexander Trufanov** — [@trufanov-nok](https://github.com/trufanov-nok) | Numerical-robustness improvements and streaming-API inspiration. |

### Language wrappers

TA-Lib reaches many languages thanks to the maintainers of these wrappers:

| Maintainer | Wrapper |
| --- | --- |
| **John Benediktsson** — [@mrjbq7](https://github.com/mrjbq7) | Python — [ta-lib-python](https://github.com/TA-Lib/ta-lib-python) · Zig — [ta-lib-zig](https://github.com/TA-Lib/ta-lib-zig) |
| **Brad Peabody** — [@bradleypeabody](https://github.com/bradleypeabody) | Go — [ta-lib-cgo](https://github.com/TA-Lib/ta-lib-cgo) |
| **Rafael Ernesto Espinosa Santiesteban** — [@rernesto](https://github.com/rernesto) | PHP — [ext-ta-lib](https://github.com/TA-Lib/ext-ta-lib) |
| **Marcelo Teixeira Monteiro** — [@tuxmonteiro](https://github.com/tuxmonteiro) | PostgreSQL — [ta_pg](https://github.com/TA-Lib/ta_pg) |
| **Victor Yang** — [@Youngv](https://github.com/Youngv) | Ruby — [ta-lib-ruby](https://github.com/TA-Lib/ta-lib-ruby) |
| **Serkan Korkmaz** — [@serkor1](https://github.com/serkor1) | R — [ta-lib-R](https://github.com/serkor1/ta-lib-R) |
| **Kevin Johnson** — [@twopirllc](https://github.com/twopirllc) | pandas — [pandas-ta](https://github.com/twopirllc/pandas-ta) |

## Other Code Contributors

Everyone else whose fixes, optimizations and new functions are recorded in the TA-Lib
source tree.

| Contributor | Contribution |
| --- | --- |
| **Adrian Michel** — [amichel.com](http://amichel.com) | Fixed the missing `atan()` in `LINEARREG_ANGLE`. |
| **Anatoliy Belsky** | Initial implementations of `IMI` and `AVGDEV`. |
| **Andrew Atkinson** | Lookback, out-of-bounds and null-pointer fixes in `APO`, `PPO`, `STOCHRSI` and `TRIX`. |
| **[@CaptainTrunky](https://github.com/CaptainTrunky)** | Conan package-manager support ([#86](https://github.com/TA-Lib/ta-lib/issues/86)). |
| **Chris** — crokusek@hotmail.com | Reported the `TRIMA` range-handling bug. |
| **Christo Fogelberg** | `period = 1` correctness fixes across `SAR`, `PLUS_DI`/`PLUS_DM` and `MINUS_DI`. |
| **Christopher Barnhouse** | Fixed an out-of-bounds write in `CDLTRISTAR`. |
| **Drew McCormack** — [trade-strategist.com](http://www.trade-strategist.com) | Initial implementation of `ULTOSC`. |
| **echo999** — echo999@ifrance.com | Found and fixed a `STOCHF` `outFastD` bug. |
| **guycom** | Reported the `ADX` divide-by-zero bug. |
| **jdoyle** | Fixed start/end range handling in `AD`. |
| **Jesus Viver** | Speed optimizations for `STDDEV`, `VAR`, `MIN` and `MAX`. |
| **John Price** — jp_talib@gcfl.net | Initial implementation of the `LINEARREG` family. |
| **JP Pienaar** | Fixed the `MACD` period-swap logic. |
| **Matthew Lindblom** | Implementation of `T3`. |
| **Michael Williamson** | Initial implementation of `BETA`. |
| **Mirek Fontan** — [fontan.cz](http://fontan.cz) | Reported a divide-by-zero bug. |
| **[@mw66](https://github.com/mw66)** | Removed the spurious unstable period from `MFI`. |
| **Peter Pudaite** | Initial `STOCHRSI`; reworked the `SAREXT` parameters. |
| **Robert Meier** | Initial implementation of `ACCBANDS`. |
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
| **Kenneth Jorgensen** — [@kennethjor](https://github.com/kennethjor) | Documentation index updates ([#70](https://github.com/TA-Lib/ta-lib/issues/70)). |
| **[@Lqingyu](https://github.com/Lqingyu)** | Out-of-bounds fix in the regression-test tooling ([#61](https://github.com/TA-Lib/ta-lib/issues/61)). |
| **Thorsten Alteholz** — [@alteholz](https://github.com/alteholz) | Typo fix in the `TA_LIB_NOT_INITIALIZE` return-code message ([#68](https://github.com/TA-Lib/ta-lib/issues/68)). |

</div>

---

Want to help? See [How to get support](/faq/) or open an issue on
[GitHub](https://github.com/TA-Lib/ta-lib/issues).
