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

#endif
