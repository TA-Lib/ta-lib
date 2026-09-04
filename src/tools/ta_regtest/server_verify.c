/* server_verify.c — Verify hand-written C test results against JSON-RPC servers.
 *
 * Uses ta_abstract metadata to build JSON requests internally, so test files
 * only pass inputs/params/outputs in signature order without any JSON knowledge.
 *
 * The hard-coded tests already validate in-process C vs the expected constants
 * at a legitimate tolerance. This module runs the OTHER, transitive check —
 * feed the SAME inputs to another language and compare to what C computed — and
 * that must be EXACT: same algorithm, same inputs => same bits (issue #115). It
 * is the same "in-process C <=> language server, bit-for-bit" operation as
 * --xlang-hash (issue #113), differing only in input source: here the inputs are
 * the test's exact arrays, sent losslessly as hex-of-IEEE-bits strings, and the
 * server returns a full-precision out_hash we diff via the shared
 * codegen_hash_compare(). Zero tolerance for C and Rust; a narrow tolerance for
 * Java and C# on transcendental-using functions only (~1 ULP: fdlibm is not the
 * system libm, and .NET's Math.* is not guaranteed to reach it either).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "server_verify.h"
#include "test_codegen.h"   /* codegen_output_hash / codegen_hash_compare / XHash* */
#include "ta_abstract.h"
#include "../../ta_common/ta_global.h"   /* TA_Globals->candleSettings[] */
#include "../ta_alloc_check.h"

/* ---- Configuration ---- */

#define SV_BUF_SIZE (256 * 1024)   /* 256KB request/response buffers */

/* The transcendental tolerance (CODEGEN_TRANSCENDENTAL_TOL), the
 * transcendental-call test (codegen_call_is_transcendental), the lossless
 * hex-bits input writer (codegen_write_hexbits_array), and the tolerance
 * element-compare (codegen_compare_tol) are all shared with the --xlang-hash
 * gate via test_codegen.h — the two run the identical "in-process C <=> language
 * server, bit-for-bit (tolerance only for transcendentals, and only in the
 * languages that need it)" operation. */

/* ---- Global state ---- */

static CodegenPipe *g_pipes[SV_MAX_PIPES];
static const char  *g_pipeLang[SV_MAX_PIPES];          /* "c"/"rust"/"java"/"csharp" */
static int          g_nbPipes = 0;
static char        *g_reqBuf  = NULL;
static char        *g_respBuf = NULL;
static int          g_lastCompatibility[SV_MAX_PIPES]; /* cached per pipe */
static int          g_curPipe = -1; /* pipe being verified (for diagnostics) */

/* Unstable period lookup table (same as test_codegen.c) */
typedef struct { const char *name; TA_FuncUnstId id; } UnstableLookup;
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
    /* IMI and MFI are intentionally omitted: both are finite sliding-window
     * indicators reclassified as stable (no TA_FUNC_FLG_UNST_PER), and their
     * lookback no longer consults the unstable period. ADXR and STOCHRSI are
     * omitted too: their own knobs were inert and retired (#129) — they follow
     * their internal ADX/RSI, whose slots the numeric sync mirrors. Keep in
     * sync with the UNSTABLE_MAP in test_codegen.c. */
    {"KAMA",         TA_FUNC_UNST_KAMA},
    {"MAMA",         TA_FUNC_UNST_MAMA},
    {"MINUS_DI",     TA_FUNC_UNST_MINUS_DI},
    {"MINUS_DM",     TA_FUNC_UNST_MINUS_DM},
    {"NATR",         TA_FUNC_UNST_NATR},
    {"PLUS_DI",      TA_FUNC_UNST_PLUS_DI},
    {"PLUS_DM",      TA_FUNC_UNST_PLUS_DM},
    {"RMA",          TA_FUNC_UNST_RMA},
    {"RSI",          TA_FUNC_UNST_RSI},
    {"T3",           TA_FUNC_UNST_T3},
};
#define NUM_UNSTABLE_MAP (sizeof(UNSTABLE_MAP) / sizeof(UNSTABLE_MAP[0]))

static TA_FuncUnstId sv_func_unst_id(const char *name)
{
    for( unsigned int i = 0; i < NUM_UNSTABLE_MAP; i++ )
    {
        if( strcmp(UNSTABLE_MAP[i].name, name) == 0 )
            return UNSTABLE_MAP[i].id;
    }
    return TA_TEST_UNST_NONE;
}

/* ---- Minimal JSON helpers ----
 * The field/array parsers and the hex-bits writer live in test_codegen.c (shared
 * with --xlang-hash); server_verify uses codegen_write_hexbits_array to serialize
 * inputs and codegen_compare_tol to parse+compare the Java-transcendental path. */

static int sv_json_is_error(const char *json)
{
    return strstr(json, "\"error\"") != NULL;
}

/* ---- Price component mapping ---- */

