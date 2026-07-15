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
 * codegen_hash_compare(). Zero tolerance for C/Rust/.NET; a narrow tolerance for
 * Java on transcendental-using functions only (fdlibm != system libm ~1 ULP).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "server_verify.h"
#include "test_codegen.h"   /* codegen_output_hash / codegen_hash_compare / XHash* */
#include "ta_abstract.h"

/* ---- Configuration ---- */

#define SV_BUF_SIZE (256 * 1024)   /* 256KB request/response buffers */
#define SV_MAX_OUTPUT 512          /* max output elements per call */

/* Java's fdlibm differs from the C libm by ~1 ULP on transcendentals, so the
 * transcendental-using functions cannot be bit-compared against Java — they get
 * this narrow tolerance instead (relative for |v|>1, absolute otherwise). Every
 * other language, and every non-transcendental function, is compared BITWISE
 * (zero tolerance) via the out_hash path. Measured drift over the hard-coded
 * scenarios: SIN/MA/MAVP/MAMA ~1.2e-16, HT_DCPHASE ~9.7e-15 (max). 1e-9 (== the
 * codegen double-leg CODEGEN_EPSILON_DOUBLE) leaves ~5 orders of margin over that
 * fdlibm noise while still failing any real algorithmic regression, which would
 * be orders of magnitude larger. */
#define SV_JAVA_TRANSCENDENTAL_TOL 1e-9

/* ---- Global state ---- */

static CodegenPipe *g_pipes[SV_MAX_PIPES];
static const char  *g_pipeLang[SV_MAX_PIPES];          /* "c"/"rust"/"java"/"dotnet" */
static int          g_nbPipes = 0;
static char        *g_reqBuf  = NULL;
static char        *g_respBuf = NULL;
static int          g_lastCompatibility[SV_MAX_PIPES]; /* cached per pipe */
static int          g_curPipe = -1; /* pipe being verified (for diagnostics) */

/* Unstable period lookup table (same as test_codegen.c) */
typedef struct { const char *name; TA_FuncUnstId id; } UnstableLookup;
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
    /* IMI and MFI are intentionally omitted: both are finite sliding-window
     * indicators reclassified as stable (no TA_FUNC_FLG_UNST_PER), and their
     * lookback no longer consults the unstable period. Keep in sync with the
     * UNSTABLE_MAP in test_codegen.c. */
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
};
#define NUM_UNSTABLE_MAP (sizeof(UNSTABLE_MAP) / sizeof(UNSTABLE_MAP[0]))

static TA_FuncUnstId sv_func_unst_id(const char *name)
{
    for( unsigned int i = 0; i < NUM_UNSTABLE_MAP; i++ )
    {
        if( strcmp(UNSTABLE_MAP[i].name, name) == 0 )
            return UNSTABLE_MAP[i].id;
    }
    return TA_FUNC_UNST_NONE;
}

/* Functions that call a transcendental C math routine (atan/sin/cos/exp/log/...).
 * Derived from a source grep of ta_codegen/input; these are the only functions
 * whose output can differ between Java's fdlibm and the C libm (~1 ULP). Every
 * other function — including sqrt/ceil/floor users (IEEE correctly-rounded) — is
 * bit-identical across languages and compared with ZERO tolerance. */
static const char *const SV_TRANSCENDENTAL[] = {
    "ACOS", "ASIN", "ATAN", "COS", "COSH", "EXP",
    "HT_DCPERIOD", "HT_DCPHASE", "HT_PHASOR", "HT_SINE", "HT_TRENDLINE",
    "HT_TRENDMODE", "LINEARREG_ANGLE", "LN", "LOG10", "MAMA",
    "SIN", "SINH", "TAN", "TANH",
};
#define NUM_SV_TRANSCENDENTAL (sizeof(SV_TRANSCENDENTAL) / sizeof(SV_TRANSCENDENTAL[0]))

static int sv_is_transcendental(const char *name)
{
    for( unsigned int i = 0; i < NUM_SV_TRANSCENDENTAL; i++ )
        if( strcmp(SV_TRANSCENDENTAL[i], name) == 0 )
            return 1;
    return 0;
}

/* The MA-dispatch functions (MA, MAVP, BBANDS, MACDEXT, APO, PPO, STOCH*)
 * route to MAMA — which uses atan — when a MAType optional parameter selects it.
 * Such a CALL is transcendental for Java even though the function name is not, so
 * transcendental-ness must be decided per call, inspecting the MAType value.
 * TA_MAType_MAMA == 7 (enums.yaml); MAType params are the only optional inputs
 * whose paramName contains "MAType". */
