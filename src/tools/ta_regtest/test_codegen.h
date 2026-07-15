#ifndef TEST_CODEGEN_H
#define TEST_CODEGEN_H

#include "ta_error_number.h"
#include "ta_test_priv.h"

/* Run codegen verification tests against one or more languages.
 * languageFilter: comma-separated list of languages to test (NULL = test all).
 *   Valid values: "rust", "c", "java", "dotnet"
 * functionFilter: CSV list of function names to test (NULL = test all).
 * Errors loudly if a requested language's server cannot be started.
 */
ErrorNumber test_codegen(const TA_History *history,
                         const char *languageFilter,
                         const char *functionFilter);

/* Bit-exact differential fuzz of the current in-process library against the
 * frozen released v0.6.4 exposed as bin/ta_064_serve. Opt-in (--fuzz-064),
 * never part of default/nightly runs. functionFilter: CSV substring filter
 * (NULL = all). Returns TA_TEST_PASS iff there is no unwaived divergence. */
ErrorNumber fuzz_ref064(const char *functionFilter);

/* Cross-language BITWISE parity gate (--xlang-hash, issue #113). Diffs each
 * protocol-capable language server (Rust today; Java after #114) against the
 * shipped in-process C library on seed-generated inputs, comparing full-precision
 * output hashes with NO tolerance. functionFilter/languageFilter: CSV filters
 * (NULL = all). Returns TA_TEST_PASS iff every server is bit-identical to C. */
ErrorNumber xlang_hash(const char *functionFilter, const char *languageFilter);

#endif