typedef struct { unsigned int flag; const char *jsonKey; } PriceComponent;
static const PriceComponent PRICE_COMPONENTS[] = {
    { TA_IN_PRICE_OPEN,         "inOpen" },
    { TA_IN_PRICE_HIGH,         "inHigh" },
    { TA_IN_PRICE_LOW,          "inLow" },
    { TA_IN_PRICE_CLOSE,        "inClose" },
    { TA_IN_PRICE_VOLUME,       "inVolume" },
    { TA_IN_PRICE_OPENINTEREST, "inOpenInterest" },
};
#define NUM_PRICE_COMPONENTS 6

/* Cached unstable periods, per pipe: every server must receive its own
 * state syncs — a shared cache would let the first pipe consume the
 * "changed" delta and leave the other servers stale.
 */
static unsigned int g_lastUnstPeriods[SV_MAX_PIPES][TA_FUNC_UNST_COUNT];
static int          g_unstInitialized[SV_MAX_PIPES];

/* Cached candle settings, per pipe, same reasoning as the periods above (#215).
 * Read from TA_Globals->candleSettings[] rather than from a copy the tests hand
 * us: the property being verified is "the server computed under the settings the
 * C library was actually holding", and only the library knows that. There is no
 * public getter, so this reads the global the setter writes -- the same thing
 * test_internals.c does. */
static TA_CandleSetting g_lastCandle[SV_MAX_PIPES][TA_AllCandleSettings];
static int              g_candleInitialized[SV_MAX_PIPES];
static int              g_candleSyncs;   /* non-vacuity: settings pushed, all pipes */

/* Comparisons whose verdict was actually taken, summed over pipes. A caller can
 * require this to advance: "no failure" and "never compared" are otherwise
 * indistinguishable, which is exactly how an erroring server read as green. */
static int              g_comparisons;

/* ---- Init / shutdown ---- */

void server_verify_init(CodegenPipe *pipes[], const char *langs[], int nbPipes)
{
    g_nbPipes = 0;
    for( int i = 0; i < nbPipes && i < SV_MAX_PIPES; i++ )
    {
        if( pipes[i] )
        {
            g_pipeLang[g_nbPipes] = langs ? langs[i] : NULL;
            g_pipes[g_nbPipes++]  = pipes[i];
        }
    }
    if( g_nbPipes > 0 )
    {
        g_reqBuf  = malloc(SV_BUF_SIZE);
        g_respBuf = malloc(SV_BUF_SIZE);
        TA_TOOL_CHECK_ALLOC(g_reqBuf);
        TA_TOOL_CHECK_ALLOC(g_respBuf);
        for( int p = 0; p < SV_MAX_PIPES; p++ )
        {
            g_lastCompatibility[p] = -1;
            g_unstInitialized[p] = 0;
            g_candleInitialized[p] = 0;
        }
        g_candleSyncs = 0;
        g_comparisons = 0;
    }
}

void server_verify_shutdown(void)
{
    free(g_reqBuf);  g_reqBuf  = NULL;
    free(g_respBuf); g_respBuf = NULL;
    g_nbPipes = 0;
    for( int p = 0; p < SV_MAX_PIPES; p++ )
    {
        g_lastCompatibility[p] = -1;
        g_unstInitialized[p] = 0;
        g_candleInitialized[p] = 0;
        g_pipeLang[p] = NULL;
    }
}

int server_verify_candle_syncs(void)
{
    return g_candleSyncs;
}

int server_verify_comparisons(void)
{
    return g_comparisons;
}

int server_verify_active(void)
{
    return g_nbPipes > 0;
}

/* ---- Sync global state to server ---- */

static ErrorNumber sync_unstable_periods(int pipeIdx)
{
    CodegenPipe *pipe = g_pipes[pipeIdx];
    char syncReq[128];
    char syncResp[256];

    if( !g_unstInitialized[pipeIdx] )
    {
        /* First call: reset server to all zeros (matches fresh TA_Initialize). */
        snprintf(syncReq, sizeof(syncReq),
                 "{\"method\":\"set_unstable_period\",\"params\":{\"id\":%d,\"period\":0}}",
                 (int)TA_FUNC_UNST_ALL);
        ErrorNumber err = codegen_pipe_call(pipe, syncReq, syncResp, sizeof(syncResp));
        if( err != TA_TEST_PASS )
            return err;
        if( sv_json_is_error(syncResp) )
        {
            printf("  SV WARN: set_unstable_period rejected by server: %s\n", syncResp);
            return TA_SV_RETCODE_MISMATCH;
        }
        memset(g_lastUnstPeriods[pipeIdx], 0, sizeof(g_lastUnstPeriods[pipeIdx]));
        g_unstInitialized[pipeIdx] = 1;
    }

    /* Only sync IDs that have changed. */
    for( int id = 0; id < TA_FUNC_UNST_COUNT; id++ )
    {
        unsigned int curPeriod = TA_GetUnstablePeriod((TA_FuncUnstId)id);
        if( curPeriod != g_lastUnstPeriods[pipeIdx][id] )
        {
            snprintf(syncReq, sizeof(syncReq),
                     "{\"method\":\"set_unstable_period\",\"params\":{\"id\":%d,\"period\":%u}}",
                     id, curPeriod);
            ErrorNumber err = codegen_pipe_call(pipe, syncReq, syncResp, sizeof(syncResp));
            if( err != TA_TEST_PASS )
                return err;
            if( sv_json_is_error(syncResp) )
            {
                printf("  SV WARN: set_unstable_period rejected by server: %s\n", syncResp);
                return TA_SV_RETCODE_MISMATCH;
            }
            g_lastUnstPeriods[pipeIdx][id] = curPeriod;
        }
    }
    return TA_TEST_PASS;
}