#define SV_MATYPE_MAMA 7
static int sv_call_is_transcendental(const char *funcName,
                                     const TA_FuncHandle *handle,
                                     const double optParams[], int nbOptParams)
{
    if( sv_is_transcendental(funcName) )
        return 1;

    const TA_FuncInfo *fi;
    if( TA_GetFuncInfo(handle, &fi) != TA_SUCCESS )
        return 0;
    int optIdx = 0;
    for( unsigned int i = 0; i < fi->nbOptInput; i++ )
    {
        const TA_OptInputParameterInfo *oi;
        TA_GetOptInputParameterInfo(handle, i, &oi);
        /* Mirror build_request's optParams indexing (one slot per optInput,
         * default beyond nbOptParams). */
        double val = (optParams && optIdx < nbOptParams) ? optParams[optIdx]
                                                         : oi->defaultValue;
        optIdx++;
        if( oi->paramName && strstr(oi->paramName, "MAType") &&
            (int)val == SV_MATYPE_MAMA )
            return 1;
    }
    return 0;
}

/* ---- Minimal JSON helpers ---- */

/* Lossless input transport (issue #115): serialize each double as its 16-hex-char
 * IEEE-754 bit pattern inside one JSON string, decoded byte-exactly by every
 * server's array parser. Replaces the old %.15g writer so C<=>server comparison
 * has no serialization rounding to hide a divergence behind. */
