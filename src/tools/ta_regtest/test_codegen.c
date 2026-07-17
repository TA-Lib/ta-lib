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

/* Display flags set by ta_regtest.c --no-guarded / --no-unguarded */
int g_hideGuarded = 0;
int g_hideUnguarded = 0;
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

/* Timing now comes from each server's JSON-RPC timing_ns field (the reference
 * baseline is ta_ref_serve, task #7), so no in-process timer is needed here. */

/* ---- Language definitions ---- */

typedef struct {
    const char *name;           /* "rust", "c", "java", "dotnet" */
    const char *display;        /* "Rust", "C", "Java", ".NET" */
    const char *const *argv;    /* NULL-terminated command array */
} CodegenLanguage;

static const char *const argv_rust[]  = {"./ta_codegen_serve_rust", NULL};
static const char *const argv_c[]     = {"./ta_codegen_serve_c", NULL};
static const char *const argv_java[]  = {"java", "-cp", "ta_codegen_java", "TaCodegenServe", NULL};
static const char *const argv_dotnet[]= {"dotnet", "ta_codegen_dotnet/TaCodegenServe.dll", NULL};
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
    {"dotnet", ".NET",         argv_dotnet},
};
#define NUM_LANGUAGES (sizeof(ALL_LANGUAGES) / sizeof(ALL_LANGUAGES[0]))

/* ---- Global timing results store (Task 12) ---- */

#define MAX_FUNCTIONS 200

typedef struct {
    char   funcName[64];
    double c_ref_ns;
    struct {
        int    tested;   /* 0=skipped, 1=pass, -1=fail */
        double avg_ns;
        double avg_ns_unguarded;
    } langs[NUM_LANGUAGES];
} FuncTimingResult;

static FuncTimingResult g_timingResults[MAX_FUNCTIONS];
static int              g_numTimingResults = 0;

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

static int json_write_double_array(char *buf, int buf_size,
                                   const TA_Real *data, int count, int widen)
{
    int pos = 0;
    buf[pos++] = '[';
    for( int i = 0; i < count; i++ )
    {
        if( i > 0 )
            pos += snprintf(buf + pos, buf_size - pos, ",");
        if( widen )
            /* Round to float then back to double; %.17g round-trips exactly. */
            pos += snprintf(buf + pos, buf_size - pos, "%.17g", (double)(float)data[i]);
        else
            pos += snprintf(buf + pos, buf_size - pos, "%.15g", data[i]);
    }
    buf[pos++] = ']';
    buf[pos] = '\0';
    return pos;
}

/* Lossless input transport (issue #115, shared via test_codegen.h): serialize
 * each double as its 16-hex-char IEEE-754 bit pattern inside one JSON string,
 * decoded byte-exactly by every server's array parser. No %.15g rounding to hide
 * a divergence behind. Used by --xlang-hash's Java leg and by server_verify. */
int codegen_write_hexbits_array(char *buf, int buf_size,
                                const TA_Real *data, int count)
{
    int pos = 0;
    buf[pos++] = '"';
    for( int i = 0; i < count; i++ )
    {
        unsigned long long bits;
        memcpy(&bits, &data[i], sizeof(bits));
        pos += snprintf(buf + pos, buf_size - pos, "%016llx", bits);
    }
    buf[pos++] = '"';
    buf[pos] = '\0';
    return pos;
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

/* ---- Unstable period lookup ---- */

/* Map function name to TA_FuncUnstId for range-sweep tolerance selection.
 * Entries are the 22 functions that carry TA_FUNC_FLG_UNST_PER, plus the 8
 * EMA-derived functions (DEMA/TEMA/TRIX/MACD/MACDEXT/MACDFIX + APO/PPO) that
 * converge like EMA. APO and PPO now default to EMA (issue #120), so their
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
    {"ADXR",         TA_FUNC_UNST_ADXR},
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
     * range sweep must use the tight TA_FUNC_UNST_NONE tolerance (IMI is
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
    {"STOCHRSI",     TA_FUNC_UNST_STOCHRSI},
    {"T3",           TA_FUNC_UNST_T3},
    /* EMA-derived: doRangeTest sweeps UNST_EMA, as the hand MA tests do. */
    {"DEMA",         TA_FUNC_UNST_EMA},
    {"TEMA",         TA_FUNC_UNST_EMA},
    {"TRIX",         TA_FUNC_UNST_EMA},
    {"MACD",         TA_FUNC_UNST_EMA},
    {"MACDEXT",      TA_FUNC_UNST_EMA},
    {"MACDFIX",      TA_FUNC_UNST_EMA},
    /* APO/PPO default to EMA (#120) -> EMA-converging, like MACDEXT. */
    {"APO",          TA_FUNC_UNST_EMA},
    {"PPO",          TA_FUNC_UNST_EMA},
};
#define NUM_UNSTABLE_MAP (sizeof(UNSTABLE_MAP) / sizeof(UNSTABLE_MAP[0]))

static TA_FuncUnstId get_unst_id(const char *funcName)
{
    for( unsigned int i = 0; i < NUM_UNSTABLE_MAP; i++ )
    {
        if( strcmp(funcName, UNSTABLE_MAP[i].name) == 0 )
            return UNSTABLE_MAP[i].id;
    }
    return TA_FUNC_UNST_NONE;
}

/* doRangeTest convergence driver; ADXR converges via its internal ADX
 * (same mapping as test_adx.c). Codegen sweeps keep get_unst_id(). */
static TA_FuncUnstId get_range_unst_id(const char *funcName)
{
    if( strcmp(funcName, "ADXR") == 0 ) return TA_FUNC_UNST_ADX;
    if( strcmp(funcName, "STOCHRSI") == 0 ) return TA_FUNC_UNST_RSI;
    return get_unst_id(funcName);
}

/* ---- Generic CodegenRangeTestParam (Task 6) ---- */

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
    double optOverride[16];

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

    /* Timing */
    long long c_ref_total_ns;
    long long server_total_ns;
    int       timing_count;
    long long server_unguarded_total_ns;
    int       timing_unguarded_count;
} CodegenRangeTestParam;

/* Forward declaration: defined with the sweep further below, used by the
 * per-function default pass as well. */
