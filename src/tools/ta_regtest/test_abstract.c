/* TA-LIB Copyright (c) 1999-2026, Mario Fortier
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the following
 * conditions are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in
 *   the documentation and/or other materials provided with the
 *   distribution.
 *
 * - Neither name of author nor the names of its contributors
 *   may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * REGENTS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  AC       Angelo Ciceri
 *
 *
 * Change history:
 *
 *  MMDDYY BY   Description
 *  -------------------------------------------------------------------
 *  112703 MF   First version.
 *  030104 MF   Add tests for TA_GetLookback
 *  062504 MF   Add test_default_calls.
 *  110206 AC   Change volume and open interest to double
 *  082607 MF   Add profiling feature.
 */

/* Description:
 *         Regression testing of the functionality provided
 *         by the ta_abstract module.
 *
 *         Also perform call to all functions for the purpose
 *         of profiling (doExtensiveProfiling option).
 */

/**** Headers ****/
#ifdef WIN32
   #include "windows.h"
#else
   #include "time.h"
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include "ta_test_priv.h"

/**** External functions declarations. ****/
/* None */

/**** External variables declarations. ****/
extern int doExtensiveProfiling;

/* Optional codegen server pipe — when set, each C TA_CallFunc is
 * replicated via the server's abstract_call endpoint and compared. */
#include "codegen_pipe.h"
#include "ta_abstract.h"
#include "../ta_alloc_check.h"
static CodegenPipe *g_abstractPipe = NULL;
/* Which language the attached server is ("c"/"rust"/"java"/"csharp"), so a
 * per-backend carve-out can name its backend instead of applying to all four. */
static const char *g_abstractLang = NULL;

/* How many optional-parameter vectors were driven through a binder, and how many
 * of each contract class, across the whole run. */
static long long g_d2Vectors = 0;
static long long g_d2NonDefault = 0;
static long long g_d2Sentinel = 0;
static long long g_d2Reject = 0;
/* Non-finite parameter probes; see d2_nonfinite_params. */
static long long g_d2NonFinite = 0;
static long long g_d2NonFiniteFuncs = 0;
/* Self-checks, mirroring --xlang-hash's oorNotRejected / sentNotDefault. Both
 * classes assert only "C and the server agree" on their own; if C stopped
 * rejecting, or stopped substituting, the two tiers would be wrong TOGETHER and
 * the leg would pass while asserting nothing. */
static long long g_d2OorNotRejected = 0;
static long long g_d2SentNotDefault = 0;
static char        *g_abstractReqBuf = NULL;
static char        *g_abstractRespBuf = NULL;
#define ABSTRACT_JSON_BUF_SIZE (512 * 1024)

extern double gDataOpen[];
extern double gDataHigh[];
extern double gDataLow[];
extern double gDataClose[];

extern int nbProfiledCall;
extern double timeInProfiledCall;
extern double worstProfiledCall;
extern int insufficientClockPrecision;

/**** Global variables definitions.    ****/
/* None */

/**** Local declarations.              ****/
typedef enum
{
	PROFILING_10000,
	PROFILING_8000,
	PROFILING_5000,
    PROFILING_2000,
	PROFILING_1000,
	PROFILING_500,
	PROFILING_100
} ProfilingType;

/**** Local functions declarations.    ****/
static ErrorNumber testLookback(TA_ParamHolder *paramHolder );
static ErrorNumber test_default_calls(void);
static ErrorNumber callWithDefaults( const char *funcName,
									 const double *input,
									 const int *input_int, int size,
									 const char *datasetName );
static ErrorNumber callAndProfile( const char *funcName, ProfilingType type );

/**** Local variables definitions.     ****/
static double inputNegData[100];
static double inputZeroData[100];
static double inputRandFltEpsilon[100];
static double inputRandDblEpsilon[100];
static double inputRandomData[2000];

static int    inputNegData_int[100];
static int    inputZeroData_int[100];
static int    inputRandFltEpsilon_int[100];
static int    inputRandDblEpsilon_int[100];
static int    inputRandomData_int[2000];

static double output[10][2000];
static int    output_int[10][2000];

/**** Global functions definitions.   ****/

/* Set the optional codegen server pipe for abstract verification.
 * When set, callWithDefaults() will also call the server and compare. */
void test_abstract_set_server(CodegenPipe *cp, const char *lang)
{
   g_abstractLang = lang;
   /* Per-server, so the summary line reports the server it names rather than a
      running total across every server tested so far. */
   g_d2Vectors = g_d2NonDefault = g_d2Sentinel = g_d2Reject = 0;
   g_d2NonFinite = g_d2NonFiniteFuncs = 0;
   g_d2OorNotRejected = g_d2SentNotDefault = 0;
   if( cp )
   {
      g_abstractPipe = cp;
      g_abstractReqBuf = malloc(ABSTRACT_JSON_BUF_SIZE);
      g_abstractRespBuf = malloc(ABSTRACT_JSON_BUF_SIZE);
      TA_TOOL_CHECK_ALLOC(g_abstractReqBuf);
      TA_TOOL_CHECK_ALLOC(g_abstractRespBuf);
   }
   else
   {
      g_abstractPipe = NULL;
      free(g_abstractReqBuf);  g_abstractReqBuf = NULL;
      free(g_abstractRespBuf); g_abstractRespBuf = NULL;
   }
}

/* Minimal JSON helpers (same as test_codegen.c — needed for parsing server responses) */
static int abstract_json_get_int(const char *json, const char *field)
{
    char pattern[256];
    snprintf(pattern, sizeof(pattern), "\"%s\":", field);
    const char *p = strstr(json, pattern);
    if( !p ) return 0;
    p += strlen(pattern);
    while( *p == ' ' ) p++;
    return atoi(p);
}

static unsigned long long abstract_json_get_ull(const char *json, const char *field)
{
    char pattern[256];
    snprintf(pattern, sizeof(pattern), "\"%s\":", field);
    const char *p = strstr(json, pattern);
    if( !p ) return 0;
    p += strlen(pattern);
    while( *p == ' ' ) p++;
    return strtoull(p, NULL, 10);
}

/* Volume is a non-negative quantity, and every TA-Lib function that consumes it
 * assumes so. The generic datasets here are sign-agnostic noise (notably
 * inputRandFltEpsilon, which is a random-signed +/-FLT_EPSILON), so feeding them
 * unchanged into a volume slot tests an input that cannot occur.
 *
 * That is not merely wasteful, it is actively misleading: with random-signed
 * volume a trailing window sum can reach exactly 0.0 while the window is NOT
 * all-zero, which trips the degenerate-input guard of any Sum(x*v)/Sum(v)
 * indicator (VWMA) on data that would never arise. ta_abstract already records
 * which slot is volume (TA_IN_PRICE_VOLUME), so use it: hand volume slots the
 * magnitude only.
 *
 * Both the in-process call and the JSON request must use this same view or the
 * two sides diverge for reasons that have nothing to do with the function. */
static const double *abstract_volume_view( const double *input, unsigned int size )
{
    static double volBuf[2000];
    unsigned int i;
    if( size > (unsigned int)(sizeof(volBuf)/sizeof(volBuf[0])) )
        return input;   /* cannot buffer it; leave the caller's data alone */
    for( i = 0; i < size; i++ )
        volBuf[i] = input[i] < 0.0 ? -input[i] : input[i];
    return volBuf;
}

/* Every price slot used to receive the SAME array, which made the whole
 * component axis untestable: with inOpen == inHigh == inLow == inClose bit for
 * bit, any permutation of them is bit-identical, so a binder that transposed two
 * components — or indexed the wrong input slot — produced identical output and no
 * gate could see it. The C# server's own comment cites transposition-detection as
 * the reason its abstract_call is an independent implementation; that detection
 * only exists once the components actually differ.
 *
 * The components are built as a COHERENT bar — low <= min(open,close) <=
 * max(open,close) <= high — from neighbouring samples of the base series, so the
 * geometry varies bar to bar. Scaling each component by a constant factor instead
 * is the obvious approach and is wrong: every bar then has identical proportions,
 * so a candlestick's "body vs the average body over the period" comparison sits
 * exactly ON its threshold, and the residual ~1e-13 operation-ordering difference
 * between the C reference and a backend flips the whole 0/100 output. Deriving
 * from different indices keeps the comparisons away from their boundaries.
 *
 * Sign- and zero-preserving by construction (min/max pick real samples and the
 * padding uses magnitudes), so it stays meaningful on every dataset here: a ]0,1[
 * ramp, negatives, and the +/-epsilon noise. Close keeps the identity so
 * single-input and close-only functions see exactly what they did. Volume is
 * excluded — it has its own magnitude view for a separate reason.
 *
 * Both the in-process call and the JSON request must use this same view, or the
 * two sides diverge for reasons that have nothing to do with the function. */
/* The views must be able to buffer the LARGEST dataset, or they silently fall
 * back to handing every component the same array — reinstating exactly the
 * symmetry they exist to break, with every test still green. Tie the capacity to
 * the datasets so enlarging one is a compile error rather than a silent
 * un-gating. */
#define ABSTRACT_VIEW_MAX (sizeof(inputRandomData)/sizeof(double))
typedef char abstract_view_fits_largest_dataset[
    ABSTRACT_VIEW_MAX >= sizeof(inputNegData)/sizeof(double) ? 1 : -1];

static const double *abstract_price_view( const double *input, unsigned int size,
                                          TA_InputFlags component )
{
    static double openBuf[ABSTRACT_VIEW_MAX], highBuf[ABSTRACT_VIEW_MAX],
                  lowBuf[ABSTRACT_VIEW_MAX], oiBuf[ABSTRACT_VIEW_MAX];
    unsigned int i;

    if( size == 0 || size > (unsigned int)ABSTRACT_VIEW_MAX )
        return input;   /* unreachable: the static assert above sizes for every dataset */

    switch( component )
    {
    case TA_IN_PRICE_OPEN:
        for( i = 0; i < size; i++ ) openBuf[i] = input[(i+1) % size];
        return openBuf;

    case TA_IN_PRICE_HIGH:
        for( i = 0; i < size; i++ )
        {
            double c = input[i], o = input[(i+1) % size], pad = input[(i+2) % size];
            double top = c > o ? c : o;
            highBuf[i] = top + (pad < 0.0 ? -pad : pad) * 0.25;
        }
        return highBuf;

    case TA_IN_PRICE_LOW:
        for( i = 0; i < size; i++ )
        {
            double c = input[i], o = input[(i+1) % size], pad = input[(i+3) % size];
            double bottom = c < o ? c : o;
            lowBuf[i] = bottom - (pad < 0.0 ? -pad : pad) * 0.25;
        }
        return lowBuf;

    case TA_IN_PRICE_OPENINTEREST:
        for( i = 0; i < size; i++ ) oiBuf[i] = input[(i+2) % size];
        return oiBuf;

    case TA_IN_PRICE_CLOSE:        /* the identity — see above */
    default:                       return input;
    }
}

/* Same symmetry problem on the generic real inputs: a function taking inReal0 and
 * inReal1 (CORREL, BETA, the vector arithmetic) received one array in both slots,
 * so swapping them was invisible. Slot 0 keeps the identity. */
#define ABSTRACT_REAL_SLOTS 4
static const double *abstract_real_view( const double *input, unsigned int size,
                                         int slot )
{
    static double slotBuf[ABSTRACT_REAL_SLOTS][ABSTRACT_VIEW_MAX];
    unsigned int i;

    /* Slot 0 keeps the identity. Every other slot gets its OWN buffer and its own
     * factor — one shared buffer would alias slot 2 onto slot 1 and quietly
     * restore the invisible-swap hole for a three-input function. The tables top
     * out at Real0/Real1 today, so the upper slots are headroom, not dead code
     * waiting to be wrong. */
    if( slot <= 0 || slot >= ABSTRACT_REAL_SLOTS ) return input;
    if( size == 0 || size > (unsigned int)ABSTRACT_VIEW_MAX ) return input;
    for( i = 0; i < size; i++ )
        slotBuf[slot][i] = input[i] * (1.0 - 0.07 * (double)slot);
    return slotBuf[slot];
}

static int abstract_json_write_double_array(char *buf, int buf_size, int pos,
                                            const double *data, int count)
{
    pos = codegen_appendc(buf, buf_size, pos, '[');
    for( int i = 0; i < count; i++ )
    {
        if( i > 0 ) pos = codegen_appendc(buf, buf_size, pos, ',');
        /* %.17g, not %.15g: 17 significant digits is what round-trips a double.
         * At 15 the server computed on subtly different inputs from the C arm,
         * which is invisible while every price component carries the SAME array
         * (both sides round identically and the degenerate high==low comparisons
         * hold either way) and becomes a false mismatch the moment they differ —
         * a threshold function like a CDL* pattern flips its whole output on one
         * ULP. Value parity here is a claim about the code, not about printf. */
        pos = codegen_appendf(buf, buf_size, pos, "%.17g", data[i]);
    }
    return codegen_appendc(buf, buf_size, pos, ']');
}

static int abstract_json_is_error(const char *json)
{
    return strstr(json, "\"error\"") != NULL;
}

static double abstract_json_get_double(const char *json, const char *field)
{
    char pattern[256];
    snprintf(pattern, sizeof(pattern), "\"%s\":", field);
    const char *p = strstr(json, pattern);
    if( !p ) return 0.0;
    p += strlen(pattern);
    while( *p == ' ' ) p++;
    return strtod(p, NULL);
}

/* Real output arrays arrive as the lossless hex-bits string every ta_codegen
 * server writes (issues #257/#258): concatenated 16-hex-char groups, one per
 * f64's IEEE-754 bit pattern. The `[` arm reads a JSON number array — no
 * server in this tree writes one any more, and it is kept only so a driver
 * pointed at an older or third-party server reads values rather than none.
 * Same shape as test_codegen.c's json_get_double_array, which carries the
 * longer note. */
static int abstract_json_get_double_array(const char *json, const char *field,
                                          double *out, int max_count)
{
    char pattern[256];
    snprintf(pattern, sizeof(pattern), "\"%s\":", field);
    const char *p = strstr(json, pattern);
    if( !p ) return 0;
    p += strlen(pattern);
    while( *p == ' ' ) p++;
    int count = 0;
    if( *p == '"' ) {
        p++;
        while( count < max_count && *p && *p != '"' ) {
            unsigned long long bits = 0;
            int k, bad = 0;
            for( k = 0; k < 16 && p[k] && p[k] != '"'; k++ ) {
                char c = p[k];
                unsigned int v;
                if     ( c >= '0' && c <= '9' ) v = (unsigned int)(c - '0');
                else if( c >= 'a' && c <= 'f' ) v = (unsigned int)(c - 'a' + 10);
                else if( c >= 'A' && c <= 'F' ) v = (unsigned int)(c - 'A' + 10);
                else { bad = 1; break; }   /* reject, never decode as zero */
                bits = (bits << 4) | v;
            }
            if( bad || k < 16 ) break;   /* malformed or truncated trailing group */
            memcpy(&out[count], &bits, sizeof(double));
            count++;
            p += 16;
        }
        return count;
    }
    if( *p != '[' ) return 0;
    p++;
    while( *p && *p != ']' && count < max_count ) {
        while( *p == ' ' || *p == ',' ) p++;
        if( *p == ']' ) break;
        out[count] = strtod(p, (char **)&p);
        count++;
    }
    return count;
}

static int abstract_json_get_int_array(const char *json, const char *field,
                                       int *out, int max_count)
{
    char pattern[256];
    snprintf(pattern, sizeof(pattern), "\"%s\":", field);
    const char *p = strstr(json, pattern);
    if( !p ) return 0;
    p += strlen(pattern);
    while( *p == ' ' ) p++;
    if( *p != '[' ) return 0;
    p++;
    int count = 0;
    while( *p && *p != ']' && count < max_count ) {
        while( *p == ' ' || *p == ',' ) p++;
        if( *p == ']' ) break;
        out[count] = (int)strtol(p, (char **)&p, 10);
        count++;
    }
    return count;
}

/* Get a JSON string field value. Copies into out (up to outSize-1 chars).
 * Returns 1 if found, 0 if not. */
static int abstract_json_get_string(const char *json, const char *field,
                                    char *out, int outSize)
{
    char pattern[256];
    snprintf(pattern, sizeof(pattern), "\"%s\":\"", field);
    const char *p = strstr(json, pattern);
    if( !p ) return 0;
    p += strlen(pattern);
    int i = 0;
    while( *p && *p != '"' && i < outSize - 1 ) {
        out[i++] = *p++;
    }
    out[i] = '\0';
    return 1;
}

#define CODEGEN_EPSILON 1e-6

/* Coverage counters for the metadata sweep. Printed and asserted by
 * test_abstract_server_metadata: a comparison that ran zero times reads exactly
 * like one that passed. */
static int g_optHintCompared = 0;
static int g_optExtendedCompared = 0;

/* Verify all ta_abstract metadata for a function against the server.
 * Calls TA_GetFuncInfo, TA_GetInputParameterInfo, TA_GetOptInputParameterInfo,
 * TA_GetOutputParameterInfo on both C and server, compares results.
 */

