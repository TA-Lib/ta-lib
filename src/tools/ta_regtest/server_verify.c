/* server_verify.c — Verify hand-written C test results against JSON-RPC servers.
 *
 * Uses ta_abstract metadata to build JSON requests internally, so test files
 * only pass inputs/params/outputs in signature order without any JSON knowledge.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "server_verify.h"
#include "ta_abstract.h"

/* ---- Configuration ---- */

#define SV_BUF_SIZE (256 * 1024)   /* 256KB request/response buffers */
#define SV_MAX_OUTPUT 512          /* max output elements per call */
#define SV_EPSILON 1e-6            /* absolute tolerance for real comparison */

/* ---- Global state ---- */

static CodegenPipe *g_pipes[SV_MAX_PIPES];
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
    {"IMI",          TA_FUNC_UNST_IMI},
    {"KAMA",         TA_FUNC_UNST_KAMA},
    {"MAMA",         TA_FUNC_UNST_MAMA},
    {"MFI",          TA_FUNC_UNST_MFI},
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

/* ---- Minimal JSON helpers ---- */

static int sv_json_write_double_array(char *buf, int buf_size,
                                      const TA_Real *data, int count)
{
    int pos = 0;
    buf[pos++] = '[';
    for( int i = 0; i < count; i++ )
    {
        if( i > 0 )
            pos += snprintf(buf + pos, buf_size - pos, ",");
        pos += snprintf(buf + pos, buf_size - pos, "%.15g", data[i]);
    }
    buf[pos++] = ']';
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

void server_verify_init(CodegenPipe *pipes[], int nbPipes)
{
    g_nbPipes = 0;
    for( int i = 0; i < nbPipes && i < SV_MAX_PIPES; i++ )
    {
        if( pipes[i] )
            g_pipes[g_nbPipes++] = pipes[i];
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

/* ---- Build JSON request from ta_abstract metadata ---- */

static int build_request(const char *funcName,
                         TA_Integer startIdx, TA_Integer endIdx,
                         int nbBars,
                         const TA_Real *inputs[],
                         const double optParams[], int nbOptParams)
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
            pos += sv_json_write_double_array(g_reqBuf + pos, SV_BUF_SIZE - pos,
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
                    pos += sv_json_write_double_array(g_reqBuf + pos, SV_BUF_SIZE - pos,
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
            pos += sv_json_write_double_array(g_reqBuf + pos, SV_BUF_SIZE - pos,
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

    pos += snprintf(g_reqBuf + pos, SV_BUF_SIZE - pos, "}}");
    return pos;
}

/* ---- Compare server response with C output ---- */

static ErrorNumber compare_output(const char *funcName,
                                  const char *resp,
                                  TA_RetCode crefRetCode,
                                  TA_Integer crefOutBegIdx,
                                  TA_Integer crefOutNbElement,
                                  const TA_Real *outReal[],
                                  const TA_Integer *outInteger[])
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
                double diff = fabs(outReal[outIdx][j] - srvBuf[j]);
                double absVal = fabs(outReal[outIdx][j]);
                double tol = (absVal > 1.0) ? SV_EPSILON * absVal : SV_EPSILON;
                if( diff > tol )
                {
                    printf("  SV FAIL [%s]: output %d [%d] C=%.15g server=%.15g (diff=%.15g)\n",
                           funcName, outIdx, j, outReal[outIdx][j], srvBuf[j], diff);
                    return TA_SV_OUTPUT_MISMATCH;
                }
            }
        }
    }

    /* Compare integer outputs */
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

    /* Build JSON request using ta_abstract metadata */
    int reqLen = build_request(funcName, startIdx, endIdx, nbBars,
                               inputs, optParams, nbOptParams);
    if( reqLen < 0 )
    {
        printf("  SV WARN [%s]: failed to build request (function not in ta_abstract?)\n",
               funcName);
        return TA_TEST_PASS; /* graceful skip */
    }

    /* Send to each active server and compare */
    for( int p = 0; p < g_nbPipes; p++ )
    {
        g_curPipe = p;
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

        /* Send function call */
        err = codegen_pipe_call(g_pipes[p], g_reqBuf, g_respBuf, SV_BUF_SIZE);
        if( err != TA_TEST_PASS )
        {
            printf("  SV WARN [%s]: pipe call failed\n", funcName);
            continue;
        }

        /* Skip if server doesn't know this function */
        if( sv_json_is_error(g_respBuf) )
        {
            /* Not a failure — server may not implement this function yet */
            continue;
        }

        /* Compare output */
        err = compare_output(funcName, g_respBuf,
                             crefRetCode, crefOutBegIdx, crefOutNbElement,
                             outReal, outInteger);
        if( err != TA_TEST_PASS )
            return err;
    }

    return TA_TEST_PASS;
}