/* Push the C library's live candle settings to one server (#215, #216 gap 6).
 *
 * Before this, no cross-language comparison could run at anything but the
 * defaults: the servers had no method to set them. A candlestick verified here
 * while the C side held custom settings was therefore being compared against a
 * server still on the defaults -- which passed anyway, because the hand-written
 * candle tests never left the defaults. Both halves of that are fixed: the
 * transport exists, and this makes every server_verify() call carry whatever
 * settings the calling test established.
 *
 * `factor` travels as hex-of-IEEE-bits, like every other double this file sends
 * (#115). %.15g would round-trip a threshold scale lossily, and the whole point
 * of the comparison downstream is that it is bitwise.
 */
static ErrorNumber sync_candle_settings(int pipeIdx)
{
    CodegenPipe *pipe = g_pipes[pipeIdx];
    char syncReq[256];
    char syncResp[256];
    ErrorNumber err;

    if( !g_candleInitialized[pipeIdx] )
    {
        /* First call: put the server on the documented defaults, matching a
         * fresh TA_Initialize, so the delta loop below has a known baseline
         * rather than trusting the server's constructor to agree. */
        snprintf(syncReq, sizeof(syncReq),
                 "{\"method\":\"restore_candle_default_settings\",\"params\":{\"settingType\":%d}}",
                 (int)TA_AllCandleSettings);
        err = codegen_pipe_call(pipe, syncReq, syncResp, sizeof(syncResp));
        if( err != TA_TEST_PASS )
            return err;
        if( sv_json_is_error(syncResp) )
        {
            printf("  SV WARN: restore_candle_default_settings rejected by server: %s\n",
                   syncResp);
            return TA_SV_RETCODE_MISMATCH;
        }
        /* Read the defaults WITHOUT leaving them applied. Calling
         * TA_RestoreCandleDefaultSettings and stopping there would do two wrong
         * things at once: silently discard whatever settings the calling test
         * had established -- a verifier mutating the library it is verifying --
         * and, by returning here, never push those settings to the server at
         * all. Both were latent only because the first caller happens to be at
         * the defaults already. Snapshot, read, put back, then fall through to
         * the delta loop so the caller's real settings are what gets sent. */
        TA_CandleSetting saved[TA_AllCandleSettings];
        memcpy(saved, TA_Globals->candleSettings, sizeof(saved));
        TA_RestoreCandleDefaultSettings( TA_AllCandleSettings );
        memcpy(g_lastCandle[pipeIdx], TA_Globals->candleSettings, sizeof(saved));
        memcpy(TA_Globals->candleSettings, saved, sizeof(saved));
        g_candleInitialized[pipeIdx] = 1;
        /* deliberately no return: the loop below pushes the caller's deltas */
    }

    for( int i = 0; i < (int)TA_AllCandleSettings; i++ )
    {
        const TA_CandleSetting *cur  = &TA_Globals->candleSettings[i];
        const TA_CandleSetting *last = &g_lastCandle[pipeIdx][i];

        /* Compared by BITS, not by ==: a factor of NaN can never be stored (the
         * setter refuses it), but -0.0 can, and -0.0 == 0.0 would make a real
         * change invisible to this loop. */
        unsigned long long curBits, lastBits;
        memcpy(&curBits,  &cur->factor,  sizeof(curBits));
        memcpy(&lastBits, &last->factor, sizeof(lastBits));
        if( cur->rangeType == last->rangeType &&
            cur->avgPeriod == last->avgPeriod &&
            curBits == lastBits )
            continue;

        snprintf(syncReq, sizeof(syncReq),
                 "{\"method\":\"set_candle_settings\",\"params\":"
                 "{\"settingType\":%d,\"rangeType\":%d,\"avgPeriod\":%d,"
                 "\"factorBits\":\"%016llx\"}}",
                 i, (int)cur->rangeType, cur->avgPeriod, curBits);
        err = codegen_pipe_call(pipe, syncReq, syncResp, sizeof(syncResp));
        if( err != TA_TEST_PASS )
            return err;
        if( sv_json_is_error(syncResp) )
        {
            printf("  SV WARN: set_candle_settings rejected by server: %s\n", syncResp);
            return TA_SV_RETCODE_MISMATCH;
        }
        g_lastCandle[pipeIdx][i] = *cur;
        g_candleSyncs++;
    }
    return TA_TEST_PASS;
}