static ErrorNumber abstract_verify_func_metadata(
    const char *funcName,
    const TA_FuncHandle *handle,
    const TA_FuncInfo *fi)
{
    if( !g_abstractPipe ) return TA_TEST_PASS;

    /* TA_GetFuncInfo */
    snprintf(g_abstractReqBuf, ABSTRACT_JSON_BUF_SIZE,
        "{\"method\":\"TA_GetFuncInfo\",\"params\":{\"funcName\":\"%s\"}}",
        funcName);
    ErrorNumber err = codegen_pipe_call(g_abstractPipe, g_abstractReqBuf,
                                        g_abstractRespBuf, ABSTRACT_JSON_BUF_SIZE);
    if( err != TA_TEST_PASS || abstract_json_is_error(g_abstractRespBuf) )
    {
        printf("  ABSTRACT ERROR [%s]: TA_GetFuncInfo server error\n", funcName);

        return TA_ABSTRACT_SERVER_ERROR;
    }

    int srvNbInput = abstract_json_get_int(g_abstractRespBuf, "nbInput");
    int srvNbOptInput = abstract_json_get_int(g_abstractRespBuf, "nbOptInput");
    int srvNbOutput = abstract_json_get_int(g_abstractRespBuf, "nbOutput");
    int srvFlags = abstract_json_get_int(g_abstractRespBuf, "flags");
    char srvName[128] = {0}, srvGroup[128] = {0};
    char srvHint[256] = {0};
    abstract_json_get_string(g_abstractRespBuf, "name", srvName, sizeof(srvName));
    abstract_json_get_string(g_abstractRespBuf, "group", srvGroup, sizeof(srvGroup));
    abstract_json_get_string(g_abstractRespBuf, "hint", srvHint, sizeof(srvHint));

    if( fi->name && strcmp(srvName, fi->name) != 0 ) {
        printf("  ABSTRACT ERROR [%s]: TA_GetFuncInfo name C=%s server=%s\n",
               funcName, fi->name, srvName);
        return TA_ABSTRACT_CALL_MISMATCH;
    }
    if( fi->group && strcmp(srvGroup, fi->group) != 0 ) {
        printf("  ABSTRACT ERROR [%s]: TA_GetFuncInfo group C=%s server=%s\n",
               funcName, fi->group, srvGroup);
        return TA_ABSTRACT_CALL_MISMATCH;
    }
    if( fi->hint && strcmp(srvHint, fi->hint) != 0 ) {
        printf("  ABSTRACT ERROR [%s]: TA_GetFuncInfo hint C=%s server=%s\n",
               funcName, fi->hint, srvHint);
        return TA_ABSTRACT_CALL_MISMATCH;
    }
    if( srvNbInput != (int)fi->nbInput ) {
        printf("  ABSTRACT ERROR [%s]: TA_GetFuncInfo nbInput C=%u server=%d\n",
               funcName, fi->nbInput, srvNbInput);
        return TA_ABSTRACT_CALL_MISMATCH;
    }
    if( srvNbOptInput != (int)fi->nbOptInput ) {
        printf("  ABSTRACT ERROR [%s]: TA_GetFuncInfo nbOptInput C=%u server=%d\n",
               funcName, fi->nbOptInput, srvNbOptInput);
        return TA_ABSTRACT_CALL_MISMATCH;
    }
    if( srvNbOutput != (int)fi->nbOutput ) {
        printf("  ABSTRACT ERROR [%s]: TA_GetFuncInfo nbOutput C=%u server=%d\n",
               funcName, fi->nbOutput, srvNbOutput);
        return TA_ABSTRACT_CALL_MISMATCH;
    }
    if( srvFlags != (int)fi->flags ) {
        printf("  ABSTRACT ERROR [%s]: TA_GetFuncInfo flags C=%d server=%d\n",
               funcName, (int)fi->flags, srvFlags);
        return TA_ABSTRACT_CALL_MISMATCH;
    }

    /* TA_GetInputParameterInfo for each input */
    for( unsigned int i = 0; i < fi->nbInput; i++ )
    {
        const TA_InputParameterInfo *crefInfo;
        TA_GetInputParameterInfo(handle, i, &crefInfo);

        snprintf(g_abstractReqBuf, ABSTRACT_JSON_BUF_SIZE,
            "{\"method\":\"TA_GetInputParameterInfo\",\"params\":{\"funcName\":\"%s\",\"paramIndex\":%u}}",
            funcName, i);
        err = codegen_pipe_call(g_abstractPipe, g_abstractReqBuf,
                                g_abstractRespBuf, ABSTRACT_JSON_BUF_SIZE);
        if( err != TA_TEST_PASS || abstract_json_is_error(g_abstractRespBuf) )
        {
            printf("  ABSTRACT ERROR [%s]: TA_GetInputParameterInfo[%u] server error\n", funcName, i);
            return TA_ABSTRACT_SERVER_ERROR;
        }

        int srvType = abstract_json_get_int(g_abstractRespBuf, "type");
        int srvFlags2 = abstract_json_get_int(g_abstractRespBuf, "flags");
        char srvParamName[128] = {0};
        abstract_json_get_string(g_abstractRespBuf, "paramName", srvParamName, sizeof(srvParamName));

        if( srvType != (int)crefInfo->type ) {
            printf("  ABSTRACT ERROR [%s]: TA_GetInputParameterInfo[%u] type C=%d server=%d\n",
                   funcName, i, (int)crefInfo->type, srvType);
            return TA_ABSTRACT_CALL_MISMATCH;
        }
        if( srvFlags2 != (int)crefInfo->flags ) {
            printf("  ABSTRACT ERROR [%s]: TA_GetInputParameterInfo[%u] flags C=%d server=%d\n",
                   funcName, i, (int)crefInfo->flags, srvFlags2);
            return TA_ABSTRACT_CALL_MISMATCH;
        }
        if( crefInfo->paramName && strcmp(srvParamName, crefInfo->paramName) != 0 ) {
            printf("  ABSTRACT ERROR [%s]: TA_GetInputParameterInfo[%u] paramName C=%s server=%s\n",
                   funcName, i, crefInfo->paramName, srvParamName);
            return TA_ABSTRACT_CALL_MISMATCH;
        }
    }

    /* TA_GetOptInputParameterInfo for each optional input */
    for( unsigned int i = 0; i < fi->nbOptInput; i++ )
    {
        const TA_OptInputParameterInfo *crefOpt;
        TA_GetOptInputParameterInfo(handle, i, &crefOpt);

        snprintf(g_abstractReqBuf, ABSTRACT_JSON_BUF_SIZE,
            "{\"method\":\"TA_GetOptInputParameterInfo\",\"params\":{\"funcName\":\"%s\",\"paramIndex\":%u}}",
            funcName, i);
        err = codegen_pipe_call(g_abstractPipe, g_abstractReqBuf,
                                g_abstractRespBuf, ABSTRACT_JSON_BUF_SIZE);
        if( err != TA_TEST_PASS || abstract_json_is_error(g_abstractRespBuf) )
        {
            printf("  ABSTRACT ERROR [%s]: TA_GetOptInputParameterInfo[%u] server error\n", funcName, i);
            return TA_ABSTRACT_SERVER_ERROR;
        }

        int srvType = abstract_json_get_int(g_abstractRespBuf, "type");
        char srvParamName[128] = {0};
        char srvDisplayName[128] = {0};
        abstract_json_get_string(g_abstractRespBuf, "paramName", srvParamName, sizeof(srvParamName));
        abstract_json_get_string(g_abstractRespBuf, "displayName", srvDisplayName, sizeof(srvDisplayName));
        double srvDefault = abstract_json_get_double(g_abstractRespBuf, "defaultValue");

        if( srvType != (int)crefOpt->type ) {
            printf("  ABSTRACT ERROR [%s]: TA_GetOptInputParameterInfo[%u] type C=%d server=%d\n",
                   funcName, i, (int)crefOpt->type, srvType);
            return TA_ABSTRACT_CALL_MISMATCH;
        }
        if( crefOpt->paramName && strcmp(srvParamName, crefOpt->paramName) != 0 ) {
            printf("  ABSTRACT ERROR [%s]: TA_GetOptInputParameterInfo[%u] paramName C=%s server=%s\n",
                   funcName, i, crefOpt->paramName, srvParamName);
            return TA_ABSTRACT_CALL_MISMATCH;
        }
        if( crefOpt->displayName && strcmp(srvDisplayName, crefOpt->displayName) != 0 ) {
            printf("  ABSTRACT ERROR [%s]: TA_GetOptInputParameterInfo[%u] displayName C=%s server=%s\n",
                   funcName, i, crefOpt->displayName, srvDisplayName);
            return TA_ABSTRACT_CALL_MISMATCH;
        }
        /* hint. For a bespoke descriptor this is a genuine YAML-vs-C check.
         * For a slot folded onto a predefined TA_DEF_UI_*, the generator now
         * folds only on full equality (#195), so the hint agreeing is what
         * selected the constant — this arm cannot fail there, and a stale
         * generator literal shows up as a decline, not a divergence. */
        {
            char srvHint[256] = {0};
            const char *crefHint = crefOpt->hint ? crefOpt->hint : "";
            abstract_json_get_string(g_abstractRespBuf, "hint", srvHint, sizeof(srvHint));
            if( strcmp(srvHint, crefHint) != 0 ) {
                printf("  ABSTRACT ERROR [%s]: TA_GetOptInputParameterInfo[%u] hint C=\"%s\" server=\"%s\"\n",
                       funcName, i, crefHint, srvHint);
                return TA_ABSTRACT_CALL_MISMATCH;
            }
            g_optHintCompared++;
        }
        /* Compare defaultValue as double with tolerance */
        {
            double diff = srvDefault - crefOpt->defaultValue;
            if( diff < 0 ) diff = -diff;
            double tol = CODEGEN_EPSILON;
            if( diff > tol ) {
                printf("  ABSTRACT ERROR [%s]: TA_GetOptInputParameterInfo[%u] defaultValue C=%.15g server=%.15g\n",
                       funcName, i, crefOpt->defaultValue, srvDefault);
                return TA_ABSTRACT_CALL_MISMATCH;
            }
        }
        /* Compare opt-input flags (IS_PERCENT/IS_DEGREE/IS_CURRENCY/ADVANCED) */
        {
            int srvOptFlags = abstract_json_get_int(g_abstractRespBuf, "flags");
            if( srvOptFlags != (int)crefOpt->flags ) {
                printf("  ABSTRACT ERROR [%s]: TA_GetOptInputParameterInfo[%u] flags C=%d server=%d\n",
                       funcName, i, (int)crefOpt->flags, srvOptFlags);
                return TA_ABSTRACT_CALL_MISMATCH;
            }
        }
        /* Compare range/list extended data if available (min/max, precision,
         * suggested optimization values, and enum value lists).
         *
         * The `if` is why g_optExtendedCompared exists: a NULL dataSet silently
         * skips every one of those comparisons, so without a count the gate
         * cannot distinguish "all ranges matched" from "no range was looked at".
         * test_abstract_server_metadata asserts the count is non-zero. */
        if( crefOpt->dataSet ) {
            g_optExtendedCompared++;
            if( crefOpt->type == TA_OptInput_IntegerRange ) {
                const TA_IntegerRange *r = (const TA_IntegerRange *)crefOpt->dataSet;
                int srvMin = abstract_json_get_int(g_abstractRespBuf, "min");
                int srvMax = abstract_json_get_int(g_abstractRespBuf, "max");
                int srvSugSt = abstract_json_get_int(g_abstractRespBuf, "suggestedStart");
                int srvSugEn = abstract_json_get_int(g_abstractRespBuf, "suggestedEnd");
                int srvSugIn = abstract_json_get_int(g_abstractRespBuf, "suggestedIncrement");
                if( srvMin != (int)r->min || srvMax != (int)r->max ) {
                    printf("  ABSTRACT ERROR [%s]: TA_GetOptInputParameterInfo[%u] range C=[%d,%d] server=[%d,%d]\n",
                           funcName, i, (int)r->min, (int)r->max, srvMin, srvMax);
                    return TA_ABSTRACT_CALL_MISMATCH;
                }
                if( srvSugSt != (int)r->suggested_start || srvSugEn != (int)r->suggested_end || srvSugIn != (int)r->suggested_increment ) {
                    printf("  ABSTRACT ERROR [%s]: TA_GetOptInputParameterInfo[%u] suggested C=[%d,%d,%d] server=[%d,%d,%d]\n",
                           funcName, i, (int)r->suggested_start, (int)r->suggested_end, (int)r->suggested_increment, srvSugSt, srvSugEn, srvSugIn);
                    return TA_ABSTRACT_CALL_MISMATCH;
                }
            } else if( crefOpt->type == TA_OptInput_RealRange ) {
                const TA_RealRange *r = (const TA_RealRange *)crefOpt->dataSet;
                double srvMin = abstract_json_get_double(g_abstractRespBuf, "min");
                double srvMax = abstract_json_get_double(g_abstractRespBuf, "max");
                int    srvPrec = abstract_json_get_int(g_abstractRespBuf, "precision");
                double srvSugSt = abstract_json_get_double(g_abstractRespBuf, "suggestedStart");
                double srvSugEn = abstract_json_get_double(g_abstractRespBuf, "suggestedEnd");
                double srvSugIn = abstract_json_get_double(g_abstractRespBuf, "suggestedIncrement");
                double diffMin = srvMin - r->min; if(diffMin<0) diffMin=-diffMin;
                double diffMax = srvMax - r->max; if(diffMax<0) diffMax=-diffMax;
                double dSt = srvSugSt - r->suggested_start; if(dSt<0) dSt=-dSt;
                double dEn = srvSugEn - r->suggested_end; if(dEn<0) dEn=-dEn;
                double dIn = srvSugIn - r->suggested_increment; if(dIn<0) dIn=-dIn;
                if( diffMin > CODEGEN_EPSILON || diffMax > CODEGEN_EPSILON ) {
                    printf("  ABSTRACT ERROR [%s]: TA_GetOptInputParameterInfo[%u] range C=[%.6g,%.6g] server=[%.6g,%.6g]\n",
                           funcName, i, r->min, r->max, srvMin, srvMax);
                    return TA_ABSTRACT_CALL_MISMATCH;
                }
                if( srvPrec != (int)r->precision ) {
                    printf("  ABSTRACT ERROR [%s]: TA_GetOptInputParameterInfo[%u] precision C=%d server=%d\n",
                           funcName, i, (int)r->precision, srvPrec);
                    return TA_ABSTRACT_CALL_MISMATCH;
                }
                if( dSt > CODEGEN_EPSILON || dEn > CODEGEN_EPSILON || dIn > CODEGEN_EPSILON ) {
                    printf("  ABSTRACT ERROR [%s]: TA_GetOptInputParameterInfo[%u] suggested C=[%.6g,%.6g,%.6g] server=[%.6g,%.6g,%.6g]\n",
                           funcName, i, r->suggested_start, r->suggested_end, r->suggested_increment, srvSugSt, srvSugEn, srvSugIn);
                    return TA_ABSTRACT_CALL_MISMATCH;
                }
            } else if( crefOpt->type == TA_OptInput_IntegerList ) {
                const TA_IntegerList *l = (const TA_IntegerList *)crefOpt->dataSet;
                char crefList[1024] = {0}; int p = 0; unsigned int vi;
                char srvList[1024] = {0};
                for( vi = 0; vi < l->nbElement; vi++ ) {
                    p = codegen_appendf(crefList, (int)sizeof(crefList), p, "%s%d=%s",
                                        vi ? ";" : "", (int)l->data[vi].value,
                                        l->data[vi].string ? l->data[vi].string : "");
                }
                abstract_json_get_string(g_abstractRespBuf, "valueList", srvList, sizeof(srvList));
                if( strcmp(srvList, crefList) != 0 ) {
                    printf("  ABSTRACT ERROR [%s]: TA_GetOptInputParameterInfo[%u] valueList C=[%s] server=[%s]\n",
                           funcName, i, crefList, srvList);
                    return TA_ABSTRACT_CALL_MISMATCH;
                }
            } else {
                /* The counter above was already incremented, so an unhandled
                 * domain would report itself as compared while comparing
                 * nothing -- exactly the vacuity g_optExtendedCompared exists to
                 * catch. TA_OptInput_RealList is the only type that lands here
                 * and no shipped function declares one; the day one does, this
                 * says so instead of passing silently (#164). */
                printf("  ABSTRACT ERROR [%s]: TA_GetOptInputParameterInfo[%u] domain type %d "
                       "has no comparison arm -- it would be counted as compared\n",
                       funcName, i, (int)crefOpt->type);
                return TA_ABSTRACT_CALL_MISMATCH;
            }
        }
    }

    /* TA_GetOutputParameterInfo for each output */
    for( unsigned int i = 0; i < fi->nbOutput; i++ )
    {
        const TA_OutputParameterInfo *crefOut;
        TA_GetOutputParameterInfo(handle, i, &crefOut);

        snprintf(g_abstractReqBuf, ABSTRACT_JSON_BUF_SIZE,
            "{\"method\":\"TA_GetOutputParameterInfo\",\"params\":{\"funcName\":\"%s\",\"paramIndex\":%u}}",
            funcName, i);
        err = codegen_pipe_call(g_abstractPipe, g_abstractReqBuf,
                                g_abstractRespBuf, ABSTRACT_JSON_BUF_SIZE);
        if( err != TA_TEST_PASS || abstract_json_is_error(g_abstractRespBuf) )
        {
            printf("  ABSTRACT ERROR [%s]: TA_GetOutputParameterInfo[%u] server error\n", funcName, i);
            return TA_ABSTRACT_SERVER_ERROR;
        }

        int srvType = abstract_json_get_int(g_abstractRespBuf, "type");
        int srvFlags3 = abstract_json_get_int(g_abstractRespBuf, "flags");
        char srvParamName[128] = {0};
        abstract_json_get_string(g_abstractRespBuf, "paramName", srvParamName, sizeof(srvParamName));

        if( srvType != (int)crefOut->type ) {
            printf("  ABSTRACT ERROR [%s]: TA_GetOutputParameterInfo[%u] type C=%d server=%d\n",
                   funcName, i, (int)crefOut->type, srvType);
            return TA_ABSTRACT_CALL_MISMATCH;
        }
        if( srvFlags3 != (int)crefOut->flags ) {
            printf("  ABSTRACT ERROR [%s]: TA_GetOutputParameterInfo[%u] flags C=%d server=%d\n",
                   funcName, i, (int)crefOut->flags, srvFlags3);
            return TA_ABSTRACT_CALL_MISMATCH;
        }
        if( crefOut->paramName && strcmp(srvParamName, crefOut->paramName) != 0 ) {
            printf("  ABSTRACT ERROR [%s]: TA_GetOutputParameterInfo[%u] paramName C=%s server=%s\n",
                   funcName, i, crefOut->paramName, srvParamName);
            return TA_ABSTRACT_CALL_MISMATCH;
        }
    }

    return TA_TEST_PASS;
}

/* ---------------------------------------------------------------------------
 * Metadata-only abstract parity for a language server.
 *
 * Runs TA_GetFuncInfo / TA_Get{Input,OptInput,Output}ParameterInfo against the
 * server set via test_abstract_set_server() for EVERY function and compares to
 * the C reference, WITHOUT the heavier abstract_call (dynamic-dispatch) path.
 * Used to lock cross-language introspection metadata parity in CI (e.g. the Rust
 * abstract_api registry).
 * --------------------------------------------------------------------------- */
typedef struct { ErrorNumber firstErr; int checked; int failed; const char *filter; } MetaParityCtx;

/* Comma-separated substring match against the function name (matches the
 * --function filter semantics used by test_codegen). NULL filter = match all. */
static int metaMatchesFilter( const char *filter, const char *name )
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

static void metaParityCb( const TA_FuncInfo *funcInfo, void *opaqueData )
{
    MetaParityCtx *ctx = (MetaParityCtx *)opaqueData;
    ErrorNumber e;
    if( !metaMatchesFilter( ctx->filter, funcInfo->name ) )
        return;
    ctx->checked++;
    e = abstract_verify_func_metadata( funcInfo->name, funcInfo->handle, funcInfo );
    if( e != TA_TEST_PASS )
    {
        ctx->failed++;
        if( ctx->firstErr == TA_TEST_PASS )
            ctx->firstErr = e;
    }
}

/* ---------------------------------------------------------------------------
 * abstract_for_each_func parity.
 *
 * Every generated server implements this RPC and, until now, NOTHING called it
 * in any language — so an implementation could be arbitrarily wrong (or absent)
 * and every gate stayed green. This gives it a caller: the server's enumeration
 * must name the same functions as C's TA_ForEachFunc, with the same group and
 * the same nbInput/nbOptInput/nbOutput for each.
 *
 * Compared as a SET, not a sequence: C's TA_ForEachFunc walks group by group
 * while the Rust/Java/C# registries enumerate name-sorted, so an order-sensitive
 * comparison would fail on a correct server.
 * --------------------------------------------------------------------------- */

#define FOREACH_MAX_FUNC 512

typedef struct {
    const char *name;
    const char *group;
    unsigned int nbInput, nbOptInput, nbOutput;
    int seen;
} ForEachExpect;

typedef struct { ForEachExpect *tab; int count; } ForEachCollectCtx;

static void forEachCollectCb( const TA_FuncInfo *fi, void *opaqueData )
{
    ForEachCollectCtx *ctx = (ForEachCollectCtx *)opaqueData;
    if( ctx->count >= FOREACH_MAX_FUNC )
        return;
    ForEachExpect *e = &ctx->tab[ctx->count++];
    e->name       = fi->name;
    e->group      = fi->group;
    e->nbInput    = fi->nbInput;
    e->nbOptInput = fi->nbOptInput;
    e->nbOutput   = fi->nbOutput;
    e->seen       = 0;
}

/* Copy the JSON object starting at `p` (which must point at '{') into `out`.
 * Returns a pointer just past its closing '}', or NULL if malformed. The
 * enumeration carries no nested objects and no braces inside strings, so a
 * depth counter is enough. */
static const char *forEachCopyObject( const char *p, char *out, int outSize )
{
    int depth = 0, i = 0;
    if( *p != '{' ) return NULL;
    while( *p ) {
        if( *p == '{' ) depth++;
        else if( *p == '}' ) depth--;
        if( i < outSize - 1 ) out[i++] = *p;
        p++;
        if( depth == 0 ) break;
    }
    out[i] = '\0';
    return depth == 0 ? p : NULL;
}

static ErrorNumber abstract_verify_for_each_func( void )
{
    static ForEachExpect expected[FOREACH_MAX_FUNC];
    ForEachCollectCtx collect;
    const char *p;
    int nbFromServer = 0;
    int i;

    if( !g_abstractPipe ) return TA_TEST_PASS;

    collect.tab = expected;
    collect.count = 0;
    TA_ForEachFunc( forEachCollectCb, &collect );

    snprintf(g_abstractReqBuf, ABSTRACT_JSON_BUF_SIZE,
             "{\"method\":\"abstract_for_each_func\",\"params\":{}}");
    if( codegen_pipe_call(g_abstractPipe, g_abstractReqBuf,
                          g_abstractRespBuf, ABSTRACT_JSON_BUF_SIZE) != TA_TEST_PASS
        || abstract_json_is_error(g_abstractRespBuf) )
    {
        printf("  ABSTRACT ERROR: abstract_for_each_func server error\n");
        return TA_ABSTRACT_SERVER_ERROR;
    }

    p = strstr(g_abstractRespBuf, "\"functions\":");
    if( !p ) {
        printf("  ABSTRACT ERROR: abstract_for_each_func response has no \"functions\" array\n");
        return TA_ABSTRACT_CALL_MISMATCH;
    }
    p = strchr(p, '[');
    if( !p ) {
        printf("  ABSTRACT ERROR: abstract_for_each_func \"functions\" is not an array\n");
        return TA_ABSTRACT_CALL_MISMATCH;
    }
    p++;

    while( *p )
    {
        char obj[512];
        char srvName[128] = {0};
        char srvGroup[128] = {0};
        int srvNbIn, srvNbOpt, srvNbOut;
        int found = -1;

        while( *p == ' ' || *p == ',' ) p++;
        if( *p == ']' || *p == '\0' ) break;

        p = forEachCopyObject(p, obj, (int)sizeof(obj));
        if( !p ) {
            printf("  ABSTRACT ERROR: abstract_for_each_func malformed entry\n");
            return TA_ABSTRACT_CALL_MISMATCH;
        }
        nbFromServer++;

        abstract_json_get_string(obj, "name", srvName, sizeof(srvName));
        abstract_json_get_string(obj, "group", srvGroup, sizeof(srvGroup));
        srvNbIn  = abstract_json_get_int(obj, "nbInput");
        srvNbOpt = abstract_json_get_int(obj, "nbOptInput");
        srvNbOut = abstract_json_get_int(obj, "nbOutput");

        for( i = 0; i < collect.count; i++ ) {
            if( expected[i].name && strcmp(expected[i].name, srvName) == 0 ) { found = i; break; }
        }
        if( found < 0 ) {
            printf("  ABSTRACT ERROR: abstract_for_each_func lists '%s', which C does not\n", srvName);
            return TA_ABSTRACT_CALL_MISMATCH;
        }
        if( expected[found].seen ) {
            printf("  ABSTRACT ERROR: abstract_for_each_func lists '%s' twice\n", srvName);
            return TA_ABSTRACT_CALL_MISMATCH;
        }
        expected[found].seen = 1;

        if( expected[found].group && strcmp(expected[found].group, srvGroup) != 0 ) {
            printf("  ABSTRACT ERROR [%s]: abstract_for_each_func group C=%s server=%s\n",
                   srvName, expected[found].group, srvGroup);
            return TA_ABSTRACT_CALL_MISMATCH;
        }
        if( srvNbIn  != (int)expected[found].nbInput ||
            srvNbOpt != (int)expected[found].nbOptInput ||
            srvNbOut != (int)expected[found].nbOutput )
        {
            printf("  ABSTRACT ERROR [%s]: abstract_for_each_func counts C=(%u,%u,%u) server=(%d,%d,%d)\n",
                   srvName, expected[found].nbInput, expected[found].nbOptInput,
                   expected[found].nbOutput, srvNbIn, srvNbOpt, srvNbOut);
            return TA_ABSTRACT_CALL_MISMATCH;
        }
    }

    for( i = 0; i < collect.count; i++ ) {
        if( !expected[i].seen ) {
            printf("  ABSTRACT ERROR: abstract_for_each_func omits '%s'\n", expected[i].name);
            return TA_ABSTRACT_CALL_MISMATCH;
        }
    }
    if( nbFromServer != collect.count ) {
        printf("  ABSTRACT ERROR: abstract_for_each_func enumerated %d functions, C has %d\n",
               nbFromServer, collect.count);
        return TA_ABSTRACT_CALL_MISMATCH;
    }
    if( collect.count == 0 ) {
        printf("  ABSTRACT ERROR: abstract_for_each_func compared nothing (C enumerated 0)\n");
        return TA_ABSTRACT_CALL_MISMATCH;
    }

    printf( "  Abstract for-each parity: %d functions enumerated, all matched\n", nbFromServer );
    return TA_TEST_PASS;
}