static void run_float_leg(CodegenRangeTestParam *p);

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
    pos += snprintf(buf + pos, bufSize - pos,
        "{\"method\":\"TA_%s\",\"params\":{\"startIdx\":%d,\"endIdx\":%d",
        fi->name, (int)startIdx, (int)endIdx);

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
                pos += snprintf(buf + pos, bufSize - pos, ",\"inOpen\":");
                pos += json_write_double_array(buf + pos, bufSize - pos,
                           p->history->open, p->nbBars, p->widenFloatInputs);
            }
            if( flags & TA_IN_PRICE_HIGH )
            {
                pos += snprintf(buf + pos, bufSize - pos, ",\"inHigh\":");
                pos += json_write_double_array(buf + pos, bufSize - pos,
                           p->history->high, p->nbBars, p->widenFloatInputs);
            }
            if( flags & TA_IN_PRICE_LOW )
            {
                pos += snprintf(buf + pos, bufSize - pos, ",\"inLow\":");
                pos += json_write_double_array(buf + pos, bufSize - pos,
                           p->history->low, p->nbBars, p->widenFloatInputs);
            }
            if( flags & TA_IN_PRICE_CLOSE )
            {
                pos += snprintf(buf + pos, bufSize - pos, ",\"inClose\":");
                pos += json_write_double_array(buf + pos, bufSize - pos,
                           p->history->close, p->nbBars, p->widenFloatInputs);
            }
            if( flags & TA_IN_PRICE_VOLUME )
            {
                pos += snprintf(buf + pos, bufSize - pos, ",\"inVolume\":");
                pos += json_write_double_array(buf + pos, bufSize - pos,
                           p->history->volume, p->nbBars, p->widenFloatInputs);
            }
            if( flags & TA_IN_PRICE_OPENINTEREST )
            {
                pos += snprintf(buf + pos, bufSize - pos, ",\"inOpenInterest\":");
                pos += json_write_double_array(buf + pos, bufSize - pos,
                           p->history->openInterest, p->nbBars, p->widenFloatInputs);
            }
            break;
        }
        case TA_Input_Real:
        {
            if( totalRealInputs == 1 )
            {
                /* Single real input: "inReal" */
                pos += snprintf(buf + pos, bufSize - pos, ",\"inReal\":");
                pos += json_write_double_array(buf + pos, bufSize - pos,
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

                pos += snprintf(buf + pos, bufSize - pos, ",\"inReal%d\":", realInputCount);
                pos += json_write_double_array(buf + pos, bufSize - pos,
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

        pos += snprintf(buf + pos, bufSize - pos, ",\"%s\":", optInfo->paramName);

        switch( optInfo->type )
        {
        case TA_OptInput_RealRange:
        case TA_OptInput_RealList:
            pos += snprintf(buf + pos, bufSize - pos, "%.15g",
                p->optOverrideActive ? p->optOverride[i] : optInfo->defaultValue);
            break;
        case TA_OptInput_IntegerRange:
            pos += snprintf(buf + pos, bufSize - pos, "%d",
                p->optOverrideActive ? (int)p->optOverride[i]
                : p->useLargePeriod  ? compute_large_int(optInfo, p->nbBars)
                                     : (int)optInfo->defaultValue);
            break;
        case TA_OptInput_IntegerList:
            pos += snprintf(buf + pos, bufSize - pos, "%d",
                p->optOverrideActive ? (int)p->optOverride[i]
                                     : (int)optInfo->defaultValue);
            break;
        }
    }

    /* Unstable period (for functions with TA_FUNC_FLG_UNST_PER) */
    if( fi->flags & TA_FUNC_FLG_UNST_PER )
    {
        int unstPeriod = (p->unstId != TA_FUNC_UNST_NONE)
                         ? (int)TA_GetUnstablePeriod(p->unstId) : 0;
        pos += snprintf(buf + pos, bufSize - pos, ",\"unstablePeriod\":%d", unstPeriod);
    }

    if( p->useFloat )
        pos += snprintf(buf + pos, bufSize - pos, ",\"use_float\":1");

    pos += snprintf(buf + pos, bufSize - pos, "}}");

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
 * value-compared. This exemption applies ONLY to comparisons whose baseline is
 * the frozen reference — NOT the float leg, whose baseline is the current double
 * variant (a self-consistency check, see the widenFloatInputs guard at callsite). */
static int codegen_ref_value_exempt(const char *name)
{
    return strcmp(name, "STOCHRSI") == 0;
}

static void compare_codegen_output_generic(
    CodegenRangeTestParam *p,
    unsigned int outputNb)
{
    /* Skip if we already have an error */
    if( p->codegenError != TA_TEST_PASS )
        return;

    /* Unsupported function -- skip silently */
    if( json_is_error(p->responseBuf) )
        return;

    /* Compare retCode */
    int cg_retCode = json_get_int(p->responseBuf, "retCode");
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
     * intentionally diverge from the frozen reference (issue #107 STOCHRSI).
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
                /* Float leg (TA_S_* single precision): the codegen widens float
                 * inputs to double, so ~1e-6-relative single-significand noise is
                 * expected. Unchanged. */
                double scale = (p->epsilonScale > 0.0) ? p->epsilonScale : 1.0;
                threshold = CODEGEN_EPSILON * scale;
                if( fabs(cVal) > 1.0 )
                {
                    double relThreshold = fabs(cVal) * 1e-6 * scale;
                    if( relThreshold > threshold ) threshold = relThreshold;
                }
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

    /* Parse server timing_ns_unguarded if present */
    const char *timingUngVal = json_find_field(p->responseBuf, "timing_ns_unguarded", &len);
    if( timingUngVal )
    {
        long long serverUngNs = strtoll(timingUngVal, NULL, 10);
        p->server_unguarded_total_ns += serverUngNs;
        p->timing_unguarded_count++;
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
    {
        build_json_request(p, 0, endIdx);

        /* Reference baseline (ta_ref_serve). */
        ErrorNumber rref = codegen_pipe_call(p->refCp, p->requestBuf,
                                             p->responseBuf, JSON_BUF_SIZE);
        if( rref != TA_TEST_PASS )
        {
            printf("EDGE SWEEP [TA_%s]: reference server call failed at (0,%d)\n",
                   p->funcInfo->name, (int)endIdx);
            p->codegenError = rref;
            return;
        }
        if( json_is_error(p->responseBuf) )
            continue;  /* function unsupported / errored at this range */
        parse_ref_baseline(p);

        /* Language server under test. A crash (e.g. a debug-build arithmetic
         * overflow) closes the pipe, so the read returns non-PASS here. */
        ErrorNumber rlang = codegen_pipe_call(p->cp, p->requestBuf,
                                              p->responseBuf, JSON_BUF_SIZE);
        if( rlang != TA_TEST_PASS )
        {
            printf("CODEGEN EDGE CRASH [TA_%s]: server stopped responding at "
                   "range (0,%d) -- likely a debug-build arithmetic overflow\n",
                   p->funcInfo->name, (int)endIdx);
            p->codegenError = rlang;
            return;
        }

        for( unsigned int o = 0; o < p->funcInfo->nbOutput; o++ )
            compare_codegen_output_generic(p, o);
        if( p->codegenError != TA_TEST_PASS )
        {
            printf("  (edge range was (0,%d))\n", (int)endIdx);
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
    if( p->outputIsInteger[outputNb] && p->unstId != TA_FUNC_UNST_NONE )
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
            TA_SetOutputParamRealPtr(p->paramHolder, i, p->outRealBufs[i]);
            break;
        case TA_Output_Integer:
            p->outputIsInteger[i] = 1;
            p->outIntBufs[i] = (TA_Integer *)calloc(MAX_NB_TEST_ELEMENT, sizeof(TA_Integer));
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
    static const char *rangeDependent[] = { "AD", "ADOSC", "OBV", "SAR", "SAREXT" };
    static const struct { const char *name; unsigned int tol; } perFuncTol[] = {
        { "MINUS_DI", 2 }, { "PLUS_DI", 2 }, { "DX", 2 },
        { "ADX", 2 }, { "ADXR", 2 },
        { "ATR", 2 }, { "NATR", 2 }, { "RSI", 2 }, { "CMO", 2 },
        { "MACD", 10 }, { "MACDEXT", 10 }, { "MACDFIX", 10 },
        { "HT_DCPHASE", 360 },
        { "HT_SINE", 10 },
    };
    for( unsigned int i = 0; i < sizeof(rangeDependent)/sizeof(rangeDependent[0]); i++ )
    {
        if( strcmp(funcInfo->name, rangeDependent[i]) == 0 )
            return TA_DO_NOT_COMPARE;
    }
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
     * get_integer_tolerance() marks TA_DO_NOT_COMPARE (accumulations seeded at
     * startIdx and path-dependent state machines -- AD, ADOSC, OBV, SAR, SAREXT).
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

    if( get_range_unst_id(funcInfo->name) != TA_FUNC_UNST_NONE )
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
    if( ctx->refFuncList )
    {
        char needle[80];
        snprintf(needle, sizeof(needle), "\"TA_%s\"", funcInfo->name);
        if( !strstr(ctx->refFuncList, needle) ) { ctx->skipped++; return; }
    }

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
    params.refCp       = ctx->refCp;
    params.requestBuf  = ctx->requestBuf;
    params.responseBuf = ctx->responseBuf;
    params.codegenError = TA_TEST_PASS;

    /* Set up inputs (using defaults for optional params) */
    setup_inputs(paramHolder, funcInfo, ctx->history);

    /* Set up output buffers */
    setup_outputs(&params);

    /* ---- Baseline from ta_ref_serve (reference-as-server, task #7) ----
     * The codegen comparison baseline is the reference C library exposed as a
     * JSON-RPC server, NOT an in-process call. ta_ref_serve links the frozen
     * pinned-tag reference and speaks the same protocol, so one request drives
     * both it and the language server under test. Post-cutover this keeps the
     * generated C server diffed against a frozen reference, not against itself.
     * (doRangeTest below still calls the in-process lib for self-coherency.) */
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

    /* Whether the backend supported this function at the default period (non-error
     * response). Used so the large-period pass can flag a server error that appears
     * ONLY at the large period as a real regression, not an unsupported-skip. */
    int defaultSupported = (codegenErr == TA_TEST_PASS)
                           && !json_is_error(params.responseBuf);

    /* Snapshot server timing from the full-range comparison call. Both c_ref_ns
     * (ta_ref_serve) and s_avg_ns are single full-range JSON-RPC calls measuring
     * the raw indicator time server-side — apples-to-apples. */
    double s_avg_ns = (params.timing_count > 0)
                      ? (double)params.server_total_ns / (double)params.timing_count
                      : 0.0;
    double s_avg_ns_unguarded = (params.timing_unguarded_count > 0)
                      ? (double)params.server_unguarded_total_ns / (double)params.timing_unguarded_count
                      : 0.0;

    /* ---- Float-variant pass (TA_S_) ----
     * Every function at default params, C server only (the other backends
     * have no single-precision API): single-precision current vs
     * single-precision frozen reference. This is the systematic coverage
     * for the 161 TA_S_ guarded+unguarded pairs.
     */
    if( params.codegenError == TA_TEST_PASS && strcmp(ctx->lang->name, "c") == 0 )
        run_float_leg(&params);

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
            get_range_unst_id(funcInfo->name),
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
            if( params.timing_unguarded_count > 0 )
                g_timingResults[resultIdx].langs[ctx->langIndex].avg_ns_unguarded = s_avg_ns_unguarded;
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
            if( params.timing_unguarded_count > 0 )
                g_timingResults[resultIdx].langs[ctx->langIndex].avg_ns_unguarded = s_avg_ns_unguarded;
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
        if( params.timing_unguarded_count > 0 )
            g_timingResults[resultIdx].langs[ctx->langIndex].avg_ns_unguarded = s_avg_ns_unguarded;
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

#define SWEEP_MAX_OPT 16

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

/* Compare the in-process GUARDED call against the ta_ref_serve baseline for
 * one sweep variant. The language servers reply with their UNGUARDED result
 * (the server re-runs TA_X_Unguarded over the same buffers), so without this
 * leg the guarded path would only ever be verified at the hand-written pins:
 * server-vs-reference checks unguarded, this checks guarded, closing the
 * guarded/unguarded/reference triangle at every sweep variant. C only — the
 * in-process library IS the C backend. */
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
     * intentionally diverge from the frozen reference (issue #107 STOCHRSI). */
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

/* Float-variant leg: re-run the current request through the TA_S_ API on
 * BOTH servers ("use_float":1) and diff single-precision against
 * single-precision. The frozen reference library exports the guarded TA_S_
 * functions, so ta_ref_serve provides a true S baseline. Widened epsilon:
 * float carries ~1e-7 relative noise per reordered operation chain. */
static void run_float_leg(CodegenRangeTestParam *p)
{
    if( p->codegenError != TA_TEST_PASS )
        return;

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
    if( codegen_pipe_call(p->cp, p->requestBuf, p->responseBuf,
                          JSON_BUF_SIZE) == TA_TEST_PASS
        && !json_is_error(p->responseBuf) )
    {
        parse_ref_baseline(p);

        /* Single-precision variant on the same inputs — must match bit-for-bit. */
        p->useFloat = 1;
        build_json_request(p, 0, p->nbBars - 1);
        if( codegen_pipe_call(p->cp, p->requestBuf, p->responseBuf,
                              JSON_BUF_SIZE) != TA_TEST_PASS )
            p->codegenError = TA_CODEGEN_RETCODE_MISMATCH;
        else
            for( unsigned int o = 0; o < p->funcInfo->nbOutput; o++ )
                compare_codegen_output_generic(p, o);
        if( p->codegenError != TA_TEST_PASS )
            printf("  (mismatch above is the FLOAT (TA_S_) leg: TA_S_ vs TA_ on widened inputs)\n");
    }

    p->useFloat = 0;
    p->widenFloatInputs = 0;
    p->epsilonScale = 0.0;
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
        run_float_leg(p);
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
        if( !strstr(ctx->refFuncList, needle) ) return;
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

        double cand[8];
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
            for( e = 0; e < l->nbElement && nc < 8; e++ )
            {
                if( l->data[e].value != (int)optInfo->defaultValue )
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
     * in-process library switched, for the guarded triangle leg). */
    if( params.codegenError == TA_TEST_PASS )
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
        params.unstId != TA_FUNC_UNST_NONE )
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

#define STREAM_MAX_OPT 8
#define STREAM_MAX_VEC 64
#define STREAM_N       240

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
    int pos = sprintf(buf,
        "{\"method\":\"stream_verify\",\"params\":{\"funcName\":\"TA_%s\","
        "\"gen_shape\":%d,\"gen_seed\":%d,\"gen_n\":%d,"
        "\"unstablePeriod\":%d,\"compatibility\":%d",
        fi->name, shape, seed, n, unstablePeriod, compat);
    /* Candle functions: ask the server for the settings-variation rounds
     * (avgPeriods bumped, then zeroed) on top of the default-settings legs. */
    if( fi->flags & TA_FUNC_FLG_CANDLESTICK )
        pos += sprintf(buf + pos, ",\"candleLegs\":1");
    unsigned int i;
    for( i = 0; i < fi->nbOptInput && i < STREAM_MAX_OPT; i++ )
    {
        const TA_OptInputParameterInfo *oi;
        TA_GetOptInputParameterInfo(fi->handle, i, &oi);
        if( oi->type == TA_OptInput_RealRange || oi->type == TA_OptInput_RealList )
            pos += sprintf(buf + pos, ",\"%s\":%.15g", oi->paramName, optVals[i]);
        else
            pos += sprintf(buf + pos, ",\"%s\":%d", oi->paramName, (int)optVals[i]);
    }
    sprintf(buf + pos, "}}");
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
    int hasMin = 0, hasMinPlus1 = 0, nvec, v;
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
     * so wraparound is hit over the fixed STREAM_N history — and for a
     * fast-path-skip function (MIDPRICE) a period ABOVE its perf threshold,
     * where the batch runs the very else-arm the stream models. The +41 (odd)
     * offset also FLIPS PARITY vs the default, so a parity-branched dual-mode
     * function (TRIMA odd/even) gets a non-degenerate ODD large period (its
     * default 30 is even; min=1 is odd but degenerate), locking the odd arm's
     * Open/Update/Peek continuation into CI. Without this vector,
     * stream_build_vectors hands every function only default/min small periods,
     * so a fast-path-skip stream is otherwise verified only in its
     * threshold-and-below regime. Deduped vs the default; not a min/enum vector. */
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
        int baseVecs = nvec;
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
    int isUnstable;

    if( ctx->error != TA_TEST_PASS ) return;
    if( !codegen_matches_filter(ctx->functionFilter, funcInfo->name) ) return;

    /* K-leg eligibility: the function's own unstable flag, or an internal
     * unstable dependency (DEMA/TEMA/TRIX/MACD map to EMA in UNSTABLE_MAP —
     * their stream values depend on EMA's ambient K). */
    isUnstable = (funcInfo->flags & TA_FUNC_FLG_UNST_PER) != 0 ||
                 get_unst_id(funcInfo->name) != TA_FUNC_UNST_NONE;
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
        for( variant = 0; variant < FUZZ_NSHAPES; variant++ )
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
                 * EMA-family arm seeds differently under Metastock). */
                if( v != 0 && !vecIsEnum[v] ) continue;
                compat = 1;
            }
            else if( variant >= 3 )
            {
                if( v != 0 ) continue; /* extra shapes: defaults vector only */
            }
            shape = (variant >= 3) ? variant : (v + variant) % 7;
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
    if( fillChecked ) ctx->streamFillFunctions++;
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
    /* .NET P/Invokes the C library and does not re-implement these builtins, so
     * eval_predicate is a C/Rust/Java check only. */
    if( strcmp(lang->name, "dotnet") == 0 )
        return TA_TEST_PASS;

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
        int pos = snprintf(reqBuf, JSON_BUF_SIZE,
            "{\"method\":\"eval_predicate\",\"params\":{\"which\":%d,\"values\":[", which);
        for( int i = 0; i < n; i++ )
            pos += snprintf(reqBuf + pos, JSON_BUF_SIZE - pos, "%s%.17g", i ? "," : "", vals[i]);
        pos += snprintf(reqBuf + pos, JSON_BUF_SIZE - pos, "],\"scale\":[");
        for( int i = 0; i < n; i++ )
            pos += snprintf(reqBuf + pos, JSON_BUF_SIZE - pos, "%s%.17g", i ? "," : "", scales[i]);
        snprintf(reqBuf + pos, JSON_BUF_SIZE - pos, "]}}");

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
            printf("  (warning: ta_ref_serve list_functions failed — subset gate disabled)\n");
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

    /* Ref differential sweep: broaden the ta_ref_serve comparison beyond the
     * default and large-period points (see sweep_one_function). */
    if( ctx.error == TA_TEST_PASS && refCp )
    {
        ctx.sweepVariants  = 0;
        ctx.sweepFunctions = 0;
        TA_ForEachFunc(sweep_one_function, &ctx);
        printf("  Ref differential sweep: %d variants across %d functions%s\n",
               ctx.sweepVariants, ctx.sweepFunctions,
               ctx.error == TA_TEST_PASS ? ", all match ta_ref_serve" : "");
    }

    /* Stream verification: batch-vs-stream bitwise, computed in-server.
     * Capability probe first: only a server implementing stream_verify
     * answers an unknown-function probe with "not_streamable"; anything
     * else (unknown method, loose foreign dispatch) would misfire on the
     * real requests, so the pass is skipped for that server. */
    if( ctx.error == TA_TEST_PASS )
    {
        ErrorNumber probeErr;
        sprintf(requestBuf,
                "{\"method\":\"stream_verify\",\"params\":{\"funcName\":\"TA_STREAM_PROBE\","
                "\"gen_shape\":0,\"gen_seed\":1,\"gen_n\":2,\"unstablePeriod\":0,\"compatibility\":0}}");
        probeErr = codegen_pipe_call(&cp, requestBuf, responseBuf, JSON_BUF_SIZE);
        if( probeErr == TA_TEST_PASS && strstr(responseBuf, "not_streamable") )
        {
            ctx.streamFunctions     = 0;
            ctx.streamLegs          = 0;
            ctx.streamSkipped       = 0;
            ctx.streamRejectArms    = 0;
            ctx.streamFillFunctions = 0;
            TA_ForEachFunc(stream_one_function, &ctx);
            printf("  Stream verify: %d functions, %d legs bit-exact vs batch, "
                   "%d expected-reject probes, %d without a stream\n"
                   "  OpenAndFill verify: %d functions, filled array == batch(0,n-1) bitwise\n",
                   ctx.streamFunctions, ctx.streamLegs, ctx.streamRejectArms,
                   ctx.streamSkipped, ctx.streamFillFunctions);
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

    /* Check if any language has unguarded data (and user hasn't hidden it) */
    int hasUnguarded = 0;
    if( !g_hideUnguarded )
        for( int ri = 0; ri < g_numTimingResults && !hasUnguarded; ri++ )
            for( unsigned int li = 0; li < NUM_LANGUAGES; li++ )
                if( showLang[li] && g_timingResults[ri].langs[li].avg_ns_unguarded > 0 )
                { hasUnguarded = 1; break; }
    int showGuarded = !g_hideGuarded;

    /* Header */
    printf("%-20s %9s", "Function", "C-ref");
    for( unsigned int li = 0; li < NUM_LANGUAGES; li++ )
    {
        if( showLang[li] )
        {
            if( showGuarded )
                printf(" %9s", ALL_LANGUAGES[li].display);
            if( hasUnguarded )
                printf(" %9s", showGuarded ? "ung" : ALL_LANGUAGES[li].display);
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
                if( hasUnguarded ) printf(" %9s", "--");
            }
            else if( st == -1 )
            {
                if( showGuarded ) printf(" %9s", "FAIL");
                if( hasUnguarded ) printf(" %9s", "--");
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

                /* Unguarded column: color relative to C-ref */
                if( hasUnguarded )
                {
                    if( r->langs[li].avg_ns_unguarded > 0 )
                    {
                        if( r->c_ref_ns > 0 && r->langs[li].avg_ns_unguarded > r->c_ref_ns )
                            printf(" \033[31m%9.0f\033[0m", r->langs[li].avg_ns_unguarded);
                        else if( r->c_ref_ns > 0 && r->langs[li].avg_ns_unguarded < r->c_ref_ns )
                            printf(" \033[32m%9.0f\033[0m", r->langs[li].avg_ns_unguarded);
                        else
                            printf(" %9.0f", r->langs[li].avg_ns_unguarded);
                    }
                    else
                        printf(" %9s", "--");
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
            if( r->langs[li].avg_ns_unguarded > 0 )
                fprintf(f, ",\"avg_ns_unguarded\":%.0f", r->langs[li].avg_ns_unguarded);
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

    /* Check if any language has unguarded data */
    int mdHasUnguarded = 0;
    for( int ri = 0; ri < g_numTimingResults && !mdHasUnguarded; ri++ )
        for( unsigned int li = 0; li < NUM_LANGUAGES; li++ )
            if( showLang[li] && g_timingResults[ri].langs[li].avg_ns_unguarded > 0 )
            { mdHasUnguarded = 1; break; }

    /* Detailed per-function table */
    fprintf(f, "## Results (ns/call)\n\n");
    fprintf(f, "| Function |  C-ref |");
    for( unsigned int li = 0; li < NUM_LANGUAGES; li++ ) {
        if( showLang[li] ) {
            fprintf(f, " %s |", ALL_LANGUAGES[li].display);
            if( mdHasUnguarded ) fprintf(f, " %s-ung |", ALL_LANGUAGES[li].display);
        }
    }
    fprintf(f, "\n|----------|--------|");
    for( unsigned int li = 0; li < NUM_LANGUAGES; li++ ) {
        if( showLang[li] ) {
            fprintf(f, "--------|");
            if( mdHasUnguarded ) fprintf(f, "--------|");
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
                if( mdHasUnguarded ) fprintf(f, "     \xe2\x80\x94 |");
            } else if( r->langs[li].tested == 1 ) {
                char t[32]; fmt_ns(t, sizeof(t), r->langs[li].avg_ns);
                fprintf(f, " %6s |", t);
                if( mdHasUnguarded ) {
                    if( r->langs[li].avg_ns_unguarded > 0 ) {
                        char u[32]; fmt_ns(u, sizeof(u), r->langs[li].avg_ns_unguarded);
                        fprintf(f, " %6s |", u);
                    } else
                        fprintf(f, "     \xe2\x80\x94 |");
                }
            } else {
                fprintf(f, "     \xe2\x80\x94 |");
                if( mdHasUnguarded ) fprintf(f, "     \xe2\x80\x94 |");
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
#define FUZZ_MAX_OPT  8
#define FUZZ_MAX_VEC  48    /* parameter vectors per function. Sized for the
                             * widest sweep (MACDEXT: 3 period ranges x up to 6
                             * candidates + 3 MAType lists x 8 = ~42, + defaults).
                             * fuzz_build_vectors reports any overflow and the
                             * caller fails the run loudly (no silent drop). */
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
    int pos = snprintf(buf, JSON_BUF_SIZE,
        "{\"method\":\"abstract_call\",\"params\":{\"funcName\":\"%s\","
        "\"startIdx\":%d,\"endIdx\":%d,"
        "\"gen_present\":1,\"gen_shape\":%d,\"gen_seed\":%d,\"gen_n\":%d",
        fi->name, s, e, shape, seed, n);
    if( fullOutput )
        pos += snprintf(buf + pos, JSON_BUF_SIZE - pos, ",\"full_output\":1");
    for( unsigned int i = 0; i < fi->nbOptInput; i++ )
    {
        const TA_OptInputParameterInfo *oi;
        TA_GetOptInputParameterInfo(fi->handle, i, &oi);
        if( oi->type == TA_OptInput_RealRange || oi->type == TA_OptInput_RealList )
            pos += snprintf(buf + pos, JSON_BUF_SIZE - pos, ",\"%s\":%.15g", oi->paramName, optVals[i]);
        else
            pos += snprintf(buf + pos, JSON_BUF_SIZE - pos, ",\"%s\":%d", oi->paramName, (int)optVals[i]);
    }
    if( fi->flags & TA_FUNC_FLG_UNST_PER )
        pos += snprintf(buf + pos, JSON_BUF_SIZE - pos, ",\"unstablePeriod\":0");
    snprintf(buf + pos, JSON_BUF_SIZE - pos, "}}");
}

static unsigned long long fuzz_parse_hash(const char *resp)
{
    int len;
    const char *h = json_find_field(resp, "out_hash", &len);
    if( !h ) return 0;
    if( *h == '"' ) h++;
    return strtoull(h, NULL, 16);
}

/* Parameter vectors: defaults + one-param-varied boundary/list sweeps. */
static int fuzz_build_vectors(const TA_FuncInfo *fi,
                              double vec[FUZZ_MAX_VEC][FUZZ_MAX_OPT],
                              int *overflow)
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
    nvec = 1;

    for( i = 0; i < fi->nbOptInput && i < FUZZ_MAX_OPT; i++ )
    {
        const TA_OptInputParameterInfo *oi;
        TA_GetOptInputParameterInfo(fi->handle, i, &oi);
        double cand[10]; int nc = 0, c;

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
                if( !dup && nc < 10 ) cand[nc++] = (double)v;
            }
        }
        else if( oi->type == TA_OptInput_IntegerList )
        {
            const TA_IntegerList *l = (const TA_IntegerList *)oi->dataSet;
            for( unsigned int e2 = 0; l && e2 < l->nbElement && nc < 10; e2++ )
                if( l->data[e2].value != (int)oi->defaultValue )
                    cand[nc++] = (double)l->data[e2].value;
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
                if( nc < 10 ) cand[nc++] = v;
            }
        }

        for( c = 0; c < nc; c++ )
        {
            /* Silent truncation would quietly stop comparing parameter values
             * vs 0.6.4 — count drops so the caller fails the run loudly. */
            if( nvec >= FUZZ_MAX_VEC ) { (*overflow)++; continue; }
            for( unsigned int j = 0; j < fi->nbOptInput && j < FUZZ_MAX_OPT; j++ )
                vec[nvec][j] = def[j];
            vec[nvec][i] = cand[c];
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
 * (docs/fma-readiness-audit.md). The current library now fuses `a*b + c` into
 * `fma()` wherever the shared codegen detector fires; the frozen v0.6.4 oracle
 * does not. So the two differ by <=~1.7e-10 relative on the fused functions —
 * authorized, below the 1e-9 contract, but no longer hash-exact.
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

enum { TOL_ABS = 0, TOL_REL_IN = 1, TOL_NAN_TO = 2 };
static const struct { const char *name; int mode; double tol; double cap; } FUZZ_064_TOL[] = {
    { "CCI",                 TOL_ABS,    1e-9, 0.0 },  /* #7   near-zero identical-price fix */
    { "LINEARREG",           TOL_REL_IN, 1e-9, 0.0 },  /* #103 O(1) sliding-sum recurrence   */
    { "LINEARREG_SLOPE",     TOL_REL_IN, 1e-9, 0.0 },  /* #103                               */
    { "LINEARREG_INTERCEPT", TOL_REL_IN, 1e-9, 0.0 },  /* #103                               */
    { "LINEARREG_ANGLE",     TOL_REL_IN, 1e-9, 0.5 },  /* #103 bounded degrees -> capped 0.5 */
    { "TSF",                 TOL_REL_IN, 1e-9, 0.0 },  /* #103                               */
    { "IMI",                 TOL_NAN_TO, 50.0, 0.0 },  /* #112 all-flat window 0/0 -> NaN, now 50.0 */
};

/* Look up a function's authorized tolerance; returns NULL if it must be exact. */
static const void *fuzz_064_tol_lookup(const char *name, int *mode, double *tol, double *cap)
{
    for( unsigned int i = 0; i < sizeof(FUZZ_064_TOL)/sizeof(FUZZ_064_TOL[0]); i++ )
        if( strcmp(name, FUZZ_064_TOL[i].name) == 0 )
        { *mode = FUZZ_064_TOL[i].mode; *tol = FUZZ_064_TOL[i].tol; *cap = FUZZ_064_TOL[i].cap;
          return &FUZZ_064_TOL[i]; }
    return NULL;
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
                if( a == b ) benignDiff = 1;        /* numerically equal => signed zero */
                else if( tolEntry && d <= tolBound ) tolDiff = 1; /* within manifest bound */
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
    if( funcInfo->nbOptInput > FUZZ_MAX_OPT ) return;

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
    int nvec = fuzz_build_vectors(funcInfo, vec, &vecOverflow);
    if( vecOverflow > 0 )
    {
        printf("FUZZ VECTOR OVERFLOW [TA_%s]: %d parameter value(s) dropped by "
               "FUZZ_MAX_VEC — they would go uncompared vs 0.6.4\n",
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
            printf("  TOLERATED TA_%s: %lld case(s) within %g%s%s vs 0.6.4 (authorized manifest bound)\n",
                   funcInfo->name, ctx->cciTol - cciTolBefore, tv,
                   tm == TOL_REL_IN ? " * max|input|" : "",
                   (tm == TOL_REL_IN && tc > 0.0) ? " (capped)" : "");
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
        printf("stochrsi-skipped: %lld STOCHRSI case(s) — intentionally diverges from 0.6.4 (issue #107); pinned by test_stoch.c\n",
               ctx.stochRsiSkipped);
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
 *     libm, ~1 ULP) drops to a CODEGEN_JAVA_TRANSCENDENTAL_TOL (1e-9) element
 *     compare, and HT_DCPHASE/HT_SINE on the zero-variance constant shape are
 *     skipped outright (xlang_java_illcond — atan2 of a null signal amplifies
 *     the ULP unboundedly; C==Rust stay bitwise there).
 * There is NO 0.6.4 here (current-vs-current), so — unlike --fuzz-064 — there
 * are none of the #98/#107 carve-outs; every case is bitwise except Java's
 * transcendentals.
 *
 * .NET P/Invokes the C library (== C by construction) so it is not a distinct
 * cross-language check. See fuzz_data.h and src/tools/ta_regtest/CLAUDE.md.
 * ======================================================================== */

typedef struct {
    const char        *name;      /* "rust", "java" — matches --language= tokens */
    const char        *display;   /* "Rust", "Java"                           */
    const char *const *argv;      /* server launch command                   */
    int                usesSeed;  /* transport: 1 = seed (gen_present + fuzz_in_hash
                                   * self-check), 0 = lossless hex-bits inputs (no
                                   * fuzz_gen port — Java, #114)              */
    CodegenPipe        cp;
    int                open;      /* 1 once the subprocess is up              */
    long long          cases;     /* cases compared against the golden        */
    long long          mism;      /* real bitwise/tolerance mismatches        */
    int                restarts;  /* recovered subprocess crashes             */
} XlangServer;

typedef struct {
    const char  *functionFilter;
    XlangServer *sv;
    int          nsv;
    char        *reqBuf;
    char        *respBuf;
    long long    comparisons;        /* golden cases evaluated                 */
    long long    nonEmpty;           /* cases with a non-empty successful output
                                      * (non-vacuity: an empty output hashes the
                                      * same on both sides, so a healthy fraction
                                      * must be non-empty for the gate to bite)   */
    long long    tolCases;           /* Java calls routed to the transcendental
                                      * tolerance path (the rest are bitwise)     */
    long long    illcondSkipped;     /* Java HT_DCPHASE/HT_SINE calls skipped on
                                      * the zero-variance constant shape (phase of
                                      * a null signal — see xlang_java_illcond)    */
    int          reportedThisFunc;
    int          funcsWithFailures;
    ErrorNumber  error;
} XlangCtx;

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
 * bitwise to CODEGEN_JAVA_TRANSCENDENTAL_TOL. Every other function — including
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
static int xlang_java_illcond(const char *name, int shape)
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
                                    int wantHash)
{
    int pos = snprintf(buf, JSON_BUF_SIZE,
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
                    pos += snprintf(buf + pos, JSON_BUF_SIZE - pos, ",\"%s\":", comp[c].key);
                    pos += codegen_write_hexbits_array(buf + pos, JSON_BUF_SIZE - pos,
                                                       comp[c].data, nbBars);
                }
        }
        else if( ii->type == TA_Input_Real )
        {
            const TA_Real *data = (realCount == 0) ? hist->close
                                : (realCount == 1) ? hist->volume : hist->close;
            if( totalRealInputs == 1 )
                pos += snprintf(buf + pos, JSON_BUF_SIZE - pos, ",\"inReal\":");
            else
                pos += snprintf(buf + pos, JSON_BUF_SIZE - pos, ",\"inReal%d\":", realCount);
            pos += codegen_write_hexbits_array(buf + pos, JSON_BUF_SIZE - pos, data, nbBars);
            realCount++;
        }
    }

    for( unsigned int i = 0; i < fi->nbOptInput; i++ )
    {
        const TA_OptInputParameterInfo *oi;
        TA_GetOptInputParameterInfo(fi->handle, i, &oi);
        if( oi->type == TA_OptInput_RealRange || oi->type == TA_OptInput_RealList )
            pos += snprintf(buf + pos, JSON_BUF_SIZE - pos, ",\"%s\":%.15g", oi->paramName, optVals[i]);
        else
            pos += snprintf(buf + pos, JSON_BUF_SIZE - pos, ",\"%s\":%d", oi->paramName, (int)optVals[i]);
    }

    if( fi->flags & TA_FUNC_FLG_UNST_PER )
        pos += snprintf(buf + pos, JSON_BUF_SIZE - pos, ",\"unstablePeriod\":0");
    if( wantHash )
        pos += snprintf(buf + pos, JSON_BUF_SIZE - pos, ",\"want_hash\":1");
    snprintf(buf + pos, JSON_BUF_SIZE - pos, "}}");
}

/* Per-function bitwise comparison: golden in-process C vs each server. */
static void xlang_one_function(const TA_FuncInfo *funcInfo, void *opaqueData)
{
    XlangCtx *ctx = (XlangCtx *)opaqueData;
    unsigned int i;

    if( ctx->error != TA_TEST_PASS ) return;
    if( !codegen_matches_filter(ctx->functionFilter, funcInfo->name) ) return;
    if( funcInfo->nbOptInput > FUZZ_MAX_OPT ) return;

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
    int vecOverflow = 0;
    int nvec = fuzz_build_vectors(funcInfo, vec, &vecOverflow);
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

    static const int sizes[] = {40, 120, 240};
    static const int seeds[] = {1, 2, 3};
    ctx->reportedThisFunc = 0;
    long long mismBefore = 0;
    for( int s = 0; s < ctx->nsv; s++ ) mismBefore += ctx->sv[s].mism;

    for( int shape = 0; shape < FUZZ_NSHAPES; shape++ )
    for( int si = 0; si < (int)(sizeof(seeds)/sizeof(seeds[0])); si++ )
    for( int zi = 0; zi < (int)(sizeof(sizes)/sizeof(sizes[0])); zi++ )
    {
        int n = sizes[zi]; if( n > FUZZ_MAXN ) n = FUZZ_MAXN;
        fuzz_gen(shape, seeds[si], n,
                 g_fzBuf[0], g_fzBuf[1], g_fzBuf[2], g_fzBuf[3], g_fzBuf[4], g_fzBuf[5]);
        hist.nbBars = (unsigned int)n;
        p.nbBars = n;

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

            /* subranges: full + two deterministic random windows (as --fuzz-064) */
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
                if( curRc == TA_SUCCESS && curNb > 0 ) ctx->nonEmpty++;

                /* C golden output buffers in logical order — the tolerance path
                 * (Java transcendentals) element-compares against these; the
                 * bitwise path compares curHash. */
                const void *goldBufs[MAX_OUTPUTS];
                for( unsigned int o = 0; o < funcInfo->nbOutput && o < MAX_OUTPUTS; o++ )
                    goldBufs[o] = p.outputIsInteger[o] ? (const void *)p.outIntBufs[o]
                                                       : (const void *)p.outRealBufs[o];

                ctx->comparisons++;

                for( int sIdx = 0; sIdx < ctx->nsv; sIdx++ )
                {
                    XlangServer *sv = &ctx->sv[sIdx];
                    if( !sv->open ) continue;

                    /* Each server's request follows its transport. Seed servers
                     * (Rust) regenerate inputs from (shape,seed,n) via
                     * gen_present; hex servers (Java, no fuzz_gen port) get the
                     * driver's exact arrays losslessly. A hex call is bitwise
                     * (want_hash) unless it reaches a transcendental (fdlibm !=
                     * the C libm), which drops to the tolerance element-compare. */
                    int tolPath = 0;
                    if( sv->usesSeed )
                        fuzz_build_request(ctx->reqBuf, funcInfo, s, e, shape, seeds[si], n, vec[k], 0);
                    else
                    {
                        tolPath = codegen_call_is_transcendental(funcInfo->handle, vec[k],
                                                                 (int)funcInfo->nbOptInput);
                        /* Chaotic phase of a null signal — not comparable across
                         * libms; C==Rust bitwise, all other shapes gated. */
                        if( tolPath && xlang_java_illcond(funcInfo->name, shape) )
                        { ctx->illcondSkipped++; continue; }
                        xlang_build_hex_request(ctx->reqBuf, funcInfo, &hist, n, s, e, vec[k], !tolPath);
                    }

                    if( !xlang_call(sv, ctx->reqBuf, ctx->respBuf) )
                    {
                        if( ctx->error == TA_TEST_PASS ) ctx->error = TA_CODEGEN_PIPE_READ_FAILED;
                        sv->mism++;
                        continue;
                    }
                    sv->cases++;
                    if( tolPath ) ctx->tolCases++;

                    if( tolPath )
                    {
                        CTolDetail d;
                        CTolVerdict cv = codegen_compare_tol(ctx->respBuf, funcInfo->nbOutput,
                                                             p.outputIsInteger, goldBufs,
                                                             curRc, curBeg, curNb,
                                                             CODEGEN_JAVA_TRANSCENDENTAL_TOL, &d);
                        if( cv != CTOL_MATCH )
                        {
                            sv->mism++;
                            if( ctx->reportedThisFunc < 3 )
                            {
                                ctx->reportedThisFunc++;
                                printf("  XLANG TOL MISMATCH TA_%s  C(golden) vs %s  "
                                       "shape=%d seed=%d n=%d range=[%d,%d]  params:",
                                       funcInfo->name, sv->display, shape, seeds[si], n, s, e);
                                for( unsigned int q = 0; q < funcInfo->nbOptInput; q++ )
                                {
                                    const TA_OptInputParameterInfo *oi;
                                    TA_GetOptInputParameterInfo(funcInfo->handle, q, &oi);
                                    printf(" %s=%.15g", oi->paramName, vec[k][q]);
                                }
                                printf("\n    retCode %d/%d  begIdx %d/%d  nbElem %d/%d",
                                       (int)curRc, d.rc, curBeg, d.begIdx, curNb, d.nbElement);
                                if( cv == CTOL_VALUE && !d.isInt )
                                    printf("  out%d[%d] C=%.17g server=%.17g diff=%.3g (tol %g)",
                                           d.output, d.element, d.cReal, d.sReal,
                                           fabs(d.cReal - d.sReal), CODEGEN_JAVA_TRANSCENDENTAL_TOL);
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
                                printf("  XLANG MISMATCH TA_%s  C(golden) vs %s  "
                                       "shape=%d seed=%d n=%d range=[%d,%d]  params:",
                                       funcInfo->name, sv->display, shape, seeds[si], n, s, e);
                                for( unsigned int q = 0; q < funcInfo->nbOptInput; q++ )
                                {
                                    const TA_OptInputParameterInfo *oi;
                                    TA_GetOptInputParameterInfo(funcInfo->handle, q, &oi);
                                    printf(" %s=%.15g", oi->paramName, vec[k][q]);
                                }
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
     * (gen_present + fuzz_in_hash); Java uses the lossless hex-bits transport
     * (usesSeed=0 — its server has no fuzz_gen port, #114) and relaxes its
     * transcendental-using calls to a tolerance (fdlibm != the C libm). .NET
     * P/Invokes the C library (== C by construction), so it is not a row. */
    static XlangServer servers[] = {
        {"rust", "Rust", argv_rust, 1, {0}, 0, 0, 0, 0},
        {"java", "Java", argv_java, 0, {0}, 0, 0, 0, 0},
    };
    int nsv = (int)(sizeof(servers)/sizeof(servers[0]));

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
               "(valid: rust, java; C is the in-process golden, .NET == C).\n",
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
               162);
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
        printf("  (%lld Java call(s) compared at the transcendental tolerance %g; "
               "every other call is bitwise)\n", ctx.tolCases, CODEGEN_JAVA_TRANSCENDENTAL_TOL);
    if( ctx.illcondSkipped > 0 )
        printf("  (%lld Java HT_DCPHASE/HT_SINE call(s) skipped on the constant "
               "shape: atan2 phase of a null signal, ill-conditioned across libms "
               "— C==Rust bitwise there)\n", ctx.illcondSkipped);

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
    if( totalMism == 0 && inFails == 0 && ctx.error == TA_TEST_PASS )
    {
        printf("PASS — every server matches the in-process C library: BIT-IDENTICAL "
               "(zero tolerance), Java transcendentals within %g "
               "(current-vs-current, all shapes, period>=2).\n",
               CODEGEN_JAVA_TRANSCENDENTAL_TOL);
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

    printf("\n=============================================\n");
    printf("All %d language(s) passed codegen verification\n", langsTested);
    printf("=============================================\n");

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