static ErrorNumber sync_compatibility(int pipeIdx)
{
    int compat = (int)TA_GetCompatibility();
    if( compat == g_lastCompatibility[pipeIdx] )
        return TA_TEST_PASS;

    /* Use local buffers to avoid overwriting g_reqBuf/g_respBuf
     * which hold the function call request. */
    char syncReq[128];
    char syncResp[256];
    snprintf(syncReq, sizeof(syncReq),
             "{\"method\":\"set_compatibility\",\"params\":{\"mode\":%d}}", compat);
    ErrorNumber err = codegen_pipe_call(g_pipes[pipeIdx], syncReq, syncResp, sizeof(syncResp));
    if( err != TA_TEST_PASS )
        return err;
    if( sv_json_is_error(syncResp) )
    {
        printf("  SV WARN: set_compatibility rejected by server: %s\n", syncResp);
        return TA_SV_RETCODE_MISMATCH;
    }
    g_lastCompatibility[pipeIdx] = compat;
    return TA_TEST_PASS;
}

/* ---- Build JSON request from ta_abstract metadata ----
 *
 * Inputs are always serialized losslessly (hex-of-IEEE-bits). When wantHash,
 * the request carries "want_hash":1 so the server returns an out_hash digest of
 * its raw outputs instead of the arrays (the bitwise path); otherwise it
 * returns the arrays, themselves lossless since #257/#258 (the
 * Java-transcendental tolerance path). Returns -1 if the
 * function is not in ta_abstract (graceful skip). */
static int build_request(const char *funcName,
                         TA_Integer startIdx, TA_Integer endIdx,
                         int nbBars,
                         const TA_Real *inputs[],
                         const double optParams[], int nbOptParams,
                         int wantHash)
{
    const TA_FuncHandle *handle;
    const TA_FuncInfo   *fi;

    if( TA_GetFuncHandle(funcName, &handle) != TA_SUCCESS )
        return -1;
    if( TA_GetFuncInfo(handle, &fi) != TA_SUCCESS )
        return -1;

    int pos = 0;
    pos = codegen_appendf(g_reqBuf, SV_BUF_SIZE, pos,
                    "{\"method\":\"TA_%s\",\"params\":{\"startIdx\":%d,\"endIdx\":%d",
                    funcName, (int)startIdx, (int)endIdx);

    /* Count real inputs for naming (inReal vs inReal0/inReal1) */
    int nbRealInputs = 0;
    for( unsigned int i = 0; i < fi->nbInput; i++ )
    {
        const TA_InputParameterInfo *info;
        TA_GetInputParameterInfo(handle, i, &info);
        if( info->type == TA_Input_Real )
            nbRealInputs++;
    }

    /* Serialize inputs */
    int inputIdx = 0;
    int realInputCount = 0;
    for( unsigned int i = 0; i < fi->nbInput; i++ )
    {
        const TA_InputParameterInfo *info;
        TA_GetInputParameterInfo(handle, i, &info);

        if( info->type == TA_Input_Real )
        {
            if( !inputs || !inputs[inputIdx] )
                return -1;

            const char *key;
            char keyBuf[32];
            if( nbRealInputs == 1 )
            {
                key = "inReal";
            }
            else
            {
                snprintf(keyBuf, sizeof(keyBuf), "inReal%d", realInputCount);
                key = keyBuf;
            }
            realInputCount++;

            pos = codegen_appendf(g_reqBuf, SV_BUF_SIZE, pos, ",\"%s\":", key);
            pos = codegen_write_hexbits_array(g_reqBuf, SV_BUF_SIZE, pos,
                                               inputs[inputIdx], nbBars);
            inputIdx++;
        }
        else if( info->type == TA_Input_Price )
        {
            for( int c = 0; c < NUM_PRICE_COMPONENTS; c++ )
            {
                if( info->flags & PRICE_COMPONENTS[c].flag )
                {
                    if( !inputs || !inputs[inputIdx] )
                        return -1;
                    pos = codegen_appendf(g_reqBuf, SV_BUF_SIZE, pos,
                                   ",\"%s\":", PRICE_COMPONENTS[c].jsonKey);
                    pos = codegen_write_hexbits_array(g_reqBuf, SV_BUF_SIZE, pos,
                                                       inputs[inputIdx], nbBars);
                    inputIdx++;
                }
            }
        }
        else if( info->type == TA_Input_Integer )
        {
            /* Rare: integer input arrays. Treat same as real for serialization. */
            if( !inputs || !inputs[inputIdx] )
                return -1;
            pos = codegen_appendf(g_reqBuf, SV_BUF_SIZE, pos, ",\"%s\":", info->paramName);
            pos = codegen_write_hexbits_array(g_reqBuf, SV_BUF_SIZE, pos,
                                               inputs[inputIdx], nbBars);
            inputIdx++;
        }
    }

    /* Serialize optional params */
    int optIdx = 0;
    for( unsigned int i = 0; i < fi->nbOptInput; i++ )
    {
        const TA_OptInputParameterInfo *optInfo;
        TA_GetOptInputParameterInfo(handle, i, &optInfo);

        double val = (optParams && optIdx < nbOptParams)
                     ? optParams[optIdx] : optInfo->defaultValue;
        optIdx++;

        if( optInfo->type == TA_OptInput_IntegerRange ||
            optInfo->type == TA_OptInput_IntegerList )
        {
            pos = codegen_appendf(g_reqBuf, SV_BUF_SIZE, pos,
                            ",\"%s\":%d", optInfo->paramName, (int)val);
        }
        else
        {
            pos = codegen_appendf(g_reqBuf, SV_BUF_SIZE, pos,
                            ",\"%s\":%.15g", optInfo->paramName, val);
        }
    }

    /* Embed unstable period inline — server handlers read this field and call
     * TA_SetUnstablePeriod before executing the function. Without it, the
     * handler resets the unstable period to 0. */
    if( fi->flags & TA_FUNC_FLG_UNST_PER )
    {
        TA_FuncUnstId unstId = sv_func_unst_id(funcName);
        int unstPeriod = (unstId != TA_TEST_UNST_NONE)
                         ? (int)TA_GetUnstablePeriod(unstId) : 0;
        pos = codegen_appendf(g_reqBuf, SV_BUF_SIZE, pos,
                        ",\"unstablePeriod\":%d", unstPeriod);
    }

    if( wantHash )
        pos = codegen_appendf(g_reqBuf, SV_BUF_SIZE, pos, ",\"want_hash\":1");

    pos = codegen_appendf(g_reqBuf, SV_BUF_SIZE, pos, "}}");
    return pos;
}