ErrorNumber test_abstract_server_metadata( const char *functionFilter )
{
    ErrorNumber retValue;
    ErrorNumber forEachErr;
    MetaParityCtx ctx;

    if( !g_abstractPipe )
        return TA_TEST_PASS;

    retValue = allocLib();
    if( retValue != TA_TEST_PASS )
        return retValue;

    ctx.firstErr = TA_TEST_PASS;
    ctx.checked  = 0;
    ctx.failed   = 0;
    ctx.filter   = functionFilter;
    g_optHintCompared = 0;
    g_optExtendedCompared = 0;
    TA_ForEachFunc( metaParityCb, &ctx );

    printf( "  Abstract metadata parity: %d functions checked, %d failed"
            " (%d opt hints, %d opt ranges/lists compared)\n",
            ctx.checked, ctx.failed, g_optHintCompared, g_optExtendedCompared );

    /* A real disagreement outranks the vacuity checks below, and must be
     * reported as itself: letting a dead server fall through to the
     * "compared 0" branch would print 168 SERVER_ERRORs and then return
     * CALL_MISMATCH, i.e. "the metadata disagreed" for a server that never
     * answered. */
    if( ctx.firstErr != TA_TEST_PASS )
    {
        freeLib();
        return ctx.firstErr;
    }

    /* Non-vacuity: these counts were printed and never asserted, so a sweep that
     * compared nothing read exactly like a pass.
     *
     * ALL of them are gated on an unfiltered run, and that is not laziness.
     * `--function` carries two different vocabularies: the hand-written tests
     * match it against a DO_TEST *group tag* (`MATH`, `Moving Averages`,
     * `COMPOSITE`), while metaMatchesFilter matches it against a *function
     * name*. Thirteen of the documented group tokens name no function at all,
     * so requiring checked != 0 under a filter turns `--codegen
     * --function="Moving Averages"` — a documented invocation — into a hard
     * failure. Nothing is lost by gating: on an unfiltered run checked == 0
     * would mean TA_ForEachFunc enumerated nothing, and
     * abstract_verify_for_each_func fails on that unconditionally. */
    if( functionFilter == NULL &&
        (ctx.checked == 0 || g_optHintCompared == 0 || g_optExtendedCompared == 0) )
    {
        printf( "  ABSTRACT ERROR: metadata parity compared %d functions, %d opt hints "
                "and %d opt ranges/lists over the whole library — expected all non-zero\n",
                ctx.checked, g_optHintCompared, g_optExtendedCompared );
        freeLib();
        return TA_ABSTRACT_CALL_MISMATCH;
    }

    /* Enumeration parity is deliberately NOT filtered by --function: the whole
     * point is that the server's list and C's are the same list. */
    forEachErr = abstract_verify_for_each_func();

    retValue = freeLib();
    if( ctx.firstErr != TA_TEST_PASS )
        return ctx.firstErr;
    if( forEachErr != TA_TEST_PASS )
        return forEachErr;
    return retValue;
}

/* Build and send an abstract_call request to the server, mirroring the
 * C TA_CallFunc that was just made with the given paramHolder.
 * Compares retCode, outBegIdx, outNBElement, lookback.
 */
static ErrorNumber abstract_verify_server_call(
    const char *funcName,
    const TA_FuncHandle *handle,
    const TA_FuncInfo *funcInfo,
    const double *input, int size,
    int startIdx, int endIdx,
    TA_RetCode crefRetCode,
    int crefBegIdx, int crefNbElement, int crefLookback,
    double crefOutReal[][2000], int crefOutInt[][2000],
    int relaxValues,
    /* Optional-parameter values to send, one per declared slot, or NULL for the
     * declared defaults. Everything but the D2 vector sweep passes NULL. */
    const double *optVals)
{
    if( !g_abstractPipe ) return TA_TEST_PASS;

    const double *volInput = abstract_volume_view( input, (unsigned int)size );

    char *buf = g_abstractReqBuf;
    int bufSize = ABSTRACT_JSON_BUF_SIZE;
    int pos = 0;

    pos = codegen_appendf(buf, bufSize, pos,
        "{\"method\":\"abstract_call\",\"params\":{\"funcName\":\"%s\""
        ",\"startIdx\":%d,\"endIdx\":%d",
        funcName, startIdx, endIdx);

    /* Input params — one view per component, identical to callWithDefaults(). */
    int totalRealInputs = 0;
    for( unsigned int i = 0; i < funcInfo->nbInput; i++ )
    {
        const TA_InputParameterInfo *ii;
        TA_GetInputParameterInfo(handle, i, &ii);
        if( ii->type == TA_Input_Real ) totalRealInputs++;
    }

    int realInputCount = 0;
    for( unsigned int i = 0; i < funcInfo->nbInput; i++ )
    {
        const TA_InputParameterInfo *inputInfo;
        TA_GetInputParameterInfo(handle, i, &inputInfo);

        switch( inputInfo->type )
        {
        case TA_Input_Price:
        {
            TA_InputFlags flags = inputInfo->flags;
            if( flags & TA_IN_PRICE_OPEN ) {
                pos = codegen_appendf(buf, bufSize, pos, ",\"inOpen\":");
                pos = abstract_json_write_double_array(buf, bufSize, pos,
                        abstract_price_view(input, (unsigned int)size, TA_IN_PRICE_OPEN), size);
            }
            if( flags & TA_IN_PRICE_HIGH ) {
                pos = codegen_appendf(buf, bufSize, pos, ",\"inHigh\":");
                pos = abstract_json_write_double_array(buf, bufSize, pos,
                        abstract_price_view(input, (unsigned int)size, TA_IN_PRICE_HIGH), size);
            }
            if( flags & TA_IN_PRICE_LOW ) {
                pos = codegen_appendf(buf, bufSize, pos, ",\"inLow\":");
                pos = abstract_json_write_double_array(buf, bufSize, pos,
                        abstract_price_view(input, (unsigned int)size, TA_IN_PRICE_LOW), size);
            }
            if( flags & TA_IN_PRICE_CLOSE ) {
                pos = codegen_appendf(buf, bufSize, pos, ",\"inClose\":");
                pos = abstract_json_write_double_array(buf, bufSize, pos,
                        abstract_price_view(input, (unsigned int)size, TA_IN_PRICE_CLOSE), size);
            }
            if( flags & TA_IN_PRICE_VOLUME ) {
                pos = codegen_appendf(buf, bufSize, pos, ",\"inVolume\":");
                pos = abstract_json_write_double_array(buf, bufSize, pos, volInput, size);
            }
            if( flags & TA_IN_PRICE_OPENINTEREST ) {
                pos = codegen_appendf(buf, bufSize, pos, ",\"inOpenInterest\":");
                pos = abstract_json_write_double_array(buf, bufSize, pos,
                        abstract_price_view(input, (unsigned int)size, TA_IN_PRICE_OPENINTEREST), size);
            }
            break;
        }
        case TA_Input_Real:
            if( totalRealInputs == 1 )
                pos = codegen_appendf(buf, bufSize, pos, ",\"inReal\":");
            else
                pos = codegen_appendf(buf, bufSize, pos, ",\"inReal%d\":", realInputCount);
            pos = abstract_json_write_double_array(buf, bufSize, pos,
                    abstract_real_view(input, (unsigned int)size, realInputCount), size);
            realInputCount++;
            break;
        case TA_Input_Integer:
            /* Silently omitted, which would leave the server's slot unbound
             * while C's is bound -- the two binders would then be compared on
             * different inputs and could agree by accident. No shipped function
             * declares an integer input, so this is unreachable today; say so
             * rather than let the first one that does slip through (#164). */
            printf("  ABSTRACT ERROR [%s]: integer input '%s' is not sent by "
                   "abstract_call -- the request builder needs an arm for it\n",
                   funcInfo->name, inputInfo->paramName);
            return TA_ABSTRACT_CALL_MISMATCH;
        }
    }

    /* Send optional params using C's defaults (from metadata).
     * This ensures the server uses the same values as C's
     * TA_ParamHolderAlloc, which initializes from defaultValue. */
    for( unsigned int i = 0; i < funcInfo->nbOptInput; i++ )
    {
        const TA_OptInputParameterInfo *optInfo;
        TA_GetOptInputParameterInfo(handle, i, &optInfo);
        pos = codegen_appendf(buf, bufSize, pos, ",\"%s\":", optInfo->paramName);
        switch( optInfo->type )
        {
        case TA_OptInput_RealRange:
        case TA_OptInput_RealList:
            /* %.17g for the same round-trip reason as the input arrays. */
            pos = codegen_appendf(buf, bufSize, pos, "%.17g",
                                  optVals ? optVals[i] : optInfo->defaultValue);
            break;
        case TA_OptInput_IntegerRange:
        case TA_OptInput_IntegerList:
            pos = codegen_appendf(buf, bufSize, pos, "%d",
                                  (int)(optVals ? optVals[i] : optInfo->defaultValue));
            break;
        }
    }

    pos = codegen_appendf(buf, bufSize, pos, "}}");

    /* Send to server */
    ErrorNumber err = codegen_pipe_call(g_abstractPipe, buf,
                                        g_abstractRespBuf, ABSTRACT_JSON_BUF_SIZE);
    if( err != TA_TEST_PASS )
    {
        printf("  ABSTRACT SERVER pipe error [%s]\n", funcName);
        return TA_ABSTRACT_SERVER_ERROR;
    }

    if( abstract_json_is_error(g_abstractRespBuf) )
    {
        printf("  ABSTRACT SERVER error [%s]\n", funcName);
        return TA_ABSTRACT_SERVER_ERROR;
    }

    /* The Rust server answers abstract_call two ways: through the SHIPPED
     * abstract_api::ParamHolder, or -- when the request carries gen_present /
     * want_hash, which is --xlang-hash's seed transport -- by rerouting to the
     * per-function handler. That selector is a payload heuristic, and both
     * replies carry the same fields, so nothing would notice if this sweep drifted
     * onto the reroute and stopped exercising the binder at all. Requiring the
     * marker makes the binder path positively observable instead of merely
     * probable. The other three servers have a single implementation and cannot
     * drift this way. */
    if( g_abstractLang && strcmp(g_abstractLang, "rust") == 0
        && abstract_json_get_int(g_abstractRespBuf, "binder") != 1 )
    {
        printf("  ABSTRACT ERROR [%s]: reply did not come from the shipped Rust binder "
               "(no \"binder\":1) — the transport split has drifted and this sweep is "
               "testing the per-function handler instead\n", funcName);
        return TA_ABSTRACT_CALL_MISMATCH;
    }

    /* Compare structural results */
    int srvRetCode = abstract_json_get_int(g_abstractRespBuf, "retCode");
    if( srvRetCode != (int)crefRetCode )
    {
        printf("  ABSTRACT ERROR [%s]: retCode C=%d server=%d\n",
               funcName, (int)crefRetCode, srvRetCode);

        return TA_ABSTRACT_CALL_MISMATCH;
    }

    if( crefRetCode != TA_SUCCESS )
        return TA_TEST_PASS;

    int srvBegIdx = abstract_json_get_int(g_abstractRespBuf, "outBegIdx");
    if( srvBegIdx != crefBegIdx )
    {
        printf("  ABSTRACT ERROR [%s]: outBegIdx C=%d server=%d\n",
               funcName, crefBegIdx, srvBegIdx);

        return TA_ABSTRACT_CALL_MISMATCH;
    }

    int srvNbElement = abstract_json_get_int(g_abstractRespBuf, "outNBElement");
    if( srvNbElement != crefNbElement )
    {
        printf("  ABSTRACT ERROR [%s]: outNBElement C=%d server=%d\n",
               funcName, crefNbElement, srvNbElement);

        return TA_ABSTRACT_CALL_MISMATCH;
    }

    int srvLookback = abstract_json_get_int(g_abstractRespBuf, "lookback");
    if( srvLookback != crefLookback )
    {
        printf("  ABSTRACT ERROR [%s]: lookback C=%d server=%d\n",
               funcName, crefLookback, srvLookback);

        return TA_ABSTRACT_LOOKBACK_MISMATCH;
    }

    /* Compare output arrays.
     *
     * relaxValues skips ONLY the output-value comparison (the structural checks
     * above — retCode, outBegIdx, outNBElement, lookback — are always verified).
     * It is set (see callWithDefaults) for the few floating-point-order-sensitive
     * functions (the Hilbert-Transform HT_* family and CCI) on the two random-noise
     * datasets. The Rust codegen is not bit-identical to the C reference (residual
     * ~1e-13 operation-ordering differences, independent of FMA); for most
     * functions/inputs that stays far under tolerance, but these few amplify it into
     * a discrete divergence at a degenerate boundary (an HT phase wraparound / trend-
     * mode flip, or CCI's division-vs-zero guard flipping) that no fixed tolerance
     * can absorb. On noise input — not a price series — exact value parity is not
     * meaningful; value parity on REAL price data is covered by test_codegen, so here
     * we keep only their structural parity strict. */
    if( crefNbElement > 0 && !relaxValues )
    {
        int realKeyIdx = 0, intKeyIdx = 0;
        for( unsigned int oi = 0; oi < funcInfo->nbOutput && oi < 10; oi++ )
        {
            const TA_OutputParameterInfo *outInfo;
            TA_GetOutputParameterInfo(handle, oi, &outInfo);

            if( outInfo->type == TA_Output_Integer )
            {
                char key[32];
                if( intKeyIdx == 0 ) snprintf(key, sizeof(key), "outInteger");
                else                 snprintf(key, sizeof(key), "outInteger%d", intKeyIdx);
                int srvOut[2000];
                int n = abstract_json_get_int_array(g_abstractRespBuf, key, srvOut, 2000);
                if( n != crefNbElement )
                {
                    printf("  ABSTRACT ERROR [%s]: int output[%u] count C=%d server=%d\n",
                           funcName, oi, crefNbElement, n);
                    return TA_ABSTRACT_CALL_MISMATCH;
                }
                for( int j = 0; j < n && j < crefNbElement; j++ )
                {
                    if( crefOutInt[oi][j] != srvOut[j] )
                    {
                        printf("  ABSTRACT ERROR [%s]: int output[%u][%d] C=%d server=%d\n",
                               funcName, oi, j, crefOutInt[oi][j], srvOut[j]);
                        return TA_ABSTRACT_CALL_MISMATCH;
                    }
                }
                intKeyIdx++;
            }
            else
            {
                char key[32];
                if( realKeyIdx == 0 ) snprintf(key, sizeof(key), "outReal");
                else                  snprintf(key, sizeof(key), "outReal%d", realKeyIdx);
                double srvOut[2000];
                int n = abstract_json_get_double_array(g_abstractRespBuf, key, srvOut, 2000);
                if( n != crefNbElement )
                {
                    printf("  ABSTRACT ERROR [%s]: real output[%u] count C=%d server=%d\n",
                           funcName, oi, crefNbElement, n);
                    return TA_ABSTRACT_CALL_MISMATCH;
                }
                for( int j = 0; j < n && j < crefNbElement; j++ )
                {
                    /* Skip sentinel values — callWithDefaults inits to TA_REAL_MIN */
                    if( crefOutReal[oi][j] <= TA_REAL_MIN ) continue;
                    double diff = crefOutReal[oi][j] - srvOut[j];
                    if( diff < 0 ) diff = -diff;
                    double tol = CODEGEN_EPSILON;
                    if( crefOutReal[oi][j] > 1.0 || crefOutReal[oi][j] < -1.0 )
                    {
                        double mag = crefOutReal[oi][j];
                        if( mag < 0 ) mag = -mag;
                        double relTol = mag * 1e-12;
                        if( relTol > tol ) tol = relTol;
                    }
                    if( diff > tol )
                    {
                        printf("  ABSTRACT ERROR [%s]: real output[%u][%d] C=%.15g server=%.15g diff=%.15g\n",
                               funcName, oi, j, crefOutReal[oi][j], srvOut[j], diff);
                        return TA_ABSTRACT_CALL_MISMATCH;
                    }
                }
                realKeyIdx++;
            }
        }
    }

    return TA_TEST_PASS;
}

/* ---- D2: drive the binders off their defaults (issue #164) -----------------
 *
 * Every dynamic-dispatch comparison in this file bound optional parameters at
 * their declared defaults, so the binders were only ever exercised at one point
 * in their domain. Three things were therefore untested through a binder: a
 * NON-DEFAULT value (a transposed opt slot produces identical output when every
 * slot carries the same number), the DEFAULT SENTINEL (which must resolve to the
 * declared default -- issue #162, and the reason Java's binder shipped a version
 * that threw), and an OUT-OF-RANGE value (which both tiers must reject).
 *
 * The C golden and the server are driven with the SAME vector, so this compares
 * two binders rather than a binder against an oracle of its own making.
 *
 * Counted, and asserted non-zero by the caller: a sweep that silently stopped
 * building vectors would otherwise read exactly like a passing one. */


#define D2_MAX_OPT 16

/* An in-range value that is NOT the declared default. `slot` offsets it so two
 * parameters of one function that share a default do not end up sharing a value
 * too -- 18 slots across 5 functions do (MACDEXT's three periods, STOCH's, ...),
 * and a swap between same-valued slots is invisible. */
static double d2_non_default( const TA_OptInputParameterInfo *oi, unsigned int slot )
{
    switch( oi->type )
    {
    case TA_OptInput_IntegerRange:
    {
        const TA_IntegerRange *r = (const TA_IntegerRange *)oi->dataSet;
        if( !r ) return oi->defaultValue;
        int def = (int)oi->defaultValue;
        int step = (int)slot + 1;
        if( def + step <= r->max ) return (double)(def + step);
        if( def - step >= r->min ) return (double)(def - step);
        if( def + 1 <= r->max ) return (double)(def + 1);
        if( def - 1 >= r->min ) return (double)(def - 1);
        return oi->defaultValue;
    }
    case TA_OptInput_RealRange:
    {
        const TA_RealRange *r = (const TA_RealRange *)oi->dataSet;
        if( !r ) return oi->defaultValue;
        double span = (r->max > r->min) ? (r->max - r->min) : 0.0;
        double step = (span > 0.0 && span < 1e30) ? span * 1e-3 * (double)(slot + 1)
                                                  : 0.125 * (double)(slot + 1);
        if( oi->defaultValue + step <= r->max ) return oi->defaultValue + step;
        if( oi->defaultValue - step >= r->min ) return oi->defaultValue - step;
        return oi->defaultValue;
    }
    case TA_OptInput_IntegerList:
    {
        const TA_IntegerList *l = (const TA_IntegerList *)oi->dataSet;
        if( !l || l->nbElement == 0 ) return oi->defaultValue;
        /* Rotate by slot so sibling MAType parameters differ from each other. */
        for( unsigned int n = 0; n < l->nbElement; n++ )
        {
            unsigned int e = (n + slot + 1) % l->nbElement;
            if( l->data[e].value != (int)oi->defaultValue )
                return (double)l->data[e].value;
        }
        return oi->defaultValue;
    }
    default:
        return oi->defaultValue;
    }
}

/* Values outside the declared range. BOTH bounds, not the first that fits: every
 * shipped integer range is [1,100000] or [2,100000], so returning on the first
 * branch probed only `min-1` and a backend that dropped `|| value > max` passed
 * for all 79 functions. Reals are included too -- #148 was the Rust backend
 * emitting NO validation for real params, so skipping them omits exactly the
 * class the historical defect lived in. Returns how many probes were written. */
static int d2_out_of_range( const TA_OptInputParameterInfo *oi, double out[2] )
{
    int n = 0;
    if( oi->type == TA_OptInput_IntegerRange )
    {
        const TA_IntegerRange *r = (const TA_IntegerRange *)oi->dataSet;
        if( !r ) return 0;
        if( (long long)r->min - 1 >= (long long)TA_INTEGER_MIN )
            out[n++] = (double)((long long)r->min - 1);
        if( (long long)r->max + 1 <= (long long)TA_INTEGER_MAX )
            out[n++] = (double)((long long)r->max + 1);
    }
    else if( oi->type == TA_OptInput_RealRange )
    {
        const TA_RealRange *r = (const TA_RealRange *)oi->dataSet;
        if( !r ) return 0;
        /* +/-1.0 is absorbed at TA_REAL_MIN/MAX magnitude, so fall back to a
         * multiple of the widest legal bound -- the same escape fuzz_add_out_of_range
         * uses, and neither value can collide with TA_REAL_DEFAULT. */
        out[n++] = (r->min - 1.0 < r->min) ? r->min - 1.0 : 2.0 * TA_REAL_MIN;
        out[n++] = (r->max + 1.0 > r->max) ? r->max + 1.0 : 2.0 * TA_REAL_MAX;
    }
    return n;
}

/* Load one vector into the C paramHolder. */
static void d2_set_opts( TA_ParamHolder *paramHolder, const TA_FuncHandle *handle,
                         const TA_FuncInfo *funcInfo, const double *vec )
{
    for( unsigned int k = 0; k < funcInfo->nbOptInput && k < D2_MAX_OPT; k++ )
    {
        const TA_OptInputParameterInfo *oi;
        TA_GetOptInputParameterInfo(handle, k, &oi);
        if( oi->type == TA_OptInput_RealRange || oi->type == TA_OptInput_RealList )
            TA_SetOptInputParamReal(paramHolder, k, vec[k]);
        else
            TA_SetOptInputParamInteger(paramHolder, k, (int)vec[k]);
    }
}

/* Drive one vector through BOTH binders and compare. `paramHolder` already has
 * this function's inputs and outputs bound by the caller. */
#define D2_CLASS_NON_DEFAULT 0
#define D2_CLASS_SENTINEL     1
#define D2_CLASS_REJECT       2

/* The all-defaults C result, captured before the sweep so the sentinel class can
 * assert what it exists to assert. */
typedef struct { TA_RetCode rc; TA_Integer beg, nb, lookback; unsigned long long sum; } D2Baseline;

/* A cheap order-sensitive digest of the output buffers actually written. */
static unsigned long long d2_digest( const TA_FuncInfo *funcInfo, const TA_FuncHandle *handle,
                                     TA_Integer nb )
{
    unsigned long long h = 1469598103934665603ULL;
    for( unsigned int o = 0; o < funcInfo->nbOutput; o++ )
    {
        const TA_OutputParameterInfo *oinfo;
        TA_GetOutputParameterInfo(handle, o, &oinfo);
        for( TA_Integer j = 0; j < nb && j < 2000; j++ )
        {
            unsigned long long bits;
            if( oinfo->type == TA_Output_Real ) memcpy(&bits, &output[o][j], sizeof(bits));
            else                                bits = (unsigned long long)(unsigned)output_int[o][j];
            h = (h ^ bits) * 1099511628211ULL;
        }
    }
    return h;
}