static int sv_json_write_hexbits_array(char *buf, int buf_size,
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

static const char *sv_json_find_field(const char *json, const char *field, int *len)
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

static int sv_json_get_int(const char *json, const char *field)
{
    int len;
    const char *val = sv_json_find_field(json, field, &len);
    if( !val ) return 0;
    return atoi(val);
}

static int sv_json_get_double_array(const char *json, const char *field,
                                    TA_Real *out, int max_count)
{
    int len;
    const char *val = sv_json_find_field(json, field, &len);
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

static int sv_json_get_int_array(const char *json, const char *field,
                                 TA_Integer *out, int max_count)
{
    int len;
    const char *val = sv_json_find_field(json, field, &len);
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
static unsigned int g_lastUnstPeriods[SV_MAX_PIPES][TA_FUNC_UNST_ALL];
static int          g_unstInitialized[SV_MAX_PIPES];

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
        for( int p = 0; p < SV_MAX_PIPES; p++ )
        {
            g_lastCompatibility[p] = -1;
            g_unstInitialized[p] = 0;
        }
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
        g_pipeLang[p] = NULL;
    }
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
    for( int id = 0; id < (int)TA_FUNC_UNST_ALL; id++ )
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
 * its raw outputs instead of %.15g arrays (the bitwise path); otherwise it
 * returns arrays (the Java-transcendental tolerance path). Returns -1 if the
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
    pos += snprintf(g_reqBuf + pos, SV_BUF_SIZE - pos,
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

            pos += snprintf(g_reqBuf + pos, SV_BUF_SIZE - pos, ",\"%s\":", key);
            pos += sv_json_write_hexbits_array(g_reqBuf + pos, SV_BUF_SIZE - pos,
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
                    pos += snprintf(g_reqBuf + pos, SV_BUF_SIZE - pos,
                                   ",\"%s\":", PRICE_COMPONENTS[c].jsonKey);
                    pos += sv_json_write_hexbits_array(g_reqBuf + pos, SV_BUF_SIZE - pos,
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
            pos += snprintf(g_reqBuf + pos, SV_BUF_SIZE - pos, ",\"%s\":", info->paramName);
            pos += sv_json_write_hexbits_array(g_reqBuf + pos, SV_BUF_SIZE - pos,
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
            pos += snprintf(g_reqBuf + pos, SV_BUF_SIZE - pos,
                            ",\"%s\":%d", optInfo->paramName, (int)val);
        }
        else
        {
            pos += snprintf(g_reqBuf + pos, SV_BUF_SIZE - pos,
                            ",\"%s\":%.15g", optInfo->paramName, val);
        }
    }

    /* Embed unstable period inline — server handlers read this field and call
     * TA_SetUnstablePeriod before executing the function. Without it, the
     * handler resets the unstable period to 0. */
    if( fi->flags & TA_FUNC_FLG_UNST_PER )
    {
        TA_FuncUnstId unstId = sv_func_unst_id(funcName);
        int unstPeriod = (unstId != TA_FUNC_UNST_NONE)
                         ? (int)TA_GetUnstablePeriod(unstId) : 0;
        pos += snprintf(g_reqBuf + pos, SV_BUF_SIZE - pos,
                        ",\"unstablePeriod\":%d", unstPeriod);
    }

    if( wantHash )
        pos += snprintf(g_reqBuf + pos, SV_BUF_SIZE - pos, ",\"want_hash\":1");

    pos += snprintf(g_reqBuf + pos, SV_BUF_SIZE - pos, "}}");
    return pos;
}

/* ---- Golden output hash (in-process C outputs, logical order) ---- */

/* Hash the C reference outputs the test already computed, in the same logical
 * order (and byte layout) the servers hash their out_hash — via the shared
 * codegen_output_hash. outReal[]/outInteger[] are the test's NULL-terminated
 * per-type buffer lists; ta_abstract tells us each output's type and order. */
static unsigned long long sv_golden_hash(const char *funcName,
                                         TA_Integer nbElement,
                                         const TA_Real *outReal[],
                                         const TA_Integer *outInteger[])
{
    const TA_FuncHandle *handle;
    const TA_FuncInfo   *fi;
    if( TA_GetFuncHandle(funcName, &handle) != TA_SUCCESS ) return 0;
    if( TA_GetFuncInfo(handle, &fi) != TA_SUCCESS ) return 0;

    const void *bufs[3];
    int isInt[3];
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
    return codegen_output_hash(fi->nbOutput, isInt, bufs, (int)nbElement);
}

/* ---- Tolerance-based element compare (Java transcendental path only) ----
 *
 * fdlibm != system libm means Java cannot be bit-compared on the transcendental
 * functions, so we parse its %.15g arrays and compare element-wise at `tol`.
 * Integer outputs (e.g. HT_TRENDMODE) must still match EXACTLY. */
static ErrorNumber compare_output_tol(const char *funcName,
                                      const char *resp,
                                      TA_RetCode crefRetCode,
                                      TA_Integer crefOutBegIdx,
                                      TA_Integer crefOutNbElement,
                                      const TA_Real *outReal[],
                                      const TA_Integer *outInteger[],
                                      double tol)
{
    int srvRetCode = sv_json_get_int(resp, "retCode");
    if( srvRetCode != (int)crefRetCode )
    {
        printf("  SV FAIL [%s] (pipe %d): retCode C=%d server=%d\n",
               funcName, g_curPipe, (int)crefRetCode, srvRetCode);
        return TA_SV_RETCODE_MISMATCH;
    }

    /* If both returned error, no output to compare */
    if( crefRetCode != TA_SUCCESS )
        return TA_TEST_PASS;

    int srvBegIdx = sv_json_get_int(resp, "outBegIdx");
    int srvNbElement = sv_json_get_int(resp, "outNBElement");

    if( srvBegIdx != (int)crefOutBegIdx )
    {
        printf("  SV FAIL [%s] (pipe %d): outBegIdx C=%d server=%d\n",
               funcName, g_curPipe, (int)crefOutBegIdx, srvBegIdx);
        return TA_SV_BEGIDX_MISMATCH;
    }

    if( srvNbElement != (int)crefOutNbElement )
    {
        printf("  SV FAIL [%s] (pipe %d): outNbElement C=%d server=%d\n",
               funcName, g_curPipe, (int)crefOutNbElement, srvNbElement);
        return TA_SV_NBELEMENT_MISMATCH;
    }

    if( crefOutNbElement == 0 )
        return TA_TEST_PASS;

    /* Compare real outputs */
    if( outReal )
    {
        TA_Real srvBuf[SV_MAX_OUTPUT];
        for( int outIdx = 0; outReal[outIdx] != NULL; outIdx++ )
        {
            const char *key;
            char keyBuf[32];
            if( outIdx == 0 )
                key = "outReal";
            else
            {
                snprintf(keyBuf, sizeof(keyBuf), "outReal%d", outIdx);
                key = keyBuf;
            }

            int srvCount = sv_json_get_double_array(resp, key, srvBuf, SV_MAX_OUTPUT);
            if( srvCount != (int)crefOutNbElement )
            {
                printf("  SV FAIL [%s]: output %d count C=%d server=%d\n",
                       funcName, outIdx, (int)crefOutNbElement, srvCount);
                return TA_SV_OUTPUT_MISMATCH;
            }

            for( int j = 0; j < (int)crefOutNbElement; j++ )
            {
                double cVal = outReal[outIdx][j], sVal = srvBuf[j];
                double diff = fabs(cVal - sVal);
                double absVal = fabs(cVal);
                double t = (absVal > 1.0) ? tol * absVal : tol;
                /* NaN is unordered, so `diff > t` is false whenever a side is
                 * NaN — guard finite-vs-NaN explicitly so the tolerance path is
                 * as NaN-discriminating as the bitwise hash path (both-NaN agree,
                 * payload aside, and pass). */
                if( (isnan(cVal) != isnan(sVal)) || diff > t )
                {
                    printf("  SV FAIL [%s]: output %d [%d] C=%.15g server=%.15g (diff=%.15g)\n",
                           funcName, outIdx, j, cVal, sVal, diff);
                    return TA_SV_OUTPUT_MISMATCH;
                }
            }
        }
    }

    /* Compare integer outputs (exact) */
    if( outInteger )
    {
        TA_Integer srvIntBuf[SV_MAX_OUTPUT];
        for( int outIdx = 0; outInteger[outIdx] != NULL; outIdx++ )
        {
            const char *key;
            char keyBuf[32];
            if( outIdx == 0 )
                key = "outInteger";
            else
            {
                snprintf(keyBuf, sizeof(keyBuf), "outInteger%d", outIdx);
                key = keyBuf;
            }

            int srvCount = sv_json_get_int_array(resp, key, srvIntBuf, SV_MAX_OUTPUT);
            if( srvCount != (int)crefOutNbElement )
            {
                printf("  SV FAIL [%s]: int output %d count C=%d server=%d\n",
                       funcName, outIdx, (int)crefOutNbElement, srvCount);
                return TA_SV_OUTPUT_MISMATCH;
            }

            for( int j = 0; j < (int)crefOutNbElement; j++ )
            {
                if( outInteger[outIdx][j] != srvIntBuf[j] )
                {
                    printf("  SV FAIL [%s]: int output %d [%d] C=%d server=%d\n",
                           funcName, outIdx, j, outInteger[outIdx][j], srvIntBuf[j]);
                    return TA_SV_OUTPUT_MISMATCH;
                }
            }
        }
    }

    return TA_TEST_PASS;
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
     * for these; C/Rust/.NET stay bitwise even here. */
    const TA_FuncHandle *handle = NULL;
    int isTranscendental = 0;
    if( TA_GetFuncHandle(funcName, &handle) == TA_SUCCESS )
        isTranscendental = sv_call_is_transcendental(funcName, handle,
                                                     optParams, nbOptParams);

    /* Send to each active server and compare. Each pipe gets its own request:
     * the bitwise pipes ask for want_hash, the Java-transcendental pipe asks for
     * arrays (tolerance path). */
    for( int p = 0; p < g_nbPipes; p++ )
    {
        g_curPipe = p;
        const char *lang = g_pipeLang[p];
        int bitwise = !(lang && strcmp(lang, "java") == 0 && isTranscendental);

        /* Sync global state (unstable periods + compatibility) */
        ErrorNumber err = sync_unstable_periods(p);
        if( err != TA_TEST_PASS )
        {
            printf("  SV WARN [%s]: failed to sync unstable periods\n", funcName);
            continue;
        }
        err = sync_compatibility(p);
        if( err != TA_TEST_PASS )
        {
            printf("  SV WARN [%s]: failed to sync compatibility\n", funcName);
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
            printf("  SV WARN [%s]: pipe call failed\n", funcName);
            continue;
        }

        /* Skip if server doesn't know this function (never happens: all servers
         * carry all 161 — this is a safety net). */
        if( sv_json_is_error(g_respBuf) )
            continue;

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
        }
        else
        {
            /* Java transcendental: element compare at the narrow tolerance. */
            err = compare_output_tol(funcName, g_respBuf,
                                     crefRetCode, crefOutBegIdx, crefOutNbElement,
                                     outReal, outInteger, SV_JAVA_TRANSCENDENTAL_TOL);
            if( err != TA_TEST_PASS )
                return err;
        }
    }

    return TA_TEST_PASS;
}