/* ---- Golden output hash (in-process C outputs, logical order) ---- */

/* Reconstruct the C outputs in ta_abstract (logical) order, mapping the test's
 * separate outReal[]/outInteger[] per-type buffer lists into the flat
 * outBufs[3]/isInt[3] the shared codegen_* core consumes. Returns nbOutput (0 if
 * the function is unknown). The lists must be complete (one buffer per output) —
 * both codegen_output_hash and codegen_compare_tol index every output. */
static unsigned int sv_output_bufs(const char *funcName,
                                   const TA_Real *outReal[],
                                   const TA_Integer *outInteger[],
                                   const void *bufs[3], int isInt[3])
{
    const TA_FuncHandle *handle;
    const TA_FuncInfo   *fi;
    if( TA_GetFuncHandle(funcName, &handle) != TA_SUCCESS ) return 0;
    if( TA_GetFuncInfo(handle, &fi) != TA_SUCCESS ) return 0;

    int realIdx = 0, intIdx = 0;
    for( unsigned int o = 0; o < fi->nbOutput && o < 3; o++ )
    {
        const TA_OutputParameterInfo *oinfo;
        TA_GetOutputParameterInfo(handle, o, &oinfo);
        if( oinfo->type == TA_Output_Integer )
        {
            isInt[o] = 1;
            bufs[o]  = outInteger ? (const void *)outInteger[intIdx++] : NULL;
        }
        else
        {
            isInt[o] = 0;
            bufs[o]  = outReal ? (const void *)outReal[realIdx++] : NULL;
        }
    }
    return fi->nbOutput;
}

/* Hash the C reference outputs the test already computed, in the same logical
 * order (and byte layout) the servers hash their out_hash — via the shared
 * codegen_output_hash. */
static unsigned long long sv_golden_hash(const char *funcName,
                                         TA_Integer nbElement,
                                         const TA_Real *outReal[],
                                         const TA_Integer *outInteger[])
{
    const void *bufs[3];
    int isInt[3];
    unsigned int nbOutput = sv_output_bufs(funcName, outReal, outInteger, bufs, isInt);
    if( nbOutput == 0 ) return 0;
    return codegen_output_hash(nbOutput, isInt, bufs, (int)nbElement);
}

/* ---- Tolerance-based element compare (Java transcendental path only) ----
 *
 * fdlibm != system libm means Java cannot be bit-compared on the transcendental
 * calls, so its output arrays are element-compared at `tol` (integers still
 * exact) via the shared codegen_compare_tol — the same primitive --xlang-hash's
 * Java leg uses. This wrapper just reconstructs the C outputs in logical order
 * and translates the verdict into server_verify's messages and error codes. */