static ErrorNumber d2_drive( const char *funcName, const TA_FuncHandle *handle,
                             const TA_FuncInfo *funcInfo, TA_ParamHolder *paramHolder,
                             const double *input, int size, const double *vec,
                             const char *what, int relaxValues,
                             int vecClass, const D2Baseline *base )
{
    TA_RetCode rc;
    TA_Integer beg = 0, nb = 0, lookback = -1;

    d2_set_opts(paramHolder, handle, funcInfo, vec);
    rc = TA_CallFunc(paramHolder, 0, size - 1, &beg, &nb);
    if( TA_GetLookback(paramHolder, &lookback) != TA_SUCCESS ) lookback = -1;

    /* An out-of-range probe the C library ACCEPTS is not out of range, and the
     * retCode parity asserted on it below is then vacuous. */
    if( vecClass == D2_CLASS_REJECT && rc != TA_BAD_PARAM )
    {
        g_d2OorNotRejected++;
        printf("  ABSTRACT ERROR [%s]: an out-of-range parameter was ACCEPTED by C "
               "(rc=%d) — the probe is not out of range and its parity is vacuous\n",
               funcName, (int)rc);
        return TA_ABSTRACT_CALL_MISMATCH;
    }

    /* The sentinel must resolve to the DECLARED DEFAULT. Comparing C-with-sentinel
     * against server-with-sentinel only proves the two agree; if C stopped
     * substituting, both would be wrong together. Compare against the
     * all-defaults result C produced before the sweep. */
    if( vecClass == D2_CLASS_SENTINEL && base )
    {
        unsigned long long dig = (rc == TA_SUCCESS) ? d2_digest(funcInfo, handle, nb) : 0;
        if( rc != base->rc || beg != base->beg || nb != base->nb
            || lookback != base->lookback || dig != base->sum )
        {
            g_d2SentNotDefault++;
            printf("  ABSTRACT ERROR [%s]: the default sentinel did not reproduce the "
                   "declared default in C (rc %d/%d beg %d/%d nb %d/%d lb %d/%d)\n",
                   funcName, (int)rc, (int)base->rc, beg, base->beg, nb, base->nb,
                   lookback, base->lookback);
            return TA_ABSTRACT_CALL_MISMATCH;
        }
    }

    g_d2Vectors++;

    ErrorNumber e = abstract_verify_server_call(
        funcName, handle, funcInfo, input, size, 0, size - 1,
        rc, beg, nb, lookback, output, output_int, relaxValues, vec);
    if( e != TA_TEST_PASS )
        printf("  ABSTRACT ERROR [%s]: the %s vector diverged\n", funcName, what);
    return e;
}

/* The whole sweep for one function. */
/* Every REAL optional parameter of every function, at NaN, +Inf and -Inf: the
 * guarded call and the lookback must both reject it.
 *
 * Systematic by construction -- driven off the declared parameter domains, so a
 * new function or a new real parameter is covered the day it lands, with no
 * list to maintain here.
 *
 * Why this is not folded into d2_param_vectors' out-of-range class, which would
 * have compared C against every language server for free: that class ships its
 * vector over JSON as `%.17g`, and NaN has no JSON number form (the same wall
 * the candle `factor` hit in #215, which is why it goes as hex bits). Carrying
 * non-finite parameters cross-language needs that transport in the abstract RPC
 * and in all four server parsers. So this leg is C-only, and deliberately does
 * NOT depend on a server pipe -- it runs in a bare ./ta_regtest, which is what
 * the autotools dist nightly executes.
 *
 * Two properties, both worth having:
 *   NaN   -- the one a range check does NOT catch. `x < min` and `x > max` are
 *            both false for NaN, so the obvious spelling accepts it; the emitted
 *            form is inverted, `!(x >= min && x <= max)`, precisely for this.
 *   +/-Inf -- already rejected, because every declared bound is finite (+/-3e37
 *            at the widest). Asserted anyway: it pins that property against a
 *            future parameter given an unbounded domain, where the inverted
 *            spelling alone would no longer be enough.
 */
static ErrorNumber d2_nonfinite_params( const char *funcName, const TA_FuncHandle *handle,
                                        const TA_FuncInfo *funcInfo,
                                        TA_ParamHolder *paramHolder, int size )
{
    if( funcInfo->nbOptInput == 0 || funcInfo->nbOptInput > D2_MAX_OPT )
        return TA_TEST_PASS;

    static const double bad[3] = { (double)NAN, (double)INFINITY, -(double)INFINITY };
    static const char  *badName[3] = { "NaN", "+Inf", "-Inf" };

    double base[D2_MAX_OPT], vec[D2_MAX_OPT];
    const TA_OptInputParameterInfo *oi;
    unsigned int k, j;
    int probed = 0;

    for( k = 0; k < funcInfo->nbOptInput; k++ )
    {
        TA_GetOptInputParameterInfo(handle, k, &oi);
        base[k] = oi->defaultValue;
    }

    for( k = 0; k < funcInfo->nbOptInput; k++ )
    {
        TA_GetOptInputParameterInfo(handle, k, &oi);
        /* Integer and choice-list slots cannot hold a non-finite value. */
        if( oi->type != TA_OptInput_RealRange && oi->type != TA_OptInput_RealList )
            continue;

        for( int b = 0; b < 3; b++ )
        {
            TA_RetCode rc;
            TA_Integer beg = 0, nb = 0, lookback = -1;

            for( j = 0; j < funcInfo->nbOptInput; j++ ) vec[j] = base[j];
            vec[k] = bad[b];

            d2_set_opts(paramHolder, handle, funcInfo, vec);
            rc = TA_CallFunc(paramHolder, 0, size - 1, &beg, &nb);
            if( rc != TA_BAD_PARAM )
            {
                printf("  ABSTRACT ERROR [%s]: %s in real parameter '%s' was ACCEPTED "
                       "(rc=%d) -- a range test of the form `x < min || x > max` is "
                       "FALSE for NaN and lets it through\n",
                       funcName, badName[b], oi->paramName, (int)rc);
                return TA_ABSTRACT_CALL_MISMATCH;
            }
            g_d2NonFinite++;
            probed++;

            /* The lookback validates the same parameters and must agree. A
             * lookback that accepted what the call rejects would hand a caller a
             * buffer size for a call that cannot run. */
            if( TA_GetLookback(paramHolder, &lookback) == TA_SUCCESS && lookback >= 0 )
            {
                printf("  ABSTRACT ERROR [%s]: the CALL rejected %s in '%s' but the "
                       "LOOKBACK accepted it (returned %d)\n",
                       funcName, badName[b], oi->paramName, lookback);
                return TA_ABSTRACT_CALL_MISMATCH;
            }
            g_d2NonFinite++;
        }
    }

    if( probed > 0 ) g_d2NonFiniteFuncs++;

    /* Leave the holder as the caller had it. */
    d2_set_opts(paramHolder, handle, funcInfo, base);
    return TA_TEST_PASS;
}

static ErrorNumber d2_param_vectors( const char *funcName, const TA_FuncHandle *handle,
                                     const TA_FuncInfo *funcInfo,
                                     TA_ParamHolder *paramHolder,
                                     const double *input, int size, int relaxValues )
{
    if( !g_abstractPipe || funcInfo->nbOptInput == 0 ) return TA_TEST_PASS;
    if( funcInfo->nbOptInput > D2_MAX_OPT )
    {
        printf("  ABSTRACT ERROR [%s]: %u optional parameters exceeds D2_MAX_OPT (%d) — "
               "the parameter-contract sweep would skip this function silently\n",
               funcName, funcInfo->nbOptInput, D2_MAX_OPT);
        return TA_ABSTRACT_CALL_MISMATCH;
    }

    double base[D2_MAX_OPT], vec[D2_MAX_OPT];
    const TA_OptInputParameterInfo *oi;
    unsigned int k, j;

    for( k = 0; k < funcInfo->nbOptInput; k++ )
    {
        TA_GetOptInputParameterInfo(handle, k, &oi);
        base[k] = oi->defaultValue;
    }

    /* The all-defaults C result the sentinel class is asserted against. The caller
     * has just made exactly this call, but recompute it here so this function does
     * not depend on the caller's buffers being untouched. */
    D2Baseline baseline;
    d2_set_opts(paramHolder, handle, funcInfo, base);
    baseline.rc = TA_CallFunc(paramHolder, 0, size - 1, &baseline.beg, &baseline.nb);
    if( TA_GetLookback(paramHolder, &baseline.lookback) != TA_SUCCESS ) baseline.lookback = -1;
    baseline.sum = (baseline.rc == TA_SUCCESS) ? d2_digest(funcInfo, handle, baseline.nb) : 0;

    /* 1. Every parameter at a DISTINCT non-default in-range value at once, so a
     *    transposed slot changes the answer even between same-default siblings. */
    for( k = 0; k < funcInfo->nbOptInput; k++ )
    {
        TA_GetOptInputParameterInfo(handle, k, &oi);
        vec[k] = d2_non_default(oi, k);
    }
    g_d2NonDefault++;
    ErrorNumber e = d2_drive(funcName, handle, funcInfo, paramHolder, input, size,
                             vec, "non-default", relaxValues, D2_CLASS_NON_DEFAULT, NULL);
    if( e != TA_TEST_PASS ) return e;

    /* 2. and 3., one parameter at a time so a failure names the slot. */
    for( k = 0; k < funcInfo->nbOptInput; k++ )
    {
        TA_GetOptInputParameterInfo(handle, k, &oi);
        for( j = 0; j < funcInfo->nbOptInput; j++ ) vec[j] = base[j];

        vec[k] = ( oi->type == TA_OptInput_RealRange || oi->type == TA_OptInput_RealList )
                 ? TA_REAL_DEFAULT : (double)TA_INTEGER_DEFAULT;
        g_d2Sentinel++;
        e = d2_drive(funcName, handle, funcInfo, paramHolder, input, size,
                     vec, "default-sentinel", relaxValues, D2_CLASS_SENTINEL, &baseline);
        if( e != TA_TEST_PASS ) return e;

        /* BOTH bounds. A choice list has no expressible outside -- that is a
         * different contract this sweep does not claim. */
        double bad[2];
        int nbad = d2_out_of_range(oi, bad);
        for( int b = 0; b < nbad; b++ )
        {
            for( j = 0; j < funcInfo->nbOptInput; j++ ) vec[j] = base[j];
            vec[k] = bad[b];
            g_d2Reject++;
            e = d2_drive(funcName, handle, funcInfo, paramHolder, input, size,
                         vec, "out-of-range", relaxValues, D2_CLASS_REJECT, NULL);
            if( e != TA_TEST_PASS ) return e;
        }
    }

    /* Leave the holder as the caller had it. */
    d2_set_opts(paramHolder, handle, funcInfo, base);
    return TA_TEST_PASS;
}

/* ---- TA_GetFuncHandle matches a name under an ASCII case fold (#278) ------
 *
 * Swept over the whole corpus rather than spot-checked on "SMA": the fold runs
 * per character, and the names that catch a partial one are the long ones
 * (CDL3STARSINSOUTH), the ones carrying a digit (CDL2CROWS) and the ones
 * carrying an underscore (HT_DCPERIOD) -- no single name stands in for those.
 *
 * The two spellings probed are all-lower and alternating case, so between them
 * every letter position is presented in both cases.
 */
typedef struct
{
   int nbChecked;
   int nbCanonical;
   int nbFailed;
} NameFoldCtx;

static void nameFoldCb( const TA_FuncInfo *funcInfo, void *opaqueData )
{
   NameFoldCtx *ctx = (NameFoldCtx *)opaqueData;
   const TA_FuncHandle *canonical;
   const TA_FuncHandle *handle;
   const TA_FuncInfo *back = NULL;
   char spelling[2][128];
   size_t i, len;
   int variant;

   len = strlen( funcInfo->name );
   if( len >= sizeof(spelling[0]) )
   {
      printf( "  ABSTRACT ERROR: '%s' overruns the case-fold probe's buffer\n",
              funcInfo->name );
      ctx->nbFailed++;
      return;
   }

   for( i = 0; i < len; i++ )
   {
      char c = funcInfo->name[i];
      char lower = ( c >= 'A' && c <= 'Z' ) ? (char)( c + ( 'a' - 'A' ) ) : c;
      char upper = ( c >= 'a' && c <= 'z' ) ? (char)( c - ( 'a' - 'A' ) ) : c;

      spelling[0][i] = lower;
      spelling[1][i] = ( i & 1 ) ? upper : lower;
   }
   spelling[0][len] = '\0';
   spelling[1][len] = '\0';

   if( TA_GetFuncHandle( funcInfo->name, &canonical ) != TA_SUCCESS )
   {
      printf( "  ABSTRACT ERROR: '%s' does not resolve under its own spelling\n",
              funcInfo->name );
      ctx->nbFailed++;
      return;
   }

   /* "Canonical" has to name something for a fold to fold onto it. Every
    * spelling probed below is derived from this entry, so a table entry stored
    * in lower case would fold onto itself just as happily and no probe here
    * would notice; this is the only line that does. Same assertion the Rust
    * registry sweep carries (abstract_api.rs, registry_tests).
    */
   ctx->nbCanonical++;
   for( i = 0; i < len; i++ )
   {
      if( funcInfo->name[i] >= 'a' && funcInfo->name[i] <= 'z' )
      {
         printf( "  ABSTRACT ERROR: '%s' is not stored in its canonical upper case\n",
                 funcInfo->name );
         ctx->nbFailed++;
         break;
      }
   }

   for( variant = 0; variant < 2; variant++ )
   {
      ctx->nbChecked++;

      if( TA_GetFuncHandle( spelling[variant], &handle ) != TA_SUCCESS )
      {
         printf( "  ABSTRACT ERROR: '%s' does not resolve for '%s'\n",
                 spelling[variant], funcInfo->name );
         ctx->nbFailed++;
         continue;
      }

      if( handle != canonical )
      {
         printf( "  ABSTRACT ERROR: '%s' resolves to a different handle than '%s'\n",
                 spelling[variant], funcInfo->name );
         ctx->nbFailed++;
         continue;
      }

      /* Anchored to the row being enumerated, not to a second lookup of its
       * own name. `handle == canonical` alone would be satisfied by a lookup
       * that sent every spelling of every name to the SAME wrong function, so
       * on its own it says only that the fold is consistent, not that it is
       * right. TA_ForEachFunc hands out `funcDef->funcInfo` and TA_GetFuncInfo
       * answers that same pointer (ta_abstract.c), so identity here is the C
       * spelling of what the other three backends assert directly against the
       * row they are iterating (`lower == f` / `Some(f.id)`).
       *
       * Comparing `back->name` to `funcInfo->name` instead, as this did, says
       * the same thing only while every name in the table is unique -- which
       * is true, and which nothing in this sweep checks. Pointer identity does
       * not lean on it.
       */
      if( TA_GetFuncInfo( handle, &back ) != TA_SUCCESS || back != funcInfo )
      {
         printf( "  ABSTRACT ERROR: '%s' resolves to '%s', not to the '%s' row\n",
                 spelling[variant],
                 ( back && back->name ) ? back->name : "(none)",
                 funcInfo->name );
         ctx->nbFailed++;
      }
   }
}

static ErrorNumber checkFuncHandleFoldsCase( void )
{
   NameFoldCtx ctx;
   const TA_FuncHandle *handle;
   TA_RetCode retCode;
   unsigned int i;

   /* A fold is a relaxation of the match, not of what a name is. Each of these
    * must still miss, or the "fold" has started inventing functions.
    */
   static const char * const notNames[] = {
      "SMA ",          /* trailing space                                   */
      " SMA",          /* leading space                                    */
      "S MA",          /* interior space                                   */
      "HT-DCPERIOD",   /* '-' is not '_'                                   */
      "SMAA",          /* longer                                           */
      "SM",            /* shorter                                          */
      "S\xC4\xB0N",    /* UTF-8 'İ' (U+0130): the Turkish-locale trap      */
      "s\xC4\xB1n",    /* UTF-8 'ı' (U+0131): the same trap, other way     */
      "NOSUCHFUNCTION"
   };

   ctx.nbChecked   = 0;
   ctx.nbCanonical = 0;
   ctx.nbFailed    = 0;
   TA_ForEachFunc( nameFoldCb, &ctx );

   if( ctx.nbChecked == 0 || ctx.nbCanonical == 0 )
   {
      printf( "  ABSTRACT ERROR: the case-fold probe compared nothing\n" );
      return TA_ABS_TST_FAIL_NAME_CASE_FOLD;
   }

   for( i = 0; i < (unsigned int)(sizeof(notNames)/sizeof(notNames[0])); i++ )
   {
      retCode = TA_GetFuncHandle( notNames[i], &handle );
      if( retCode != TA_FUNC_NOT_FOUND )
      {
         printf( "  ABSTRACT ERROR: '%s' resolved [%d]; the fold widened the match\n",
                 notNames[i], retCode );
         return TA_ABS_TST_FAIL_NAME_CASE_FOLD;
      }
   }

   /* The empty name and a NULL are still bad parameters, not a miss. */
   if( TA_GetFuncHandle( "", &handle ) != TA_BAD_PARAM ||
       TA_GetFuncHandle( NULL, &handle ) != TA_BAD_PARAM )
   {
      printf( "  ABSTRACT ERROR: an empty or NULL name is no longer TA_BAD_PARAM\n" );
      return TA_ABS_TST_FAIL_NAME_CASE_FOLD;
   }

   if( ctx.nbFailed != 0 )
      return TA_ABS_TST_FAIL_NAME_CASE_FOLD;

   printf( "  Name lookup case fold: %d spellings resolved to their own row, "
           "%d name(s) stored upper case\n",
           ctx.nbChecked, ctx.nbCanonical );
   return TA_TEST_PASS;
}

ErrorNumber test_abstract( void )
{
   ErrorNumber retValue;
   TA_RetCode retCode;
   TA_ParamHolder *paramHolder;
   const TA_FuncHandle *handle;
   int i;
   const char *xmlArray;

   printf( "Testing Abstract interface\n" );

   retValue = allocLib();
   if( retValue != TA_TEST_PASS )
      return retValue;

   /* Verify that a name resolves whatever case it is spelled in. */
   retValue = checkFuncHandleFoldsCase();
   if( retValue != TA_TEST_PASS )
      return retValue;

   /* Verify TA_GetLookback. */
   retCode = TA_GetFuncHandle( "STOCH", &handle );
   if( retCode != TA_SUCCESS )
   {
      printf( "Can't get the function handle [%d]\n", retCode );
      return TA_ABS_TST_FAIL_GETFUNCHANDLE;
   }

   retCode = TA_ParamHolderAlloc( handle, &paramHolder );
   if( retCode != TA_SUCCESS )
   {
      printf( "Can't allocate the param holder [%d]\n", retCode );
      return TA_ABS_TST_FAIL_PARAMHOLDERALLOC;
   }

   retValue = testLookback(paramHolder);
   if( retValue != TA_TEST_PASS )
   {
      printf( "testLookback() failed [%d]\n", retValue );
      TA_ParamHolderFree( paramHolder );
      return retValue;
   }

   retCode = TA_ParamHolderFree( paramHolder );
   if( retCode != TA_SUCCESS )
   {
      printf( "TA_ParamHolderFree failed [%d]\n", retCode );
      return TA_ABS_TST_FAIL_PARAMHOLDERFREE;
   }

   retValue = freeLib();
   if( retValue != TA_TEST_PASS )
      return retValue;

   /* Call all the TA functions through the abstract interface. */
   retValue = allocLib();
   if( retValue != TA_TEST_PASS )
      return retValue;

   retValue = test_default_calls();
   if( retValue != TA_TEST_PASS )
   {
      printf( "TA-Abstract default call failed\n" );
      return retValue;
   }

   retValue = freeLib();
   if( retValue != TA_TEST_PASS )
      return retValue;

   /* Verify that the TA_FunctionDescription is null terminated
    * and as at least 500 characters (less is guaranteed bad...)
    */
   xmlArray = TA_FunctionDescriptionXML();
   {
      unsigned long long crefChecksum = 0;
      for( i=0; i < 1000000; i++ )
      {
         if( xmlArray[i] == 0x0 )
            break;
         crefChecksum += (unsigned char)xmlArray[i];
      }

      if( i < 500)
      {
         printf( "TA_FunctionDescriptionXML failed. Size too small.\n" );
         return TA_ABS_TST_FAIL_FUNCTION_DESC_SMALL;
      }

      if( i == 1000000 )
      {
         printf( "TA_FunctionDescriptionXML failed. Size too large (missing null?).\n" );
         return TA_ABS_TST_FAIL_FUNCTION_DESC_LARGE;
      }

      /* If server is connected, verify TA_FunctionDescriptionXML length and
       * order-independent checksum (byte sum) match. Using a byte-sum checksum
       * allows the XML to have functions in different sort order while still
       * verifying the same content is present. */
      if( g_abstractPipe )
      {
         snprintf(g_abstractReqBuf, ABSTRACT_JSON_BUF_SIZE,
             "{\"method\":\"TA_FunctionDescriptionXML\"}");
         ErrorNumber srvErr = codegen_pipe_call(g_abstractPipe, g_abstractReqBuf,
                                                 g_abstractRespBuf, ABSTRACT_JSON_BUF_SIZE);
         if( srvErr != TA_TEST_PASS || abstract_json_is_error(g_abstractRespBuf) )
         {
            printf("  ABSTRACT ERROR: TA_FunctionDescriptionXML server error\n");
            return TA_ABSTRACT_SERVER_ERROR;
         }
         {
            int srvLen = abstract_json_get_int(g_abstractRespBuf, "length");
            unsigned long long srvChecksum = abstract_json_get_ull(g_abstractRespBuf, "checksum");
            if( srvLen != i )
            {
               printf("  ABSTRACT ERROR: TA_FunctionDescriptionXML length C=%d server=%d\n",
                      i, srvLen);
               return TA_ABSTRACT_CALL_MISMATCH;
            }
            if( srvChecksum != crefChecksum )
            {
               printf("  ABSTRACT ERROR: TA_FunctionDescriptionXML checksum C=%llu server=%llu\n",
                      crefChecksum, srvChecksum);
               return TA_ABSTRACT_CALL_MISMATCH;
            }
      }
   }
   } /* end crefChecksum scope */

   printf( "  Non-finite parameter contract: %lld probe(s) over %lld function(s) "
           "— every real optional parameter at NaN, +Inf and -Inf, call and lookback\n",
           g_d2NonFinite, g_d2NonFiniteFuncs );
   /* Literal floors. 24 real parameters x 3 values x 2 assertions (call and
    * lookback) = 144, across the 14 functions that declare one. Derived counts
    * would move with the corpus and let the sweep go quiet unnoticed; these fail
    * loudly if a parameter stops being probed. Raise them when a real parameter
    * is added -- that is the point. */
   if( g_d2NonFinite < 144 || g_d2NonFiniteFuncs < 14 )
   {
      printf( "  Failed: the non-finite parameter sweep probed %lld value(s) over "
              "%lld function(s); it was written with 144 over 14\n",
              g_d2NonFinite, g_d2NonFiniteFuncs );
      return TA_ABSTRACT_CALL_MISMATCH;
   }

   if( g_abstractPipe )
   {
      printf( "  Abstract server verification: all calls match C\n" );
      printf( "  Binder parameter contract (#164): %lld vector(s) driven through both "
              "binders — %lld non-default, %lld default-sentinel, %lld out-of-range "
              "(both bounds, integer and real)\n",
              g_d2Vectors, g_d2NonDefault, g_d2Sentinel, g_d2Reject );

      /* Every count is asserted, not merely printed. The whole sweep is built
       * from the declared domains, so a metadata change (a range collapsing, a
       * domain retyped) can stop it producing vectors — and a sweep that quietly
       * built none reads exactly like a passing one. Each class is what it is:
       * non-default and sentinel exist for every function with a parameter,
       * out-of-range only for the integer ranges (a choice list has no
       * expressible outside, which is a different contract). */
      /* The two self-checks fail the run where they fire, so reaching here with a
         non-zero count would mean one was counted but not acted on. */
      if( g_d2OorNotRejected != 0 || g_d2SentNotDefault != 0 )
      {
         printf( "  ABSTRACT ERROR: %lld out-of-range probe(s) accepted by C and %lld "
                 "sentinel(s) that did not reproduce the declared default\n",
                 g_d2OorNotRejected, g_d2SentNotDefault );
         return TA_ABSTRACT_CALL_MISMATCH;
      }

      if( g_d2Vectors == 0 || g_d2NonDefault == 0 || g_d2Sentinel == 0 || g_d2Reject == 0 )
      {
         printf( "  ABSTRACT ERROR: the binder parameter-contract sweep produced no "
                 "vectors of some class (%lld/%lld/%lld/%lld) — the binders are back "
                 "to being tested only at their declared defaults\n",
                 g_d2Vectors, g_d2NonDefault, g_d2Sentinel, g_d2Reject );
         return TA_ABSTRACT_CALL_MISMATCH;
      }
   }

   return TA_TEST_PASS; /* Succcess. */
}

