/* test_codegen.c — generic ta_abstract-driven codegen verification
 *
 * Replaces 3 hand-coded callbacks (SMA, MULT, RSI) with one generic
 * callback that uses ta_abstract to call ANY TA function. Handles
 * price inputs, multi-output, integer outputs, real optional params,
 * and unstable periods.
 */
#include "test_codegen.h"
#include "codegen_pipe.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

/* Display flag set by ta_regtest.c --no-guarded */
int g_hideGuarded = 0;

/* Float legs that actually compared values, i.e. where the server acknowledged
 * "use_float". Asserted non-zero so the leg cannot pass by never running —
 * but only when a language that HAS a float surface was tested. Rust is
 * concrete f64 and has none, so a Rust-only run legitimately compares zero
 * (that is what the dev nightly's debug-profile job does). */
static long g_floatLegCompared = 0;
static int  g_floatCapableLangTested = 0;
#include <limits.h>
#ifdef __APPLE__
#include <mach/mach_time.h>
#endif
#if defined(WIN32) || defined(_WIN32)
#include <windows.h>
/* Portability shims for the report-writing paths (MSVC has the same
 * functionality under different names; PATH_MAX is POSIX-only). */
#define popen  _popen
#define pclose _pclose
#ifndef PATH_MAX
#define PATH_MAX MAX_PATH
#endif
/* realpath(rel, abs) and _fullpath(abs, rel, size) swap their arguments. */
#define realpath(rel, abs) _fullpath((abs), (rel), PATH_MAX)
#else
#include <limits.h> /* PATH_MAX */
#endif

#include "ta_libc.h"
#include "ta_abstract.h"
#include "ta_utility.h"  /* TA_IS_ZERO / TA_IS_ZERO_SCALED / TA_IS_ZERO_OR_NEG (predicate parity truth) */
#include "fuzz_data.h"   /* shared, byte-identical input generator + output hasher */
#include "../ta_alloc_check.h"

/* Timing now comes from each server's JSON-RPC timing_ns field (the reference
 * baseline is ta_ref_serve, task #7), so no in-process timer is needed here. */

/* ---- Language definitions ---- */

typedef struct {
    const char *name;           /* "rust", "c", "java", "csharp" */
    const char *display;        /* "Rust", "C", "Java", "C#" */
    const char *const *argv;    /* NULL-terminated command array */
} CodegenLanguage;

static const char *const argv_rust[]  = {"./ta_codegen_serve_rust", NULL};
static const char *const argv_c[]     = {"./ta_codegen_serve_c", NULL};
static const char *const argv_java[]  = {"java", "-cp", "ta_codegen_java", "TaCodegenServe", NULL};
static const char *const argv_csharp[]= {"dotnet", "ta_codegen_csharp/TaCodegenServe.dll", NULL};
/* Reference oracle (reference-as-server, task #7): the frozen reference C
 * library exposed as a JSON-RPC server. NOT a tested language — it is the
 * baseline every language server (including the generated C server) is diffed
 * against. Built from the pinned-tag worktree by scripts/regtest.py so it stays
 * frozen once src/ta_func becomes the generated code. */
static const char *const argv_cref[]  = {"./ta_ref_serve", NULL};
static const CodegenLanguage ALL_LANGUAGES[] = {
    {"c",      "C",            argv_c},
    {"rust",   "Rust",         argv_rust},
    {"java",   "Java",         argv_java},
    {"csharp", "C#",           argv_csharp},
};
#define NUM_LANGUAGES (sizeof(ALL_LANGUAGES) / sizeof(ALL_LANGUAGES[0]))

/* See test_codegen.h. Rust, Java and (managed) C# pin compatibility to Default
 * and expose no setter, so their Metastock legs are skipped rather than run
 * vacuously. */
int codegen_lang_has_compatibility_api(const char *lang)
{
    if( !lang ) return 1;
    return !(strcmp(lang, "rust") == 0 || strcmp(lang, "java") == 0
             || strcmp(lang, "csharp") == 0);
}

/* Which language servers implement the state-equivalence leg (#240): the
 * handle after Open(P) + (n-P) updates compared field-by-field against the
 * handle after Open(n).
 *
 * C only, and the reason is structural rather than a plan to catch up: the C
 * server is ONE translation unit that #includes every ta_*.c, so a stream
 * state struct is a complete type there and the comparator costs no new API
 * surface. In Rust the state lives in the `ta-lib` crate and the server is a
 * separate crate; Java and C# would need the fields opened up to their
 * servers. What the leg catches — a wrong offset in an emitted ring read — is
 * one IR rewrite reaching all four backends identically, which is the same
 * argument check_candle_windows.py makes for scanning one backend. */
static int codegen_lang_has_stream_state_probe(const char *lang)
{
    return lang && strcmp(lang, "c") == 0;
}

/* Which languages cannot be held bit-identical on a call that reaches a
 * transcendental. THE single definition — both --xlang-hash (which copies it
 * into XlangServer.tolTranscendental) and server_verify's own per-call check
 * read it, because when they were two separate literals they disagreed.
 *
 * Java: fdlibm is not the C libm (#113).
 * C#:   .NET does not guarantee `Math.*` reaches the platform libm, and on
 *       some hosts it does not. dev-nightly run 30776189041 produced 25 TA_LN
 *       mismatches on ubuntu-latest x86-64 from a commit that was bitwise-clean
 *       on ubuntu-24.04-arm and on a glibc-2.39 + .NET-10.0.10 box. Not a
 *       special-value problem: C and C# agree bit-for-bit on 0.0, -0.0 and
 *       negatives including the NaN payload, so it is a normal-value 1 ULP
 *       difference. A bitwise claim verified on one machine is a claim about
 *       that machine.
 * Rust: reaches the same libm as the in-process golden — stays bitwise. */
int codegen_lang_needs_transcendental_tol(const char *lang)
{
    if( !lang ) return 0;
    return strcmp(lang, "java") == 0 || strcmp(lang, "csharp") == 0;
}

/* Which languages can be ASKED whether TA_INTEGER_DEFAULT on an enum:MAType
 * parameter selects the declared default. C and C# type that parameter as an
 * integer (a C# enum is an int with names), so the sentinel reaches their
 * validation and the substitution is what maps it back (#162). Rust types it as
 * a real enum yet stays answerable: its library-side TryFrom maps i32::MIN to
 * MAType::DEFAULT, which the per-slot prologue then substitutes -- do NOT "fix"
 * that asymmetry by adding rust below, it would delete a working leg. Java's MAType is
 * a real enum and Core takes MAType: the value is unrepresentable rather than
 * mishandled, its server would die constructing one
 * (MAType.values()[Integer.MIN_VALUE] throws), and substituting server-side
 * would only manufacture a green. Withheld vectors are counted and printed.
 *
 * Lives up here with its sibling capability predicates because it has TWO
 * consumers: --xlang-hash's parameter vectors and the sentinel float leg
 * (#170). Same reason codegen_lang_needs_transcendental_tol is one definition. */
static int codegen_lang_can_pass_enum_sentinel(const char *lang)
{
    if( !lang ) return 1;
    return strcmp(lang, "java") != 0;
}

/* The choice list's DEFAULT member (#182), or -1 when the enum declares none.
 * This is the spelling a typed enum CAN carry, so it reaches the substitution in
 * the one backend TA_INTEGER_DEFAULT cannot. Keyed on the name rather than a
 * literal 11: the value is enums.yaml's to choose. */
static int codegen_enum_default_member(const TA_OptInputParameterInfo *optInfo)
{
    const TA_IntegerList *l;
    unsigned int e;
    if( !optInfo || optInfo->type != TA_OptInput_IntegerList ) return -1;
    l = (const TA_IntegerList *)optInfo->dataSet;
    if( !l ) return -1;
    for( e = 0; e < l->nbElement; e++ )
        if( l->data[e].string && strcmp(l->data[e].string, "DEFAULT") == 0 )
            return l->data[e].value;
    return -1;
}

/* ---- Default-sentinel float leg counters (issue #170) ----
 * The sentinel pass is a SUBSET of the float leg, so it gets its own counters
 * rather than folding into g_floatLegCompared: a small subset behind a big
 * aggregate can be switched off entirely while the aggregate's floor stays
 * green on the cases that still run (the #162 lesson). Per LANGUAGE, because
 * the interesting failure — one server erroring on every sentinel request — is
 * invisible in a total that another server keeps non-zero.
 *
 * `eligible` counts functions that reached the pass with at least one slot
 * actually carrying a sentinel; it is what makes the floor precise under
 * --function= filters that select only parameterless functions. */
static long g_floatSentinelCompared[NUM_LANGUAGES];
static long g_floatSentinelEligible[NUM_LANGUAGES];
/* Of the comparisons, those that actually diffed OUTPUT ELEMENTS. A pass whose
 * two halves both answer "success, zero elements" compares retCode and nothing
 * else, so counting it as coverage would be the empty-output vacuity every
 * sibling gate here guards against. This, not `compared`, is what the floor
 * tests. */
static long g_floatSentinelWithOutput[NUM_LANGUAGES];
static long g_floatSentinelEnumWithheld = 0;

/* ---- startIdx-axis sweep counters (#236 step 2) ----
 * Per LANGUAGE, for the same reason as the sentinel counters above: a total
 * stays green while one server errors on every non-zero-startIdx request,
 * because the others keep it non-zero.
 *
 * `compared` counts (function, range) pairs that reached a real comparison;
 * `withOutput` counts those that diffed at least one OUTPUT ELEMENT. A pair
 * where both sides answer "success, zero elements" compares a retCode and
 * nothing else, so it is `compared` but not coverage — which is exactly the
 * shape this axis is meant to reach, since a produced count of zero is the one
 * case where the output bound switches off. */
static long g_startSweepCompared[NUM_LANGUAGES];
static long g_startSweepWithOutput[NUM_LANGUAGES];
/* Pairs withheld because the frozen reference is known-wrong there (see
 * ref_diverges_on_partial_range). Printed, so the carve-out cannot quietly grow
 * to swallow the axis. */
static long g_startSweepSkipped98 = 0;
/* ---- Return-code census (#236 step 4) ----
 * The retCode of every value-comparison call and every index-range case,
 * counted per LANGUAGE and per code. NOT every code on the wire: the unstable
 * period, candle-setting, predicate-parity and stream_verify legs answer codes
 * this census does not see.
 *
 * Two jobs. The first is a floor: the server is about to become the place where
 * a thrown failure is turned back into a code, and a normalisation layer is a
 * new place for vacuity to hide. If it mapped everything onto one code and the
 * corpus only ever produced that code, nothing would notice — so the codes the
 * corpus is known to reach are required to keep being reached, per language,
 * because one server erroring on every request of a kind is invisible in a total
 * another server keeps non-zero. Same lesson as the float-sentinel counters
 * above, same shape.
 *
 * The second was a one-off: the printed distribution was captured from the
 * C-shaped path BEFORE the harness was pointed at the public API and diffed
 * against the same run afterwards, byte for byte, which is how the switch was
 * shown to move no return code — something no value comparison can see, because
 * a rejected call has no values to compare. That diff was manual and nothing
 * here re-checks it; what SURVIVES in the code is the floor above. Pinning the
 * exact counts was considered and rejected: they move with every added
 * function, so the pin would be re-baselined rather than believed.
 */
#define RC_BUCKETS 8
static const int RC_CODE[RC_BUCKETS] = { 0, 2, 3, 12, 13, 17, 5000, -1 };
static const char *const RC_NAME[RC_BUCKETS] = {
    "Success", "BadParam", "AllocErr", "OutOfRangeStartIndex",
    "OutOfRangeEndIndex", "InsufficientHistory", "InternalError", "other"
};
static long g_retCodeSeen[NUM_LANGUAGES][RC_BUCKETS];
/* Set when a language's pass COMPLETED, which `g_codegenCompared[]` cannot
 * stand in for: a filter selecting only post-cutover functions compares no
 * values and leaves that counter at 0 on a language that very much ran. */
static int g_langRan[NUM_LANGUAGES];

static void record_retcode( int langIndex, int code )
{
    int b;
    if( langIndex < 0 || langIndex >= (int)NUM_LANGUAGES )
        return;
    for( b = 0; b < RC_BUCKETS - 1; b++ )
        if( RC_CODE[b] == code ) { g_retCodeSeen[langIndex][b]++; return; }
    g_retCodeSeen[langIndex][RC_BUCKETS - 1]++;
}

/* Calls where the server REPORTED allocating an output buffer larger than the
 * count it produced. Counted and floored: with the servers sized to the produced
 * count by default, dropping this would leave the whole harness proving only
 * that the minimum is accepted, never that anything above it is — and the bound
 * is a minimum, which is the whole point (a caller re-using a pre-allocated
 * buffer passes a larger one).
 *
 * Read off the server's own `out_len`, not off the pad this file asked for. A
 * counter incremented beside `outPad = OUT_SLACK_PAD` measures the harness's
 * intention: set the pad to 0, or break the server's parsing of it, and the
 * floor stays green over calls that are all exactly sized. */
static long g_slackCalls[NUM_LANGUAGES];
/* How much larger. Small on purpose — a big pad is the state this replaced, and
 * would not distinguish "the bound is a minimum" from "the bound is unchecked". */
#define OUT_SLACK_PAD 7

/* The reference this leg compares against is `ta_ref_serve`, the frozen
 * pre-cutover library — and issue #98 fixed two functions whose OLD behaviour it
 * still has. Both changed what they compute on a PARTIAL range only, so the
 * divergence is exactly `startIdx > lookback`, which is precisely what the
 * startIdx axis sends. `--fuzz-064` makes the same carve-out against v0.6.4 (its
 * `skipped98` counter); this is the same two names for the same reason, and the
 * cases it withholds are covered against the in-process C library by
 * `--xlang-hash`, whose golden is not frozen. */
static int ref_diverges_on_partial_range( const char *name, TA_Integer startIdx,
                                          TA_Integer lookback )
{
    if( startIdx <= lookback )
        return 0;
    return strcmp(name, "TRIX") == 0 || strcmp(name, "NATR") == 0;
}

/* Functions that reached a real value comparison, per language. The closing
 * banner used to read "All N language(s) passed codegen verification" off
 * `langsTested` alone — a count of servers that STARTED, not of anything
 * compared. A run filtered to post-cutover functions skips every one of them
 * for want of a frozen ta_ref_serve baseline and still printed that line over a
 * "0 passed, 0 failed" table, which is the most confident-sounding output in
 * the tool attached to the least evidence. The skip itself is legitimate (those
 * functions are covered by server_verify, --xlang-hash and their hard-coded
 * tests), so this is not a failure on a filtered run — it is a banner that must
 * stop claiming a pass it did not earn. Unfiltered it IS a failure: 171 shipped
 * functions cannot all legitimately skip, so zero there means the sweep went
 * dark. Mirrors the sentinel floor below. */
static long g_codegenCompared[NUM_LANGUAGES];

/* One line per language per kind of skipped leg, so the coverage a language
 * cannot take is stated in the log instead of quietly vanishing. */
#define MAX_COMPAT_NOTES (NUM_LANGUAGES * 4)
static void note_compat_skip(const char *lang, const char *what)
{
    static const char *reportedLang[MAX_COMPAT_NOTES];
    static const char *reportedWhat[MAX_COMPAT_NOTES];
    static int nbReported = 0;
    int i;
    for( i = 0; i < nbReported; i++ )
        if( reportedLang[i] == lang && reportedWhat[i] == what ) return;
    if( nbReported < (int)MAX_COMPAT_NOTES )
    {
        reportedLang[nbReported] = lang;
        reportedWhat[nbReported] = what;
        nbReported++;
    }
    printf("  NOTE [%s]: %s skipped - no compatibility API in this language\n",
           lang, what);
}

/* ---- Global timing results store (Task 12) ---- */

#define MAX_FUNCTIONS 200

typedef struct {
    char   funcName[64];
    double c_ref_ns;
    struct {
        int    tested;   /* 0=skipped, 1=pass, -1=fail */
        double avg_ns;
    } langs[NUM_LANGUAGES];
} FuncTimingResult;

static FuncTimingResult g_timingResults[MAX_FUNCTIONS];
static int              g_numTimingResults = 0;

/* ---- Per-function stream counters, per language ------------------------- *
 *
 * `legs` and `benign` are deterministic integers over the same algorithm and
 * the same generated inputs, so two languages that render the SAME source
 * must produce the SAME pair for every function. That is the only observer
 * this tree has for a class of defect nothing else can see: `svXtierNe`
 * counts a cross-tier +0.0/-0.0 difference as benign and never fails, and
 * `stream_verify` runs both of its arms inside ONE server — so a defect whose
 * only symptom is a zero's sign is green in the stream gate, absent from
 * --xlang-hash (batch only), and invisible in the nightly.
 *
 * Compared JAVA vs C# ONLY, deliberately. Those two render identically, down
 * to `Math.Min` keeping -0.0. C and Rust legitimately differ: the C `min()`
 * macro is a ternary that returns +0.0 for `min(-0.0, 0.0)`, and C also
 * counts candidate prefixes rather than successful opens, so its `legs` is on
 * a different accounting entirely. Adding either to the comparison would make
 * this a permanent red that teaches nothing. */
typedef struct {
    char      funcName[64];
    int       seen[NUM_LANGUAGES];
    int       legs[NUM_LANGUAGES];
    long long benign[NUM_LANGUAGES];
} FuncStreamCounters;

static FuncStreamCounters g_streamCounters[MAX_FUNCTIONS];
static int                g_numStreamCounters = 0;

/* Bits set in `v`. Only the range-site mask uses it, and only in a message and
 * a floor, so a loop is clearer here than a builtin that is not C89. */
static int codegen_popcount( int v )
{
    int n = 0;
    unsigned int u = (unsigned int)v;
    while( u ) { n += (int)(u & 1u); u >>= 1; }
    return n;
}

static void record_stream_counters( const char *funcName, int langIndex,
                                    int legs, long long benign )
{
    int i;
    if( langIndex < 0 || langIndex >= (int)NUM_LANGUAGES )
        return;
    for( i = 0; i < g_numStreamCounters; i++ )
    {
        if( strcmp(g_streamCounters[i].funcName, funcName) == 0 )
            break;
    }
    if( i == g_numStreamCounters )
    {
        if( g_numStreamCounters >= MAX_FUNCTIONS )
            return;
        memset(&g_streamCounters[i], 0, sizeof(FuncStreamCounters));
        strncpy(g_streamCounters[i].funcName, funcName,
                sizeof(g_streamCounters[i].funcName) - 1);
        g_numStreamCounters++;
    }
    g_streamCounters[i].seen[langIndex]   = 1;
    g_streamCounters[i].legs[langIndex]   = legs;
    g_streamCounters[i].benign[langIndex] = benign;
}

/* Index of a language by its --language= name, or -1. */
static int lang_index_by_name( const char *name )
{
    unsigned int i;
    for( i = 0; i < NUM_LANGUAGES; i++ )
    {
        if( strcmp(ALL_LANGUAGES[i].name, name) == 0 )
            return (int)i;
    }
    return -1;
}

/* Returns the number of functions whose Java and C# counters disagree. Prints
 * each. Silent (and 0) when either language was not exercised in this run —
 * `--language=csharp` alone has nothing to compare against, which is a
 * narrowed run, not a failure. */
static int check_stream_counter_parity( void )
{
    int javaIdx = lang_index_by_name("java");
    int csIdx   = lang_index_by_name("csharp");
    int i, bad = 0, compared = 0;

    if( javaIdx < 0 || csIdx < 0 )
        return 0;

    for( i = 0; i < g_numStreamCounters; i++ )
    {
        const FuncStreamCounters *r = &g_streamCounters[i];
        if( !r->seen[javaIdx] || !r->seen[csIdx] )
            continue;
        compared++;
        if( r->legs[javaIdx] != r->legs[csIdx]
            || r->benign[javaIdx] != r->benign[csIdx] )
        {
            printf("STREAM COUNTER MISMATCH [TA_%s]: java legs=%d benign=%lld, "
                   "csharp legs=%d benign=%lld\n",
                   r->funcName,
                   r->legs[javaIdx], r->benign[javaIdx],
                   r->legs[csIdx], r->benign[csIdx]);
            bad++;
        }
    }
    if( compared > 0 )
    {
        printf("  Java/C# stream counter parity: %d function(s) compared, "
               "%d mismatch(es)\n", compared, bad);
    }
    return bad;
}

/* ---- Constants ---- */

#define CODEGEN_EPSILON  1e-6   /* float leg (TA_S_*): single-precision noise */
/* Double-leg cross-language / cross-version tolerance. Tightened from 1e-6 to
 * 1e-9 (issue #113 follow-up): a full-precision measurement of every language
 * server vs the frozen reference showed the real floor is <1e-11 for all 161
 * functions except LINEARREG_ANGLE (~4.4e-10, the authorized #103 recurrence) —
 * the %.15g transport was never the limit. Applied as 1e-9 * max(1, |value|). */
#define CODEGEN_EPSILON_DOUBLE  1e-9
#define JSON_BUF_SIZE    (128 * 1024)   /* 128KB: enough for OHLCV inputs */
#define MAX_OUTPUTS      3              /* Max outputs any TA function has */

/* ---- Minimal JSON helpers (no library dependency) ---- */

static int json_write_double_array(char *buf, int buf_size, int pos,
                                   const TA_Real *data, int count, int widen)
{
    pos = codegen_appendc(buf, buf_size, pos, '[');
    for( int i = 0; i < count; i++ )
    {
        if( i > 0 )
            pos = codegen_appendc(buf, buf_size, pos, ',');
        if( widen )
            /* Round to float then back to double; %.17g round-trips exactly. */
            pos = codegen_appendf(buf, buf_size, pos, "%.17g", (double)(float)data[i]);
        else
            pos = codegen_appendf(buf, buf_size, pos, "%.15g", data[i]);
    }
    return codegen_appendc(buf, buf_size, pos, ']');
}

/* Lossless input transport (issue #115, shared via test_codegen.h): serialize
 * each double as its 16-hex-char IEEE-754 bit pattern inside one JSON string,
 * decoded byte-exactly by every server's array parser. No %.15g rounding to hide
 * a divergence behind. Used by --xlang-hash's Java leg and by server_verify. */
int codegen_write_hexbits_array(char *buf, int buf_size, int pos,
                                const TA_Real *data, int count)
{
    pos = codegen_appendc(buf, buf_size, pos, '"');
    for( int i = 0; i < count; i++ )
    {
        unsigned long long bits;
        memcpy(&bits, &data[i], sizeof(bits));
        pos = codegen_appendf(buf, buf_size, pos, "%016llx", bits);
    }
    return codegen_appendc(buf, buf_size, pos, '"');
}

static const char *json_find_field(const char *json, const char *field, int *len)
{
    char pattern[256];
    snprintf(pattern, sizeof(pattern), "\"%s\":", field);
    const char *p = strstr(json, pattern);
    if( !p ) return NULL;
    p += strlen(pattern);
    while( *p == ' ' ) p++;

    const char *start = p;
    if( *p == '"' )
    {
        p++;
        while( *p && *p != '"' ) p++;
        if( *p == '"' ) p++;
    }
    else if( *p == '[' )
    {
        int depth = 1;
        p++;
        while( *p && depth > 0 )
        {
            if( *p == '[' ) depth++;
            else if( *p == ']' ) depth--;
            p++;
        }
    }
    else
    {
        while( *p && *p != ',' && *p != '}' ) p++;
    }

    *len = (int)(p - start);
    return start;
}

static int json_get_int(const char *json, const char *field)
{
    int len;
    const char *val = json_find_field(json, field, &len);
    if( !val ) return 0;
    return atoi(val);
}

static int json_get_double_array(const char *json, const char *field,
                                 TA_Real *out, int max_count)
{
    int len;
    const char *val = json_find_field(json, field, &len);
    if( !val || *val != '[' ) return 0;

    int count = 0;
    const char *p = val + 1;
    while( *p && *p != ']' && count < max_count )
    {
        while( *p == ' ' || *p == ',' ) p++;
        if( *p == ']' ) break;
        out[count] = strtod(p, (char **)&p);
        count++;
    }
    return count;
}

static int json_get_int_array(const char *json, const char *field,
                              TA_Integer *out, int max_count)
{
    int len;
    const char *val = json_find_field(json, field, &len);
    if( !val || *val != '[' ) return 0;

    int count = 0;
    const char *p = val + 1;
    while( *p && *p != ']' && count < max_count )
    {
        while( *p == ' ' || *p == ',' ) p++;
        if( *p == ']' ) break;
        out[count] = (TA_Integer)strtol(p, (char **)&p, 10);
        count++;
    }
    return count;
}

static int json_is_error(const char *json)
{
    return strstr(json, "\"error\"") != NULL;
}

/* The only error responses a `call` may legitimately answer: a name this
 * backend does not implement. Every other error on that path is a defect --
 * see the caller. */
static int json_error_is_unsupported(const char *json)
{
    return strstr(json, "Unknown function") != NULL ||
           strstr(json, "Unknown method")   != NULL;
}

/* ---- Unstable period lookup ---- */

/* Map function name to TA_FuncUnstId for range-sweep tolerance selection.
 * Entries are the 20 functions that carry TA_FUNC_FLG_UNST_PER, plus the
 * derived functions (DEMA/TEMA/TRIX/MACD/MACDEXT/MACDFIX + APO/PPO via EMA,
 * ADXR/STOCHRSI via ADX/RSI) that converge through an internal callee. APO and PPO now default to EMA (issue #120), so their
 * default-parameter range sweep is EMA-converging and needs the loose convergence
 * envelope; the envelope is a safe superset for their finite-window (SMA/WMA/…)
 * parameterisations too. (MACDEXT defaults to SMA but is in this list on the same
 * safe-superset basis — it converges only when an optional MA type is set to EMA.)
 * IMI and MFI are deliberately excluded (finite-window, stable).
 */
typedef struct {
    const char   *name;
    TA_FuncUnstId id;
} UnstableLookup;

static const UnstableLookup UNSTABLE_MAP[] = {
    {"ADX",          TA_FUNC_UNST_ADX},
    {"ATR",          TA_FUNC_UNST_ATR},
    {"CMO",          TA_FUNC_UNST_CMO},
    {"DX",           TA_FUNC_UNST_DX},
    {"EMA",          TA_FUNC_UNST_EMA},
    {"HT_DCPERIOD",  TA_FUNC_UNST_HT_DCPERIOD},
    {"HT_DCPHASE",   TA_FUNC_UNST_HT_DCPHASE},
    {"HT_PHASOR",    TA_FUNC_UNST_HT_PHASOR},
    {"HT_SINE",      TA_FUNC_UNST_HT_SINE},
    {"HT_TRENDLINE", TA_FUNC_UNST_HT_TRENDLINE},
    {"HT_TRENDMODE", TA_FUNC_UNST_HT_TRENDMODE},
    /* IMI and MFI are NOT listed here on purpose. Both are finite
     * sliding-window indicators, not recursive/converging ones, so their
     * range sweep must use the tight TA_TEST_UNST_NONE tolerance (IMI is
     * bit-exact; MFI drifts only ~1e-13 via its running accumulator), not
     * the loose convergence envelope this map selects. Their TA_FUNC_UNST_*
     * enum entries are retained for ABI but no longer advertise instability.
     */
    {"KAMA",         TA_FUNC_UNST_KAMA},
    {"MAMA",         TA_FUNC_UNST_MAMA},
    {"MINUS_DI",     TA_FUNC_UNST_MINUS_DI},
    {"MINUS_DM",     TA_FUNC_UNST_MINUS_DM},
    {"NATR",         TA_FUNC_UNST_NATR},
    {"PLUS_DI",      TA_FUNC_UNST_PLUS_DI},
    {"PLUS_DM",      TA_FUNC_UNST_PLUS_DM},
    {"RSI",          TA_FUNC_UNST_RSI},
    {"T3",           TA_FUNC_UNST_T3},
    /* EMA-derived: doRangeTest sweeps UNST_EMA, as the hand MA tests do. */
    {"DEMA",         TA_FUNC_UNST_EMA},
    {"TEMA",         TA_FUNC_UNST_EMA},
    {"TRIX",         TA_FUNC_UNST_EMA},
    {"MACD",         TA_FUNC_UNST_EMA},
    {"MACDEXT",      TA_FUNC_UNST_EMA},
    {"MACDFIX",      TA_FUNC_UNST_EMA},
    /* APO/PPO default to EMA (#120) -> EMA-converging, like MACDEXT. PVO is
     * PPO over volume and defaults to EMA the same way, so it belongs here for
     * the same reason. Its absence classified it EPSILON, and the range gate
     * that would have said so is the one post-cutover functions never reach --
     * measured, PVO moves by whole multiples across startIdx (4.7 at 40, 25.1
     * at 80) where a genuine EPSILON function moves by ~1e-13. */
    {"APO",          TA_FUNC_UNST_EMA},
    {"PPO",          TA_FUNC_UNST_EMA},
    {"PVO",          TA_FUNC_UNST_EMA},
    /* EFI smooths its force series with the same EMA. */
    {"EFI",          TA_FUNC_UNST_EMA},
    /* SMI's three EMA stages are seeded and advanced exactly as ema.c does,
     * and its lookback is the sum of three ema_lookback() terms, so the whole
     * pipeline shifts with UNST_EMA. Measured: outBegIdx 45 -> 54 at the
     * defaults when the unstable period is set to 3. */
    {"SMI",          TA_FUNC_UNST_EMA},
    /* ADXR/STOCHRSI own knobs were inert and retired (#129); they converge
     * via their internal ADX/RSI, like the EMA-derived set above. */
    {"ADXR",         TA_FUNC_UNST_ADX},
    {"STOCHRSI",     TA_FUNC_UNST_RSI},
};
#define NUM_UNSTABLE_MAP (sizeof(UNSTABLE_MAP) / sizeof(UNSTABLE_MAP[0]))

static TA_FuncUnstId get_unst_id(const char *funcName)
{
    for( unsigned int i = 0; i < NUM_UNSTABLE_MAP; i++ )
    {
        if( strcmp(funcName, UNSTABLE_MAP[i].name) == 0 )
            return UNSTABLE_MAP[i].id;
    }
    return TA_TEST_UNST_NONE;
}

/* ---- Generic CodegenRangeTestParam (Task 6) ---- */

/* Widest optional-parameter list any pass may drive at once. Sizes
 * CodegenRangeTestParam::optOverride[] below; the ref differential sweep
 * (which introduced it) refuses functions with more than this many. */
#define SWEEP_MAX_OPT 16

typedef struct {
    /* ta_abstract function metadata */
    const TA_FuncInfo *funcInfo;
    TA_ParamHolder   *paramHolder;

    /* Input data */
    const TA_History *history;
    int nbBars;

    /* Output buffers indexed by outputNb (logical output index).
     * Each output is EITHER real or integer, never both.
     * We allocate both arrays per outputNb for simplicity; only
     * the correct one (per outputIsInteger[i]) is used. */
    TA_Real    *outRealBufs[MAX_OUTPUTS];
    TA_Integer *outIntBufs[MAX_OUTPUTS];
    int         outputIsInteger[MAX_OUTPUTS];   /* Flag per output */
    int         totalOutputs;

    /* Cached results from TA_CallFunc (for multi-output) */
    TA_RetCode  lastRetCode;
    TA_Integer  lastBegIdx;
    TA_Integer  lastNbElement;

    /* Unstable period info */
    TA_FuncUnstId unstId;

    /* Codegen pipe (language server under test) */
    CodegenPipe *cp;
    /* Which language `cp` speaks: its ALL_LANGUAGES name and index. The pipe
     * alone does not say, and the sentinel float leg needs both — the name to
     * decide whether that language's optional-parameter surface can carry the
     * enum sentinel, the index for its per-language non-vacuity counters. */
    const char *langName;
    int         langIndex;
    /* Reference oracle pipe (ta_ref_serve) — fills the comparison baseline */
    CodegenPipe *refCp;
    char *requestBuf;
    char *responseBuf;

    /* Error tracking */
    ErrorNumber codegenError;

    /* When set, build_json_request uses a large value for every IntegerRange
     * opt param (Task 10 large-period coverage) instead of the default. */
    int useLargePeriod;

    /* Ref differential sweep: when optOverrideActive, build_json_request
     * emits optOverride[i] for optional param i instead of the default (or
     * large-period) value. Stored as double; integer params truncate on
     * emission. Takes precedence over useLargePeriod. */
    int    optOverrideActive;
    double optOverride[SWEEP_MAX_OPT];

    /* Float-variant leg: build_json_request adds "use_float":1, routing the
     * servers through the single-precision TA_S_ API. Comparisons then use
     * an epsilon widened by epsilonScale (float noise from codegen operation
     * reordering; 0 means the default scale of 1). */
    int    useFloat;
    double epsilonScale;
    /* Float leg: when set, build_json_request rounds every input value to float
     * and back to double (serialized with %.17g, exact) so the double-variant
     * baseline and the use_float single-precision leg operate on identical
     * float-derived inputs. Lets the float leg assert TA_S_<F> == TA_<F> on
     * widened inputs bit-for-bit (the single-precision variants now compute in
     * double, PR #33). */
    int    widenFloatInputs;
    int    sweepFloatLeg;   /* run the float leg per sweep variant (C only) */

    /* Extra elements the server is asked to allocate past the produced count.
     *
     * The output bound is a MINIMUM, not an equality — a caller re-using a
     * pre-allocated buffer passes a larger one, and the reported OutRange is
     * what says which part was written. Both halves need exercising, and they
     * pull in opposite directions: sized to the produced count the bound is
     * reachable at last (#236 step 2), sized larger it proves slack is still
     * accepted. So the full-range value comparison sends a pad and the
     * startIdx axis sends none, rather than every call being one or the
     * other. */
    int    outPad;

    /* Set once the default full-range pass has shown this language answers this
     * function at all (a non-error response). Distinguishes "this backend does
     * not implement it" from "it implements it and broke on this range", which
     * the edge sweep needs: an error response is silently skipped as an
     * unsupported function, and with output buffers sized to the produced count
     * an out-of-bounds write becomes exactly such a response (an
     * ArrayIndexOutOfBoundsException in Java, a panic-closed pipe in Rust). */
    int    langSupported;

    /* Timing */
    long long c_ref_total_ns;
    long long server_total_ns;
    int       timing_count;
} CodegenRangeTestParam;

/* Forward declaration: defined with the sweep further below, used by the
 * per-function default pass as well. */
static void run_float_leg(CodegenRangeTestParam *p, int withSentinel);

/* ---- Large-period stress coverage (Task 10) ----
 * The default codegen sweep uses each opt param's default (e.g. period 14), so a
 * period-dependent buffer sized smaller than a larger period would never be
 * exercised. These helpers drive a second comparison pass with every
 * IntegerRange opt param pushed above the historical CIRCBUF static-buffer sizes
 * (50 for MFI, 30 for CCI), catching that class of regression for ALL
 * period-parameterized indicators. */

/* A stress period for an IntegerRange opt param: above the historical CIRCBUF
 * static buffers (50/30), clamped to [min,max] and bounded so meaningful output
 * remains. Uses default+50 (not a fixed constant) so multi-period functions like
 * APO/PPO/ADOSC keep their fast<slow ordering and don't collapse to an all-zero
 * difference series (which would make the comparison pass while verifying nothing). */
static int compute_large_int(const TA_OptInputParameterInfo *optInfo, int nbBars)
{
    const TA_IntegerRange *r = (const TA_IntegerRange *)optInfo->dataSet;
    int lo = r ? (int)r->min : 1;
    int hi = r ? (int)r->max : 1;
    int target = (int)optInfo->defaultValue + 50;
    if( target > hi ) target = hi;
    if( target > nbBars - 5 ) target = nbBars - 5;
    if( target < lo ) target = lo;
    return target;
}

/* Set every IntegerRange opt param on the holder to its stress period; IntegerList
 * (enums) and Real* params are left at their defaults. Returns the count that
 * ended up strictly larger than the default (so a large-period pass is meaningful). */
static int set_large_opt_periods(TA_ParamHolder *paramHolder,
                                 const TA_FuncInfo *funcInfo, int nbBars)
{
    unsigned int i;
    int nLarger = 0;
    for( i = 0; i < funcInfo->nbOptInput; i++ )
    {
        const TA_OptInputParameterInfo *optInfo;
        TA_GetOptInputParameterInfo(funcInfo->handle, i, &optInfo);
        if( optInfo->type != TA_OptInput_IntegerRange )
            continue;
        int large = compute_large_int(optInfo, nbBars);
        if( large > (int)optInfo->defaultValue )
            nLarger++;
        TA_SetOptInputParamInteger(paramHolder, i, large);
    }
    return nLarger;
}

/* Restore every IntegerRange opt param to its default. */
static void reset_opt_periods_to_default(TA_ParamHolder *paramHolder,
                                         const TA_FuncInfo *funcInfo)
{
    unsigned int i;
    for( i = 0; i < funcInfo->nbOptInput; i++ )
    {
        const TA_OptInputParameterInfo *optInfo;
        TA_GetOptInputParameterInfo(funcInfo->handle, i, &optInfo);
        if( optInfo->type == TA_OptInput_IntegerRange )
            TA_SetOptInputParamInteger(paramHolder, i, (int)optInfo->defaultValue);
    }
}

/* ---- Generic JSON request builder (Task 7) ---- */

static int build_json_request(CodegenRangeTestParam *p,
                              TA_Integer startIdx, TA_Integer endIdx)
{
    const TA_FuncInfo *fi = p->funcInfo;
    char *buf = p->requestBuf;
    int bufSize = JSON_BUF_SIZE;
    int pos = 0;
    unsigned int i;
    int realInputCount = 0;

    /* Method and startIdx/endIdx */
    pos = codegen_appendf(buf, bufSize, pos,
        "{\"method\":\"TA_%s\",\"params\":{\"startIdx\":%d,\"endIdx\":%d,\"out_pad\":%d",
        fi->name, (int)startIdx, (int)endIdx, p->outPad);

    /* Pre-count real inputs to decide naming convention */
    int totalRealInputs = 0;
    for( i = 0; i < fi->nbInput; i++ )
    {
        const TA_InputParameterInfo *tmpInfo;
        TA_GetInputParameterInfo(fi->handle, i, &tmpInfo);
        if( tmpInfo->type == TA_Input_Real )
            totalRealInputs++;
    }

    /* Input parameters */
    for( i = 0; i < fi->nbInput; i++ )
    {
        const TA_InputParameterInfo *inputInfo;
        TA_GetInputParameterInfo(fi->handle, i, &inputInfo);

        switch( inputInfo->type )
        {
        case TA_Input_Price:
        {
            TA_InputFlags flags = inputInfo->flags;
            if( flags & TA_IN_PRICE_OPEN )
            {
                pos = codegen_appendf(buf, bufSize, pos, ",\"inOpen\":");
                pos = json_write_double_array(buf, bufSize, pos,
                           p->history->open, p->nbBars, p->widenFloatInputs);
            }
            if( flags & TA_IN_PRICE_HIGH )
            {
                pos = codegen_appendf(buf, bufSize, pos, ",\"inHigh\":");
                pos = json_write_double_array(buf, bufSize, pos,
                           p->history->high, p->nbBars, p->widenFloatInputs);
            }
            if( flags & TA_IN_PRICE_LOW )
            {
                pos = codegen_appendf(buf, bufSize, pos, ",\"inLow\":");
                pos = json_write_double_array(buf, bufSize, pos,
                           p->history->low, p->nbBars, p->widenFloatInputs);
            }
            if( flags & TA_IN_PRICE_CLOSE )
            {
                pos = codegen_appendf(buf, bufSize, pos, ",\"inClose\":");
                pos = json_write_double_array(buf, bufSize, pos,
                           p->history->close, p->nbBars, p->widenFloatInputs);
            }
            if( flags & TA_IN_PRICE_VOLUME )
            {
                pos = codegen_appendf(buf, bufSize, pos, ",\"inVolume\":");
                pos = json_write_double_array(buf, bufSize, pos,
                           p->history->volume, p->nbBars, p->widenFloatInputs);
            }
            if( flags & TA_IN_PRICE_OPENINTEREST )
            {
                pos = codegen_appendf(buf, bufSize, pos, ",\"inOpenInterest\":");
                pos = json_write_double_array(buf, bufSize, pos,
                           p->history->openInterest, p->nbBars, p->widenFloatInputs);
            }
            break;
        }
        case TA_Input_Real:
        {
            if( totalRealInputs == 1 )
            {
                /* Single real input: "inReal" */
                pos = codegen_appendf(buf, bufSize, pos, ",\"inReal\":");
                pos = json_write_double_array(buf, bufSize, pos,
                           p->history->close, p->nbBars, p->widenFloatInputs);
            }
            else
            {
                /* Multiple real inputs: "inReal0", "inReal1", etc.
                 * Map: inReal0=close, inReal1=volume (matches old MULT behavior).
                 */
                const TA_Real *data;
                if( realInputCount == 0 )
                    data = p->history->close;
                else if( realInputCount == 1 )
                    data = p->history->volume;
                else
                    data = p->history->close;  /* fallback */

                pos = codegen_appendf(buf, bufSize, pos, ",\"inReal%d\":", realInputCount);
                pos = json_write_double_array(buf, bufSize, pos,
                           data, p->nbBars, p->widenFloatInputs);
            }
            realInputCount++;
            break;
        }
        case TA_Input_Integer:
            /* Integer inputs are rare (unused in practice).
             * Pass close prices cast to integers as placeholder. */
            break;
        }
    }

    /* Optional input parameters */
    for( i = 0; i < fi->nbOptInput; i++ )
    {
        const TA_OptInputParameterInfo *optInfo;
        TA_GetOptInputParameterInfo(fi->handle, i, &optInfo);

        pos = codegen_appendf(buf, bufSize, pos, ",\"%s\":", optInfo->paramName);

        switch( optInfo->type )
        {
        case TA_OptInput_RealRange:
        case TA_OptInput_RealList:
            pos = codegen_appendf(buf, bufSize, pos, "%.15g",
                p->optOverrideActive ? p->optOverride[i] : optInfo->defaultValue);
            break;
        case TA_OptInput_IntegerRange:
            pos = codegen_appendf(buf, bufSize, pos, "%d",
                p->optOverrideActive ? (int)p->optOverride[i]
                : p->useLargePeriod  ? compute_large_int(optInfo, p->nbBars)
                                     : (int)optInfo->defaultValue);
            break;
        case TA_OptInput_IntegerList:
            pos = codegen_appendf(buf, bufSize, pos, "%d",
                p->optOverrideActive ? (int)p->optOverride[i]
                                     : (int)optInfo->defaultValue);
            break;
        }
    }

    /* Unstable period (for functions with TA_FUNC_FLG_UNST_PER) */
    if( fi->flags & TA_FUNC_FLG_UNST_PER )
    {
        int unstPeriod = (p->unstId != TA_TEST_UNST_NONE)
                         ? (int)TA_GetUnstablePeriod(p->unstId) : 0;
        pos = codegen_appendf(buf, bufSize, pos, ",\"unstablePeriod\":%d", unstPeriod);
    }

    if( p->useFloat )
        pos = codegen_appendf(buf, bufSize, pos, ",\"use_float\":1");

    pos = codegen_appendf(buf, bufSize, pos, "}}");

    return pos;
}

/* ---- Generic output comparison (Task 8) ---- */

/* Functions whose OUTPUT VALUES intentionally diverge from the frozen pre-cutover
 * reference (ta_ref_serve) and are pinned by hand-written tests instead.
 * STOCHRSI (issue #107): its internal STOCHF now guards the divide with
 * TA_IS_ZERO where the reference divided a sub-epsilon flat-RSI-window residue
 * into full-scale [0,100] noise — so ta_ref_serve is the wrong value oracle for
 * it (same reason it is excluded from --fuzz-064). STOCHRSI's structural parity
 * (retCode/outBegIdx/outNBElement) stays strict on every backend, and its values
 * are pinned by test_stoch.c (test_stochrsi_epsilon_issue107). Standalone STOCH/
 * STOCHF keep the same guard but do NOT diverge from the reference on raw OHLC
 * (a flat raw window has highest==lowest exactly, diff==0), so they stay strictly
 * value-compared.
 *
 * CORREL (issue #242): the reference carries the one-pass
 * sumX2-(sumX*sumX)/period form, which keeps only the digits that survive that
 * subtraction. It reported a perfect correlation as 0, as -1, and as -1.73 —
 * outside [-1,1] — so it cannot referee the shifted-data form that replaced it.
 * Its values are pinned instead by test_correl.c, against oracles that share no
 * code with either version: a fresh per-window two-pass, NIST StRD Norris's
 * certified R-Squared, and identities exact by construction.
 *
 * This exemption applies ONLY to comparisons whose baseline is
 * the frozen reference — NOT the float leg, whose baseline is the current double
 * variant (a self-consistency check, see the widenFloatInputs guard at callsite). */
static int codegen_ref_value_exempt(const char *name)
{
    return strcmp(name, "STOCHRSI") == 0
        || strcmp(name, "CORREL") == 0;
}

static void compare_codegen_output_generic(
    CodegenRangeTestParam *p,
    unsigned int outputNb)
{
    /* Skip if we already have an error */
    if( p->codegenError != TA_TEST_PASS )
        return;

    /* An error response is one of two very different things. A name this
     * backend does not implement is the legitimate skip this leg has always
     * allowed. Anything else is an exception that ESCAPED -- and #236 step 4
     * put the public wrapper in this call path, which is exactly where a
     * genuine NullPointerException or IndexOutOfBounds would newly surface
     * (a carried failure is converted to a code and never reaches here).
     *
     * Returning silently on the second counted the call as compared --
     * `ctx->passed++` runs regardless -- so a crash in the tier this harness
     * now exists to test read as PASS. */
    if( json_is_error(p->responseBuf) )
    {
        if( json_error_is_unsupported(p->responseBuf) )
            return;
        printf("CODEGEN MISMATCH [TA_%s]: %s answered an ERROR response where a "
               "retCode was due -- an exception escaped the public API: %s\n",
               p->funcInfo->name, ALL_LANGUAGES[p->langIndex].display,
               p->responseBuf);
        p->codegenError = TA_CODEGEN_RETCODE_MISMATCH;
        return;
    }

    /* Compare retCode */
    int cg_retCode = json_get_int(p->responseBuf, "retCode");
    /* Once per CALL, not once per output: the census counts what the server
     * answered, and it answers once however many outputs are then read out. */
    if( outputNb == 0 )
        record_retcode(p->langIndex, cg_retCode);
    if( (int)p->lastRetCode != cg_retCode )
    {
        printf("CODEGEN MISMATCH [TA_%s]: retCode C=%d codegen=%d\n",
               p->funcInfo->name, (int)p->lastRetCode, cg_retCode);
        p->codegenError = TA_CODEGEN_RETCODE_MISMATCH;
        return;
    }

    /* If C returned error, both agree -- done */
    if( p->lastRetCode != TA_SUCCESS )
        return;

    /* If C produced no output, skip comparison */
    if( p->lastNbElement == 0 )
        return;

    /* Compare outBegIdx */
    int cg_begIdx = json_get_int(p->responseBuf, "outBegIdx");
    if( p->lastBegIdx != cg_begIdx )
    {
        printf("CODEGEN MISMATCH [TA_%s]: outBegIdx C=%d codegen=%d\n",
               p->funcInfo->name, (int)p->lastBegIdx, cg_begIdx);
        p->codegenError = TA_CODEGEN_BEGIDX_MISMATCH;
        return;
    }

    /* Compare outNBElement */
    int cg_nbElement = json_get_int(p->responseBuf, "outNBElement");
    if( p->lastNbElement != cg_nbElement )
    {
        printf("CODEGEN MISMATCH [TA_%s]: outNBElement C=%d codegen=%d\n",
               p->funcInfo->name, (int)p->lastNbElement, cg_nbElement);
        p->codegenError = TA_CODEGEN_NBELEMENT_MISMATCH;
        return;
    }

    /* Structural parity verified above; skip the VALUE diff for functions that
     * intentionally diverge from the frozen reference (#107 STOCHRSI, #242
     * CORREL).
     * NOT in the float leg (widenFloatInputs): there the baseline is the current
     * double variant, so TA_S_ vs TA_ self-consistency must stay strictly checked. */
    if( !p->widenFloatInputs && codegen_ref_value_exempt(p->funcInfo->name) )
        return;

    /* Compare output values for the requested outputNb */
    if( p->outputIsInteger[outputNb] )
    {
        /* Integer output comparison (exact match) */
        char fieldName[64];
        if( outputNb == 0 )
            snprintf(fieldName, sizeof(fieldName), "outInteger");
        else
            snprintf(fieldName, sizeof(fieldName), "outInteger%d", outputNb);

        TA_Integer cg_out[MAX_NB_TEST_ELEMENT];
        int parsed = json_get_int_array(p->responseBuf, fieldName,
                                         cg_out, MAX_NB_TEST_ELEMENT);
        for( int i = 0; i < p->lastNbElement && i < parsed; i++ )
        {
            if( p->outIntBufs[outputNb][i] != cg_out[i] )
            {
                printf("CODEGEN MISMATCH [TA_%s]: %s[%d] C=%d codegen=%d\n",
                       p->funcInfo->name, fieldName, i,
                       (int)p->outIntBufs[outputNb][i], (int)cg_out[i]);
                p->codegenError = TA_CODEGEN_OUTPUT_MISMATCH;
                return;
            }
        }
    }
    else
    {
        /* Real output comparison (epsilon) */
        char fieldName[64];
        if( outputNb == 0 )
            snprintf(fieldName, sizeof(fieldName), "outReal");
        else
            snprintf(fieldName, sizeof(fieldName), "outReal%d", outputNb);

        TA_Real cg_out[MAX_NB_TEST_ELEMENT];
        int parsed = json_get_double_array(p->responseBuf, fieldName,
                                            cg_out, MAX_NB_TEST_ELEMENT);
        for( int i = 0; i < p->lastNbElement && i < parsed; i++ )
        {
            double cVal = p->outRealBufs[outputNb][i];
            double diff = fabs(cVal - cg_out[i]);
            double threshold;
            if( p->widenFloatInputs )
            {
                /* Float leg: BOTH sides are the same server on the same
                 * float-widened inputs — its single-precision entry point vs its
                 * own double one — so equal computation must give equal doubles
                 * and the only spread is the transport (<1e-11 through %.15g;
                 * Java and C# serialise shortest-round-trip, i.e. exactly).
                 *
                 * The old 1e-6 here dated from when this leg compared against the
                 * frozen single-precision reference, which computed IN float.
                 * That rationale is gone, and 1e-6 is useless against the defect
                 * this leg exists for: one arithmetic op left in float is ~6e-8
                 * relative (float's own resolution is 2^-23 = 1.19e-7), i.e.
                 * BELOW the threshold. Use the same 1e-9 the double leg proved
                 * holds through this transport.
                 *
                 * epsilonScale is still honoured so a caller can widen it. */
                double scale = (p->epsilonScale > 0.0) ? p->epsilonScale : 1.0;
                threshold = CODEGEN_EPSILON_DOUBLE * fmax(1.0, fabs(cVal)) * scale;
            }
            else
            {
                /* Double leg, tightened 1e-6 -> 1e-9 (issue #113 follow-up).
                 * A full-precision measurement of every language server against the
                 * frozen reference found the cross-language / cross-version
                 * divergence is <1e-11 for all 161 functions EXCEPT LINEARREG_ANGLE
                 * (~4.4e-10, the authorized #103 O(1) sliding-sum recurrence vs the
                 * frozen O(n) recompute). The 1e-6 floor was never a %.15g-transport
                 * limit — the transport contributes <1e-11 — so 1e-9 holds with
                 * margin: 1e-9 absolute below 1, 1e-9 relative above. Bit-exact
                 * cross-language parity on seed data is separately gated by
                 * --xlang-hash. */
                threshold = CODEGEN_EPSILON_DOUBLE * fmax(1.0, fabs(cVal));
            }
            if( diff > threshold )
            {
                printf("CODEGEN MISMATCH [TA_%s]: %s[%d] C=%.10f codegen=%.10f diff=%.2e\n",
                       p->funcInfo->name, fieldName, i,
                       p->outRealBufs[outputNb][i], cg_out[i], diff);
                p->codegenError = TA_CODEGEN_OUTPUT_MISMATCH;
                return;
            }
        }
    }

    /* Parse server timing_ns if present */
    int len;
    const char *timingVal = json_find_field(p->responseBuf, "timing_ns", &len);
    if( timingVal )
    {
        long long serverNs = strtoll(timingVal, NULL, 10);
        if( outputNb == 0 && serverNs == 0 )
            fprintf(stderr, "DEBUG timing_ns=0 for TA_%s, raw='%.20s'\n", p->funcInfo->name, timingVal);
        p->server_total_ns += serverNs;
        p->timing_count++;
    }
    else if( outputNb == 0 )
    {
        /* Debug: show first 120 chars of response when timing_ns is missing */
        fprintf(stderr, "DEBUG no timing_ns for TA_%s: %.120s\n", p->funcInfo->name, p->responseBuf);
    }

}

/* ---- Edge-range server sweep ----
 * The full-range codegen comparison (and the doRangeTest sweep) exercise the
 * language servers only at the full range [0, nbBars-1]. That misses the
 * lookback-boundary corners where a composed indicator forms an empty/short
 * internal sub-range -- e.g. APO/PPO computing (fastMA - slowMA) while the slow
 * MA is still empty, or IMI's window at startIdx == lookback. In a release
 * server the resulting usize underflow wraps harmlessly (the wrapped value is
 * dead); in a DEBUG-profile server it panics on the overflow check.
 *
 * This sweep drives the server across short ranges near the lookback (startIdx 0,
 * endIdx 0..lookback+margin) and diffs each against ta_ref_serve, so:
 *   - release: adds value-coherency coverage at the lookback boundary, and
 *   - debug:   turns any arithmetic overflow/underflow into a hard failure
 *              (a server crash closes the pipe -> a non-PASS read here).
 *
 * Comparing ref-vs-server at the SAME (startIdx, endIdx) is always valid, so no
 * DO_NOT_COMPARE exemptions are needed: path-dependence (AD/OBV/SAR/...) only
 * affects cross-range coherency, which this does not test. */
#define EDGE_SWEEP_MARGIN 3
static double parse_ref_baseline(CodegenRangeTestParam *p);  /* defined below */

/* One (startIdx, endIdx) pair, reference against the server under test. Returns
 * 0 when the caller should stop (the error is already recorded and printed), 1
 * to carry on -- including when the range was skipped because the reference
 * itself answered an error. */
static int edge_compare_one(CodegenRangeTestParam *p, TA_Integer startIdx,
                            TA_Integer endIdx, int countAsStartSweep)
{
    build_json_request(p, startIdx, endIdx);

    /* Reference baseline (ta_ref_serve). */
    ErrorNumber rref = codegen_pipe_call(p->refCp, p->requestBuf,
                                         p->responseBuf, JSON_BUF_SIZE);
    if( rref != TA_TEST_PASS )
    {
        printf("EDGE SWEEP [TA_%s]: reference server call failed at (%d,%d)\n",
               p->funcInfo->name, (int)startIdx, (int)endIdx);
        p->codegenError = rref;
        return 0;
    }
    if( json_is_error(p->responseBuf) )
        return 1;  /* function unsupported / errored at this range */
    parse_ref_baseline(p);
    TA_Integer refNb = p->lastNbElement;

    /* Language server under test. A crash (e.g. a debug-build arithmetic
     * overflow) closes the pipe, so the read returns non-PASS here. */
    ErrorNumber rlang = codegen_pipe_call(p->cp, p->requestBuf,
                                          p->responseBuf, JSON_BUF_SIZE);
    if( rlang != TA_TEST_PASS )
    {
        printf("CODEGEN EDGE CRASH [TA_%s]: server stopped responding at "
               "range (%d,%d) -- likely a debug-build arithmetic overflow\n",
               p->funcInfo->name, (int)startIdx, (int)endIdx);
        p->codegenError = rlang;
        return 0;
    }

    /* The reference answered this exact request, so an error from a server that
     * answered the same function at the full range is a divergence, not an
     * unsupported-skip -- and compare_codegen_output_generic returns SILENTLY on
     * an error response, so without this the interesting failures land in the
     * one place nothing looks. */
    if( p->langSupported && json_is_error(p->responseBuf) )
    {
        printf("CODEGEN EDGE MISMATCH [TA_%s]: server error at range (%d,%d) "
               "where the reference produced a result: %.160s\n",
               p->funcInfo->name, (int)startIdx, (int)endIdx, p->responseBuf);
        p->codegenError = TA_CODEGEN_RETCODE_MISMATCH;
        return 0;
    }

    for( unsigned int o = 0; o < p->funcInfo->nbOutput; o++ )
        compare_codegen_output_generic(p, o);
    if( p->codegenError != TA_TEST_PASS )
    {
        printf("  (edge range was (%d,%d))\n", (int)startIdx, (int)endIdx);
        return 0;
    }
    if( countAsStartSweep && p->langIndex >= 0 && p->langIndex < (int)NUM_LANGUAGES )
    {
        g_startSweepCompared[p->langIndex]++;
        if( refNb > 0 )
            g_startSweepWithOutput[p->langIndex]++;
    }
    return 1;
}

static void run_edge_range_sweep(CodegenRangeTestParam *p)
{
    if( p->codegenError != TA_TEST_PASS )
        return;

    TA_Integer lookback = 0;
    if( TA_GetLookback(p->paramHolder, &lookback) != TA_SUCCESS || lookback < 0 )
        return;

    TA_Integer maxEnd = lookback + EDGE_SWEEP_MARGIN;
    if( maxEnd > p->nbBars - 1 )
        maxEnd = p->nbBars - 1;

    p->useLargePeriod = 0;
    for( TA_Integer endIdx = 0; endIdx <= maxEnd; endIdx++ )
        if( !edge_compare_one(p, 0, endIdx, 0) )
            return;

    /* ---- The startIdx axis (#236 step 2) ----
     * Every other server-facing call in this file pins startIdx to 0, and two
     * things are unreachable there.
     *
     * One: a guard that is wrong only when startIdx > 0. No suite and no
     * cross-language leg could see one -- that is the defect class this axis
     * exists for, and it is why the pairs below are not all at the lookback
     * boundary.
     *
     * Two: the CLAMP itself. At startIdx <= lookback the call begins at the
     * lookback and produces fewer values than the range is wide; above it, it
     * begins where it was asked to and produces exactly the range width. Both
     * arms of `max(startIdx, lookback)` need a case, and the pair that separates
     * a correct produced-count bound from a range-width one is the FIRST arm --
     * which startIdx 0 does reach, but only ever with the whole series, so a
     * server sizing its buffers to the range width was slack by exactly the
     * lookback and never approached the bound. Sized to the produced extent
     * (see the server emitters), each pair below is now a live test of it.
     *
     * Comparing ref-vs-server at the SAME (startIdx, endIdx) is valid whatever
     * the function's range-stability class, so no exemptions are needed here
     * either: path dependence affects cross-range coherency, which this does
     * not test. */
    {
        TA_Integer N = p->nbBars;
        TA_Integer pairs[6][2];
        int nPairs = 0, i, j;

        pairs[nPairs][0] = lookback > 0 ? lookback - 1 : 0;
        pairs[nPairs++][1] = N - 1;                   /* just below the clamp */
        pairs[nPairs][0] = lookback;
        pairs[nPairs++][1] = N - 1;                   /* exactly at the clamp */
        pairs[nPairs][0] = lookback + 1;
        pairs[nPairs++][1] = N - 1;                   /* just above it */
        pairs[nPairs][0] = N / 2;
        pairs[nPairs++][1] = N - 1;                   /* mid-series */
        pairs[nPairs][0] = N - 1;
        pairs[nPairs++][1] = N - 1;                   /* a single trailing bar */
        pairs[nPairs][0] = lookback + 1;
        pairs[nPairs++][1] = lookback + 2;            /* narrow, and not at 0 */

        for( i = 0; i < nPairs; i++ )
        {
            TA_Integer sIdx = pairs[i][0], eIdx = pairs[i][1];
            int dup = 0;

            if( sIdx < 1 || sIdx > N - 1 || eIdx > N - 1 || eIdx < sIdx )
                continue;   /* startIdx 0 is the sweep above; the rest is off the series */
            if( ref_diverges_on_partial_range(p->funcInfo->name, sIdx, lookback) )
            {
                g_startSweepSkipped98++;
                continue;
            }
            for( j = 0; j < i; j++ )
                if( pairs[j][0] == sIdx && pairs[j][1] == eIdx ) { dup = 1; break; }
            if( dup )
                continue;

            if( !edge_compare_one(p, sIdx, eIdx, 1) )
                return;
        }
    }
}

/* ---- Generic doRangeTest callback (Task 8) ---- */

static TA_RetCode codegen_range_generic(
    TA_Integer startIdx, TA_Integer endIdx,
    TA_Real *outputBuffer, TA_Integer *outputBufferInt,
    TA_Integer *outBegIdx, TA_Integer *outNbElement,
    TA_Integer *lookback, void *opaqueData,
    unsigned int outputNb, unsigned int *isOutputInteger)
{
    CodegenRangeTestParam *p = (CodegenRangeTestParam *)opaqueData;

    /* Unstable integer outputs (HT_TRENDMODE) go through the real-path
     * comparator, like the hand tests' FREE_INT_BUFFER conversion. */
    *isOutputInteger = (unsigned int)p->outputIsInteger[outputNb];
    if( p->outputIsInteger[outputNb] && p->unstId != TA_TEST_UNST_NONE )
        *isOutputInteger = 0;

    /* Get lookback */
    TA_GetLookback(p->paramHolder, lookback);

    /* Call TA_CallFunc for EVERY invocation (not just outputNb==0).
     * doRangeTest iterates all startIdx values for output 0, then all for
     * output 1, etc. — so the startIdx/endIdx differ between outputNb calls
     * and we cannot cache across them. */

    /* Re-point all output buffers (TA_CallFunc writes into these) */
    for( unsigned int i = 0; i < p->funcInfo->nbOutput; i++ )
    {
        if( p->outputIsInteger[i] )
            TA_SetOutputParamIntegerPtr(p->paramHolder, i, p->outIntBufs[i]);
        else
            TA_SetOutputParamRealPtr(p->paramHolder, i, p->outRealBufs[i]);
    }

    /* Call the C reference function via ta_abstract */
    p->lastRetCode = TA_CallFunc(p->paramHolder, startIdx, endIdx,
                                  &p->lastBegIdx, &p->lastNbElement);


    *outBegIdx = p->lastBegIdx;
    *outNbElement = p->lastNbElement;

    /* Copy the requested output into the doRangeTest buffer */
    if( p->lastRetCode == TA_SUCCESS && p->lastNbElement > 0 )
    {
        if( *isOutputInteger )
        {
            for( int i = 0; i < p->lastNbElement; i++ )
                outputBufferInt[i] = p->outIntBufs[outputNb][i];
        }
        else if( p->outputIsInteger[outputNb] )
        {
            for( int i = 0; i < p->lastNbElement; i++ )
                outputBuffer[i] = (TA_Real)p->outIntBufs[outputNb][i];
        }
        else
        {
            for( int i = 0; i < p->lastNbElement; i++ )
                outputBuffer[i] = p->outRealBufs[outputNb][i];
        }
    }

    /* Codegen comparison is done once in test_one_function (full range).
     * This callback only handles C reference coherency for doRangeTest. */

    return p->lastRetCode;
}

/* ---- Setup helpers (Task 9) ---- */

static void setup_inputs(TA_ParamHolder *paramHolder,
                         const TA_FuncInfo *funcInfo,
                         const TA_History *history)
{
    unsigned int i;
    int realInputCount = 0;

    for( i = 0; i < funcInfo->nbInput; i++ )
    {
        const TA_InputParameterInfo *inputInfo;
        TA_GetInputParameterInfo(funcInfo->handle, i, &inputInfo);

        switch( inputInfo->type )
        {
        case TA_Input_Price:
            TA_SetInputParamPricePtr(paramHolder, i,
                inputInfo->flags & TA_IN_PRICE_OPEN         ? history->open : NULL,
                inputInfo->flags & TA_IN_PRICE_HIGH         ? history->high : NULL,
                inputInfo->flags & TA_IN_PRICE_LOW          ? history->low  : NULL,
                inputInfo->flags & TA_IN_PRICE_CLOSE        ? history->close : NULL,
                inputInfo->flags & TA_IN_PRICE_VOLUME       ? history->volume : NULL,
                inputInfo->flags & TA_IN_PRICE_OPENINTEREST ? history->openInterest : NULL);
            break;

        case TA_Input_Real:
        {
            const TA_Real *data;
            if( realInputCount == 0 )
                data = history->close;
            else if( realInputCount == 1 )
                data = history->volume;
            else
                data = history->close;  /* fallback */
            TA_SetInputParamRealPtr(paramHolder, i, data);
            realInputCount++;
            break;
        }
        case TA_Input_Integer:
            /* Integer inputs are rare; pass NULL -- TA_CallFunc will
             * return an error for these functions, which is fine. */
            break;
        }
    }
}

/* Optional inputs are left at defaults (TA_ParamHolderAlloc sets them). */

static void setup_outputs(CodegenRangeTestParam *p)
{
    unsigned int i;

    memset(p->outputIsInteger, 0, sizeof(p->outputIsInteger));
    memset(p->outRealBufs, 0, sizeof(p->outRealBufs));
    memset(p->outIntBufs, 0, sizeof(p->outIntBufs));
    p->totalOutputs = 0;

    for( i = 0; i < p->funcInfo->nbOutput && i < MAX_OUTPUTS; i++ )
    {
        const TA_OutputParameterInfo *outputInfo;
        TA_GetOutputParameterInfo(p->funcInfo->handle, i, &outputInfo);

        switch( outputInfo->type )
        {
        case TA_Output_Real:
            p->outputIsInteger[i] = 0;
            p->outRealBufs[i] = (TA_Real *)calloc(MAX_NB_TEST_ELEMENT, sizeof(TA_Real));
            TA_TOOL_CHECK_ALLOC(p->outRealBufs[i]);
            TA_SetOutputParamRealPtr(p->paramHolder, i, p->outRealBufs[i]);
            break;
        case TA_Output_Integer:
            p->outputIsInteger[i] = 1;
            p->outIntBufs[i] = (TA_Integer *)calloc(MAX_NB_TEST_ELEMENT, sizeof(TA_Integer));
            TA_TOOL_CHECK_ALLOC(p->outIntBufs[i]);
            TA_SetOutputParamIntegerPtr(p->paramHolder, i, p->outIntBufs[i]);
            break;
        }
        p->totalOutputs++;
    }
}

static void free_outputs(CodegenRangeTestParam *p)
{
    for( int i = 0; i < p->totalOutputs; i++ )
    {
        free(p->outRealBufs[i]);
        p->outRealBufs[i] = NULL;
        free(p->outIntBufs[i]);
        p->outIntBufs[i] = NULL;
    }
}

/* ---- Determine integer tolerance for doRangeTest ---- */

static unsigned int get_integer_tolerance(const TA_FuncInfo *funcInfo)
{
    /* Compare values by default (#98: DO_NOT_COMPARE-for-all hid the TRIX
     * mislabeling for two decades). Exceptions: accumulations seeded at
     * startIdx and path-dependent state machines cannot converge.
     * Tolerances mirror the hand-written tests (test_adx.c, test_1in_*.c),
     * extended to the whole Wilder family for sampling robustness. */
    /* The start-dependent set is declared at the function definition site by the
     * `path_dependent` YAML flag (issue #127), surfaced through ta_abstract as
     * TA_FUNC_FLG_PATH_DEP — a single source of truth read here from the same
     * flags every ta_abstract consumer sees, no hand-edited second list for each
     * new cumulative / seed-anchored indicator. */
    static const struct { const char *name; unsigned int tol; } perFuncTol[] = {
        { "MINUS_DI", 2 }, { "PLUS_DI", 2 }, { "DX", 2 },
        { "ADX", 2 }, { "ADXR", 2 },
        { "ATR", 2 }, { "NATR", 2 }, { "RSI", 2 }, { "CMO", 2 },
        { "MACD", 10 }, { "MACDEXT", 10 }, { "MACDFIX", 10 },
        { "HT_DCPHASE", 360 },
        { "HT_SINE", 10 },
    };
    if( funcInfo->flags & TA_FUNC_FLG_PATH_DEP )
        return TA_DO_NOT_COMPARE;
    for( unsigned int i = 0; i < sizeof(perFuncTol)/sizeof(perFuncTol[0]); i++ )
    {
        if( strcmp(funcInfo->name, perFuncTol[i].name) == 0 )
            return perFuncTol[i].tol;
    }
    return 0;
}

/* ---- Determine range-stability class for doRangeTestEx ---- */

/* Explicit per-function range-test tolerance class (see TA_RangeStability).
 * This REPLACES the old implicit "unstId == NONE ? tight : loose" inference:
 * the tolerance now follows each function's numerical nature, not merely whether
 * it carries an unstable-period id.
 *
 *   SKIP       - accumulation seeded at startIdx / path-dependent state machine
 *                (derived from get_integer_tolerance()==DO_NOT_COMPARE, the same
 *                single source the integer-output skip uses -- never a 2nd list).
 *   EXACT      - fresh-recomputed finite window; bit-exact across ranges.
 *   EPSILON    - finite window with running-accumulator / reorder FP drift, and
 *                the default for anything without an unstable period.
 *   CONVERGING - carries an unstable-period id (recursive / IIR).
 *
 * SKIP, then the explicit EXACT/EPSILON entries, are checked BEFORE the
 * unstId-derived CONVERGING, so a vestigial unstable-period id (the IMI/MFI trap)
 * trips the guard in doRangeTestEx instead of silently drawing the loose
 * tolerance. */
static TA_RangeStability stability_class(const TA_FuncInfo *funcInfo)
{
    /* SKIP is NOT maintained as a second list here: it is exactly the set that
     * get_integer_tolerance() marks TA_DO_NOT_COMPARE -- the functions carrying
     * the `path_dependent` YAML flag (accumulations seeded at startIdx and
     * path-dependent state machines), surfaced through ta_abstract as
     * TA_FUNC_FLG_PATH_DEP (issue #127).
     * Deriving it from that single source guarantees the real-output skip (this
     * class, via dataWithinReasonableRange) and the integer-output skip
     * (doRangeTestFixSize, keyed on the same DO_NOT_COMPARE) can never drift
     * apart. Checked first: a skipped function is skipped whatever else it is. */
    if( get_integer_tolerance(funcInfo) == TA_DO_NOT_COMPARE )
        return TA_STABLE_SKIP;

    /* Fresh-recomputed finite window -> bit-exact across ranges: every output bar
     * rebuilds its result from the raw input window (or one input element) with
     * NO floating-point total carried across bars, so the value is independent of
     * startIdx. Audited function-by-function from the input .c sources and then
     * confirmed by this range gate running at the strict TA_STABLE_EXACT (==)
     * tolerance. (IMI is the archetype, issue #14.) */
    static const char *exact[] = {
        /* per-element vector math / unary transforms */
        "ACOS", "ASIN", "ATAN", "CEIL", "COS", "COSH", "EXP", "FLOOR", "LN",
        "LOG10", "SIN", "SINH", "SQRT", "TAN", "TANH", "ADD", "SUB", "MULT", "DIV",
        /* per-bar price transforms */
        "AVGPRICE", "MEDPRICE", "TYPPRICE", "WCLPRICE", "BOP", "TRANGE",
        /* per-bar momentum (difference / ratio of two bars) */
        "MOM", "ROC", "ROCP", "ROCR", "ROCR100",
        /* comparison-selected window extrema (cached min/max, no FP accumulation) */
        "MIN", "MAX", "MINMAX", "MIDPOINT", "MIDPRICE", "WILLR", "AROON", "AROONOSC",
        /* fresh per-bar rescan (window re-summed in bar-absolute order each output) */
        "AVGDEV",
        /* fresh sliding window, no accumulator */
        "IMI",
        /* NOTE: LINEARREG / LINEARREG_ANGLE / LINEARREG_INTERCEPT / LINEARREG_SLOPE
         * / TSF moved OUT of EXACT to the EPSILON default (perf #103): they now
         * carry SumY/SumXY in an O(1) sliding recurrence instead of re-summing the
         * window each bar, so their output picks up ~1e-9 running-accumulator drift
         * across ranges -- the same class as SMA/CORREL/STDDEV. */
    };

    /* Finite window carried in a RUNNING ACCUMULATOR (running sum/total updated
     * add-head/subtract-trailing, or MA-based at the default SMA type) -> differs
     * only by ~1e-9 FP rounding across ranges. This is also the default for any
     * function not listed above, so the array need only carry MFI (issue #4) as a
     * documented archetype. Audited running-accumulator functions that rely on
     * that default: ACCBANDS, BBANDS, BETA, CCI, CORREL, MA, MAVP, SMA,
     * STDDEV, STOCH, STOCHF, SUM, TRIMA, ULTOSC, VAR, WMA. MACDEXT — and now
     * APO/PPO (issue #120) — stay CONVERGING: they are EPSILON at the SMA MA type
     * but carry the EMA unstable-period id (via UNSTABLE_MAP), so the convergence
     * envelope covers every MA-type sweep. APO/PPO default to EMA (their default
     * sweep is genuinely converging); MACDEXT defaults to SMA and is listed on the
     * safe-superset basis (converging only for its EMA-type parameterisations). */
    static const char *epsilon[] = { "MFI" };

    for( unsigned int i = 0; i < sizeof(exact)/sizeof(exact[0]); i++ )
        if( strcmp(funcInfo->name, exact[i]) == 0 )
            return TA_STABLE_EXACT;
    for( unsigned int i = 0; i < sizeof(epsilon)/sizeof(epsilon[0]); i++ )
        if( strcmp(funcInfo->name, epsilon[i]) == 0 )
            return TA_STABLE_EPSILON;

    if( get_unst_id(funcInfo->name) != TA_TEST_UNST_NONE )
        return TA_STABLE_CONVERGING;

    /* Default: a finite-window function without an unstable period compares at
     * the tight epsilon tolerance (identical to the pre-explicit behaviour). */
    return TA_STABLE_EPSILON;
}


/* ---- Filter helper ---- */

static int codegen_matches_filter(const char *filter, const char *name)
{
    char filterCopy[1024];
    char *token;
    if( filter == NULL ) return 1;
    strncpy(filterCopy, filter, sizeof(filterCopy) - 1);
    filterCopy[sizeof(filterCopy) - 1] = '\0';
    token = strtok(filterCopy, ",");
    while( token != NULL )
    {
        if( strstr(name, token) != NULL ) return 1;
        token = strtok(NULL, ",");
    }
    return 0;
}

/* ---- Reference-as-server baseline (task #7) ----
 * Parse a ta_ref_serve JSON-RPC response into the same baseline fields that
 * compare_codegen_output_generic() diffs each language server against. This
 * replaces the former in-process TA_CallFunc baseline so that post-cutover
 * (when src/ta_func is the generated code) the reference comes from the frozen
 * reference library exposed as a server, not from an in-process call that would
 * be the generated code comparing against itself. Field names mirror
 * compare_codegen_output_generic() exactly (output 0 has no numeric suffix).
 * Returns the server's timing_ns (raw indicator time) for the C-ref column. */
static double parse_ref_baseline(CodegenRangeTestParam *p)
{
    p->lastRetCode   = (TA_RetCode)json_get_int(p->responseBuf, "retCode");
    p->lastBegIdx    = json_get_int(p->responseBuf, "outBegIdx");
    p->lastNbElement = json_get_int(p->responseBuf, "outNBElement");

    if( p->lastRetCode == TA_SUCCESS && p->lastNbElement > 0 )
    {
        for( unsigned int o = 0; o < p->funcInfo->nbOutput; o++ )
        {
            char fieldName[64];
            if( p->outputIsInteger[o] )
            {
                if( o == 0 ) snprintf(fieldName, sizeof(fieldName), "outInteger");
                else         snprintf(fieldName, sizeof(fieldName), "outInteger%d", (int)o);
                json_get_int_array(p->responseBuf, fieldName,
                                   p->outIntBufs[o], MAX_NB_TEST_ELEMENT);
            }
            else
            {
                if( o == 0 ) snprintf(fieldName, sizeof(fieldName), "outReal");
                else         snprintf(fieldName, sizeof(fieldName), "outReal%d", (int)o);
                json_get_double_array(p->responseBuf, fieldName,
                                      p->outRealBufs[o], MAX_NB_TEST_ELEMENT);
            }
        }
    }

    int len;
    const char *t = json_find_field(p->responseBuf, "timing_ns", &len);
    return t ? (double)strtoll(t, NULL, 10) : 0.0;
}

/* ---- Per-function callback for TA_ForEachFunc (Task 9) ---- */

typedef struct {
    const TA_History *history;
    const char       *functionFilter;
    CodegenPipe      *cp;
    CodegenPipe      *refCp;       /* ta_ref_serve oracle (shared across languages) */
    const char       *refFuncList; /* ta_ref_serve list_functions payload (subset
                                    * gate: skip functions the frozen reference
                                    * lacks — post-tag additions have no baseline) */
    char             *requestBuf;
    char             *responseBuf;
    ErrorNumber       error;
    int               passed;
    int               failed;
    int               skipped;
    /* Names behind the aggregate `skipped` count. An unnamed "N skipped" reads
     * as noise; naming them is what makes a post-cutover addition's reduced
     * coverage visible at a glance (issue #137). */
    char              skipNames[MAX_FUNCTIONS][20];
    int               nbSkipNames;
    char              intInputSkipNames[MAX_FUNCTIONS][20];
    int               nbIntInputSkipNames;
    /* sweep_one_function has its own subset gate that used to `return` with no
     * counter at all — sweep skips were invisible even in the aggregate. */
    int               sweepSkipped;
    /* Post-cutover functions that reached the range-stability leg. Counted so
     * "post-cutover" cannot quietly come to mean "range-unverified" again. */
    int               postCutRangeChecked;
    /* Of those, the ones whose class actually compared VALUES across ranges.
     * TA_STABLE_SKIP reaches the leg and checks coherency only, so counting it
     * as "verified" overstates the ratchet below -- and the inert set grows
     * with every new path-dependent indicator (NVI, PVI, WAD today). */
    int               postCutRangeValueCompared;
    int               langIndex;   /* index into ALL_LANGUAGES */
    const CodegenLanguage *lang;
    /* Ref differential sweep counters */
    int               sweepVariants;
    int               sweepFunctions;
    /* Stream verification counters */
    int               streamFunctions;
    int               streamLegs;
    int               streamSkipped;
    int               streamRejectArms;
    int               streamFillFunctions; /* funcs whose OpenAndFill == batch(0,n-1) bitwise */
    int               streamUFillFunctions;/* funcs whose UpdateAndFill == batch over the same bars (#246) */
    int               streamStateFunctions; /* funcs whose handle state matched Open(n) (#240) */
    int               streamStateLegs;      /* legs that compared handle state (#240) */
    int               streamRangeFunctions; /* funcs whose handle OutRange matched batch (#241) */
    int               streamRangeLegs;      /* legs that compared the handle's OutRange (#241) */
    int               streamRangeSites;     /* OR of the range-compare sites that fired (#241) */
    int               streamRangeSitesN;    /* how many sites this server says it has */
    long long         streamBenign;        /* cross-tier +0.0/-0.0 pairs (#147) — never a failure */
} ForEachFuncContext;

static void test_one_function(const TA_FuncInfo *funcInfo, void *opaqueData)
{
    ForEachFuncContext *ctx = (ForEachFuncContext *)opaqueData;

    /* Stop iterating if we already hit an error */
    if( ctx->error != TA_TEST_PASS )
        return;

    /* Apply function filter */
    if( !codegen_matches_filter(ctx->functionFilter, funcInfo->name) )
        return;

    /* Subset gate: the comparison baseline is the FROZEN ta_ref_serve, so a
     * function added after the pinned reference tag has no baseline there
     * (ta_ref_serve omits it from list_functions and stubs its symbol — see
     * scripts/serve_version.py). Skip it rather than hard-fail on the missing
     * baseline; it stays covered by server_verify, --xlang-hash and its
     * hard-coded tests. Mirrors the --fuzz-064 subset gate. */

    /* Skip functions with integer inputs (very rare, no test data) */
    unsigned int hasIntegerInput = 0;
    for( unsigned int i = 0; i < funcInfo->nbInput; i++ )
    {
        const TA_InputParameterInfo *inputInfo;
        TA_GetInputParameterInfo(funcInfo->handle, i, &inputInfo);
        if( inputInfo->type == TA_Input_Integer )
        {
            hasIntegerInput = 1;
            break;
        }
    }
    if( hasIntegerInput )
    {
        /* Record skip in global table */
        int ridx = -1;
        for( int i = 0; i < g_numTimingResults; i++ )
        {
            if( strcmp(g_timingResults[i].funcName, funcInfo->name) == 0 )
            {
                ridx = i;
                break;
            }
        }
        if( ridx < 0 && g_numTimingResults < MAX_FUNCTIONS )
        {
            ridx = g_numTimingResults++;
            memset(&g_timingResults[ridx], 0, sizeof(FuncTimingResult));
            strncpy(g_timingResults[ridx].funcName, funcInfo->name,
                    sizeof(g_timingResults[ridx].funcName) - 1);
        }
        /* langs[langIndex].tested stays 0 (skipped) */
        if( ctx->nbIntInputSkipNames < MAX_FUNCTIONS )
            strncpy(ctx->intInputSkipNames[ctx->nbIntInputSkipNames++], funcInfo->name,
                    sizeof(ctx->intInputSkipNames[0]) - 1);
        ctx->skipped++;
        return;
    }

    /* Reset all unstable periods to 0 — doRangeTest leaves them at
     * high values which contaminates subsequent functions. */
    TA_SetUnstablePeriod(TA_FUNC_UNST_ALL, 0);

    printf("  %-40s ", funcInfo->name);
    fflush(stdout);

    /* Allocate param holder */
    TA_ParamHolder *paramHolder = NULL;
    TA_RetCode retCode = TA_ParamHolderAlloc(funcInfo->handle, &paramHolder);
    if( retCode != TA_SUCCESS )
    {
        printf("FAILED (ParamHolderAlloc: %d)\n", retCode);
        ctx->error = TA_CODEGEN_ALLOC_FAILED;
        return;
    }

    /* Build CodegenRangeTestParam */
    CodegenRangeTestParam params;
    memset(&params, 0, sizeof(params));
    params.funcInfo    = funcInfo;
    params.paramHolder = paramHolder;
    params.history     = ctx->history;
    params.nbBars      = (int)ctx->history->nbBars;
    params.unstId      = get_unst_id(funcInfo->name);
    params.cp          = ctx->cp;
    params.langName    = ctx->lang->name;
    params.langIndex   = ctx->langIndex;
    params.refCp       = ctx->refCp;
    params.requestBuf  = ctx->requestBuf;
    params.responseBuf = ctx->responseBuf;
    params.codegenError = TA_TEST_PASS;

    /* Set up inputs (using defaults for optional params) */
    setup_inputs(paramHolder, funcInfo, ctx->history);

    /* Set up output buffers */
    setup_outputs(&params);

    /* Frozen-reference subset gate, applied HERE rather than on entry.
     *
     * The functions added after the pinned reference tag have no ta_ref_serve
     * baseline, so everything below that diffs against it must be skipped. The
     * run names them rather than counting them, because the set only grows.
     *
     * Two legs must NOT be skipped, and both are here for the same reason --
     * they compare against something other than the frozen reference, so the
     * missing baseline is irrelevant to them:
     *
     *   FLOAT — compares a language's single-precision entry point against that
     *   same language's own double entry point; never touches refCp. Skipping it
     *   left 14 shipped float entry points (7 Java, 7 C#) with no value
     *   verification at all.
     *
     *   RANGE STABILITY — codegen_range_generic calls TA_CallFunc on the
     *   in-process library, never a server at all. Skipping it left every
     *   post-cutover function with no startIdx-stability coverage, which is how
     *   PVO's missing UNSTABLE_MAP row survived from #119.
     *
     * Run both, then skip the reference-dependent remainder. */
    if( ctx->refFuncList )
    {
        char needle[80];
        snprintf(needle, sizeof(needle), "\"TA_%s\"", funcInfo->name);
        if( !strstr(ctx->refFuncList, needle) )
        {
            if( strcmp(ctx->lang->name, "rust") != 0 )
                run_float_leg(&params, 1);
            if( params.codegenError != TA_TEST_PASS )
            {
                printf("CODEGEN FAILED (code=%d)  (TA_%s is post-reference: "
                       "only the float leg ran)\n",
                       params.codegenError, funcInfo->name);
                ctx->failed++;
                ctx->error = params.codegenError;
                return;
            }
            /* The RANGE-STABILITY leg must not be skipped here either, for the
             * same reason as the float leg above: codegen_range_generic calls
             * TA_CallFunc on the in-process library and never touches refCp or
             * the language server, so it needs no frozen baseline. Leaving it
             * below this gate left every post-cutover function with no
             * startIdx-stability coverage at all -- which is how PVO's missing
             * UNSTABLE_MAP row survived since #119. */
            {
                TA_Integer postLookback = 0;
                if( TA_GetLookback( paramHolder, &postLookback ) == TA_SUCCESS &&
                    (int)ctx->history->nbBars > postLookback )
                {
                    TA_RangeStability postStability = stability_class(funcInfo);
                    ErrorNumber rangeErr = doRangeTestEx(
                        codegen_range_generic,
                        postStability,
                        get_unst_id(funcInfo->name),
                        (void *)&params,
                        funcInfo->nbOutput,
                        get_integer_tolerance(funcInfo));
                    if( rangeErr != TA_TEST_PASS )
                    {
                        printf("RANGE TEST FAILED (code=%d)  (TA_%s is "
                               "post-reference)\n", rangeErr, funcInfo->name);
                        ctx->failed++;
                        ctx->error = rangeErr;
                        return;
                    }
                    ctx->postCutRangeChecked++;
                    /* Reaching the leg is what the ratchet proves; comparing
                     * values is a strictly stronger thing that TA_STABLE_SKIP
                     * does not do. Split the two rather than let one number
                     * claim both. */
                    if( postStability != TA_STABLE_SKIP )
                        ctx->postCutRangeValueCompared++;
                }
            }
            if( ctx->nbSkipNames < MAX_FUNCTIONS )
                strncpy(ctx->skipNames[ctx->nbSkipNames++], funcInfo->name,
                        sizeof(ctx->skipNames[0]) - 1);
            ctx->skipped++;
            return;
        }
    }

    /* ---- Baseline from ta_ref_serve (reference-as-server, task #7) ----
     * The codegen comparison baseline is the reference C library exposed as a
     * JSON-RPC server, NOT an in-process call. ta_ref_serve links the frozen
     * pinned-tag reference and speaks the same protocol, so one request drives
     * both it and the language server under test. Post-cutover this keeps the
     * generated C server diffed against a frozen reference, not against itself.
     * (doRangeTest below still calls the in-process lib for self-coherency.) */
    /* This leg — the full-range value comparison and its baseline — is the one
     * that sends an OVER-SIZED output. See CodegenRangeTestParam::outPad. */
    params.outPad = OUT_SLACK_PAD;
    build_json_request(&params, 0, params.nbBars - 1);
    /* Warmup (discard) then measured baseline call (same request). */
    codegen_pipe_call(params.refCp, params.requestBuf, params.responseBuf, JSON_BUF_SIZE);
    ErrorNumber refErr = codegen_pipe_call(params.refCp, params.requestBuf,
                                           params.responseBuf, JSON_BUF_SIZE);
    if( refErr != TA_TEST_PASS || json_is_error(params.responseBuf) )
    {
        printf("FAILED (ta_ref_serve: %s for TA_%s)\n",
               refErr != TA_TEST_PASS ? "call failed" : "no result",
               funcInfo->name);
        free_outputs(&params);
        TA_ParamHolderFree(paramHolder);
        ctx->error = (refErr != TA_TEST_PASS) ? refErr : TA_CODEGEN_RETCODE_MISMATCH;
        ctx->failed++;
        return;
    }
    /* The reference is the frozen pre-cutover server and does not report
     * `out_len`; only the language server under test does, further down. */
    double c_avg_ns = parse_ref_baseline(&params);
    params.c_ref_total_ns = (long long)c_avg_ns;
    /* Default-period element count, captured for the doRangeTest guard below
     * (params.lastNbElement is overwritten by the large-period pass). */
    TA_Integer nbElem = params.lastNbElement;

    /* Warmup call: discard the first call to eliminate cold-start effects
     * (JVM class loading, Rust monomorphization, CPU cache priming, etc.) */
    build_json_request(&params, 0, params.nbBars - 1);
    codegen_pipe_call(params.cp, params.requestBuf,
                      params.responseBuf, JSON_BUF_SIZE);

    /* Codegen comparison: one full-range JSON-RPC call, compare all outputs.
     * This is done BEFORE doRangeTest to separate concerns:
     * - codegen comparison: does generated code match C reference?
     * - range test: is the C function coherent across sub-ranges?
     */
    build_json_request(&params, 0, params.nbBars - 1);
    ErrorNumber codegenErr = codegen_pipe_call(params.cp, params.requestBuf,
                                               params.responseBuf, JSON_BUF_SIZE);
    if( codegenErr != TA_TEST_PASS )
    {
        params.codegenError = codegenErr;
    }
    else
    {
        for( unsigned int outNb = 0; outNb < funcInfo->nbOutput; outNb++ )
            compare_codegen_output_generic(&params, outNb);
    }

    /* Back to the produced count for every leg below: the edge sweep and the
     * startIdx axis are what make the bound reachable, and a pad would undo
     * exactly that. */
    params.outPad = 0;

    /* Did the server actually allocate more than it produced? `out_len` is what
     * it allocated; `lastNbElement` is what it wrote. Counted here, off the
     * response, so the floor below tests the effect and not this file's
     * intention. A server that ignored `out_pad` reports the two as equal and
     * the floor fires. */
    if( codegenErr == TA_TEST_PASS && !json_is_error(params.responseBuf)
        && ctx->langIndex >= 0 && ctx->langIndex < (int)NUM_LANGUAGES )
    {
        int outLen = json_get_int(params.responseBuf, "out_len");
        if( outLen > params.lastNbElement )
            g_slackCalls[ctx->langIndex]++;
    }

    /* Whether the backend supported this function at the default period (non-error
     * response). Used so the large-period pass can flag a server error that appears
     * ONLY at the large period as a real regression, not an unsupported-skip. */
    int defaultSupported = (codegenErr == TA_TEST_PASS)
                           && !json_is_error(params.responseBuf);
    params.langSupported = defaultSupported;

    /* Snapshot server timing from the full-range comparison call. Both c_ref_ns
     * (ta_ref_serve) and s_avg_ns are single full-range JSON-RPC calls measuring
     * the raw indicator time server-side — apples-to-apples. */
    double s_avg_ns = (params.timing_count > 0)
                      ? (double)params.server_total_ns / (double)params.timing_count
                      : 0.0;

    /* ---- Float-variant pass ----
     * Every function at default params: the language's single-precision entry
     * point against its OWN double entry point on float-widened inputs. That is
     * PR #33's contract, it needs no oracle, and it holds per language.
     *
     * C, Java and C# all ship a float surface (TA_S_<N>, the float[] overload of
     * the Java core, the float[] overload of the C# core) — 168 functions each.
     * Rust is concrete f64 and has none, so it is the only exclusion.
     *
     * This ran C-only until the Java and C# servers gained a float path. While
     * it did, a k-factor defect sat in all three float surfaces and only C's was
     * reachable by any gate.
     *
     * withSentinel=1: this is the pass that reaches Java and C#, so it is where
     * the default-parameter sentinel has to be sent (issue #170). It runs after
     * the timing snapshot above, so the extra calls do not skew the reported ns.
     */
    if( strcmp(ctx->lang->name, "rust") != 0 )
        g_floatCapableLangTested = 1;
    if( params.codegenError == TA_TEST_PASS && strcmp(ctx->lang->name, "rust") != 0 )
        run_float_leg(&params, 1);

    /* ---- Large-period pass (Task 10) ----
     * Re-run the codegen value comparison with every IntegerRange opt param pushed
     * above the historical CIRCBUF static-buffer sizes (50/30), so period-dependent
     * buffer regressions (the MFI/CCI overflow class) are caught. Runs after the
     * timing snapshot (no skew); periods are restored before doRangeTest. Skipped
     * when the default pass already failed. Note: an indicator whose large-period
     * lookback exceeds the test history (e.g. high EMA-multiplier functions like T3)
     * produces no output and is skipped here — such functions have no period-sized
     * buffer, so the overflow class does not apply to them. */
    if( params.codegenError == TA_TEST_PASS )
    {
        int nLarge = set_large_opt_periods(paramHolder, funcInfo, params.nbBars);
        if( nLarge > 0 )
        {
            /* Large-period baseline also comes from ta_ref_serve; the same request
             * (built with useLargePeriod) then drives the language server. */
            params.useLargePeriod = 1;
            build_json_request(&params, 0, params.nbBars - 1);
            ErrorNumber lref = codegen_pipe_call(params.refCp, params.requestBuf,
                                                 params.responseBuf, JSON_BUF_SIZE);
            if( lref != TA_TEST_PASS )
            {
                params.codegenError = lref;
            }
            else if( !json_is_error(params.responseBuf) )
            {
                parse_ref_baseline(&params);
                if( params.lastNbElement > 0 )
                {
                    ErrorNumber le = codegen_pipe_call(params.cp, params.requestBuf,
                                                       params.responseBuf, JSON_BUF_SIZE);
                    if( le != TA_TEST_PASS )
                        params.codegenError = le;
                    else if( defaultSupported && json_is_error(params.responseBuf) )
                    {
                        /* Reference produced output at this period but the backend
                         * errored only at the large period -- a real divergence, not
                         * an unsupported-skip. */
                        printf("CODEGEN MISMATCH [TA_%s]: large-period (lnb=%d) server "
                               "error where C reference succeeded\n",
                               funcInfo->name, (int)params.lastNbElement);
                        params.codegenError = TA_CODEGEN_RETCODE_MISMATCH;
                    }
                    else
                        for( unsigned int o = 0; o < funcInfo->nbOutput; o++ )
                            compare_codegen_output_generic(&params, o);
                }
            }
            /* else: reference produced no result at the large period (e.g. lookback
             * exceeds the test history) — nothing to compare, like the old lnb==0. */
            params.useLargePeriod = 0;
        }
        /* set_large_opt_periods mutated the holder (for every IntegerRange param,
         * even when nLarge==0); always restore so doRangeTest and the next function
         * run at the default period. */
        reset_opt_periods_to_default(paramHolder, funcInfo);
    }

    /* Edge-range server sweep: drive the server across short ranges near the
     * lookback and diff each against ta_ref_serve (see run_edge_range_sweep).
     * Runs at default params, after the large-period restore above. */
    run_edge_range_sweep(&params);

    /* Run doRangeTest with the generic callback (C reference coherency only).
     * Skip when lookback exceeds data range (no output possible) or the edge
     * sweep already failed. */
    ErrorNumber errNb = TA_TEST_PASS;
    if( nbElem > 0 && params.codegenError == TA_TEST_PASS )
    {
        errNb = doRangeTestEx(
            codegen_range_generic,
            stability_class(funcInfo),
            get_unst_id(funcInfo->name),
            (void *)&params,
            funcInfo->nbOutput,
            get_integer_tolerance(funcInfo));
    }

    /* Record results in global timing table */
    int resultIdx = -1;
    for( int i = 0; i < g_numTimingResults; i++ )
    {
        if( strcmp(g_timingResults[i].funcName, funcInfo->name) == 0 )
        {
            resultIdx = i;
            break;
        }
    }
    if( resultIdx < 0 && g_numTimingResults < MAX_FUNCTIONS )
    {
        resultIdx = g_numTimingResults++;
        memset(&g_timingResults[resultIdx], 0, sizeof(FuncTimingResult));
        strncpy(g_timingResults[resultIdx].funcName, funcInfo->name,
                sizeof(g_timingResults[resultIdx].funcName) - 1);
        g_timingResults[resultIdx].c_ref_ns = c_avg_ns;
    }

    /* Check for codegen mismatch */
    if( params.codegenError != TA_TEST_PASS )
    {
        printf("CODEGEN FAILED (code=%d)\n", params.codegenError);
        if( resultIdx >= 0 && ctx->langIndex < (int)NUM_LANGUAGES )
        {
            g_timingResults[resultIdx].langs[ctx->langIndex].tested  = -1;
            g_timingResults[resultIdx].langs[ctx->langIndex].avg_ns  = s_avg_ns;
        }
        free_outputs(&params);
        TA_ParamHolderFree(paramHolder);
        ctx->failed++;
        ctx->error = params.codegenError;
        return;
    }

    if( errNb != TA_TEST_PASS )
    {
        printf("RANGE TEST FAILED (code=%d)\n", errNb);
        if( resultIdx >= 0 && ctx->langIndex < (int)NUM_LANGUAGES )
        {
            g_timingResults[resultIdx].langs[ctx->langIndex].tested  = -1;
            g_timingResults[resultIdx].langs[ctx->langIndex].avg_ns  = s_avg_ns;
        }
        free_outputs(&params);
        TA_ParamHolderFree(paramHolder);
        ctx->failed++;
        ctx->error = errNb;
        return;
    }

    /* Mark pass in global table */
    if( resultIdx >= 0 && ctx->langIndex < (int)NUM_LANGUAGES )
    {
        g_timingResults[resultIdx].langs[ctx->langIndex].tested  = 1;
        g_timingResults[resultIdx].langs[ctx->langIndex].avg_ns  = s_avg_ns;
    }

    /* Print result with timing and speedup ratio */
    if( s_avg_ns > 0 && c_avg_ns > 0 )
    {
        double ratio = c_avg_ns / s_avg_ns;
        printf("PASS   (c-ref: %.0fns, %s: %.0fns, %.2fx %s)\n",
               c_avg_ns, ctx->lang->display, s_avg_ns,
               (ratio >= 1.0) ? ratio : 1.0 / ratio,
               (ratio >= 1.0) ? "faster" : "slower");
    }
    else
    {
        printf("PASS\n");
    }
    ctx->passed++;

    free_outputs(&params);
    TA_ParamHolderFree(paramHolder);
}


/* ---- Ref differential sweep (#94 groundwork) ----
 * The default and large-period passes above diff each language server against
 * ta_ref_serve at two parameter points per function. This sweep broadens the
 * sample: every IntegerRange param at a few non-default values, every
 * IntegerList (MAType) value, RealRange params at their suggested bounds,
 * plus a Metastock-compatibility pass and an unstable-period pass at the
 * defaults. Purely differential: for every variant both servers must agree on
 * retCode, outBegIdx, outNBElement and every output value.
 *
 * Integer periods are floored at max(min, 2): period=1 is the intentional
 * divergence from the frozen reference fixed for #48/#59 (the reference is
 * wrong there), and that territory is owned by the PERIOD1/BOUNDARY
 * hand-written group with its own pinned expected values.
 */

/* SWEEP_MAX_OPT is defined with CodegenRangeTestParam, which sizes
 * optOverride[] by it — the two were always coupled (sweep_one_function bails
 * on nbOptInput > SWEEP_MAX_OPT precisely so the writes fit), just not visibly. */

/* --- Post-freeze enum values (issue #139) --------------------------------
 * TA_MAType_HMA (9) exists only in the current library: the frozen oracles
 * (ta_ref_serve @ reference-pre-cutover, ta_064_serve @ v0.6.4) reject it
 * with TA_BAD_PARAM while the current side computes -- a guaranteed false
 * mismatch that would diff the feature itself, not a bug. Vector builders
 * that feed a FROZEN oracle therefore skip IntegerList values above this
 * max, and the affected run summaries print the skip count so the exclusion
 * is loud, never silent. Current-vs-current gates are unaffected and DO
 * exercise the new value: --xlang-hash, stream_verify's enum sweep, the
 * VARIANT gate and the COMPOSITE hand tests (TA_MAType_HMA dispatch parity).
 * When a frozen oracle is re-frozen on a tag that includes #139, raise (or
 * retire) this max accordingly. */
#define FROZEN_ORACLE_MATYPE_MAX 8   /* == TA_MAType_T3; 9+ postdate the frozen
                                        oracles (HMA #139, DISABLED #93, DEFAULT #182) */
static long long g_frozenEnumSkips = 0;

static int frozen_excludes_enum_value(const TA_OptInputParameterInfo *oi, int value)
{
    if( oi->paramName && strstr(oi->paramName, "MAType")
        && value > FROZEN_ORACLE_MATYPE_MAX )
    {
        g_frozenEnumSkips++;
        return 1;
    }
    return 0;
}

/* Send a set_compatibility to one server. Returns 1 on success. */
static int sweep_set_compat(CodegenPipe *pipe, int mode, char *respBuf)
{
    char req[96];
    snprintf(req, sizeof(req),
             "{\"method\":\"set_compatibility\",\"params\":{\"mode\":%d}}", mode);
    if( codegen_pipe_call(pipe, req, respBuf, JSON_BUF_SIZE) != TA_TEST_PASS )
        return 0;
    return !json_is_error(respBuf);
}

/* In-process GUARDED comparison buffers for the sweep triangle (see below). */
static TA_Real    sweepGuardedReal[MAX_OUTPUTS][MAX_NB_TEST_ELEMENT];
static TA_Integer sweepGuardedInt[MAX_OUTPUTS][MAX_NB_TEST_ELEMENT];

/* Compare the in-process GUARDED call against the ta_ref_serve baseline for one
 * sweep variant. This is the one sweep check that does not cross the JSON-RPC
 * boundary, so it cannot be blurred by %.15g. C only — the in-process library IS
 * the C backend. */
static void sweep_compare_guarded(CodegenRangeTestParam *p)
{
    unsigned int i;
    int outBegIdx = -1, outNbElement = -1;

    if( p->paramHolder == NULL )
        return;

    /* Apply this variant's optional params to the holder. */
    for( i = 0; i < p->funcInfo->nbOptInput; i++ )
    {
        const TA_OptInputParameterInfo *optInfo;
        TA_GetOptInputParameterInfo(p->funcInfo->handle, i, &optInfo);
        if( optInfo->type == TA_OptInput_RealRange ||
            optInfo->type == TA_OptInput_RealList )
            TA_SetOptInputParamReal(p->paramHolder, i, p->optOverride[i]);
        else
            TA_SetOptInputParamInteger(p->paramHolder, i, (int)p->optOverride[i]);
    }

    if( TA_CallFunc(p->paramHolder, 0, p->nbBars - 1,
                    &outBegIdx, &outNbElement) != p->lastRetCode
        || outBegIdx != p->lastBegIdx
        || outNbElement != p->lastNbElement )
    {
        printf("SWEEP GUARDED MISMATCH [TA_%s]: rc/begIdx/nbElement "
               "guarded=%d/%d vs ref=%d/%d (nb %d vs %d)\n",
               p->funcInfo->name, outBegIdx, outNbElement,
               (int)p->lastBegIdx, (int)p->lastNbElement,
               outNbElement, (int)p->lastNbElement);
        p->codegenError = TA_CODEGEN_BEGIDX_MISMATCH;
        return;
    }

    /* Structural parity verified above; skip the VALUE diff for functions that
     * intentionally diverge from the frozen reference (#107 STOCHRSI, #242
     * CORREL). */
    if( codegen_ref_value_exempt(p->funcInfo->name) )
        return;

    for( i = 0; i < p->funcInfo->nbOutput && i < MAX_OUTPUTS; i++ )
    {
        int j;
        if( p->outputIsInteger[i] )
        {
            for( j = 0; j < outNbElement; j++ )
                if( sweepGuardedInt[i][j] != p->outIntBufs[i][j] )
                {
                    printf("SWEEP GUARDED MISMATCH [TA_%s]: outInt%u[%d] "
                           "guarded=%d ref=%d\n", p->funcInfo->name, i, j,
                           sweepGuardedInt[i][j], p->outIntBufs[i][j]);
                    p->codegenError = TA_CODEGEN_OUTPUT_MISMATCH;
                    return;
                }
        }
        else
        {
            for( j = 0; j < outNbElement; j++ )
                if( fabs(sweepGuardedReal[i][j] - p->outRealBufs[i][j]) > 1e-6 )
                {
                    printf("SWEEP GUARDED MISMATCH [TA_%s]: out%u[%d] "
                           "guarded=%.12g ref=%.12g\n", p->funcInfo->name, i, j,
                           sweepGuardedReal[i][j], p->outRealBufs[i][j]);
                    p->codegenError = TA_CODEGEN_OUTPUT_MISMATCH;
                    return;
                }
        }
    }
}

/* Everything a float-leg pass overwrites in *p, so the leg can be a no-op on
 * the caller's state.
 *
 * Every pass calls parse_ref_baseline(), which rewrites lastRetCode /
 * lastBegIdx / lastNbElement and the output buffers — the exact fields
 * sweep_compare_guarded() diffs the in-process guarded call against — and the
 * sentinel pass additionally rewrites optOverride[]. Before this snapshot the
 * leg was safe only because it happened to be the LAST statement of
 * sweep_run_variant(); an extra pass inserted anywhere else produced
 *
 *     SWEEP GUARDED MISMATCH [TA_ACCBANDS]: guarded=2/250 vs ref=19/233
 *
 * — the guarded call at the swept period 3 against a baseline the extra pass
 * had left holding the default period 20. That is an ordering constraint no
 * signature states, so it is removed rather than documented (issue #170).
 *
 * File-static, like this file's other big scratch (sweepGuardedReal): ~10KB,
 * and the structs that reach here are main() locals on a 1MB Windows stack.
 * Not reentrant — nothing calls the leg from inside the leg. */
static struct {
    TA_RetCode lastRetCode;
    TA_Integer lastBegIdx;
    TA_Integer lastNbElement;
    TA_Real    real[MAX_OUTPUTS][MAX_NB_TEST_ELEMENT];
    TA_Integer integer[MAX_OUTPUTS][MAX_NB_TEST_ELEMENT];
    int        optOverrideActive;
    double     optOverride[SWEEP_MAX_OPT];
    /* Request-shaping flags and the timing accumulators compare_codegen_output_
     * generic() adds to. Snapshotted rather than zeroed on the way out so this
     * really is "everything the leg touches" — a later reader should not have to
     * re-derive which of these the callers happen to leave at zero. */
    int        useFloat;
    int        widenFloatInputs;
    double     epsilonScale;
    long long  server_total_ns;
    int        timing_count;
} g_floatLegSaved;

static void float_leg_save_state(const CodegenRangeTestParam *p)
{
    unsigned int o;
    g_floatLegSaved.lastRetCode       = p->lastRetCode;
    g_floatLegSaved.lastBegIdx        = p->lastBegIdx;
    g_floatLegSaved.lastNbElement     = p->lastNbElement;
    g_floatLegSaved.optOverrideActive = p->optOverrideActive;
    g_floatLegSaved.useFloat          = p->useFloat;
    g_floatLegSaved.widenFloatInputs  = p->widenFloatInputs;
    g_floatLegSaved.epsilonScale      = p->epsilonScale;
    g_floatLegSaved.server_total_ns   = p->server_total_ns;
    g_floatLegSaved.timing_count      = p->timing_count;
    memcpy(g_floatLegSaved.optOverride, p->optOverride,
           sizeof(g_floatLegSaved.optOverride));
    for( o = 0; o < p->funcInfo->nbOutput && o < MAX_OUTPUTS; o++ )
    {
        if( p->outRealBufs[o] )
            memcpy(g_floatLegSaved.real[o], p->outRealBufs[o],
                   MAX_NB_TEST_ELEMENT * sizeof(TA_Real));
        if( p->outIntBufs[o] )
            memcpy(g_floatLegSaved.integer[o], p->outIntBufs[o],
                   MAX_NB_TEST_ELEMENT * sizeof(TA_Integer));
    }
}

static void float_leg_restore_state(CodegenRangeTestParam *p)
{
    unsigned int o;
    p->lastRetCode       = g_floatLegSaved.lastRetCode;
    p->lastBegIdx        = g_floatLegSaved.lastBegIdx;
    p->lastNbElement     = g_floatLegSaved.lastNbElement;
    p->optOverrideActive = g_floatLegSaved.optOverrideActive;
    p->useFloat          = g_floatLegSaved.useFloat;
    p->widenFloatInputs  = g_floatLegSaved.widenFloatInputs;
    p->epsilonScale      = g_floatLegSaved.epsilonScale;
    p->server_total_ns   = g_floatLegSaved.server_total_ns;
    p->timing_count      = g_floatLegSaved.timing_count;
    memcpy(p->optOverride, g_floatLegSaved.optOverride,
           sizeof(g_floatLegSaved.optOverride));
    for( o = 0; o < p->funcInfo->nbOutput && o < MAX_OUTPUTS; o++ )
    {
        if( p->outRealBufs[o] )
            memcpy(p->outRealBufs[o], g_floatLegSaved.real[o],
                   MAX_NB_TEST_ELEMENT * sizeof(TA_Real));
        if( p->outIntBufs[o] )
            memcpy(p->outIntBufs[o], g_floatLegSaved.integer[o],
                   MAX_NB_TEST_ELEMENT * sizeof(TA_Integer));
    }
}

/* One float-leg pass at whatever parameter vector p->optOverride* currently
 * selects: the language's single-precision entry point against its OWN double
 * one, on float-widened inputs. Returns 1 when an acknowledged comparison
 * happened, 0 when the server could not answer at all.
 *
 * `strict` makes an error response from the double half a FAILURE instead of a
 * skip, and `what` labels the pass in that message. Only the sentinel pass sets
 * it: the resolved-default pass has already proven this server answers this
 * function, so an error can then only mean the sentinel did not resolve.
 *
 * The float half needs no `strict` equivalent — an error response carries no
 * "used_float", so the acknowledgment check below already fails it. */
static int float_leg_pass(CodegenRangeTestParam *p, int strict, const char *what)
{
    int compared = 0;

    /* TA_S_<F>(float) is now bit-identical to TA_<F>((double)float) (the
     * single-precision variants widen every float input read, PR #33). Verify
     * that invariant bit-for-bit: the baseline is the double variant on
     * float-widened inputs; the use_float leg is the single-precision variant on
     * the same inputs. Both hit the C server with %.17g-exact widened inputs
     * (the use_float leg rounds back to float, idempotent). Replaces the old
     * comparison against the frozen single-precision reference, which computed
     * in float. */
    p->widenFloatInputs = 1;
    p->epsilonScale = 0.0;

    /* Baseline: double variant on the float-widened inputs. */
    p->useFloat = 0;
    build_json_request(p, 0, p->nbBars - 1);
    int callOk = (codegen_pipe_call(p->cp, p->requestBuf, p->responseBuf,
                                    JSON_BUF_SIZE) == TA_TEST_PASS);
    if( callOk && !json_is_error(p->responseBuf) )
    {
        parse_ref_baseline(p);

        /* Single-precision variant on the same inputs. */
        p->useFloat = 1;
        build_json_request(p, 0, p->nbBars - 1);
        if( codegen_pipe_call(p->cp, p->requestBuf, p->responseBuf,
                              JSON_BUF_SIZE) != TA_TEST_PASS )
            p->codegenError = TA_CODEGEN_RETCODE_MISMATCH;
        else
        {
            /* The server must SAY it took the float path. Without this the leg
             * fails open: a server that ignores "use_float" returns the double
             * result twice, and every comparison below passes trivially — which
             * is indistinguishable from "the float surface is verified". Every
             * sibling gate in this file carries a floor like this. */
            int len = 0;
            const char *ack = json_find_field(p->responseBuf, "used_float", &len);
            if( !ack || strtol(ack, NULL, 10) != 1 )
            {
                printf("CODEGEN FLOAT LEG NOT TAKEN [TA_%s]%s: server did not acknowledge "
                       "use_float — the leg would have passed while comparing the double "
                       "result against itself\n", p->funcInfo->name, what);
                p->codegenError = TA_CODEGEN_OUTPUT_MISMATCH;
            }
            else
            {
                compared = 1;
                for( unsigned int o = 0; o < p->funcInfo->nbOutput; o++ )
                    compare_codegen_output_generic(p, o);
            }
        }
        if( p->codegenError != TA_TEST_PASS )
            printf("  (mismatch above is the FLOAT (TA_S_) leg%s: TA_S_ vs TA_ on widened inputs)\n",
                   what);
    }
    else if( strict )
    {
        printf("CODEGEN FLOAT LEG REJECTED [TA_%s]%s: %s where the resolved-default "
               "request on the same function succeeded\n",
               p->funcInfo->name, what,
               callOk ? "the server answered with an error"
                      : "the server call failed (pipe closed?)");
        p->codegenError = TA_CODEGEN_RETCODE_MISMATCH;
    }

    p->useFloat = 0;
    p->widenFloatInputs = 0;
    p->epsilonScale = 0.0;
    return compared;
}

/* Put every optional parameter at its "use the declared default" sentinel.
 * Returns how many slots actually carry one.
 *
 * The single exclusion is a CHOICE-LIST parameter on a language whose
 * optional-parameter surface cannot represent the value — Java, whose Core
 * takes a real MAType enum and whose generated test server dies constructing
 * one from Integer.MIN_VALUE (#162). That slot is left at its explicit default
 * and counted; the function's other parameters still ride the sentinel, which
 * is strictly more coverage than skipping the function. */
static int float_leg_set_sentinels(CodegenRangeTestParam *p)
{
    unsigned int i;
    int nbSent = 0;

    /* optOverrideActive makes build_json_request read optOverride[i] for EVERY
     * optional parameter, so a function wider than the array would send it off
     * the end. The ref sweep holds the same invariant by refusing such a
     * function up front; here it cannot be a silent skip — that would drop the
     * function's sentinel coverage without saying so. Widest shipped function is
     * SAREXT at 8, so this is a guard against a future definition, not a live
     * case. */
    if( p->funcInfo->nbOptInput > SWEEP_MAX_OPT )
    {
        printf("CODEGEN FLOAT SENTINEL OVERFLOW [TA_%s]: %u optional parameters "
               "exceeds SWEEP_MAX_OPT (%d) — raise it, do not skip the function\n",
               p->funcInfo->name, p->funcInfo->nbOptInput, SWEEP_MAX_OPT);
        p->codegenError = TA_CODEGEN_OUTPUT_MISMATCH;
        return 0;
    }

    for( i = 0; i < p->funcInfo->nbOptInput; i++ )
    {
        const TA_OptInputParameterInfo *optInfo;
        int enumDefaultMember;
        TA_GetOptInputParameterInfo(p->funcInfo->handle, i, &optInfo);
        enumDefaultMember = codegen_enum_default_member(optInfo);

        switch( optInfo->type )
        {
        case TA_OptInput_IntegerRange:
            p->optOverride[i] = (double)TA_INTEGER_DEFAULT;
            nbSent++;
            break;
        case TA_OptInput_IntegerList:
            if( codegen_lang_can_pass_enum_sentinel(p->langName) )
            {
                p->optOverride[i] = (double)TA_INTEGER_DEFAULT;
                nbSent++;
            }
            else if( enumDefaultMember >= 0 )
            {
                /* Java cannot hold TA_INTEGER_DEFAULT here, but the enum's own
                 * DEFAULT member carries the identical contract (#182) and IS
                 * representable — and Java's float overload is the one surface
                 * whose substitution no other gate reaches. */
                p->optOverride[i] = (double)enumDefaultMember;
                nbSent++;
            }
            else
            {
                p->optOverride[i] = optInfo->defaultValue;
                g_floatSentinelEnumWithheld++;
            }
            break;
        case TA_OptInput_RealRange:
        case TA_OptInput_RealList:
            p->optOverride[i] = TA_REAL_DEFAULT;
            nbSent++;
            break;
        default:
            p->optOverride[i] = optInfo->defaultValue;
            break;
        }
    }
    p->optOverrideActive = 1;
    return nbSent;
}

/* Float-variant leg: re-run the current request through the TA_S_ API on the
 * server under test ("use_float":1) and diff single-precision against its own
 * double result.
 *
 * withSentinel adds a SECOND pass with every optional parameter set to
 * TA_INTEGER_DEFAULT / TA_REAL_DEFAULT (issue #170). Both halves of that pass
 * carry the sentinel, so the property under test is "each tier substitutes the
 * SAME declared default" — self-contained, no oracle, and it holds for the
 * post-reference functions that have no ta_ref_serve baseline.
 *
 * That vector is the one that exposed the TA_S_EMA defect fixed in 2e9767397:
 * the float body derived EMA's k factor from the raw sentinel because the
 * initialiser ran before the prologue substituted it, while the double tier
 * was right. The same defect was live in Java's float emaInternal and C#'s
 * float Ema, where no gate could see it — reaching only resolved defaults,
 * this leg could not have caught it.
 *
 * Not asserted here: float(sentinel) == float(default). A body that mishandles
 * the sentinel diverges from its own double tier (the pair check above) or is
 * rejected outright (the `strict` arm), and the double tier's own
 * sentinel-selects-the-default contract is gated by --xlang-hash (#148).
 *
 * The sweep passes withSentinel=0: it is C-only, already varies parameters, and
 * a per-variant sentinel pass would re-send one identical request each time. */
static void run_float_leg(CodegenRangeTestParam *p, int withSentinel)
{
    if( p->codegenError != TA_TEST_PASS )
        return;

    float_leg_save_state(p);

    if( float_leg_pass(p, 0, "") )
    {
        g_floatLegCompared++;

        if( withSentinel && p->codegenError == TA_TEST_PASS
            && p->langIndex >= 0 && p->langIndex < (int)NUM_LANGUAGES )
        {
            /* A function with no sentinel-able optional parameter has nothing
             * to add here — the pass would repeat the request just made. */
            if( float_leg_set_sentinels(p) > 0 )
            {
                g_floatSentinelEligible[p->langIndex]++;
                if( float_leg_pass(p, 1, " [default-sentinel]") )
                {
                    g_floatSentinelCompared[p->langIndex]++;
                    if( p->lastNbElement > 0 )
                        g_floatSentinelWithOutput[p->langIndex]++;
                }
            }
        }
    }

    float_leg_restore_state(p);
}

/* Run one variant: ta_ref_serve fills the baseline, the language server is
 * diffed against it. Returns 1 if the variant was comparable (counted), 0 if
 * the reference could not answer it. Mismatches land in p->codegenError. */
static int sweep_run_variant(CodegenRangeTestParam *p)
{
    build_json_request(p, 0, p->nbBars - 1);
    if( codegen_pipe_call(p->refCp, p->requestBuf, p->responseBuf,
                          JSON_BUF_SIZE) != TA_TEST_PASS
        || json_is_error(p->responseBuf) )
        return 0;   /* reference cannot answer this variant -- nothing to diff */
    parse_ref_baseline(p);

    if( codegen_pipe_call(p->cp, p->requestBuf, p->responseBuf,
                          JSON_BUF_SIZE) != TA_TEST_PASS )
    {
        p->codegenError = TA_CODEGEN_RETCODE_MISMATCH;
        return 1;
    }
    for( unsigned int o = 0; o < p->funcInfo->nbOutput; o++ )
        compare_codegen_output_generic(p, o);

    if( p->codegenError == TA_TEST_PASS )
        sweep_compare_guarded(p);
    if( p->sweepFloatLeg )
        run_float_leg(p, 0);
    return 1;
}

static void sweep_one_function(const TA_FuncInfo *funcInfo, void *opaqueData)
{
    ForEachFuncContext *ctx = (ForEachFuncContext *)opaqueData;
    unsigned int i;

    if( ctx->error != TA_TEST_PASS )
        return;
    if( !codegen_matches_filter(ctx->functionFilter, funcInfo->name) )
        return;
    if( funcInfo->nbOptInput == 0 || funcInfo->nbOptInput > SWEEP_MAX_OPT )
        return;

    /* Subset gate: this sweep diffs against the FROZEN ta_ref_serve too, so skip
     * functions the reference lacks (post-tag additions — see test_one_function). */
    if( ctx->refFuncList )
    {
        char needle[80];
        snprintf(needle, sizeof(needle), "\"TA_%s\"", funcInfo->name);
        if( !strstr(ctx->refFuncList, needle) ) { ctx->sweepSkipped++; return; }
    }

    /* Skip functions with integer inputs (same rule as the main pass). */
    for( i = 0; i < funcInfo->nbInput; i++ )
    {
        const TA_InputParameterInfo *inputInfo;
        TA_GetInputParameterInfo(funcInfo->handle, i, &inputInfo);
        if( inputInfo->type == TA_Input_Integer )
            return;
    }

    TA_SetUnstablePeriod(TA_FUNC_UNST_ALL, 0);

    CodegenRangeTestParam params;
    memset(&params, 0, sizeof(params));
    params.funcInfo    = funcInfo;
    params.paramHolder = NULL;
    params.history     = ctx->history;
    params.nbBars      = (int)ctx->history->nbBars;
    params.unstId      = get_unst_id(funcInfo->name);
    params.cp          = ctx->cp;
    params.langName    = ctx->lang->name;
    params.langIndex   = ctx->langIndex;
    params.refCp       = ctx->refCp;
    params.requestBuf  = ctx->requestBuf;
    params.responseBuf = ctx->responseBuf;
    params.codegenError = TA_TEST_PASS;
    setup_outputs(&params);

    /* In-process GUARDED triangle leg (see sweep_compare_guarded): only while
     * sweeping the C server — the in-process library is the C backend, so
     * repeating it for the other language iterations would be identical. */
    if( strcmp(ctx->lang->name, "c") == 0 )
    {
        params.sweepFloatLeg = 1;
        if( TA_ParamHolderAlloc(funcInfo->handle, &params.paramHolder) == TA_SUCCESS )
        {
            setup_inputs(params.paramHolder, funcInfo, ctx->history);
            for( i = 0; i < funcInfo->nbOutput && i < MAX_OUTPUTS; i++ )
            {
                const TA_OutputParameterInfo *outputInfo;
                TA_GetOutputParameterInfo(funcInfo->handle, i, &outputInfo);
                if( outputInfo->type == TA_Output_Real )
                    TA_SetOutputParamRealPtr(params.paramHolder, i, &sweepGuardedReal[i][0]);
                else
                    TA_SetOutputParamIntegerPtr(params.paramHolder, i, &sweepGuardedInt[i][0]);
            }
        }
        else
            params.paramHolder = NULL;
    }

    /* Seed every override with the default value. */
    double defVals[SWEEP_MAX_OPT];
    for( i = 0; i < funcInfo->nbOptInput; i++ )
    {
        const TA_OptInputParameterInfo *optInfo;
        TA_GetOptInputParameterInfo(funcInfo->handle, i, &optInfo);
        defVals[i] = optInfo->defaultValue;
        params.optOverride[i] = optInfo->defaultValue;
    }
    params.optOverrideActive = 1;

    int variants = 0;
    const char *failParam = NULL;
    double failValue = 0.0;

    /* One param varied at a time, the others at their defaults. */
    for( i = 0; i < funcInfo->nbOptInput && params.codegenError == TA_TEST_PASS; i++ )
    {
        const TA_OptInputParameterInfo *optInfo;
        TA_GetOptInputParameterInfo(funcInfo->handle, i, &optInfo);

        /* Sized past the widest single-param list (MAType: 12 values today, 11
         * non-default after #93's DISABLED and #182's DEFAULT) so the cap below never silently drops
         * a value even if FROZEN_ORACLE_MATYPE_MAX is retired after a re-freeze
         * (which would let all non-default MATypes through). */
        double cand[16];
        int nc = 0;

        if( optInfo->type == TA_OptInput_IntegerRange )
        {
            const TA_IntegerRange *r = (const TA_IntegerRange *)optInfo->dataSet;
            int lo  = (r->min < 2) ? 2 : r->min;       /* floor: see header comment */
            int hi  = (r->max > 100) ? 100 : r->max;   /* keep lookbacks < nbBars */
            int def = (int)optInfo->defaultValue;
            int base[5];
            int b, k;
            base[0] = lo; base[1] = lo + 1; base[2] = lo + 7;
            base[3] = def - 1; base[4] = def + 3;
            for( b = 0; b < 5; b++ )
            {
                int v = base[b];
                if( v < lo ) v = lo;
                if( v > hi ) v = hi;
                if( v == def ) continue;
                for( k = 0; k < nc; k++ )
                    if( (int)cand[k] == v ) break;
                if( k == nc )
                    cand[nc++] = (double)v;
            }
        }
        else if( optInfo->type == TA_OptInput_IntegerList )
        {
            const TA_IntegerList *l = (const TA_IntegerList *)optInfo->dataSet;
            unsigned int e;
            for( e = 0; e < l->nbElement; e++ )
            {
                if( l->data[e].value == (int)optInfo->defaultValue )
                    continue;
                /* This sweep diffs against the frozen ta_ref_serve: skip enum
                 * values it predates (counted; see FROZEN_ORACLE_MATYPE_MAX).
                 * Evaluated BEFORE the cand cap so the exclusion stays loud --
                 * an `nc` bound in the loop condition would silently truncate
                 * the tail value instead of counting it. */
                if( frozen_excludes_enum_value( optInfo, l->data[e].value ) )
                    continue;
                if( nc < 16 )
                    cand[nc++] = (double)l->data[e].value;
            }
        }
        else if( optInfo->type == TA_OptInput_RealRange )
        {
            const TA_RealRange *r = (const TA_RealRange *)optInfo->dataSet;
            double sugg[2];
            int b;
            sugg[0] = r->suggested_start;
            sugg[1] = r->suggested_end;
            for( b = 0; b < 2; b++ )
            {
                double v = sugg[b];
                if( fabs(v) > 1e30 ) continue;             /* unbounded sentinel */
                if( v < r->min || v > r->max ) continue;
                if( v == optInfo->defaultValue ) continue;
                cand[nc++] = v;
            }
        }
        else
            continue;

        for( int c = 0; c < nc && params.codegenError == TA_TEST_PASS; c++ )
        {
            params.optOverride[i] = cand[c];
            variants += sweep_run_variant(&params);
            if( params.codegenError != TA_TEST_PASS )
            {
                failParam = optInfo->paramName;
                failValue = cand[c];
            }
            params.optOverride[i] = defVals[i];
        }
    }

    /* Metastock-compatibility pass at defaults (both servers AND the
     * in-process library switched, for the guarded triangle leg). Languages
     * with no compatibility API cannot take this leg — skip it out loud. */
    if( params.codegenError == TA_TEST_PASS &&
        !codegen_lang_has_compatibility_api(ctx->lang->name) )
    {
        note_compat_skip(ctx->lang->name, "sweep Metastock pass");
    }
    else if( params.codegenError == TA_TEST_PASS )
    {
        if( sweep_set_compat(params.refCp, 1, params.responseBuf) &&
            sweep_set_compat(params.cp,    1, params.responseBuf) )
        {
            TA_SetCompatibility(TA_COMPATIBILITY_METASTOCK);
            variants += sweep_run_variant(&params);
            TA_SetCompatibility(TA_COMPATIBILITY_DEFAULT);
            if( params.codegenError != TA_TEST_PASS )
            {
                failParam = "compatibility=METASTOCK";
                failValue = 1;
            }
        }
        sweep_set_compat(params.refCp, 0, params.responseBuf);
        sweep_set_compat(params.cp,    0, params.responseBuf);
    }

    /* Unstable-period pass at defaults (sent per-call to both servers).
     * Only genuinely recursive functions still carry TA_FUNC_FLG_UNST_PER and
     * a mapped unstId, so IMI and MFI (finite-window, reclassified stable) are
     * naturally excluded here. That also retires the former explicit IMI
     * carve-out: IMI's u>0 output diverges from the frozen ref due to fix #98,
     * but with no unstable flag this variant no longer runs for it at all. */
    if( params.codegenError == TA_TEST_PASS &&
        (funcInfo->flags & TA_FUNC_FLG_UNST_PER) &&
        params.unstId != TA_TEST_UNST_NONE )
    {
        TA_SetUnstablePeriod(params.unstId, 3);
        variants += sweep_run_variant(&params);
        TA_SetUnstablePeriod(TA_FUNC_UNST_ALL, 0);
        /* The per-call unstablePeriod field is sticky server-side (each call
         * sets the server's global for that function). Send one defaults call
         * carrying 0 so BOTH servers are restored for later functions and
         * languages — ta_ref_serve is shared across the language loop, and
         * dependents like ADOSC read EMA's global without sending the field. */
        if( params.codegenError == TA_TEST_PASS )
            variants += sweep_run_variant(&params);
        if( params.codegenError != TA_TEST_PASS )
        {
            failParam = "unstablePeriod";
            failValue = 3;
        }
    }

    if( params.paramHolder != NULL )
        TA_ParamHolderFree(params.paramHolder);

    ctx->sweepVariants += variants;
    ctx->sweepFunctions++;

    if( params.codegenError != TA_TEST_PASS )
    {
        printf("  REF SWEEP FAIL [TA_%s]: %s=%g (mismatch detail above)\n",
               funcInfo->name, failParam ? failParam : "?", failValue);
        ctx->failed++;
        ctx->error = params.codegenError;
    }

    free_outputs(&params);
}

/* ---- Stream verification pass (batch-vs-stream, in-server bitwise) ----
 *
 * For each function the server sends stream_verify: it generates the input
 * series from a seed (fuzz_data.h), runs BOTH the batch function (startIdx=0)
 * and the stream trajectory (Open on a warm-up prefix, Update per remaining
 * bar, Peek spot-asserted equal to the following Update) fully in-process,
 * and compares BITWISE per bar. The driver only reads the match flags, so
 * bit-exactness never rides the lossy JSON float path. Non-streamable
 * functions (and servers without the method) answer with an error and are
 * counted as skips. See docs/streaming-api-proposal.md, Verification. */

/* Headroom over the widest shipped function (SAREXT, 8) so a normal-sized new
 * function cannot reach the cap. Overflow is a hard failure, never a skip. */
#define STREAM_MAX_OPT 16
/* Sized for the widest stream-vector enumeration: MACDEXT carries 3 MAType
 * params, so its count is 8*M-1 in the MAType-list length M (base 4 + 3 params *
 * (2 base-vector crosses * (M-1) non-default arms + 1 out-of-list) + the 2 *
 * (M-1) multi-enum diagonal, #181). M=12 today (#93 added DISABLED, #182
 * DEFAULT) => 95; 128 keeps runway for 4 more MATypes before MACDEXT reaches
 * it again. Overflow is a hard failure, never a skip. */
#define STREAM_MAX_VEC 128
#define STREAM_N       240
/* Stream-leg variants: 0 = ambient defaults, 1 = unstable period, 2 = Metastock,
 * then one per data shape from MONO_UP up (FUZZ_NSHAPES - 1 of them). */
#define STREAM_NVARIANT (3 + FUZZ_NSHAPES - 1)

static int stream_flag(const char *resp, const char *key)
{
    const char *p = strstr(resp, key);
    if( !p ) return -1;
    return atoi(p + strlen(key));
}

static void stream_build_request(char *buf, const TA_FuncInfo *fi,
                                 const double *optVals,
                                 int shape, int seed, int n,
                                 int unstablePeriod, int compat)
{
    int pos = codegen_appendf(buf, JSON_BUF_SIZE, 0,
        "{\"method\":\"stream_verify\",\"params\":{\"funcName\":\"TA_%s\","
        "\"gen_shape\":%d,\"gen_seed\":%d,\"gen_n\":%d,"
        "\"unstablePeriod\":%d,\"compatibility\":%d",
        fi->name, shape, seed, n, unstablePeriod, compat);
    /* Candle functions: ask the server for the settings-variation rounds
     * (avgPeriods bumped, then zeroed) on top of the default-settings legs. */
    if( fi->flags & TA_FUNC_FLG_CANDLESTICK )
        pos = codegen_appendf(buf, JSON_BUF_SIZE, pos, ",\"candleLegs\":1");
    unsigned int i;
    for( i = 0; i < fi->nbOptInput && i < STREAM_MAX_OPT; i++ )
    {
        const TA_OptInputParameterInfo *oi;
        TA_GetOptInputParameterInfo(fi->handle, i, &oi);
        if( oi->type == TA_OptInput_RealRange || oi->type == TA_OptInput_RealList )
            pos = codegen_appendf(buf, JSON_BUF_SIZE, pos, ",\"%s\":%.15g", oi->paramName, optVals[i]);
        else
            pos = codegen_appendf(buf, JSON_BUF_SIZE, pos, ",\"%s\":%d", oi->paramName, (int)optVals[i]);
    }
    codegen_appendf(buf, JSON_BUF_SIZE, pos, "}}");
}

/* Param vectors: defaults, integer params at their true minimum (period==1
 * territory, issues #93/#94 — deliberately NOT floored at 2 like the 0.6.4
 * fuzz), and min+1. Real params stay at their defaults. Then one extra
 * vector per non-default enum (MAType) list value, everything else at
 * defaults: dispatch streams (MA) select their sub-stream by these values,
 * so every arm gets its own bit-exact legs; arms without a sub-stream are
 * verified as documented Open rejects server-side ("unsupportedArm").
 * vecIsEnum marks the sweep vectors so the variant loop can add K and
 * Metastock legs (the selected arm may be unstable — EMA/KAMA/T3 — or
 * compatibility-seeded — EMA). */
static int stream_build_vectors(const TA_FuncInfo *fi,
                                double vec[STREAM_MAX_VEC][STREAM_MAX_OPT],
                                int vecIsEnum[STREAM_MAX_VEC],
                                int vecIsMin[STREAM_MAX_VEC],
                                int *overflow)
{
    unsigned int i, e;
    int hasMin = 0, hasMinPlus1 = 0, nvec, v, baseVecs = 0;
    for( v = 0; v < STREAM_MAX_VEC; v++ ) vecIsMin[v] = 0;
    for( i = 0; i < fi->nbOptInput && i < STREAM_MAX_OPT; i++ )
    {
        const TA_OptInputParameterInfo *oi;
        TA_GetOptInputParameterInfo(fi->handle, i, &oi);
        vec[0][i] = vec[1][i] = vec[2][i] = oi->defaultValue;
        if( oi->type == TA_OptInput_IntegerRange )
        {
            const TA_IntegerRange *r = (const TA_IntegerRange *)oi->dataSet;
            if( r )
            {
                if( (int)r->min != (int)oi->defaultValue )
                {
                    vec[1][i] = (double)(int)r->min;
                    hasMin = 1;
                }
                if( (int)r->min + 1 <= (int)r->max &&
                    (int)r->min + 1 != (int)oi->defaultValue )
                {
                    vec[2][i] = (double)((int)r->min + 1);
                    hasMinPlus1 = 1;
                }
            }
        }
    }
    if( hasMin && hasMinPlus1 ) nvec = 3;
    else if( hasMin ) nvec = 2;
    else if( hasMinPlus1 )
    {
        for( i = 0; i < fi->nbOptInput && i < STREAM_MAX_OPT; i++ )
            vec[1][i] = vec[2][i];
        nvec = 2;
    }
    else nvec = 1;
    /* The below-default boundary vectors (v>=1: range.min and min+1) carry the
     * smallest periods. For a dual-mode function (DI/DM) the min period selects
     * the degenerate arm, which IGNORES the unstable period while the general
     * arm honors it — so the K-leg (variant 1) must run on these vectors too,
     * else period=1+K (the only place the two arms can diverge) goes untested.
     * fuzz-064 floors periods at 2, so this is the sole gate covering it. */
    for( v = 0; v < nvec; v++ ) vecIsEnum[v] = 0;
    for( v = 1; v < nvec; v++ ) vecIsMin[v] = 1;

    /* One ABOVE-default "large window" vector (default+41, clamped to the
     * range). It exercises the general/large-period regime — a big ring/window
     * so wraparound is hit over the fixed STREAM_N history. The +41 (odd)
     * offset also FLIPS PARITY vs the default, so a parity-branched dual-mode
     * function (TRIMA odd/even) gets a non-degenerate ODD large period (its
     * default 30 is even; min=1 is odd but degenerate), locking the odd arm's
     * Open/Update/Peek continuation into CI. Without this vector,
     * stream_build_vectors hands every function only default/min small periods,
     * and no leg would reach a ring large enough to wrap. Deduped vs the
     * default; not a min/enum vector. */
    {
        int any = 0;
        if( nvec < STREAM_MAX_VEC )
        {
            for( i = 0; i < fi->nbOptInput && i < STREAM_MAX_OPT; i++ )
            {
                const TA_OptInputParameterInfo *oi;
                TA_GetOptInputParameterInfo(fi->handle, i, &oi);
                vec[nvec][i] = oi->defaultValue;
                if( oi->type == TA_OptInput_IntegerRange )
                {
                    const TA_IntegerRange *r = (const TA_IntegerRange *)oi->dataSet;
                    int def_i = (int)oi->defaultValue, big = def_i + 41;
                    if( r && big > (int)r->max ) big = (int)r->max;
                    if( big != def_i ) { vec[nvec][i] = (double)big; any = 1; }
                }
            }
            if( any ) { vecIsEnum[nvec] = 0; vecIsMin[nvec] = 0; nvec++; }
        }
    }

    /* Enum (MAType) sweep vectors: each non-default list value crossed with
     * (a) the defaults vector AND (b) the boundary vector when one exists
     * (period==1 x MAType covers the identity-beats-unsupported-arm ordering
     * — TA_MA_Open(1, MAMA) must stream — and min-period x arm covers the
     * selected sub-stream's own boundary seeding). Plus one OUT-OF-LIST
     * value per enum param: batch rejects it (the dispatch default arm), so
     * the stream must reject too — reject-parity for the default arm.
     * *overflow reports values silently dropped by the STREAM_MAX_VEC cap
     * (the caller fails the run loudly: silent truncation would quietly
     * stop testing arms). */
    {
        baseVecs = nvec;
        *overflow = 0;
        for( i = 0; i < fi->nbOptInput && i < STREAM_MAX_OPT; i++ )
        {
            const TA_OptInputParameterInfo *oi;
            TA_GetOptInputParameterInfo(fi->handle, i, &oi);
            if( oi->type == TA_OptInput_IntegerList )
            {
                const TA_IntegerList *l = (const TA_IntegerList *)oi->dataSet;
                int b, maxList = 0;
                unsigned int j;
                if( !l ) continue;
                for( b = 0; b < baseVecs && b < 2; b++ )
                {
                    for( e = 0; e < l->nbElement; e++ )
                    {
                        if( l->data[e].value == (int)oi->defaultValue ) continue;
                        if( nvec >= STREAM_MAX_VEC ) { (*overflow)++; continue; }
                        for( j = 0; j < fi->nbOptInput && j < STREAM_MAX_OPT; j++ )
                            vec[nvec][j] = vec[b][j];
                        vec[nvec][i] = (double)l->data[e].value;
                        vecIsEnum[nvec] = 1;
                        nvec++;
                    }
                }
                for( e = 0; e < l->nbElement; e++ )
                    if( l->data[e].value > maxList ) maxList = l->data[e].value;
                if( nvec >= STREAM_MAX_VEC ) { (*overflow)++; }
                else
                {
                    for( j = 0; j < fi->nbOptInput && j < STREAM_MAX_OPT; j++ )
                        vec[nvec][j] = vec[0][j];
                    vec[nvec][i] = (double)(maxList + 91); /* out of list */
                    vecIsEnum[nvec] = 1;
                    nvec++;
                }
            }
        }
    }

    /* Multi-enum DIAGONAL: every enum param moved to the SAME non-default list
     * value at once. The sweep above overwrites exactly ONE enum slot per
     * vector, so a function carrying more than one — MACDEXT (3 MATypes) and
     * STOCH (2) — never sees them non-default TOGETHER, and a specialization
     * guarded on "all of them are X" is unreachable by construction. That is
     * the hole issue #181 fell through: MACDEXT's batch body delegates an
     * all-EMA call to TA_MACD's single lockstep pass, the streaming tier
     * composes the generic three-MA path instead, and no stream leg ever
     * selected all-EMA to hold the two to each other. The full cross is M^N
     * (1331 vectors for MACDEXT at M=12) and is what makes this deliberately
     * uncovered; the diagonal is M-1 and reaches every "all slots equal" guard.
     *
     * Crossed with the same base vectors as the sweep above, which puts the
     * periods on BOTH sides of a guard that also tests them: MACDEXT's fast
     * path needs every period >= 2, which the defaults vector satisfies and the
     * boundary vector (signal period 1) does not — so the diagonal covers the
     * specialization and its fallback rather than only one of them. */
    {
        int nEnum = 0, firstEnum = -1;
        for( i = 0; i < fi->nbOptInput && i < STREAM_MAX_OPT; i++ )
        {
            const TA_OptInputParameterInfo *oi;
            TA_GetOptInputParameterInfo(fi->handle, i, &oi);
            if( oi->type == TA_OptInput_IntegerList && oi->dataSet )
            {
                if( firstEnum < 0 ) firstEnum = (int)i;
                nEnum++;
            }
        }
        if( nEnum >= 2 )
        {
            const TA_OptInputParameterInfo *oi0;
            const TA_IntegerList *l0;
            int b;
            TA_GetOptInputParameterInfo(fi->handle, (unsigned int)firstEnum, &oi0);
            l0 = (const TA_IntegerList *)oi0->dataSet;
            for( b = 0; b < baseVecs && b < 2; b++ )
            {
                for( e = 0; e < l0->nbElement; e++ )
                {
                    /* Take the value only when EVERY enum slot lists it (the
                     * diagonal has to be a legal vector, not a reject probe —
                     * out-of-list rejection is the sweep's job above), and only
                     * when at least one slot actually moves off its default
                     * (an all-defaults diagonal is the base vector again). */
                    int value = l0->data[e].value, shared = 1, moves = 0;
                    unsigned int j;
                    for( j = 0; j < fi->nbOptInput && j < STREAM_MAX_OPT; j++ )
                    {
                        const TA_OptInputParameterInfo *oj;
                        const TA_IntegerList *lj;
                        unsigned int k;
                        int found = 0;
                        TA_GetOptInputParameterInfo(fi->handle, j, &oj);
                        if( oj->type != TA_OptInput_IntegerList || !oj->dataSet ) continue;
                        lj = (const TA_IntegerList *)oj->dataSet;
                        for( k = 0; k < lj->nbElement; k++ )
                            if( lj->data[k].value == value ) found = 1;
                        if( !found ) { shared = 0; break; }
                        if( value != (int)oj->defaultValue ) moves = 1;
                    }
                    if( !shared || !moves ) continue;
                    if( nvec >= STREAM_MAX_VEC ) { (*overflow)++; continue; }
                    for( j = 0; j < fi->nbOptInput && j < STREAM_MAX_OPT; j++ )
                    {
                        const TA_OptInputParameterInfo *oj;
                        TA_GetOptInputParameterInfo(fi->handle, j, &oj);
                        vec[nvec][j] = ( oj->type == TA_OptInput_IntegerList && oj->dataSet )
                                     ? (double)value : vec[b][j];
                    }
                    vecIsEnum[nvec] = 1;
                    nvec++;
                }
            }
        }
    }

    /* Real-param sweep: one extra vector with every RealRange optional param
     * moved to a non-default suggested value. Without it STDDEV/VAR's
     * `optInNbDev != 1.0` scaling branch of the sqrt combine map is verified
     * VACUOUSLY (every stream leg runs the default 1.0 branch only). Kept off
     * the enum cross above so it stays a single default-shape leg
     * (vecIsEnum == 0 -> variant 0 only). Non-default value comes from the
     * abstract suggested_start/end (the same source the guarded sweep uses),
     * filtered to a finite, in-range, non-default candidate. */
    {
        unsigned int j;
        int haveReal = 0;
        double rvec[STREAM_MAX_OPT];
        for( j = 0; j < fi->nbOptInput && j < STREAM_MAX_OPT; j++ )
            rvec[j] = vec[0][j];
        for( i = 0; i < fi->nbOptInput && i < STREAM_MAX_OPT; i++ )
        {
            const TA_OptInputParameterInfo *oi;
            TA_GetOptInputParameterInfo(fi->handle, i, &oi);
            if( oi->type == TA_OptInput_RealRange )
            {
                const TA_RealRange *r = (const TA_RealRange *)oi->dataSet;
                double sugg[2];
                int b;
                if( !r ) continue;
                sugg[0] = r->suggested_start;
                sugg[1] = r->suggested_end;
                for( b = 0; b < 2; b++ )
                {
                    double vv = sugg[b];
                    if( fabs(vv) > 1e30 ) continue;          /* unbounded sentinel */
                    if( vv < r->min || vv > r->max ) continue;
                    if( vv == oi->defaultValue ) continue;
                    rvec[i] = vv;
                    haveReal = 1;
                    break;
                }
            }
        }
        if( haveReal )
        {
            if( nvec >= STREAM_MAX_VEC ) { (*overflow)++; }
            else
            {
                for( j = 0; j < fi->nbOptInput && j < STREAM_MAX_OPT; j++ )
                    vec[nvec][j] = rvec[j];
                vecIsEnum[nvec] = 0;
                nvec++;
            }
        }
    }
    return nvec;
}

static void stream_one_function(const TA_FuncInfo *funcInfo, void *opaqueData)
{
    ForEachFuncContext *ctx = (ForEachFuncContext *)opaqueData;
    double vec[STREAM_MAX_VEC][STREAM_MAX_OPT];
    int vecIsEnum[STREAM_MAX_VEC];
    int vecIsMin[STREAM_MAX_VEC];
    int nvec, v, variant, legs = 0, rejArms = 0, vecOverflow = 0;
    int fillChecked = 0;   /* set once any leg reports OpenAndFill was verified */
    int ufillChecked = 0;  /* set once any leg reports UpdateAndFill was verified (#246) */
    int stateChecked = 0;  /* set once any leg reports the state-equivalence compare */
    int stateLegs = 0;     /* how many legs actually compared handle state */
    int stateOfLegs = 0;   /* value legs in the requests that reported it */
    int rangeChecked = 0;  /* set once any leg reports the OutRange compare (#241) */
    int rangeLegs = 0;     /* how many legs actually compared a handle's OutRange */
    int rangeSites = 0;    /* OR of the range-compare sites that fired */
    int rangeSitesN = 0;   /* how many the server says it has */
    long long benign = 0;  /* signed-zero cases this function's legs reported */
    int isUnstable;

    if( ctx->error != TA_TEST_PASS ) return;
    if( !codegen_matches_filter(ctx->functionFilter, funcInfo->name) ) return;

    /* K-leg eligibility: the function's own unstable flag, or an internal
     * unstable dependency (DEMA/TEMA/TRIX/MACD map to EMA in UNSTABLE_MAP —
     * their stream values depend on EMA's ambient K). */
    isUnstable = (funcInfo->flags & TA_FUNC_FLG_UNST_PER) != 0 ||
                 get_unst_id(funcInfo->name) != TA_TEST_UNST_NONE;
    nvec = stream_build_vectors(funcInfo, vec, vecIsEnum, vecIsMin, &vecOverflow);

    /* Silent truncation would quietly stop testing params beyond the cap. */
    if( funcInfo->nbOptInput > STREAM_MAX_OPT )
    {
        printf("STREAM PARAM OVERFLOW [TA_%s]: %u opt params > STREAM_MAX_OPT\n",
               funcInfo->name, funcInfo->nbOptInput);
        ctx->failed++;
        ctx->error = TA_CODEGEN_STREAM_MISMATCH;
        return;
    }
    if( vecOverflow > 0 )
    {
        printf("STREAM VECTOR OVERFLOW [TA_%s]: %d enum value(s) dropped by "
               "STREAM_MAX_VEC — arms would go unverified\n",
               funcInfo->name, vecOverflow);
        ctx->failed++;
        ctx->error = TA_CODEGEN_STREAM_MISMATCH;
        return;
    }

    for( v = 0; v < nvec; v++ )
    {
        /* Variants: ambient defaults; plus (defaults vector only) one
         * unstable-period leg, one Metastock-compatibility leg, and the
         * remaining data shapes so ALL fuzz shapes (incl. CONSTANT, TIE_HEAVY,
         * and FUZZ_CANDLE — the pattern-rich inside-bar shape that makes the
         * candlestick streams non-vacuous) are exercised every run. */
        for( variant = 0; variant < STREAM_NVARIANT; variant++ )
        {
            int K = 0, compat = 0, shape;
            ErrorNumber pipeErr;
            if( variant == 1 )
            {
                /* K-leg: defaults vector when the function is unstable, plus
                 * every enum-sweep vector — the selected sub-stream may be
                 * unstable (MA dispatching to EMA/KAMA/T3) even when the
                 * dispatcher itself carries no unstable flag — plus the
                 * below-default boundary vectors of an unstable function, so a
                 * dual-mode degenerate arm (DI/DM at period=1, which ignores K)
                 * is verified against batch under a warm unstable period. */
                if( !( (v == 0 && isUnstable) || vecIsEnum[v]
                       || (vecIsMin[v] && isUnstable) ) ) continue;
                K = 3;
            }
            else if( variant == 2 )
            {
                /* Metastock leg: defaults vector + enum-sweep vectors (an
                 * EMA-family arm seeds differently under Metastock). Skipped
                 * where the language cannot switch mode — the server would
                 * otherwise just re-verify the Default leg. */
                if( v != 0 && !vecIsEnum[v] ) continue;
                if( !codegen_lang_has_compatibility_api(ctx->lang->name) )
                {
                    note_compat_skip(ctx->lang->name, "stream Metastock leg");
                    continue;
                }
                compat = 1;
            }
            else if( variant >= 3 )
            {
                /* Extra shapes: defaults vector, plus the below-default
                 * boundary vectors (range.min and min+1).
                 *
                 * The v-rotation `(v + variant) % 7` only ever hands a
                 * non-default vector shape v itself, so before this the
                 * boundary periods were verified on MONO_UP/MONO_DOWN alone
                 * and every shape from CONSTANT up was verified at exactly one
                 * period — the default. That made the cross-tier signed-zero
                 * arm (#147) INERT for the family it was written for:
                 * FUZZ_WITH_ZEROS reached MIN/MAX/MINMAX only at period 30,
                 * where a window with no negative bar (and hence a +/-0
                 * extremum) does not occur in 240 bars, and reached MIDPOINT at
                 * 14, where a tie at ONE extremum cannot change the bits of
                 * (highest+lowest)/2. At range.min/min+1 the same shape carries
                 * 5-21 windows per function whose emitted bits are a tie-break
                 * choice. Costs <= 2 extra vectors x 8 shapes per function. */
                if( v != 0 && !vecIsMin[v] ) continue;
            }
            /* Variants 3.. walk the shape list from MONO_UP up, so every shape
             * but RANDWALK (which variant 0 already runs at the defaults) is
             * reached here — in EVERY language and at ambient K and
             * compatibility.
             *
             * The rotation alone does not reach them (#240). A function with
             * ONE parameter vector — every candlestick — has only v == 0, so
             * `(v + variant) % 7` yields shape 1 solely at variant 1, the
             * unstable-period leg, which a non-unstable function skips; and
             * shape 2 solely at variant 2, the Metastock leg, which the
             * languages without a compatibility API skip. So MONO_UP was run
             * by nobody and MONO_DOWN by C alone. Not academic: 58 of
             * CDLCOUNTERATTACK's 66 firing bars in this corpus are on
             * MONO_DOWN, and the MONO_DOWN leg is the one that caught a
             * one-bar ring rotation for it. */
            shape = (variant >= 3) ? (variant - 2) : (v + variant) % 7;
            stream_build_request(ctx->requestBuf, funcInfo, vec[v],
                                 shape, 1234 + v * 7 + variant, STREAM_N,
                                 K, compat);
            pipeErr = codegen_pipe_call(ctx->cp, ctx->requestBuf,
                                        ctx->responseBuf, JSON_BUF_SIZE);
            if( pipeErr != TA_TEST_PASS )
            {
                printf("STREAM PIPE FAIL [TA_%s]\n", funcInfo->name);
                ctx->error = pipeErr;
                return;
            }
            if( json_is_error(ctx->responseBuf) ||
                stream_flag(ctx->responseBuf, "\"ok\":") < 0 )
            {
                /* No stream on the server side. The ta_abstract flag is the
                 * public contract: a TA_FUNC_FLG_STREAM function without a
                 * server stream (or vice versa below) is a set mismatch. */
                if( funcInfo->flags & TA_FUNC_FLG_STREAM )
                {
                    printf("STREAM SET MISMATCH [TA_%s]: TA_FUNC_FLG_STREAM is set "
                           "but the server has no stream for it\n", funcInfo->name);
                    ctx->failed++;
                    ctx->error = TA_CODEGEN_STREAM_MISMATCH;
                    return;
                }
                ctx->streamSkipped++;
                return;
            }
            if( !(funcInfo->flags & TA_FUNC_FLG_STREAM) )
            {
                printf("STREAM SET MISMATCH [TA_%s]: server streams it but "
                       "TA_FUNC_FLG_STREAM is not set in ta_abstract\n", funcInfo->name);
                ctx->failed++;
                ctx->error = TA_CODEGEN_STREAM_MISMATCH;
                return;
            }
            /* OpenAndFill leg (loop tier today): the whole filled array ==
             * batch(0,n-1) bitwise. Checked before the generic ok flag so a
             * fill-only regression reports its own message; folded into ok
             * server-side too, so it fails even if this check regresses. */
            if( stream_flag(ctx->responseBuf, "\"fill_checked\":") == 1 )
            {
                fillChecked = 1;
                if( stream_flag(ctx->responseBuf, "\"fill_ok\":") != 1 )
                {
                    printf("STREAM FILL MISMATCH [TA_%s] vector=%d K=%d compat=%d "
                           "(OpenAndFill array != batch(0,n-1))\n"
                           "  request:  %s\n  response: %s\n",
                           funcInfo->name, v, K, compat,
                           ctx->requestBuf, ctx->responseBuf);
                    ctx->failed++;
                    ctx->error = TA_CODEGEN_STREAM_MISMATCH;
                    return;
                }
            }
            /* UpdateAndFill leg (#246): Open(P) plus ONE UpdateAndFill over the
             * remaining bars must write exactly what batch(0,n-1) reports for
             * those bars, reject an aliased output, and treat a zero count as a
             * no-op. Reported apart from fill_ok so a regression names which of
             * the two filling entry points broke; folded into ok server-side
             * too, so it fails even if this check regresses. */
            if( stream_flag(ctx->responseBuf, "\"ufill_checked\":") == 1 )
            {
                ufillChecked = 1;
                if( stream_flag(ctx->responseBuf, "\"ufill_ok\":") != 1 )
                {
                    printf("STREAM UPDATEFILL MISMATCH [TA_%s] vector=%d K=%d compat=%d "
                           "(UpdateAndFill over the tail != batch over the same bars)\n"
                           "  request:  %s\n  response: %s\n",
                           funcInfo->name, v, K, compat,
                           ctx->requestBuf, ctx->responseBuf);
                    ctx->failed++;
                    ctx->error = TA_CODEGEN_STREAM_MISMATCH;
                    return;
                }
            }
            /* State-equivalence leg (#240): the handle after Open(P) + (n-P)
             * updates must be bit-identical to the handle after Open(n). It
             * compares STATE, so it fires on the bar a running total first
             * diverges — independent of whether any pattern ever fires, which
             * is what the value comparison above cannot be for a candlestick
             * (a 3-valued output hides a total until it crosses a threshold).
             * Checked before the generic ok flag so the failure names the
             * field; folded into ok server-side too, so a regression here
             * still fails the run. */
            if( stream_flag(ctx->responseBuf, "\"state_checked\":") == 1 )
            {
                stateChecked = 1;
                stateLegs += stream_flag(ctx->responseBuf, "\"state_legs\":");
                stateOfLegs += stream_flag(ctx->responseBuf, "\"legs\":");
                if( stream_flag(ctx->responseBuf, "\"state_ok\":") != 1 )
                {
                    printf("STREAM STATE MISMATCH [TA_%s] vector=%d K=%d compat=%d\n"
                           "  Open(P)+updates and Open(n) left different handles\n"
                           "  request:  %s\n  response: %s\n",
                           funcInfo->name, v, K, compat,
                           ctx->requestBuf, ctx->responseBuf);
                    ctx->failed++;
                    ctx->error = TA_CODEGEN_STREAM_MISMATCH;
                    return;
                }
            }
            /* Range leg (#241): the handle's OutRange must equal what the batch
             * call reports over the same bars — for the OpenAndFill handle, for
             * Open(P) plus the updates that carry it to bar n-1, and for the
             * startIdx-anchored open. It ties the streaming tier to the batch
             * tier through one number pair, which no value leg does: every one
             * of those compares outputs, and an output is the same whether the
             * handle knows how many of them it has produced. Checked before the
             * generic ok flag so the failure names the leg; folded into ok
             * server-side too. */
            if( stream_flag(ctx->responseBuf, "\"range_checked\":") == 1 )
            {
                rangeChecked = 1;
                rangeLegs += stream_flag(ctx->responseBuf, "\"range_legs\":");
                {
                    /* Which of THIS server's range-compare sites fired, and how
                     * many it has. Reported by the server rather than held as a
                     * per-language constant here, so the two cannot drift when a
                     * language gains a site. */
                    int m = stream_flag(ctx->responseBuf, "\"range_sites\":");
                    int nsites = stream_flag(ctx->responseBuf, "\"range_sites_n\":");
                    if( m > 0 ) rangeSites |= m;
                    if( nsites > rangeSitesN ) rangeSitesN = nsites;
                }
                if( stream_flag(ctx->responseBuf, "\"range_ok\":") != 1 )
                {
                    printf("STREAM RANGE MISMATCH [TA_%s] vector=%d K=%d compat=%d\n"
                           "  a handle's OutRange != the batch range over the same bars\n"
                           "  request:  %s\n  response: %s\n",
                           funcInfo->name, v, K, compat,
                           ctx->requestBuf, ctx->responseBuf);
                    ctx->failed++;
                    ctx->error = TA_CODEGEN_STREAM_MISMATCH;
                    return;
                }
            }
            if( stream_flag(ctx->responseBuf, "\"ok\":") != 1 ||
                stream_flag(ctx->responseBuf, "\"peek_ok\":") != 1 )
            {
                printf("STREAM MISMATCH [TA_%s] vector=%d K=%d compat=%d\n"
                       "  request:  %s\n  response: %s\n",
                       funcInfo->name, v, K, compat,
                       ctx->requestBuf, ctx->responseBuf);
                ctx->failed++;
                ctx->error = TA_CODEGEN_STREAM_MISMATCH;
                return;
            }
            {
                int l = stream_flag(ctx->responseBuf, "\"legs\":");
                if( l > 0 ) legs += l;
                if( stream_flag(ctx->responseBuf, "\"unsupportedArm\":") == 1 )
                    rejArms++;
                /* Cross-tier +0.0/-0.0 pairs the server chose not to fail on
                 * (issue #147). Reported, never a failure — the same benign
                 * class --fuzz-064 carries. `-1` is a server that predates the
                 * field, which stream_flag reports as absent, not as a count. */
                {
                    int z = stream_flag(ctx->responseBuf, "\"benign\":");
                    if( z > 0 ) benign += z;
                }
            }
        }
    }
    /* Verified-leg floor: every stream-flagged function must contribute at
     * least one bit-exact leg. Expected-reject arms (unsupportedArm) answer
     * ok:1/legs:0 by design, so without this floor a plan-derivation
     * regression that marks every arm unsupported — making Open reject AND
     * the precheck bless the reject — would pass with zero comparisons. */
    if( legs <= 0 )
    {
        printf("STREAM VACUOUS [TA_%s]: 0 verified legs (%d expected-reject "
               "probes)\n", funcInfo->name, rejArms);
        ctx->failed++;
        ctx->error = TA_CODEGEN_STREAM_MISMATCH;
        return;
    }
    ctx->streamFunctions++;
    ctx->streamLegs += legs;
    ctx->streamRejectArms += rejArms;
    ctx->streamBenign += benign;
    /* Per-function, per-language, for the Java/C# equality check at the end of
     * the run — see FuncStreamCounters. */
    record_stream_counters(funcInfo->name, ctx->langIndex, legs, benign);
    if( fillChecked ) ctx->streamFillFunctions++;
    if( ufillChecked ) ctx->streamUFillFunctions++;
    if( stateChecked ) ctx->streamStateFunctions++;
    if( rangeChecked ) ctx->streamRangeFunctions++;
    ctx->streamRangeLegs += rangeLegs;
    ctx->streamRangeSites |= rangeSites;
    if( rangeSitesN > ctx->streamRangeSitesN ) ctx->streamRangeSitesN = rangeSitesN;
    /* Per-leg, not per-function: a function counts as covered as soon as ONE of
     * its legs compares, so without this a reference open that quietly failed
     * on 15 of 16 legs would still read as full coverage. Every leg that
     * compares values also compares state, so within the requests that answered
     * the state leg at all the two counts must agree. (Requests that did not —
     * a batch that produced nothing, an expected-reject arm — are excluded from
     * both sides rather than counted as a shortfall.) */
    if( stateChecked && stateLegs != stateOfLegs )
    {
        printf("STREAM STATE PARTIAL [TA_%s]: %d of %d legs compared handle "
               "state\n", funcInfo->name, stateLegs, stateOfLegs);
        ctx->failed++;
        ctx->error = TA_CODEGEN_STREAM_MISMATCH;
        return;
    }
    ctx->streamStateLegs += stateLegs;
    /* Named per function, like --fuzz-064's BENIGN line: a summary total that
     * starts moving says only that something did, not what. */
    if( benign > 0 )
        printf("  BENIGN TA_%s: %lld cross-tier signed-zero case(s) "
               "(numerically equal, +0.0 vs -0.0)\n", funcInfo->name, benign);
}

/* ---- Test orchestration (Task 9) ---- */

/* Cross-language parity for the boolean near-zero builtins (issue #107 follow-up).
 * IS_ZERO / IS_ZERO_SCALED / IS_ZERO_OR_NEG are emitted as different but
 * semantically-equal forms per backend (C two-sided macro, Rust `.abs() < eps`,
 * Java two-sided). The eval_predicate server method evaluates the SAME form the
 * indicators use; here we drive a finite boundary-input table through one
 * language server and require every 0/1 result to match the in-process C macro.
 * This is the only place IS_ZERO_SCALED's firing branch is exercised across
 * languages (MFI never fires it on the --codegen history data). */
static ErrorNumber test_predicate_parity(CodegenPipe *cp, const CodegenLanguage *lang,
                                         char *reqBuf, char *respBuf)
{
    /* Finite boundary table (NaN/inf excluded: the servers parse them
     * inconsistently and TA-Lib does not define NaN behaviour). Values are sent
     * with %.17g. The forms are semantically identical, so the goal is to catch a
     * *form* divergence (wrong epsilon, missing abs, wrong operator, un-applied
     * scale) — NOT sub-ULP JSON-transport differences. We therefore test values
     * comfortably inside/outside the band (0.5x/0.9x vs 1.1x/2x the threshold),
     * plus the exact boundary only at CLEAN values (0, +/-E, E*1.0) that strtod,
     * serde and Double.parseDouble all parse to the identical double. */
    double vals[160], scales[160];
    int n = 0;
    const double E = 1e-14;   /* TA_EPSILON */
    const double b1[] = {
        0.0, -0.0, E, -E,                        /* clean exact boundary (E is 1e-14) */
        0.5 * E, 0.9 * E, -0.5 * E, -0.9 * E,    /* inside the |v| < E band */
        1.1 * E, 2.0 * E, -1.1 * E, -2.0 * E,    /* outside */
        1.0, -1.0, 100.0, -100.0, 1e-300, -1e-300, 5e-324, -5e-324
    };
    for( unsigned k = 0; k < sizeof(b1) / sizeof(b1[0]) && n < 160; k++ )
    { vals[n] = b1[k]; scales[n] = 1.0; n++; }
    const double scaleSet[] = { 0.0, 1e-6, 1.0, 100.0, 1e6, 1e12 };
    for( unsigned k = 0; k < sizeof(scaleSet) / sizeof(scaleSet[0]); k++ )
    {
        double sc = scaleSet[k], thr = E * sc;
        /* Fractional offsets only — robust to a sub-ULP parse of the ugly E*sc
         * product; the clean scale==1 case above already exercises v==E exactly. */
        double around[] = { 0.5 * thr, 0.9 * thr, 1.1 * thr, 2.0 * thr,
                            -0.5 * thr, -1.1 * thr };
        for( unsigned j = 0; j < sizeof(around) / sizeof(around[0]) && n < 160; j++ )
        { vals[n] = around[j]; scales[n] = sc; n++; }
    }

    for( int which = 0; which <= 2; which++ )
    {
        int pos = codegen_appendf(reqBuf, JSON_BUF_SIZE, 0,
            "{\"method\":\"eval_predicate\",\"params\":{\"which\":%d,\"values\":[", which);
        for( int i = 0; i < n; i++ )
            pos = codegen_appendf(reqBuf, JSON_BUF_SIZE, pos, "%s%.17g", i ? "," : "", vals[i]);
        pos = codegen_appendf(reqBuf, JSON_BUF_SIZE, pos, "],\"scale\":[");
        for( int i = 0; i < n; i++ )
            pos = codegen_appendf(reqBuf, JSON_BUF_SIZE, pos, "%s%.17g", i ? "," : "", scales[i]);
        codegen_appendf(reqBuf, JSON_BUF_SIZE, pos, "]}}");

        if( codegen_pipe_call(cp, reqBuf, respBuf, JSON_BUF_SIZE) != TA_TEST_PASS
            || json_is_error(respBuf) )
        {
            printf("  PREDICATE PARITY [%s]: eval_predicate call failed (which=%d)\n",
                   lang->display, which);
            return TA_PREDICATE_PARITY_CALL_FAILED;
        }
        int got[160];
        int parsed = json_get_int_array(respBuf, "outInteger", got, 160);
        if( parsed != n )
        {
            printf("  PREDICATE PARITY [%s]: expected %d results, got %d (which=%d)\n",
                   lang->display, n, parsed, which);
            return TA_PREDICATE_PARITY_MISMATCH;
        }
        for( int i = 0; i < n; i++ )
        {
            double v = vals[i], s = scales[i];
            int truth = ( which == 1 ) ? ( TA_IS_ZERO_SCALED(v, s) ? 1 : 0 )
                      : ( which == 2 ) ? ( TA_IS_ZERO_OR_NEG(v)    ? 1 : 0 )
                      :                  ( TA_IS_ZERO(v)           ? 1 : 0 );
            if( got[i] != truth )
            {
                const char *pn = ( which == 1 ) ? "IS_ZERO_SCALED"
                               : ( which == 2 ) ? "IS_ZERO_OR_NEG" : "IS_ZERO";
                printf("  PREDICATE PARITY [%s]: %s(v=%.17g, scale=%.17g) = %d but C macro = %d\n",
                       lang->display, pn, v, s, got[i], truth);
                return TA_PREDICATE_PARITY_MISMATCH;
            }
        }
    }
    printf("  Predicate parity (IS_ZERO family): %d values x 3 builtins match the C macro\n", n);
    return TA_TEST_PASS;
}

/* TA_MAX_INDEX must bound startIdx/endIdx in EVERY backend, not just C
 * (issue #180). Without this, deleting the cap from java.rs, csharp.rs or
 * rust_lang.rs leaves every gate green: test_abstract.c's index-range gate
 * drives the in-process C library only, and no other driver sends an index
 * anywhere near the cap.
 *
 * SPANS ARE DELIBERATELY TINY. The servers size their output buffers
 * `endIdx - startIdx + 1` BEFORE dispatching, so the obvious probe
 * (startIdx=0, endIdx=TA_MAX_INDEX+1) would allocate 800MB per output on each
 * server and test the allocator rather than the guard. Every pair below spans
 * at most two elements while still being out of range.
 *
 * Two cases C covers that this cannot, by construction:
 *  - negative indices: JSON numbers reach the servers through unsigned parses,
 *    so a negative startIdx is not expressible over the wire.
 *  - endIdx == TA_MAX_INDEX accepted: proving it needs a call that gets PAST
 *    the range check, i.e. a real 800MB-per-array call. test_abstract.c reaches
 *    it in-process instead, via an out-of-range optional parameter.
 * What is left is exactly the part that can diverge silently: the two
 * rejections and the startIdx boundary.
 *
 * A backend missing the guard does not merely answer the wrong code — it
 * indexes an array of a few elements at ~1e8 and takes the server down. Both
 * outcomes fail here; only the diagnostic differs. */
typedef struct
{
    int         startIdx;
    int         endIdx;
    TA_RetCode  expected;
    const char *what;
    /* When non-NULL, appended to the request verbatim. Lets one table carry the
     * PARAMETER rejection beside the index ones: `--codegen` reached
     * TA_BAD_PARAM on no call at all in any language before this, so the one
     * code every backend routes its catch-all through was compared nowhere in
     * the default pipeline. (`--xlang-hash` does compare it, on the min-1 /
     * max+1 vector of every integer parameter — but it is not part of a plain
     * `--codegen` run, so the tier this step rerouted had no BadParam case
     * looking at it.) The census below is what made that visible. */
    const char *extraParams;
} XlangIndexRangeCase;

static ErrorNumber test_index_range_xlang(CodegenPipe *cp, const CodegenLanguage *lang,
                                          int langIndex,
                                          char *reqBuf, char *respBuf)
{
    static const XlangIndexRangeCase CASES[] = {
        { TA_MAX_INDEX+1, TA_MAX_INDEX+1, TA_OUT_OF_RANGE_START_INDEX,
          "startIdx > TA_MAX_INDEX", NULL },
        { TA_MAX_INDEX,   TA_MAX_INDEX+1, TA_OUT_OF_RANGE_END_INDEX,
          "endIdx > TA_MAX_INDEX", NULL },
        { 10,             9,              TA_OUT_OF_RANGE_END_INDEX,
          "endIdx < startIdx", NULL },
        { TA_MAX_INDEX,   TA_MAX_INDEX-1, TA_OUT_OF_RANGE_END_INDEX,
          "startIdx == TA_MAX_INDEX accepted", NULL },
        /* Valid indices, out-of-domain period: the catch-all, on a call every
         * backend has to reject the same way. `optInTimePeriod` is spelled
         * explicitly rather than left absent, because an absent field is a
         * different test in each server -- C and Java read 0, Rust substitutes
         * the documented default -- and this case is about the DOMAIN check, not
         * about what a missing field means. TA_AD takes no optional parameter and
         * is skipped. */
        { 0,              5,              TA_BAD_PARAM,
          "optInTimePeriod below its documented range", ",\"optInTimePeriod\":0" }
    };
    /* One per input shape: a single real array, a real array with several
     * outputs, and a full price bundle. The prologue is emitted from one
     * template, so this is about shapes, not about coverage of all 168. */
    static const char *const FUNCS[] = { "TA_SMA", "TA_BBANDS", "TA_AD" };
    static const int NB = 8;
    double data[8] = { 10.0, 11.0, 12.0, 11.5, 13.0, 12.5, 14.0, 13.5 };
    unsigned int f, c;
    int nbChecked = 0;

    for( f = 0; f < sizeof(FUNCS)/sizeof(FUNCS[0]); f++ )
    {
        for( c = 0; c < sizeof(CASES)/sizeof(CASES[0]); c++ )
        {
            const XlangIndexRangeCase *tc = &CASES[c];
            int rc, pos;

            /* A parameter case needs a parameter. TA_AD has none. */
            if( tc->extraParams != NULL && strcmp(FUNCS[f], "TA_AD") == 0 )
                continue;

            pos = codegen_appendf(reqBuf, JSON_BUF_SIZE, 0,
                    "{\"method\":\"%s\",\"params\":{\"startIdx\":%d,\"endIdx\":%d",
                    FUNCS[f], tc->startIdx, tc->endIdx);
            if( strcmp(FUNCS[f], "TA_AD") == 0 )
            {
                pos = codegen_appendf(reqBuf, JSON_BUF_SIZE, pos, ",\"inHigh\":");
                pos = json_write_double_array(reqBuf, JSON_BUF_SIZE, pos, data, NB, 0);
                pos = codegen_appendf(reqBuf, JSON_BUF_SIZE, pos, ",\"inLow\":");
                pos = json_write_double_array(reqBuf, JSON_BUF_SIZE, pos, data, NB, 0);
                pos = codegen_appendf(reqBuf, JSON_BUF_SIZE, pos, ",\"inClose\":");
                pos = json_write_double_array(reqBuf, JSON_BUF_SIZE, pos, data, NB, 0);
                pos = codegen_appendf(reqBuf, JSON_BUF_SIZE, pos, ",\"inVolume\":");
                pos = json_write_double_array(reqBuf, JSON_BUF_SIZE, pos, data, NB, 0);
            }
            else
            {
                pos = codegen_appendf(reqBuf, JSON_BUF_SIZE, pos, ",\"inReal\":");
                pos = json_write_double_array(reqBuf, JSON_BUF_SIZE, pos, data, NB, 0);
            }
            if( tc->extraParams != NULL )
                pos = codegen_appendf(reqBuf, JSON_BUF_SIZE, pos, "%s", tc->extraParams);
            codegen_appendf(reqBuf, JSON_BUF_SIZE, pos, "}}");

            if( codegen_pipe_call(cp, reqBuf, respBuf, JSON_BUF_SIZE) != TA_TEST_PASS
                || json_is_error(respBuf) )
            {
                printf("  INDEX RANGE XLANG [%s]: %s %s (startIdx=%d endIdx=%d) "
                       "call failed: %s\n",
                       lang->display, FUNCS[f], tc->what, tc->startIdx, tc->endIdx,
                       respBuf);
                return TA_INDEX_RANGE_XLANG_CALL_FAILED;
            }
            rc = json_get_int(respBuf, "retCode");
            /* This leg is where OUT_OF_RANGE_{START,END}_INDEX come from — the
             * census floor below would have nothing to stand on without it. */
            record_retcode(langIndex, rc);
            if( rc != (int)tc->expected )
            {
                printf("  INDEX RANGE XLANG [%s]: %s %s (startIdx=%d endIdx=%d) "
                       "returned %d, C returns %d\n",
                       lang->display, FUNCS[f], tc->what, tc->startIdx, tc->endIdx,
                       rc, (int)tc->expected);
                return TA_INDEX_RANGE_XLANG_MISMATCH;
            }
            nbChecked++;
        }
    }
    printf("  Index range (#180): %d case(s) match C's TA_MAX_INDEX contract\n",
           nbChecked);
    return TA_TEST_PASS;
}

/* set_unstable_period's set-all wildcard (id == TA_FUNC_UNST_ALL) must really
 * reach every function on every server (issue #144).
 *
 * Nothing proved that before: server_verify sends the wildcard exactly once, to
 * reset to *zero*, which a server that mishandles the sentinel answers
 * identically because its array is already zero. The Java server did mishandle
 * it — it sized `unstablePeriod` by the enum's length (a slot per sentinel) and
 * compared the wildcard against that length, so the id the driver sent landed in
 * an unread slot and "set all" was inert.
 *
 * ADOSC is the probe because it is the rare shape that can observe the server's
 * *stored* state: it carries no `unstablePeriod` request field of its own (so
 * the call cannot re-set what it is meant to read), yet its lookback is
 * TA_EMA_Lookback, which adds EMA's unstable period. The expected outBegIdx is
 * taken from the in-process C library under the same setting rather than
 * hardcoded, and the two legs are asserted to differ so a lookback that stopped
 * depending on the unstable period turns into a failure, not a silent pass. */
static ErrorNumber test_unstable_wildcard(CodegenPipe *cp, const CodegenLanguage *lang,
                                          char *reqBuf, char *respBuf)
{
    #define UW_NBBAR 60
    #define UW_FAST  3
    #define UW_SLOW  10
    TA_Real h[UW_NBBAR], l[UW_NBBAR], c[UW_NBBAR], v[UW_NBBAR];
    const int periods[2] = { 5, 0 };   /* non-zero first, then restore to zero */
    int expected[2];

    for( int i = 0; i < UW_NBBAR; i++ )
    {
        double base = 100.0 + (double)((i * 7) % 13);
        h[i] = base + 2.0;
        l[i] = base - 2.0;
        c[i] = base + 0.5;
        v[i] = 1000.0 + (double)((i * 3) % 17) * 10.0;
    }

    /* Non-vacuity: the probe is only meaningful if the two settings really do
     * move ADOSC's lookback in the C library. */
    for( int k = 0; k < 2; k++ )
    {
        TA_SetUnstablePeriod(TA_FUNC_UNST_ALL, (unsigned int)periods[k]);
        expected[k] = TA_ADOSC_Lookback(UW_FAST, UW_SLOW);
    }
    TA_SetUnstablePeriod(TA_FUNC_UNST_ALL, 0);
    if( expected[0] == expected[1] || expected[0] < 0 || expected[1] < 0 )
    {
        printf("  UNSTABLE WILDCARD [%s]: probe is vacuous — ADOSC lookback is %d for "
               "unstable %d and %d\n", lang->display, expected[0], periods[0], periods[1]);
        return TA_UNSTABLE_WILDCARD_VACUOUS;
    }

    for( int k = 0; k < 2; k++ )
    {
        codegen_appendf(reqBuf, JSON_BUF_SIZE, 0,
                "{\"method\":\"set_unstable_period\",\"params\":{\"id\":%d,\"period\":%d}}",
                (int)TA_FUNC_UNST_ALL, periods[k]);
        if( codegen_pipe_call(cp, reqBuf, respBuf, JSON_BUF_SIZE) != TA_TEST_PASS
            || json_is_error(respBuf) )
        {
            printf("  UNSTABLE WILDCARD [%s]: set_unstable_period(ALL, %d) failed: %s\n",
                   lang->display, periods[k], respBuf);
            return TA_UNSTABLE_WILDCARD_CALL_FAILED;
        }

        int pos = codegen_appendf(reqBuf, JSON_BUF_SIZE, 0,
                "{\"method\":\"TA_ADOSC\",\"params\":{\"startIdx\":0,\"endIdx\":%d,"
                "\"optInFastPeriod\":%d,\"optInSlowPeriod\":%d,\"inHigh\":",
                UW_NBBAR - 1, UW_FAST, UW_SLOW);
        pos = json_write_double_array(reqBuf, JSON_BUF_SIZE, pos, h, UW_NBBAR, 0);
        pos = codegen_appendf(reqBuf, JSON_BUF_SIZE, pos, ",\"inLow\":");
        pos = json_write_double_array(reqBuf, JSON_BUF_SIZE, pos, l, UW_NBBAR, 0);
        pos = codegen_appendf(reqBuf, JSON_BUF_SIZE, pos, ",\"inClose\":");
        pos = json_write_double_array(reqBuf, JSON_BUF_SIZE, pos, c, UW_NBBAR, 0);
        pos = codegen_appendf(reqBuf, JSON_BUF_SIZE, pos, ",\"inVolume\":");
        pos = json_write_double_array(reqBuf, JSON_BUF_SIZE, pos, v, UW_NBBAR, 0);
        codegen_appendf(reqBuf, JSON_BUF_SIZE, pos, "}}");

        if( codegen_pipe_call(cp, reqBuf, respBuf, JSON_BUF_SIZE) != TA_TEST_PASS
            || json_is_error(respBuf) )
        {
            printf("  UNSTABLE WILDCARD [%s]: TA_ADOSC call failed: %s\n",
                   lang->display, respBuf);
            return TA_UNSTABLE_WILDCARD_CALL_FAILED;
        }
        int rc     = json_get_int(respBuf, "retCode");
        int begIdx = json_get_int(respBuf, "outBegIdx");
        if( rc != (int)TA_SUCCESS || begIdx != expected[k] )
        {
            printf("  UNSTABLE WILDCARD [%s]: after set_unstable_period(ALL, %d), "
                   "ADOSC outBegIdx = %d (retCode %d) but C says %d — the wildcard did "
                   "not reach every function\n",
                   lang->display, periods[k], begIdx, rc, expected[k]);
            return TA_UNSTABLE_WILDCARD_MISMATCH;
        }
    }

    printf("  Unstable-period wildcard: set-all reaches EMA (ADOSC outBegIdx %d at "
           "unstable %d, %d at %d)\n",
           expected[0], periods[0], expected[1], periods[1]);
    return TA_TEST_PASS;
    #undef UW_NBBAR
    #undef UW_FAST
    #undef UW_SLOW
}

/* The unstable period's VALUE domain, held identical on every server (#186).
 *
 * The wildcard probe above proves a legal period reaches every function; this
 * proves an illegal one reaches none of them. Until #186 the four backends
 * disagreed outright — C bounded the value, Java bounded only the low end, Rust
 * bounded nothing and took a signed parameter, C# had no setter at all — and no
 * cross-language run could see it, because every period the harness had ever
 * sent was legal.
 *
 * Three things are asserted, in the order that makes each one non-vacuous:
 *   1. TA_MAX_INDEX itself is ACCEPTED. Without this the whole check passes
 *      against a server that rejects everything, and a guard tightened by one
 *      ships unnoticed.
 *   2. TA_MAX_INDEX + 1 and 2^31-1 are REJECTED, on the single-id path and on
 *      the set-all wildcard alike.
 *   3. A rejected call WROTE NOTHING. This is the half an "it errored" check
 *      cannot see, and the half C's own test pins (test_internals.c). ADOSC is
 *      the probe for the same reason the wildcard test uses it: it carries no
 *      `unstablePeriod` request field, so the call cannot re-set what it reads,
 *      yet its lookback is TA_EMA_Lookback and so moves with EMA's period. */
static ErrorNumber test_unstable_bounds(CodegenPipe *cp, const CodegenLanguage *lang,
                                        char *reqBuf, char *respBuf)
{
    #define UB_NBBAR 60
    #define UB_FAST  3
    #define UB_SLOW  10
    TA_Real h[UB_NBBAR], l[UB_NBBAR], c[UB_NBBAR], v[UB_NBBAR];
    /* Values every backend must refuse. TA_MAX_INDEX+1 is the first one past the
     * ceiling; 2^31-1 is the value that overflowed the lookback negative. */
    const long long rejects[2] = { (long long)TA_MAX_INDEX + 1, 2147483647LL };
    const int ids[2] = { (int)TA_FUNC_UNST_EMA, (int)TA_FUNC_UNST_ALL };
    const int marker = 4;   /* the good value a rejected call must not disturb */
    int expected, i, k, r;

    for( i = 0; i < UB_NBBAR; i++ )
    {
        double base = 100.0 + (double)((i * 7) % 13);
        h[i] = base + 2.0;
        l[i] = base - 2.0;
        c[i] = base + 0.5;
        v[i] = 1000.0 + (double)((i * 3) % 17) * 10.0;
    }

    /* (1) The ceiling is a bound, not an off-by-one: TA_MAX_INDEX is legal. */
    for( k = 0; k < 2; k++ )
    {
        codegen_appendf(reqBuf, JSON_BUF_SIZE, 0,
                "{\"method\":\"set_unstable_period\",\"params\":{\"id\":%d,\"period\":%d}}",
                ids[k], (int)TA_MAX_INDEX);
        if( codegen_pipe_call(cp, reqBuf, respBuf, JSON_BUF_SIZE) != TA_TEST_PASS
            || json_is_error(respBuf) )
        {
            printf("  UNSTABLE BOUND [%s]: id %d rejected the TA_MAX_INDEX ceiling (%d), "
                   "which C accepts: %s\n", lang->display, ids[k], (int)TA_MAX_INDEX, respBuf);
            return TA_UNSTABLE_BOUND_CEILING;
        }
    }

    /* Park a known-good value so step (3) has something to observe. */
    codegen_appendf(reqBuf, JSON_BUF_SIZE, 0,
            "{\"method\":\"set_unstable_period\",\"params\":{\"id\":%d,\"period\":%d}}",
            (int)TA_FUNC_UNST_ALL, marker);
    if( codegen_pipe_call(cp, reqBuf, respBuf, JSON_BUF_SIZE) != TA_TEST_PASS
        || json_is_error(respBuf) )
    {
        printf("  UNSTABLE BOUND [%s]: could not park the marker period: %s\n",
               lang->display, respBuf);
        return TA_UNSTABLE_BOUND_CEILING;
    }

    TA_SetUnstablePeriod(TA_FUNC_UNST_ALL, (unsigned int)marker);
    expected = TA_ADOSC_Lookback(UB_FAST, UB_SLOW);
    TA_SetUnstablePeriod(TA_FUNC_UNST_ALL, 0);

    /* (2) and (3): each out-of-range value is refused, and leaves the marker. */
    for( r = 0; r < 2; r++ )
    {
        for( k = 0; k < 2; k++ )
        {
            codegen_appendf(reqBuf, JSON_BUF_SIZE, 0,
                    "{\"method\":\"set_unstable_period\",\"params\":{\"id\":%d,\"period\":%lld}}",
                    ids[k], rejects[r]);
            if( codegen_pipe_call(cp, reqBuf, respBuf, JSON_BUF_SIZE) != TA_TEST_PASS )
            {
                printf("  UNSTABLE BOUND [%s]: transport failed for id %d period %lld\n",
                       lang->display, ids[k], rejects[r]);
                return TA_UNSTABLE_BOUND_NOT_REJECTED;
            }
            if( !json_is_error(respBuf) )
            {
                printf("  UNSTABLE BOUND [%s]: id %d ACCEPTED period %lld, which C rejects "
                       "with TA_BAD_PARAM: %s\n",
                       lang->display, ids[k], rejects[r], respBuf);
                return TA_UNSTABLE_BOUND_NOT_REJECTED;
            }

            /* The rejected call must not have written. */
            int pos = codegen_appendf(reqBuf, JSON_BUF_SIZE, 0,
                    "{\"method\":\"TA_ADOSC\",\"params\":{\"startIdx\":0,\"endIdx\":%d,"
                    "\"optInFastPeriod\":%d,\"optInSlowPeriod\":%d,\"inHigh\":",
                    UB_NBBAR - 1, UB_FAST, UB_SLOW);
            pos = json_write_double_array(reqBuf, JSON_BUF_SIZE, pos, h, UB_NBBAR, 0);
            pos = codegen_appendf(reqBuf, JSON_BUF_SIZE, pos, ",\"inLow\":");
            pos = json_write_double_array(reqBuf, JSON_BUF_SIZE, pos, l, UB_NBBAR, 0);
            pos = codegen_appendf(reqBuf, JSON_BUF_SIZE, pos, ",\"inClose\":");
            pos = json_write_double_array(reqBuf, JSON_BUF_SIZE, pos, c, UB_NBBAR, 0);
            pos = codegen_appendf(reqBuf, JSON_BUF_SIZE, pos, ",\"inVolume\":");
            pos = json_write_double_array(reqBuf, JSON_BUF_SIZE, pos, v, UB_NBBAR, 0);
            codegen_appendf(reqBuf, JSON_BUF_SIZE, pos, "}}");

            if( codegen_pipe_call(cp, reqBuf, respBuf, JSON_BUF_SIZE) != TA_TEST_PASS
                || json_is_error(respBuf) )
            {
                printf("  UNSTABLE BOUND [%s]: TA_ADOSC call failed after a rejected "
                       "period: %s\n", lang->display, respBuf);
                return TA_UNSTABLE_BOUND_WROTE_ANYWAY;
            }
            if( json_get_int(respBuf, "outBegIdx") != expected )
            {
                printf("  UNSTABLE BOUND [%s]: id %d period %lld was refused but still "
                       "changed the stored state — ADOSC outBegIdx = %d, expected %d "
                       "(the marker %d)\n",
                       lang->display, ids[k], rejects[r],
                       json_get_int(respBuf, "outBegIdx"), expected, marker);
                return TA_UNSTABLE_BOUND_WROTE_ANYWAY;
            }
        }
    }

    /* Leave the server as we found it. */
    codegen_appendf(reqBuf, JSON_BUF_SIZE, 0,
            "{\"method\":\"set_unstable_period\",\"params\":{\"id\":%d,\"period\":0}}",
            (int)TA_FUNC_UNST_ALL);
    codegen_pipe_call(cp, reqBuf, respBuf, JSON_BUF_SIZE);

    printf("  Unstable-period bounds: %d accepted, %lld and %lld refused with no write\n",
           (int)TA_MAX_INDEX, rejects[0], rejects[1]);
    return TA_TEST_PASS;
    #undef UB_NBBAR
    #undef UB_FAST
    #undef UB_SLOW
}

/* Fuzz-port self-check for stream-capable servers (capability-gated): a
 * server that PORTS fuzz_gen (Java FuzzData, Rust fuzz.rs) must reproduce the
 * driver's inputs byte-identically, or every stream leg silently exercises
 * different data. Servers that compile fuzz_data.h directly (the C server)
 * answer no fuzz_in_hash and are skipped. Returns the number of mismatched
 * shapes (0 = port bit-identical or no port to check). */
#ifndef FUZZ_MAXN
#define FUZZ_MAXN 256   /* bars per config (kept equal to the fuzz section's define) */
#endif
static unsigned long long xlang_in_hash_local(int shape, int seed, int n);
static unsigned long long xlang_parse_hash(const char *resp, const char *field, int *present);
static int stream_fuzz_port_selfcheck(CodegenPipe *cp, char *requestBuf, char *responseBuf)
{
    int fuzzChecked = 0, fuzzFails = 0, shape;
    int n = 240;
    if( n > FUZZ_MAXN ) n = FUZZ_MAXN;
    for( shape = 0; shape < FUZZ_NSHAPES; shape++ )
    {
        int present = 0;
        unsigned long long ih;
        codegen_appendf(requestBuf, JSON_BUF_SIZE, 0,
                "{\"method\":\"fuzz_in_hash\",\"params\":{"
                "\"gen_shape\":%d,\"gen_seed\":7,\"gen_n\":%d}}", shape, n);
        if( codegen_pipe_call(cp, requestBuf, responseBuf, JSON_BUF_SIZE) != TA_TEST_PASS )
            break;
        ih = xlang_parse_hash(responseBuf, "in_hash", &present);
        if( !present ) break;   /* in-process fuzz (C server) — nothing to check */
        fuzzChecked++;
        if( ih != xlang_in_hash_local(shape, 7, n) )
        {
            printf("  STREAM FUZZ PORT MISMATCH shape=%d (server fuzz_gen != C)\n", shape);
            fuzzFails++;
        }
    }
    if( fuzzChecked > 0 )
        printf("  Fuzz-port self-check: %d/%d shapes bit-identical\n",
               fuzzChecked - fuzzFails, fuzzChecked);
    return fuzzFails;
}

static ErrorNumber test_codegen_for_language(
    const CodegenLanguage *lang,
    int langIndex,
    const TA_History *history,
    const char *functionFilter,
    CodegenPipe *refCp)
{
    CodegenPipe cp;
    ErrorNumber errNb;

    printf("\n");
    printf("Codegen verification: %s\n", lang->display);
    printf("---------------------------------------------\n");

    errNb = codegen_pipe_open(&cp, lang->argv);
    if( errNb != TA_TEST_PASS )
    {
        printf("FAILED: Cannot start %s server", lang->display);
        if( strcmp(lang->name, "rust") == 0 )
            printf(" (is ./ta_codegen built?)");
        else
            printf(" (run: ta_codegen build --lang=%s)", lang->name);
        printf("\n");
        return errNb;
    }
    printf("  Server started (pid=%d)\n", cp.child_pid);

    /* Allocate reusable JSON buffers */
    char *requestBuf = malloc(JSON_BUF_SIZE);
    char *responseBuf = malloc(JSON_BUF_SIZE);
    if( !requestBuf || !responseBuf )
    {
        free(requestBuf);
        free(responseBuf);
        codegen_pipe_close(&cp);
        return TA_CODEGEN_ALLOC_FAILED;
    }

    /* Use TA_ForEachFunc to iterate all functions */
    ForEachFuncContext ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.history        = history;
    ctx.functionFilter = functionFilter;
    ctx.cp             = &cp;
    ctx.refCp          = refCp;
    ctx.requestBuf     = requestBuf;
    ctx.responseBuf    = responseBuf;
    ctx.error          = TA_TEST_PASS;
    ctx.passed         = 0;
    ctx.failed         = 0;
    ctx.skipped        = 0;
    ctx.nbSkipNames    = 0;
    ctx.nbIntInputSkipNames = 0;
    ctx.sweepSkipped   = 0;
    ctx.postCutRangeChecked = 0;
    ctx.postCutRangeValueCompared = 0;
    ctx.langIndex      = langIndex;
    ctx.lang           = lang;

    /* Cache the frozen reference's supported-function set for the subset gate:
     * functions added after the pinned tag have no ta_ref_serve baseline and are
     * skipped (see test_one_function / sweep_one_function). Mirrors --fuzz-064. */
    char *refFuncList = NULL;
    if( refCp )
    {
        refFuncList = malloc(JSON_BUF_SIZE);
        if( refFuncList
            && codegen_pipe_call(refCp, "{\"method\":\"list_functions\",\"params\":{}}",
                                 refFuncList, JSON_BUF_SIZE) == TA_TEST_PASS
            && strstr(refFuncList, "\"functions\"") )
            ctx.refFuncList = refFuncList;
        else
        {
            /* Fail, don't warn. With refFuncList NULL the subset gate is off,
             * every function is compared against a reference that does not
             * have all of them, and the run still prints "0 skipped" — which
             * reads as MORE coverage than a healthy run, not less (#137). */
            printf("\nCODEGEN FAILED: ta_ref_serve list_functions failed, so the "
                   "subset gate cannot be applied.\n"
                   "  Continuing would report '0 skipped' while silently comparing "
                   "post-cutover functions\n  against a reference that lacks them.\n");
            free(refFuncList);
            free(requestBuf);
            free(responseBuf);
            codegen_pipe_close(&cp);
            return TA_CODEGEN_SUBSET_GATE_UNAVAILABLE;
        }
    }

    TA_ForEachFunc(test_one_function, &ctx);

    /* Cross-language boolean-builtin parity (IS_ZERO family) vs the in-process
     * C macro. Independent of the frozen reference (ta_ref_serve predates the
     * eval_predicate method), so it runs against the current language server. */
    if( ctx.error == TA_TEST_PASS )
    {
        ErrorNumber predErr = test_predicate_parity(&cp, lang, requestBuf, responseBuf);
        if( predErr != TA_TEST_PASS )
        {
            ctx.error = predErr;
            ctx.failed++;
        }
    }

    /* TA_MAX_INDEX bounds startIdx/endIdx in this backend too (#180). Not
     * frozen-reference-dependent: ta_ref_serve predates the cap, so the
     * expectation comes from the in-process C contract. */
    if( ctx.error == TA_TEST_PASS )
    {
        ErrorNumber idxErr = test_index_range_xlang(&cp, lang, langIndex, requestBuf, responseBuf);
        if( idxErr != TA_TEST_PASS )
        {
            ctx.error = idxErr;
            ctx.failed++;
        }
    }

    /* set_unstable_period's set-all wildcard actually reaches every function
     * (#144). Leaves the server back at all-zeros for the passes below. */
    if( ctx.error == TA_TEST_PASS )
    {
        ErrorNumber unstErr = test_unstable_wildcard(&cp, lang, requestBuf, responseBuf);
        if( unstErr != TA_TEST_PASS )
        {
            ctx.error = unstErr;
            ctx.failed++;
        }
    }

    /* The value domain that wildcard leaves untested: TA_MAX_INDEX accepted,
     * anything above it refused with no write, on every server (#186). Also
     * leaves the server back at all-zeros. */
    if( ctx.error == TA_TEST_PASS )
    {
        ErrorNumber boundErr = test_unstable_bounds(&cp, lang, requestBuf, responseBuf);
        if( boundErr != TA_TEST_PASS )
        {
            ctx.error = boundErr;
            ctx.failed++;
        }
    }

    /* Ref differential sweep: broaden the ta_ref_serve comparison beyond the
     * default and large-period points (see sweep_one_function). */
    if( ctx.error == TA_TEST_PASS && refCp )
    {
        ctx.sweepVariants  = 0;
        ctx.sweepFunctions = 0;
        g_frozenEnumSkips  = 0;
        TA_ForEachFunc(sweep_one_function, &ctx);

        /* Vacuity floor. A sweep MISMATCH is already loud (REF SWEEP FAIL,
         * nonzero exit); this catches the opposite — a green run that verified
         * nothing. Every guard in sweep_one_function `return`s without failing:
         * the filter, the nbOptInput bounds, and the refFuncList subset gate. So
         * an empty/garbled refFuncList, or a broken enumeration, prints
         * "0 variants across 0 functions, all match ta_ref_serve" and passes.
         * Two floors, both self-scaling (no coverage constant to maintain):
         * an unfiltered sweep must verify something, and it must not skip more
         * functions as absent-from-the-reference than it actually sweeps.
         * Skipped under --function, where sweeping one function — or zero, for a
         * post-reference one like CMF — is exactly the intent. Same shape as the
         * STREAM VACUOUS / STREAM FILL VACUOUS floors above. */
        if( ctx.error == TA_TEST_PASS && ctx.functionFilter == NULL )
        {
            if( ctx.sweepFunctions <= 0 || ctx.sweepVariants <= 0 )
            {
                printf("REF SWEEP VACUOUS: %d variants across %d functions — an "
                       "unfiltered sweep must verify something\n",
                       ctx.sweepVariants, ctx.sweepFunctions);
                ctx.error = TA_CODEGEN_SWEEP_VACUOUS;
                ctx.failed++;
            }
            else if( ctx.sweepSkipped > ctx.sweepFunctions )
            {
                printf("REF SWEEP VACUOUS: %d function(s) skipped as absent from "
                       "ta_ref_serve, only %d swept — the subset gate is skipping "
                       "more than it verifies\n",
                       ctx.sweepSkipped, ctx.sweepFunctions);
                ctx.error = TA_CODEGEN_SWEEP_VACUOUS;
                ctx.failed++;
            }
        }

        printf("  Ref differential sweep: %d variants across %d functions%s\n",
               ctx.sweepVariants, ctx.sweepFunctions,
               ctx.error == TA_TEST_PASS ? ", all match ta_ref_serve" : "");
        if( g_frozenEnumSkips > 0 )
            printf("  post-freeze enums: %lld MAType value(s) > %d excluded vs ta_ref_serve "
                   "(#139, #93, #182; covered current-vs-current by xlang-hash/stream/COMPOSITE)\n",
                   g_frozenEnumSkips, FROZEN_ORACLE_MATYPE_MAX);
    }

    /* Stream verification: batch-vs-stream bitwise, computed in-server.
     * Capability probe first: only a server implementing stream_verify
     * answers an unknown-function probe with "not_streamable"; anything
     * else (unknown method, loose foreign dispatch) would misfire on the
     * real requests, so the pass is skipped for that server. */
    if( ctx.error == TA_TEST_PASS )
    {
        ErrorNumber probeErr;
        codegen_appendf(requestBuf, JSON_BUF_SIZE, 0,
                "{\"method\":\"stream_verify\",\"params\":{\"funcName\":\"TA_STREAM_PROBE\","
                "\"gen_shape\":0,\"gen_seed\":1,\"gen_n\":2,\"unstablePeriod\":0,\"compatibility\":0}}");
        probeErr = codegen_pipe_call(&cp, requestBuf, responseBuf, JSON_BUF_SIZE);
        if( probeErr == TA_TEST_PASS && strstr(responseBuf, "not_streamable") )
        {
            if( stream_fuzz_port_selfcheck(&cp, requestBuf, responseBuf) > 0 )
                ctx.error = TA_CODEGEN_OUTPUT_MISMATCH;
            ctx.streamFunctions     = 0;
            ctx.streamLegs          = 0;
            ctx.streamSkipped       = 0;
            ctx.streamRejectArms    = 0;
            ctx.streamFillFunctions = 0;
            ctx.streamUFillFunctions = 0;
            ctx.streamStateFunctions = 0;
            ctx.streamStateLegs     = 0;
            ctx.streamRangeFunctions = 0;
            ctx.streamRangeLegs     = 0;
            ctx.streamRangeSites    = 0;
            ctx.streamRangeSitesN   = 0;
            ctx.streamBenign        = 0;
            TA_ForEachFunc(stream_one_function, &ctx);
            /* The benign total is printed unconditionally, zero included: the
             * whole point of counting the +/-0 class rather than ignoring it is
             * that a change which starts flipping zeros shows up as a number
             * moving off 0, in a line that is always there to compare against. */
            printf("  Stream verify: %d functions, %d legs bit-exact vs batch, "
                   "%d expected-reject probes, %d without a stream, "
                   "%lld benign signed-zero\n"
                   "  OpenAndFill verify: %d functions, filled array == batch(0,n-1) bitwise\n"
                   "  UpdateAndFill verify: %d functions, n bars in one call == batch over the same bars\n"
                   "  State-equivalence verify: %d functions, %d legs, handle after "
                   "Open(P)+updates == handle after Open(n) bitwise\n"
                   "  OutRange verify: %d functions, %d legs across %d of %d compare "
                   "site(s), the handle's range == the batch range over the same bars\n",
                   ctx.streamFunctions, ctx.streamLegs, ctx.streamRejectArms,
                   ctx.streamSkipped, ctx.streamBenign, ctx.streamFillFunctions,
                   ctx.streamUFillFunctions,
                   ctx.streamStateFunctions, ctx.streamStateLegs,
                   ctx.streamRangeFunctions, ctx.streamRangeLegs,
                   codegen_popcount(ctx.streamRangeSites), ctx.streamRangeSitesN);
            /* Coverage ratchet: every function with a server stream must ALSO
             * verify OpenAndFill (the emit side and this verify side both gate on
             * the same has_open_and_fill, so they cannot desync silently — but if
             * a future tier stops emitting OpenAndFill, that function still streams
             * and passes the legs floor while emitting no fill leg. This floor
             * fails loudly the moment fill coverage drops below stream coverage,
             * the OpenAndFill analogue of the legs<=0 STREAM VACUOUS floor. */
            if( ctx.error == TA_TEST_PASS &&
                ctx.streamFillFunctions != ctx.streamFunctions )
            {
                printf("STREAM FILL VACUOUS: only %d of %d streaming functions "
                       "verified OpenAndFill — every streamable function must also "
                       "gate-verify its fill array\n",
                       ctx.streamFillFunctions, ctx.streamFunctions);
                ctx.error = TA_CODEGEN_STREAM_MISMATCH;
            }
            /* The same ratchet for UpdateAndFill (#246). It has the same shape
             * as the fill floor above and exists for the same reason: the leg
             * is unconditional in every server, so a function that streams and
             * reports no UpdateAndFill leg is a tier whose emitter was missed —
             * which the legs floor and the value legs both read as full
             * coverage. */
            if( ctx.error == TA_TEST_PASS &&
                ctx.streamUFillFunctions != ctx.streamFunctions )
            {
                printf("STREAM UPDATEFILL VACUOUS: only %d of %d streaming functions "
                       "verified UpdateAndFill — every streamable function must also "
                       "gate-verify its n-bar filler\n",
                       ctx.streamUFillFunctions, ctx.streamFunctions);
                ctx.error = TA_CODEGEN_STREAM_MISMATCH;
            }
            /* The same ratchet for the state-equivalence leg (#240). The
             * comparators are generated from the state struct itself and drop
             * out as a SET when a sub-handle's callee loses its own, so a
             * quietly shrinking set is the failure mode to catch; on the
             * languages that do not emit the leg the count is 0 and the floor
             * is the language's own (0 == not offered), never a partial. */
            if( ctx.error == TA_TEST_PASS &&
                ctx.streamStateFunctions != 0 &&
                ctx.streamStateFunctions != ctx.streamFunctions )
            {
                printf("STREAM STATE PARTIAL: only %d of %d streaming functions "
                       "compared handle state — the generated comparator set has "
                       "shrunk (a sub-handle callee lost its comparator, or a new "
                       "state field has no rule)\n",
                       ctx.streamStateFunctions, ctx.streamFunctions);
                ctx.error = TA_CODEGEN_STREAM_MISMATCH;
            }
            if( ctx.error == TA_TEST_PASS && ctx.streamStateFunctions == 0 &&
                codegen_lang_has_stream_state_probe(lang->name) )
            {
                printf("STREAM STATE VACUOUS: the %s server offers the "
                       "state-equivalence leg but reported it for 0 of %d "
                       "streaming functions\n", lang->name, ctx.streamFunctions);
                ctx.error = TA_CODEGEN_STREAM_MISMATCH;
            }
            /* The range leg's ratchet (#241). Unlike the state leg this one is
             * offered by EVERY server — the range is public API in all four
             * backends — so the floor is unconditional: any streaming function
             * that did not report it is a tier whose emitter was missed, and a
             * count of 0 is a server that stopped answering the leg entirely.
             * Both read as full coverage without this. */
            if( ctx.error == TA_TEST_PASS &&
                ctx.streamRangeFunctions != ctx.streamFunctions )
            {
                printf("STREAM RANGE VACUOUS: only %d of %d streaming functions "
                       "compared the handle's OutRange against the batch range\n",
                       ctx.streamRangeFunctions, ctx.streamFunctions);
                ctx.error = TA_CODEGEN_STREAM_MISMATCH;
            }
            /* Per-SITE, not per-total. The floor above counts functions and
             * the leg total is far above any threshold worth setting, so a whole
             * compare site going dead — the anchored open, say, or the fill —
             * stays green on both: the surviving sites carry the counts. Each
             * server reports which of its own sites fired; every one has to have
             * fired somewhere in the run. Corpus-wide rather than per function,
             * because a site can legitimately not run for a given function or
             * vector (C's anchored compare needs lb < Sidx < svN-1). */
            /* Fails CLOSED on a server that stops declaring its site count.
             * `range_sites_n` is the server's own claim about itself, so
             * skipping the ratchet when it is absent would let the leg be
             * disarmed by deleting one field — the exact shape this ratchet
             * exists to catch. A server that answered the leg at all must say
             * how many sites it has. */
            if( ctx.error == TA_TEST_PASS && ctx.streamRangeFunctions > 0 &&
                ctx.streamRangeSitesN <= 0 )
            {
                printf("STREAM RANGE PARTIAL: the %s server compared ranges for "
                       "%d function(s) but never declared how many compare sites "
                       "it has — the per-site ratchet cannot run\n",
                       lang->name, ctx.streamRangeFunctions);
                ctx.error = TA_CODEGEN_STREAM_MISMATCH;
            }
            if( ctx.error == TA_TEST_PASS && ctx.streamFunctions > 0 &&
                ctx.streamRangeSitesN > 0 &&
                ctx.streamRangeSites != (1 << ctx.streamRangeSitesN) - 1 )
            {
                printf("STREAM RANGE PARTIAL: only %d of %d range compare site(s) "
                       "ever fired (mask 0x%x) — a whole site class is dead\n",
                       codegen_popcount(ctx.streamRangeSites), ctx.streamRangeSitesN,
                       (unsigned)ctx.streamRangeSites);
                ctx.error = TA_CODEGEN_STREAM_MISMATCH;
            }
        }
        else if( probeErr == TA_TEST_PASS )
        {
            printf("  Stream verify: not supported by this server (stage 1 is C-only)\n");
        }
        else
        {
            ctx.error = probeErr;
        }
    }

    free(requestBuf);
    free(responseBuf);
    free(refFuncList);
    codegen_pipe_close(&cp);

    if( ctx.error != TA_TEST_PASS )
        return ctx.error;

    printf("\n  %s: %d passed, %d failed, %d skipped\n",
           lang->display, ctx.passed, ctx.failed, ctx.skipped);

    if( langIndex >= 0 && (unsigned int)langIndex < NUM_LANGUAGES )
        g_codegenCompared[langIndex] = ctx.passed;
    g_langRan[langIndex] = 1;

    /* Name the skips — an unnamed "6 skipped" reads as noise. The set is the
     * same for every language, so print it once (issue #137). All four variants
     * of every function are gated bitwise anyway by the VARIANT group. */
    if( langIndex == 0 )
    {
        int s;
        /* Name the value-exempt functions too. Their structural parity is
         * checked here, but the frozen reference is the wrong VALUE oracle for
         * them, so this leg compares no numbers -- and a gate that silently
         * compares nothing reads exactly like one that compared and agreed.
         * Say so, and say what does pin them instead. */
        printf("    values not ref-compared (reference is the wrong oracle): "
               "STOCHRSI (#107, pinned by test_stoch.c), "
               "CORREL (#242, pinned by test_correl.c + --xlang-hash)\n");
        if( ctx.nbSkipNames > 0 )
        {
            printf("    no frozen-reference baseline (post-cutover): ");
            for( s = 0; s < ctx.nbSkipNames; s++ )
                printf("%s%s", ctx.skipNames[s], (s + 1 < ctx.nbSkipNames) ? "," : "");
            printf("  [sweep skipped %d; all still bitwise-gated by VARIANT]\n",
                   ctx.sweepSkipped);
            /* The reference-independent legs still run for these, and must:
             * a post-cutover function is not an unverified one. */
            printf("    post-cutover range-stability verified: %d of %d"
                   " (%d value-compared, %d path-dependent: coherency only)\n",
                   ctx.postCutRangeChecked, ctx.nbSkipNames,
                   ctx.postCutRangeValueCompared,
                   ctx.postCutRangeChecked - ctx.postCutRangeValueCompared);
            if( ctx.postCutRangeChecked < ctx.nbSkipNames )
            {
                printf("CODEGEN FAILED: %d post-cutover function(s) skipped the "
                       "range-stability leg, which needs no frozen baseline\n",
                       ctx.nbSkipNames - ctx.postCutRangeChecked);
                return TA_CODEGEN_RANGE_VACUOUS;
            }
        }
        if( ctx.nbIntInputSkipNames > 0 )
        {
            printf("    integer inputs (no test data): ");
            for( s = 0; s < ctx.nbIntInputSkipNames; s++ )
                printf("%s%s", ctx.intInputSkipNames[s],
                       (s + 1 < ctx.nbIntInputSkipNames) ? "," : "");
            printf("\n");
        }
    }

    return TA_TEST_PASS;
}

/* ---- Main entry point ---- */

static int language_matches_filter(const char *filter, const char *name)
{
    char filterCopy[1024];
    char *token;
    if( filter == NULL ) return 1;
    strncpy(filterCopy, filter, sizeof(filterCopy) - 1);
    filterCopy[sizeof(filterCopy) - 1] = '\0';
    token = strtok(filterCopy, ",");
    while( token != NULL )
    {
        if( strcmp(name, token) == 0 ) return 1;
        token = strtok(NULL, ",");
    }
    return 0;
}

/* ---- Cross-language timing table (Task 12) ---- */

static void print_timing_table(const char *languageFilter)
{
    if( g_numTimingResults == 0 )
        return;

    /* Collect which language columns to show */
    int showLang[NUM_LANGUAGES];
    for( unsigned int li = 0; li < NUM_LANGUAGES; li++ )
        showLang[li] = language_matches_filter(languageFilter, ALL_LANGUAGES[li].name);

    printf("\n");
    printf("=============================================\n");
    printf("Codegen Results + Timing (avg ns/call)\n");
    printf("=============================================\n");

    int showGuarded = !g_hideGuarded;

    /* Header */
    printf("%-20s %9s", "Function", "C-ref");
    for( unsigned int li = 0; li < NUM_LANGUAGES; li++ )
    {
        if( showLang[li] )
        {
            if( showGuarded )
                printf(" %9s", ALL_LANGUAGES[li].display);
        }
    }
    printf("\n");

    /* Rows — C column uses ANSI color: red if slower than C-ref, green if faster */
    for( int ri = 0; ri < g_numTimingResults; ri++ )
    {
        FuncTimingResult *r = &g_timingResults[ri];
        printf("%-20s %9.0f", r->funcName, r->c_ref_ns);
        for( unsigned int li = 0; li < NUM_LANGUAGES; li++ )
        {
            if( !showLang[li] )
                continue;
            int st = r->langs[li].tested;
            if( st == 0 )
            {
                if( showGuarded ) printf(" %9s", "--");
            }
            else if( st == -1 )
            {
                if( showGuarded ) printf(" %9s", "FAIL");
            }
            else
            {
                /* Guarded column: color relative to C-ref */
                if( showGuarded )
                {
                    if( r->c_ref_ns > 0 )
                    {
                        if( r->langs[li].avg_ns > r->c_ref_ns )
                            printf(" \033[31m%9.0f\033[0m", r->langs[li].avg_ns);
                        else if( r->langs[li].avg_ns < r->c_ref_ns )
                            printf(" \033[32m%9.0f\033[0m", r->langs[li].avg_ns);
                        else
                            printf(" %9.0f", r->langs[li].avg_ns);
                    }
                    else
                        printf(" %9.0f", r->langs[li].avg_ns);
                }

            }
        }
        printf("\n");
    }
}

/* ---- JSONL rolling report (Task 13) ---- */

static void write_timing_report(const char *filepath)
{
    FILE *f = fopen(filepath, "a");
    if( !f ) return;

    /* Get git SHA */
    char gitSha[64] = "unknown";
    FILE *git = popen("git rev-parse --short HEAD 2>/dev/null", "r");
    if( git ) { if( fgets(gitSha, sizeof(gitSha), git) == NULL ) strcpy(gitSha, "unknown"); pclose(git); }
    char *nl = strchr(gitSha, '\n');
    if( nl ) *nl = '\0';

    /* Get timestamp */
    time_t now = time(NULL);
    char timestamp[64];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%dT%H:%M:%SZ", gmtime(&now));

    /* Write JSONL line */
    fprintf(f, "{\"timestamp\":\"%s\",\"git_sha\":\"%s\",\"results\":{",
            timestamp, gitSha);

    int first = 1;
    for( int ri = 0; ri < g_numTimingResults; ri++ )
    {
        FuncTimingResult *r = &g_timingResults[ri];
        if( !first ) fprintf(f, ",");
        first = 0;
        fprintf(f, "\"%s\":{\"c_ref_ns\":%.3f,\"langs\":{", r->funcName, r->c_ref_ns);
        int firstLang = 1;
        for( unsigned int li = 0; li < NUM_LANGUAGES; li++ )
        {
            int st = r->langs[li].tested;
            if( st == 0 ) continue;   /* skip not-tested */
            if( !firstLang ) fprintf(f, ",");
            firstLang = 0;
            fprintf(f, "\"%s\":{\"status\":\"%s\",\"avg_ns\":%.0f",
                    ALL_LANGUAGES[li].name,
                    (st == 1) ? "pass" : "fail",
                    r->langs[li].avg_ns);
            fprintf(f, "}");
        }
        fprintf(f, "}}");
    }

    fprintf(f, "}}\n");
    fclose(f);
}

/* ---- Markdown report writer ---- */

static void fmt_ns(char *buf, int buf_size, double ns)
{
    if( ns <= 0 )      snprintf(buf, buf_size, "<42");
    else if( ns < 100 ) snprintf(buf, buf_size, "%.0f", ns);
    else               snprintf(buf, buf_size, "%.0f", ns);
}

static void fmt_ratio(char *buf, int buf_size, double val, double ref)
{
    if( val <= 0 || ref <= 0 ) { snprintf(buf, buf_size, "\xe2\x80\x94"); return; }
    double ratio = val / ref;
    if( ratio > 1.1 )      snprintf(buf, buf_size, "%.1f\xc3\x97 slower", ratio);
    else if( ratio < 0.9 ) snprintf(buf, buf_size, "%.1f\xc3\x97 faster", 1.0/ratio);
    else                    snprintf(buf, buf_size, "\xe2\x89\x88 same");
}

static void write_markdown_report(const char *filepath, const char *languageFilter)
{
    if( g_numTimingResults == 0 ) return;

    FILE *f = fopen(filepath, "w");
    if( !f ) return;

    /* Collect which languages to include */
    int showLang[NUM_LANGUAGES];
    int numShown = 0;
    for( unsigned int li = 0; li < NUM_LANGUAGES; li++ ) {
        showLang[li] = language_matches_filter(languageFilter, ALL_LANGUAGES[li].name);
        if( showLang[li] ) numShown++;
    }

    /* Git SHA */
    char gitSha[64] = "unknown";
    FILE *git = popen("git rev-parse --short HEAD 2>/dev/null", "r");
    if( git ) { if( fgets(gitSha, sizeof(gitSha), git) == NULL ) strcpy(gitSha, "unknown"); pclose(git); }
    char *nl = strchr(gitSha, '\n');
    if( nl ) *nl = '\0';

    /* Timestamp */
    time_t now = time(NULL);
    char timestamp[64];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M", localtime(&now));

    /* Count pass/fail per language + averages */
    int total = g_numTimingResults;
    int langPass[NUM_LANGUAGES];
    double langSum[NUM_LANGUAGES];
    int langMeasured[NUM_LANGUAGES];
    memset(langPass, 0, sizeof(langPass));
    memset(langSum, 0, sizeof(langSum));
    memset(langMeasured, 0, sizeof(langMeasured));

    double cRefSum = 0; int cRefCount = 0;
    for( int ri = 0; ri < g_numTimingResults; ri++ ) {
        FuncTimingResult *r = &g_timingResults[ri];
        if( r->c_ref_ns > 0 ) { cRefSum += r->c_ref_ns; cRefCount++; }
        for( unsigned int li = 0; li < NUM_LANGUAGES; li++ ) {
            if( r->langs[li].tested == 1 ) langPass[li]++;
            if( r->langs[li].tested == 1 && r->langs[li].avg_ns > 0 ) {
                langSum[li] += r->langs[li].avg_ns;
                langMeasured[li]++;
            }
        }
    }
    double cRefAvg = cRefCount > 0 ? cRefSum / cRefCount : 0;

    /* Header */
    fprintf(f, "# ta_regtest Cross-Language Report\n\n");
    fprintf(f, "> **Date:** %s  \n", timestamp);
    fprintf(f, "> **Git:** `%s`  \n", gitSha);
    fprintf(f, "> **Indicators:** %d  \n", total);

    int allPass = 1;
    for( unsigned int li = 0; li < NUM_LANGUAGES; li++ ) {
        if( showLang[li] && langPass[li] < total ) allPass = 0;
    }
    fprintf(f, "> **Status:** %s\n\n",
            allPass ? "\xe2\x9c\x85 ALL PASSING" : "\xe2\x9d\x8c FAILURES DETECTED");

    /* Summary table */
    fprintf(f, "## Summary\n\n```\n");
    fprintf(f, "\xe2\x94\x8c\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80"
            "\xe2\x94\xac\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80"
            "\xe2\x94\xac\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80"
            "\xe2\x94\xac\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80"
            "\xe2\x94\xac\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80"
            "\xe2\x94\x90\n");

    fprintf(f, "\xe2\x94\x82 %-9s\xe2\x94\x82 %-5s\xe2\x94\x82 %-5s\xe2\x94\x82 %-11s\xe2\x94\x82 %-15s\xe2\x94\x82\n",
            "Language", "Pass", "Fail", "Avg (ns)", "vs C-ref");

    fprintf(f, "\xe2\x94\x9c\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80"
            "\xe2\x94\xbc\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80"
            "\xe2\x94\xbc\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80"
            "\xe2\x94\xbc\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80"
            "\xe2\x94\xbc\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80"
            "\xe2\x94\xa4\n");

    /* C-ref row */
    {
        char avg[32]; fmt_ns(avg, sizeof(avg), cRefAvg);
        fprintf(f, "\xe2\x94\x82 %-9s\xe2\x94\x82 %-5d\xe2\x94\x82 %-5d\xe2\x94\x82 %-11s\xe2\x94\x82 %-15s\xe2\x94\x82\n",
                "C-ref", total, 0, avg, "baseline");
    }

    /* Per-language rows */
    for( unsigned int li = 0; li < NUM_LANGUAGES; li++ ) {
        if( !showLang[li] ) continue;
        double avg = langMeasured[li] > 0 ? langSum[li] / langMeasured[li] : 0;
        char avgStr[32], vsStr[32];
        if( langMeasured[li] < total / 2 ) {
            fmt_ns(avgStr, sizeof(avgStr), avg);
            char tmp[40]; snprintf(tmp, sizeof(tmp), "~%s*", avgStr);
            avgStr[0] = '\0'; strncat(avgStr, tmp, sizeof(avgStr) - 1);
            snprintf(vsStr, sizeof(vsStr), "*%d/%d measured", langMeasured[li], total);
        } else {
            fmt_ns(avgStr, sizeof(avgStr), avg);
            fmt_ratio(vsStr, sizeof(vsStr), avg, cRefAvg);
        }
        int fail = total - langPass[li];
        fprintf(f, "\xe2\x94\x82 %-9s\xe2\x94\x82 %-5d\xe2\x94\x82 %-5d\xe2\x94\x82 %-11s\xe2\x94\x82 %-15s\xe2\x94\x82\n",
                ALL_LANGUAGES[li].display, langPass[li], fail, avgStr, vsStr);
    }

    fprintf(f, "\xe2\x94\x94\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80"
            "\xe2\x94\xb4\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80"
            "\xe2\x94\xb4\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80"
            "\xe2\x94\xb4\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80"
            "\xe2\x94\xb4\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80"
            "\xe2\x94\x98\n");
    fprintf(f, "```\n\n");

    /* Detailed per-function table */
    fprintf(f, "## Results (ns/call)\n\n");
    fprintf(f, "| Function |  C-ref |");
    for( unsigned int li = 0; li < NUM_LANGUAGES; li++ ) {
        if( showLang[li] ) {
            fprintf(f, " %s |", ALL_LANGUAGES[li].display);
        }
    }
    fprintf(f, "\n|----------|--------|");
    for( unsigned int li = 0; li < NUM_LANGUAGES; li++ ) {
        if( showLang[li] ) {
            fprintf(f, "--------|");
        }
    }
    fprintf(f, "\n");

    for( int ri = 0; ri < g_numTimingResults; ri++ ) {
        FuncTimingResult *r = &g_timingResults[ri];
        char cref[32]; fmt_ns(cref, sizeof(cref), r->c_ref_ns);
        fprintf(f, "| %-8s | %6s |", r->funcName, cref);
        for( unsigned int li = 0; li < NUM_LANGUAGES; li++ ) {
            if( !showLang[li] ) continue;
            if( r->langs[li].tested == -1 ) {
                fprintf(f, " FAIL   |");
            } else if( r->langs[li].tested == 1 ) {
                char t[32]; fmt_ns(t, sizeof(t), r->langs[li].avg_ns);
                fprintf(f, " %6s |", t);
            } else {
                fprintf(f, "     \xe2\x80\x94 |");
            }
        }
        fprintf(f, "\n");
    }

    fprintf(f, "\n*Generated by ta_regtest — %s*\n", timestamp);
    fclose(f);
}

/* Bit-exact differential fuzz of the current library vs frozen v0.6.4.
 * Seed-generated inputs (fuzz_data.h), outputs compared by hash.
 * Full spec: src/tools/ta_regtest/CLAUDE.md (--fuzz-064). */

static const char *const argv_064[] = {"./ta_064_serve", NULL};

#define FUZZ_MAXN     256   /* bars per config (<= MAX_NB_TEST_ELEMENT) */
#define FUZZ_MAX_OPT  16
#define FUZZ_MAX_CAND 24    /* candidate values per single param. Only the MAType
                             * list can approach it: 12 values today (#93 added
                             * DISABLED, #182 DEFAULT) => 11 non-default. Overflow here is
                             * counted into *overflow so it fails the run LOUDLY —
                             * without this the MAType sweep would truncate
                             * silently (it never reaches the FUZZ_MAX_VEC guard). */
#define FUZZ_MAX_VEC  80    /* parameter vectors per function. MACDEXT is widest:
                             * 3 period ranges (<= 6 candidates + 2 reject + 1
                             * sentinel each) + 3 MAType lists (M-1 values + 1
                             * sentinel each, #162) + the defaults vector <= 3*M+28
                             * in the MAType-list length M. M=12 today => 64 worst
                             * case, 63 actually built (one of optInSignalPeriod's
                             * boundary candidates lands on its own default and is
                             * dropped). 80 gives runway to M=17, and still matches
                             * STREAM_MAX_VEC.
                             * fuzz_build_vectors reports any overflow (this cap or
                             * the cand cap) and the caller fails the run loudly. */
#define FUZZ_MIN_PERIOD 2   /* period 1 is out of scope vs 0.6.4 (see CLAUDE.md) */
typedef char fuzz_maxn_fits_output_bufs[FUZZ_MAXN <= MAX_NB_TEST_ELEMENT ? 1 : -1];

static double fuzz_canon15(double x)
{
    char b[40];
    snprintf(b, sizeof(b), "%.15g", x);
    return strtod(b, NULL);
}

/* Static scratch (one function at a time; TA_ForEachFunc is serial). */
static double     g_fzBuf[6][FUZZ_MAXN];              /* O,H,L,C,V,OI          */
static TA_Real    g_fz064Real[MAX_OUTPUTS][MAX_NB_TEST_ELEMENT];
static TA_Integer g_fz064Int[MAX_OUTPUTS][MAX_NB_TEST_ELEMENT];

typedef struct {
    const char  *functionFilter;
    CodegenPipe *cp;
    char        *reqBuf;
    char        *respBuf;
    const char  *funcList;   /* 0.6.4's list_functions payload (subset gate) */
    long long    comparisons, matches, benign, failures;
    long long    skipped98;   /* TRIX startIdx>lookback cases (issue #98 fix) */
    long long    cciTol;      /* CCI near-zero cases tolerated vs 0.6.4 (issue #7 fix) */
    long long    fmaTol;      /* cases tolerated by the one-time FMA re-baselining gate (PR #96) */
    double       maxFmaRel;   /* largest FMA-tolerated relative divergence observed (evidence vs the 1e-9 contract) */
    long long    stochRsiSkipped; /* STOCHRSI cases skipped: intentionally diverges from 0.6.4 (issue #107) */
    long long    mfiSkipped;      /* MFI cases skipped: v0.6.4 categorically wrong there (issue #244) */
    long long    varianceSkipped; /* VAR/STDDEV/BBANDS cases skipped: cancellation-free variance re-baseline (issue #118) */
    long long    xySkipped;      /* CORREL/BETA cases skipped: same re-baseline over two series (issue #242) */
    int          reportedThisFunc;
    int          funcsWithFailures, funcsBenign, funcsSkipped;
    int          serverRestarts;
    ErrorNumber  error;
} FuzzContext;

/* Send ctx->reqBuf, read ctx->respBuf. Reopen the oracle once if it died;
 * an unrecoverable oracle fails the run (never a false green). */
static int fuzz_call(FuzzContext *ctx)
{
    if( codegen_pipe_call(ctx->cp, ctx->reqBuf, ctx->respBuf, JSON_BUF_SIZE) == TA_TEST_PASS )
        return 1;
    ctx->serverRestarts++;
    codegen_pipe_close(ctx->cp);
    if( codegen_pipe_open(ctx->cp, argv_064) != TA_TEST_PASS )
    {
        ctx->error = TA_CODEGEN_ALLOC_FAILED;
        return 0;
    }
    if( codegen_pipe_call(ctx->cp, ctx->reqBuf, ctx->respBuf, JSON_BUF_SIZE) == TA_TEST_PASS )
        return 1;
    ctx->error = TA_CODEGEN_PIPE_READ_FAILED;   /* died on reopen+retry — never a false green */
    return 0;
}

/* ---- Shared "in-process C <=> server, bit-for-bit" core (issue #115) ---------
 * codegen_output_hash / codegen_hash_compare / codegen_hash_report back BOTH the
 * --xlang-hash gate and server_verify (declared in test_codegen.h). Same
 * operation, different input source: a seed vs the hard-coded test arrays. */

/* Golden output hash in logical order — byte-identical to every server's
 * out_hash. Reuses the shared fuzz_hash_* FNV primitives (fuzz_data.h). */
unsigned long long codegen_output_hash(unsigned int nbOutput,
                                       const int *outIsInteger,
                                       const void *const *outBufs, int nb)
{
    unsigned long long h = fuzz_hash_init();
    if( nb > 0 )
        for( unsigned int o = 0; o < nbOutput && o < MAX_OUTPUTS; o++ )
            h = fuzz_hash_bytes(h, outBufs[o],
                                (unsigned long)nb *
                                (outIsInteger[o] ? sizeof(int) : sizeof(double)));
    return fuzz_hash_fin(h);
}

/* Hash the in-process outputs in logical output order (matches the server). */
static unsigned long long fuzz_hash_local(const CodegenRangeTestParam *p, int nb)
{
    const void *bufs[MAX_OUTPUTS];
    unsigned int o;
    for( o = 0; o < p->funcInfo->nbOutput && o < MAX_OUTPUTS; o++ )
        bufs[o] = p->outputIsInteger[o] ? (const void *)p->outIntBufs[o]
                                        : (const void *)p->outRealBufs[o];
    return codegen_output_hash(p->funcInfo->nbOutput, p->outputIsInteger, bufs, nb);
}

/* Build an abstract_call request that generates its inputs from (shape,seed,n).
 * hash mode unless fullOutput. */
static void fuzz_build_request(char *buf, const TA_FuncInfo *fi,
                               int s, int e, int shape, int seed, int n,
                               const double *optVals, int fullOutput)
{
    int pos = codegen_appendf(buf, JSON_BUF_SIZE, 0,
        "{\"method\":\"abstract_call\",\"params\":{\"funcName\":\"%s\","
        "\"startIdx\":%d,\"endIdx\":%d,"
        "\"gen_present\":1,\"gen_shape\":%d,\"gen_seed\":%d,\"gen_n\":%d",
        fi->name, s, e, shape, seed, n);
    if( fullOutput )
        pos = codegen_appendf(buf, JSON_BUF_SIZE, pos, ",\"full_output\":1");
    for( unsigned int i = 0; i < fi->nbOptInput; i++ )
    {
        const TA_OptInputParameterInfo *oi;
        TA_GetOptInputParameterInfo(fi->handle, i, &oi);
        if( oi->type == TA_OptInput_RealRange || oi->type == TA_OptInput_RealList )
            pos = codegen_appendf(buf, JSON_BUF_SIZE, pos, ",\"%s\":%.15g", oi->paramName, optVals[i]);
        else
            pos = codegen_appendf(buf, JSON_BUF_SIZE, pos, ",\"%s\":%d", oi->paramName, (int)optVals[i]);
    }
    if( fi->flags & TA_FUNC_FLG_UNST_PER )
        pos = codegen_appendf(buf, JSON_BUF_SIZE, pos, ",\"unstablePeriod\":0");
    codegen_appendf(buf, JSON_BUF_SIZE, pos, "}}");
}

static unsigned long long fuzz_parse_hash(const char *resp)
{
    int len;
    const char *h = json_find_field(resp, "out_hash", &len);
    if( !h ) return 0;
    if( *h == '"' ) h++;
    return strtoull(h, NULL, 16);
}

/* Parameter-contract vector classes (issue #148). */
#define FUZZ_VEC_NORMAL   0
#define FUZZ_VEC_REJECT   1  /* below-min / above-max: must be REJECTED          */
#define FUZZ_VEC_SENTINEL 2  /* TA_*_DEFAULT: must resolve to the declared default */

/* Below-min / above-max for one bounded optional parameter — the only candidates
 * this builder emits that a correct implementation must REJECT; every other one
 * is deliberately pulled inside the range, which is how #148 (the Rust backend
 * emitting no validation at all for `real` params) survived every gate in this
 * file. Every real bound is finite and checked, so both sides always have a
 * rejection to compare; an integer side is skipped only when the probe would
 * leave TA_INTEGER_MIN/MAX. Lists have no bound to exceed; their out-of-list arm
 * lives in the stream vector builder. */
static void fuzz_add_out_of_range(const TA_OptInputParameterInfo *oi,
                                  double cand[FUZZ_MAX_CAND],
                                  char candKind[FUZZ_MAX_CAND],
                                  int *nc, int *overflow)
{
    double oor[2];
    int    noor = 0;

    if( oi->type == TA_OptInput_IntegerRange )
    {
        const TA_IntegerRange *r = (const TA_IntegerRange *)oi->dataSet;
        if( !r ) return;
        if( (long long)r->min - 1 >= (long long)TA_INTEGER_MIN )
            oor[noor++] = (double)((long long)r->min - 1);
        if( (long long)r->max + 1 <= (long long)TA_INTEGER_MAX )
            oor[noor++] = (double)((long long)r->max + 1);
    }
    else if( oi->type == TA_OptInput_RealRange )
    {
        const TA_RealRange *r = (const TA_RealRange *)oi->dataSet;
        if( !r ) return;
        /* +/-1.0 is absorbed at sentinel magnitude, so fall back to a multiple of
         * the widest legal bound. Neither can collide with TA_REAL_DEFAULT. */
        oor[noor++] = (r->min - 1.0 < r->min) ? r->min - 1.0 : 2.0 * TA_REAL_MIN;
        oor[noor++] = (r->max + 1.0 > r->max) ? r->max + 1.0 : 2.0 * TA_REAL_MAX;
    }
    else
        return;

    for( int b = 0; b < noor; b++ )
    {
        /* Same loud-overflow contract as the in-range candidates: a dropped
         * candidate silently un-gates a parameter. */
        if( *nc >= FUZZ_MAX_CAND ) { (*overflow)++; continue; }
        candKind[*nc] = FUZZ_VEC_REJECT;
        cand[(*nc)++] = oor[b];
    }
}

/* The "use the default" sentinel for one optional parameter: t3(x, 5, -4e37) must
 * give exactly t3(x, 5, 0.7), as TA_T3 always has. Emitted for EVERY optional
 * domain, with no exemption — range params bounded or not (the five
 * [TA_REAL_MIN, TA_REAL_MAX] reals get no range check, so this is their only
 * contract) and choice lists alike. Excluding IntegerList here is what hid #162.
 * Java cannot take the choice-list vector at all; it is exempted per SERVER
 * instead — see codegen_lang_can_pass_enum_sentinel. */
static void fuzz_add_default_sentinel(const TA_OptInputParameterInfo *oi,
                                      double cand[FUZZ_MAX_CAND],
                                      char candKind[FUZZ_MAX_CAND],
                                      int *nc, int *overflow)
{
    double sentinel;

    if( oi->type == TA_OptInput_IntegerRange || oi->type == TA_OptInput_IntegerList )
        sentinel = (double)TA_INTEGER_DEFAULT;
    else
        sentinel = TA_REAL_DEFAULT;   /* RealRange + RealList (none shipped yet) */

    if( *nc >= FUZZ_MAX_CAND ) { (*overflow)++; return; }
    candKind[*nc] = FUZZ_VEC_SENTINEL;
    cand[(*nc)++] = sentinel;
}

/* Parameter vectors: defaults + one-param-varied boundary/list sweeps, plus the
 * two contract classes (see the !frozenOracle guard below). */
/* frozenOracle: 1 when the vectors feed a frozen oracle (--fuzz-064's
 * ta_064_serve) -- IntegerList values the freeze predates are then excluded
 * (see FROZEN_ORACLE_MATYPE_MAX). --xlang-hash is current-vs-current and
 * passes 0, so the new values stay bitwise-gated there.
 *
 * kind: one FUZZ_VEC_* class per returned vector, or NULL if not needed. */
static int fuzz_build_vectors(const TA_FuncInfo *fi,
                              double vec[FUZZ_MAX_VEC][FUZZ_MAX_OPT],
                              int *overflow,
                              int frozenOracle,
                              char kind[FUZZ_MAX_VEC])
{
    *overflow = 0;
    double def[FUZZ_MAX_OPT];
    unsigned int i;
    for( i = 0; i < fi->nbOptInput && i < FUZZ_MAX_OPT; i++ )
    {
        const TA_OptInputParameterInfo *oi;
        TA_GetOptInputParameterInfo(fi->handle, i, &oi);
        def[i] = (oi->type == TA_OptInput_RealRange || oi->type == TA_OptInput_RealList)
                 ? fuzz_canon15(oi->defaultValue) : (double)(int)oi->defaultValue;
    }

    int nvec = 0;
    for( i = 0; i < fi->nbOptInput && i < FUZZ_MAX_OPT; i++ ) vec[0][i] = def[i];
    if( kind ) kind[0] = FUZZ_VEC_NORMAL;
    nvec = 1;

    for( i = 0; i < fi->nbOptInput && i < FUZZ_MAX_OPT; i++ )
    {
        const TA_OptInputParameterInfo *oi;
        TA_GetOptInputParameterInfo(fi->handle, i, &oi);
        double cand[FUZZ_MAX_CAND]; int nc = 0, c;
        char candKind[FUZZ_MAX_CAND];
        memset(candKind, FUZZ_VEC_NORMAL, sizeof(candKind));

        if( oi->type == TA_OptInput_IntegerRange )
        {
            const TA_IntegerRange *r = (const TA_IntegerRange *)oi->dataSet;
            int def_i = (int)oi->defaultValue;
            int lo = r ? (int)r->min : FUZZ_MIN_PERIOD;
            if( lo < FUZZ_MIN_PERIOD ) lo = FUZZ_MIN_PERIOD;  /* period 1 tested by non-0.6.4 comparisons */
            /* min / min+1 / min+7 boundary, plus the tight neighbourhood around
             * the default (default-1, default+1) and one a bit further out
             * (default+3). vec[0] already carries the default itself, so the
             * full {default-1, default, default+1} triple is covered. */
            int base[6]; base[0]=lo; base[1]=lo+1; base[2]=lo+7;
            base[3]=def_i-1; base[4]=def_i+1; base[5]=def_i+3;
            for( int b = 0; b < 6; b++ )
            {
                int v = base[b];
                if( v < lo ) v = lo;
                if( r && v > (int)r->max ) v = (int)r->max;
                if( v == def_i ) continue;
                int dup = 0; for( c = 0; c < nc; c++ ) if( (int)cand[c] == v ) dup = 1;
                if( !dup && nc < FUZZ_MAX_CAND ) cand[nc++] = (double)v;  /* <= 6, cannot overflow */
            }
        }
        else if( oi->type == TA_OptInput_IntegerList )
        {
            const TA_IntegerList *l = (const TA_IntegerList *)oi->dataSet;
            for( unsigned int e2 = 0; l && e2 < l->nbElement; e2++ )
            {
                if( l->data[e2].value == (int)oi->defaultValue ) continue;
                if( frozenOracle && frozen_excludes_enum_value( oi, l->data[e2].value ) )
                    continue;
                /* A MAType list longer than cand[] would otherwise truncate
                 * SILENTLY here (this loop never reaches the FUZZ_MAX_VEC guard
                 * below), quietly dropping arms from the sweep. Count it so the
                 * run fails loudly instead. #93 and #182 took the list to 12 (11
                 * non-default); FUZZ_MAX_CAND keeps headroom above that. */
                if( nc >= FUZZ_MAX_CAND ) { (*overflow)++; continue; }
                cand[nc++] = (double)l->data[e2].value;
            }
        }
        else if( oi->type == TA_OptInput_RealRange )
        {
            const TA_RealRange *r = (const TA_RealRange *)oi->dataSet;
            double s2[2]; s2[0] = r ? r->suggested_start : 0; s2[1] = r ? r->suggested_end : 0;
            for( int b = 0; b < 2; b++ )
            {
                double v = fuzz_canon15(s2[b]);
                if( fabs(v) > 1e30 ) continue;
                if( r && (v < r->min || v > r->max) ) continue;
                if( v == def[i] ) continue;
                if( nc < FUZZ_MAX_CAND ) cand[nc++] = v;  /* <= 2, cannot overflow */
            }
        }

        /* The two contract legs (current-vs-current only). */
        if( !frozenOracle )
        {
            fuzz_add_out_of_range(oi, cand, candKind, &nc, overflow);
            fuzz_add_default_sentinel(oi, cand, candKind, &nc, overflow);
        }

        for( c = 0; c < nc; c++ )
        {
            /* Silent truncation would quietly stop comparing parameter values
             * vs 0.6.4 — count drops so the caller fails the run loudly. */
            if( nvec >= FUZZ_MAX_VEC ) { (*overflow)++; continue; }
            for( unsigned int j = 0; j < fi->nbOptInput && j < FUZZ_MAX_OPT; j++ )
                vec[nvec][j] = def[j];
            vec[nvec][i] = cand[c];
            if( kind ) kind[nvec] = candKind[c];
            nvec++;
        }
    }
    return nvec;
}

/* Tolerance for CCI vs 0.6.4 (issue #7 / SF bug 107 only). CCI's algorithm is
 * byte-identical to 0.6.4 except the final guard: where prices over the period
 * are (near-)identical the fix now returns exactly 0.0, whereas 0.6.4 divided
 * sub-epsilon residue into a near-zero value (observed ~5e-14). This tolerance
 * absorbs exactly that — orders of magnitude below any real CCI value, so a
 * genuine divergence still fails. Applied ONLY to CCI. */
/* latest -> 0.6.4 tolerance manifest: the authorized, bounded numeric
 * divergences from the frozen reference. Everything not listed here must be
 * bit-exact (hash-equal). Each entry cites the issue that authorized it.
 *
 *   TOL_ABS    : |current - v0.6.4| <= tol                (a fixed absolute bound)
 *   TOL_REL_IN : |current - v0.6.4| <= min(tol * inScale, cap), where inScale is
 *                the max |primary input| over the case. Used for algebraic
 *                re-orderings whose rounding is proportional to the DATA
 *                magnitude, which a fixed absolute bound cannot express because
 *                the fuzz shapes span 1e-7 .. 1e9 (FUZZ_EXTREME) and cross slope
 *                zero (the running-sum LINEARREG family, #103). Certified max
 *                ratio 2.9e-11 (ANGLE) -> tol 1e-9 keeps ~35x margin while staying
 *                tight on real prices.
 *                `cap` (0 = none) bounds the data-scaled tolerance for outputs
 *                that DON'T grow with input magnitude: ANGLE is atan-compressed
 *                degrees in [-90,90], so on FUZZ_EXTREME (inScale ~2e9) an
 *                uncapped bound would balloon to ~2 deg; the cap holds it near
 *                ANGLE's true worst-case drift (measured 0.065 deg) so a real
 *                future ANGLE regression can't hide, while realistic-scale cases
 *                (bound ~1e-7 deg) are unaffected (cap never binds there).
 *
 *   TOL_NAN_TO : 0.6.4 emitted NaN from an unguarded x/0 in a *successful* call;
 *                the fix substitutes a defined neutral value. This is NOT a
 *                numeric bound — it is the categorical divergence NaN(0.6.4) ->
 *                finite. Tolerated ONLY when 0.6.4 is NaN AND current equals the
 *                authorized value carried in `tol` (e.g. IMI #112: 50.0). Any
 *                other element diff for such a function is a real failure, so a
 *                regression that returns a *different* value where 0.6.4 was NaN
 *                (or diverges anywhere 0.6.4 was finite) still fails. Kept
 *                maximally tight by requiring the exact neutral value, not merely
 *                "any finite". */
/* ---- One-time FMA re-baselining transition gate (PR #96) -------------------
 * TA-Lib adopted an explicit-FMA numerical contract: each function is faithful
 * to its algorithm within 1e-9 relative, NOT bit-for-bit
 * (docs/studies/fma-readiness-audit.md). The current library now fuses `a*b + c`
 * into `fma()` wherever the shared codegen detector fires; the frozen v0.6.4
 * oracle does not. So the two differ by <=~1.7e-10 relative on the fused
 * functions — authorized, below the 1e-9 contract, but no longer hash-exact.
 *
 * While this is 1, any per-element diff NOT covered by an explicit FUZZ_064_TOL
 * entry is tolerated when it is within the contract itself:
 *     |current - v0.6.4| <= 1e-9 * max(|current|, |v0.6.4|, inScale)
 * Output-relative (so it scales correctly for volume-magnitude outputs like
 * ADOSC and bounded oscillators alike), floored at the input scale so the few
 * functions that difference two large near-equal quantities (DEMA/MACDFIX/
 * MACDEXT EMA cascades, HT_PHASOR) are judged against the ULP-of-operands drift
 * near a zero-crossing rather than the ill-posed near-zero output. These cases
 * are counted and reported on their OWN summary line, with the largest relative
 * divergence seen, so a genuine >1e-9 regression still fails loudly and is never
 * folded into the authorized-manifest bucket. Integer outputs are NOT given any
 * tolerance here — a candlestick/index flip must still fail.
 *
 * RE-FREEZE (once a FMA-enabled release is tagged): point
 * scripts/build_064_serve.py's REF_TAG at that release, rebuild the oracle, then
 * set FMA_TRANSITION_TOLERANCE to 0 — current == new reference bitwise, so every
 * function returns to strict hash-exact comparison against the FMA baseline. */
#define FMA_TRANSITION_TOLERANCE 1
#define FMA_TRANSITION_REL 1e-9

/* Functions whose output differences two large near-equal quantities (EMA
 * cascades near a zero-crossing, near-zero phasor components). Their FMA drift
 * is bounded by the ULP of the ~price-scale operands, not the tiny output, so
 * the transition tolerance for them is floored at the input scale rather than
 * the ill-posed output-relative bound. Kept to the SHORT list that empirically
 * needs it (all four exceed output-relative 1e-9 near a crossing without it);
 * every other function — bounded oscillators included — stays tight
 * output-relative so an extreme-scale regression cannot hide behind inScale. */
static int fma_needs_input_scale(const char *name)
{
    /* The EMA-cascade differences (output = a difference of large ~price-scale
     * EMAs, tiny near a crossing) + HT_PHASOR (near-zero I/Q components). NOT the
     * bounded oscillators (HT_SINE/HT_DCPHASE/HT_DCPERIOD/STOCH/...), which are
     * well-conditioned at their own scale and stay tight output-relative. */
    return strcmp(name, "DEMA") == 0 || strcmp(name, "TEMA") == 0
        || strcmp(name, "TRIX") == 0 || strcmp(name, "MACD") == 0
        || strcmp(name, "MACDFIX") == 0 || strcmp(name, "MACDEXT") == 0
        || strcmp(name, "APO") == 0 || strcmp(name, "PPO") == 0
        || strcmp(name, "HT_PHASOR") == 0;
}

enum { TOL_ABS = 0, TOL_REL_IN = 1, TOL_NAN_TO = 2, TOL_REL_OUT = 3 };
typedef struct { const char *name; int mode; double tol; double cap; } TA_Fuzz064Tol;
static const TA_Fuzz064Tol FUZZ_064_TOL[] = {
    { "CCI",                 TOL_ABS,    1e-9, 0.0 },  /* #7   near-zero identical-price fix */
    /* #118 cancellation-free variance. Bounded relative to the OUTPUT, not the
     * input: VAR's output is a squared quantity, so an inScale-relative bound is
     * the wrong dimension for it and would be meaningless at FUZZ_EXTREME
     * magnitudes. Only well-conditioned windows reach here at all — see
     * fuzz_variance_condition(). */
    { "VAR",                 TOL_REL_OUT, 1e-9, 0.0 }, /* #118 */
    { "STDDEV",              TOL_REL_OUT, 1e-9, 0.0 }, /* #118 */
    { "BBANDS",              TOL_REL_OUT, 1e-9, 0.0 }, /* #118 */
    /* #242 the same treatment for the two-series forms. Output-relative: CORREL
     * is a coefficient in [-1,1] and BETA a ratio of return scales, so neither
     * is commensurate with the input magnitude. Only well-conditioned windows
     * reach here -- see fuzz_correl_condition() / fuzz_beta_condition(). */
    { "CORREL",              TOL_REL_OUT, 4e-9, 0.0 },  /* #242  measured 1.12e-09            */
    { "BETA",                TOL_REL_OUT, 1e-9, 0.0 },  /* #242  measured 3.08e-10            */
    { "LINEARREG",           TOL_REL_IN, 1e-9, 0.0 },  /* #103 O(1) sliding-sum recurrence   */
    { "LINEARREG_SLOPE",     TOL_REL_IN, 1e-9, 0.0 },  /* #103                               */
    { "LINEARREG_INTERCEPT", TOL_REL_IN, 1e-9, 0.0 },  /* #103                               */
    { "LINEARREG_ANGLE",     TOL_REL_IN, 1e-9, 0.5 },  /* #103 bounded degrees -> capped 0.5 */
    { "TSF",                 TOL_REL_IN, 1e-9, 0.0 },  /* #103                               */
    { "IMI",                 TOL_NAN_TO, 50.0, 0.0 },  /* #112 all-flat window 0/0 -> NaN, now 50.0 */
};

/* Largest divergence each manifest entry actually absorbed, in the units of its
 * own bound. A gate that tolerates should say HOW MUCH it tolerated: without it
 * an entry can be an order of magnitude looser than the divergence it authorizes
 * and nothing says so -- and the next person to set one has no measurement to
 * size it from. The FMA bucket already reports its own ("max observed 4.13e-11");
 * this is the same for the named entries. Indexed by FUZZ_064_TOL slot. */
static double g_fuzz064TolMax[sizeof(FUZZ_064_TOL)/sizeof(FUZZ_064_TOL[0])];

/* Record `achieved` (in bound units) against the entry `e` returned by lookup. */
static void fuzz_064_tol_record(const void *e, double achieved)
{
    long idx;
    if( !e ) return;
    idx = (const TA_Fuzz064Tol *)e - FUZZ_064_TOL;
    if( idx < 0 || (unsigned long)idx >= sizeof(FUZZ_064_TOL)/sizeof(FUZZ_064_TOL[0]) ) return;
    if( achieved > g_fuzz064TolMax[idx] ) g_fuzz064TolMax[idx] = achieved;
}

/* Look up a function's authorized tolerance; returns NULL if it must be exact. */
static const void *fuzz_064_tol_lookup(const char *name, int *mode, double *tol, double *cap)
{
    for( unsigned int i = 0; i < sizeof(FUZZ_064_TOL)/sizeof(FUZZ_064_TOL[0]); i++ )
        if( strcmp(name, FUZZ_064_TOL[i].name) == 0 )
        { *mode = FUZZ_064_TOL[i].mode; *tol = FUZZ_064_TOL[i].tol; *cap = FUZZ_064_TOL[i].cap;
          return &FUZZ_064_TOL[i]; }
    return NULL;
}

/* Conditioning of v0.6.4's variance form over the windows a case evaluates.
 *
 * 0.6.4 computes variance as E[x^2] - mean^2, which cancels catastrophically
 * when the mean dominates the spread (SourceForge bug 90); 0.8.1 uses the
 * shifted-data form and does not. The severity is the condition number
 * kappa = mean^2 / variance: 0.6.4 loses roughly log10(kappa) significant
 * digits, i.e. its relative error is about DBL_EPSILON * kappa.
 *
 * Crucially the severity is NOT a property of the window alone. Both versions
 * keep RUNNING sums over the sliding window, so the accumulators carry rounding
 * from every value they ever absorbed, not just the ones currently inside it.
 * On FUZZ_EXTREME (values alternating ~1e9 and ~1e-7) a window of tiny values
 * looks perfectly conditioned on its own -- measured kappa 26.7 -- while 0.6.4
 * reports 256 for a true variance of 7.6e-16, because its accumulator absorbed
 * the ~1e9 values earlier and carries an absolute error of eps*(1e9)^2 ~ 220.
 *
 * So the measure is the largest magnitude the accumulators absorb over the case,
 * squared, against the smallest window variance the case reports:
 *
 *     kappa = max|x|^2 / min(variance)
 *
 * The naive bound on 0.6.4's relative error is DBL_EPSILON * kappa, but the
 * sliding accumulator also rounds once per step, so long cases drift further:
 * at kappa just under 1e6 a 240-bar VAR case was measured at 1.21e-9 relative,
 * about 5x the naive estimate. The threshold is therefore set an order of
 * magnitude tighter than the model, which keeps observed divergence inside the
 * manifest's 1e-9 bound with margin. Returns HUGE_VAL for a flat window, where
 * 0.6.4 can go negative and produce NaN through the sqrt. */
#define FUZZ_VAR_MAX_KAPPA 1.0e5
static double fuzz_variance_condition( const double *x, int n, int period, int s, int e )
{
    double maxAbs = 0.0, minVar = HUGE_VAL;
    int t, first, j;

    if( period < 2 ) return 0.0;      /* no cancellation possible */
    first = (s > period - 1) ? s : period - 1;
    if( first > e || first >= n ) return 0.0;

    /* Largest magnitude the running accumulators absorb over this case, from the
     * first bar they read (window start of the first output) through the last. */
    for( j = first - period + 1; j <= e && j < n; j++ )
    {
        double m = fabs(x[j]);
        if( m > maxAbs ) maxAbs = m;
    }

    /* Smallest window variance the case reports. Two-pass on purpose: the test
     * must not reuse the algorithm under test to decide whether to trust the
     * oracle. */
    for( t = first; t <= e && t < n; t++ )
    {
        double sum = 0.0, mean, var = 0.0;
        for( j = t - period + 1; j <= t; j++ ) sum += x[j];
        mean = sum / (double)period;
        for( j = t - period + 1; j <= t; j++ ) { double dv = x[j] - mean; var += dv * dv; }
        var /= (double)period;
        if( !(var > 0.0) ) return HUGE_VAL;   /* flat window: 0.6.4 can go negative */
        if( var < minVar ) minVar = var;
    }
    if( !(minVar > 0.0) || !(maxAbs > 0.0) ) return HUGE_VAL;

    return (maxAbs * maxAbs) / minVar;
}

/* Conditioning of v0.6.4's one-pass CORREL/BETA form over the windows a case
 * evaluates (issue #242). Same measure, same reasoning and the same caveat as
 * fuzz_variance_condition() above: v0.6.4 extracts each sum of squares as
 * S2 - (S*S)/n, a difference of two ~n*mean^2 quantities, and its running
 * accumulators carry rounding from every value they ever absorbed -- so the
 * severity is a property of the CASE, not of any one window. Hence
 *
 *     kappa = max|v|^2 / min(window variance)
 *
 * over the values the accumulators actually read.
 *
 * HUGE_VAL (always skip) is returned for the windows where v0.6.4 does not
 * merely lose digits but has no answer at all: a flat window, where its
 * subtraction lands either side of zero, and any window its ABSOLUTE epsilon
 * guard zeroes. Those are categorical divergences -- v0.6.4 returns exactly 0,
 * or a correlation outside [-1,1] -- and no numeric tolerance can express them.
 * They are precisely what #242 fixed, so v0.6.4 is not an oracle there.
 *
 * Two-pass on purpose: the test must not reuse either implementation to decide
 * whether to trust the oracle. */
#define FUZZ_XY_MAX_KAPPA 1.0e5

/* Shared core: kappa of one series over the windows [first..e], plus the
 * smallest sum-of-squared-deviations any window reaches (the quantity v0.6.4's
 * epsilon guard is applied to). Returns 0 when there is nothing to judge. */
static double fuzz_series_condition(const double *v, int n, int period,
                                    int first, int e, double *outMinSS)
{
    double maxAbs = 0.0, minVar = HUGE_VAL, minSS = HUGE_VAL;
    int t, j;

    if( outMinSS ) *outMinSS = 0.0;
    if( period < 2 ) return 0.0;
    if( first > e || first >= n ) return 0.0;

    for( j = first - period + 1; j <= e && j < n; j++ )
    {
        double m = fabs(v[j]);
        if( m > maxAbs ) maxAbs = m;
    }

    for( t = first; t <= e && t < n; t++ )
    {
        double sum = 0.0, mean, ss = 0.0;
        for( j = t - period + 1; j <= t; j++ ) sum += v[j];
        mean = sum / (double)period;
        for( j = t - period + 1; j <= t; j++ ) { double d = v[j] - mean; ss += d * d; }
        if( !(ss > 0.0) ) { if( outMinSS ) *outMinSS = 0.0; return HUGE_VAL; }
        if( ss < minSS ) minSS = ss;
        if( ss / (double)period < minVar ) minVar = ss / (double)period;
    }
    if( !(minVar > 0.0) || !(maxAbs > 0.0) ) return HUGE_VAL;
    if( outMinSS ) *outMinSS = minSS;
    return (maxAbs * maxAbs) / minVar;
}

/* CORREL: v0.6.4 guards the PRODUCT of the two sums of squares against a fixed
 * TA_EPSILON, so the pair is what decides whether it returns a number at all. */
/* v0.6.4 is not an oracle for the MFI cases it gets categorically wrong
 * (issue #244). Unlike the variance and CORREL/BETA carve-outs, this is not a
 * question of lost digits, so there is no kappa to threshold: v0.6.4 either
 * reports the index or it reports something that is not one.
 *
 *   1. Its `sum < 1.0` guard fires. Money flow is a price times a volume, so
 *      that literal lives in whatever unit the instrument happens to be quoted
 *      in; where it fires, v0.6.4 emits 0 for an index that is well defined.
 *   2. The window is empty -- no bar moved, or none carried volume -- so the
 *      true sums are 0/0. v0.6.4's running sums then hold nothing but the
 *      rounding residue they accumulated, of arbitrary sign, and it divides
 *      that by itself.
 *   3. Some window is one-sided: every bar that moved went the same way, so
 *      the true sum on the other side is exactly 0 and again what v0.6.4
 *      divides by is residue. This is what put its output above 100.
 *
 * Everything else IS compared, and at ZERO tolerance -- no manifest entry: over
 * the fuzz corpus all 3222 surviving case-slots are bit-identical to v0.6.4,
 * because neither the reseed nor the range clamp can fire on a case that got
 * past this predicate. Two-pass on purpose, like fuzz_variance_condition()
 * above: the test must not reuse the algorithm under test to decide whether to
 * trust the oracle. */
static int fuzz_mfi_064_blind( const double *h, const double *l,
                               const double *c, const double *v,
                               int n, int period, int s, int e )
{
    int t, j, first;

    if( period < 1 ) return 0;
    first = (s > period) ? s : period;
    if( first > e || first >= n ) return 0;

    for( t = first; t <= e && t < n; t++ )
    {
        double pos = 0.0, neg = 0.0, total;
        for( j = t - period + 1; j <= t; j++ )
        {
            double tp  = (h[j]   + l[j]   + c[j])   / 3.0;
            double tpp = (h[j-1] + l[j-1] + c[j-1]) / 3.0;
            if     ( tp > tpp ) pos += tp * v[j];
            else if( tp < tpp ) neg += tp * v[j];
        }
        total = pos + neg;
        if( !(total > 0.0) )                return 1;   /* (2) empty window   */
        if( total < 1.0 )                   return 1;   /* (1) v0.6.4's guard */
        if( !(pos > 0.0) || !(neg > 0.0) )  return 1;   /* (3) one-sided      */
    }
    return 0;
}

static double fuzz_correl_condition(const double *x, const double *y,
                                    int n, int period, int s, int e)
{
    double kx, ky, ssx = 0.0, ssy = 0.0;
    int first = (s > period - 1) ? s : period - 1;

    kx = fuzz_series_condition(x, n, period, first, e, &ssx);
    if( kx == HUGE_VAL ) return HUGE_VAL;
    ky = fuzz_series_condition(y, n, period, first, e, &ssy);
    if( ky == HUGE_VAL ) return HUGE_VAL;
    if( kx == 0.0 && ky == 0.0 ) return 0.0;
    /* v0.6.4: if( !TA_IS_ZERO_OR_NEG(ssX*ssY) ) ... else 0.0 */
    if( ssx * ssy < 1e-14 ) return HUGE_VAL;
    return (kx > ky) ? kx : ky;
}

/* BETA: same, over the RETURNS (its regressor), with BETA's own zero-price
 * guard, and against its own absolute guard on n*S_xx - S_x*S_x. Both series
 * matter: the denominator cancels on x, the numerator on x and y alike. */
static double fuzz_beta_condition(const double *p0, const double *p1,
                                  int n, int period, int s, int e)
{
    static double rx[MAX_NB_TEST_ELEMENT], ry[MAX_NB_TEST_ELEMENT];
    double kx, ky, ssx = 0.0, ssy = 0.0;
    int first, j;

    if( n > MAX_NB_TEST_ELEMENT || n < 2 ) return HUGE_VAL;
    /* BETA's lookback is optInTimePeriod: the first output at bar t reads the
     * `period` returns ending at t, and a return needs its predecessor. */
    rx[0] = ry[0] = 0.0;
    for( j = 1; j < n; j++ )
    {
        rx[j] = ( p0[j-1] > 1e-14 || p0[j-1] < -1e-14 )
                ? ( p0[j] - p0[j-1] ) / p0[j-1] : 0.0;
        ry[j] = ( p1[j-1] > 1e-14 || p1[j-1] < -1e-14 )
                ? ( p1[j] - p1[j-1] ) / p1[j-1] : 0.0;
    }
    first = (s > period) ? s : period;
    if( first > e || first >= n ) return 0.0;

    kx = fuzz_series_condition(rx, n, period, first, e, &ssx);
    if( kx == HUGE_VAL ) return HUGE_VAL;
    ky = fuzz_series_condition(ry, n, period, first, e, &ssy);
    if( ky == HUGE_VAL ) return HUGE_VAL;
    if( kx == 0.0 && ky == 0.0 ) return 0.0;
    /* v0.6.4: if( !TA_IS_ZERO(n*S_xx - S_x*S_x) ) ... else 0.0, and that
     * quantity is exactly period * ssx. */
    if( (double)period * ssx < 1e-14 ) return HUGE_VAL;
    if( ky > kx ) kx = ky;

    /* The NUMERATOR cancels on its own axis, and the denominator measure above
     * is blind to it: a window where the two return series are uncorrelated has
     * a perfectly well-conditioned S_xx and a slope that is pure residue --
     * 1e-16 against a natural scale of order 1, with the two versions disagreeing
     * on its SIGN. Judging that as a relative divergence is meaningless.
     *
     * The measure is the Cauchy-Schwarz ceiling over what survives, which is
     * exactly 1/|correlation| on the window: 1 when the returns move together,
     * unbounded as they decouple. So this reads "skip where the two series are
     * essentially uncorrelated", and at the shared 1e5 threshold that is
     * |r| < 1e-5. CORREL needs no such term -- its OUTPUT is r, so a window
     * this measure would reject is one its own epsilon-guard check already has.
     */
    for( j = first; j <= e && j < n; j++ )
    {
        double mx = 0.0, my = 0.0, sxx = 0.0, syy = 0.0, sxy = 0.0, ceil_, kn;
        int t;
        for( t = j - period + 1; t <= j; t++ ) { mx += rx[t]; my += ry[t]; }
        mx /= (double)period; my /= (double)period;
        for( t = j - period + 1; t <= j; t++ )
        {
            double dx = rx[t] - mx, dy = ry[t] - my;
            sxx += dx * dx; syy += dy * dy; sxy += dx * dy;
        }
        if( sxx <= 0.0 || syy <= 0.0 ) return HUGE_VAL;
        ceil_ = sqrt( sxx * syy );
        if( !(fabs(sxy) > 0.0) ) return HUGE_VAL;
        kn = ceil_ / fabs(sxy);
        if( kn > kx ) kx = kn;
    }
    return kx;
}

/* Returns 0 if a REAL divergence, 1 if benign (+0.0 vs -0.0), 2 if tolerated
 * within the latest->0.6.4 manifest bound. Prints detail, capped per func.
 * inScale = max |primary input| over the case (for TOL_REL_IN entries). */
static int fuzz_classify_and_report(FuzzContext *ctx, const TA_FuncInfo *fi,
                                    CodegenRangeTestParam *p, int shape, int seed, int n,
                                    int s, int e, const double *optVals, double inScale,
                                    int curRc, int curBeg, int curNb,
                                    int refRc, int refBeg, int refNb)
{
    int report = (ctx->reportedThisFunc < 3);
    if( curRc != refRc || curBeg != refBeg || curNb != refNb )
    {
        if( report )
        {
            ctx->reportedThisFunc++;
            printf("  MISMATCH TA_%s shape=%d seed=%d n=%d range=[%d,%d]: "
                   "rc %d/%d begIdx %d/%d nbElem %d/%d (current/v0.6.4)\n",
                   fi->name, shape, seed, n, s, e, curRc, refRc, curBeg, refBeg, curNb, refNb);
        }
        return 0;
    }

    /* Values differ: re-issue with full_output to inspect elements. */
    fuzz_build_request(ctx->reqBuf, fi, s, e, shape, seed, n, optVals, 1);
    if( !fuzz_call(ctx) )
        return 0;   /* treat as real; the pipe failure is also counted */

    int realDiff = 0, benignDiff = 0, tolDiff = 0, fmaDiff = 0;
    int tolMode = 0; double tolVal = 0.0, tolCap = 0.0;
    const void *tolEntry = fuzz_064_tol_lookup(fi->name, &tolMode, &tolVal, &tolCap);
    /* TOL_REL_IN bound is data-scaled (optionally capped); TOL_ABS is fixed. */
    double tolBound = 0.0;
    if( tolEntry )
    {
        if( tolMode == TOL_REL_IN )
        {
            tolBound = tolVal * inScale;
            if( tolCap > 0.0 && tolBound > tolCap ) tolBound = tolCap;
        }
        else tolBound = tolVal;
    }
    int firstO = -1, firstJ = -1;
    for( unsigned int o = 0; o < fi->nbOutput && o < MAX_OUTPUTS; o++ )
    {
        char field[32];
        if( p->outputIsInteger[o] )
        {
            snprintf(field, sizeof(field), o == 0 ? "outInteger" : "outInteger%u", o);
            json_get_int_array(ctx->respBuf, field, g_fz064Int[o], MAX_NB_TEST_ELEMENT);
            for( int j = 0; j < curNb; j++ )
                if( p->outIntBufs[o][j] != g_fz064Int[o][j] )
                { realDiff = 1; if( firstO < 0 ) { firstO = (int)o; firstJ = j; } }
        }
        else
        {
            snprintf(field, sizeof(field), o == 0 ? "outReal" : "outReal%u", o);
            json_get_double_array(ctx->respBuf, field, g_fz064Real[o], MAX_NB_TEST_ELEMENT);
            for( int j = 0; j < curNb; j++ )
            {
                double a = p->outRealBufs[o][j], b = g_fz064Real[o][j];
                if( memcmp(&a, &b, sizeof(double)) == 0 ) continue;
                /* NaN-to-neutral manifest case (#112): 0.6.4's successful call
                 * emitted NaN (an unguarded x/0); the fix substitutes a defined
                 * neutral value. Tolerate ONLY when 0.6.4 is NaN AND current is
                 * exactly the authorized value; any other diff is real. (b != b
                 * is true only for NaN — catches -nan too.) */
                if( tolEntry && tolMode == TOL_NAN_TO )
                {
                    if( (b != b) && a == tolVal ) tolDiff = 1;
                    else { realDiff = 1; if( firstO < 0 ) { firstO = (int)o; firstJ = j; } }
                    continue;
                }
                double d = a - b; if( d < 0 ) d = -d;
                /* TOL_REL_OUT is output-relative, so its bound is per element and
                 * cannot be precomputed like the others. */
                double outBound = tolBound;
                if( tolEntry && tolMode == TOL_REL_OUT )
                {
                    double m = fabs(a) > fabs(b) ? fabs(a) : fabs(b);
                    outBound = tolVal * m;
                }
                if( a == b ) benignDiff = 1;        /* numerically equal => signed zero */
                else if( tolEntry && d <= outBound )
                {
                    tolDiff = 1;                    /* within manifest bound */
                    if( tolMode == TOL_REL_OUT )
                    {
                        double m = fabs(a) > fabs(b) ? fabs(a) : fabs(b);
                        if( m > 0.0 ) fuzz_064_tol_record(tolEntry, d / m);
                    }
                    else if( tolMode == TOL_REL_IN )
                    {
                        if( inScale > 0.0 ) fuzz_064_tol_record(tolEntry, d / inScale);
                    }
                    else fuzz_064_tol_record(tolEntry, d);
                }
#if FMA_TRANSITION_TOLERANCE
                /* One-time FMA re-baseline: within the 1e-9 relative contract,
                 * output-relative (`1e-9 × max(|current|, |v0.6.4|)`). The
                 * input-scale floor is applied ONLY to the functions that
                 * difference two large near-equal quantities (see
                 * fma_needs_input_scale): near the difference's zero-crossing the
                 * FMA drift is a ULP of the ~price-scale operands yet unbounded
                 * relative to the near-zero output, so output-relative is
                 * ill-posed there. Everyone else stays tight output-relative — a
                 * blanket inScale floor would over-loosen bounded oscillators at
                 * extreme input magnitude (HT_SINE ∈ [-1,1] would get a ~2.0
                 * bound on FUZZ_EXTREME's close≈2e9, masking real divergence).
                 * Tracked separately so a >bound regression still fails. */
                else if( !tolEntry )
                {
                    double m = fabs(a) > fabs(b) ? fabs(a) : fabs(b);
                    if( fma_needs_input_scale(fi->name) && inScale > m ) m = inScale;
                    if( m > 0.0 && d <= FMA_TRANSITION_REL * m )
                    {
                        fmaDiff = 1;
                        if( d / m > ctx->maxFmaRel ) ctx->maxFmaRel = d / m;
                    }
                    else { realDiff = 1; if( firstO < 0 ) { firstO = (int)o; firstJ = j; } }
                }
#endif
                else { realDiff = 1; if( firstO < 0 ) { firstO = (int)o; firstJ = j; } }
            }
        }
    }

    if( !realDiff && (benignDiff || tolDiff || fmaDiff) )
        return tolDiff ? 2 : (fmaDiff ? 3 : 1);   /* 2 = manifest, 3 = FMA re-baseline, 1 = signed-zero */

    if( report )
    {
        ctx->reportedThisFunc++;
        printf("  MISMATCH TA_%s shape=%d seed=%d n=%d range=[%d,%d]  params:",
               fi->name, shape, seed, n, s, e);
        for( unsigned int i = 0; i < fi->nbOptInput; i++ )
        {
            const TA_OptInputParameterInfo *oi;
            TA_GetOptInputParameterInfo(fi->handle, i, &oi);
            printf(" %s=%.15g", oi->paramName, optVals[i]);
        }
        if( !realDiff )
            printf("\n    hash differs but elements match (nbElem %d) — investigate\n", curNb);
        else if( p->outputIsInteger[firstO] )
            printf("\n    out%d[%d]: current=%d  v0.6.4=%d\n",
                   firstO, firstJ, p->outIntBufs[firstO][firstJ], g_fz064Int[firstO][firstJ]);
        else
            printf("\n    out%d[%d]: current=%.17g (%a)  v0.6.4=%.17g (%a)\n",
                   firstO, firstJ, p->outRealBufs[firstO][firstJ], p->outRealBufs[firstO][firstJ],
                   g_fz064Real[firstO][firstJ], g_fz064Real[firstO][firstJ]);
    }
    return 0;
}

static void fuzz_one_function(const TA_FuncInfo *funcInfo, void *opaqueData)
{
    FuzzContext *ctx = (FuzzContext *)opaqueData;
    unsigned int i;

    if( ctx->error != TA_TEST_PASS ) return;
    if( !codegen_matches_filter(ctx->functionFilter, funcInfo->name) ) return;

    /* Overflowing the cap must FAIL, not skip. A silent return here would drop
     * the function from the differential entirely — no message, no counter — so
     * the run stays green while testing nothing. Same treatment as the
     * STREAM_MAX_OPT guard above. */
    if( funcInfo->nbOptInput > FUZZ_MAX_OPT )
    {
        printf("FUZZ PARAM OVERFLOW [TA_%s]: %u opt params > FUZZ_MAX_OPT (%d) — "
               "raise the cap; skipping would make this function's gate vacuous\n",
               funcInfo->name, funcInfo->nbOptInput, FUZZ_MAX_OPT);
        ctx->failures++;
        ctx->funcsWithFailures++;
        ctx->error = TA_CODEGEN_OUTPUT_MISMATCH;
        return;
    }

    /* Subset tolerance is 0.6.4-only: skip functions added after 0.6.4. */
    if( ctx->funcList )
    {
        char needle[80];
        snprintf(needle, sizeof(needle), "\"TA_%s\"", funcInfo->name);
        if( !strstr(ctx->funcList, needle) ) { ctx->funcsSkipped++; return; }
    }

    /* STOCHRSI intentionally diverges from 0.6.4 (issue #107): its internal
     * STOCHF now guards the divide with TA_IS_ZERO where 0.6.4 divided a sub-
     * epsilon flat-RSI-window residue into full-scale [0,100] noise. That makes
     * 0.6.4 the wrong oracle for STOCHRSI, so it is excluded from the differential
     * fuzz; the new behaviour is pinned by hardcoded tests in test_stoch.c.
     * (STOCH/STOCHF on raw OHLC do NOT diverge and stay strictly compared.) */
    if( strcmp(funcInfo->name, "STOCHRSI") == 0 ) { ctx->stochRsiSkipped++; return; }


    /* VAR/STDDEV/BBANDS intentionally diverge from 0.6.4 (issue #118): their
     * variance moved from the catastrophically-cancelling E[x^2]-mean^2 to a
     * cancellation-free shifted-data form, so on ILL-CONDITIONED windows 0.6.4
     * (which collapsed - SourceForge bug 90) is the wrong oracle. Those cases are
     * skipped per-case below, gated on fuzz_variance_condition(); every
     * well-conditioned case IS compared, at the manifest's output-relative bound.
     * The new behaviour is additionally pinned by test_stddev.c and the BBANDS
     * stable-variance test, stays bitwise cross-language (--xlang-hash) and
     * batch==stream (stream_verify). */
    int isVarianceFunc = ( strcmp(funcInfo->name, "VAR") == 0 ||
                           strcmp(funcInfo->name, "STDDEV") == 0 ||
                           strcmp(funcInfo->name, "BBANDS") == 0 );

    /* CORREL/BETA intentionally diverge from 0.6.4 (issue #242), for the same
     * reason and by the same remedy as #118 above: their sums moved off the
     * cancelling one-pass form onto shifted data with a reseed, and their fixed
     * TA_EPSILON guards became scale-relative. On an ILL-CONDITIONED window
     * 0.6.4 does not merely round differently -- it returned exactly 0, or a
     * correlation outside [-1,1] -- so it is the wrong oracle there. Skipped
     * per-case below on fuzz_correl_condition()/fuzz_beta_condition(); every
     * better-conditioned case IS compared, at the manifest's output-relative
     * bound. The new behaviour is pinned by test_correl.c / test_beta.c against
     * oracles sharing no code with either version, stays bitwise cross-language
     * (--xlang-hash) and batch==stream (stream_verify). */
    int isCorrelFunc = ( strcmp(funcInfo->name, "CORREL") == 0 );
    int isBetaFunc   = ( strcmp(funcInfo->name, "BETA")   == 0 );

    for( i = 0; i < funcInfo->nbInput; i++ )
    {
        const TA_InputParameterInfo *ii;
        TA_GetInputParameterInfo(funcInfo->handle, i, &ii);
        if( ii->type == TA_Input_Integer ) return;   /* no test data for these */
    }

    TA_ParamHolder *paramHolder = NULL;
    if( TA_ParamHolderAlloc(funcInfo->handle, &paramHolder) != TA_SUCCESS ) return;

    TA_History hist;
    memset(&hist, 0, sizeof(hist));
    hist.open = g_fzBuf[0]; hist.high = g_fzBuf[1]; hist.low = g_fzBuf[2];
    hist.close = g_fzBuf[3]; hist.volume = g_fzBuf[4]; hist.openInterest = g_fzBuf[5];

    CodegenRangeTestParam p;
    memset(&p, 0, sizeof(p));
    p.funcInfo = funcInfo;
    p.paramHolder = paramHolder;
    p.history = &hist;
    setup_inputs(paramHolder, funcInfo, &hist);
    setup_outputs(&p);

    double vec[FUZZ_MAX_VEC][FUZZ_MAX_OPT];
    int vecOverflow = 0;
    /* frozenOracle=1: no contract candidates — the 0.6.4 oracle certifies numbers,
     * not the parameter contract. NULL kind: nothing to flag. */
    int nvec = fuzz_build_vectors(funcInfo, vec, &vecOverflow, 1, NULL);
    if( vecOverflow > 0 )
    {
        printf("FUZZ VECTOR OVERFLOW [TA_%s]: %d parameter value(s) dropped by "
               "the FUZZ_MAX_VEC/FUZZ_MAX_CAND caps — they would go uncompared vs 0.6.4\n",
               funcInfo->name, vecOverflow);
        ctx->failures++;   /* run fails: failures != 0 (see the 064 exit check) */
        TA_ParamHolderFree(paramHolder);
        return;
    }

    static const int sizes[]  = {40, 120, 240};
    static const int seeds[]  = {1, 2, 3};
    ctx->reportedThisFunc = 0;
    long long failBefore = ctx->failures;
    long long benignBefore = ctx->benign;
    long long cciTolBefore = ctx->cciTol;
    long long fmaTolBefore = ctx->fmaTol;

    for( int shape = 0; shape < FUZZ_NSHAPES; shape++ )
    for( int si = 0; si < (int)(sizeof(seeds)/sizeof(seeds[0])); si++ )
    for( int zi = 0; zi < (int)(sizeof(sizes)/sizeof(sizes[0])); zi++ )
    {
        int n = sizes[zi]; if( n > FUZZ_MAXN ) n = FUZZ_MAXN;
        fuzz_gen(shape, seeds[si], n,
                 g_fzBuf[0], g_fzBuf[1], g_fzBuf[2], g_fzBuf[3], g_fzBuf[4], g_fzBuf[5]);
        hist.nbBars = (unsigned int)n;
        p.nbBars = n;
        /* Data scale for the manifest's TOL_REL_IN bound (max |close|; close is
         * the single real input the LINEARREG family reads). Floored at 1. */
        double inScale = 1.0;
        for( int z = 0; z < n; z++ )
            if( fabs(g_fzBuf[3][z]) > inScale ) inScale = fabs(g_fzBuf[3][z]);

        for( int k = 0; k < nvec; k++ )
        {
            for( i = 0; i < funcInfo->nbOptInput && i < FUZZ_MAX_OPT; i++ )
            {
                const TA_OptInputParameterInfo *oi;
                TA_GetOptInputParameterInfo(funcInfo->handle, i, &oi);
                if( oi->type == TA_OptInput_RealRange || oi->type == TA_OptInput_RealList )
                    TA_SetOptInputParamReal(paramHolder, i, vec[k][i]);
                else
                    TA_SetOptInputParamInteger(paramHolder, i, (int)vec[k][i]);
            }

            /* subranges: full, plus two deterministic random windows */
            unsigned long long rs = 0xF0F0ULL ^ ((unsigned long long)shape<<8)
                                    ^ ((unsigned long long)seeds[si]<<16) ^ ((unsigned long long)k<<24);
            int ranges[3][2];
            ranges[0][0] = 0; ranges[0][1] = n - 1;
            for( int rr = 1; rr < 3; rr++ )
            {
                int rsS = (int)(fuzz_sm_unit(&rs) * n);
                int rsE = rsS + (int)(fuzz_sm_unit(&rs) * (n - rsS));
                if( rsS > n - 1 ) rsS = n - 1;
                if( rsE > n - 1 ) rsE = n - 1;
                if( rsE < rsS ) rsE = rsS;
                ranges[rr][0] = rsS; ranges[rr][1] = rsE;
            }

            for( int ri = 0; ri < 3; ri++ )
            {
                int s = ranges[ri][0], e = ranges[ri][1];
                TA_SetUnstablePeriod(TA_FUNC_UNST_ALL, 0);

                /* #98 fixes diverge from 0.6.4 only on their trigger cases:
                 * TRIX/NATR startIdx > lookback; NATR also when a close in
                 * the output range is zero (old code wrote outReal[0]). */
                if( strcmp(funcInfo->name, "TRIX") == 0 ||
                    strcmp(funcInfo->name, "NATR") == 0 )
                {
                    TA_Integer lb98 = 0;
                    int skip98 = 0;
                    if( TA_GetLookback(paramHolder, &lb98) == TA_SUCCESS )
                    {
                        if( s > lb98 )
                            skip98 = 1;
                        else if( strcmp(funcInfo->name, "NATR") == 0 )
                        {
                            for( int z = (s > lb98 ? s : lb98); z <= e; z++ )
                                if( g_fzBuf[3][z] < 0.00000001 &&
                                    g_fzBuf[3][z] > -0.00000001 )
                                { skip98 = 1; break; }
                        }
                    }
                    if( skip98 ) { ctx->skipped98++; continue; }
                }

                /* #118: compare against 0.6.4 only where its cancelling variance
                 * form still has significant digits. optInTimePeriod is opt 0 for
                 * all three; the primary input is close (g_fzBuf[3]). */
                if( isVarianceFunc )
                {
                    double kappa = fuzz_variance_condition( g_fzBuf[3], n, (int)vec[k][0], s, e );
                    if( kappa > FUZZ_VAR_MAX_KAPPA ) { ctx->varianceSkipped++; continue; }
                }

                /* #242: the same carve-out over two series. Both take
                 * (inReal0, inReal1) = (close, volume) per setup_inputs, and
                 * optInTimePeriod is opt 0 for both. */
                if( isCorrelFunc || isBetaFunc )
                {
                    double kappa = isCorrelFunc
                        ? fuzz_correl_condition( g_fzBuf[3], g_fzBuf[4], n, (int)vec[k][0], s, e )
                        : fuzz_beta_condition  ( g_fzBuf[3], g_fzBuf[4], n, (int)vec[k][0], s, e );
                    if( kappa > FUZZ_XY_MAX_KAPPA ) { ctx->xySkipped++; continue; }
                }

                /* #244: skip only the cases v0.6.4 reports wrongly; the rest
                 * stay bit-exact. g_fzBuf is O,H,L,C,V,OI and optInTimePeriod
                 * is opt 0. */
                if( strcmp(funcInfo->name, "MFI") == 0 &&
                    fuzz_mfi_064_blind( g_fzBuf[1], g_fzBuf[2], g_fzBuf[3], g_fzBuf[4],
                                        n, (int)vec[k][0], s, e ) )
                { ctx->mfiSkipped++; continue; }

                TA_Integer curBeg = 0, curNb = 0;
                for( unsigned int o = 0; o < funcInfo->nbOutput; o++ )
                {
                    if( p.outputIsInteger[o] )
                        TA_SetOutputParamIntegerPtr(paramHolder, o, p.outIntBufs[o]);
                    else
                        TA_SetOutputParamRealPtr(paramHolder, o, p.outRealBufs[o]);
                }
                TA_RetCode curRc = TA_CallFunc(paramHolder, s, e, &curBeg, &curNb);
                unsigned long long curHash =
                    fuzz_hash_local(&p, (curRc == TA_SUCCESS) ? curNb : 0);

                fuzz_build_request(ctx->reqBuf, funcInfo, s, e, shape, seeds[si], n, vec[k], 0);
                if( !fuzz_call(ctx) ) { if( ctx->error != TA_TEST_PASS ) return; continue; }
                int refRc  = json_get_int(ctx->respBuf, "retCode");
                int refBeg = json_get_int(ctx->respBuf, "outBegIdx");
                int refNb  = json_get_int(ctx->respBuf, "outNBElement");
                unsigned long long refHash = fuzz_parse_hash(ctx->respBuf);

                ctx->comparisons++;
                int mismatch = 0;
                if( (int)curRc != refRc ) mismatch = 1;
                else if( curRc == TA_SUCCESS )
                {
                    if( curBeg != refBeg || curNb != refNb ) mismatch = 1;
                    else if( curHash != refHash ) mismatch = 1;
                }
                if( !mismatch ) { ctx->matches++; continue; }

                int cls = fuzz_classify_and_report(ctx, funcInfo, &p, shape, seeds[si], n, s, e,
                                             vec[k], inScale, (int)curRc, curBeg, curNb,
                                             refRc, refBeg, refNb);
                if( cls == 0 )      ctx->failures++;
                else if( cls == 2 ) ctx->cciTol++;   /* manifest-tolerated (CCI #7 / LINEARREG #103 / IMI #112) — not a failure */
                else if( cls == 3 ) ctx->fmaTol++;   /* one-time FMA re-baseline tolerance (PR #96) — not a failure */
                else                ctx->benign++;
            }
        }
    }

    if( ctx->failures > failBefore ) ctx->funcsWithFailures++;
    else if( ctx->cciTol > cciTolBefore )
    {
        int tm = 0; double tv = 0.0, tc = 0.0;
        fuzz_064_tol_lookup(funcInfo->name, &tm, &tv, &tc);
        if( tm == TOL_NAN_TO )
            printf("  TOLERATED TA_%s: %lld case(s) where v0.6.4 emitted NaN (x/0) and current "
                   "returns the guarded %g (authorized manifest)\n",
                   funcInfo->name, ctx->cciTol - cciTolBefore, tv);
        else
        {
            int ti = 0; double tmax = 0.0;
            const void *te = fuzz_064_tol_lookup(funcInfo->name, &ti, &tv, &tc);
            if( te ) tmax = g_fuzz064TolMax[(const TA_Fuzz064Tol *)te - FUZZ_064_TOL];
            printf("  TOLERATED TA_%s: %lld case(s) within %g%s%s vs 0.6.4 (authorized manifest bound, max observed %.3g)\n",
                   funcInfo->name, ctx->cciTol - cciTolBefore, tv,
                   tm == TOL_REL_IN ? " * max|input|" : "",
                   (tm == TOL_REL_IN && tc > 0.0) ? " (capped)" : "", tmax);
        }
    }
    else if( ctx->fmaTol > fmaTolBefore )
        printf("  FMA-REBASELINE TA_%s: %lld case(s) within 1e-9 relative of v0.6.4 "
               "(explicit fma() adoption, PR #96)\n",
               funcInfo->name, ctx->fmaTol - fmaTolBefore);
    else if( ctx->benign > benignBefore )
    {
        ctx->funcsBenign++;
        printf("  BENIGN TA_%s: %lld signed-zero case(s) (numerically equal, +0.0 vs -0.0)\n",
               funcInfo->name, ctx->benign - benignBefore);
    }
    free_outputs(&p);
    TA_ParamHolderFree(paramHolder);
}

ErrorNumber fuzz_ref064(const char *functionFilter)
{
    printf("\n=============================================\n");
    printf("Bit-exact differential fuzz vs released v0.6.4\n");
    printf("=============================================\n");

    CodegenPipe cp;
    if( codegen_pipe_open(&cp, argv_064) != TA_TEST_PASS )
    {
        printf("FAILED: cannot start ta_064_serve.\n"
               "        Build it first (scripts/build_064_serve.py): it links the\n"
               "        frozen v0.6.4 lib from the ../ta-lib-064 worktree.\n");
        return TA_CODEGEN_ALLOC_FAILED;
    }
    printf("Oracle: ta_064_serve (pid=%d)\n\n", cp.child_pid);

    FuzzContext ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.functionFilter = functionFilter;
    ctx.cp = &cp;
    ctx.reqBuf = malloc(JSON_BUF_SIZE);
    ctx.respBuf = malloc(JSON_BUF_SIZE);
    char *funcList = malloc(JSON_BUF_SIZE);
    ctx.error = TA_TEST_PASS;
    if( !ctx.reqBuf || !ctx.respBuf || !funcList )
    { free(ctx.reqBuf); free(ctx.respBuf); free(funcList);
        codegen_pipe_close(&cp); return TA_CODEGEN_ALLOC_FAILED; }

    /* Subset gate: cache 0.6.4's supported-function set. Functions added after
     * 0.6.4 are skipped (no baseline), never failed — see fuzz_one_function. */
    if( codegen_pipe_call(&cp, "{\"method\":\"list_functions\",\"params\":{}}",
                          funcList, JSON_BUF_SIZE) == TA_TEST_PASS
        && strstr(funcList, "\"functions\"") )
        ctx.funcList = funcList;
    else
        printf("  (warning: list_functions failed — subset gate disabled)\n");

    g_frozenEnumSkips = 0;
    TA_ForEachFunc(fuzz_one_function, &ctx);

    free(ctx.reqBuf); free(ctx.respBuf); free(funcList);
    codegen_pipe_close(&cp);

    printf("\n---------------------------------------------\n");
    printf("comparisons: %lld   matches: %lld   benign(signed-zero): %lld   "
           "cci-tolerated: %lld   fma-tolerated: %lld   failures: %lld\n",
           ctx.comparisons, ctx.matches, ctx.benign, ctx.cciTol, ctx.fmaTol, ctx.failures);
    printf("functions: %d not-in-0.6.4 (skipped), %d with benign-only diffs, %d with real failures\n",
           ctx.funcsSkipped, ctx.funcsBenign, ctx.funcsWithFailures);
    if( ctx.skipped98 > 0 )
        printf("skipped: %lld TRIX/NATR partial-range case(s) — fixed in 0.8.1, issue #98\n",
               ctx.skipped98);
    if( ctx.cciTol > 0 )
        printf("manifest-tolerated: %lld case(s) under an authorized latest->0.6.4 entry "
               "(CCI #7 near-zero; LINEARREG family + TSF #103 sliding-sum; IMI #112 NaN->50.0)\n", ctx.cciTol);
#if FMA_TRANSITION_TOLERANCE
    if( ctx.fmaTol > 0 )
        printf("fma-rebaseline: %lld case(s) within the 1e-9 relative FMA contract vs 0.6.4 "
               "(max observed %.3g); one-time transition, re-freeze to hash-exact after the "
               "FMA-enabled release is tagged (PR #96)\n", ctx.fmaTol, ctx.maxFmaRel);
#endif
    if( ctx.stochRsiSkipped > 0 )
        printf("stochrsi-skipped: %lld STOCHRSI function(s) skipped entirely — intentionally diverges from 0.6.4 (issue #107); pinned by test_stoch.c\n",
               ctx.stochRsiSkipped);
    if( ctx.mfiSkipped > 0 )
        printf("mfi-skipped: %lld MFI case(s) where v0.6.4 reports a non-index (issue #244): its 1.0 guard fired, the window was empty, or a one-sided window left it dividing residue. Every other MFI case was compared bit-exact\n",
               ctx.mfiSkipped);
    if( g_frozenEnumSkips > 0 )
        printf("post-freeze enums: %lld MAType value(s) > %d excluded vs v0.6.4 "
               "(#139, #93, #182; covered current-vs-current by xlang-hash/stream/COMPOSITE)\n",
               g_frozenEnumSkips, FROZEN_ORACLE_MATYPE_MAX);
    if( ctx.varianceSkipped > 0 )
        printf("variance-skipped: %lld VAR/STDDEV/BBANDS case(s) ill-conditioned for 0.6.4 (kappa > %.0e, issue #118); every better-conditioned case was compared\n",
               ctx.varianceSkipped, (double)FUZZ_VAR_MAX_KAPPA);
    if( ctx.xySkipped > 0 )
        printf("correl/beta-skipped: %lld CORREL/BETA case(s) ill-conditioned for 0.6.4 (kappa > %.0e, issue #242); every better-conditioned case was compared\n",
               ctx.xySkipped, (double)FUZZ_XY_MAX_KAPPA);
    if( ctx.serverRestarts )
        printf("oracle restarts (recovered crashes): %d\n", ctx.serverRestarts);
    if( ctx.comparisons == 0 )
    {
        printf("FAIL — zero comparisons (broken ta_064_serve or over-narrow filter?).\n");
        return TA_CODEGEN_OUTPUT_MISMATCH;
    }
    if( ctx.failures == 0 && ctx.error == TA_TEST_PASS )
    {
        printf("PASS — current library is bit-identical to v0.6.4 at period>=2"
               " (benign signed-zero and authorized manifest tolerances aside; STOCHRSI excluded, issue #107).\n");
        return TA_TEST_PASS;
    }
    printf("FAIL — %lld real divergence(s) across %d function(s).\n",
           ctx.failures, ctx.funcsWithFailures);
    return ctx.error != TA_TEST_PASS ? ctx.error : TA_CODEGEN_OUTPUT_MISMATCH;
}

/* ======================================================================== *
 * --xlang-hash: cross-language BITWISE parity gate (issue #113).
 *
 * Proves the generated language servers compute BIT-IDENTICAL outputs to the
 * shipped in-process C library, with NO tolerance (contrast --codegen's
 * CODEGEN_EPSILON_DOUBLE == 1e-9 element-wise gate) — the one carve-out is
 * Java's transcendental calls (below). It closes the two precision losses that
 * make a %.15g cross-language comparison unable to see ~1e-10 FMA drift: (a)
 * inputs cross full-precision — a seed both sides regenerate, or lossless
 * hex-of-IEEE-bits — so no JSON float-parse rounding; (b) outputs are compared
 * by a full-precision FNV-1a hash of the raw bytes — never %.15g-serialized.
 * Since PR #96 every backend fuses the identical a*b+c sites (shared detector in
 * backends/fma.rs) and builds with -ffp-contract=off, so this gate is expected
 * GREEN; any mismatch is a real fusion-site / codegen divergence to fix.
 *
 * The C library is linked IN-PROCESS in ta_regtest, so there is no JSON-RPC
 * boundary on the C side and no precision to reconcile — it is the GOLDEN
 * reference (exactly as --fuzz-064 uses the in-process current library). Each
 * language server crosses the boundary and is diffed against it, per its
 * transport (XlangServer.usesSeed):
 *   - Rust: the seed transport (gen_present + fuzz_in_hash self-check), diffed
 *     BITWISE — Rust uses the system libm, so it is bit-identical to C.
 *   - Java: no in-server fuzz_gen port (#114 is complete), so the driver sends
 *     the exact seed-generated arrays losslessly (hex-of-IEEE-bits, the #115
 *     server_verify transport) and requests want_hash. Non-transcendental calls
 *     are diffed BITWISE; a call that reaches a transcendental (fdlibm != the C
 *     libm, ~1 ULP) drops to a CODEGEN_TRANSCENDENTAL_TOL (1e-9) element
 *     compare, and HT_DCPHASE/HT_SINE on the zero-variance constant shape are
 *     skipped outright (xlang_illcond — atan2 of a null signal amplifies
 *     the ULP unboundedly; C and Rust stay bitwise there).
 *   - C#: the same hex-bits transport as Java (the managed server has no
 *     fuzz_gen port either), and it takes the SAME tolerance lane. It was
 *     briefly configured as fully bitwise on the strength of a green local run;
 *     dev-nightly 30776189041 then produced 25 TA_LN mismatches on
 *     ubuntu-latest x86-64 from a commit that was clean on aarch64 and on the
 *     dev box. .NET does not guarantee `Math.*` reaches the platform libm.
 *     Math.FusedMultiplyAdd IS correctly rounded, so the FMA contract is
 *     unaffected — only the transcendentals moved.
 * There is NO 0.6.4 here (current-vs-current), so — unlike --fuzz-064 — there
 * are none of the #98/#107 carve-outs; every case is bitwise except the
 * transcendental calls of Java and C#. See fuzz_data.h and
 * src/tools/ta_regtest/CLAUDE.md.
 * ======================================================================== */

/* The non-zero unstable period this gate sweeps (#116). Small on purpose: the
 * value only has to move the lookback for the leg to bite, and every extra bar
 * of unstable period is a bar of output the comparison no longer covers. The
 * ref differential sweep it replaces used 3 as well. */
#define XLANG_UNST_PERIOD 3

typedef struct {
    const char        *name;      /* "rust", "java", "csharp" — --language= tokens */
    const char        *display;   /* "Rust", "Java", "C#"                     */
    const char *const *argv;      /* server launch command                   */
    int                usesSeed;  /* transport: 1 = seed (gen_present + fuzz_in_hash
                                   * self-check), 0 = lossless hex-bits inputs (no
                                   * fuzz_gen port — Java #114, C#)           */
    int                tolTranscendental; /* 1 = calls reaching a transcendental
                                   * drop to the 1e-9 element compare; 0 = every
                                   * call is bitwise. DELIBERATELY independent
                                   * of the transport: inheriting the tolerance
                                   * with usesSeed=0 would silently untest 20
                                   * functions at bit level.
                                   *
                                   * Java: fdlibm != the C libm, known since
                                   * #113. C#: .NET does NOT guarantee `Math.*`
                                   * defers to the platform libm, and on some
                                   * hosts it does not — dev-nightly run
                                   * 30776189041 hit 25 TA_LN mismatches on
                                   * ubuntu-latest x86-64 while the SAME commit
                                   * was bitwise-clean on ubuntu-24.04-arm and
                                   * on a glibc-2.39 + .NET-10.0.10 dev box.
                                   * Not special values: C and C# agree
                                   * bit-for-bit on 0.0/-0.0/negatives incl. the
                                   * NaN payload — it is a normal-value 1 ULP
                                   * difference. A bitwise claim verified on one
                                   * machine is a claim about that machine.
                                   * Rust stays 0: it reaches the same libm the
                                   * golden does.                             */
    int                enumSentinel; /* 1 = this language's optional-param surface
                                   * can CARRY the integer default sentinel on a
                                   * choice-list parameter, so the #162 sentinel
                                   * leg is a real assertion there. 0 = it cannot
                                   * — see codegen_lang_can_pass_enum_sentinel. */
    CodegenPipe        cp;
    int                open;      /* 1 once the subprocess is up              */
    long long          cases;     /* cases compared against the golden        */
    long long          mism;      /* real bitwise/tolerance mismatches        */
    int                restarts;  /* recovered subprocess crashes             */
} XlangServer;

/* 1 when this parameter vector puts the default sentinel on a CHOICE-LIST
 * parameter. Only one parameter is varied per vector and the all-defaults vector
 * never holds a sentinel, so scanning the vector is exact — and it keeps the
 * FUZZ_VEC_SENTINEL class single, rather than splitting it into two that every
 * existing `kind[k] == FUZZ_VEC_SENTINEL` test would have to be taught. */
static int xlang_sentinel_on_choice_list(const TA_FuncInfo *fi, const double *vals)
{
    for( unsigned int i = 0; i < fi->nbOptInput && i < FUZZ_MAX_OPT; i++ )
    {
        const TA_OptInputParameterInfo *oi;
        TA_GetOptInputParameterInfo(fi->handle, i, &oi);
        if( oi->type == TA_OptInput_IntegerList
            && (int)vals[i] == TA_INTEGER_DEFAULT )
            return 1;
    }
    return 0;
}

typedef struct {
    const char  *functionFilter;
    XlangServer *sv;
    int          nsv;
    char        *reqBuf;
    char        *respBuf;
    long long    comparisons;        /* golden cases evaluated                 */
    long long    funcsSwept;         /* functions past the --function filter — printed
                                      * in the PASS line so a caller can assert the
                                      * sweep was not vacuous (synth_gate.py does)   */
    long long    nonEmpty;           /* cases with a non-empty successful output
                                      * (non-vacuity: an empty output hashes the
                                      * same on both sides, so a healthy fraction
                                      * must be non-empty for the gate to bite)   */
    long long    tolCases;           /* Java calls routed to the transcendental
                                      * tolerance path (the rest are bitwise)     */
    long long    illcondSkipped;     /* Java HT_DCPHASE/HT_SINE calls skipped on
                                      * the zero-variance constant shape (phase of
                                      * a null signal — see xlang_illcond)    */
    long long    oorCases;           /* per-server comparisons on an out-of-range
                                      * parameter vector — the retCode-parity leg
                                      * (batch tier)                               */
    long long    oorNotRejected;     /* out-of-range vectors the in-process C
                                      * library ACCEPTED — the candidate was not
                                      * out of range, so its parity assertion was
                                      * vacuous. Fails the run.                    */
    long long    sentCases;          /* per-server comparisons on a sentinel vector */
    long long    sentEnumCases;      /* ... of which on a CHOICE-LIST sentinel. Kept
                                      * apart from sentCases because the range params
                                      * keep that total large whether this leg runs
                                      * or not (#162)                               */
    long long    lbSentEnumCases;    /* ... the same, on the lookback tier          */
    long long    sentEnumSkipped;    /* choice-list sentinel vectors skipped for a
                                      * server whose typed enum cannot carry the
                                      * sentinel (Java) — counted, not silent     */
    long long    sentNotDefault;     /* sentinel vectors where the in-process C
                                      * library did NOT reproduce the explicit
                                      * default's result. Fails the run.           */
    long long    unstCases;          /* per-server comparisons run at a NON-ZERO
                                      * unstable period (#116). Zero here means
                                      * the axis is not being exercised at all —
                                      * printed so it cannot go quiet unnoticed  */
    long long    unstFuncs;          /* functions that carried an unstable leg     */
    long long    lbCases;            /* per-server lookback-tier comparisons       */
    long long    lbOorCases;         /* ... of which on an out-of-range vector     */
    long long    lbSentCases;        /* ... of which on a default-sentinel vector  */
    int          reportedThisFunc;
    int          funcsWithFailures;
    ErrorNumber  error;
} XlangCtx;

/* Count the functions the gate will actually visit, so the banner cannot drift
 * from reality the way a hardcoded literal does (it read 162 against 165). */
static void codegen_count_cb(const TA_FuncInfo *funcInfo, void *opaqueData)
{
    (void)funcInfo;
    (*(int *)opaqueData)++;
}

static int codegen_function_count(void)
{
    int n = 0;
    TA_ForEachFunc(codegen_count_cb, &n);
    return n;
}

/* Parse a hex hash string field; *present=0 if the field is absent (which for a
 * gen_present request means the server does not speak the out_hash protocol). */
static unsigned long long xlang_parse_hash(const char *resp, const char *field, int *present)
{
    int len;
    const char *h = json_find_field(resp, field, &len);
    if( !h ) { if( present ) *present = 0; return 0; }
    if( present ) *present = 1;
    if( *h == '"' ) h++;
    return strtoull(h, NULL, 16);
}

/* Shared per-server compare: parse a server's hash-mode response and diff it
 * against the in-process C golden. Pure — no I/O. See test_codegen.h. */
XHashVerdict codegen_hash_compare(const char *resp,
                                  TA_RetCode goldRc, int goldBeg, int goldNb,
                                  unsigned long long goldHash, XHashParsed *parsed)
{
    XHashParsed local;
    int present = 0;
    if( !parsed ) parsed = &local;
    parsed->rc        = json_get_int(resp, "retCode");
    parsed->begIdx    = json_get_int(resp, "outBegIdx");
    parsed->nbElement = json_get_int(resp, "outNBElement");
    parsed->hash      = xlang_parse_hash(resp, "out_hash", &present);
    if( !present )                    return XHASH_NO_HASH;
    if( parsed->rc != (int)goldRc )   return XHASH_RETCODE;
    if( goldRc != TA_SUCCESS )        return XHASH_MATCH;  /* matching error code */
    if( parsed->begIdx != goldBeg || parsed->nbElement != goldNb )
                                      return XHASH_SHAPE;
    if( parsed->hash != goldHash )    return XHASH_BITS;
    return XHASH_MATCH;
}

/* Shared diagnostic tail (the prefix — seed/scenario — is the caller's). */
void codegen_hash_report(const char *who, TA_RetCode goldRc, int goldBeg,
                         int goldNb, unsigned long long goldHash,
                         const XHashParsed *parsed)
{
    printf("    retCode %d/%d  begIdx %d/%d  nbElem %d/%d  hash %016llx/%016llx (golden/%s)\n",
           (int)goldRc, parsed->rc, goldBeg, parsed->begIdx, goldNb, parsed->nbElement,
           goldHash, parsed->hash, who);
}

/* ---- Java-transcendental tolerance path (shared with server_verify, #113/#115)
 * Which CALLS reach a transcendental C math routine (atan/sin/cos/exp/log/...) —
 * the only ones whose Java (fdlibm) output can differ from the C libm (~1 ULP),
 * hence the only ones --xlang-hash's Java leg and server_verify relax from
 * bitwise to CODEGEN_TRANSCENDENTAL_TOL. Every other function — including
 * sqrt/ceil/floor users (IEEE correctly-rounded) — stays bit-identical across
 * languages. Source-derived from a grep of ta_codegen/input. ---- */
static const char *const CODEGEN_TRANSCENDENTAL[] = {
    "ACOS", "ASIN", "ATAN", "COS", "COSH", "EXP",
    "HT_DCPERIOD", "HT_DCPHASE", "HT_PHASOR", "HT_SINE", "HT_TRENDLINE",
    "HT_TRENDMODE", "LINEARREG_ANGLE", "LN", "LOG10", "MAMA",
    "SIN", "SINH", "TAN", "TANH",
};

int codegen_is_transcendental(const char *name)
{
    for( unsigned int i = 0;
         i < sizeof(CODEGEN_TRANSCENDENTAL) / sizeof(CODEGEN_TRANSCENDENTAL[0]); i++ )
        if( strcmp(CODEGEN_TRANSCENDENTAL[i], name) == 0 )
            return 1;
    return 0;
}

/* The MA-dispatch functions (MA, MAVP, BBANDS, MACDEXT, APO, PPO, STOCH*) route
 * to MAMA — which uses atan — when a MAType optional parameter selects it, so
 * transcendental-ness is a per-CALL property (TA_MAType_MAMA == 7, enums.yaml;
 * MAType params are the only optInputs whose paramName contains "MAType"). */
int codegen_call_is_transcendental(const TA_FuncHandle *handle,
                                   const double optVals[], int nbOpt)
{
    const TA_FuncInfo *fi;
    if( TA_GetFuncInfo(handle, &fi) != TA_SUCCESS )
        return 0;
    if( codegen_is_transcendental(fi->name) )
        return 1;
    for( unsigned int i = 0; i < fi->nbOptInput; i++ )
    {
        const TA_OptInputParameterInfo *oi;
        TA_GetOptInputParameterInfo(handle, i, &oi);
        /* One optVals slot per optInput (default assumed beyond nbOpt). */
        double val = (optVals && (int)i < nbOpt) ? optVals[i] : oi->defaultValue;
        if( oi->paramName && strstr(oi->paramName, "MAType") &&
            (int)val == 7 /* TA_MAType_MAMA */ )
            return 1;
    }
    return 0;
}

/* Shared tolerance element-compare (the Java transcendental path). Parallels
 * codegen_hash_compare's retCode/shape gating, then compares each output's
 * elements: reals at `tol` (relative for |v|>1, absolute otherwise; finite-vs-
 * NaN always fails, so the tolerance path is as NaN-discriminating as the
 * bitwise hash of raw bytes), integers exact. Output field keys follow the raw
 * output index (outReal/outReal1/…, outInteger/outInteger1/…); every
 * multi-output TA function is type-homogeneous, so that equals within-type
 * indexing. Both gates' output lengths are far under CODEGEN_TOL_MAX_OUT. */
#define CODEGEN_TOL_MAX_OUT 512
CTolVerdict codegen_compare_tol(const char *resp,
                                unsigned int nbOutput, const int *outIsInteger,
                                const void *const *outBufs,
                                TA_RetCode goldRc, int goldBeg, int goldNb,
                                double tol, CTolDetail *detail)
{
    CTolDetail local;
    if( !detail ) detail = &local;
    memset(detail, 0, sizeof(*detail));
    detail->rc        = json_get_int(resp, "retCode");
    detail->begIdx    = json_get_int(resp, "outBegIdx");
    detail->nbElement = json_get_int(resp, "outNBElement");
    if( detail->rc != (int)goldRc )   return CTOL_RETCODE;
    if( goldRc != TA_SUCCESS )        return CTOL_MATCH;   /* matching error code */
    if( detail->begIdx != goldBeg || detail->nbElement != goldNb )
                                      return CTOL_SHAPE;
    if( goldNb <= 0 )                 return CTOL_MATCH;

    for( unsigned int o = 0; o < nbOutput && o < MAX_OUTPUTS; o++ )
    {
        char field[32];
        if( outIsInteger[o] )
        {
            snprintf(field, sizeof(field), o == 0 ? "outInteger" : "outInteger%u", o);
            TA_Integer srv[CODEGEN_TOL_MAX_OUT];
            int cnt = json_get_int_array(resp, field, srv, CODEGEN_TOL_MAX_OUT);
            if( cnt != goldNb )
            { detail->output = (int)o; detail->isInt = 1; detail->srvCount = cnt; return CTOL_COUNT; }
            const TA_Integer *gold = (const TA_Integer *)outBufs[o];
            for( int j = 0; j < goldNb; j++ )
                if( gold[j] != srv[j] )
                {
                    detail->output = (int)o; detail->element = j; detail->isInt = 1;
                    detail->cInt = gold[j]; detail->sInt = srv[j];
                    return CTOL_VALUE;
                }
        }
        else
        {
            snprintf(field, sizeof(field), o == 0 ? "outReal" : "outReal%u", o);
            TA_Real srv[CODEGEN_TOL_MAX_OUT];
            int cnt = json_get_double_array(resp, field, srv, CODEGEN_TOL_MAX_OUT);
            if( cnt != goldNb )
            { detail->output = (int)o; detail->isInt = 0; detail->srvCount = cnt; return CTOL_COUNT; }
            const TA_Real *gold = (const TA_Real *)outBufs[o];
            for( int j = 0; j < goldNb; j++ )
            {
                double c = gold[j], sv = srv[j];
                double diff = fabs(c - sv);
                double t = (fabs(c) > 1.0) ? tol * fabs(c) : tol;
                if( (isnan(c) != isnan(sv)) || diff > t )
                {
                    detail->output = (int)o; detail->element = j; detail->isInt = 0;
                    detail->cReal = c; detail->sReal = sv;
                    return CTOL_VALUE;
                }
            }
        }
    }
    return CTOL_MATCH;
}

/* Send req to one server; reopen once if it died (a dead server that cannot be
 * recovered marks itself closed so the run fails loudly — never a false green). */
static int xlang_call(XlangServer *s, const char *req, char *resp)
{
    if( !s->open ) return 0;
    if( codegen_pipe_call(&s->cp, req, resp, JSON_BUF_SIZE) == TA_TEST_PASS ) return 1;
    s->restarts++;
    codegen_pipe_close(&s->cp);
    if( codegen_pipe_open(&s->cp, s->argv) != TA_TEST_PASS ) { s->open = 0; return 0; }
    if( codegen_pipe_call(&s->cp, req, resp, JSON_BUF_SIZE) == TA_TEST_PASS ) return 1;
    s->open = 0;
    return 0;
}

/* Golden input hash: hash the seed-generated O,H,L,C,V,OI arrays in order —
 * byte-identical to each server's fuzz_in_hash handler. */
static unsigned long long xlang_in_hash_local(int shape, int seed, int n)
{
    fuzz_gen(shape, seed, n,
             g_fzBuf[0], g_fzBuf[1], g_fzBuf[2], g_fzBuf[3], g_fzBuf[4], g_fzBuf[5]);
    unsigned long long h = fuzz_hash_init();
    for( int a = 0; a < 6; a++ )
        h = fuzz_hash_bytes(h, g_fzBuf[a], (unsigned long)n * sizeof(double));
    return fuzz_hash_fin(h);
}

/* Phase 1: prove every SEED-transport server's ported fuzz_gen reproduces the C
 * inputs exactly. A divergence here would surface below as an output mismatch,
 * but reporting it as an INPUT mismatch isolates a generator-port bug from a real
 * indicator bug. Hex-transport servers (Java) receive the driver's exact arrays,
 * so they have no fuzz_gen port to self-check — they are skipped here. Returns
 * the number of input-port mismatches (0 = all seed ports bit-identical). */
static int xlang_selfcheck_inputs(XlangCtx *ctx)
{
    static const int sizes[] = {40, 120, 240};
    static const int seeds[] = {1, 2, 3};
    int fails = 0;
    printf("Input-port self-check (fuzz_gen parity)...\n");
    for( int shape = 0; shape < FUZZ_NSHAPES; shape++ )
    for( int si = 0; si < (int)(sizeof(seeds)/sizeof(seeds[0])); si++ )
    for( int zi = 0; zi < (int)(sizeof(sizes)/sizeof(sizes[0])); zi++ )
    {
        int n = sizes[zi]; if( n > FUZZ_MAXN ) n = FUZZ_MAXN;
        unsigned long long gold = xlang_in_hash_local(shape, seeds[si], n);
        snprintf(ctx->reqBuf, JSON_BUF_SIZE,
                 "{\"method\":\"fuzz_in_hash\",\"params\":{"
                 "\"gen_shape\":%d,\"gen_seed\":%d,\"gen_n\":%d}}", shape, seeds[si], n);
        for( int s = 0; s < ctx->nsv; s++ )
        {
            XlangServer *sv = &ctx->sv[s];
            if( !sv->open || !sv->usesSeed ) continue;   /* hex servers send inputs */
            if( !xlang_call(sv, ctx->reqBuf, ctx->respBuf) )
            {
                printf("  INPUT PIPE FAIL [%s] shape=%d seed=%d n=%d\n",
                       sv->display, shape, seeds[si], n);
                fails++; ctx->error = TA_CODEGEN_PIPE_READ_FAILED; continue;
            }
            int present = 0;
            unsigned long long ih = xlang_parse_hash(ctx->respBuf, "in_hash", &present);
            if( !present )
            {
                printf("  INPUT PROTOCOL MISSING [%s]: no fuzz_in_hash support "
                       "(server out of date?)\n", sv->display);
                fails++; ctx->error = TA_CODEGEN_OUTPUT_MISMATCH; continue;
            }
            if( ih != gold )
            {
                printf("  INPUT PORT MISMATCH [%s] shape=%d seed=%d n=%d: "
                       "server=%016llx golden=%016llx — the ported fuzz_gen "
                       "diverges from fuzz_data.h (fix before trusting outputs)\n",
                       sv->display, shape, seeds[si], n, ih, gold);
                fails++;
            }
        }
    }
    if( !fails ) printf("  input-port parity: OK (all seed servers reproduce fuzz_gen bit-for-bit)\n");
    return fails;
}

/* HT_DCPHASE and HT_SINE derive their output from atan2 of the Hilbert
 * transform's in-phase/quadrature components. On a zero-variance signal
 * (FUZZ_CONSTANT: flat O=H=L=C) those components are floating-point noise (~0),
 * so the phase is atan2(≈0,≈0) — chaotically sensitive to the last bit of every
 * transcendental step. C and Rust share the system libm and stay BIT-IDENTICAL
 * there (0 mismatches), but Java's fdlibm differs by ~1 ULP and this
 * ill-conditioning amplifies that to whole degrees. It is not a codegen
 * divergence — every non-degenerate shape agrees within the 1e-9 tolerance, and
 * atan2 of a null signal is mathematically undefined — so no fixed tolerance can
 * separate it from fdlibm noise. The Java leg skips exactly these two functions
 * on exactly the constant shape (reported as a skip count for transparency);
 * every other shape, function, and language stays fully gated. */
static int xlang_illcond(const char *name, int shape)
{
    return shape == FUZZ_CONSTANT &&
           (strcmp(name, "HT_DCPHASE") == 0 || strcmp(name, "HT_SINE") == 0);
}

/* Build a per-function TA_<name> request with LOSSLESS hex-bits inputs (the
 * server_verify transport, #115) for servers without a fuzz_gen port (Java,
 * #114): the driver already holds the exact seed-generated arrays, so it
 * serializes them directly rather than asking the server to regenerate. When
 * wantHash, the request carries want_hash so the server returns out_hash for the
 * bitwise path; otherwise it returns %.15g arrays (the Java-transcendental
 * tolerance path). Inputs map exactly as setup_inputs / build_json_request:
 * single or real0 = close, real1 = volume, price = OHLCV per flags. */
static void xlang_build_hex_request(char *buf, const TA_FuncInfo *fi,
                                    const TA_History *hist, int nbBars,
                                    int s, int e, const double *optVals,
                                    int unstPeriod, int wantHash)
{
    int pos = codegen_appendf(buf, JSON_BUF_SIZE, 0,
        "{\"method\":\"TA_%s\",\"params\":{\"startIdx\":%d,\"endIdx\":%d",
        fi->name, s, e);

    int totalRealInputs = 0;
    for( unsigned int i = 0; i < fi->nbInput; i++ )
    {
        const TA_InputParameterInfo *ii;
        TA_GetInputParameterInfo(fi->handle, i, &ii);
        if( ii->type == TA_Input_Real ) totalRealInputs++;
    }

    int realCount = 0;
    for( unsigned int i = 0; i < fi->nbInput; i++ )
    {
        const TA_InputParameterInfo *ii;
        TA_GetInputParameterInfo(fi->handle, i, &ii);
        if( ii->type == TA_Input_Price )
        {
            const struct { TA_InputFlags flag; const char *key; const TA_Real *data; } comp[] = {
                { TA_IN_PRICE_OPEN,         "inOpen",         hist->open },
                { TA_IN_PRICE_HIGH,         "inHigh",         hist->high },
                { TA_IN_PRICE_LOW,          "inLow",          hist->low },
                { TA_IN_PRICE_CLOSE,        "inClose",        hist->close },
                { TA_IN_PRICE_VOLUME,       "inVolume",       hist->volume },
                { TA_IN_PRICE_OPENINTEREST, "inOpenInterest", hist->openInterest },
            };
            for( int c = 0; c < 6; c++ )
                if( ii->flags & comp[c].flag )
                {
                    pos = codegen_appendf(buf, JSON_BUF_SIZE, pos, ",\"%s\":", comp[c].key);
                    pos = codegen_write_hexbits_array(buf, JSON_BUF_SIZE, pos,
                                                       comp[c].data, nbBars);
                }
        }
        else if( ii->type == TA_Input_Real )
        {
            const TA_Real *data = (realCount == 0) ? hist->close
                                : (realCount == 1) ? hist->volume : hist->close;
            if( totalRealInputs == 1 )
                pos = codegen_appendf(buf, JSON_BUF_SIZE, pos, ",\"inReal\":");
            else
                pos = codegen_appendf(buf, JSON_BUF_SIZE, pos, ",\"inReal%d\":", realCount);
            pos = codegen_write_hexbits_array(buf, JSON_BUF_SIZE, pos, data, nbBars);
            realCount++;
        }
    }

    for( unsigned int i = 0; i < fi->nbOptInput; i++ )
    {
        const TA_OptInputParameterInfo *oi;
        TA_GetOptInputParameterInfo(fi->handle, i, &oi);
        if( oi->type == TA_OptInput_RealRange || oi->type == TA_OptInput_RealList )
            pos = codegen_appendf(buf, JSON_BUF_SIZE, pos, ",\"%s\":%.15g", oi->paramName, optVals[i]);
        else
            pos = codegen_appendf(buf, JSON_BUF_SIZE, pos, ",\"%s\":%d", oi->paramName, (int)optVals[i]);
    }

    if( fi->flags & TA_FUNC_FLG_UNST_PER )
        pos = codegen_appendf(buf, JSON_BUF_SIZE, pos, ",\"unstablePeriod\":%d", unstPeriod);
    if( wantHash )
        pos = codegen_appendf(buf, JSON_BUF_SIZE, pos, ",\"want_hash\":1");
    codegen_appendf(buf, JSON_BUF_SIZE, pos, "}}");
}

/* Load one parameter vector into the ta_abstract holder (shared by the batch
 * leg, the sentinel's explicit-default re-run, and the lookback leg). */
static void xlang_set_opt_params(TA_ParamHolder *paramHolder, const TA_FuncInfo *fi,
                                 const double *vals)
{
    for( unsigned int i = 0; i < fi->nbOptInput && i < FUZZ_MAX_OPT; i++ )
    {
        const TA_OptInputParameterInfo *oi;
        TA_GetOptInputParameterInfo(fi->handle, i, &oi);
        if( oi->type == TA_OptInput_RealRange || oi->type == TA_OptInput_RealList )
            TA_SetOptInputParamReal(paramHolder, i, vals[i]);
        else
            TA_SetOptInputParamInteger(paramHolder, i, (int)vals[i]);
    }
}

/* Print one parameter vector for a diagnostic line. */
static void xlang_print_params(const TA_FuncInfo *fi, const double *vals)
{
    for( unsigned int q = 0; q < fi->nbOptInput; q++ )
    {
        const TA_OptInputParameterInfo *oi;
        TA_GetOptInputParameterInfo(fi->handle, q, &oi);
        printf(" %s=%.15g", oi->paramName, vals[q]);
    }
}

/* ---- Lookback tier (issue #148) --------------------------------------------
 * The lookback entry point validates the same optional parameters as the batch
 * call, from its own copy of the check, and a backend can therefore diverge at
 * one tier and not the other. #148 was missing at BOTH: the Rust backend's
 * single validation helper feeds lookback, batch and stream, so an absent arm
 * silently un-gated all of them. This leg compares the lookback each language
 * reports for the SAME parameter vector, including the out-of-range vectors —
 * where the answer must be "rejected", not a number.
 *
 * Lookback is a pure function of the optional parameters, so it is swept once
 * per parameter vector rather than once per (shape, seed, size, subrange).
 * `abstract_get_lookback` is the RPC every generated server already implements
 * (test_abstract.c drives it at default parameters). */
static void xlang_build_lookback_request(char *buf, const TA_FuncInfo *fi,
                                         const double *optVals)
{
    int pos = codegen_appendf(buf, JSON_BUF_SIZE, 0,
        "{\"method\":\"abstract_get_lookback\",\"params\":{\"funcName\":\"%s\"", fi->name);
    for( unsigned int i = 0; i < fi->nbOptInput; i++ )
    {
        const TA_OptInputParameterInfo *oi;
        TA_GetOptInputParameterInfo(fi->handle, i, &oi);
        if( oi->type == TA_OptInput_RealRange || oi->type == TA_OptInput_RealList )
            pos = codegen_appendf(buf, JSON_BUF_SIZE, pos, ",\"%s\":%.15g", oi->paramName, optVals[i]);
        else
            pos = codegen_appendf(buf, JSON_BUF_SIZE, pos, ",\"%s\":%d", oi->paramName, (int)optVals[i]);
    }
    codegen_appendf(buf, JSON_BUF_SIZE, pos, "}}");
}

/* Normalize a server's `lookback` reply to the C convention: >= 0 is a real
 * lookback, -1 means "parameters rejected". C and Java return -1 directly; the
 * Rust crate's `<fn>_lookback` returns `usize::MAX`, which prints as a value far
 * above any representable TA lookback — so "negative, or above INT_MAX" is the
 * usize-width-independent invalid test rather than a hardcoded 2^64-1.
 * *present = 0 when the field is absent (server error / unknown method). */
static long long xlang_lookback_norm(const char *resp, int *present)
{
    int len;
    const char *v = json_find_field(resp, "lookback", &len);
    if( !v ) { if( present ) *present = 0; return -1; }
    if( present ) *present = 1;
    if( *v == '"' ) v++;
    if( *v == '-' ) return -1;
    unsigned long long u = strtoull(v, NULL, 10);
    return (u > (unsigned long long)INT_MAX) ? -1 : (long long)u;
}

/* One lookback-tier sweep for a function: every parameter vector, every server. */
static void xlang_lookback_leg(const TA_FuncInfo *funcInfo, XlangCtx *ctx,
                               TA_ParamHolder *paramHolder,
                               const double vec[FUZZ_MAX_VEC][FUZZ_MAX_OPT],
                               const char *kind, int nvec)
{
    /* Match the batch leg's ambient state so the two tiers are comparable and
     * the servers' unstable period (0 at spawn) agrees with ours. */
    TA_SetUnstablePeriod(TA_FUNC_UNST_ALL, 0);

    /* vec[0] is the all-defaults vector by construction — the value a sentinel
     * must resolve to. */
    xlang_set_opt_params(paramHolder, funcInfo, vec[0]);
    TA_Integer defRaw = -1;
    if( TA_GetLookback(paramHolder, &defRaw) != TA_SUCCESS ) defRaw = -1;
    long long defLb = (defRaw < 0) ? -1 : (long long)defRaw;

    for( int k = 0; k < nvec; k++ )
    {
        xlang_set_opt_params(paramHolder, funcInfo, vec[k]);

        TA_Integer goldRaw = -1;
        if( TA_GetLookback(paramHolder, &goldRaw) != TA_SUCCESS ) goldRaw = -1;
        long long gold = (goldRaw < 0) ? -1 : (long long)goldRaw;

        /* Self-check, same contract as the batch leg: an out-of-range vector the
         * C lookback ACCEPTS is not out of range, and any parity we assert on it
         * is vacuous. */
        if( kind[k] == FUZZ_VEC_REJECT && gold >= 0 )
        {
            ctx->oorNotRejected++;
            if( ctx->reportedThisFunc < 3 )
            {
                ctx->reportedThisFunc++;
                printf("  XLANG OUT-OF-RANGE ACCEPTED BY C (lookback) TA_%s: lookback=%lld  params:",
                       funcInfo->name, gold);
                xlang_print_params(funcInfo, vec[k]);
                printf("\n");
            }
        }
        /* Sentinel self-check: `<param> = TA_*_DEFAULT` must give the SAME
         * lookback as the explicit default. If C ever stopped substituting, the
         * cross-language comparison below would still pass (both sides equally
         * wrong), so this is what keeps the sentinel leg honest. */
        if( kind[k] == FUZZ_VEC_SENTINEL && gold != defLb )
        {
            ctx->sentNotDefault++;
            if( ctx->reportedThisFunc < 3 )
            {
                ctx->reportedThisFunc++;
                printf("  XLANG SENTINEL != DEFAULT IN C (lookback) TA_%s: lookback %lld "
                       "with the sentinel vs %lld with the explicit default  params:",
                       funcInfo->name, gold, defLb);
                xlang_print_params(funcInfo, vec[k]);
                printf("\n");
            }
        }

        xlang_build_lookback_request(ctx->reqBuf, funcInfo, vec[k]);

        /* Derived once per vector, not per server (issue #162). */
        int enumSent = ( kind[k] == FUZZ_VEC_SENTINEL )
                       && xlang_sentinel_on_choice_list(funcInfo, vec[k]);

        for( int sIdx = 0; sIdx < ctx->nsv; sIdx++ )
        {
            XlangServer *sv = &ctx->sv[sIdx];
            if( !sv->open ) continue;
            /* The request is never SENT to a server that cannot represent the
             * value: Java's MAType.values()[Integer.MIN_VALUE] throws out of an
             * uncaught handler and takes the subprocess with it. */
            if( enumSent && !sv->enumSentinel ) { ctx->sentEnumSkipped++; continue; }
            if( !xlang_call(sv, ctx->reqBuf, ctx->respBuf) )
            {
                if( ctx->error == TA_TEST_PASS ) ctx->error = TA_CODEGEN_PIPE_READ_FAILED;
                sv->mism++;
                continue;
            }
            int present = 0;
            long long srv = xlang_lookback_norm(ctx->respBuf, &present);
            sv->cases++;
            ctx->lbCases++;
            if( kind[k] == FUZZ_VEC_REJECT )   ctx->lbOorCases++;
            if( kind[k] == FUZZ_VEC_SENTINEL ) ctx->lbSentCases++;
            if( enumSent )                     ctx->lbSentEnumCases++;

            if( !present )
            {
                printf("  XLANG LOOKBACK PROTOCOL MISSING [%s] TA_%s: response has no "
                       "lookback field (%.120s)\n", sv->display, funcInfo->name, ctx->respBuf);
                if( ctx->error == TA_TEST_PASS ) ctx->error = TA_CODEGEN_OUTPUT_MISMATCH;
                sv->mism++;
                continue;
            }
            if( srv != gold )
            {
                sv->mism++;
                if( ctx->reportedThisFunc < 3 )
                {
                    ctx->reportedThisFunc++;
                    printf("  XLANG LOOKBACK MISMATCH TA_%s  C(golden) vs %s%s  params:",
                           funcInfo->name, sv->display,
                           kind[k] == FUZZ_VEC_REJECT
                               ? "  [out-of-range vector: both tiers must REJECT]"
                           : kind[k] == FUZZ_VEC_SENTINEL
                               ? "  [sentinel vector: both tiers must resolve it to the default]"
                               : "");
                    xlang_print_params(funcInfo, vec[k]);
                    printf("\n    lookback %lld/%lld (golden/%s; -1 = parameters rejected)\n",
                           gold, srv, sv->display);
                }
            }
        }
    }
}

/* Issue ONE hash-mode call for `optVals` against `sv` and parse the reply
 * (retCode / outBegIdx / outNBElement / out_hash). Returns 0 on a pipe failure
 * or a reply with no out_hash (the caller reports and fails). */
static int xlang_hash_call(XlangCtx *ctx, XlangServer *sv, const TA_FuncInfo *fi,
                           const TA_History *hist, int n, int s, int e,
                           int shape, int seed, const double *optVals,
                           int unstPeriod, XHashParsed *out)
{
    /* A non-zero unstable period cannot ride the seed transport: abstract_call
     * carries no FuncUnstId, and no driver has ever sent one, so the Rust
     * handler ignores the field outright and the C one would apply it to id 0
     * (TA_FUNC_UNST_ADX) whatever function was called. The per-function method
     * hardcodes the right id in its generated handler, so the unstable legs
     * take the hex transport on every server. */
    if( sv->usesSeed && unstPeriod == 0 )
        fuzz_build_request(ctx->reqBuf, fi, s, e, shape, seed, n, optVals, 0);
    else
        xlang_build_hex_request(ctx->reqBuf, fi, hist, n, s, e, optVals, unstPeriod, 1);

    if( !xlang_call(sv, ctx->reqBuf, ctx->respBuf) )
    {
        if( ctx->error == TA_TEST_PASS ) ctx->error = TA_CODEGEN_PIPE_READ_FAILED;
        return 0;
    }
    int present = 0;
    out->rc        = json_get_int(ctx->respBuf, "retCode");
    out->begIdx    = json_get_int(ctx->respBuf, "outBegIdx");
    out->nbElement = json_get_int(ctx->respBuf, "outNBElement");
    out->hash      = xlang_parse_hash(ctx->respBuf, "out_hash", &present);
    return present;
}

/* Per-function bitwise comparison: golden in-process C vs each server. */
static void xlang_one_function(const TA_FuncInfo *funcInfo, void *opaqueData)
{
    XlangCtx *ctx = (XlangCtx *)opaqueData;
    unsigned int i;

    if( ctx->error != TA_TEST_PASS ) return;
    if( !codegen_matches_filter(ctx->functionFilter, funcInfo->name) ) return;
    ctx->funcsSwept++;

    /* See fuzz_one_function: a silent skip would remove this function from the
     * cross-language bitwise gate with no trace. Fail loudly instead. */
    if( funcInfo->nbOptInput > FUZZ_MAX_OPT )
    {
        printf("XLANG PARAM OVERFLOW [TA_%s]: %u opt params > FUZZ_MAX_OPT (%d) — "
               "raise the cap; skipping would make this function's gate vacuous\n",
               funcInfo->name, funcInfo->nbOptInput, FUZZ_MAX_OPT);
        ctx->funcsWithFailures++;
        ctx->error = TA_CODEGEN_OUTPUT_MISMATCH;
        return;
    }

    for( i = 0; i < funcInfo->nbInput; i++ )
    {
        const TA_InputParameterInfo *ii;
        TA_GetInputParameterInfo(funcInfo->handle, i, &ii);
        if( ii->type == TA_Input_Integer ) return;   /* no test data */
    }

    TA_ParamHolder *paramHolder = NULL;
    if( TA_ParamHolderAlloc(funcInfo->handle, &paramHolder) != TA_SUCCESS ) return;

    TA_History hist;
    memset(&hist, 0, sizeof(hist));
    hist.open = g_fzBuf[0]; hist.high = g_fzBuf[1]; hist.low = g_fzBuf[2];
    hist.close = g_fzBuf[3]; hist.volume = g_fzBuf[4]; hist.openInterest = g_fzBuf[5];

    CodegenRangeTestParam p;
    memset(&p, 0, sizeof(p));
    p.funcInfo = funcInfo;
    p.paramHolder = paramHolder;
    p.history = &hist;
    setup_inputs(paramHolder, funcInfo, &hist);
    setup_outputs(&p);

    double vec[FUZZ_MAX_VEC][FUZZ_MAX_OPT];
    char kind[FUZZ_MAX_VEC];
    int vecOverflow = 0;
    /* frozenOracle=0: the current-vs-current path, so the sweep also carries the
     * two contract classes flagged in `kind` (issue #148). */
    memset(kind, FUZZ_VEC_NORMAL, sizeof(kind));
    int nvec = fuzz_build_vectors(funcInfo, vec, &vecOverflow, 0, kind);
    if( vecOverflow > 0 )
    {
        printf("XLANG VECTOR OVERFLOW [TA_%s]: %d parameter value(s) dropped\n",
               funcInfo->name, vecOverflow);
        for( int s = 0; s < ctx->nsv; s++ ) ctx->sv[s].mism++;   /* fail the run */
        ctx->funcsWithFailures++;
        free_outputs(&p);
        TA_ParamHolderFree(paramHolder);
        return;
    }

    /* ---- Unstable-period axis (#116) ----
     * The parameter sweep above holds the unstable period at 0, so until now
     * nothing in this gate exercised a non-zero one: that axis was covered only
     * by the ref differential sweep, i.e. only by the frozen ta_ref_serve, and
     * would have been lost with it. XLANG_UNST_PERIOD runs FIRST and 0 LAST so
     * every function leaves the servers at the value the next one expects. */
    int unstVals[2] = { 0, 0 };
    int nUnst = 1;
    TA_FuncUnstId unstId = get_unst_id(funcInfo->name);
    if( (funcInfo->flags & TA_FUNC_FLG_UNST_PER) && unstId != TA_TEST_UNST_NONE )
    {
        /* Non-vacuity: the leg only asserts something if the unstable period
         * actually reaches this function. Lookback is the cheap observable —
         * it is what the period feeds, and a flat lookback means the flag is
         * lying, so fail loudly rather than bank a leg that compares nothing. */
        TA_Integer lbZero = -1, lbUnst = -1;
        xlang_set_opt_params(paramHolder, funcInfo, vec[0]);
        TA_SetUnstablePeriod(TA_FUNC_UNST_ALL, 0);
        if( TA_GetLookback(paramHolder, &lbZero) != TA_SUCCESS ) lbZero = -1;
        TA_SetUnstablePeriod(unstId, (unsigned int)XLANG_UNST_PERIOD);
        if( TA_GetLookback(paramHolder, &lbUnst) != TA_SUCCESS ) lbUnst = -1;
        TA_SetUnstablePeriod(TA_FUNC_UNST_ALL, 0);

        if( lbZero < 0 || lbUnst <= lbZero )
        {
            printf("  XLANG UNSTABLE PROBE VACUOUS [TA_%s]: lookback %d at unstable 0 vs "
                   "%d at %d — TA_FUNC_FLG_UNST_PER says the period reaches this function "
                   "and it does not, so its unstable leg would assert nothing\n",
                   funcInfo->name, (int)lbZero, (int)lbUnst, XLANG_UNST_PERIOD);
            for( int s = 0; s < ctx->nsv; s++ ) ctx->sv[s].mism++;
            ctx->funcsWithFailures++;
            ctx->error = TA_CODEGEN_OUTPUT_MISMATCH;
            free_outputs(&p);
            TA_ParamHolderFree(paramHolder);
            return;
        }
        unstVals[0] = XLANG_UNST_PERIOD;
        unstVals[1] = 0;
        nUnst = 2;
        ctx->unstFuncs++;
    }

    static const int sizes[] = {40, 120, 240};
    static const int seeds[] = {1, 2, 3};
    ctx->reportedThisFunc = 0;
    long long mismBefore = 0;
    for( int s = 0; s < ctx->nsv; s++ ) mismBefore += ctx->sv[s].mism;

    /* Lookback tier first — same vectors, no data needed (issue #148). */
    xlang_lookback_leg(funcInfo, ctx, paramHolder, (const double (*)[FUZZ_MAX_OPT])vec,
                       kind, nvec);

    for( int ui = 0; ui < nUnst; ui++ )
    for( int shape = 0; shape < FUZZ_NSHAPES; shape++ )
    for( int si = 0; si < (int)(sizeof(seeds)/sizeof(seeds[0])); si++ )
    for( int zi = 0; zi < (int)(sizeof(sizes)/sizeof(sizes[0])); zi++ )
    {
        const int curUnst = unstVals[ui];
        int n = sizes[zi]; if( n > FUZZ_MAXN ) n = FUZZ_MAXN;
        fuzz_gen(shape, seeds[si], n,
                 g_fzBuf[0], g_fzBuf[1], g_fzBuf[2], g_fzBuf[3], g_fzBuf[4], g_fzBuf[5]);
        hist.nbBars = (unsigned int)n;
        p.nbBars = n;

        for( int k = 0; k < nvec; k++ )
        {
            /* A rejected vector's verdict comes from the parameter values alone —
             * one dataset is all the discrimination it has. The sentinel class is
             * NOT pruned: its assertion compares outputs, which only diverge on
             * data where the wrong substituted value changes them. */
            if( kind[k] == FUZZ_VEC_REJECT && (shape || si || zi) ) continue;
            /* Same argument for the unstable axis: both contract classes assert
             * parameter VALIDATION, which runs before any unstable-period logic,
             * so repeating them at a non-zero period adds no discrimination. */
            if( kind[k] != FUZZ_VEC_NORMAL && curUnst != 0 ) continue;

            xlang_set_opt_params(paramHolder, funcInfo, vec[k]);

            /* subranges: full + two deterministic random windows (as --fuzz-064),
             * plus -- once per (function, parameter vector) -- every ordered pair
             * drawn from {0,1,2,3}.
             *
             * Those ten tight ranges are where an off-by-one in a lookback clamp
             * shows up: (0,0) asks for a single output at bar 0, and the rest walk
             * the first few bars in both endpoints. Each is a VALID call. A function
             * whose lookback is 0 must answer with endIdx-startIdx+1 values starting
             * at startIdx; one with a lookback above the range must answer SUCCESS
             * WITH NO VALUES. Never an error, never a trap, and identically in every
             * language.
             *
             * They are pinned rather than left to the two random windows, which draw
             * rsS from fuzz_sm_unit()*n and rsE above it: landing on a range this
             * tight is a coincidence, so coverage was luck-dependent and uneven.
             * Measured with a deliberate count==0 fault injected into the Rust
             * wrapper: the random windows caught it on 119 of the 174 functions,
             * these ranges on all 144 that can produce an empty result.
             *
             * Pinned to the first unstable/shape/seed/size so the cost is
             * 174 x vectors x 10 rather than a multiplier on the whole sweep --
             * these probe the clamp arithmetic, which does not vary with the data.
             * The e < s permutations are deliberately absent: that rejection is
             * emitted from one prologue template and is already gated
             * deterministically by the #180 index-range probe. */
            unsigned long long rs = 0xF0F0ULL ^ ((unsigned long long)shape<<8)
                                    ^ ((unsigned long long)seeds[si]<<16) ^ ((unsigned long long)k<<24);
            static const int TIGHT[10][2] = {
                {0,0},{0,1},{0,2},{0,3},{1,1},{1,2},{1,3},{2,2},{2,3},{3,3}
            };
            int ranges[3+10][2];
            int nranges = 3;
            ranges[0][0] = 0; ranges[0][1] = n - 1;
            for( int rr = 1; rr < 3; rr++ )
            {
                int rsS = (int)(fuzz_sm_unit(&rs) * n);
                int rsE = rsS + (int)(fuzz_sm_unit(&rs) * (n - rsS));
                if( rsS > n - 1 ) rsS = n - 1;
                if( rsE > n - 1 ) rsE = n - 1;
                if( rsE < rsS ) rsE = rsS;
                ranges[rr][0] = rsS; ranges[rr][1] = rsE;
            }
            if( ui == 0 && shape == 0 && si == 0 && zi == 0 && n >= 4 )
            {
                for( int t = 0; t < 10; t++ )
                {
                    ranges[nranges][0] = TIGHT[t][0];
                    ranges[nranges][1] = TIGHT[t][1];
                    nranges++;
                }
            }

            for( int ri = 0; ri < nranges; ri++ )
            {
                int s = ranges[ri][0], e = ranges[ri][1];
                /* Same reason: validation runs before any startIdx/endIdx logic, so
                 * the contract is subrange-independent too. */
                if( kind[k] != FUZZ_VEC_NORMAL && ri != 0 ) continue;
                TA_SetUnstablePeriod(TA_FUNC_UNST_ALL, 0);
                if( curUnst ) TA_SetUnstablePeriod(unstId, (unsigned int)curUnst);

                TA_Integer curBeg = 0, curNb = 0;
                for( unsigned int o = 0; o < funcInfo->nbOutput; o++ )
                {
                    if( p.outputIsInteger[o] )
                        TA_SetOutputParamIntegerPtr(paramHolder, o, p.outIntBufs[o]);
                    else
                        TA_SetOutputParamRealPtr(paramHolder, o, p.outRealBufs[o]);
                }
                /* Sentinel leg (issue #148): the same call with the parameter set
                 * explicitly to its default, so "the sentinel selects the default"
                 * is asserted rather than inferred. Shares the output buffers, so
                 * it must run BEFORE the real call. */
                unsigned long long defHash = 0;
                TA_RetCode defRc = TA_SUCCESS;
                TA_Integer defBeg = 0, defNb = 0;
                if( kind[k] == FUZZ_VEC_SENTINEL )
                {
                    xlang_set_opt_params(paramHolder, funcInfo, vec[0]);
                    defRc = TA_CallFunc(paramHolder, s, e, &defBeg, &defNb);
                    defHash = fuzz_hash_local(&p, (defRc == TA_SUCCESS) ? defNb : 0);
                    xlang_set_opt_params(paramHolder, funcInfo, vec[k]);
                }

                TA_RetCode curRc = TA_CallFunc(paramHolder, s, e, &curBeg, &curNb);
                unsigned long long curHash =
                    fuzz_hash_local(&p, (curRc == TA_SUCCESS) ? curNb : 0);
                if( curRc == TA_SUCCESS && curNb > 0 ) ctx->nonEmpty++;

                if( kind[k] == FUZZ_VEC_SENTINEL )
                {
                    if( curRc != defRc || curBeg != defBeg || curNb != defNb ||
                        curHash != defHash )
                    {
                        ctx->sentNotDefault++;
                        if( ctx->reportedThisFunc < 3 )
                        {
                            ctx->reportedThisFunc++;
                            printf("  XLANG SENTINEL != DEFAULT IN C TA_%s  shape=%d seed=%d "
                                   "n=%d range=[%d,%d]  params:",
                                   funcInfo->name, shape, seeds[si], n, s, e);
                            xlang_print_params(funcInfo, vec[k]);
                            printf("\n    sentinel retCode %d begIdx %d nbElem %d hash %016llx"
                                   "  vs explicit default retCode %d begIdx %d nbElem %d hash %016llx\n",
                                   (int)curRc, curBeg, curNb, curHash,
                                   (int)defRc, defBeg, defNb, defHash);
                        }
                    }
                }

                /* An out-of-range vector C ACCEPTS is not out of range, and the
                 * retCode parity below is then vacuous — fail loudly.
                 * codegen_hash_compare and codegen_compare_tol both diff retCode
                 * first, so Success where C returns TA_BAD_PARAM is a hard mismatch. */
                if( kind[k] == FUZZ_VEC_REJECT && curRc != TA_BAD_PARAM )
                {
                    ctx->oorNotRejected++;
                    if( ctx->reportedThisFunc < 3 )
                    {
                        ctx->reportedThisFunc++;
                        printf("  XLANG OUT-OF-RANGE ACCEPTED BY C TA_%s: retCode=%d "
                               "(expected TA_BAD_PARAM)  params:", funcInfo->name, (int)curRc);
                        xlang_print_params(funcInfo, vec[k]);
                        printf("\n");
                    }
                }

                /* C golden output buffers in logical order — the tolerance path
                 * (Java transcendentals) element-compares against these; the
                 * bitwise path compares curHash. */
                const void *goldBufs[MAX_OUTPUTS];
                for( unsigned int o = 0; o < funcInfo->nbOutput && o < MAX_OUTPUTS; o++ )
                    goldBufs[o] = p.outputIsInteger[o] ? (const void *)p.outIntBufs[o]
                                                       : (const void *)p.outRealBufs[o];

                ctx->comparisons++;

                /* Derived once per vector, not per server (issue #162). */
                int enumSent = ( kind[k] == FUZZ_VEC_SENTINEL )
                               && xlang_sentinel_on_choice_list(funcInfo, vec[k]);

                for( int sIdx = 0; sIdx < ctx->nsv; sIdx++ )
                {
                    XlangServer *sv = &ctx->sv[sIdx];
                    if( !sv->open ) continue;

                    /* Sentinel leg (issue #148). "TA_*_DEFAULT selects the declared
                     * default" is a property of one implementation, so assert it
                     * inside each server: sentinel vs explicit default, bit-for-bit,
                     * no tolerance needed. vec[0] already ties the default's result
                     * back to the C golden, so the chain closes. */
                    if( kind[k] == FUZZ_VEC_SENTINEL )
                    {
                        XHashParsed sent, dflt;
                        /* Not sent to a server whose typed enum cannot carry it
                         * (issue #162) — the request would kill the subprocess,
                         * and a server-side substitution would only fake it. */
                        if( enumSent && !sv->enumSentinel )
                        {
                            ctx->sentEnumSkipped++;
                            continue;
                        }
                        int okS = xlang_hash_call(ctx, sv, funcInfo, &hist, n, s, e,
                                                  shape, seeds[si], vec[k], curUnst, &sent);
                        int okD = okS && xlang_hash_call(ctx, sv, funcInfo, &hist, n, s, e,
                                                         shape, seeds[si], vec[0], curUnst, &dflt);
                        if( !okS || !okD )
                        {
                            sv->mism++;
                            if( ctx->error == TA_TEST_PASS ) ctx->error = TA_CODEGEN_OUTPUT_MISMATCH;
                            continue;
                        }
                        sv->cases++;
                        ctx->sentCases++;
                        if( enumSent ) ctx->sentEnumCases++;
                        if( sent.rc != dflt.rc || sent.begIdx != dflt.begIdx ||
                            sent.nbElement != dflt.nbElement || sent.hash != dflt.hash )
                        {
                            sv->mism++;
                            if( ctx->reportedThisFunc < 3 )
                            {
                                ctx->reportedThisFunc++;
                                printf("  XLANG SENTINEL MISMATCH TA_%s [%s]: the default "
                                       "sentinel does not select the declared default  "
                                       "shape=%d seed=%d n=%d range=[%d,%d]  params:",
                                       funcInfo->name, sv->display, shape, seeds[si], n, s, e);
                                xlang_print_params(funcInfo, vec[k]);
                                printf("\n    sentinel retCode %d begIdx %d nbElem %d hash %016llx\n"
                                       "    default  retCode %d begIdx %d nbElem %d hash %016llx\n",
                                       sent.rc, sent.begIdx, sent.nbElement, sent.hash,
                                       dflt.rc, dflt.begIdx, dflt.nbElement, dflt.hash);
                            }
                        }
                        continue;
                    }

                    /* Each server's request follows its transport. Seed servers
                     * (Rust) regenerate inputs from (shape,seed,n) via
                     * gen_present; hex servers (Java, C# — no fuzz_gen port) get
                     * the driver's exact arrays losslessly. The tolerance lane is
                     * a SEPARATE per-server flag, not implied by the transport:
                     * only Java (tolTranscendental) drops a transcendental-using
                     * call to the element compare — C# stays bitwise on all of
                     * them. */
                    int tolPath = 0;
                    /* curUnst != 0 forces the hex transport even for a seed
                     * server — see xlang_hash_call for why abstract_call cannot
                     * carry an unstable period. */
                    if( sv->usesSeed && curUnst == 0 )
                        fuzz_build_request(ctx->reqBuf, funcInfo, s, e, shape, seeds[si], n, vec[k], 0);
                    else
                    {
                        /* A rejected vector stays on the hash path even on a
                         * transcendental: want_hash returns right after the call,
                         * whereas the tolerance path serialises the outputs, and
                         * sending a rejected vector down that path killed the Java
                         * server. */
                        tolPath = sv->tolTranscendental &&
                                  kind[k] != FUZZ_VEC_REJECT &&
                                  codegen_call_is_transcendental(funcInfo->handle, vec[k],
                                                                 (int)funcInfo->nbOptInput);
                        /* Chaotic phase of a null signal — not comparable across
                         * libms. Gated on tolPath, so exactly the servers that
                         * cannot be held bitwise on transcendentals skip it
                         * (Java, and C# since run 30776189041). Rust and the C
                         * golden stay bitwise there. */
                        if( tolPath && xlang_illcond(funcInfo->name, shape) )
                        { ctx->illcondSkipped++; continue; }
                        xlang_build_hex_request(ctx->reqBuf, funcInfo, &hist, n, s, e, vec[k],
                                                curUnst, !tolPath);
                    }

                    if( !xlang_call(sv, ctx->reqBuf, ctx->respBuf) )
                    {
                        if( ctx->error == TA_TEST_PASS ) ctx->error = TA_CODEGEN_PIPE_READ_FAILED;
                        sv->mism++;
                        continue;
                    }
                    sv->cases++;
                    if( tolPath ) ctx->tolCases++;
                    if( curUnst )  ctx->unstCases++;
                    if( kind[k] == FUZZ_VEC_REJECT ) ctx->oorCases++;

                    if( tolPath )
                    {
                        CTolDetail d;
                        CTolVerdict cv = codegen_compare_tol(ctx->respBuf, funcInfo->nbOutput,
                                                             p.outputIsInteger, goldBufs,
                                                             curRc, curBeg, curNb,
                                                             CODEGEN_TRANSCENDENTAL_TOL, &d);
                        if( cv != CTOL_MATCH )
                        {
                            sv->mism++;
                            if( ctx->reportedThisFunc < 3 )
                            {
                                ctx->reportedThisFunc++;
                                printf("  XLANG TOL MISMATCH TA_%s  C(golden) vs %s  "
                                       "shape=%d seed=%d n=%d range=[%d,%d]  params:",
                                       funcInfo->name, sv->display, shape, seeds[si], n, s, e);
                                xlang_print_params(funcInfo, vec[k]);
                                printf("\n    retCode %d/%d  begIdx %d/%d  nbElem %d/%d",
                                       (int)curRc, d.rc, curBeg, d.begIdx, curNb, d.nbElement);
                                if( cv == CTOL_VALUE && !d.isInt )
                                    printf("  out%d[%d] C=%.17g server=%.17g diff=%.3g (tol %g)",
                                           d.output, d.element, d.cReal, d.sReal,
                                           fabs(d.cReal - d.sReal), CODEGEN_TRANSCENDENTAL_TOL);
                                else if( cv == CTOL_VALUE )
                                    printf("  int out%d[%d] C=%d server=%d",
                                           d.output, d.element, d.cInt, d.sInt);
                                else if( cv == CTOL_COUNT )
                                    printf("  out%d count %d/%d", d.output, curNb, d.srvCount);
                                printf("\n");
                            }
                        }
                    }
                    else
                    {
                        XHashParsed hp;
                        XHashVerdict v = codegen_hash_compare(ctx->respBuf, curRc, curBeg,
                                                              curNb, curHash, &hp);
                        if( v == XHASH_NO_HASH )
                        {
                            printf("  XLANG PROTOCOL MISSING [%s] TA_%s: response has no out_hash "
                                   "(server lacks gen_present/want_hash support?)\n", sv->display, funcInfo->name);
                            if( ctx->error == TA_TEST_PASS ) ctx->error = TA_CODEGEN_OUTPUT_MISMATCH;
                        }
                        if( v != XHASH_MATCH )
                        {
                            sv->mism++;
                            if( ctx->reportedThisFunc < 3 )
                            {
                                ctx->reportedThisFunc++;
                                printf("  XLANG MISMATCH TA_%s  C(golden) vs %s%s  "
                                       "shape=%d seed=%d n=%d range=[%d,%d]  params:",
                                       funcInfo->name, sv->display,
                                       kind[k] == FUZZ_VEC_REJECT
                                           ? "  [out-of-range vector: both must REJECT]" : "",
                                       shape, seeds[si], n, s, e);
                                xlang_print_params(funcInfo, vec[k]);
                                printf("\n");
                                codegen_hash_report(sv->display, curRc, curBeg, curNb, curHash, &hp);
                            }
                        }
                    }
                }
            }
        }
    }

    long long mismAfter = 0;
    for( int s = 0; s < ctx->nsv; s++ ) mismAfter += ctx->sv[s].mism;
    if( mismAfter > mismBefore ) ctx->funcsWithFailures++;

    free_outputs(&p);
    TA_ParamHolderFree(paramHolder);
}

ErrorNumber xlang_hash(const char *functionFilter, const char *languageFilter)
{
    printf("\n=============================================\n");
    printf("Cross-language BITWISE parity gate (--xlang-hash)\n");
    printf("=============================================\n");

    /* Each generated language server, diffed against the in-process C golden (C
     * is the golden, not a server row). Rust uses the seed transport
     * (gen_present + fuzz_in_hash); Java and the managed C# use the lossless
     * hex-bits transport (usesSeed=0 — no fuzz_gen port). Java and C# both
     * relax their transcendental-using calls to the 1e-9 element compare
     * (tolTranscendental=1) for different reasons: Java's fdlibm is not the C
     * libm, and .NET does not guarantee `Math.*` reaches the platform libm.
     * Every non-transcendental call in both stays bitwise. Rust reaches the
     * same libm as the golden and is bitwise throughout. */
    static XlangServer servers[] = {
        {"rust",   "Rust", argv_rust,   1, 0, 0, {0}, 0, 0, 0, 0},
        {"java",   "Java", argv_java,   0, 0, 0, {0}, 0, 0, 0, 0},
        {"csharp", "C#",   argv_csharp, 0, 0, 0, {0}, 0, 0, 0, 0},
    };
    int nsv = (int)(sizeof(servers)/sizeof(servers[0]));

    /* tolTranscendental is FILLED IN from the shared predicate rather than
     * written per row above: server_verify.c applies the same rule on its own
     * path, and when the two were separate literals they disagreed — C# was
     * bitwise here and Java-only there. One definition, both gates.
     * enumSentinel comes from its own predicate for the same reason. */
    for( int s = 0; s < nsv; s++ )
    {
        servers[s].tolTranscendental =
            codegen_lang_needs_transcendental_tol(servers[s].name);
        servers[s].enumSentinel =
            codegen_lang_can_pass_enum_sentinel(servers[s].name);
    }

    XlangCtx ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.functionFilter = functionFilter;
    ctx.sv = servers;
    ctx.nsv = nsv;
    ctx.reqBuf = malloc(JSON_BUF_SIZE);
    ctx.respBuf = malloc(JSON_BUF_SIZE);
    ctx.error = TA_TEST_PASS;
    if( !ctx.reqBuf || !ctx.respBuf )
    { free(ctx.reqBuf); free(ctx.respBuf); return TA_CODEGEN_ALLOC_FAILED; }

    int nopen = 0, nrequested = 0;
    for( int s = 0; s < nsv; s++ )
    {
        XlangServer *sv = &servers[s];
        if( languageFilter && !language_matches_filter(languageFilter, sv->name) )
        { sv->open = 0; continue; }
        nrequested++;
        if( codegen_pipe_open(&sv->cp, sv->argv) == TA_TEST_PASS )
        { sv->open = 1; nopen++; printf("server up: %-4s (pid=%d)\n", sv->display, sv->cp.child_pid); }
        else
        {
            printf("FAILED to start %s server (%s). Build the servers first "
                   "(scripts/build.py servers / xlang-hash).\n", sv->display, sv->argv[0]);
            sv->open = 0;
            ctx.error = TA_CODEGEN_ALLOC_FAILED;
        }
    }
    printf("\n");

    if( nrequested == 0 )
    {
        printf("FAIL — no language server matched --language=%s "
               "(valid: rust, java, csharp; C is the in-process golden).\n",
               languageFilter ? languageFilter : "");
        free(ctx.reqBuf); free(ctx.respBuf);
        return TA_CODEGEN_OUTPUT_MISMATCH;
    }

    int inFails = 0;
    if( ctx.error == TA_TEST_PASS )
        inFails = xlang_selfcheck_inputs(&ctx);

    if( inFails == 0 && ctx.error == TA_TEST_PASS )
    {
        printf("\nOutput parity gate (%d function(s) x shapes x seeds x sizes x params x subranges)...\n",
               codegen_function_count());
        TA_ForEachFunc(xlang_one_function, &ctx);
    }
    else if( inFails > 0 )
        printf("\nSkipping the output gate: %d input-port mismatch(es) make output "
               "hashes meaningless — fix the ported fuzz_gen first.\n", inFails);

    for( int s = 0; s < nsv; s++ )
        if( servers[s].open ) codegen_pipe_close(&servers[s].cp);

    long long totalMism = 0, totalCases = 0, totalRestarts = 0;
    printf("\n---------------------------------------------\n");
    printf("golden cases: %lld   (in-process C library; %lld with non-empty output = %.0f%% non-vacuous)\n",
           ctx.comparisons, ctx.nonEmpty,
           ctx.comparisons ? 100.0 * (double)ctx.nonEmpty / (double)ctx.comparisons : 0.0);
    for( int s = 0; s < nsv; s++ )
    {
        if( servers[s].cases == 0 && !servers[s].open && !languageFilter ) continue;
        if( servers[s].cases == 0 && languageFilter
            && !language_matches_filter(languageFilter, servers[s].name) ) continue;
        printf("%-4s: %lld cases, %lld mismatch(es)%s\n",
               servers[s].display, servers[s].cases, servers[s].mism,
               servers[s].restarts ? " (server restarted)" : "");
        totalMism += servers[s].mism;
        totalCases += servers[s].cases;
        totalRestarts += servers[s].restarts;
    }
    if( ctx.tolCases > 0 )
        printf("  (%lld tolerance-lane call(s) across Java+C# compared at the "
               "transcendental tolerance %g; every other call is bitwise)\n",
               ctx.tolCases, CODEGEN_TRANSCENDENTAL_TOL);
    if( ctx.illcondSkipped > 0 )
        printf("  (%lld HT_DCPHASE/HT_SINE call(s) skipped on the constant shape "
               "across the tolerance-lane servers: atan2 phase of a null signal, "
               "ill-conditioned across libms — C and Rust bitwise there)\n",
               ctx.illcondSkipped);
    printf("unstable period (#116): %lld case(s) at unstable %d across %lld function(s)\n",
           ctx.unstCases, XLANG_UNST_PERIOD, ctx.unstFuncs);
    printf("param contract (#148): reject %lld batch + %lld lookback, sentinel %lld batch "
           "+ %lld lookback; %lld lookback case(s) total\n",
           ctx.oorCases, ctx.lbOorCases, ctx.sentCases, ctx.lbSentCases, ctx.lbCases);
    printf("  of the sentinel cases, %lld batch + %lld lookback put it on a CHOICE-LIST "
           "parameter (#162)%s\n",
           ctx.sentEnumCases, ctx.lbSentEnumCases,
           ctx.sentEnumSkipped ? "" : " — none skipped");
    if( ctx.sentEnumSkipped > 0 )
        printf("  (%lld more NOT asked of a server whose typed enum cannot carry the "
               "sentinel — Java: MAType is a real enum and Core takes MAType, so "
               "(MAType)Integer.MIN_VALUE cannot be constructed. Type safety discharges "
               "#162 there rather than a check.)\n", ctx.sentEnumSkipped);

    free(ctx.reqBuf); free(ctx.respBuf);

    if( totalCases == 0 && inFails == 0 )
    {
        printf("FAIL — zero comparisons (over-narrow filter or servers down?).\n");
        return TA_CODEGEN_OUTPUT_MISMATCH;
    }
    /* Non-vacuity: an empty output (outNBElement==0) hashes identically on both
     * sides, so a run where NOTHING produced output would pass vacuously. Fail if
     * so — the seed shapes/sizes are chosen to always yield real output. */
    if( ctx.comparisons > 0 && ctx.nonEmpty == 0 )
    {
        printf("FAIL — VACUOUS: every golden case had empty output; the gate would "
               "match all-empty==all-empty. Check the fuzz sizes/params.\n");
        return TA_CODEGEN_OUTPUT_MISMATCH;
    }
    /* Contract-leg integrity (issue #148). An out-of-range candidate the C
     * library accepts proves the candidate — not the implementation — is wrong,
     * and every parity assertion made on it was vacuous. */
    if( ctx.oorNotRejected > 0 )
    {
        printf("FAIL — %lld out-of-range parameter vector(s) were ACCEPTED by the "
               "in-process C library. The candidates fuzz_add_out_of_range derives "
               "from the ta_abstract range no longer match what the generated code "
               "validates; the rejection-parity leg is vacuous until they agree.\n",
               ctx.oorNotRejected);
        return TA_CODEGEN_OUTPUT_MISMATCH;
    }
    /* Likewise for the sentinel: if C itself stopped resolving TA_*_DEFAULT to
     * the declared default, "both languages agree" would no longer mean the
     * contract holds. */
    if( ctx.sentNotDefault > 0 )
    {
        printf("FAIL — %lld sentinel vector(s) did not reproduce the explicit default's "
               "result in the in-process C library. TA_REAL_DEFAULT / TA_INTEGER_DEFAULT "
               "must select the declared default (src/ta_func/ta_T3.c:76-77); until it "
               "does, the sentinel-parity leg is vacuous.\n", ctx.sentNotDefault);
        return TA_CODEGEN_OUTPUT_MISMATCH;
    }
    /* Non-vacuity: with no rejected and no sentinel vector this gate is back to
     * what it was before #148 — asserting only that VALUES agree. Unfiltered runs
     * only; --function= can select functions with no range-typed param at all. */
    if( !functionFilter && ctx.comparisons > 0 &&
        (ctx.oorCases == 0 || ctx.lbOorCases == 0 ||
         ctx.sentCases == 0 || ctx.lbSentCases == 0) )
    {
        printf("FAIL — VACUOUS CONTRACT LEG: reject %lld batch / %lld lookback, sentinel "
               "%lld batch / %lld lookback — no vector any language had to reject or "
               "resolve, so the parameter contract is not gated at all.\n",
               ctx.oorCases, ctx.lbOorCases, ctx.sentCases, ctx.lbSentCases);
        return TA_CODEGEN_OUTPUT_MISMATCH;
    }
    /* The unstable-period axis needs a floor of its own for the same reason: it
     * is a strict subset of the case total, which the 148 other functions keep
     * large whether this leg runs or not. Unfiltered runs only — --function=
     * can legitimately select nothing that carries TA_FUNC_FLG_UNST_PER. */
    if( !functionFilter && ctx.comparisons > 0 && ctx.unstCases == 0 )
    {
        printf("FAIL — VACUOUS UNSTABLE LEG: not one case ran at a non-zero unstable "
               "period, so nothing here gates it. %lld function(s) carry "
               "TA_FUNC_FLG_UNST_PER; if that is now zero the flag or the sweep "
               "stopped enumerating them (#116).\n", ctx.unstFuncs);
        return TA_CODEGEN_OUTPUT_MISMATCH;
    }
    /* The choice-list sentinel needs a floor of its OWN. It is a strict subset of
     * sentCases and the range params alone keep that in the thousands, so with
     * this leg excluded the guard above stays green — which is how #162 survived.
     * Requiring an enum-capable server to be open keeps `--language=java`, whose
     * skip is legitimate, from tripping it. */
    int enumCapableOpen = 0;
    for( int s = 0; s < nsv; s++ )
        if( servers[s].open && servers[s].enumSentinel ) enumCapableOpen = 1;
    if( !functionFilter && enumCapableOpen && ctx.comparisons > 0 &&
        (ctx.sentEnumCases == 0 || ctx.lbSentEnumCases == 0) )
    {
        printf("FAIL — VACUOUS CHOICE-LIST SENTINEL LEG: %lld batch / %lld lookback case(s) "
               "put TA_INTEGER_DEFAULT on an enum:MAType parameter. That leg is what gates "
               "issue #162; with none of it running, a backend can stop substituting the "
               "declared default and every other count here stays healthy.\n",
               ctx.sentEnumCases, ctx.lbSentEnumCases);
        return TA_CODEGEN_OUTPUT_MISMATCH;
    }
    if( totalMism == 0 && inFails == 0 && ctx.error == TA_TEST_PASS )
    {
        printf("PASS — %lld function(s) swept: every server matches the in-process C "
               "library: BIT-IDENTICAL (zero tolerance), Java+C# transcendentals "
               "within %g (current-vs-current, all shapes, period>=2).\n",
               ctx.funcsSwept, CODEGEN_TRANSCENDENTAL_TOL);
        return TA_TEST_PASS;
    }
    printf("FAIL — %lld output mismatch(es) + %d input-port mismatch(es) across %d function(s).\n",
           totalMism, inFails, ctx.funcsWithFailures);
    return ctx.error != TA_TEST_PASS ? ctx.error : TA_CODEGEN_OUTPUT_MISMATCH;
}

/* ------------------------------------------------------------------------ *
 * Non-vacuity guard for candlestick pattern coverage (issue #109).
 *
 * A candlestick differential/stream test passes VACUOUSLY when the input data
 * never triggers the pattern: every output is 0, and all-zero == all-zero holds
 * regardless of the implementation. Both the fuzz-064 (current vs frozen v0.6.4)
 * and stream_verify (stream vs batch) gates draw their inputs from fuzz_gen()
 * shapes, so a pattern that fires on NO shape has its bit-exactness asserted
 * against all-zero output — its real decision logic goes unverified.
 *
 * This guard asserts that EVERY candlestick fires at least one non-zero output
 * on the shapes the candlestick sweeps actually run — the "stream shape set"
 * {RANDWALK, CONSTANT, TIE_HEAVY, EXTREME, WITH_ZEROS, CANDLE, ZEROSUM} — at
 * BOTH the stream size (240) and the guard's larger size (512). MONO_UP /
 * MONO_DOWN are excluded because the candlestick stream legs skip them, so a
 * pattern firing only there would still be stream-vacuous. The pattern-rich
 * FUZZ_CANDLE shape (fuzz_data.h) is grown with deterministic per-family windows
 * so the rarer multi-candle patterns fire; this guard fails loudly if any
 * non-exempt pattern's coverage is (or becomes) vacuous.
 *
 * cdl_pending[] lists patterns not yet covered by a deterministic window
 * (tracked in issue #109); they are exempt from the assertion and the list
 * shrinks to empty as each family's window lands. The Hikkake pair additionally
 * keeps the stronger 4-output-class check (detection AND +/-200 confirmation,
 * both directions); the MC/DC gate in test_candlestick.c is the independent
 * backstop for the pattern logic itself. */

/* Shapes the candlestick stream/fuzz sweeps actually run for a pattern. */
static const int CDL_STREAM_SHAPES[] =
    { FUZZ_RANDWALK, FUZZ_CONSTANT, FUZZ_TIE_HEAVY, FUZZ_EXTREME,
      FUZZ_WITH_ZEROS, FUZZ_CANDLE, FUZZ_ZEROSUM };
#define CDL_NSTREAM_SHAPES ((int)(sizeof(CDL_STREAM_SHAPES)/sizeof(int)))

/* Patterns not yet covered by a deterministic FUZZ_CANDLE window (issue #109).
 * Exempt from the assertion until their family's window lands. The allowlist is
 * now EMPTY — every candlestick fires on a stream-run shape (the whole catalog
 * of once-vacuous families has landed). The lone NULL keeps the table valid ISO
 * C; re-add a name here (with a #109-tracked reason) only if a pattern is ever
 * found that deterministic geometry genuinely cannot trigger. */
static const char * const cdl_pending[] = {
    NULL
};
#define CDL_NPENDING ((int)(sizeof(cdl_pending)/sizeof(cdl_pending[0])))

static int cdl_is_pending(const char *name)
{
    int i;
    for( i = 0; i < CDL_NPENDING; i++ )
        if( cdl_pending[i] && strcmp(name, cdl_pending[i]) == 0 ) return 1;
    return 0;
}

/* Patterns with a deterministic FUZZ_CANDLE window (fuzz_data.h, issue #109).
 * These are held to a STRONGER standard: they must fire on FUZZ_CANDLE (shape 7)
 * specifically, not merely on some stream-run shape. That way a window that rots
 * (an edit to fuzz_data.h that stops producing the pattern) is caught even for
 * patterns that also happen to fire on the random shapes — otherwise the random
 * firing would mask the broken window. Grows as each family's window lands. */
static const char * const cdl_catalog[] = {
    "CDL2CROWS", "CDL3BLACKCROWS", "CDL3WHITESOLDIERS", "CDL3STARSINSOUTH",
    "CDL3LINESTRIKE", "CDLCONCEALBABYSWALL", "CDLMATHOLD", "CDLRISEFALL3METHODS",
    "CDLADVANCEBLOCK", "CDLINNECK", "CDLUNIQUE3RIVER",
    "CDLKICKING",
    "CDLKICKINGBYLENGTH",
    "CDLDARKCLOUDCOVER",
    "CDLPIERCING",
    "CDLTHRUSTING",
    "CDLHOMINGPIGEON",
    "CDL3INSIDE",
    "CDLIDENTICAL3CROWS",
    "CDLSTALLEDPATTERN",
    "CDLUPSIDEGAP2CROWS",
    "CDLBREAKAWAY",
    "CDLLADDERBOTTOM",
    "CDLXSIDEGAP3METHODS"
};
#define CDL_NCATALOG ((int)(sizeof(cdl_catalog)/sizeof(cdl_catalog[0])))

static int cdl_in_catalog(const char *name)
{
    int i;
    for( i = 0; i < CDL_NCATALOG; i++ )
        if( strcmp(name, cdl_catalog[i]) == 0 ) return 1;
    return 0;
}

/* Count non-zero outputs of a candlestick on one (shape,seed,n) fuzz case. */
static int cdl_fire_on(const TA_FuncHandle *handle, int shape, int seed, int n)
{
    static double o[512], h[512], l[512], c[512], vv[512], oi[512];
    static int out[512];
    TA_ParamHolder *ph; int beg = 0, nb = 0, k, cnt = 0;
    if( n > 512 ) n = 512;
    fuzz_gen(shape, seed, n, o, h, l, c, vv, oi);
    if( TA_ParamHolderAlloc(handle, &ph) != TA_SUCCESS ) return -1;
    TA_SetInputParamPricePtr(ph, 0, o, h, l, c, vv, oi);
    TA_SetOutputParamIntegerPtr(ph, 0, out);
    if( TA_CallFunc(ph, 0, n - 1, &beg, &nb) == TA_SUCCESS )
        for( k = 0; k < nb; k++ ) if( out[k] ) cnt++;
    TA_ParamHolderFree(ph);
    return cnt;
}

/* Fires on at least one stream-run shape at size n (seeds 1..6)? */
static int cdl_fires_at_size(const TA_FuncHandle *handle, int n)
{
    int si, seed;
    for( si = 0; si < CDL_NSTREAM_SHAPES; si++ )
        for( seed = 1; seed <= 6; seed++ )
            if( cdl_fire_on(handle, CDL_STREAM_SHAPES[si], seed, n) > 0 ) return 1;
    return 0;
}

/* Fires on FUZZ_CANDLE specifically at size n (seeds 1..6)? */
static int cdl_fires_candle_at_size(const TA_FuncHandle *handle, int n)
{
    int seed;
    for( seed = 1; seed <= 6; seed++ )
        if( cdl_fire_on(handle, FUZZ_CANDLE, seed, n) > 0 ) return 1;
    return 0;
}

/* Collector for TA_ForEachFunc: gather candlestick handles + names. */
typedef struct { const TA_FuncHandle *h[128]; const char *nm[128]; int n; } CdlList;
static void cdl_collect(const TA_FuncInfo *fi, void *opaque)
{
    CdlList *L = (CdlList *)opaque;
    if( (fi->flags & TA_FUNC_FLG_CANDLESTICK) && L->n < 128 )
    { L->h[L->n] = fi->handle; L->nm[L->n] = fi->name; L->n++; }
}

static ErrorNumber verify_fuzz_candle_nonvacuous(void)
{
    CdlList L; int i, failed = 0;
    L.n = 0;
    TA_ForEachFunc(cdl_collect, &L);
    for( i = 0; i < L.n; i++ )
    {
        int f240 = cdl_fires_at_size(L.h[i], 240);
        int f512 = cdl_fires_at_size(L.h[i], 512);
        if( cdl_is_pending(L.nm[i]) )
        {
            if( f240 && f512 )
                printf("NOTE: pending candlestick %s now fires on the stream "
                       "shapes — promote it out of cdl_pending[] (issue #109).\n",
                       L.nm[i]);
            continue;
        }
        if( !(f240 && f512) )
        {
            printf("CANDLE VACUOUS: %s fires on no stream-run shape "
                   "(N=240:%d N=512:%d) — its fuzz-064/stream coverage is "
                   "all-zero==all-zero. Add a deterministic FUZZ_CANDLE window "
                   "(issue #109) or list it in cdl_pending[].\n",
                   L.nm[i], f240, f512);
            failed++;
        }
        else if( cdl_in_catalog(L.nm[i]) )
        {
            /* Held to the stronger FUZZ_CANDLE-specific standard. */
            int c240 = cdl_fires_candle_at_size(L.h[i], 240);
            int c512 = cdl_fires_candle_at_size(L.h[i], 512);
            if( !(c240 && c512) )
            {
                printf("CANDLE WINDOW BROKEN: %s is listed in cdl_catalog[] but no "
                       "longer fires on FUZZ_CANDLE (N=240:%d N=512:%d) — its "
                       "deterministic window in fuzz_data.h regressed (issue #109).\n",
                       L.nm[i], c240, c512);
                failed++;
            }
        }
    }
    if( failed )
        return TA_TSTCDL_PREDICATE_VACUOUS;

    /* Stronger check for the Hikkake pair: the pattern shape must fire ALL FOUR
     * output classes (detection AND +/-200 confirmation, both directions). */
    {
        static double o[512], h[512], l[512], c[512], vv[512], oi[512];
        static int out[512];
        struct { const char *nm;
                 TA_RetCode (*fn)(int,int,const double*,const double*,const double*,const double*,int*,int*,int*); }
            F[2] = { { "CDLHIKKAKE", TA_CDLHIKKAKE }, { "CDLHIKKAKEMOD", TA_CDLHIKKAKEMOD } };
        int fi;
        for( fi = 0; fi < 2; fi++ )
        {
            int p100=0, n100=0, p200=0, n200=0, seed;
            for( seed = 1; seed <= 6; seed++ )
            {
                int bi=0, nb=0, k;
                fuzz_gen(FUZZ_CANDLE, seed, 512, o, h, l, c, vv, oi);
                if( F[fi].fn(0, 511, o, h, l, c, &bi, &nb, out) != TA_SUCCESS ) continue;
                for( k = 0; k < nb; k++ ) { int val=out[k];
                    if(val==100)p100++; else if(val==-100)n100++; else if(val==200)p200++; else if(val==-200)n200++; }
            }
            if( !(p100 && n100 && p200 && n200) )
            {
                printf("FUZZ_CANDLE VACUOUS for %s: +100=%d -100=%d +200=%d -200=%d "
                       "(the pattern shape must fire detection AND confirmation)\n",
                       F[fi].nm, p100, n100, p200, n200);
                return TA_TSTCDL_PREDICATE_VACUOUS;
            }
        }
    }
    return TA_TEST_PASS;
}

/* Guard the FUZZ_ZEROSUM data shape (fuzz_data.h) that makes the ACCBANDS
 * degenerate else branch (TA_IS_ZERO(high+low) -> upper=high, lower=low)
 * non-vacuous vs v0.6.4 and in stream_verify. It must (a) actually produce bars
 * with high+low == 0, and (b) keep ACCBANDS FINITE on them: if the else branch
 * were skipped, 4*(high-low)/(high+low) divides by zero -> inf/nan propagates
 * into every band whose window covers that bar, so the finiteness check BITES
 * (proven by neutering the else branch). Without this guard a future edit to the
 * generator could silently stop landing high+low in the 1e-14 band and the
 * differential/stream coverage of that branch would go vacuous. */
static ErrorNumber verify_fuzz_zerosum_nonvacuous(void)
{
    static double o[512], h[512], l[512], c[512], vv[512], oi[512];
    static double up[512], mid[512], low[512];
    int seed, zeroBars = 0, outBars = 0;
    for( seed = 1; seed <= 6; seed++ )
    {
        int bi = 0, nb = 0, k;
        fuzz_gen(FUZZ_ZEROSUM, seed, 512, o, h, l, c, vv, oi);
        for( k = 0; k < 512; k++ )
            if( h[k] + l[k] == 0.0 ) zeroBars++;
        if( TA_ACCBANDS(0, 511, h, l, c, 20, &bi, &nb, up, mid, low) != TA_SUCCESS )
        {
            printf("FUZZ_ZEROSUM: ACCBANDS call failed\n");
            return TA_TSTCDL_PREDICATE_VACUOUS;
        }
        for( k = 0; k < nb; k++ )
        {
            outBars++;
            if( !isfinite(up[k]) || !isfinite(mid[k]) || !isfinite(low[k]) )
            {
                printf("FUZZ_ZEROSUM: ACCBANDS non-finite band at bar %d "
                       "(the high+low==0 else branch is broken)\n", k);
                return TA_TSTCDL_PREDICATE_VACUOUS;
            }
        }
    }
    if( zeroBars == 0 || outBars == 0 )
    {
        printf("FUZZ_ZEROSUM VACUOUS: high+low==0 bars=%d, output bars=%d "
               "(the shape must land high+low in the TA_IS_ZERO band)\n",
               zeroBars, outBars);
        return TA_TSTCDL_PREDICATE_VACUOUS;
    }
    return TA_TEST_PASS;
}

/* ACCBANDS supports input==output aliasing: the fused single-loop rewrite reads
 * every current and trailing bar BEFORE it writes any band for that output
 * index, so an output buffer that aliases an input buffer is only overwritten
 * after its last read. No generic gate exercises input==output, so verify it
 * directly — each of the 3 outputs aliased onto each of the 3 inputs must
 * reproduce the separate-buffer result BIT-FOR-BIT (all 3 bands, to catch an
 * aliased write corrupting a still-needed read of another band's input). */
static ErrorNumber verify_accbands_inplace_aliasing(void)
{
    enum { AN = 300, AP = 14 };
    static double o[AN], h[AN], l[AN], c[AN], vv[AN], oi[AN];
    static double refU[AN], refM[AN], refL[AN];
    static double bh[AN], bl[AN], bc[AN], s1[AN], s2[AN];
    int bi, nb, refNb, k, op, ip;
    fuzz_gen(FUZZ_ZEROSUM, 2, AN, o, h, l, c, vv, oi);  /* mix of degenerate + normal bars */
    if( TA_ACCBANDS(0, AN - 1, h, l, c, AP, &bi, &refNb, refU, refM, refL) != TA_SUCCESS )
    {
        printf("ACCBANDS aliasing: reference call failed\n");
        return TA_TSTCDL_PREDICATE_VACUOUS;
    }
    for( op = 0; op < 3; op++ )       /* which output band aliases an input */
    for( ip = 0; ip < 3; ip++ )       /* which input it aliases */
    {
        double *in[3], *bandbuf[3], *scratch[2];
        double *ref[3];
        int si = 0;
        ref[0] = refU; ref[1] = refM; ref[2] = refL;
        for( k = 0; k < AN; k++ ) { bh[k] = h[k]; bl[k] = l[k]; bc[k] = c[k]; }
        in[0] = bh; in[1] = bl; in[2] = bc;
        scratch[0] = s1; scratch[1] = s2;
        for( k = 0; k < 3; k++ )
            bandbuf[k] = (k == op) ? in[ip] : scratch[si++];
        if( TA_ACCBANDS(0, AN - 1, in[0], in[1], in[2], AP,
                        &bi, &nb, bandbuf[0], bandbuf[1], bandbuf[2]) != TA_SUCCESS )
        {
            printf("ACCBANDS aliasing[out=%d,in=%d]: call failed\n", op, ip);
            return TA_TSTCDL_PREDICATE_VACUOUS;
        }
        for( k = 0; k < nb; k++ )
        {
            int band;
            for( band = 0; band < 3; band++ )
                if( memcmp(&bandbuf[band][k], &ref[band][k], sizeof(double)) != 0 )
                {
                    printf("ACCBANDS aliasing[out=%d,in=%d]: band %d bar %d "
                           "differs from separate-buffer result (in-place unsafe)\n",
                           op, ip, band, k);
                    return TA_TSTCDL_PREDICATE_VACUOUS;
                }
        }
    }
    return TA_TEST_PASS;
}

ErrorNumber test_codegen(const TA_History *history,
                         const char *languageFilter,
                         const char *functionFilter)
{
    ErrorNumber errNb;
    int langsTested = 0;

    printf("\n");
    printf("=============================================\n");
    printf("Codegen Multi-Language Verification\n");
    printf("=============================================\n");

    /* Reject unknown --language= tokens. The langsTested==0 check below only
     * fires when EVERY token misses, so "c,csharpp" would silently test C alone
     * and report green for a language nobody ran. */
    if( languageFilter )
    {
        char filterCopy[1024];
        char *token;
        strncpy(filterCopy, languageFilter, sizeof(filterCopy) - 1);
        filterCopy[sizeof(filterCopy) - 1] = '\0';
        token = strtok(filterCopy, ",");
        while( token != NULL )
        {
            unsigned int li;
            for( li = 0; li < NUM_LANGUAGES; li++ )
                if( strcmp(token, ALL_LANGUAGES[li].name) == 0 ) break;
            if( li == NUM_LANGUAGES )
            {
                printf("\nFAIL - unknown --language token '%s' (valid:", token);
                for( li = 0; li < NUM_LANGUAGES; li++ )
                    printf(" %s", ALL_LANGUAGES[li].name);
                printf(").\n");
                return TA_REGTEST_BAD_USER_PARAM;
            }
            token = strtok(NULL, ",");
        }
    }

    /* Non-vacuity guard for the candlestick pattern data shape. */
    errNb = verify_fuzz_candle_nonvacuous();
    if( errNb != TA_TEST_PASS )
        return errNb;

    /* Non-vacuity guard for the ACCBANDS high+low==0 degenerate branch, plus a
     * direct input==output aliasing check for the fused single-loop rewrite. */
    errNb = verify_fuzz_zerosum_nonvacuous();
    if( errNb != TA_TEST_PASS )
        return errNb;
    errNb = verify_accbands_inplace_aliasing();
    if( errNb != TA_TEST_PASS )
        return errNb;

    /* Spawn the reference oracle once; it is the shared baseline for every
     * language server, including the generated C server (reference-as-server,
     * task #7). The runner no longer computes the baseline in-process. */
    CodegenPipe refCp;
    errNb = codegen_pipe_open(&refCp, argv_cref);
    if( errNb != TA_TEST_PASS )
    {
        printf("\nFAILED: cannot start ta_ref_serve (the reference oracle).\n"
               "        Build it via scripts/regtest.py (it builds ta_ref_serve\n"
               "        from the pinned-tag reference worktree into bin/).\n");
        return errNb;
    }
    printf("Reference oracle: ta_ref_serve (pid=%d)\n", refCp.child_pid);

    for( unsigned int i = 0; i < NUM_LANGUAGES; i++ )
    {
        if( !language_matches_filter(languageFilter, ALL_LANGUAGES[i].name) )
            continue;

        errNb = test_codegen_for_language(&ALL_LANGUAGES[i], (int)i, history,
                                          functionFilter, &refCp);
        if( errNb != TA_TEST_PASS )
        {
            codegen_pipe_close(&refCp);
            return errNb;
        }

        langsTested++;
    }

    codegen_pipe_close(&refCp);

    if( langsTested == 0 )
    {
        printf("\nNo languages matched filter '%s'\n",
               languageFilter ? languageFilter : "(none)");
        return TA_REGTEST_BAD_USER_PARAM;
    }

    print_timing_table(languageFilter);

    /* Non-vacuity for the float leg: it compares a language's single-precision
     * entry point against its own double one, and a server that silently ignored
     * "use_float" would compare the double result with itself and pass. The
     * acknowledgment is checked per call; this is the run-level floor. */
    if( g_floatCapableLangTested && g_floatLegCompared == 0 )
    {
        printf("\nCODEGEN FAILED: the float leg compared nothing — no server "
               "acknowledged use_float on any function\n");
        return TA_CODEGEN_OUTPUT_MISMATCH;
    }

    /* Non-vacuity for the default-sentinel pass (#170), PER LANGUAGE. A total
     * would stay green while one server answered every sentinel request with an
     * error and compared nothing, because the others keep it non-zero — and one
     * server silently opting out is exactly the shape of the hole this closes.
     * `eligible` (functions that reached the pass with a sentinel-able
     * parameter) rather than a bare count, so a --function= filter naming only
     * parameterless functions is a legitimate zero rather than a failure. */
    {
        long sentinelTotal = 0, sentinelEligible = 0;
        for( unsigned int li = 0; li < NUM_LANGUAGES; li++ )
        {
            if( g_floatSentinelEligible[li] > 0 && g_floatSentinelWithOutput[li] == 0 )
            {
                printf("\nCODEGEN FAILED: the default-sentinel float leg compared "
                       "no output for %s — %ld function(s) reached it, %ld produced an "
                       "acknowledged comparison, none of them any output element\n",
                       ALL_LANGUAGES[li].display, g_floatSentinelEligible[li],
                       g_floatSentinelCompared[li]);
                return TA_CODEGEN_OUTPUT_MISMATCH;
            }
            sentinelTotal    += g_floatSentinelWithOutput[li];
            sentinelEligible += g_floatSentinelEligible[li];
        }

        /* Non-vacuity for the startIdx axis (#236 step 2), on the same terms.
         * It is not gated on `functionFilter`: every shipped function reaches
         * `run_edge_range_sweep`, and every one of them has at least one legal
         * (startIdx > 0, endIdx) pair in a 252-bar series, so a language line
         * reading zero means the pairs stopped being sent — not that the filter
         * chose badly. `withOutput` rather than `compared`, so a server that
         * answered every one of them "success, zero elements" is not counted as
         * having verified anything. */
        for( unsigned int li = 0; li < NUM_LANGUAGES; li++ )
        {
            if( g_startSweepCompared[li] > 0 && g_startSweepWithOutput[li] == 0 )
            {
                printf("\nCODEGEN FAILED: the startIdx-axis sweep compared no "
                       "output element for %s — %ld range(s) reached it and every "
                       "one produced nothing\n",
                       ALL_LANGUAGES[li].display, g_startSweepCompared[li]);
                return TA_CODEGEN_OUTPUT_MISMATCH;
            }
        }
        {
            long startSweepTotal = 0, valuesCompared = 0;
            for( unsigned int li = 0; li < NUM_LANGUAGES; li++ )
            {
                startSweepTotal += g_startSweepWithOutput[li];
                valuesCompared  += g_codegenCompared[li];
            }
            if( valuesCompared > 0 && startSweepTotal == 0 )
            {
                printf("\nCODEGEN FAILED: the startIdx-axis sweep compared "
                       "nothing anywhere, on a run that did compare values — "
                       "every server-facing call is back at startIdx 0\n");
                return TA_CODEGEN_OUTPUT_MISMATCH;
            }
        /* ---- Return-code census (#236 step 4) ----
         * Printed per language, and floored on the codes this corpus is known to
         * reach. The floor is a LIST, not a total: a server that stopped
         * answering one of them entirely is invisible in a sum the others keep
         * non-zero, and that is precisely the shape of a normalisation layer
         * that has quietly collapsed onto one code.
         *
         * AllocErr, InternalError and InsufficientHistory are NOT floored here
         * and are named rather than silently omitted: the first two are
         * unreachable from the managed batch tier (an allocation failure
         * terminates the process, #178), and the third is streaming-only, which
         * this leg does not drive. `other` must stay ZERO — a code outside the
         * documented function-tier set reaching the wire is a defect however
         * often it happens. */
        {
            static const int FLOORED[] = { 0, 2, 12, 13 };   /* Success, BadParam, both index codes */
            unsigned int li, b, fi;
            int anyLang = 0;
            for( li = 0; li < NUM_LANGUAGES; li++ )
            {
                long total = 0;
                for( b = 0; b < RC_BUCKETS; b++ ) total += g_retCodeSeen[li][b];
                if( total == 0 )
                    continue;               /* language not run */
                anyLang = 1;
                printf("  retCode census [%s]:", ALL_LANGUAGES[li].display);
                for( b = 0; b < RC_BUCKETS; b++ )
                    if( g_retCodeSeen[li][b] > 0 )
                        printf(" %s=%ld", RC_NAME[b], g_retCodeSeen[li][b]);
                printf("\n");
                if( g_retCodeSeen[li][RC_BUCKETS - 1] > 0 )
                {
                    printf("\nCODEGEN FAILED: %s answered %ld call(s) with a code outside "
                           "the documented function-tier set\n",
                           ALL_LANGUAGES[li].display,
                           g_retCodeSeen[li][RC_BUCKETS - 1]);
                    return TA_CODEGEN_RETCODE_MISMATCH;
                }
                for( fi = 0; fi < sizeof(FLOORED)/sizeof(FLOORED[0]); fi++ )
                {
                    /* Success comes from the VALUE comparison, which a filtered
                     * run — or a corpus of post-cutover functions, which have no
                     * frozen baseline to compare against — legitimately never
                     * reaches. The other three come from test_index_range_xlang,
                     * which runs whatever the filter says, so they are floored
                     * unconditionally. Found by the synth gate: its nine
                     * synthetic functions are all post-cutover, so Rust compared
                     * no values and a blanket floor called that a defect. */
                    if( FLOORED[fi] == 0 && g_codegenCompared[li] == 0 )
                        continue;
                    for( b = 0; b < RC_BUCKETS; b++ )
                    {
                        if( RC_CODE[b] != FLOORED[fi] ) continue;
                        if( g_retCodeSeen[li][b] == 0 )
                        {
                            printf("\nCODEGEN FAILED: %s never answered %s (%d) — a code this "
                                   "corpus reaches on every other language stopped being "
                                   "produced, so whatever maps codes on this one is no longer "
                                   "being exercised across its range\n",
                                   ALL_LANGUAGES[li].display, RC_NAME[b], RC_CODE[b]);
                            return TA_CODEGEN_RETCODE_MISMATCH;
                        }
                    }
                }
            }
            /* The guard this replaces asked whether the census counted
             * nothing ANYWHERE, and could not fire: a language only reaches
             * the census after its own pass completed clean, and that pass
             * includes the index-range leg, which records unconditionally --
             * so "compared something" already implied "counted something".
             *
             * The reachable hole is per-language, and it is the shape every
             * other floor in this file is written per-language to avoid: one
             * server's row going empty is read as "language not run" by the
             * skip above and absorbed, because the other three keep the total
             * non-zero. A language that RAN must have answered something. */
            for( li = 0; li < NUM_LANGUAGES; li++ )
            {
                long total = 0;
                if( !g_langRan[li] )
                    continue;
                for( b = 0; b < RC_BUCKETS; b++ ) total += g_retCodeSeen[li][b];
                if( total == 0 )
                {
                    printf("\nCODEGEN FAILED: %s ran but its retCode census is "
                           "EMPTY -- the census stopped observing this one "
                           "language, which the per-language floors below "
                           "cannot see because they skip an empty row as "
                           "\"language not run\"\n", ALL_LANGUAGES[li].display);
                    return TA_CODEGEN_RETCODE_MISMATCH;
                }
            }
        }

            /* The other half of the bound: it is a MINIMUM. Every leg above
             * asks for exactly the produced count, so without this the harness
             * would prove only that the minimum is accepted — never that a
             * caller re-using a bigger pre-allocated buffer still is. */
            for( unsigned int li = 0; li < NUM_LANGUAGES; li++ )
            {
                if( g_codegenCompared[li] > 0 && g_slackCalls[li] == 0 )
                {
                    printf("\nCODEGEN FAILED: %s never once REPORTED an output "
                           "buffer larger than the count it produced — either the "
                           "out_pad request field stopped being honoured, or the "
                           "harness is asserting the bound is an equality, which "
                           "it is not\n",
                           ALL_LANGUAGES[li].display);
                    return TA_CODEGEN_OUTPUT_MISMATCH;
                }
            }
            {
                long slackTotal = 0;
                for( unsigned int li = 0; li < NUM_LANGUAGES; li++ )
                    slackTotal += g_slackCalls[li];
                if( slackTotal > 0 )
                    printf("  output bound: %ld call(s) at the produced count + %d "
                           "(slack is legal), the rest exactly at it\n",
                           slackTotal, OUT_SLACK_PAD);
            }

            if( startSweepTotal > 0 )
            {
                printf("  startIdx-axis sweep: %ld range(s) with output compared "
                       "at startIdx > 0", startSweepTotal);
                if( g_startSweepSkipped98 > 0 )
                    printf(", %ld withheld (TRIX/NATR partial range — the frozen "
                           "reference predates issue #98)", g_startSweepSkipped98);
                printf("\n");
            }
        }

        /* The per-language floor above is silent when NOTHING is eligible
         * anywhere — which is what a bug making float_leg_set_sentinels always
         * return 0 would look like. On an unfiltered run that is impossible on
         * its face: 79 of the shipped functions carry an optional parameter. So
         * assert it, and only there, since --function= may legitimately select
         * none. */
        if( functionFilter == NULL && g_floatCapableLangTested
            && sentinelEligible == 0 )
        {
            printf("\nCODEGEN FAILED: the default-sentinel float leg found no "
                   "eligible function in an unfiltered run — every shipped "
                   "function appeared to have no optional parameter, so the leg "
                   "asserted nothing anywhere\n");
            return TA_CODEGEN_OUTPUT_MISMATCH;
        }

        /* What the banner is allowed to claim. See g_codegenCompared. */
        {
            long comparedTotal = 0;
            unsigned int li;

            for( li = 0; li < NUM_LANGUAGES; li++ )
                comparedTotal += g_codegenCompared[li];

            if( comparedTotal == 0 )
            {
                printf("\n=============================================\n");
                if( functionFilter == NULL )
                {
                    printf("CODEGEN FAILED: the sweep value-compared NOTHING in an "
                           "unfiltered run — %d language server(s) started and every "
                           "shipped function was skipped, so this run asserted no "
                           "output value anywhere\n", langsTested);
                    printf("=============================================\n");
                    return TA_CODEGEN_OUTPUT_MISMATCH;
                }
                printf("NO VALUE COMPARISON: %d language server(s) started and ran "
                       "the structural legs, but --function=%s selected no function "
                       "this sweep can value-compare — every match was skipped (see "
                       "the skip lines above).\n", langsTested, functionFilter);
                printf("  This is NOT a pass. Post-cutover functions have no frozen "
                       "ta_ref_serve baseline here; their cross-language values are "
                       "gated by:  ta_regtest --xlang-hash --function=%s\n",
                       functionFilter);
                printf("=============================================\n");
                write_timing_report("ta_regtest_timing.jsonl");
                write_markdown_report("ta_regtest_report.md", languageFilter);
                return TA_TEST_PASS;
            }
        }

        /* Java/C# stream counter equality — the only observer for a
         * signed-zero-only divergence (see FuncStreamCounters). Runs before
         * the pass banner so a mismatch cannot print underneath it. */
        if( check_stream_counter_parity() > 0 )
        {
            printf("\n=============================================\n");
            printf("FAILED: Java and C# disagree on stream leg/benign counts.\n");
            printf("  Both render the same source, so these are the same\n"
                   "  deterministic integers over the same inputs. A difference\n"
                   "  means one of them computes a different value somewhere the\n"
                   "  bitwise legs let through -- in practice a +0.0/-0.0 split,\n"
                   "  which svXtierNe counts as benign and never fails on.\n");
            printf("=============================================\n");
            write_timing_report("ta_regtest_timing.jsonl");
            write_markdown_report("ta_regtest_report.md", languageFilter);
            return TA_CODEGEN_OUTPUT_MISMATCH;
        }

        printf("\n=============================================\n");
        if( g_floatCapableLangTested )
        {
            printf("All %d language(s) passed codegen verification (float leg: %ld "
                   "acknowledged comparison(s))\n", langsTested, g_floatLegCompared);
            printf("  default-sentinel pass: %ld comparison(s) with output over %ld "
                   "eligible function-language pair(s)", sentinelTotal, sentinelEligible);
            if( g_floatSentinelEnumWithheld > 0 )
                printf(", %ld choice-list slot(s) withheld (see "
                       "codegen_lang_can_pass_enum_sentinel)", g_floatSentinelEnumWithheld);
            printf("\n");
        }
        else
            printf("All %d language(s) passed codegen verification (no float leg: "
                   "Rust has no single-precision surface)\n", langsTested);
        printf("=============================================\n");
    }

    /* Write report files */
    write_timing_report("ta_regtest_timing.jsonl");
    write_markdown_report("ta_regtest_report.md", languageFilter);

    /* Print absolute paths */
    {
        char jsonl_path[PATH_MAX];
        char md_path[PATH_MAX];
        if( realpath("ta_regtest_timing.jsonl", jsonl_path) )
            printf("\nJSONL: %s\n", jsonl_path);
        else
            printf("\nJSONL: ta_regtest_timing.jsonl\n");
        if( realpath("ta_regtest_report.md", md_path) )
            printf("Report: %s\n", md_path);
        else
            printf("Report: ta_regtest_report.md\n");
    }

    /* Print summary chart to stdout */
    {
        int total = g_numTimingResults;
        double cRefSum = 0; int cRefCount = 0;
        int langPass[NUM_LANGUAGES];
        double langSum[NUM_LANGUAGES];
        int langMeasured[NUM_LANGUAGES];
        memset(langPass, 0, sizeof(langPass));
        memset(langSum, 0, sizeof(langSum));
        memset(langMeasured, 0, sizeof(langMeasured));

        for( int ri = 0; ri < g_numTimingResults; ri++ ) {
            FuncTimingResult *r = &g_timingResults[ri];
            if( r->c_ref_ns > 0 ) { cRefSum += r->c_ref_ns; cRefCount++; }
            for( unsigned int li = 0; li < NUM_LANGUAGES; li++ ) {
                if( r->langs[li].tested == 1 ) langPass[li]++;
                if( r->langs[li].tested == 1 && r->langs[li].avg_ns > 0 ) {
                    langSum[li] += r->langs[li].avg_ns;
                    langMeasured[li]++;
                }
            }
        }
        double cRefAvg = cRefCount > 0 ? cRefSum / cRefCount : 0;

        printf("\n\xe2\x94\x8c\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80"
               "\xe2\x94\xac\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80"
               "\xe2\x94\xac\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80"
               "\xe2\x94\xac\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80"
               "\xe2\x94\xac\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80"
               "\xe2\x94\x90\n");
        printf("\xe2\x94\x82 %-9s\xe2\x94\x82 %-5s\xe2\x94\x82 %-5s\xe2\x94\x82 %-11s\xe2\x94\x82 %-15s\xe2\x94\x82\n",
               "Language", "Pass", "Fail", "Avg (ns)", "vs C-ref");
        printf("\xe2\x94\x9c\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80"
               "\xe2\x94\xbc\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80"
               "\xe2\x94\xbc\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80"
               "\xe2\x94\xbc\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80"
               "\xe2\x94\xbc\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80"
               "\xe2\x94\xa4\n");

        /* C-ref row */
        {
            char avg[32]; fmt_ns(avg, sizeof(avg), cRefAvg);
            printf("\xe2\x94\x82 %-9s\xe2\x94\x82 %-5d\xe2\x94\x82 %-5d\xe2\x94\x82 %-11s\xe2\x94\x82 %-15s\xe2\x94\x82\n",
                   "C-ref", total, 0, avg, "baseline");
        }

        for( unsigned int li = 0; li < NUM_LANGUAGES; li++ ) {
            if( !language_matches_filter(languageFilter, ALL_LANGUAGES[li].name) )
                continue;
            double avg = langMeasured[li] > 0 ? langSum[li] / langMeasured[li] : 0;
            char avgStr[32], vsStr[32];
            if( langMeasured[li] < total / 2 ) {
                fmt_ns(avgStr, sizeof(avgStr), avg);
                char tmp[40]; snprintf(tmp, sizeof(tmp), "~%s*", avgStr);
                avgStr[0] = '\0'; strncat(avgStr, tmp, sizeof(avgStr) - 1);
                snprintf(vsStr, sizeof(vsStr), "*%d/%d measured", langMeasured[li], total);
            } else {
                fmt_ns(avgStr, sizeof(avgStr), avg);
                fmt_ratio(vsStr, sizeof(vsStr), avg, cRefAvg);
            }
            int fail = total - langPass[li];
            printf("\xe2\x94\x82 %-9s\xe2\x94\x82 %-5d\xe2\x94\x82 %-5d\xe2\x94\x82 %-11s\xe2\x94\x82 %-15s\xe2\x94\x82\n",
                   ALL_LANGUAGES[li].display, langPass[li], fail, avgStr, vsStr);
        }

        printf("\xe2\x94\x94\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80"
               "\xe2\x94\xb4\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80"
               "\xe2\x94\xb4\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80"
               "\xe2\x94\xb4\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80"
               "\xe2\x94\xb4\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80"
               "\xe2\x94\x98\n");
    }

    return TA_TEST_PASS;
}

/* Abstract codegen tests are integrated into test_abstract.c via
 * test_abstract_set_server(). */