static ErrorNumber compare_output_tol(const char *funcName,
                                      const char *resp,
                                      TA_RetCode crefRetCode,
                                      TA_Integer crefOutBegIdx,
                                      TA_Integer crefOutNbElement,
                                      const TA_Real *outReal[],
                                      const TA_Integer *outInteger[],
                                      double tol)
{
    const void *bufs[3];
    int isInt[3];
    unsigned int nbOutput = sv_output_bufs(funcName, outReal, outInteger, bufs, isInt);
    if( nbOutput == 0 )
        return TA_TEST_PASS;   /* not in ta_abstract — graceful skip */

    CTolDetail d;
    CTolVerdict v = codegen_compare_tol(resp, nbOutput, isInt, bufs,
                                        crefRetCode, (int)crefOutBegIdx,
                                        (int)crefOutNbElement, tol, &d);
    switch( v )
    {
    case CTOL_MATCH:
        return TA_TEST_PASS;
    case CTOL_RETCODE:
        printf("  SV FAIL [%s] (pipe %d): retCode C=%d server=%d\n",
               funcName, g_curPipe, (int)crefRetCode, d.rc);
        return TA_SV_RETCODE_MISMATCH;
    case CTOL_SHAPE:
        if( d.begIdx != (int)crefOutBegIdx )
        {
            printf("  SV FAIL [%s] (pipe %d): outBegIdx C=%d server=%d\n",
                   funcName, g_curPipe, (int)crefOutBegIdx, d.begIdx);
            return TA_SV_BEGIDX_MISMATCH;
        }
        printf("  SV FAIL [%s] (pipe %d): outNbElement C=%d server=%d\n",
               funcName, g_curPipe, (int)crefOutNbElement, d.nbElement);
        return TA_SV_NBELEMENT_MISMATCH;
    case CTOL_COUNT:
        printf("  SV FAIL [%s]: %soutput %d count C=%d server=%d\n",
               funcName, d.isInt ? "int " : "", d.output,
               (int)crefOutNbElement, d.srvCount);
        return TA_SV_OUTPUT_MISMATCH;
    case CTOL_VALUE:
    default:
        if( d.isInt )
            printf("  SV FAIL [%s]: int output %d [%d] C=%d server=%d\n",
                   funcName, d.output, d.element, d.cInt, d.sInt);
        else
            printf("  SV FAIL [%s]: output %d [%d] C=%.15g server=%.15g (diff=%.15g)\n",
                   funcName, d.output, d.element, d.cReal, d.sReal,
                   fabs(d.cReal - d.sReal));
        return TA_SV_OUTPUT_MISMATCH;
    }
}

/* ---- Main entry point ---- */