/**** Local functions definitions.     ****/
static ErrorNumber testLookback( TA_ParamHolder *paramHolder )
{
  TA_RetCode retCode;
  int lookback;

  /* Change the parameters of STOCH and verify that TA_GetLookback respond correctly. */
  retCode = TA_SetOptInputParamInteger( paramHolder, 0, 3 );
  if( retCode != TA_SUCCESS )
  {
     printf( "TA_SetOptInputParamInteger call failed [%d]\n", retCode );
     return TA_ABS_TST_FAIL_OPTINPUTPARAMINTEGER;
  }

  retCode = TA_SetOptInputParamInteger( paramHolder, 1, 4 );
  if( retCode != TA_SUCCESS )
  {
     printf( "TA_SetOptInputParamInteger call failed [%d]\n", retCode );
     return TA_ABS_TST_FAIL_OPTINPUTPARAMINTEGER;
  }

  retCode = TA_SetOptInputParamInteger( paramHolder, 2, (TA_Integer)TA_MAType_SMA );
  if( retCode != TA_SUCCESS )
  {
     printf( "TA_SetOptInputParamInteger call failed [%d]\n", retCode );
     return TA_ABS_TST_FAIL_OPTINPUTPARAMINTEGER;
  }

  retCode = TA_SetOptInputParamInteger( paramHolder, 3, 4 );
  if( retCode != TA_SUCCESS )
  {
     printf( "TA_SetOptInputParamInteger call failed [%d]\n", retCode );
     return TA_ABS_TST_FAIL_OPTINPUTPARAMINTEGER;
  }

  retCode = TA_SetOptInputParamInteger( paramHolder, 4, (TA_Integer)TA_MAType_SMA );
  if( retCode != TA_SUCCESS )
  {
     printf( "TA_SetOptInputParamInteger call failed [%d]\n", retCode );
     return TA_ABS_TST_FAIL_OPTINPUTPARAMINTEGER;
  }

  retCode = TA_GetLookback(paramHolder,&lookback);
  if( retCode != TA_SUCCESS )
  {
     printf( "TA_GetLookback failed [%d]\n", retCode );
     return TA_ABS_TST_FAIL_GETLOOKBACK_CALL_1;
  }

  if( lookback != 8 )
  {
     printf( "TA_GetLookback failed [%d != 8]\n", lookback );
     return TA_ABS_TST_FAIL_GETLOOKBACK_1;
  }

  /* Verify server agrees with C lookback (params: 3,4,SMA,4,SMA). */
  if( g_abstractPipe )
  {
     snprintf(g_abstractReqBuf, ABSTRACT_JSON_BUF_SIZE,
         "{\"method\":\"abstract_get_lookback\",\"params\":{"
         "\"funcName\":\"STOCH\","
         "\"optInFastK_Period\":3,\"optInSlowK_Period\":4,"
         "\"optInSlowK_MAType\":0,\"optInSlowD_Period\":4,"
         "\"optInSlowD_MAType\":0}}");
     ErrorNumber srvErr = codegen_pipe_call(g_abstractPipe, g_abstractReqBuf,
                                            g_abstractRespBuf, ABSTRACT_JSON_BUF_SIZE);
     if( srvErr != TA_TEST_PASS || abstract_json_is_error(g_abstractRespBuf) )
     {
        printf("ABSTRACT ERROR: STOCH abstract_get_lookback server error\n");
        return TA_ABSTRACT_SERVER_ERROR;
     }
     int srvLookback = abstract_json_get_int(g_abstractRespBuf, "lookback");
     if( srvLookback != lookback )
     {
        printf("ABSTRACT ERROR: STOCH lookback C=%d server=%d\n",
               lookback, srvLookback);
        return TA_ABSTRACT_LOOKBACK_MISMATCH;
     }
  }

  /* Change one parameter and check again. */
  retCode = TA_SetOptInputParamInteger( paramHolder, 3, 3 );
  if( retCode != TA_SUCCESS )
  {
     printf( "TA_SetOptInputParamInteger call failed [%d]\n", retCode );
     return TA_ABS_TST_FAIL_OPTINPUTPARAMINTEGER;
  }

  retCode = TA_GetLookback(paramHolder,&lookback);
  if( retCode != TA_SUCCESS )
  {
     printf( "TA_GetLookback failed [%d]\n", retCode );
     return TA_ABS_TST_FAIL_GETLOOKBACK_CALL_2;
  }

  if( lookback != 7 )
  {
     printf( "TA_GetLookback failed [%d != 7]\n", lookback );
     return TA_ABS_TST_FAIL_GETLOOKBACK_2;
  }

  /* Verify server agrees with changed param (params: 3,4,SMA,3,SMA). */
  if( g_abstractPipe )
  {
     snprintf(g_abstractReqBuf, ABSTRACT_JSON_BUF_SIZE,
         "{\"method\":\"abstract_get_lookback\",\"params\":{"
         "\"funcName\":\"STOCH\","
         "\"optInFastK_Period\":3,\"optInSlowK_Period\":4,"
         "\"optInSlowK_MAType\":0,\"optInSlowD_Period\":3,"
         "\"optInSlowD_MAType\":0}}");
     ErrorNumber srvErr = codegen_pipe_call(g_abstractPipe, g_abstractReqBuf,
                                            g_abstractRespBuf, ABSTRACT_JSON_BUF_SIZE);
     if( srvErr != TA_TEST_PASS || abstract_json_is_error(g_abstractRespBuf) )
     {
        printf("ABSTRACT ERROR: STOCH abstract_get_lookback server error\n");
        return TA_ABSTRACT_SERVER_ERROR;
     }
     int srvLookback = abstract_json_get_int(g_abstractRespBuf, "lookback");
     if( srvLookback != lookback )
     {
        printf("ABSTRACT ERROR: STOCH lookback C=%d server=%d\n",
               lookback, srvLookback);
        return TA_ABSTRACT_LOOKBACK_MISMATCH;
     }
  }

  return TA_TEST_PASS;
}

/* Some processings are a bit different for functions under
 * the Math Operator and Math Transform category.
 */
static int isMath( const TA_FuncInfo *funcInfo )
{
   int notMath;
   notMath = (strlen(funcInfo->group) < 4) ||
	   !((tolower(funcInfo->group[0]) == 'm') &&
	     (tolower(funcInfo->group[1]) == 'a') &&
	     (tolower(funcInfo->group[2]) == 't') &&
	     (tolower(funcInfo->group[3]) == 'h'));

   return !notMath;
}

#if 0
// Unused for now
static int isCandlePattern( const TA_FuncInfo *funcInfo )
{
   int notCandlePattern;
   notCandlePattern = (strlen(funcInfo->group) < 3) ||
	   !((tolower(funcInfo->name[0]) == 'c') &&
	     (tolower(funcInfo->name[1]) == 'd') &&
	     (tolower(funcInfo->name[2]) == 'l'));

   return !notCandlePattern;
}
#endif

