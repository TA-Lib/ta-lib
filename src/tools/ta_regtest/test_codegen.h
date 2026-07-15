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

/* ---- Shared "in-process C <=> language server, bit-for-bit" core (issue #115).
 * One operation with a pluggable input source: --xlang-hash feeds a seed;
 * server_verify feeds the hard-coded test's exact arrays (lossless hex-bits).
 * Both hash the C outputs (codegen_output_hash), request the server's out_hash,
 * and diff via codegen_hash_compare. ---- */

/* Verdict of comparing one server's out_hash response to the in-process C golden
 * on identical inputs. */
typedef enum {
    XHASH_MATCH = 0,   /* bit-identical (or matching non-Success retCode) */
    XHASH_NO_HASH,     /* response carried no out_hash (server lacks want_hash) */
    XHASH_RETCODE,     /* retCode differs */
    XHASH_SHAPE,       /* outBegIdx / outNBElement differs */
    XHASH_BITS         /* output bytes differ (bitwise divergence) */
} XHashVerdict;

/* Server-side values parsed from the response, for the caller's diagnostic. */
typedef struct { int rc, begIdx, nbElement; unsigned long long hash; } XHashParsed;

/* Golden output hash: FNV-1a (fuzz_hash_*) over `nb` elements of each output in
 * logical order — reals as f64 bytes, integers as i32 bytes — byte-identical to
 * every server's out_hash. outIsInteger[o] selects the type; outBufs[o] is the
 * matching TA_Real* / TA_Integer* buffer. nb==0 hashes nothing (the FNV basis),
 * matching an empty server output. */
unsigned long long codegen_output_hash(unsigned int nbOutput,
                                        const int *outIsInteger,
                                        const void *const *outBufs, int nb);

/* Pure parse+compare of a server's hash-mode response vs the in-process C golden.
 * Fills *parsed (server side) for diagnostics. Returns XHASH_MATCH iff the server
 * is bit-identical to C. No I/O — the caller does the send/reporting prefix. */
XHashVerdict codegen_hash_compare(const char *resp,
                                  TA_RetCode goldRc, int goldBeg, int goldNb,
                                  unsigned long long goldHash, XHashParsed *parsed);

/* Shared diagnostic tail: prints the common
 *   "retCode a/b  begIdx c/d  nbElem e/f  hash G/H (golden/<who>)" line. */
void codegen_hash_report(const char *who, TA_RetCode goldRc, int goldBeg,
                         int goldNb, unsigned long long goldHash,
                         const XHashParsed *parsed);

#endif