ErrorNumber server_verify(
    const char       *funcName,
    TA_Integer        startIdx,
    TA_Integer        endIdx,
    int               nbBars,
    TA_RetCode        crefRetCode,
    TA_Integer        crefOutBegIdx,
    TA_Integer        crefOutNbElement,
    const TA_Real    *inputs[],
    const double      optParams[],
    int               nbOptParams,
    const TA_Real    *outReal[],
    const TA_Integer *outInteger[])
{
    if( g_nbPipes == 0 )
        return TA_TEST_PASS;

    /* Skip server verification for error cases (parameter validation
     * is implementation-specific and may differ between C and server). */
    if( crefRetCode != TA_SUCCESS )
        return TA_TEST_PASS;

    /* Golden hash of the C reference outputs (bitwise path). */
    unsigned long long goldHash =
        sv_golden_hash(funcName, crefOutNbElement, outReal, outInteger);

    /* Decide transcendental-ness once (per call — it depends on the MAType
     * argument, not just the function name). Only Java relaxes to a tolerance
     * for these; C/Rust/C# stay bitwise even here. */
    const TA_FuncHandle *handle = NULL;
    int isTranscendental = 0;
    if( TA_GetFuncHandle(funcName, &handle) == TA_SUCCESS )
        isTranscendental = codegen_call_is_transcendental(handle,
                                                          optParams, nbOptParams);

    /* Send to each active server and compare. Each pipe gets its own request:
     * the bitwise pipes ask for want_hash, the Java-transcendental pipe asks for
     * arrays (tolerance path). */
    for( int p = 0; p < g_nbPipes; p++ )
    {
        g_curPipe = p;
        const char *lang = g_pipeLang[p];
        /* Shared predicate, not a hardcoded "java": this line and the
         * --xlang-hash server table used to carry the rule separately and
         * drifted apart, leaving C# bitwise here and tolerant there. */
        int bitwise = !(codegen_lang_needs_transcendental_tol(lang) && isTranscendental);

        /* Sync global state (unstable periods + compatibility) */
        ErrorNumber err = sync_unstable_periods(p);
        if( err != TA_TEST_PASS )
        {
            printf("  SV WARN [%s]: failed to sync unstable periods\n", funcName);
            continue;
        }
        if( TA_GetCompatibility() != TA_COMPATIBILITY_DEFAULT &&
            !codegen_lang_has_compatibility_api(lang) )
        {
            /* A Metastock leg, against a backend that deliberately has no
             * compatibility API. Not a coverage gap deferred: TA_SetCompatibility
             * is deprecated in C and unreachable from Rust/Java/C# by design, so
             * there is no second implementation to compare against — permanently,
             * for all three. Silent because it is an invariant, not news. */
            continue;
        }
        err = sync_compatibility(p);
        if( err != TA_TEST_PASS )
        {
            printf("  SV WARN [%s]: failed to sync compatibility\n", funcName);
            continue;
        }
        err = sync_candle_settings(p);
        if( err != TA_TEST_PASS )
        {
            printf("  SV WARN [%s]: failed to sync candle settings\n", funcName);
            continue;
        }

        /* Build JSON request using ta_abstract metadata */
        int reqLen = build_request(funcName, startIdx, endIdx, nbBars,
                                   inputs, optParams, nbOptParams, bitwise);
        if( reqLen < 0 )
        {
            printf("  SV WARN [%s]: failed to build request (function not in ta_abstract?)\n",
                   funcName);
            return TA_TEST_PASS; /* graceful skip — same for every pipe */
        }

        /* Send function call */
        err = codegen_pipe_call(g_pipes[p], g_reqBuf, g_respBuf, SV_BUF_SIZE);
        if( err != TA_TEST_PASS )
        {
            printf("  SV FAIL [%s] (pipe %d): pipe call failed\n", funcName, p);
            return err;
        }

        /* An error response is a FAILURE, not a skip. This used to `continue`
         * on the reasoning that it never happens; a proxy erroring every
         * TA_CDL* call then produced a byte-identical green run, because a
         * skipped comparison and a passed one look the same from outside.
         * A response with no out_hash is already a hard failure two branches
         * below -- an outright error must not be treated more leniently. */
        if( sv_json_is_error(g_respBuf) )
        {
            printf("  SV FAIL [%s] (pipe %d, %s): server returned an error: %.160s\n",
                   funcName, p, lang ? lang : "?", g_respBuf);
            return TA_SV_RETCODE_MISMATCH;
        }

        if( bitwise )
        {
            XHashParsed hp;
            XHashVerdict v = codegen_hash_compare(g_respBuf, crefRetCode,
                                                  (int)crefOutBegIdx,
                                                  (int)crefOutNbElement, goldHash, &hp);
            if( v == XHASH_NO_HASH )
            {
                printf("  SV FAIL [%s] (pipe %d, %s): response has no out_hash "
                       "(server lacks want_hash support?)\n",
                       funcName, p, g_pipeLang[p] ? g_pipeLang[p] : "?");
                return TA_SV_OUTPUT_MISMATCH;
            }
            if( v != XHASH_MATCH )
            {
                printf("  SV FAIL [%s] (pipe %d, %s): BITWISE mismatch vs in-process C\n",
                       funcName, p, g_pipeLang[p] ? g_pipeLang[p] : "?");
                codegen_hash_report(g_pipeLang[p] ? g_pipeLang[p] : "server",
                                    crefRetCode, (int)crefOutBegIdx,
                                    (int)crefOutNbElement, goldHash, &hp);
                return (v == XHASH_RETCODE) ? TA_SV_RETCODE_MISMATCH
                     : (v == XHASH_SHAPE)   ? TA_SV_NBELEMENT_MISMATCH
                                            : TA_SV_OUTPUT_MISMATCH;
            }
            g_comparisons++;
        }
        else
        {
            /* Java transcendental: element compare at the narrow tolerance. */
            err = compare_output_tol(funcName, g_respBuf,
                                     crefRetCode, crefOutBegIdx, crefOutNbElement,
                                     outReal, outInteger, CODEGEN_TRANSCENDENTAL_TOL);
            if( err != TA_TEST_PASS )
                return err;
            g_comparisons++;
        }
    }

    return TA_TEST_PASS;
}

/* ---- Lookback-tier vs batch-tier agreement, per server (issue #256) ----
 * See the doc comment in server_verify.h for the property this checks and how
 * it differs from server_verify() above. */

/* Build the abstract_get_lookback request for `funcName` at `optParams` into
 * g_reqBuf. Mirrors build_request()'s optional-parameter serialization. */
static void build_lookback_request(const TA_FuncHandle *handle,
                                   const TA_FuncInfo *fi,
                                   const char *funcName,
                                   const double optParams[], int nbOptParams)
{
    int pos = codegen_appendf(g_reqBuf, SV_BUF_SIZE, 0,
        "{\"method\":\"abstract_get_lookback\",\"params\":{\"funcName\":\"%s\"", funcName);
    for( unsigned int i = 0; i < fi->nbOptInput; i++ )
    {
        const TA_OptInputParameterInfo *oi;
        TA_GetOptInputParameterInfo(handle, i, &oi);
        double v = (optParams && (int)i < nbOptParams) ? optParams[i] : oi->defaultValue;
        if( oi->type == TA_OptInput_RealRange || oi->type == TA_OptInput_RealList )
            pos = codegen_appendf(g_reqBuf, SV_BUF_SIZE, pos, ",\"%s\":%.15g", oi->paramName, v);
        else
            pos = codegen_appendf(g_reqBuf, SV_BUF_SIZE, pos, ",\"%s\":%d", oi->paramName, (int)v);
    }
    codegen_appendf(g_reqBuf, SV_BUF_SIZE, pos, "}}");
}

