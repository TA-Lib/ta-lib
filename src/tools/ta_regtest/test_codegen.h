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

/* Can this language server select a compatibility variant?
 * C still carries the deprecated TA_SetCompatibility and .NET P/Invokes into it.
 * The Rust crate does not: its mode is pinned to Default with no public setter,
 * so a Metastock leg would silently re-run the Default one — callers must skip
 * it visibly instead. Java's shipped Core lost its setter too, but the Java
 * server embeds its own Core copy whose field stays settable, so the Metastock
 * branches in the generated Java keep their bit-exact coverage.
 * Returns 1 when the mode can be switched. */
int codegen_lang_has_compatibility_api(const char *lang);

/* Bit-exact differential fuzz of the current in-process library against the
 * frozen released v0.6.4 exposed as bin/ta_064_serve. Opt-in (--fuzz-064),
 * never part of default/nightly runs. functionFilter: CSV substring filter
 * (NULL = all). Returns TA_TEST_PASS iff there is no unwaived divergence. */
ErrorNumber fuzz_ref064(const char *functionFilter);

/* Cross-language BITWISE parity gate (--xlang-hash, issue #113). Diffs each
 * generated language server against the shipped in-process C library on
 * seed-generated inputs, comparing full-precision output hashes with NO
 * tolerance. Rust crosses the boundary with a seed (gen_present); Java crosses
 * it with the lossless hex-bits transport (its server has no fuzz_gen port,
 * #114), and its transcendental-using calls fall back to a narrow tolerance
 * because fdlibm != the C libm (see codegen_call_is_transcendental below).
 * functionFilter/languageFilter: CSV filters (NULL = all). Returns TA_TEST_PASS
 * iff every server matches C (bitwise, or within tolerance where noted). */
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

/* ---- Java-transcendental tolerance path (shared by --xlang-hash + server_verify)
 * Java's fdlibm differs from the C libm by ~1 ULP on transcendentals, so a call
 * that reaches one (atan/sin/cos/exp/log/...) cannot be bit-compared against
 * Java: those calls swap the bitwise out_hash path for a narrow element-compare
 * at this tolerance (relative for |v|>1, absolute otherwise). Every other
 * language, and every non-transcendental call, stays bitwise. Measured Java
 * drift over both gates' scenarios peaks ~9.7e-15 (HT_DCPHASE), so 1e-9 keeps
 * ~5 orders of margin over fdlibm noise while still failing any real algorithmic
 * regression (orders of magnitude larger). Equals CODEGEN_EPSILON_DOUBLE. ---- */
#define CODEGEN_JAVA_TRANSCENDENTAL_TOL 1e-9

/* True if the FUNCTION name calls a transcendental C math routine directly.
 * Source-derived fixed list (ta_codegen/input grep). */
int codegen_is_transcendental(const char *name);

/* True if THIS CALL reaches a transcendental — the name test above, OR an
 * MA-dispatch function (MA/MAVP/BBANDS/MACDEXT/APO/PPO/STOCH*) whose *MAType
 * optional parameter selects TA_MAType_MAMA (which uses atan). optVals[i] is one
 * value per optInput in signature order; defaults are assumed beyond nbOpt. */
int codegen_call_is_transcendental(const TA_FuncHandle *handle,
                                   const double optVals[], int nbOpt);

/* Serialize `count` doubles as one JSON string of 16-hex-char IEEE-754 bit
 * groups (lossless, no float-parse rounding, no library) — the shared input
 * transport for both gates' C<=>server comparisons. Returns bytes written. */
int codegen_write_hexbits_array(char *buf, int buf_size,
                                const TA_Real *data, int count);

/* Verdict of a tolerance element-compare of a server's %.15g array response vs
 * the in-process C golden (the Java-transcendental path). Mirrors
 * codegen_hash_compare's retCode/shape gating; reals compared at `tol`,
 * integers exact. */
typedef enum {
    CTOL_MATCH = 0,   /* within tolerance (or matching non-Success retCode) */
    CTOL_RETCODE,     /* retCode differs */
    CTOL_SHAPE,       /* outBegIdx / outNBElement differs */
    CTOL_COUNT,       /* a returned output array's length != outNBElement */
    CTOL_VALUE        /* an element exceeds the tolerance (or finite-vs-NaN) */
} CTolVerdict;

/* First divergence, filled for the caller's diagnostic on COUNT/VALUE (rc/begIdx
 * /nbElement are always the server-side parse). */
typedef struct {
    int    rc, begIdx, nbElement; /* server side (parsed from the response) */
    int    output, element;       /* which output / element diverged */
    int    isInt;                 /* the diverging output's type */
    double cReal, sReal;          /* golden vs server value (real outputs) */
    int    cInt, sInt;            /* golden vs server value (integer outputs) */
    int    srvCount;              /* server array length (CTOL_COUNT only) */
} CTolDetail;

/* Parse a server's array-mode response and element-compare against the C golden
 * in logical output order — outBufs[o] is a TA_Real* or TA_Integer* per
 * outIsInteger[o], nb elements each. Reals at `tol` (relative for |v|>1,
 * absolute otherwise; finite-vs-NaN always fails), integers exact. Fills *detail
 * on COUNT/VALUE. No I/O. */
CTolVerdict codegen_compare_tol(const char *resp,
                                unsigned int nbOutput, const int *outIsInteger,
                                const void *const *outBufs,
                                TA_RetCode goldRc, int goldBeg, int goldNb,
                                double tol, CTolDetail *detail);

#endif