static void testDefault( const TA_FuncInfo *funcInfo, void *opaqueData )
{
	static int nbFunctionDone = 0;
   ErrorNumber *errorNumber;
   errorNumber = (ErrorNumber *)opaqueData;
   if( *errorNumber != TA_TEST_PASS )
      return;

/*   if( !isCandlePattern(funcInfo) )
	   return;*/

   /* Verify ta_abstract metadata once per function (not per dataset). */
   {
      const TA_FuncHandle *handle = funcInfo->handle;
      ErrorNumber srvErr = abstract_verify_func_metadata(funcInfo->name, handle, funcInfo);
      if( srvErr != TA_TEST_PASS )
      {
         *errorNumber = srvErr;
         printf( "Failed for [%s][metadata]\n", funcInfo->name );
         return;
      }
   }

#define CALL(x) { \
	*errorNumber = callWithDefaults( funcInfo->name, x, x##_int, sizeof(x)/sizeof(double), #x ); \
	if( *errorNumber != TA_TEST_PASS ) { \
	   printf( "Failed for [%s][%s]\n", funcInfo->name, #x ); \
       return; \
	} \
}
   /* Do not test value outside the ]0..1[ domain for the "Math" groups. */
   if( !isMath(funcInfo) )
   {
      CALL( inputNegData );
      CALL( inputZeroData );
      CALL( inputRandFltEpsilon );
      CALL( inputRandDblEpsilon );
   }

   CALL( inputRandomData );

#undef CALL

#define CALL(x) { \
	*errorNumber = callAndProfile( funcInfo->name, x ); \
	if( *errorNumber != TA_TEST_PASS ) { \
	   printf( "Failed for [%s][%s]\n", funcInfo->name, #x ); \
       return; \
	} \
}
   if( doExtensiveProfiling /*&& (nbFunctionDone<5)*/ )
   {
	   nbFunctionDone++;
	   printf( "%s ", funcInfo->name );
       CALL( PROFILING_100 );
       CALL( PROFILING_500 );
	   CALL( PROFILING_1000 );
       CALL( PROFILING_2000 );
       CALL( PROFILING_5000 );
       CALL( PROFILING_8000 );
	   CALL( PROFILING_10000 );
	   printf( "\n" );
   }
}

static ErrorNumber callWithDefaults( const char *funcName, const double *input, const int *input_int, int size, const char *datasetName )
{
   TA_ParamHolder *paramHolder;
   const TA_FuncHandle *handle;
   const TA_FuncInfo *funcInfo;
   const TA_InputParameterInfo *inputInfo;
   const TA_OutputParameterInfo *outputInfo;

   TA_RetCode retCode;
   unsigned int i;
   int j;
   int outBegIdx, outNbElement, lookback;

   /* Relax server output-VALUE parity for the floating-point-order-sensitive
    * functions on the random-noise datasets only — see abstract_verify_server_call()
    * for the full rationale. The Rust codegen is not bit-identical to the C
    * reference (residual ~1e-13 operation-ordering differences, independent of FMA).
    * For most functions/inputs that stays far under tolerance, but a few amplify it
    * into a *discrete* output difference at a degenerate boundary:
    *   - the Hilbert-Transform family (HT_*) — chaotic phase/trend-mode transforms
    *     that phase-wrap or flip their integer trend mode; and
    *   - CCI — whose `(lastValue-theAverage) != 0` guard flips between the 0.015
    *     division and a hard 0 when the mean and last value cancel to the last bit.
    * These only surface on the NON-deterministic inputs (random ]0,1[ values, and
    * the two random-sign epsilon sets), where the data is noise rather than a
    * price series, so exact value parity is not meaningful. Structural parity
    * (retCode/outBegIdx/outNBElement/lookback) stays strict for every function on
    * every dataset; value parity stays strict on the deterministic datasets
    * (monotonic ramp, zeros) and — on real price data — in test_codegen.
    * Both epsilon sets are listed: until the initialisation was fixed the Flt
    * array actually carried the DBL_EPSILON values, so naming one covered both.
    *
    * CDL* against the Rust server on these sets used to be excluded too, for a
    * defect that turned out to be in the TEST SERVER, not the library: serde_json
    * parsed 2.7755575615628914e-16 one ULP low, so the Rust server alone computed
    * on different inputs than C/Java/C#. CDLLONGLINE exposed it because
    * `upperShadow` and `candleaverage(ShadowShort)` are EXACTLY equal there, so
    * one ULP flips the verdict. Fixed at the parser (tools/Cargo.toml enables
    * serde_json's arbitrary_precision); the exclusion is gone and CDL* is held to
    * the same strict value parity as everything else. */
   int isEpsilonSet = ( datasetName != NULL )
                      && ( strcmp(datasetName, "inputRandFltEpsilon") == 0
                           || strcmp(datasetName, "inputRandDblEpsilon") == 0 );
   int isNoiseSet   = isEpsilonSet
                      || ( datasetName != NULL
                           && strcmp(datasetName, "inputRandomData") == 0 );

   int relaxValues =
         ( ( strncmp(funcName, "HT_", 3) == 0 || strcmp(funcName, "CCI") == 0 ) && isNoiseSet );

   retCode = TA_GetFuncHandle( funcName, &handle );
   if( retCode != TA_SUCCESS )
   {
      printf( "Can't get the function handle [%d]\n", retCode );
      return TA_ABS_TST_FAIL_GETFUNCHANDLE;
   }

   retCode = TA_ParamHolderAlloc( handle, &paramHolder );
   if( retCode != TA_SUCCESS )
   {
      printf( "Can't allocate the param holder [%d]\n", retCode );
      return TA_ABS_TST_FAIL_PARAMHOLDERALLOC;
   }

   TA_GetFuncInfo( handle, &funcInfo );

   /* Counts only the generic real slots, so inReal0/inReal1 get the same views
    * the JSON builder gives them (which counts them the same way). */
   int realSlot = 0;

   for( i=0; i < funcInfo->nbInput; i++ )
   {
      TA_GetInputParameterInfo( handle, i, &inputInfo );
	  switch(inputInfo->type)
	  {
	  case TA_Input_Price:
         /* One view per component -- see abstract_price_view(); volume gets the
          * magnitude only -- see abstract_volume_view(). The JSON request builder
          * applies the identical views. */
         /* openInterest is bound symmetrically with the JSON builder rather than
          * hardcoded NULL. No shipped function declares it, so this is dormant —
          * but a flagged component passed NULL is TA_BAD_PARAM from
          * SET_PARAM_INFO, so the asymmetry would have been a silent bind failure
          * on one arm the day a function did declare it. The rc is checked for
          * the same reason: it was being discarded. */
         {
            TA_RetCode bindRc = TA_SetInputParamPricePtr( paramHolder, i,
			 inputInfo->flags&TA_IN_PRICE_OPEN?abstract_price_view(input,(unsigned int)size,TA_IN_PRICE_OPEN):NULL,
			 inputInfo->flags&TA_IN_PRICE_HIGH?abstract_price_view(input,(unsigned int)size,TA_IN_PRICE_HIGH):NULL,
			 inputInfo->flags&TA_IN_PRICE_LOW?abstract_price_view(input,(unsigned int)size,TA_IN_PRICE_LOW):NULL,
			 inputInfo->flags&TA_IN_PRICE_CLOSE?abstract_price_view(input,(unsigned int)size,TA_IN_PRICE_CLOSE):NULL,
			 inputInfo->flags&TA_IN_PRICE_VOLUME?abstract_volume_view(input,(unsigned int)size):NULL,
			 inputInfo->flags&TA_IN_PRICE_OPENINTEREST?abstract_price_view(input,(unsigned int)size,TA_IN_PRICE_OPENINTEREST):NULL );
            if( bindRc != TA_SUCCESS )
            {
               printf( "  ABSTRACT ERROR [%s]: TA_SetInputParamPricePtr[%u] rc=%d\n",
                       funcName, i, (int)bindRc );
               TA_ParamHolderFree( paramHolder );
               return TA_ABS_TST_FAIL_CALLFUNC;
            }
         }
		 break;
	  case TA_Input_Real:
         TA_SetInputParamRealPtr( paramHolder, i,
             abstract_real_view(input,(unsigned int)size,realSlot++) );
		 break;
	  case TA_Input_Integer:
         TA_SetInputParamIntegerPtr( paramHolder, i, input_int );
         break;
	  }
   }

   for( i=0; i < funcInfo->nbOutput; i++ )
   {
      TA_GetOutputParameterInfo( handle, i, &outputInfo );
	  switch(outputInfo->type)
	  {
	  case TA_Output_Real:
	     TA_SetOutputParamRealPtr(paramHolder,i,&output[i][0]);
         for( j=0; j < 2000; j++ )
            output[i][j] = TA_REAL_MIN;
		 break;
	  case TA_Output_Integer:
	     TA_SetOutputParamIntegerPtr(paramHolder,i,&output_int[i][0]);
         for( j=0; j < 2000; j++ )
            output_int[i][j] = TA_INTEGER_MIN;
		 break;
	  }
   }

   /* Do the function call. */
   retCode = TA_CallFunc(paramHolder,0,size-1,&outBegIdx,&outNbElement);
   if( retCode != TA_SUCCESS )
   {
      printf( "TA_CallFunc() failed zero data test [%d]\n", retCode );
      TA_ParamHolderFree( paramHolder );
      return TA_ABS_TST_FAIL_CALLFUNC_1;
   }

   /* Verify consistency with Lookback */
   retCode = TA_GetLookback( paramHolder, &lookback );
   if( retCode != TA_SUCCESS )
   {
      printf( "TA_GetLookback() failed zero data test [%d]\n", retCode );
      TA_ParamHolderFree( paramHolder );
      return TA_ABS_TST_FAIL_CALLFUNC_2;
   }

   if( outBegIdx != lookback )
   {
      printf( "TA_GetLookback() != outBegIdx [%d != %d]\n", lookback, outBegIdx );
      TA_ParamHolderFree( paramHolder );
      return TA_ABS_TST_FAIL_CALLFUNC_3;
   }

   /* A successful call writes finite values -- unless the function declares
    * TA_FUNC_FLG_NAN_INF_OUT, the seven whose own domain has holes (ACOS/ASIN
    * outside [-1,1], LN/LOG10/SQRT on a negative, DIV on 0/0 or x/0, VWMA on a
    * volume-less window). Those are exempt; every other function is held to
    * finite output on all five datasets, which is what makes the flag a
    * contract rather than a docs annotation (issue #191).
    *
    * Placed HERE, against the call above, and not further down: the server
    * verification and d2_param_vectors both re-issue TA_CallFunc into these
    * same output buffers, so anywhere after them this would be reading another
    * vector's output against this call's outNbElement.
    *
    * The datasets stay well inside double's range, so this cannot fire on the
    * overflow class (non-finite only past ~1e160 of input) that is deliberately
    * unflagged.
    */
   if( !(funcInfo->flags & TA_FUNC_FLG_NAN_INF_OUT) )
   {
      for( i=0; i < funcInfo->nbOutput; i++ )
      {
         TA_GetOutputParameterInfo( handle, i, &outputInfo );
         if( outputInfo->type != TA_Output_Real )
            continue;
         for( j=0; j < outNbElement; j++ )
         {
            if( !isfinite(output[i][j]) )
            {
               printf( "Failed: non-finite output[%d][%d] = %e\n", i, j, output[i][j] );
               TA_ParamHolderFree( paramHolder );
               return TA_ABS_TST_FAIL_INVALID_OUTPUT;
            }
         }
      }
   }

   /* If server is connected, verify TA_GetLookback independently. */
   if( g_abstractPipe )
   {
      int pos = 0;
      pos = codegen_appendf(g_abstractReqBuf, ABSTRACT_JSON_BUF_SIZE, pos,
          "{\"method\":\"abstract_get_lookback\",\"params\":{\"funcName\":\"%s\"",
          funcName);

      /* Send same default params as C uses */
      for( unsigned int k = 0; k < funcInfo->nbOptInput; k++ )
      {
         const TA_OptInputParameterInfo *oi;
         TA_GetOptInputParameterInfo(handle, k, &oi);
         pos = codegen_appendf(g_abstractReqBuf, ABSTRACT_JSON_BUF_SIZE, pos,
             ",\"%s\":", oi->paramName);
         if( oi->type == TA_OptInput_RealRange || oi->type == TA_OptInput_RealList )
            pos = codegen_appendf(g_abstractReqBuf, ABSTRACT_JSON_BUF_SIZE, pos,
                "%.17g", oi->defaultValue);
         else
            pos = codegen_appendf(g_abstractReqBuf, ABSTRACT_JSON_BUF_SIZE, pos,
                "%d", (int)oi->defaultValue);
      }
      codegen_appendf(g_abstractReqBuf, ABSTRACT_JSON_BUF_SIZE, pos, "}}");

      ErrorNumber srvErr = codegen_pipe_call(g_abstractPipe, g_abstractReqBuf,
                                              g_abstractRespBuf, ABSTRACT_JSON_BUF_SIZE);
      if( srvErr != TA_TEST_PASS || abstract_json_is_error(g_abstractRespBuf) )
      {
         printf("  ABSTRACT ERROR [%s]: abstract_get_lookback server error\n", funcName);
         TA_ParamHolderFree( paramHolder );
         return TA_ABSTRACT_SERVER_ERROR;
      }
      {
         int srvLookback = abstract_json_get_int(g_abstractRespBuf, "lookback");
         if( srvLookback != lookback )
         {
            printf("  ABSTRACT ERROR [%s]: TA_GetLookback C=%d server=%d\n",
                   funcName, lookback, srvLookback);
            TA_ParamHolderFree( paramHolder );
            return TA_ABSTRACT_LOOKBACK_MISMATCH;
         }
      }
   }

   /* If server is connected, replicate the full TA_CallFunc and compare. */
   {
      ErrorNumber srvErr = abstract_verify_server_call(
          funcName, handle, funcInfo, input, size,
          0, size-1,
          TA_SUCCESS, outBegIdx, outNbElement, lookback,
          output, output_int, relaxValues, NULL);
      if( srvErr != TA_TEST_PASS )
      {
         TA_ParamHolderFree( paramHolder );
         return srvErr;
      }

      /* D2: the same binder, off its defaults. Bounded to ONE dataset on purpose
       * -- the contract being asserted is about parameter VALUES, not about the
       * data, so running it on all five would multiply the cost without adding a
       * claim. inputRandomData is the one with real dynamic range. */
      if( datasetName && strcmp(datasetName, "inputRandomData") == 0 )
      {
         srvErr = d2_param_vectors( funcName, handle, funcInfo, paramHolder,
                                    input, size, relaxValues );
         if( srvErr != TA_TEST_PASS )
         {
            TA_ParamHolderFree( paramHolder );
            return srvErr;
         }

         /* C-only, so unlike the sweep above it runs with no server pipe. */
         srvErr = d2_nonfinite_params( funcName, handle, funcInfo, paramHolder, size );
         if( srvErr != TA_TEST_PASS )
         {
            TA_ParamHolderFree( paramHolder );
            return srvErr;
         }
      }
   }

   /* Do another function call where startIdx == endIdx == 0.
    * In that case, outBegIdx should ALWAYS be zero.
    */
   retCode = TA_CallFunc(paramHolder,0,0,&outBegIdx,&outNbElement);
   if( retCode != TA_SUCCESS )
   {
      printf( "TA_CallFunc() failed data test 4 [%d]\n", retCode );
      TA_ParamHolderFree( paramHolder );
      return TA_ABS_TST_FAIL_CALLFUNC_4;
   }

   if( outBegIdx != 0 )
   {
      printf( "failed outBegIdx=%d when startIdx==endIdx==0\n", outBegIdx );
      TA_ParamHolderFree( paramHolder );
      return TA_ABS_TST_FAIL_STARTEND_ZERO;
   }

   /* If server is connected, replicate the startIdx==endIdx==0 call. */
   {
      ErrorNumber srvErr = abstract_verify_server_call(
          funcName, handle, funcInfo, input, size,
          0, 0,
          retCode, outBegIdx, outNbElement, lookback,
          output, output_int, relaxValues, NULL);
      if( srvErr != TA_TEST_PASS )
      {
         TA_ParamHolderFree( paramHolder );
         return srvErr;
      }
   }

   retCode = TA_ParamHolderFree( paramHolder );
   if( retCode != TA_SUCCESS )
   {
      printf( "TA_ParamHolderFree failed [%d]\n", retCode );
      return TA_ABS_TST_FAIL_PARAMHOLDERFREE;
   }

   return TA_TEST_PASS;
}

/* Passing the same buffer for two different output arguments has no correct
 * result (each output clobbers the other), so every function with two or more
 * outputs must reject it with TA_BAD_PARAM. For a given function this aliases
 * each pair of same-typed outputs to a single buffer (leaving the others
 * distinct) and verifies the rejection. Driven generically over every function
 * ta_abstract reports, so current and future multi-output functions are covered.
 * (Issue #108.) */
static ErrorNumber checkOutputAliasRejected( const TA_FuncInfo *funcInfo )
{
   TA_ParamHolder *paramHolder;
   const TA_FuncHandle *handle = funcInfo->handle;
   const TA_InputParameterInfo *inputInfo;
   const TA_OutputParameterInfo *outInfoA, *outInfoB, *outInfo;
   TA_RetCode retCode;
   unsigned int i, oa, ob;
   int outBegIdx, outNbElement;
   const int size = 252;

   if( funcInfo->nbOutput < 2 )
      return TA_TEST_PASS;

   for( oa = 0; oa < funcInfo->nbOutput; oa++ )
   {
      for( ob = oa + 1; ob < funcInfo->nbOutput; ob++ )
      {
         /* Two outputs can only share one buffer if they are the same type. */
         TA_GetOutputParameterInfo( handle, oa, &outInfoA );
         TA_GetOutputParameterInfo( handle, ob, &outInfoB );
         if( outInfoA->type != outInfoB->type )
            continue;

         retCode = TA_ParamHolderAlloc( handle, &paramHolder );
         if( retCode != TA_SUCCESS )
            return TA_ABS_TST_FAIL_PARAMHOLDERALLOC;

         /* Inputs: every slot uses the same random array (mirrors callWithDefaults). */
         for( i = 0; i < funcInfo->nbInput; i++ )
         {
            TA_GetInputParameterInfo( handle, i, &inputInfo );
            switch( inputInfo->type )
            {
            case TA_Input_Price:
               TA_SetInputParamPricePtr( paramHolder, i,
                  inputInfo->flags&TA_IN_PRICE_OPEN?inputRandomData:NULL,
                  inputInfo->flags&TA_IN_PRICE_HIGH?inputRandomData:NULL,
                  inputInfo->flags&TA_IN_PRICE_LOW?inputRandomData:NULL,
                  inputInfo->flags&TA_IN_PRICE_CLOSE?inputRandomData:NULL,
                  inputInfo->flags&TA_IN_PRICE_VOLUME?inputRandomData:NULL, NULL );
               break;
            case TA_Input_Real:
               TA_SetInputParamRealPtr( paramHolder, i, inputRandomData );
               break;
            case TA_Input_Integer:
               TA_SetInputParamIntegerPtr( paramHolder, i, inputRandomData_int );
               break;
            }
         }

         /* Outputs: distinct buffers, except output ob is aliased onto output oa. */
         for( i = 0; i < funcInfo->nbOutput; i++ )
         {
            unsigned int slot = (i == ob) ? oa : i;
            TA_GetOutputParameterInfo( handle, i, &outInfo );
            if( outInfo->type == TA_Output_Integer )
               TA_SetOutputParamIntegerPtr( paramHolder, i, &output_int[slot][0] );
            else
               TA_SetOutputParamRealPtr( paramHolder, i, &output[slot][0] );
         }

         retCode = TA_CallFunc( paramHolder, 0, size - 1, &outBegIdx, &outNbElement );
         TA_ParamHolderFree( paramHolder );

         if( retCode != TA_BAD_PARAM )
         {
            printf( "  OUTPUT ALIAS [%s]: outputs %u and %u aliased to one buffer but "
                    "not rejected (rc=%d, expected TA_BAD_PARAM)\n",
                    funcInfo->name, oa, ob, retCode );
            return TA_ABS_TST_FAIL_OUTPUT_ALIAS;
         }
      }
   }
   return TA_TEST_PASS;
}

static void testOutputAlias( const TA_FuncInfo *funcInfo, void *opaqueData )
{
   ErrorNumber *errorNumber = (ErrorNumber *)opaqueData;
   ErrorNumber err;
   if( *errorNumber != TA_TEST_PASS )
      return;
   err = checkOutputAliasRejected( funcInfo );
   if( err != TA_TEST_PASS )
      *errorNumber = err;
}

/* TA_MAX_INDEX bounds the API domain: an index above it is rejected rather than
 * computed, identically in all four backends. The guard is generated into every
 * function's prologue, so this drives it over every function ta_abstract
 * reports. (Issue #180.)
 *
 * Every case here returns from the prologue BEFORE any input is dereferenced,
 * and that constraint is what shapes the boundary rows. An index near
 * TA_MAX_INDEX with buffers to match would be 800 MB per array, so a *successful*
 * call at the boundary is not affordable in this suite and the accepting side
 * has to be observed some other way:
 *
 *  - startIdx == TA_MAX_INDEX accepted: pair it with a smaller endIdx. Answering
 *    _END_INDEX proves the start check let it through; a `>=` off-by-one would
 *    answer _START_INDEX instead.
 *  - endIdx == TA_MAX_INDEX accepted: reach the NEXT check in the prologue.
 *    An out-of-range optional parameter answers TA_BAD_PARAM, which is only
 *    reachable once both range checks have passed.
 *
 * So an off-by-one fails in either direction: too strict shows up on the two
 * boundary-accept rows, too lax on the MAX+1 rows. */

typedef struct
{
   int          startIdx;
   int          endIdx;
   int          badParam;   /* 1 = also pass an out-of-range optional parameter */
   TA_RetCode   expected;
   const char  *what;
} TA_IndexRangeCase;

static const TA_IndexRangeCase TA_INDEX_RANGE_CASES[] =
{
   { -1,               10,                 0, TA_OUT_OF_RANGE_START_INDEX, "startIdx < 0" },
   { TA_MAX_INDEX+1,   TA_MAX_INDEX+1,     0, TA_OUT_OF_RANGE_START_INDEX, "startIdx > TA_MAX_INDEX" },
   { 0,                -1,                 0, TA_OUT_OF_RANGE_END_INDEX,   "endIdx < 0" },
   { 0,                TA_MAX_INDEX+1,     0, TA_OUT_OF_RANGE_END_INDEX,   "endIdx > TA_MAX_INDEX" },
   { 10,               9,                  0, TA_OUT_OF_RANGE_END_INDEX,   "endIdx < startIdx" },
   { TA_MAX_INDEX,     TA_MAX_INDEX-1,     0, TA_OUT_OF_RANGE_END_INDEX,   "startIdx == TA_MAX_INDEX accepted" },
   { TA_MAX_INDEX,     TA_MAX_INDEX,       1, TA_BAD_PARAM,                "endIdx == TA_MAX_INDEX accepted" }
};

static int indexRangeNbFuncs;        /* functions enumerated                 */
static int indexRangeNbChecked;      /* rows actually run                    */
static int indexRangeNbAccept;       /* boundary-accept rows actually run    */
static int indexRangeNbNoProbe;      /* functions with no usable probe       */

/* The boundary-accept row needs an optional-parameter value the prologue is
 * CERTAIN to reject, because the row's safety depends on it: the call is made
 * at startIdx == endIdx == TA_MAX_INDEX, so if validation let the value
 * through, the body would run and read ~1e8 elements past a 2000-element array.
 * "Certain" therefore means the range the generated prologue enforces is the
 * same range ta_abstract advertises -- true for IntegerRange and RealRange,
 * which the prologue emits as a literal `< min || > max` from the same YAML.
 *
 * Enum ("integer list") parameters are deliberately NOT used. Generated
 * prologues emit no range check for them -- TA_MA only remaps the default
 * sentinel and defers to a switch far below the index checks -- so an unlisted
 * value would not be rejected there, and the row would turn into an
 * out-of-bounds read. A function with only enum optional parameters skips this
 * row and is counted as skipped instead.
 *
 * Returns 1 and fills *whichParam / *intValue / *realValue / *isReal, or 0 when
 * the function has no usable probe. */
static int indexRangeBadOptValue( const TA_FuncInfo *funcInfo,
                                  unsigned int *whichParam,
                                  int *intValue, double *realValue, int *isReal )
{
   const TA_OptInputParameterInfo *optInfo;
   unsigned int i;

   for( i = 0; i < funcInfo->nbOptInput; i++ )
   {
      TA_GetOptInputParameterInfo( funcInfo->handle, i, &optInfo );
      if( optInfo->type == TA_OptInput_IntegerRange )
      {
         const TA_IntegerRange *r = (const TA_IntegerRange *)optInfo->dataSet;
         /* A parameter whose max is already TA_INTEGER_MAX has no value above
          * it to pass, so it cannot serve as the probe. */
         if( r && r->max < TA_INTEGER_MAX )
         {
            *whichParam = i; *isReal = 0; *intValue = r->max + 1;
            return 1;
         }
      }
      else if( optInfo->type == TA_OptInput_RealRange )
      {
         const TA_RealRange *r = (const TA_RealRange *)optInfo->dataSet;
         if( r && r->max < TA_REAL_MAX )
         {
            *whichParam = i; *isReal = 1; *realValue = r->max * 2.0 + 1.0;
            return 1;
         }
      }
   }
   return 0;
}

static ErrorNumber checkIndexRangeRejected( const TA_FuncInfo *funcInfo )
{
   TA_ParamHolder *paramHolder;
   const TA_FuncHandle *handle = funcInfo->handle;
   const TA_InputParameterInfo *inputInfo;
   const TA_OutputParameterInfo *outInfo;
   TA_RetCode retCode;
   unsigned int i, c, badParamIdx = 0;
   int outBegIdx, outNbElement;
   int badInt = 0, badIsReal = 0;
   double badReal = 0.0;
   int haveProbe = indexRangeBadOptValue( funcInfo, &badParamIdx,
                                          &badInt, &badReal, &badIsReal );

   indexRangeNbFuncs++;
   if( !haveProbe )
      indexRangeNbNoProbe++;

   for( c = 0; c < sizeof(TA_INDEX_RANGE_CASES)/sizeof(TA_INDEX_RANGE_CASES[0]); c++ )
   {
      const TA_IndexRangeCase *tc = &TA_INDEX_RANGE_CASES[c];

      /* Not applicable: no optional parameter this function is certain to
       * reject, so the row cannot be made safe. Counted above, and reported in
       * the coverage line, rather than dropped silently. */
      if( tc->badParam && !haveProbe )
         continue;

      retCode = TA_ParamHolderAlloc( handle, &paramHolder );
      if( retCode != TA_SUCCESS )
         return TA_ABS_TST_FAIL_PARAMHOLDERALLOC;

      for( i = 0; i < funcInfo->nbInput; i++ )
      {
         TA_GetInputParameterInfo( handle, i, &inputInfo );
         switch( inputInfo->type )
         {
         case TA_Input_Price:
            TA_SetInputParamPricePtr( paramHolder, i,
               inputInfo->flags&TA_IN_PRICE_OPEN?inputRandomData:NULL,
               inputInfo->flags&TA_IN_PRICE_HIGH?inputRandomData:NULL,
               inputInfo->flags&TA_IN_PRICE_LOW?inputRandomData:NULL,
               inputInfo->flags&TA_IN_PRICE_CLOSE?inputRandomData:NULL,
               inputInfo->flags&TA_IN_PRICE_VOLUME?inputRandomData:NULL, NULL );
            break;
         case TA_Input_Real:
            TA_SetInputParamRealPtr( paramHolder, i, inputRandomData );
            break;
         case TA_Input_Integer:
            TA_SetInputParamIntegerPtr( paramHolder, i, inputRandomData_int );
            break;
         }
      }

      for( i = 0; i < funcInfo->nbOutput; i++ )
      {
         TA_GetOutputParameterInfo( handle, i, &outInfo );
         if( outInfo->type == TA_Output_Integer )
            TA_SetOutputParamIntegerPtr( paramHolder, i, &output_int[i][0] );
         else
            TA_SetOutputParamRealPtr( paramHolder, i, &output[i][0] );
      }

      if( tc->badParam )
      {
         if( badIsReal )
            TA_SetOptInputParamReal( paramHolder, badParamIdx, badReal );
         else
            TA_SetOptInputParamInteger( paramHolder, badParamIdx, badInt );
      }

      retCode = TA_CallFunc( paramHolder, tc->startIdx, tc->endIdx,
                             &outBegIdx, &outNbElement );
      TA_ParamHolderFree( paramHolder );

      if( retCode != tc->expected )
      {
         printf( "  INDEX RANGE [%s]: %s (startIdx=%d endIdx=%d) returned %d, "
                 "expected %d\n",
                 funcInfo->name, tc->what, tc->startIdx, tc->endIdx,
                 retCode, tc->expected );
         return TA_ABS_TST_FAIL_INDEX_RANGE;
      }
      indexRangeNbChecked++;
      if( tc->badParam )
         indexRangeNbAccept++;
   }
   return TA_TEST_PASS;
}

static void testIndexRange( const TA_FuncInfo *funcInfo, void *opaqueData )
{
   ErrorNumber *errorNumber = (ErrorNumber *)opaqueData;
   ErrorNumber err;
   if( *errorNumber != TA_TEST_PASS )
      return;
   err = checkIndexRangeRejected( funcInfo );
   if( err != TA_TEST_PASS )
      *errorNumber = err;
}

/* Reusing an input buffer as one of the outputs (in-place transform) is a
 * documented guarantee: "the caller can reuse the input buffer to store one
 * of the outputs ... All TA functions support this" (website/src/api,
 * "Output Size"). This gate runs every function twice — separate buffers,
 * then with one real output aliased onto a private copy of one real input —
 * and requires bitwise-identical retCode/outBegIdx/outNBElement and ALL
 * outputs, for every (input component, real output) pair. (Issue #130.)
 *
 * Two data choices are load-bearing:
 * - startIdx = 0: the LINEARREG-family defect only corrupts output when the
 *   write cursor starts at the input's origin (clean at startIdx > lookback).
 * - Every series is distinct per (slot, component) and spans ~[3,30] with
 *   varying integer parts, so the slot-1 series used as MAVP's inPeriods
 *   takes the multi-pass internal path (a constant period hides the defect).
 * Integer outputs can never alias a real input (different element type);
 * they are still compared for collateral damage. */

#define IO_ALIAS_SIZE    252
#define IO_ALIAS_MAX_IN  4
#define IO_ALIAS_MAX_OUT 4

static double ioAliasData[IO_ALIAS_MAX_IN][6][IO_ALIAS_SIZE]; /* [slot][O,H,L,C,V,OI][bar] */
static double ioAliasScratch[IO_ALIAS_SIZE];
static double ioAliasRefOut[IO_ALIAS_MAX_OUT][IO_ALIAS_SIZE];
static double ioAliasOut[IO_ALIAS_MAX_OUT][IO_ALIAS_SIZE];
static int    ioAliasRefOutInt[IO_ALIAS_MAX_OUT][IO_ALIAS_SIZE];
static int    ioAliasOutInt[IO_ALIAS_MAX_OUT][IO_ALIAS_SIZE];
static int    ioAliasNbChecked;

static double ioAliasRand01( unsigned int *seed )
{
   *seed = (*seed * 1103515245u) + 12345u;
   return (double)((*seed >> 16) & 0x7fff) / 32768.0;
}

static void ioAliasInitData( void )
{
   unsigned int seed = 20260130u; /* deterministic; gate for issue #130 */
   int slot, j;
   double base, o, c;

   for( slot = 0; slot < IO_ALIAS_MAX_IN; slot++ )
   {
      base = 16.0;
      for( j = 0; j < IO_ALIAS_SIZE; j++ )
      {
         base += (ioAliasRand01(&seed) - 0.5) * 2.0;
         if( base < 4.0 )  base = 4.0 + ioAliasRand01(&seed);
         if( base > 28.0 ) base = 28.0 - ioAliasRand01(&seed);
         o = base + (ioAliasRand01(&seed) - 0.5);
         c = base + (ioAliasRand01(&seed) - 0.5);
         ioAliasData[slot][0][j] = o;
         ioAliasData[slot][1][j] = (o > c ? o : c) + ioAliasRand01(&seed);
         ioAliasData[slot][2][j] = (o < c ? o : c) - ioAliasRand01(&seed);
         ioAliasData[slot][3][j] = c;
         ioAliasData[slot][4][j] = 500.0 + 1000.0 * ioAliasRand01(&seed);
         ioAliasData[slot][5][j] = 500.0 + 1000.0 * ioAliasRand01(&seed);
      }
   }
}

/* Set every input from its canonical series, except slot aliasSlot's
 * component aliasComp which reads from ioAliasScratch. aliasSlot -1 = none.
 * Single-real inputs are component 0 of their slot. */
static TA_RetCode ioAliasSetInputs( TA_ParamHolder *paramHolder,
                                    const TA_FuncInfo *funcInfo,
                                    int aliasSlot, int aliasComp )
{
   const TA_InputParameterInfo *inputInfo;
   TA_RetCode retCode = TA_SUCCESS;
   const double *comp[6];
   unsigned int i;
   int c;

   for( i = 0; i < funcInfo->nbInput && retCode == TA_SUCCESS; i++ )
   {
      TA_GetInputParameterInfo( funcInfo->handle, i, &inputInfo );
      switch( inputInfo->type )
      {
      case TA_Input_Price:
         for( c = 0; c < 6; c++ )
            comp[c] = ((int)i == aliasSlot && c == aliasComp) ? ioAliasScratch
                                                              : ioAliasData[i][c];
         retCode = TA_SetInputParamPricePtr( paramHolder, i,
            inputInfo->flags & TA_IN_PRICE_OPEN         ? comp[0] : NULL,
            inputInfo->flags & TA_IN_PRICE_HIGH         ? comp[1] : NULL,
            inputInfo->flags & TA_IN_PRICE_LOW          ? comp[2] : NULL,
            inputInfo->flags & TA_IN_PRICE_CLOSE        ? comp[3] : NULL,
            inputInfo->flags & TA_IN_PRICE_VOLUME       ? comp[4] : NULL,
            inputInfo->flags & TA_IN_PRICE_OPENINTEREST ? comp[5] : NULL );
         break;
      case TA_Input_Real:
         retCode = TA_SetInputParamRealPtr( paramHolder, i,
            ((int)i == aliasSlot && aliasComp == 0) ? ioAliasScratch
                                                    : ioAliasData[i][0] );
         break;
      case TA_Input_Integer:
         retCode = TA_SetInputParamIntegerPtr( paramHolder, i, inputRandomData_int );
         break;
      }
   }
   return retCode;
}

static ErrorNumber checkInPlaceAliasCorrect( const TA_FuncInfo *funcInfo )
{
   static const char *compName[6] = { "open", "high", "low", "close", "volume", "oi" };
   static const TA_InputFlags compFlag[6] =
      { TA_IN_PRICE_OPEN, TA_IN_PRICE_HIGH, TA_IN_PRICE_LOW,
        TA_IN_PRICE_CLOSE, TA_IN_PRICE_VOLUME, TA_IN_PRICE_OPENINTEREST };

   TA_ParamHolder *paramHolder;
   const TA_FuncHandle *handle = funcInfo->handle;
   const TA_InputParameterInfo *inputInfo;
   const TA_OutputParameterInfo *outInfo;
   TA_RetCode retCode;
   ErrorNumber errNumber = TA_TEST_PASS;
   unsigned int i, o, k;
   int c, j, refBegIdx, refNbElement, outBegIdx, outNbElement;

   if( funcInfo->nbInput > IO_ALIAS_MAX_IN || funcInfo->nbOutput > IO_ALIAS_MAX_OUT )
   {
      printf( "  IN-PLACE ALIAS [%s]: gate capacity exceeded (%u inputs, %u outputs)\n",
              funcInfo->name, funcInfo->nbInput, funcInfo->nbOutput );
      return TA_ABS_TST_FAIL_INPLACE_ALIAS;
   }

   /* Reference run: every buffer distinct. */
   retCode = TA_ParamHolderAlloc( handle, &paramHolder );
   if( retCode != TA_SUCCESS )
      return TA_ABS_TST_FAIL_PARAMHOLDERALLOC;
   if( ioAliasSetInputs( paramHolder, funcInfo, -1, -1 ) != TA_SUCCESS )
   {
      TA_ParamHolderFree( paramHolder );
      return TA_ABS_TST_FAIL_PARAMREALPTR;
   }
   for( k = 0; k < funcInfo->nbOutput; k++ )
   {
      TA_GetOutputParameterInfo( handle, k, &outInfo );
      if( outInfo->type == TA_Output_Integer )
         TA_SetOutputParamIntegerPtr( paramHolder, k, &ioAliasRefOutInt[k][0] );
      else
         TA_SetOutputParamRealPtr( paramHolder, k, &ioAliasRefOut[k][0] );
   }
   retCode = TA_CallFunc( paramHolder, 0, IO_ALIAS_SIZE-1, &refBegIdx, &refNbElement );
   TA_ParamHolderFree( paramHolder );
   if( retCode != TA_SUCCESS )
   {
      printf( "  IN-PLACE ALIAS [%s]: reference call failed (rc=%d)\n",
              funcInfo->name, retCode );
      return TA_ABS_TST_FAIL_INPLACE_ALIAS;
   }

   /* One aliased run per (real output o) x (real input component i,c).
    * Report every failing pair, not just the first. */
   for( o = 0; o < funcInfo->nbOutput; o++ )
   {
      TA_GetOutputParameterInfo( handle, o, &outInfo );
      if( outInfo->type == TA_Output_Integer )
         continue;

      for( i = 0; i < funcInfo->nbInput; i++ )
      {
         TA_GetInputParameterInfo( handle, i, &inputInfo );
         for( c = 0; c < 6; c++ )
         {
            const double *series;
            if( inputInfo->type == TA_Input_Real )
            {
               if( c != 0 )
                  continue;
               series = ioAliasData[i][0];
            }
            else if( inputInfo->type == TA_Input_Price )
            {
               if( !(inputInfo->flags & compFlag[c]) )
                  continue;
               series = ioAliasData[i][c];
            }
            else
               continue; /* integer input cannot alias a real output */

            memcpy( ioAliasScratch, series, sizeof(ioAliasScratch) );

            /* Poison the non-aliased buffers: a function that stops writing
             * an output must not pass on values left over from the previous
             * aliased run. */
            for( k = 0; k < funcInfo->nbOutput; k++ )
            {
               for( j = 0; j < IO_ALIAS_SIZE; j++ )
               {
                  ioAliasOut[k][j] = TA_REAL_MIN;
                  ioAliasOutInt[k][j] = TA_INTEGER_MIN;
               }
            }
            outBegIdx = -1;
            outNbElement = -1;

            retCode = TA_ParamHolderAlloc( handle, &paramHolder );
            if( retCode != TA_SUCCESS )
               return TA_ABS_TST_FAIL_PARAMHOLDERALLOC;
            if( ioAliasSetInputs( paramHolder, funcInfo, (int)i, c ) != TA_SUCCESS )
            {
               TA_ParamHolderFree( paramHolder );
               return TA_ABS_TST_FAIL_PARAMREALPTR;
            }
            for( k = 0; k < funcInfo->nbOutput; k++ )
            {
               TA_GetOutputParameterInfo( handle, k, &outInfo );
               if( outInfo->type == TA_Output_Integer )
                  TA_SetOutputParamIntegerPtr( paramHolder, k, &ioAliasOutInt[k][0] );
               else if( k == o )
                  TA_SetOutputParamRealPtr( paramHolder, k, ioAliasScratch );
               else
                  TA_SetOutputParamRealPtr( paramHolder, k, &ioAliasOut[k][0] );
            }
            retCode = TA_CallFunc( paramHolder, 0, IO_ALIAS_SIZE-1,
                                   &outBegIdx, &outNbElement );
            TA_ParamHolderFree( paramHolder );

            if( (retCode != TA_SUCCESS) ||
                (outBegIdx != refBegIdx) || (outNbElement != refNbElement) )
            {
               printf( "  IN-PLACE ALIAS [%s]: out%u <- in%u.%s: rc=%d begIdx=%d nb=%d"
                       " (expected rc=0 begIdx=%d nb=%d)\n",
                       funcInfo->name, o, i,
                       inputInfo->type == TA_Input_Real ? "real" : compName[c],
                       retCode, outBegIdx, outNbElement, refBegIdx, refNbElement );
               errNumber = TA_ABS_TST_FAIL_INPLACE_ALIAS;
               continue;
            }

            for( k = 0; k < funcInfo->nbOutput; k++ )
            {
               TA_GetOutputParameterInfo( handle, k, &outInfo );
               if( outInfo->type == TA_Output_Integer )
               {
                  if( memcmp( ioAliasOutInt[k], ioAliasRefOutInt[k],
                              (size_t)refNbElement * sizeof(int) ) != 0 )
                  {
                     printf( "  IN-PLACE ALIAS [%s]: out%u <- in%u.%s: int output %u differs\n",
                             funcInfo->name, o, i,
                             inputInfo->type == TA_Input_Real ? "real" : compName[c], k );
                     errNumber = TA_ABS_TST_FAIL_INPLACE_ALIAS;
                  }
               }
               else
               {
                  const double *aliased = (k == o) ? ioAliasScratch : ioAliasOut[k];
                  int nbDiff = 0, firstDiff = -1;
                  for( j = 0; j < refNbElement; j++ )
                  {
                     if( memcmp( &aliased[j], &ioAliasRefOut[k][j], sizeof(double) ) != 0 )
                     {
                        if( firstDiff < 0 )
                           firstDiff = j;
                        nbDiff++;
                     }
                  }
                  if( nbDiff != 0 )
                  {
                     printf( "  IN-PLACE ALIAS [%s]: out%u <- in%u.%s: output %u wrong in"
                             " %d/%d values (first at [%d]: %.17g, expected %.17g)\n",
                             funcInfo->name, o, i,
                             inputInfo->type == TA_Input_Real ? "real" : compName[c],
                             k, nbDiff, refNbElement, firstDiff,
                             aliased[firstDiff], ioAliasRefOut[k][firstDiff] );
                     errNumber = TA_ABS_TST_FAIL_INPLACE_ALIAS;
                  }
               }
            }
            if( refNbElement > 0 )
               ioAliasNbChecked++;
         }
      }
   }
   return errNumber;
}

static void testInPlaceAlias( const TA_FuncInfo *funcInfo, void *opaqueData )
{
   ErrorNumber *errorNumber = (ErrorNumber *)opaqueData;
   ErrorNumber err = checkInPlaceAliasCorrect( funcInfo );
   /* Keep enumerating on failure so one run reports every offender. */
   if( err != TA_TEST_PASS && *errorNumber == TA_TEST_PASS )
      *errorNumber = err;
}

/* ---- The ParamHolder ERROR contract (issue #164) ---------------------------
 *
 * Everything else in this file drives the holder down its success path. The
 * four RetCodes that exist only in the dynamic tier -- TA_INVALID_PARAM_HOLDER_TYPE,
 * TA_INPUT_NOT_ALL_INITIALIZE, TA_OUTPUT_NOT_ALL_INITIALIZE and the
 * paramIndex/NULL TA_BAD_PARAM -- had their VALUES pinned by test_internals.c
 * and were otherwise never produced by anything, so no test could tell a
 * correct rejection from a silent accept. That matters more than it looks:
 * the managed backends signal these same conditions by throwing, and their
 * tests assert the throw, which left C the only backend whose refusals were
 * unasserted.
 *
 * Driven over every function ta_abstract reports, exercising whichever slots
 * each function's descriptors admit, so new functions are covered without a
 * roster. Counted per class and floored below -- a sweep that stopped
 * constructing cases would otherwise read exactly like a passing one. */
static long long g_holderTypeErr = 0;   /* TA_INVALID_PARAM_HOLDER_TYPE  */
static long long g_holderIndexErr = 0;  /* paramIndex out of range       */
static long long g_holderNullErr = 0;   /* NULL value pointer            */
static long long g_holderPriceNullErr = 0; /* NULL consumed price component */
static long long g_holderInputErr = 0;  /* TA_INPUT_NOT_ALL_INITIALIZE   */
static long long g_holderOutputErr = 0; /* TA_OUTPUT_NOT_ALL_INITIALIZE  */

/* Report a wrong RetCode from a call that had to be refused. */
static int holder_expect( const char *funcName, const char *what,
                          TA_RetCode got, TA_RetCode want )
{
   if( got == want )
      return 1;
   printf( "  HOLDER CONTRACT [%s]: %s returned %d, expected %d\n",
           funcName, what, (int)got, (int)want );
   return 0;
}

/* A rejected setter must leave the holder computing exactly what it computed
 * before -- the other half of the rule, the call tier's half having landed with
 * #265.
 *
 * The sharp case is a RE-bind. On a fresh holder a partial write is masked: the
 * slot is still marked unbound and TA_CallFunc answers TA_INPUT_NOT_ALL_INITIALIZE
 * before dispatching. But the bitmaps only ever clear -- there is no unbind -- so
 * over a bundle that already works, a setter that checked and wrote one component
 * at a time committed the ones ahead of the offending one and left the rest
 * holding the previous bundle, and TA_CallFunc then succeeded over a mixture of
 * the two.
 *
 * WILLR, because it consumes High|Low|Close: close is the last required
 * component and so the natural place to trip the setter. The rebind uses a
 * different PHASE rather than an offset -- WILLR is (hh - c) / (hh - ll), which a
 * uniform shift leaves unchanged, so the control below would pass on a setter
 * that did nothing at all.
 */
static ErrorNumber testHolderStaysReusable( void )
{
   const TA_FuncHandle *handle;
   TA_ParamHolder *holder;
   TA_RetCode retCode;
   static double h1[252], l1[252], c1[252];
   static double h2[252], l2[252], c2[252];
   static double want[252], got[252], moved[252];
   int wantBeg, wantNb, gotBeg, gotNb, movedBeg, movedNb;
   int i, differs;

   for( i = 0; i < 252; i++ )
   {
      h1[i] = 100.0 + 8.0 * sin( i / 9.0 );
      l1[i] = h1[i] - 4.0;
      c1[i] = h1[i] - 2.0;
      h2[i] = 100.0 + 8.0 * sin( (i / 9.0) + 1.3 );
      l2[i] = h2[i] - 4.0;
      c2[i] = h2[i] - 2.0;
   }

   retCode = TA_GetFuncHandle( "WILLR", &handle );
   if( retCode != TA_SUCCESS ) return TA_ABS_TST_FAIL_GETFUNCHANDLE;

   #define REUSE_BIND(dest) \
   { \
      if( TA_SetInputParamPricePtr( holder, 0, NULL, h1, l1, c1, NULL, NULL ) != TA_SUCCESS || \
          TA_SetOptInputParamInteger( holder, 0, 14 ) != TA_SUCCESS || \
          TA_SetOutputParamRealPtr( holder, 0, dest ) != TA_SUCCESS ) \
      { \
         TA_ParamHolderFree( holder ); \
         return TA_ABS_TST_FAIL_HOLDER_NOT_REUSABLE; \
      } \
   }

   /* What a correctly bound holder produces. */
   if( TA_ParamHolderAlloc( handle, &holder ) != TA_SUCCESS )
      return TA_ABS_TST_FAIL_PARAMHOLDERALLOC;
   REUSE_BIND( want )
   retCode = TA_CallFunc( holder, 0, 251, &wantBeg, &wantNb );
   TA_ParamHolderFree( holder );
   if( retCode != TA_SUCCESS || wantNb <= 0 )
   {
      printf( "  HOLDER REUSE: the reference call produced nothing (%d, %d)\n",
              (int)retCode, wantNb );
      return TA_ABS_TST_FAIL_HOLDER_NOT_REUSABLE;
   }

   /* The same bind, a REJECTED rebind that forgets close, then the same call. */
   if( TA_ParamHolderAlloc( handle, &holder ) != TA_SUCCESS )
      return TA_ABS_TST_FAIL_PARAMHOLDERALLOC;
   REUSE_BIND( got )
   if( TA_CallFunc( holder, 0, 251, &gotBeg, &gotNb ) != TA_SUCCESS )
   {
      TA_ParamHolderFree( holder );
      return TA_ABS_TST_FAIL_HOLDER_NOT_REUSABLE;
   }
   retCode = TA_SetInputParamPricePtr( holder, 0, NULL, h2, l2, NULL, NULL, NULL );
   if( retCode != TA_BAD_PARAM )
   {
      printf( "  HOLDER REUSE: a bundle missing a consumed component returned %d, "
              "expected TA_BAD_PARAM\n", (int)retCode );
      TA_ParamHolderFree( holder );
      return TA_ABS_TST_FAIL_HOLDER_NOT_REUSABLE;
   }
   retCode = TA_CallFunc( holder, 0, 251, &gotBeg, &gotNb );
   TA_ParamHolderFree( holder );
   if( retCode != TA_SUCCESS || gotBeg != wantBeg || gotNb != wantNb )
   {
      printf( "  HOLDER REUSE: after a rejected setter the call answered %d, "
              "(%d,%d) vs (%d,%d)\n", (int)retCode, gotBeg, gotNb, wantBeg, wantNb );
      return TA_ABS_TST_FAIL_HOLDER_NOT_REUSABLE;
   }
   for( i = 0; i < wantNb; i++ )
   {
      if( memcmp( &got[i], &want[i], sizeof(double) ) != 0 )
      {
         printf( "  HOLDER REUSE: a rejected setter changed what the holder "
                 "computes, at output[%d]\n", i );
         return TA_ABS_TST_FAIL_HOLDER_NOT_REUSABLE;
      }
   }

   /* Control: a CORRECT rebind must reach the output, or the loop above passes
    * for a setter that stopped working altogether. */
   if( TA_ParamHolderAlloc( handle, &holder ) != TA_SUCCESS )
      return TA_ABS_TST_FAIL_PARAMHOLDERALLOC;
   REUSE_BIND( moved )
   if( TA_CallFunc( holder, 0, 251, &movedBeg, &movedNb ) != TA_SUCCESS ||
       TA_SetInputParamPricePtr( holder, 0, NULL, h2, l2, c2, NULL, NULL ) != TA_SUCCESS ||
       TA_CallFunc( holder, 0, 251, &movedBeg, &movedNb ) != TA_SUCCESS )
   {
      TA_ParamHolderFree( holder );
      return TA_ABS_TST_FAIL_HOLDER_NOT_REUSABLE;
   }
   TA_ParamHolderFree( holder );
   #undef REUSE_BIND

   if( movedBeg != wantBeg || movedNb != wantNb )
   {
      printf( "  HOLDER REUSE: a correct rebind moved the reported range "
              "(%d,%d) vs (%d,%d)\n", movedBeg, movedNb, wantBeg, wantNb );
      return TA_ABS_TST_FAIL_HOLDER_NOT_REUSABLE;
   }

   differs = 0;
   for( i = 0; i < wantNb; i++ )
      if( memcmp( &moved[i], &want[i], sizeof(double) ) != 0 )
         differs = 1;
   if( !differs )
   {
      printf( "  HOLDER REUSE: a correct rebind did not reach the output -- the "
              "comparison above proves nothing\n" );
      return TA_ABS_TST_FAIL_HOLDER_NOT_REUSABLE;
   }

   printf( "ParamHolder reuse: a rejected setter leaves the holder computing "
           "%d unchanged value(s); a correct rebind moves them\n", wantNb );
   return TA_TEST_PASS;
}

static ErrorNumber checkHolderErrorContract( const TA_FuncInfo *funcInfo )
{
   const TA_FuncHandle *handle = funcInfo->handle;
   const TA_InputParameterInfo *inputInfo;
   const TA_OptInputParameterInfo *optInfo;
   const TA_OutputParameterInfo *outInfo;
   TA_ParamHolder *paramHolder;
   TA_RetCode retCode;
   unsigned int i;
   int ok = 1;
   int outBegIdx, outNbElement;
   /* One buffer PER OUTPUT SLOT, not one shared: binding every output to the
    * same array is the aliasing #108 rejects, and it only stays invisible here
    * because TA_CallFunc answers NULL/unbound before it dispatches. Relying on
    * that ordering would make this test's meaning depend on an unrelated one. */
   #define HOLDER_MAX_OUT 8
   static double dummyReal[HOLDER_MAX_OUT][252];
   static int    dummyInt[HOLDER_MAX_OUT][252];

   if( funcInfo->nbOutput > HOLDER_MAX_OUT )
   {
      printf( "  HOLDER CONTRACT [%s]: %u outputs exceeds HOLDER_MAX_OUT (%d)\n",
              funcInfo->name, funcInfo->nbOutput, HOLDER_MAX_OUT );
      return TA_ABS_TST_FAIL_HOLDER_CONTRACT;
   }

   retCode = TA_ParamHolderAlloc( handle, &paramHolder );
   if( retCode != TA_SUCCESS )
      return TA_ABS_TST_FAIL_PARAMHOLDERALLOC;

   /* 1. paramIndex out of range on all six setters. */
   ok &= holder_expect( funcInfo->name, "SetInputParamRealPtr past nbInput",
            TA_SetInputParamRealPtr( paramHolder, funcInfo->nbInput, dummyReal[0] ), TA_BAD_PARAM );
   ok &= holder_expect( funcInfo->name, "SetInputParamIntegerPtr past nbInput",
            TA_SetInputParamIntegerPtr( paramHolder, funcInfo->nbInput, dummyInt[0] ), TA_BAD_PARAM );
   ok &= holder_expect( funcInfo->name, "SetInputParamPricePtr past nbInput",
            TA_SetInputParamPricePtr( paramHolder, funcInfo->nbInput, dummyReal[0], dummyReal[0],
                                      dummyReal[0], dummyReal[0], dummyReal[0], dummyReal[0] ), TA_BAD_PARAM );
   ok &= holder_expect( funcInfo->name, "SetOptInputParamInteger past nbOptInput",
            TA_SetOptInputParamInteger( paramHolder, funcInfo->nbOptInput, 1 ), TA_BAD_PARAM );
   ok &= holder_expect( funcInfo->name, "SetOptInputParamReal past nbOptInput",
            TA_SetOptInputParamReal( paramHolder, funcInfo->nbOptInput, 1.0 ), TA_BAD_PARAM );
   ok &= holder_expect( funcInfo->name, "SetOutputParamRealPtr past nbOutput",
            TA_SetOutputParamRealPtr( paramHolder, funcInfo->nbOutput, dummyReal[0] ), TA_BAD_PARAM );
   ok &= holder_expect( funcInfo->name, "SetOutputParamIntegerPtr past nbOutput",
            TA_SetOutputParamIntegerPtr( paramHolder, funcInfo->nbOutput, dummyInt[0] ), TA_BAD_PARAM );
   g_holderIndexErr += 7;

   /* 2. NULL value pointer on a slot that DOES exist -- otherwise the
    *    paramIndex check above would answer first and this would prove nothing. */
   ok &= holder_expect( funcInfo->name, "SetInputParamRealPtr(NULL)",
            TA_SetInputParamRealPtr( paramHolder, 0, NULL ), TA_BAD_PARAM );
   ok &= holder_expect( funcInfo->name, "SetInputParamIntegerPtr(NULL)",
            TA_SetInputParamIntegerPtr( paramHolder, 0, NULL ), TA_BAD_PARAM );
   ok &= holder_expect( funcInfo->name, "SetOutputParamRealPtr(NULL)",
            TA_SetOutputParamRealPtr( paramHolder, 0, NULL ), TA_BAD_PARAM );
   ok &= holder_expect( funcInfo->name, "SetOutputParamIntegerPtr(NULL)",
            TA_SetOutputParamIntegerPtr( paramHolder, 0, NULL ), TA_BAD_PARAM );
   g_holderNullErr += 4;

   /* 2b. The PRICE setter's own NULL class, which nothing reached before #266:
    *     every call site here either passed all six or passed NULL only where
    *     the flags say the component is unconsumed. Offer NULL for a component
    *     the function DOES consume, one at a time, and require TA_BAD_PARAM. */
   for( i = 0; i < funcInfo->nbInput; i++ )
   {
      unsigned int c;
      TA_GetInputParameterInfo( handle, i, &inputInfo );
      if( !inputInfo || inputInfo->type != TA_Input_Price )
         continue;
      for( c = 0; c < 6; c++ )
      {
         static const TA_InputFlags priceBit[6] =
            { TA_IN_PRICE_OPEN, TA_IN_PRICE_HIGH, TA_IN_PRICE_LOW,
              TA_IN_PRICE_CLOSE, TA_IN_PRICE_VOLUME, TA_IN_PRICE_OPENINTEREST };
         const TA_Real *arg[6];
         unsigned int k;
         if( !(inputInfo->flags & priceBit[c]) )
            continue;
         for( k = 0; k < 6; k++ )
            arg[k] = (k == c) ? NULL : &dummyReal[0][0];
         ok &= holder_expect( funcInfo->name, "SetInputParamPricePtr(NULL component)",
                  TA_SetInputParamPricePtr( paramHolder, i, arg[0], arg[1], arg[2],
                                            arg[3], arg[4], arg[5] ), TA_BAD_PARAM );
         /* Its OWN counter, not g_holderNullErr: classes 2 and 6 drive that one to
          * 6 per function unconditionally, so folding 2b into it would let this
          * whole loop go dead behind a floor that is already satisfied. */
         g_holderPriceNullErr++;
      }
   }

   /* 3. Type mismatch: offer each slot the setter its declared type forbids. */
   for( i = 0; i < funcInfo->nbInput; i++ )
   {
      TA_GetInputParameterInfo( handle, i, &inputInfo );
      if( inputInfo->type == TA_Input_Real )
      {
         ok &= holder_expect( funcInfo->name, "real input via SetInputParamIntegerPtr",
                  TA_SetInputParamIntegerPtr( paramHolder, i, dummyInt[0] ),
                  TA_INVALID_PARAM_HOLDER_TYPE );
         ok &= holder_expect( funcInfo->name, "real input via SetInputParamPricePtr",
                  TA_SetInputParamPricePtr( paramHolder, i, dummyReal[0], dummyReal[0], dummyReal[0],
                                            dummyReal[0], dummyReal[0], dummyReal[0] ),
                  TA_INVALID_PARAM_HOLDER_TYPE );
         g_holderTypeErr += 2;
      }
      else if( inputInfo->type == TA_Input_Price )
      {
         ok &= holder_expect( funcInfo->name, "price input via SetInputParamRealPtr",
                  TA_SetInputParamRealPtr( paramHolder, i, dummyReal[0] ),
                  TA_INVALID_PARAM_HOLDER_TYPE );
         ok &= holder_expect( funcInfo->name, "price input via SetInputParamIntegerPtr",
                  TA_SetInputParamIntegerPtr( paramHolder, i, dummyInt[0] ),
                  TA_INVALID_PARAM_HOLDER_TYPE );
         g_holderTypeErr += 2;
      }
   }

   for( i = 0; i < funcInfo->nbOptInput; i++ )
   {
      TA_GetOptInputParameterInfo( handle, i, &optInfo );
      if( optInfo->type == TA_OptInput_IntegerRange || optInfo->type == TA_OptInput_IntegerList )
      {
         ok &= holder_expect( funcInfo->name, "integer opt via SetOptInputParamReal",
                  TA_SetOptInputParamReal( paramHolder, i, 1.0 ),
                  TA_INVALID_PARAM_HOLDER_TYPE );
         g_holderTypeErr++;
      }
      else
      {
         ok &= holder_expect( funcInfo->name, "real opt via SetOptInputParamInteger",
                  TA_SetOptInputParamInteger( paramHolder, i, 1 ),
                  TA_INVALID_PARAM_HOLDER_TYPE );
         g_holderTypeErr++;
      }
   }

   for( i = 0; i < funcInfo->nbOutput; i++ )
   {
      TA_GetOutputParameterInfo( handle, i, &outInfo );
      if( outInfo->type == TA_Output_Real )
      {
         ok &= holder_expect( funcInfo->name, "real output via SetOutputParamIntegerPtr",
                  TA_SetOutputParamIntegerPtr( paramHolder, i, dummyInt[i] ),
                  TA_INVALID_PARAM_HOLDER_TYPE );
      }
      else
      {
         ok &= holder_expect( funcInfo->name, "integer output via SetOutputParamRealPtr",
                  TA_SetOutputParamRealPtr( paramHolder, i, dummyReal[i] ),
                  TA_INVALID_PARAM_HOLDER_TYPE );
      }
      g_holderTypeErr++;
   }

   /* 4. Outputs bound, inputs not: TA_CallFunc must refuse. The input test runs
    *    FIRST in TA_CallFunc, so this order is the only one that can observe it. */
   for( i = 0; i < funcInfo->nbOutput; i++ )
   {
      TA_GetOutputParameterInfo( handle, i, &outInfo );
      if( outInfo->type == TA_Output_Real )
         TA_SetOutputParamRealPtr( paramHolder, i, dummyReal[i] );
      else
         TA_SetOutputParamIntegerPtr( paramHolder, i, dummyInt[i] );
   }
   ok &= holder_expect( funcInfo->name, "CallFunc with no input bound",
            TA_CallFunc( paramHolder, 0, 251, &outBegIdx, &outNbElement ),
            TA_INPUT_NOT_ALL_INITIALIZE );
   g_holderInputErr++;

   TA_ParamHolderFree( paramHolder );

   /* 5. Inputs bound, outputs not. A fresh holder, because the bitmaps only
    *    ever clear -- there is no unbind. */
   retCode = TA_ParamHolderAlloc( handle, &paramHolder );
   if( retCode != TA_SUCCESS )
      return TA_ABS_TST_FAIL_PARAMHOLDERALLOC;

   for( i = 0; i < funcInfo->nbInput; i++ )
   {
      TA_GetInputParameterInfo( handle, i, &inputInfo );
      if( inputInfo->type == TA_Input_Real )
         TA_SetInputParamRealPtr( paramHolder, i, dummyReal[0] );
      else if( inputInfo->type == TA_Input_Integer )
         TA_SetInputParamIntegerPtr( paramHolder, i, dummyInt[0] );
      else
         TA_SetInputParamPricePtr( paramHolder, i, dummyReal[0], dummyReal[0], dummyReal[0],
                                   dummyReal[0], dummyReal[0], dummyReal[0] );
   }
   ok &= holder_expect( funcInfo->name, "CallFunc with no output bound",
            TA_CallFunc( paramHolder, 0, 251, &outBegIdx, &outNbElement ),
            TA_OUTPUT_NOT_ALL_INITIALIZE );
   g_holderOutputErr++;

   /* 6. Fully bound, but NULL out-params: still TA_BAD_PARAM. */
   for( i = 0; i < funcInfo->nbOutput; i++ )
   {
      TA_GetOutputParameterInfo( handle, i, &outInfo );
      if( outInfo->type == TA_Output_Real )
         TA_SetOutputParamRealPtr( paramHolder, i, dummyReal[i] );
      else
         TA_SetOutputParamIntegerPtr( paramHolder, i, dummyInt[i] );
   }
   ok &= holder_expect( funcInfo->name, "CallFunc(outBegIdx=NULL)",
            TA_CallFunc( paramHolder, 0, 251, NULL, &outNbElement ), TA_BAD_PARAM );
   ok &= holder_expect( funcInfo->name, "CallFunc(outNbElement=NULL)",
            TA_CallFunc( paramHolder, 0, 251, &outBegIdx, NULL ), TA_BAD_PARAM );
   g_holderNullErr += 2;

   TA_ParamHolderFree( paramHolder );

   return ok ? TA_TEST_PASS : TA_ABS_TST_FAIL_HOLDER_CONTRACT;
}

static void testHolderErrorContract( const TA_FuncInfo *funcInfo, void *opaqueData )
{
   ErrorNumber *errorNumber = (ErrorNumber *)opaqueData;
   ErrorNumber err = checkHolderErrorContract( funcInfo );
   /* Keep enumerating on failure so one run reports every offender. */
   if( err != TA_TEST_PASS && *errorNumber == TA_TEST_PASS )
      *errorNumber = err;
}

static ErrorNumber test_default_calls(void)
{
   ErrorNumber errNumber;
   unsigned int i;
   unsigned int sign;
   double tempDouble;

   errNumber = TA_TEST_PASS;

   for( i=0; i < sizeof(inputNegData)/sizeof(double); i++ )
   {
      inputNegData[i] = -((double)((int)i));
	  inputNegData_int[i] = -(int)i;
   }

   for( i=0; i < sizeof(inputZeroData)/sizeof(double); i++ )
   {
      inputZeroData[i] = 0.0;
	  inputZeroData_int[i] = (int)inputZeroData[i];
   }

   for( i=0; i < sizeof(inputRandomData)/sizeof(double); i++ )
   {
      /* Make 100% sure input range is ]0..1[ */
	  tempDouble = (double)rand() / ((double)(RAND_MAX)+(double)(1));
      while( (tempDouble <= 0.0) || (tempDouble >= 1.0) )
	  {
		  tempDouble = (double)rand() / ((double)(RAND_MAX)+(double)(1));
	  }
      inputRandomData[i] = tempDouble;
      inputRandomData_int[i] = (int)inputRandomData[i];
   }

   /* Two DISTINCT epsilon datasets. Both loops used to write the Flt array, so
    * the second silently destroyed the first: FLT_EPSILON never reached a test,
    * and inputRandDblEpsilon — declared, and passed to CALL() below — was never
    * written at all, leaving it zero-filled and bit-identical to inputZeroData.
    * The advertised five-dataset sweep was four, one of them a duplicate. */
   for( i=0; i < sizeof(inputRandFltEpsilon)/sizeof(double); i++ )
   {
       sign= (unsigned int)rand()%2;
       inputRandFltEpsilon[i] = (sign?1.0:-1.0)*(FLT_EPSILON);
       inputRandFltEpsilon_int[i] = sign?TA_INTEGER_MIN:TA_INTEGER_MAX;
   }

   for( i=0; i < sizeof(inputRandDblEpsilon)/sizeof(double); i++ )
   {
       sign= (unsigned int)rand()%2;
       inputRandDblEpsilon[i] = (sign?1.0:-1.0)*(DBL_EPSILON);
       inputRandDblEpsilon_int[i] = sign?1:-1;
   }

   if( doExtensiveProfiling )
   {
		   printf( "\n[PROFILING START]\n" );
   }

   TA_ForEachFunc( testDefault, &errNumber );

   if( doExtensiveProfiling )
   {
		   printf( "[PROFILING END]\n" );
   }

   /* Every multi-output function must reject output-buffer aliasing (issue #108). */
   if( errNumber == TA_TEST_PASS )
      TA_ForEachFunc( testOutputAlias, &errNumber );

   /* Every function must bound startIdx/endIdx by TA_MAX_INDEX (issue #180). */
   if( errNumber == TA_TEST_PASS )
   {
      indexRangeNbFuncs = indexRangeNbChecked = 0;
      indexRangeNbAccept = indexRangeNbNoProbe = 0;
      TA_ForEachFunc( testIndexRange, &errNumber );
      if( errNumber == TA_TEST_PASS )
         printf( "  Index range (#180): %d case(s) over %d function(s); the "
                 "startIdx boundary is checked for all, the endIdx boundary for "
                 "%d (the other %d expose no optional parameter the prologue is "
                 "certain to reject)\n",
                 indexRangeNbChecked, indexRangeNbFuncs, indexRangeNbAccept,
                 indexRangeNbNoProbe );

      /* Exact accounting rather than a round floor. Every function runs the six
       * always-applicable rows, and the boundary-accept row either ran or was
       * counted as unprobeable — so these two identities pin the whole table.
       * A single "at least N cases" floor would not: the six rejection rows
       * alone satisfy it, and the boundary-accept half could vanish unnoticed.
       * That half is precisely what catches a `>=` off-by-one. */
      if( errNumber == TA_TEST_PASS && indexRangeNbFuncs < 150 )
      {
         printf( "Failed: index-range gate saw only %d function(s)\n",
                 indexRangeNbFuncs );
         errNumber = TA_ABS_TST_FAIL_INDEX_RANGE;
      }
      if( errNumber == TA_TEST_PASS &&
          ( indexRangeNbChecked != 6 * indexRangeNbFuncs + indexRangeNbAccept ||
            indexRangeNbAccept  != indexRangeNbFuncs - indexRangeNbNoProbe ) )
      {
         printf( "Failed: index-range accounting (%d cases, %d funcs, %d accept, "
                 "%d unprobeable)\n", indexRangeNbChecked, indexRangeNbFuncs,
                 indexRangeNbAccept, indexRangeNbNoProbe );
         errNumber = TA_ABS_TST_FAIL_INDEX_RANGE;
      }
   }

   /* In-place (input==output) aliasing must be bitwise-correct (issue #130). */
   if( errNumber == TA_TEST_PASS )
   {
      ioAliasInitData();
      ioAliasNbChecked = 0;
      TA_ForEachFunc( testInPlaceAlias, &errNumber );
      /* Vacuity floor: ~100 functions have a real output; a collapse of the
       * pair enumeration must fail loudly, not pass silently. */
      if( errNumber == TA_TEST_PASS && ioAliasNbChecked < 100 )
      {
         printf( "Failed: in-place alias gate vacuous (%d pairs checked)\n",
                 ioAliasNbChecked );
         errNumber = TA_ABS_TST_FAIL_INPLACE_ALIAS_VACUOUS;
      }
      if( errNumber == TA_TEST_PASS )
         printf( "In-place alias gate: %d (input,output) pairs bitwise-verified\n",
                 ioAliasNbChecked );
   }

   /* The ParamHolder error contract -- the refusals, not the successes (#164). */
   if( errNumber == TA_TEST_PASS )
   {
      g_holderTypeErr = g_holderIndexErr = g_holderNullErr = 0;
      g_holderInputErr = g_holderOutputErr = g_holderPriceNullErr = 0;
      TA_ForEachFunc( testHolderErrorContract, &errNumber );

      /* Each class must have been reached. Every function contributes to every
       * one of these, so a zero means the sweep stopped building cases, not
       * that the corpus lacks them. */
      if( errNumber == TA_TEST_PASS &&
          ( g_holderTypeErr == 0 || g_holderIndexErr == 0 || g_holderNullErr == 0 ||
            g_holderInputErr == 0 || g_holderOutputErr == 0 ||
            g_holderPriceNullErr == 0 ) )
      {
         printf( "Failed: ParamHolder error-contract gate vacuous "
                 "(type=%lld index=%lld null=%lld priceNull=%lld input=%lld output=%lld)\n",
                 g_holderTypeErr, g_holderIndexErr, g_holderNullErr,
                 g_holderPriceNullErr, g_holderInputErr, g_holderOutputErr );
         errNumber = TA_ABS_TST_FAIL_HOLDER_CONTRACT_VACUOUS;
      }
      if( errNumber == TA_TEST_PASS )
         printf( "ParamHolder error contract: %lld refusals asserted "
                 "(%lld wrong-type, %lld bad-index, %lld NULL, %lld NULL price "
                 "component, %lld unbound-input, %lld unbound-output)\n",
                 g_holderTypeErr + g_holderIndexErr + g_holderNullErr +
                 g_holderPriceNullErr + g_holderInputErr + g_holderOutputErr,
                 g_holderTypeErr, g_holderIndexErr, g_holderNullErr,
                 g_holderPriceNullErr, g_holderInputErr, g_holderOutputErr );
   }

   /* A rejected SETTER leaves the holder as it found it (#266). */
   if( errNumber == TA_TEST_PASS )
      errNumber = testHolderStaysReusable();

   return errNumber;
}

static ErrorNumber callAndProfile( const char *funcName, ProfilingType type )
{
   TA_ParamHolder *paramHolder;
   const TA_FuncHandle *handle;
   const TA_FuncInfo *funcInfo;
   const TA_InputParameterInfo *inputInfo;
   const TA_OutputParameterInfo *outputInfo;

   TA_RetCode retCode;
   int h, i, j, k;
   int outBegIdx, outNbElement;

   /* Variables to control iteration and corresponding input size */
   int nbInnerLoop, nbOuterLoop;
   int stepSize;
   int inputSize;

   /* Variables measuring the execution time */
#ifdef WIN32
   LARGE_INTEGER startClock;
   LARGE_INTEGER endClock;
#else
   clock_t startClock;
   clock_t endClock;
#endif
   double clockDelta;
   int nbProfiledCallLocal;
   double timeInProfiledCallLocal;
   double worstProfiledCallLocal;

   nbProfiledCallLocal = 0;
   timeInProfiledCallLocal = 0.0;
   worstProfiledCallLocal = 0.0;
   nbInnerLoop = nbOuterLoop = stepSize = inputSize = 0;

   switch( type )
   {
   case PROFILING_10000:
	   nbInnerLoop = 1;
	   nbOuterLoop = 100;
	   stepSize = 10000;
	   inputSize = 10000;
	   break;
   case PROFILING_8000:
	   nbInnerLoop = 2;
	   nbOuterLoop = 50;
	   stepSize = 2000;
	   inputSize = 8000;
       break;
   case PROFILING_5000:
	   nbInnerLoop = 2;
	   nbOuterLoop = 50;
	   stepSize = 5000;
	   inputSize = 5000;
	   break;
   case PROFILING_2000:
	   nbInnerLoop = 5;
	   nbOuterLoop = 20;
	   stepSize = 2000;
	   inputSize = 2000;
	   break;
   case PROFILING_1000:
	   nbInnerLoop = 10;
	   nbOuterLoop = 10;
	   stepSize = 1000;
	   inputSize = 1000;
	   break;
   case PROFILING_500:
	   nbInnerLoop = 20;
	   nbOuterLoop = 5;
	   stepSize = 500;
	   inputSize = 500;
	   break;
   case PROFILING_100:
	   nbInnerLoop = 100;
	   nbOuterLoop = 1;
	   stepSize = 100;
	   inputSize = 100;
	   break;
   }

   retCode = TA_GetFuncHandle( funcName, &handle );
   if( retCode != TA_SUCCESS )
   {
      printf( "Can't get the function handle [%d]\n", retCode );
      return TA_ABS_TST_FAIL_GETFUNCHANDLE;
   }

   retCode = TA_ParamHolderAlloc( handle, &paramHolder );
   if( retCode != TA_SUCCESS )
   {
      printf( "Can't allocate the param holder [%d]\n", retCode );
      return TA_ABS_TST_FAIL_PARAMHOLDERALLOC;
   }

   TA_GetFuncInfo( handle, &funcInfo );

   for( i=0; i < (int)funcInfo->nbOutput; i++ )
   {
      TA_GetOutputParameterInfo( handle, i, &outputInfo );
	  switch(outputInfo->type)
	  {
	  case TA_Output_Real:
	     TA_SetOutputParamRealPtr(paramHolder,i,&output[i][0]);
         for( j=0; j < 2000; j++ )
            output[i][j] = TA_REAL_MIN;
		 break;
	  case TA_Output_Integer:
	     TA_SetOutputParamIntegerPtr(paramHolder,i,&output_int[i][0]);
         for( j=0; j < 2000; j++ )
            output_int[i][j] = TA_INTEGER_MIN;
		 break;
	  }
   }

   for( h=0; h < 2; h++ )
   {
   for( i=0; i < nbOuterLoop; i++ )
   {
	   for( j=0; j < nbInnerLoop; j++ )
	   {
		   /* Prepare input. */
		   for( k=0; k < (int)funcInfo->nbInput; k++ )
		   {
			  TA_GetInputParameterInfo( handle, k, &inputInfo );
			  switch(inputInfo->type)
			  {
			  case TA_Input_Price:
				 TA_SetInputParamPricePtr( paramHolder, k,
					 inputInfo->flags&TA_IN_PRICE_OPEN?&gDataOpen[j*stepSize]:NULL,
					 inputInfo->flags&TA_IN_PRICE_HIGH?&gDataHigh[j*stepSize]:NULL,
					 inputInfo->flags&TA_IN_PRICE_LOW?&gDataLow[j*stepSize]:NULL,
					 inputInfo->flags&TA_IN_PRICE_CLOSE?&gDataClose[j*stepSize]:NULL,
					 inputInfo->flags&TA_IN_PRICE_VOLUME?&gDataClose[j*stepSize]:NULL, NULL );
				 break;
			  case TA_Input_Real:
				 TA_SetInputParamRealPtr( paramHolder, k, &gDataClose[j*stepSize] );
				 break;
			  case TA_Input_Integer:
				 printf( "\nError: Integer input not yet supported for profiling.\n" );
				 return TA_ABS_TST_FAIL_CALLFUNC_1;
			  }
		   }

           #ifdef WIN32
              QueryPerformanceCounter(&startClock);
           #else
              startClock = clock();
           #endif

		   /* Do the function call. */
		   retCode = TA_CallFunc(paramHolder,0,inputSize-1,&outBegIdx,&outNbElement);
		   if( retCode != TA_SUCCESS )
		   {
		      printf( "TA_CallFunc() failed zero data test [%d]\n", retCode );
		      TA_ParamHolderFree( paramHolder );
		      return TA_ABS_TST_FAIL_CALLFUNC_1;
		   }

		   #ifdef WIN32
			   QueryPerformanceCounter(&endClock);
			   clockDelta = (double)((__int64)endClock.QuadPart - (__int64) startClock.QuadPart);
		   #else
			   endClock = clock();
			   clockDelta = (double)(endClock - startClock);
		   #endif

		   /* Setup global profiling info. */
		   if( clockDelta <= 0 )
		   {
			   printf( "Error: Insufficient timer precision to perform benchmarking on this platform.\n" );
			   return TA_ABS_TST_FAIL_CALLFUNC_1;
		   }
		   else
		   {
			   if( clockDelta > worstProfiledCall )
			      worstProfiledCall = clockDelta;
			   timeInProfiledCall += clockDelta;
			   nbProfiledCall++;
		   }

		   /* Setup local profiling info for this particular function. */
		   if( clockDelta > worstProfiledCallLocal )
			   worstProfiledCallLocal = clockDelta;
		   timeInProfiledCallLocal += clockDelta;
		   nbProfiledCallLocal++;
	   }
   }
   }

   /* Output statistic (remove worst call, average the others. */
   printf( "%g ", (timeInProfiledCallLocal-worstProfiledCallLocal)/(double)(nbProfiledCallLocal-1));

   retCode = TA_ParamHolderFree( paramHolder );
   if( retCode != TA_SUCCESS )
   {
      printf( "TA_ParamHolderFree failed [%d]\n", retCode );
      return TA_ABS_TST_FAIL_PARAMHOLDERFREE;
   }

   return TA_TEST_PASS;
}