ErrorNumber server_verify_lookback_parity(
    const char    *funcName,
    TA_Integer     startIdx,
    TA_Integer     endIdx,
    int            nbBars,
    const TA_Real *inputs[],
    const double   optParams[],
    int            nbOptParams)
{
    if( g_nbPipes == 0 )
        return TA_TEST_PASS;

    const TA_FuncHandle *handle;
    const TA_FuncInfo   *fi;
    if( TA_GetFuncHandle(funcName, &handle) != TA_SUCCESS ) return TA_TEST_PASS;
    if( TA_GetFuncInfo(handle, &fi) != TA_SUCCESS ) return TA_TEST_PASS;

    for( int p = 0; p < g_nbPipes; p++ )
    {
        g_curPipe = p;
        const char *lang = g_pipeLang[p];

        ErrorNumber err = sync_unstable_periods(p);
        if( err != TA_TEST_PASS )
        {
            printf("  SV WARN [%s]: lookback-parity: failed to sync unstable periods\n", funcName);
            continue;
        }
        if( TA_GetCompatibility() != TA_COMPATIBILITY_DEFAULT &&
            !codegen_lang_has_compatibility_api(lang) )
            continue;   /* same graceful skip as server_verify() above */
        err = sync_compatibility(p);
        if( err != TA_TEST_PASS )
        {
            printf("  SV WARN [%s]: lookback-parity: failed to sync compatibility\n", funcName);
            continue;
        }
        err = sync_candle_settings(p);
        if( err != TA_TEST_PASS )
        {
            printf("  SV WARN [%s]: lookback-parity: failed to sync candle settings\n", funcName);
            continue;
        }

        /* ---- lookback tier ---- */
        build_lookback_request(handle, fi, funcName, optParams, nbOptParams);
        err = codegen_pipe_call(g_pipes[p], g_reqBuf, g_respBuf, SV_BUF_SIZE);
        if( err != TA_TEST_PASS )
        {
            printf("  SV FAIL [%s] (pipe %d): lookback-parity lookback call failed\n", funcName, p);
            return err;
        }
        int lbLen;
        if( !json_find_field(g_respBuf, "lookback", &lbLen) )
        {
            printf("  SV FAIL [%s] (pipe %d, %s): abstract_get_lookback response has no "
                   "lookback field (%.120s)\n", funcName, p, lang ? lang : "?", g_respBuf);
            return TA_SV_OUTPUT_MISMATCH;
        }
        int lookback = json_get_int(g_respBuf, "lookback");
        int lbRejected = (lookback < 0);

        /* ---- batch tier, same vector, same server. want_hash=1 to keep the
         * response small -- the output VALUES are irrelevant here, only retCode. */
        int reqLen = build_request(funcName, startIdx, endIdx, nbBars,
                                   inputs, optParams, nbOptParams, /*wantHash=*/1);
        if( reqLen < 0 )
        {
            printf("  SV WARN [%s]: lookback-parity: failed to build batch request\n", funcName);
            return TA_TEST_PASS;   /* not in ta_abstract -- graceful skip */
        }
        err = codegen_pipe_call(g_pipes[p], g_reqBuf, g_respBuf, SV_BUF_SIZE);
        if( err != TA_TEST_PASS )
        {
            printf("  SV FAIL [%s] (pipe %d): lookback-parity batch call failed\n", funcName, p);
            return err;
        }
        int rcLen;
        if( !json_find_field(g_respBuf, "retCode", &rcLen) )
        {
            printf("  SV FAIL [%s] (pipe %d, %s): batch response has no retCode field "
                   "(%.120s)\n", funcName, p, lang ? lang : "?", g_respBuf);
            return TA_SV_OUTPUT_MISMATCH;
        }
        int retCode = json_get_int(g_respBuf, "retCode");
        int batchRejected = (retCode != TA_SUCCESS);

        g_comparisons++;
        if( lbRejected != batchRejected )
        {
            printf("  SV FAIL [%s] (pipe %d, %s): lookback/batch tier disagree -- "
                   "lookback=%d (%s), batch retCode=%d (%s)\n",
                   funcName, p, lang ? lang : "?",
                   lookback, lbRejected ? "rejected" : "accepted",
                   retCode, batchRejected ? "rejected" : "accepted");
            return TA_SV_LOOKBACK_PARITY_MISMATCH;
        }
    }

    return TA_TEST_PASS;
}
