#ifndef TEST_CODEGEN_H
#define TEST_CODEGEN_H

#include "ta_error_number.h"
#include "ta_test_priv.h"

/* The widest output arity the hand-written harness sizes its buffers for.
 * The generated side counts this from the corpus (backends/common.rs
 * max_output_arity(), #262); this define is the one place the number is still
 * written down by hand. It is enforced, not assumed: main() fails at startup
 * with TA_CODEGEN_OUTPUT_ARITY_EXCEEDS_CAP if any registered function is
 * wider (codegen_output_arity_within_cap below), because the comparison loops
 * clamp at this cap (a wider function would silently pass with its extra
 * outputs unverified) and the range-test param struct sizes its buffer arrays
 * with it (issue #352). */
#define CODEGEN_MAX_OUTPUTS  4

/* Run codegen verification tests against one or more languages.
 * languageFilter: comma-separated list of languages to test (NULL = test all).
 *   Valid values: "rust", "c", "java", "csharp"
 * functionFilter: CSV list of function names to test (NULL = test all).
 * Errors loudly if a requested language's server cannot be started.
 */
ErrorNumber test_codegen_unstable_map( void );

/* Fails with TA_CODEGEN_OUTPUT_ARITY_EXCEEDS_CAP if any registered function
 * has nbOutput > CODEGEN_MAX_OUTPUTS (issue #352). Call with the library
 * initialized, ahead of every run mode — the clamped loops and sized buffers
 * this protects are used by the normal suite, --codegen, --fuzz-064 and
 * --xlang-hash alike. */
ErrorNumber codegen_output_arity_within_cap( void );

ErrorNumber test_codegen(const TA_History *history,
                         const char *languageFilter,
                         const char *functionFilter);

/* The --function rule for a SHORT token (one or two characters): it must be a
 * whole '_'-delimited component of `name`, so DI selects DI, PLUS_DI and
 * MINUS_DI but not DIV, and HT selects the HT_* family. Longer tokens stay a
 * plain substring match. Two letters sit inside too many names to narrow
 * anything loosely -- "MA" is a substring of 26 shipped names, "ER" of 12 --
 * so a short token matched by substring runs a sweep the caller cannot opt out
 * of. Callers apply this to a function name, or to one element of a group tag.
 */
int codegen_short_filter_token_matches(const char *name, const char *token);

/* Can this language server select a compatibility variant?
 * Only C still carries the deprecated TA_SetCompatibility. Rust, Java and the
 * managed C# are pinned to Default with no public setter — their backends
 * constant-fold the Metastock branches out of the generated code entirely, so
 * those arms do not exist to be selected. A Metastock leg would silently
 * re-run the Default one — callers must skip it visibly instead. The
 * Metastock arms keep their bit-exact coverage in C.
 * Returns 1 when the mode can be switched. */
int codegen_lang_has_compatibility_api(const char *lang);

/* True if this language must drop transcendental-reaching calls to the 1e-9
 * element compare instead of a bitwise hash. Single definition shared by
 * --xlang-hash and server_verify; see the comment on the implementation. */
int codegen_lang_needs_transcendental_tol(const char *lang);

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
 * #114). Java and C# both fall back to a narrow tolerance on transcendental-
 * using calls (see codegen_lang_needs_transcendental_tol).
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

/* ---- Transcendental tolerance path (shared by --xlang-hash + server_verify)
 * A call reaching a transcendental (atan/sin/cos/exp/log/...) cannot be
 * bit-compared against a language whose math does not resolve to the same libm
 * the in-process golden uses: Java's fdlibm never does, and .NET's `Math.*` is
 * not guaranteed to (proven host-dependent — see
 * codegen_lang_needs_transcendental_tol). Those calls swap the bitwise out_hash
 * path for a narrow element-compare at this tolerance (relative for |v|>1,
 * absolute otherwise). Every other language, and every non-transcendental call,
 * stays bitwise. Measured Java drift over both gates' scenarios peaks ~9.7e-15
 * (HT_DCPHASE), so 1e-9 keeps ~5 orders of margin over libm noise while still
 * failing any real algorithmic regression (orders of magnitude larger). Equals
 * CODEGEN_EPSILON_DOUBLE. ---- */
#define CODEGEN_TRANSCENDENTAL_TOL 1e-9

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
 * transport for both gates' C<=>server comparisons. Appends at absolute `pos`
 * (bounded, like codegen_appendf) and returns the new position. */
int codegen_write_hexbits_array(char *buf, int buf_size, int pos,
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

/* ---- Minimal JSON field reader, shared with server_verify.c ----
 * `json_find_field` returns NULL when the field is absent (presence, not just a
 * parsed value) -- a missing field must fail loudly rather than a caller reading
 * a default int as if it were a real answer. `json_get_int` is the atoi
 * convenience once presence does not matter. */
const char *json_find_field(const char *json, const char *field, int *len);
int json_get_int(const char *json, const char *field);

#endif
