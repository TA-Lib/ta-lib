/* Auto-generated instruction-count benchmark for ta_codegen C output.
 * Links the shipped library; measures retired instructions per entry point.
 * Output: `NAME KIND measured|skipped RETCODE` per line, plus one callgrind
 * dump named `NAME/KIND` per measured region.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

#include "ta_libc.h"
#include "bench_corpus.h"
#include "tools/ta_alloc_check.h"

/* Built without the valgrind headers the binary still compiles and still
   prints its rows, so `--dry-run` works anywhere; anything that would report a
   MEASUREMENT refuses to start (main), because a no-op dump is a suite that
   compares nothing and reads green. */
#if defined(__has_include)
#  if __has_include(<valgrind/callgrind.h>)
#    include <valgrind/callgrind.h>
#    define ICOUNT_HAVE_CALLGRIND 1
#  endif
#endif

#ifdef ICOUNT_HAVE_CALLGRIND
#  define ICOUNT_ZERO()        CALLGRIND_ZERO_STATS
#  define ICOUNT_DUMP(marker)  CALLGRIND_DUMP_STATS_AT(marker)
#  define ICOUNT_UNDER_VG()    (RUNNING_ON_VALGRIND != 0)
#else
#  define ICOUNT_ZERO()        do { } while(0)
#  define ICOUNT_DUMP(marker)  do { (void)(marker); } while(0)
#  define ICOUNT_UNDER_VG()    0
#endif

/* The per-bar feed rotates over the first ICOUNT_MASK+1 bars, so the update
   and peek regions stay in cache and are reproducible at any --points. */
#define ICOUNT_MASK 4095

static int g_rows = 0, g_measured = 0, g_skipped = 0;

static void icount_row(const char *nm, const char *kind, int measured, TA_RetCode rc)
{
    printf("%s %s %s %d\n", nm, kind, measured ? "measured" : "skipped", (int)rc);
    g_rows++;
    if( measured ) g_measured++; else g_skipped++;
}


static double *g_open, *g_high, *g_low, *g_close, *g_volume, *g_oi, *g_periods;
static int g_nPoints;

/* Corpus selection (--shape / --seed / --regime-period / --trend-strength).
 * The defaults reproduce the seed-42 walk this binary generated inline before
 * bench_corpus.h existed. */
static BenchCorpusCfg g_corpus = { BENCH_RANDWALK, BENCH_CORPUS_SEED,
                                   BENCH_CORPUS_PERIOD, BENCH_CORPUS_TREND };

static void generate_price_data(int n) {
    g_nPoints = n;
    g_open   = calloc(n, sizeof(double));
    g_high   = calloc(n, sizeof(double));
    g_low    = calloc(n, sizeof(double));
    g_close  = calloc(n, sizeof(double));
    g_volume = calloc(n, sizeof(double));
    g_oi     = calloc(n, sizeof(double));
    g_periods = calloc(n, sizeof(double));
    if( !g_open || !g_high || !g_low || !g_close || !g_volume || !g_oi || !g_periods )
        TA_TOOL_OOM("the price data arrays");
    bench_corpus_gen(&g_corpus, n,
                     g_open, g_high, g_low, g_close, g_volume, g_oi, g_periods);
}

#define MAX_POINTS 200000
static double g_outBuf0[MAX_POINTS];
static double g_outBuf1[MAX_POINTS];
static double g_outBuf2[MAX_POINTS];
static double g_outBuf3[MAX_POINTS];
static int g_outIntBuf0[MAX_POINTS];
static int g_outIntBuf1[MAX_POINTS];

static volatile int g_sink = 0;

/* strtok_r and strcasestr are POSIX; the MSVC CRT has neither. strtok_s is the
 * same call with the same reentrancy contract, and the case-insensitive search
 * is short enough to spell out rather than reach for a platform extension.
 */
#if defined(_WIN32)
#define bench_strtok_r(str, delim, save) strtok_s((str), (delim), (save))
#else
#define bench_strtok_r(str, delim, save) strtok_r((str), (delim), (save))
#endif

static const char *bench_strcasestr(const char *hay, const char *needle) {
    if( !*needle ) return hay;
    for( ; *hay; hay++ )
    {
        const char *h = hay, *n = needle;
        while( *h && *n && tolower((unsigned char)*h) == tolower((unsigned char)*n) )
        {
            h++;
            n++;
        }
        if( !*n ) return hay;
    }
    return NULL;
}

static int func_matches(const char *filter, const char *name) {
    if( !filter || !*filter ) return 1;
    char buf[512]; strncpy(buf, filter, sizeof(buf)-1); buf[sizeof(buf)-1]='\0';
    char *saveptr = NULL;
    for( char *tok = bench_strtok_r(buf, ",", &saveptr); tok; tok = bench_strtok_r(NULL, ",", &saveptr) )
        if( bench_strcasestr(name, tok) ) return 1;
    return 0;
}

static void icount_AC(int iters) {
    const char *nm = "AC";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_AC_Stream *st = NULL;
    TA_AC_Stream *stf = NULL;
    double v0 = 0.0;

    ICOUNT_ZERO();
    rc = TA_AC(0, g_nPoints - 1, g_high, g_low, 5, 34, 5, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("AC/batch");
    icount_row(nm, "batch", 1, rc);
    acc += g_outBuf0[0];

    ICOUNT_ZERO();
    rc = TA_AC_OpenAndFill(&stf, g_high, g_low, g_nPoints, 5, 34, 5, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("AC/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += g_outBuf0[0];
    if( stf ) TA_AC_Close(stf);

    ICOUNT_ZERO();
    rc = TA_AC_Open(&st, g_high, g_low, g_nPoints, 5, 34, 5, &v0);
    ICOUNT_DUMP("AC/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_AC_Update(st, g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("AC/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_AC_Peek(st, g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("AC/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_AC_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_ACCBANDS(int iters) {
    const char *nm = "ACCBANDS";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_ACCBANDS_Stream *st = NULL;
    TA_ACCBANDS_Stream *stf = NULL;
    double v0 = 0.0;
    double v1 = 0.0;
    double v2 = 0.0;

    ICOUNT_ZERO();
    rc = TA_ACCBANDS(0, g_nPoints - 1, g_high, g_low, g_close, 20, &outBegIdx, &outNBElement, g_outBuf0, g_outBuf1, g_outBuf2);
    ICOUNT_DUMP("ACCBANDS/batch");
    icount_row(nm, "batch", 1, rc);
    acc += g_outBuf0[0];
    acc += g_outBuf1[0];
    acc += g_outBuf2[0];

    ICOUNT_ZERO();
    rc = TA_ACCBANDS_OpenAndFill(&stf, g_high, g_low, g_close, g_nPoints, 20, &outBegIdx, &outNBElement, g_outBuf0, g_outBuf1, g_outBuf2);
    ICOUNT_DUMP("ACCBANDS/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += g_outBuf0[0];
    acc += g_outBuf1[0];
    acc += g_outBuf2[0];
    if( stf ) TA_ACCBANDS_Close(stf);

    ICOUNT_ZERO();
    rc = TA_ACCBANDS_Open(&st, g_high, g_low, g_close, g_nPoints, 20, &v0, &v1, &v2);
    ICOUNT_DUMP("ACCBANDS/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_ACCBANDS_Update(st, g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &v0, &v1, &v2);
            acc += v0;
            acc += v1;
            acc += v2;
        }
        ICOUNT_DUMP("ACCBANDS/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_ACCBANDS_Peek(st, g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &v0, &v1, &v2);
            acc += v0;
            acc += v1;
            acc += v2;
        }
        ICOUNT_DUMP("ACCBANDS/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_ACCBANDS_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_ACOS(int iters) {
    const char *nm = "ACOS";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_ACOS_Stream *st = NULL;
    TA_ACOS_Stream *stf = NULL;
    double v0 = 0.0;

    ICOUNT_ZERO();
    rc = TA_ACOS(0, g_nPoints - 1, g_close, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("ACOS/batch");
    icount_row(nm, "batch", 1, rc);
    acc += g_outBuf0[0];

    ICOUNT_ZERO();
    rc = TA_ACOS_OpenAndFill(&stf, g_close, g_nPoints, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("ACOS/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += g_outBuf0[0];
    if( stf ) TA_ACOS_Close(stf);

    ICOUNT_ZERO();
    rc = TA_ACOS_Open(&st, g_close, g_nPoints, &v0);
    ICOUNT_DUMP("ACOS/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_ACOS_Update(st, g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("ACOS/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_ACOS_Peek(st, g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("ACOS/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_ACOS_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_AD(int iters) {
    const char *nm = "AD";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_AD_Stream *st = NULL;
    TA_AD_Stream *stf = NULL;
    double v0 = 0.0;

    ICOUNT_ZERO();
    rc = TA_AD(0, g_nPoints - 1, g_high, g_low, g_close, g_volume, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("AD/batch");
    icount_row(nm, "batch", 1, rc);
    acc += g_outBuf0[0];

    ICOUNT_ZERO();
    rc = TA_AD_OpenAndFill(&stf, g_high, g_low, g_close, g_volume, g_nPoints, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("AD/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += g_outBuf0[0];
    if( stf ) TA_AD_Close(stf);

    ICOUNT_ZERO();
    rc = TA_AD_Open(&st, g_high, g_low, g_close, g_volume, g_nPoints, &v0);
    ICOUNT_DUMP("AD/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_AD_Update(st, g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], g_volume[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("AD/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_AD_Peek(st, g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], g_volume[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("AD/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_AD_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_ADD(int iters) {
    const char *nm = "ADD";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_ADD_Stream *st = NULL;
    TA_ADD_Stream *stf = NULL;
    double v0 = 0.0;

    ICOUNT_ZERO();
    rc = TA_ADD(0, g_nPoints - 1, g_close, g_high, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("ADD/batch");
    icount_row(nm, "batch", 1, rc);
    acc += g_outBuf0[0];

    ICOUNT_ZERO();
    rc = TA_ADD_OpenAndFill(&stf, g_close, g_high, g_nPoints, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("ADD/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += g_outBuf0[0];
    if( stf ) TA_ADD_Close(stf);

    ICOUNT_ZERO();
    rc = TA_ADD_Open(&st, g_close, g_high, g_nPoints, &v0);
    ICOUNT_DUMP("ADD/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_ADD_Update(st, g_close[it & ICOUNT_MASK], g_high[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("ADD/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_ADD_Peek(st, g_close[it & ICOUNT_MASK], g_high[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("ADD/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_ADD_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_ADOSC(int iters) {
    const char *nm = "ADOSC";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_ADOSC_Stream *st = NULL;
    TA_ADOSC_Stream *stf = NULL;
    double v0 = 0.0;

    ICOUNT_ZERO();
    rc = TA_ADOSC(0, g_nPoints - 1, g_high, g_low, g_close, g_volume, 3, 10, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("ADOSC/batch");
    icount_row(nm, "batch", 1, rc);
    acc += g_outBuf0[0];

    ICOUNT_ZERO();
    rc = TA_ADOSC_OpenAndFill(&stf, g_high, g_low, g_close, g_volume, g_nPoints, 3, 10, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("ADOSC/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += g_outBuf0[0];
    if( stf ) TA_ADOSC_Close(stf);

    ICOUNT_ZERO();
    rc = TA_ADOSC_Open(&st, g_high, g_low, g_close, g_volume, g_nPoints, 3, 10, &v0);
    ICOUNT_DUMP("ADOSC/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_ADOSC_Update(st, g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], g_volume[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("ADOSC/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_ADOSC_Peek(st, g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], g_volume[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("ADOSC/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_ADOSC_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_ADR(int iters) {
    const char *nm = "ADR";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_ADR_Stream *st = NULL;
    TA_ADR_Stream *stf = NULL;
    double v0 = 0.0;

    ICOUNT_ZERO();
    rc = TA_ADR(0, g_nPoints - 1, g_high, g_low, 14, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("ADR/batch");
    icount_row(nm, "batch", 1, rc);
    acc += g_outBuf0[0];

    ICOUNT_ZERO();
    rc = TA_ADR_OpenAndFill(&stf, g_high, g_low, g_nPoints, 14, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("ADR/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += g_outBuf0[0];
    if( stf ) TA_ADR_Close(stf);

    ICOUNT_ZERO();
    rc = TA_ADR_Open(&st, g_high, g_low, g_nPoints, 14, &v0);
    ICOUNT_DUMP("ADR/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_ADR_Update(st, g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("ADR/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_ADR_Peek(st, g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("ADR/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_ADR_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_ADX(int iters) {
    const char *nm = "ADX";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_ADX_Stream *st = NULL;
    TA_ADX_Stream *stf = NULL;
    double v0 = 0.0;

    ICOUNT_ZERO();
    rc = TA_ADX(0, g_nPoints - 1, g_high, g_low, g_close, 14, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("ADX/batch");
    icount_row(nm, "batch", 1, rc);
    acc += g_outBuf0[0];

    ICOUNT_ZERO();
    rc = TA_ADX_OpenAndFill(&stf, g_high, g_low, g_close, g_nPoints, 14, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("ADX/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += g_outBuf0[0];
    if( stf ) TA_ADX_Close(stf);

    ICOUNT_ZERO();
    rc = TA_ADX_Open(&st, g_high, g_low, g_close, g_nPoints, 14, &v0);
    ICOUNT_DUMP("ADX/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_ADX_Update(st, g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("ADX/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_ADX_Peek(st, g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("ADX/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_ADX_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_ADXR(int iters) {
    const char *nm = "ADXR";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_ADXR_Stream *st = NULL;
    TA_ADXR_Stream *stf = NULL;
    double v0 = 0.0;

    ICOUNT_ZERO();
    rc = TA_ADXR(0, g_nPoints - 1, g_high, g_low, g_close, 14, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("ADXR/batch");
    icount_row(nm, "batch", 1, rc);
    acc += g_outBuf0[0];

    ICOUNT_ZERO();
    rc = TA_ADXR_OpenAndFill(&stf, g_high, g_low, g_close, g_nPoints, 14, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("ADXR/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += g_outBuf0[0];
    if( stf ) TA_ADXR_Close(stf);

    ICOUNT_ZERO();
    rc = TA_ADXR_Open(&st, g_high, g_low, g_close, g_nPoints, 14, &v0);
    ICOUNT_DUMP("ADXR/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_ADXR_Update(st, g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("ADXR/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_ADXR_Peek(st, g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("ADXR/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_ADXR_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_AO(int iters) {
    const char *nm = "AO";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_AO_Stream *st = NULL;
    TA_AO_Stream *stf = NULL;
    double v0 = 0.0;

    ICOUNT_ZERO();
    rc = TA_AO(0, g_nPoints - 1, g_high, g_low, 5, 34, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("AO/batch");
    icount_row(nm, "batch", 1, rc);
    acc += g_outBuf0[0];

    ICOUNT_ZERO();
    rc = TA_AO_OpenAndFill(&stf, g_high, g_low, g_nPoints, 5, 34, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("AO/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += g_outBuf0[0];
    if( stf ) TA_AO_Close(stf);

    ICOUNT_ZERO();
    rc = TA_AO_Open(&st, g_high, g_low, g_nPoints, 5, 34, &v0);
    ICOUNT_DUMP("AO/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_AO_Update(st, g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("AO/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_AO_Peek(st, g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("AO/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_AO_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_APO(int iters) {
    const char *nm = "APO";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_APO_Stream *st = NULL;
    TA_APO_Stream *stf = NULL;
    double v0 = 0.0;

    ICOUNT_ZERO();
    rc = TA_APO(0, g_nPoints - 1, g_close, 12, 26, 1, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("APO/batch");
    icount_row(nm, "batch", 1, rc);
    acc += g_outBuf0[0];

    ICOUNT_ZERO();
    rc = TA_APO_OpenAndFill(&stf, g_close, g_nPoints, 12, 26, 1, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("APO/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += g_outBuf0[0];
    if( stf ) TA_APO_Close(stf);

    ICOUNT_ZERO();
    rc = TA_APO_Open(&st, g_close, g_nPoints, 12, 26, 1, &v0);
    ICOUNT_DUMP("APO/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_APO_Update(st, g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("APO/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_APO_Peek(st, g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("APO/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_APO_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_AROON(int iters) {
    const char *nm = "AROON";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_AROON_Stream *st = NULL;
    TA_AROON_Stream *stf = NULL;
    double v0 = 0.0;
    double v1 = 0.0;

    ICOUNT_ZERO();
    rc = TA_AROON(0, g_nPoints - 1, g_high, g_low, 14, &outBegIdx, &outNBElement, g_outBuf0, g_outBuf1);
    ICOUNT_DUMP("AROON/batch");
    icount_row(nm, "batch", 1, rc);
    acc += g_outBuf0[0];
    acc += g_outBuf1[0];

    ICOUNT_ZERO();
    rc = TA_AROON_OpenAndFill(&stf, g_high, g_low, g_nPoints, 14, &outBegIdx, &outNBElement, g_outBuf0, g_outBuf1);
    ICOUNT_DUMP("AROON/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += g_outBuf0[0];
    acc += g_outBuf1[0];
    if( stf ) TA_AROON_Close(stf);

    ICOUNT_ZERO();
    rc = TA_AROON_Open(&st, g_high, g_low, g_nPoints, 14, &v0, &v1);
    ICOUNT_DUMP("AROON/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_AROON_Update(st, g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], &v0, &v1);
            acc += v0;
            acc += v1;
        }
        ICOUNT_DUMP("AROON/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_AROON_Peek(st, g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], &v0, &v1);
            acc += v0;
            acc += v1;
        }
        ICOUNT_DUMP("AROON/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_AROON_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_AROONOSC(int iters) {
    const char *nm = "AROONOSC";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_AROONOSC_Stream *st = NULL;
    TA_AROONOSC_Stream *stf = NULL;
    double v0 = 0.0;

    ICOUNT_ZERO();
    rc = TA_AROONOSC(0, g_nPoints - 1, g_high, g_low, 14, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("AROONOSC/batch");
    icount_row(nm, "batch", 1, rc);
    acc += g_outBuf0[0];

    ICOUNT_ZERO();
    rc = TA_AROONOSC_OpenAndFill(&stf, g_high, g_low, g_nPoints, 14, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("AROONOSC/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += g_outBuf0[0];
    if( stf ) TA_AROONOSC_Close(stf);

    ICOUNT_ZERO();
    rc = TA_AROONOSC_Open(&st, g_high, g_low, g_nPoints, 14, &v0);
    ICOUNT_DUMP("AROONOSC/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_AROONOSC_Update(st, g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("AROONOSC/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_AROONOSC_Peek(st, g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("AROONOSC/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_AROONOSC_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_ASIN(int iters) {
    const char *nm = "ASIN";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_ASIN_Stream *st = NULL;
    TA_ASIN_Stream *stf = NULL;
    double v0 = 0.0;

    ICOUNT_ZERO();
    rc = TA_ASIN(0, g_nPoints - 1, g_close, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("ASIN/batch");
    icount_row(nm, "batch", 1, rc);
    acc += g_outBuf0[0];

    ICOUNT_ZERO();
    rc = TA_ASIN_OpenAndFill(&stf, g_close, g_nPoints, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("ASIN/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += g_outBuf0[0];
    if( stf ) TA_ASIN_Close(stf);

    ICOUNT_ZERO();
    rc = TA_ASIN_Open(&st, g_close, g_nPoints, &v0);
    ICOUNT_DUMP("ASIN/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_ASIN_Update(st, g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("ASIN/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_ASIN_Peek(st, g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("ASIN/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_ASIN_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_ATAN(int iters) {
    const char *nm = "ATAN";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_ATAN_Stream *st = NULL;
    TA_ATAN_Stream *stf = NULL;
    double v0 = 0.0;

    ICOUNT_ZERO();
    rc = TA_ATAN(0, g_nPoints - 1, g_close, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("ATAN/batch");
    icount_row(nm, "batch", 1, rc);
    acc += g_outBuf0[0];

    ICOUNT_ZERO();
    rc = TA_ATAN_OpenAndFill(&stf, g_close, g_nPoints, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("ATAN/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += g_outBuf0[0];
    if( stf ) TA_ATAN_Close(stf);

    ICOUNT_ZERO();
    rc = TA_ATAN_Open(&st, g_close, g_nPoints, &v0);
    ICOUNT_DUMP("ATAN/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_ATAN_Update(st, g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("ATAN/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_ATAN_Peek(st, g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("ATAN/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_ATAN_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_ATR(int iters) {
    const char *nm = "ATR";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_ATR_Stream *st = NULL;
    TA_ATR_Stream *stf = NULL;
    double v0 = 0.0;

    ICOUNT_ZERO();
    rc = TA_ATR(0, g_nPoints - 1, g_high, g_low, g_close, 14, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("ATR/batch");
    icount_row(nm, "batch", 1, rc);
    acc += g_outBuf0[0];

    ICOUNT_ZERO();
    rc = TA_ATR_OpenAndFill(&stf, g_high, g_low, g_close, g_nPoints, 14, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("ATR/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += g_outBuf0[0];
    if( stf ) TA_ATR_Close(stf);

    ICOUNT_ZERO();
    rc = TA_ATR_Open(&st, g_high, g_low, g_close, g_nPoints, 14, &v0);
    ICOUNT_DUMP("ATR/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_ATR_Update(st, g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("ATR/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_ATR_Peek(st, g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("ATR/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_ATR_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_AVGDEV(int iters) {
    const char *nm = "AVGDEV";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_AVGDEV_Stream *st = NULL;
    TA_AVGDEV_Stream *stf = NULL;
    double v0 = 0.0;

    ICOUNT_ZERO();
    rc = TA_AVGDEV(0, g_nPoints - 1, g_close, 14, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("AVGDEV/batch");
    icount_row(nm, "batch", 1, rc);
    acc += g_outBuf0[0];

    ICOUNT_ZERO();
    rc = TA_AVGDEV_OpenAndFill(&stf, g_close, g_nPoints, 14, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("AVGDEV/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += g_outBuf0[0];
    if( stf ) TA_AVGDEV_Close(stf);

    ICOUNT_ZERO();
    rc = TA_AVGDEV_Open(&st, g_close, g_nPoints, 14, &v0);
    ICOUNT_DUMP("AVGDEV/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_AVGDEV_Update(st, g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("AVGDEV/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_AVGDEV_Peek(st, g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("AVGDEV/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_AVGDEV_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_AVGPRICE(int iters) {
    const char *nm = "AVGPRICE";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_AVGPRICE_Stream *st = NULL;
    TA_AVGPRICE_Stream *stf = NULL;
    double v0 = 0.0;

    ICOUNT_ZERO();
    rc = TA_AVGPRICE(0, g_nPoints - 1, g_open, g_high, g_low, g_close, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("AVGPRICE/batch");
    icount_row(nm, "batch", 1, rc);
    acc += g_outBuf0[0];

    ICOUNT_ZERO();
    rc = TA_AVGPRICE_OpenAndFill(&stf, g_open, g_high, g_low, g_close, g_nPoints, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("AVGPRICE/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += g_outBuf0[0];
    if( stf ) TA_AVGPRICE_Close(stf);

    ICOUNT_ZERO();
    rc = TA_AVGPRICE_Open(&st, g_open, g_high, g_low, g_close, g_nPoints, &v0);
    ICOUNT_DUMP("AVGPRICE/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_AVGPRICE_Update(st, g_open[it & ICOUNT_MASK], g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("AVGPRICE/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_AVGPRICE_Peek(st, g_open[it & ICOUNT_MASK], g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("AVGPRICE/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_AVGPRICE_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_BBANDS(int iters) {
    const char *nm = "BBANDS";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_BBANDS_Stream *st = NULL;
    TA_BBANDS_Stream *stf = NULL;
    double v0 = 0.0;
    double v1 = 0.0;
    double v2 = 0.0;

    ICOUNT_ZERO();
    rc = TA_BBANDS(0, g_nPoints - 1, g_close, 20, 2.000000000000000, 2.000000000000000, 0, &outBegIdx, &outNBElement, g_outBuf0, g_outBuf1, g_outBuf2);
    ICOUNT_DUMP("BBANDS/batch");
    icount_row(nm, "batch", 1, rc);
    acc += g_outBuf0[0];
    acc += g_outBuf1[0];
    acc += g_outBuf2[0];

    ICOUNT_ZERO();
    rc = TA_BBANDS_OpenAndFill(&stf, g_close, g_nPoints, 20, 2.000000000000000, 2.000000000000000, 0, &outBegIdx, &outNBElement, g_outBuf0, g_outBuf1, g_outBuf2);
    ICOUNT_DUMP("BBANDS/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += g_outBuf0[0];
    acc += g_outBuf1[0];
    acc += g_outBuf2[0];
    if( stf ) TA_BBANDS_Close(stf);

    ICOUNT_ZERO();
    rc = TA_BBANDS_Open(&st, g_close, g_nPoints, 20, 2.000000000000000, 2.000000000000000, 0, &v0, &v1, &v2);
    ICOUNT_DUMP("BBANDS/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_BBANDS_Update(st, g_close[it & ICOUNT_MASK], &v0, &v1, &v2);
            acc += v0;
            acc += v1;
            acc += v2;
        }
        ICOUNT_DUMP("BBANDS/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_BBANDS_Peek(st, g_close[it & ICOUNT_MASK], &v0, &v1, &v2);
            acc += v0;
            acc += v1;
            acc += v2;
        }
        ICOUNT_DUMP("BBANDS/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_BBANDS_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_BETA(int iters) {
    const char *nm = "BETA";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_BETA_Stream *st = NULL;
    TA_BETA_Stream *stf = NULL;
    double v0 = 0.0;

    ICOUNT_ZERO();
    rc = TA_BETA(0, g_nPoints - 1, g_close, g_high, 5, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("BETA/batch");
    icount_row(nm, "batch", 1, rc);
    acc += g_outBuf0[0];

    ICOUNT_ZERO();
    rc = TA_BETA_OpenAndFill(&stf, g_close, g_high, g_nPoints, 5, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("BETA/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += g_outBuf0[0];
    if( stf ) TA_BETA_Close(stf);

    ICOUNT_ZERO();
    rc = TA_BETA_Open(&st, g_close, g_high, g_nPoints, 5, &v0);
    ICOUNT_DUMP("BETA/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_BETA_Update(st, g_close[it & ICOUNT_MASK], g_high[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("BETA/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_BETA_Peek(st, g_close[it & ICOUNT_MASK], g_high[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("BETA/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_BETA_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_BOP(int iters) {
    const char *nm = "BOP";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_BOP_Stream *st = NULL;
    TA_BOP_Stream *stf = NULL;
    double v0 = 0.0;

    ICOUNT_ZERO();
    rc = TA_BOP(0, g_nPoints - 1, g_open, g_high, g_low, g_close, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("BOP/batch");
    icount_row(nm, "batch", 1, rc);
    acc += g_outBuf0[0];

    ICOUNT_ZERO();
    rc = TA_BOP_OpenAndFill(&stf, g_open, g_high, g_low, g_close, g_nPoints, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("BOP/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += g_outBuf0[0];
    if( stf ) TA_BOP_Close(stf);

    ICOUNT_ZERO();
    rc = TA_BOP_Open(&st, g_open, g_high, g_low, g_close, g_nPoints, &v0);
    ICOUNT_DUMP("BOP/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_BOP_Update(st, g_open[it & ICOUNT_MASK], g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("BOP/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_BOP_Peek(st, g_open[it & ICOUNT_MASK], g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("BOP/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_BOP_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_CCI(int iters) {
    const char *nm = "CCI";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_CCI_Stream *st = NULL;
    TA_CCI_Stream *stf = NULL;
    double v0 = 0.0;

    ICOUNT_ZERO();
    rc = TA_CCI(0, g_nPoints - 1, g_high, g_low, g_close, 14, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("CCI/batch");
    icount_row(nm, "batch", 1, rc);
    acc += g_outBuf0[0];

    ICOUNT_ZERO();
    rc = TA_CCI_OpenAndFill(&stf, g_high, g_low, g_close, g_nPoints, 14, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("CCI/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += g_outBuf0[0];
    if( stf ) TA_CCI_Close(stf);

    ICOUNT_ZERO();
    rc = TA_CCI_Open(&st, g_high, g_low, g_close, g_nPoints, 14, &v0);
    ICOUNT_DUMP("CCI/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_CCI_Update(st, g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("CCI/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_CCI_Peek(st, g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("CCI/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_CCI_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_CDL2CROWS(int iters) {
    const char *nm = "CDL2CROWS";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_CDL2CROWS_Stream *st = NULL;
    TA_CDL2CROWS_Stream *stf = NULL;
    int iv0 = 0;

    ICOUNT_ZERO();
    rc = TA_CDL2CROWS(0, g_nPoints - 1, g_open, g_high, g_low, g_close, &outBegIdx, &outNBElement, g_outIntBuf0);
    ICOUNT_DUMP("CDL2CROWS/batch");
    icount_row(nm, "batch", 1, rc);
    acc += (double)g_outIntBuf0[0];

    ICOUNT_ZERO();
    rc = TA_CDL2CROWS_OpenAndFill(&stf, g_open, g_high, g_low, g_close, g_nPoints, &outBegIdx, &outNBElement, g_outIntBuf0);
    ICOUNT_DUMP("CDL2CROWS/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += (double)g_outIntBuf0[0];
    if( stf ) TA_CDL2CROWS_Close(stf);

    ICOUNT_ZERO();
    rc = TA_CDL2CROWS_Open(&st, g_open, g_high, g_low, g_close, g_nPoints, &iv0);
    ICOUNT_DUMP("CDL2CROWS/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_CDL2CROWS_Update(st, g_open[it & ICOUNT_MASK], g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &iv0);
            acc += (double)iv0;
        }
        ICOUNT_DUMP("CDL2CROWS/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_CDL2CROWS_Peek(st, g_open[it & ICOUNT_MASK], g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &iv0);
            acc += (double)iv0;
        }
        ICOUNT_DUMP("CDL2CROWS/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_CDL2CROWS_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_CDL3BLACKCROWS(int iters) {
    const char *nm = "CDL3BLACKCROWS";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_CDL3BLACKCROWS_Stream *st = NULL;
    TA_CDL3BLACKCROWS_Stream *stf = NULL;
    int iv0 = 0;

    ICOUNT_ZERO();
    rc = TA_CDL3BLACKCROWS(0, g_nPoints - 1, g_open, g_high, g_low, g_close, &outBegIdx, &outNBElement, g_outIntBuf0);
    ICOUNT_DUMP("CDL3BLACKCROWS/batch");
    icount_row(nm, "batch", 1, rc);
    acc += (double)g_outIntBuf0[0];

    ICOUNT_ZERO();
    rc = TA_CDL3BLACKCROWS_OpenAndFill(&stf, g_open, g_high, g_low, g_close, g_nPoints, &outBegIdx, &outNBElement, g_outIntBuf0);
    ICOUNT_DUMP("CDL3BLACKCROWS/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += (double)g_outIntBuf0[0];
    if( stf ) TA_CDL3BLACKCROWS_Close(stf);

    ICOUNT_ZERO();
    rc = TA_CDL3BLACKCROWS_Open(&st, g_open, g_high, g_low, g_close, g_nPoints, &iv0);
    ICOUNT_DUMP("CDL3BLACKCROWS/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_CDL3BLACKCROWS_Update(st, g_open[it & ICOUNT_MASK], g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &iv0);
            acc += (double)iv0;
        }
        ICOUNT_DUMP("CDL3BLACKCROWS/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_CDL3BLACKCROWS_Peek(st, g_open[it & ICOUNT_MASK], g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &iv0);
            acc += (double)iv0;
        }
        ICOUNT_DUMP("CDL3BLACKCROWS/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_CDL3BLACKCROWS_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_CDL3INSIDE(int iters) {
    const char *nm = "CDL3INSIDE";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_CDL3INSIDE_Stream *st = NULL;
    TA_CDL3INSIDE_Stream *stf = NULL;
    int iv0 = 0;

    ICOUNT_ZERO();
    rc = TA_CDL3INSIDE(0, g_nPoints - 1, g_open, g_high, g_low, g_close, &outBegIdx, &outNBElement, g_outIntBuf0);
    ICOUNT_DUMP("CDL3INSIDE/batch");
    icount_row(nm, "batch", 1, rc);
    acc += (double)g_outIntBuf0[0];

    ICOUNT_ZERO();
    rc = TA_CDL3INSIDE_OpenAndFill(&stf, g_open, g_high, g_low, g_close, g_nPoints, &outBegIdx, &outNBElement, g_outIntBuf0);
    ICOUNT_DUMP("CDL3INSIDE/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += (double)g_outIntBuf0[0];
    if( stf ) TA_CDL3INSIDE_Close(stf);

    ICOUNT_ZERO();
    rc = TA_CDL3INSIDE_Open(&st, g_open, g_high, g_low, g_close, g_nPoints, &iv0);
    ICOUNT_DUMP("CDL3INSIDE/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_CDL3INSIDE_Update(st, g_open[it & ICOUNT_MASK], g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &iv0);
            acc += (double)iv0;
        }
        ICOUNT_DUMP("CDL3INSIDE/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_CDL3INSIDE_Peek(st, g_open[it & ICOUNT_MASK], g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &iv0);
            acc += (double)iv0;
        }
        ICOUNT_DUMP("CDL3INSIDE/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_CDL3INSIDE_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_CDL3LINESTRIKE(int iters) {
    const char *nm = "CDL3LINESTRIKE";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_CDL3LINESTRIKE_Stream *st = NULL;
    TA_CDL3LINESTRIKE_Stream *stf = NULL;
    int iv0 = 0;

    ICOUNT_ZERO();
    rc = TA_CDL3LINESTRIKE(0, g_nPoints - 1, g_open, g_high, g_low, g_close, &outBegIdx, &outNBElement, g_outIntBuf0);
    ICOUNT_DUMP("CDL3LINESTRIKE/batch");
    icount_row(nm, "batch", 1, rc);
    acc += (double)g_outIntBuf0[0];

    ICOUNT_ZERO();
    rc = TA_CDL3LINESTRIKE_OpenAndFill(&stf, g_open, g_high, g_low, g_close, g_nPoints, &outBegIdx, &outNBElement, g_outIntBuf0);
    ICOUNT_DUMP("CDL3LINESTRIKE/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += (double)g_outIntBuf0[0];
    if( stf ) TA_CDL3LINESTRIKE_Close(stf);

    ICOUNT_ZERO();
    rc = TA_CDL3LINESTRIKE_Open(&st, g_open, g_high, g_low, g_close, g_nPoints, &iv0);
    ICOUNT_DUMP("CDL3LINESTRIKE/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_CDL3LINESTRIKE_Update(st, g_open[it & ICOUNT_MASK], g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &iv0);
            acc += (double)iv0;
        }
        ICOUNT_DUMP("CDL3LINESTRIKE/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_CDL3LINESTRIKE_Peek(st, g_open[it & ICOUNT_MASK], g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &iv0);
            acc += (double)iv0;
        }
        ICOUNT_DUMP("CDL3LINESTRIKE/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_CDL3LINESTRIKE_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_CDL3OUTSIDE(int iters) {
    const char *nm = "CDL3OUTSIDE";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_CDL3OUTSIDE_Stream *st = NULL;
    TA_CDL3OUTSIDE_Stream *stf = NULL;
    int iv0 = 0;

    ICOUNT_ZERO();
    rc = TA_CDL3OUTSIDE(0, g_nPoints - 1, g_open, g_high, g_low, g_close, &outBegIdx, &outNBElement, g_outIntBuf0);
    ICOUNT_DUMP("CDL3OUTSIDE/batch");
    icount_row(nm, "batch", 1, rc);
    acc += (double)g_outIntBuf0[0];

    ICOUNT_ZERO();
    rc = TA_CDL3OUTSIDE_OpenAndFill(&stf, g_open, g_high, g_low, g_close, g_nPoints, &outBegIdx, &outNBElement, g_outIntBuf0);
    ICOUNT_DUMP("CDL3OUTSIDE/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += (double)g_outIntBuf0[0];
    if( stf ) TA_CDL3OUTSIDE_Close(stf);

    ICOUNT_ZERO();
    rc = TA_CDL3OUTSIDE_Open(&st, g_open, g_high, g_low, g_close, g_nPoints, &iv0);
    ICOUNT_DUMP("CDL3OUTSIDE/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_CDL3OUTSIDE_Update(st, g_open[it & ICOUNT_MASK], g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &iv0);
            acc += (double)iv0;
        }
        ICOUNT_DUMP("CDL3OUTSIDE/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_CDL3OUTSIDE_Peek(st, g_open[it & ICOUNT_MASK], g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &iv0);
            acc += (double)iv0;
        }
        ICOUNT_DUMP("CDL3OUTSIDE/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_CDL3OUTSIDE_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_CDL3STARSINSOUTH(int iters) {
    const char *nm = "CDL3STARSINSOUTH";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_CDL3STARSINSOUTH_Stream *st = NULL;
    TA_CDL3STARSINSOUTH_Stream *stf = NULL;
    int iv0 = 0;

    ICOUNT_ZERO();
    rc = TA_CDL3STARSINSOUTH(0, g_nPoints - 1, g_open, g_high, g_low, g_close, &outBegIdx, &outNBElement, g_outIntBuf0);
    ICOUNT_DUMP("CDL3STARSINSOUTH/batch");
    icount_row(nm, "batch", 1, rc);
    acc += (double)g_outIntBuf0[0];

    ICOUNT_ZERO();
    rc = TA_CDL3STARSINSOUTH_OpenAndFill(&stf, g_open, g_high, g_low, g_close, g_nPoints, &outBegIdx, &outNBElement, g_outIntBuf0);
    ICOUNT_DUMP("CDL3STARSINSOUTH/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += (double)g_outIntBuf0[0];
    if( stf ) TA_CDL3STARSINSOUTH_Close(stf);

    ICOUNT_ZERO();
    rc = TA_CDL3STARSINSOUTH_Open(&st, g_open, g_high, g_low, g_close, g_nPoints, &iv0);
    ICOUNT_DUMP("CDL3STARSINSOUTH/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_CDL3STARSINSOUTH_Update(st, g_open[it & ICOUNT_MASK], g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &iv0);
            acc += (double)iv0;
        }
        ICOUNT_DUMP("CDL3STARSINSOUTH/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_CDL3STARSINSOUTH_Peek(st, g_open[it & ICOUNT_MASK], g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &iv0);
            acc += (double)iv0;
        }
        ICOUNT_DUMP("CDL3STARSINSOUTH/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_CDL3STARSINSOUTH_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_CDL3WHITESOLDIERS(int iters) {
    const char *nm = "CDL3WHITESOLDIERS";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_CDL3WHITESOLDIERS_Stream *st = NULL;
    TA_CDL3WHITESOLDIERS_Stream *stf = NULL;
    int iv0 = 0;

    ICOUNT_ZERO();
    rc = TA_CDL3WHITESOLDIERS(0, g_nPoints - 1, g_open, g_high, g_low, g_close, &outBegIdx, &outNBElement, g_outIntBuf0);
    ICOUNT_DUMP("CDL3WHITESOLDIERS/batch");
    icount_row(nm, "batch", 1, rc);
    acc += (double)g_outIntBuf0[0];

    ICOUNT_ZERO();
    rc = TA_CDL3WHITESOLDIERS_OpenAndFill(&stf, g_open, g_high, g_low, g_close, g_nPoints, &outBegIdx, &outNBElement, g_outIntBuf0);
    ICOUNT_DUMP("CDL3WHITESOLDIERS/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += (double)g_outIntBuf0[0];
    if( stf ) TA_CDL3WHITESOLDIERS_Close(stf);

    ICOUNT_ZERO();
    rc = TA_CDL3WHITESOLDIERS_Open(&st, g_open, g_high, g_low, g_close, g_nPoints, &iv0);
    ICOUNT_DUMP("CDL3WHITESOLDIERS/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_CDL3WHITESOLDIERS_Update(st, g_open[it & ICOUNT_MASK], g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &iv0);
            acc += (double)iv0;
        }
        ICOUNT_DUMP("CDL3WHITESOLDIERS/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_CDL3WHITESOLDIERS_Peek(st, g_open[it & ICOUNT_MASK], g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &iv0);
            acc += (double)iv0;
        }
        ICOUNT_DUMP("CDL3WHITESOLDIERS/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_CDL3WHITESOLDIERS_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_CDLABANDONEDBABY(int iters) {
    const char *nm = "CDLABANDONEDBABY";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_CDLABANDONEDBABY_Stream *st = NULL;
    TA_CDLABANDONEDBABY_Stream *stf = NULL;
    int iv0 = 0;

    ICOUNT_ZERO();
    rc = TA_CDLABANDONEDBABY(0, g_nPoints - 1, g_open, g_high, g_low, g_close, 0.300000000000000, &outBegIdx, &outNBElement, g_outIntBuf0);
    ICOUNT_DUMP("CDLABANDONEDBABY/batch");
    icount_row(nm, "batch", 1, rc);
    acc += (double)g_outIntBuf0[0];

    ICOUNT_ZERO();
    rc = TA_CDLABANDONEDBABY_OpenAndFill(&stf, g_open, g_high, g_low, g_close, g_nPoints, 0.300000000000000, &outBegIdx, &outNBElement, g_outIntBuf0);
    ICOUNT_DUMP("CDLABANDONEDBABY/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += (double)g_outIntBuf0[0];
    if( stf ) TA_CDLABANDONEDBABY_Close(stf);

    ICOUNT_ZERO();
    rc = TA_CDLABANDONEDBABY_Open(&st, g_open, g_high, g_low, g_close, g_nPoints, 0.300000000000000, &iv0);
    ICOUNT_DUMP("CDLABANDONEDBABY/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_CDLABANDONEDBABY_Update(st, g_open[it & ICOUNT_MASK], g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &iv0);
            acc += (double)iv0;
        }
        ICOUNT_DUMP("CDLABANDONEDBABY/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_CDLABANDONEDBABY_Peek(st, g_open[it & ICOUNT_MASK], g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &iv0);
            acc += (double)iv0;
        }
        ICOUNT_DUMP("CDLABANDONEDBABY/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_CDLABANDONEDBABY_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_CDLADVANCEBLOCK(int iters) {
    const char *nm = "CDLADVANCEBLOCK";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_CDLADVANCEBLOCK_Stream *st = NULL;
    TA_CDLADVANCEBLOCK_Stream *stf = NULL;
    int iv0 = 0;

    ICOUNT_ZERO();
    rc = TA_CDLADVANCEBLOCK(0, g_nPoints - 1, g_open, g_high, g_low, g_close, &outBegIdx, &outNBElement, g_outIntBuf0);
    ICOUNT_DUMP("CDLADVANCEBLOCK/batch");
    icount_row(nm, "batch", 1, rc);
    acc += (double)g_outIntBuf0[0];

    ICOUNT_ZERO();
    rc = TA_CDLADVANCEBLOCK_OpenAndFill(&stf, g_open, g_high, g_low, g_close, g_nPoints, &outBegIdx, &outNBElement, g_outIntBuf0);
    ICOUNT_DUMP("CDLADVANCEBLOCK/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += (double)g_outIntBuf0[0];
    if( stf ) TA_CDLADVANCEBLOCK_Close(stf);

    ICOUNT_ZERO();
    rc = TA_CDLADVANCEBLOCK_Open(&st, g_open, g_high, g_low, g_close, g_nPoints, &iv0);
    ICOUNT_DUMP("CDLADVANCEBLOCK/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_CDLADVANCEBLOCK_Update(st, g_open[it & ICOUNT_MASK], g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &iv0);
            acc += (double)iv0;
        }
        ICOUNT_DUMP("CDLADVANCEBLOCK/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_CDLADVANCEBLOCK_Peek(st, g_open[it & ICOUNT_MASK], g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &iv0);
            acc += (double)iv0;
        }
        ICOUNT_DUMP("CDLADVANCEBLOCK/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_CDLADVANCEBLOCK_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_CDLBELTHOLD(int iters) {
    const char *nm = "CDLBELTHOLD";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_CDLBELTHOLD_Stream *st = NULL;
    TA_CDLBELTHOLD_Stream *stf = NULL;
    int iv0 = 0;

    ICOUNT_ZERO();
    rc = TA_CDLBELTHOLD(0, g_nPoints - 1, g_open, g_high, g_low, g_close, &outBegIdx, &outNBElement, g_outIntBuf0);
    ICOUNT_DUMP("CDLBELTHOLD/batch");
    icount_row(nm, "batch", 1, rc);
    acc += (double)g_outIntBuf0[0];

    ICOUNT_ZERO();
    rc = TA_CDLBELTHOLD_OpenAndFill(&stf, g_open, g_high, g_low, g_close, g_nPoints, &outBegIdx, &outNBElement, g_outIntBuf0);
    ICOUNT_DUMP("CDLBELTHOLD/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += (double)g_outIntBuf0[0];
    if( stf ) TA_CDLBELTHOLD_Close(stf);

    ICOUNT_ZERO();
    rc = TA_CDLBELTHOLD_Open(&st, g_open, g_high, g_low, g_close, g_nPoints, &iv0);
    ICOUNT_DUMP("CDLBELTHOLD/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_CDLBELTHOLD_Update(st, g_open[it & ICOUNT_MASK], g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &iv0);
            acc += (double)iv0;
        }
        ICOUNT_DUMP("CDLBELTHOLD/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_CDLBELTHOLD_Peek(st, g_open[it & ICOUNT_MASK], g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &iv0);
            acc += (double)iv0;
        }
        ICOUNT_DUMP("CDLBELTHOLD/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_CDLBELTHOLD_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_CDLBREAKAWAY(int iters) {
    const char *nm = "CDLBREAKAWAY";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_CDLBREAKAWAY_Stream *st = NULL;
    TA_CDLBREAKAWAY_Stream *stf = NULL;
    int iv0 = 0;

    ICOUNT_ZERO();
    rc = TA_CDLBREAKAWAY(0, g_nPoints - 1, g_open, g_high, g_low, g_close, &outBegIdx, &outNBElement, g_outIntBuf0);
    ICOUNT_DUMP("CDLBREAKAWAY/batch");
    icount_row(nm, "batch", 1, rc);
    acc += (double)g_outIntBuf0[0];

    ICOUNT_ZERO();
    rc = TA_CDLBREAKAWAY_OpenAndFill(&stf, g_open, g_high, g_low, g_close, g_nPoints, &outBegIdx, &outNBElement, g_outIntBuf0);
    ICOUNT_DUMP("CDLBREAKAWAY/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += (double)g_outIntBuf0[0];
    if( stf ) TA_CDLBREAKAWAY_Close(stf);

    ICOUNT_ZERO();
    rc = TA_CDLBREAKAWAY_Open(&st, g_open, g_high, g_low, g_close, g_nPoints, &iv0);
    ICOUNT_DUMP("CDLBREAKAWAY/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_CDLBREAKAWAY_Update(st, g_open[it & ICOUNT_MASK], g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &iv0);
            acc += (double)iv0;
        }
        ICOUNT_DUMP("CDLBREAKAWAY/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_CDLBREAKAWAY_Peek(st, g_open[it & ICOUNT_MASK], g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &iv0);
            acc += (double)iv0;
        }
        ICOUNT_DUMP("CDLBREAKAWAY/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_CDLBREAKAWAY_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_CDLCLOSINGMARUBOZU(int iters) {
    const char *nm = "CDLCLOSINGMARUBOZU";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_CDLCLOSINGMARUBOZU_Stream *st = NULL;
    TA_CDLCLOSINGMARUBOZU_Stream *stf = NULL;
    int iv0 = 0;

    ICOUNT_ZERO();
    rc = TA_CDLCLOSINGMARUBOZU(0, g_nPoints - 1, g_open, g_high, g_low, g_close, &outBegIdx, &outNBElement, g_outIntBuf0);
    ICOUNT_DUMP("CDLCLOSINGMARUBOZU/batch");
    icount_row(nm, "batch", 1, rc);
    acc += (double)g_outIntBuf0[0];

    ICOUNT_ZERO();
    rc = TA_CDLCLOSINGMARUBOZU_OpenAndFill(&stf, g_open, g_high, g_low, g_close, g_nPoints, &outBegIdx, &outNBElement, g_outIntBuf0);
    ICOUNT_DUMP("CDLCLOSINGMARUBOZU/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += (double)g_outIntBuf0[0];
    if( stf ) TA_CDLCLOSINGMARUBOZU_Close(stf);

    ICOUNT_ZERO();
    rc = TA_CDLCLOSINGMARUBOZU_Open(&st, g_open, g_high, g_low, g_close, g_nPoints, &iv0);
    ICOUNT_DUMP("CDLCLOSINGMARUBOZU/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_CDLCLOSINGMARUBOZU_Update(st, g_open[it & ICOUNT_MASK], g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &iv0);
            acc += (double)iv0;
        }
        ICOUNT_DUMP("CDLCLOSINGMARUBOZU/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_CDLCLOSINGMARUBOZU_Peek(st, g_open[it & ICOUNT_MASK], g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &iv0);
            acc += (double)iv0;
        }
        ICOUNT_DUMP("CDLCLOSINGMARUBOZU/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_CDLCLOSINGMARUBOZU_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_CDLCONCEALBABYSWALL(int iters) {
    const char *nm = "CDLCONCEALBABYSWALL";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_CDLCONCEALBABYSWALL_Stream *st = NULL;
    TA_CDLCONCEALBABYSWALL_Stream *stf = NULL;
    int iv0 = 0;

    ICOUNT_ZERO();
    rc = TA_CDLCONCEALBABYSWALL(0, g_nPoints - 1, g_open, g_high, g_low, g_close, &outBegIdx, &outNBElement, g_outIntBuf0);
    ICOUNT_DUMP("CDLCONCEALBABYSWALL/batch");
    icount_row(nm, "batch", 1, rc);
    acc += (double)g_outIntBuf0[0];

    ICOUNT_ZERO();
    rc = TA_CDLCONCEALBABYSWALL_OpenAndFill(&stf, g_open, g_high, g_low, g_close, g_nPoints, &outBegIdx, &outNBElement, g_outIntBuf0);
    ICOUNT_DUMP("CDLCONCEALBABYSWALL/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += (double)g_outIntBuf0[0];
    if( stf ) TA_CDLCONCEALBABYSWALL_Close(stf);

    ICOUNT_ZERO();
    rc = TA_CDLCONCEALBABYSWALL_Open(&st, g_open, g_high, g_low, g_close, g_nPoints, &iv0);
    ICOUNT_DUMP("CDLCONCEALBABYSWALL/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_CDLCONCEALBABYSWALL_Update(st, g_open[it & ICOUNT_MASK], g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &iv0);
            acc += (double)iv0;
        }
        ICOUNT_DUMP("CDLCONCEALBABYSWALL/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_CDLCONCEALBABYSWALL_Peek(st, g_open[it & ICOUNT_MASK], g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &iv0);
            acc += (double)iv0;
        }
        ICOUNT_DUMP("CDLCONCEALBABYSWALL/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_CDLCONCEALBABYSWALL_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_CDLCOUNTERATTACK(int iters) {
    const char *nm = "CDLCOUNTERATTACK";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_CDLCOUNTERATTACK_Stream *st = NULL;
    TA_CDLCOUNTERATTACK_Stream *stf = NULL;
    int iv0 = 0;

    ICOUNT_ZERO();
    rc = TA_CDLCOUNTERATTACK(0, g_nPoints - 1, g_open, g_high, g_low, g_close, &outBegIdx, &outNBElement, g_outIntBuf0);
    ICOUNT_DUMP("CDLCOUNTERATTACK/batch");
    icount_row(nm, "batch", 1, rc);
    acc += (double)g_outIntBuf0[0];

    ICOUNT_ZERO();
    rc = TA_CDLCOUNTERATTACK_OpenAndFill(&stf, g_open, g_high, g_low, g_close, g_nPoints, &outBegIdx, &outNBElement, g_outIntBuf0);
    ICOUNT_DUMP("CDLCOUNTERATTACK/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += (double)g_outIntBuf0[0];
    if( stf ) TA_CDLCOUNTERATTACK_Close(stf);

    ICOUNT_ZERO();
    rc = TA_CDLCOUNTERATTACK_Open(&st, g_open, g_high, g_low, g_close, g_nPoints, &iv0);
    ICOUNT_DUMP("CDLCOUNTERATTACK/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_CDLCOUNTERATTACK_Update(st, g_open[it & ICOUNT_MASK], g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &iv0);
            acc += (double)iv0;
        }
        ICOUNT_DUMP("CDLCOUNTERATTACK/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_CDLCOUNTERATTACK_Peek(st, g_open[it & ICOUNT_MASK], g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &iv0);
            acc += (double)iv0;
        }
        ICOUNT_DUMP("CDLCOUNTERATTACK/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_CDLCOUNTERATTACK_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_CDLDARKCLOUDCOVER(int iters) {
    const char *nm = "CDLDARKCLOUDCOVER";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_CDLDARKCLOUDCOVER_Stream *st = NULL;
    TA_CDLDARKCLOUDCOVER_Stream *stf = NULL;
    int iv0 = 0;

    ICOUNT_ZERO();
    rc = TA_CDLDARKCLOUDCOVER(0, g_nPoints - 1, g_open, g_high, g_low, g_close, 0.500000000000000, &outBegIdx, &outNBElement, g_outIntBuf0);
    ICOUNT_DUMP("CDLDARKCLOUDCOVER/batch");
    icount_row(nm, "batch", 1, rc);
    acc += (double)g_outIntBuf0[0];

    ICOUNT_ZERO();
    rc = TA_CDLDARKCLOUDCOVER_OpenAndFill(&stf, g_open, g_high, g_low, g_close, g_nPoints, 0.500000000000000, &outBegIdx, &outNBElement, g_outIntBuf0);
    ICOUNT_DUMP("CDLDARKCLOUDCOVER/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += (double)g_outIntBuf0[0];
    if( stf ) TA_CDLDARKCLOUDCOVER_Close(stf);

    ICOUNT_ZERO();
    rc = TA_CDLDARKCLOUDCOVER_Open(&st, g_open, g_high, g_low, g_close, g_nPoints, 0.500000000000000, &iv0);
    ICOUNT_DUMP("CDLDARKCLOUDCOVER/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_CDLDARKCLOUDCOVER_Update(st, g_open[it & ICOUNT_MASK], g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &iv0);
            acc += (double)iv0;
        }
        ICOUNT_DUMP("CDLDARKCLOUDCOVER/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_CDLDARKCLOUDCOVER_Peek(st, g_open[it & ICOUNT_MASK], g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &iv0);
            acc += (double)iv0;
        }
        ICOUNT_DUMP("CDLDARKCLOUDCOVER/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_CDLDARKCLOUDCOVER_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_CDLDOJI(int iters) {
    const char *nm = "CDLDOJI";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_CDLDOJI_Stream *st = NULL;
    TA_CDLDOJI_Stream *stf = NULL;
    int iv0 = 0;

    ICOUNT_ZERO();
    rc = TA_CDLDOJI(0, g_nPoints - 1, g_open, g_high, g_low, g_close, &outBegIdx, &outNBElement, g_outIntBuf0);
    ICOUNT_DUMP("CDLDOJI/batch");
    icount_row(nm, "batch", 1, rc);
    acc += (double)g_outIntBuf0[0];

    ICOUNT_ZERO();
    rc = TA_CDLDOJI_OpenAndFill(&stf, g_open, g_high, g_low, g_close, g_nPoints, &outBegIdx, &outNBElement, g_outIntBuf0);
    ICOUNT_DUMP("CDLDOJI/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += (double)g_outIntBuf0[0];
    if( stf ) TA_CDLDOJI_Close(stf);

    ICOUNT_ZERO();
    rc = TA_CDLDOJI_Open(&st, g_open, g_high, g_low, g_close, g_nPoints, &iv0);
    ICOUNT_DUMP("CDLDOJI/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_CDLDOJI_Update(st, g_open[it & ICOUNT_MASK], g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &iv0);
            acc += (double)iv0;
        }
        ICOUNT_DUMP("CDLDOJI/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_CDLDOJI_Peek(st, g_open[it & ICOUNT_MASK], g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &iv0);
            acc += (double)iv0;
        }
        ICOUNT_DUMP("CDLDOJI/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_CDLDOJI_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_CDLDOJISTAR(int iters) {
    const char *nm = "CDLDOJISTAR";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_CDLDOJISTAR_Stream *st = NULL;
    TA_CDLDOJISTAR_Stream *stf = NULL;
    int iv0 = 0;

    ICOUNT_ZERO();
    rc = TA_CDLDOJISTAR(0, g_nPoints - 1, g_open, g_high, g_low, g_close, &outBegIdx, &outNBElement, g_outIntBuf0);
    ICOUNT_DUMP("CDLDOJISTAR/batch");
    icount_row(nm, "batch", 1, rc);
    acc += (double)g_outIntBuf0[0];

    ICOUNT_ZERO();
    rc = TA_CDLDOJISTAR_OpenAndFill(&stf, g_open, g_high, g_low, g_close, g_nPoints, &outBegIdx, &outNBElement, g_outIntBuf0);
    ICOUNT_DUMP("CDLDOJISTAR/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += (double)g_outIntBuf0[0];
    if( stf ) TA_CDLDOJISTAR_Close(stf);

    ICOUNT_ZERO();
    rc = TA_CDLDOJISTAR_Open(&st, g_open, g_high, g_low, g_close, g_nPoints, &iv0);
    ICOUNT_DUMP("CDLDOJISTAR/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_CDLDOJISTAR_Update(st, g_open[it & ICOUNT_MASK], g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &iv0);
            acc += (double)iv0;
        }
        ICOUNT_DUMP("CDLDOJISTAR/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_CDLDOJISTAR_Peek(st, g_open[it & ICOUNT_MASK], g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &iv0);
            acc += (double)iv0;
        }
        ICOUNT_DUMP("CDLDOJISTAR/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_CDLDOJISTAR_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_CDLDRAGONFLYDOJI(int iters) {
    const char *nm = "CDLDRAGONFLYDOJI";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_CDLDRAGONFLYDOJI_Stream *st = NULL;
    TA_CDLDRAGONFLYDOJI_Stream *stf = NULL;
    int iv0 = 0;

    ICOUNT_ZERO();
    rc = TA_CDLDRAGONFLYDOJI(0, g_nPoints - 1, g_open, g_high, g_low, g_close, &outBegIdx, &outNBElement, g_outIntBuf0);
    ICOUNT_DUMP("CDLDRAGONFLYDOJI/batch");
    icount_row(nm, "batch", 1, rc);
    acc += (double)g_outIntBuf0[0];

    ICOUNT_ZERO();
    rc = TA_CDLDRAGONFLYDOJI_OpenAndFill(&stf, g_open, g_high, g_low, g_close, g_nPoints, &outBegIdx, &outNBElement, g_outIntBuf0);
    ICOUNT_DUMP("CDLDRAGONFLYDOJI/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += (double)g_outIntBuf0[0];
    if( stf ) TA_CDLDRAGONFLYDOJI_Close(stf);

    ICOUNT_ZERO();
    rc = TA_CDLDRAGONFLYDOJI_Open(&st, g_open, g_high, g_low, g_close, g_nPoints, &iv0);
    ICOUNT_DUMP("CDLDRAGONFLYDOJI/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_CDLDRAGONFLYDOJI_Update(st, g_open[it & ICOUNT_MASK], g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &iv0);
            acc += (double)iv0;
        }
        ICOUNT_DUMP("CDLDRAGONFLYDOJI/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_CDLDRAGONFLYDOJI_Peek(st, g_open[it & ICOUNT_MASK], g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &iv0);
            acc += (double)iv0;
        }
        ICOUNT_DUMP("CDLDRAGONFLYDOJI/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_CDLDRAGONFLYDOJI_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_CDLENGULFING(int iters) {
    const char *nm = "CDLENGULFING";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_CDLENGULFING_Stream *st = NULL;
    TA_CDLENGULFING_Stream *stf = NULL;
    int iv0 = 0;

    ICOUNT_ZERO();
    rc = TA_CDLENGULFING(0, g_nPoints - 1, g_open, g_high, g_low, g_close, &outBegIdx, &outNBElement, g_outIntBuf0);
    ICOUNT_DUMP("CDLENGULFING/batch");
    icount_row(nm, "batch", 1, rc);
    acc += (double)g_outIntBuf0[0];

    ICOUNT_ZERO();
    rc = TA_CDLENGULFING_OpenAndFill(&stf, g_open, g_high, g_low, g_close, g_nPoints, &outBegIdx, &outNBElement, g_outIntBuf0);
    ICOUNT_DUMP("CDLENGULFING/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += (double)g_outIntBuf0[0];
    if( stf ) TA_CDLENGULFING_Close(stf);

    ICOUNT_ZERO();
    rc = TA_CDLENGULFING_Open(&st, g_open, g_high, g_low, g_close, g_nPoints, &iv0);
    ICOUNT_DUMP("CDLENGULFING/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_CDLENGULFING_Update(st, g_open[it & ICOUNT_MASK], g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &iv0);
            acc += (double)iv0;
        }
        ICOUNT_DUMP("CDLENGULFING/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_CDLENGULFING_Peek(st, g_open[it & ICOUNT_MASK], g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &iv0);
            acc += (double)iv0;
        }
        ICOUNT_DUMP("CDLENGULFING/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_CDLENGULFING_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_CDLEVENINGDOJISTAR(int iters) {
    const char *nm = "CDLEVENINGDOJISTAR";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_CDLEVENINGDOJISTAR_Stream *st = NULL;
    TA_CDLEVENINGDOJISTAR_Stream *stf = NULL;
    int iv0 = 0;

    ICOUNT_ZERO();
    rc = TA_CDLEVENINGDOJISTAR(0, g_nPoints - 1, g_open, g_high, g_low, g_close, 0.300000000000000, &outBegIdx, &outNBElement, g_outIntBuf0);
    ICOUNT_DUMP("CDLEVENINGDOJISTAR/batch");
    icount_row(nm, "batch", 1, rc);
    acc += (double)g_outIntBuf0[0];

    ICOUNT_ZERO();
    rc = TA_CDLEVENINGDOJISTAR_OpenAndFill(&stf, g_open, g_high, g_low, g_close, g_nPoints, 0.300000000000000, &outBegIdx, &outNBElement, g_outIntBuf0);
    ICOUNT_DUMP("CDLEVENINGDOJISTAR/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += (double)g_outIntBuf0[0];
    if( stf ) TA_CDLEVENINGDOJISTAR_Close(stf);

    ICOUNT_ZERO();
    rc = TA_CDLEVENINGDOJISTAR_Open(&st, g_open, g_high, g_low, g_close, g_nPoints, 0.300000000000000, &iv0);
    ICOUNT_DUMP("CDLEVENINGDOJISTAR/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_CDLEVENINGDOJISTAR_Update(st, g_open[it & ICOUNT_MASK], g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &iv0);
            acc += (double)iv0;
        }
        ICOUNT_DUMP("CDLEVENINGDOJISTAR/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_CDLEVENINGDOJISTAR_Peek(st, g_open[it & ICOUNT_MASK], g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &iv0);
            acc += (double)iv0;
        }
        ICOUNT_DUMP("CDLEVENINGDOJISTAR/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_CDLEVENINGDOJISTAR_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_CDLEVENINGSTAR(int iters) {
    const char *nm = "CDLEVENINGSTAR";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_CDLEVENINGSTAR_Stream *st = NULL;
    TA_CDLEVENINGSTAR_Stream *stf = NULL;
    int iv0 = 0;

    ICOUNT_ZERO();
    rc = TA_CDLEVENINGSTAR(0, g_nPoints - 1, g_open, g_high, g_low, g_close, 0.300000000000000, &outBegIdx, &outNBElement, g_outIntBuf0);
    ICOUNT_DUMP("CDLEVENINGSTAR/batch");
    icount_row(nm, "batch", 1, rc);
    acc += (double)g_outIntBuf0[0];

    ICOUNT_ZERO();
    rc = TA_CDLEVENINGSTAR_OpenAndFill(&stf, g_open, g_high, g_low, g_close, g_nPoints, 0.300000000000000, &outBegIdx, &outNBElement, g_outIntBuf0);
    ICOUNT_DUMP("CDLEVENINGSTAR/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += (double)g_outIntBuf0[0];
    if( stf ) TA_CDLEVENINGSTAR_Close(stf);

    ICOUNT_ZERO();
    rc = TA_CDLEVENINGSTAR_Open(&st, g_open, g_high, g_low, g_close, g_nPoints, 0.300000000000000, &iv0);
    ICOUNT_DUMP("CDLEVENINGSTAR/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_CDLEVENINGSTAR_Update(st, g_open[it & ICOUNT_MASK], g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &iv0);
            acc += (double)iv0;
        }
        ICOUNT_DUMP("CDLEVENINGSTAR/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_CDLEVENINGSTAR_Peek(st, g_open[it & ICOUNT_MASK], g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &iv0);
            acc += (double)iv0;
        }
        ICOUNT_DUMP("CDLEVENINGSTAR/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_CDLEVENINGSTAR_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_CDLGAPSIDESIDEWHITE(int iters) {
    const char *nm = "CDLGAPSIDESIDEWHITE";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_CDLGAPSIDESIDEWHITE_Stream *st = NULL;
    TA_CDLGAPSIDESIDEWHITE_Stream *stf = NULL;
    int iv0 = 0;

    ICOUNT_ZERO();
    rc = TA_CDLGAPSIDESIDEWHITE(0, g_nPoints - 1, g_open, g_high, g_low, g_close, &outBegIdx, &outNBElement, g_outIntBuf0);
    ICOUNT_DUMP("CDLGAPSIDESIDEWHITE/batch");
    icount_row(nm, "batch", 1, rc);
    acc += (double)g_outIntBuf0[0];

    ICOUNT_ZERO();
    rc = TA_CDLGAPSIDESIDEWHITE_OpenAndFill(&stf, g_open, g_high, g_low, g_close, g_nPoints, &outBegIdx, &outNBElement, g_outIntBuf0);
    ICOUNT_DUMP("CDLGAPSIDESIDEWHITE/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += (double)g_outIntBuf0[0];
    if( stf ) TA_CDLGAPSIDESIDEWHITE_Close(stf);

    ICOUNT_ZERO();
    rc = TA_CDLGAPSIDESIDEWHITE_Open(&st, g_open, g_high, g_low, g_close, g_nPoints, &iv0);
    ICOUNT_DUMP("CDLGAPSIDESIDEWHITE/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_CDLGAPSIDESIDEWHITE_Update(st, g_open[it & ICOUNT_MASK], g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &iv0);
            acc += (double)iv0;
        }
        ICOUNT_DUMP("CDLGAPSIDESIDEWHITE/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_CDLGAPSIDESIDEWHITE_Peek(st, g_open[it & ICOUNT_MASK], g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &iv0);
            acc += (double)iv0;
        }
        ICOUNT_DUMP("CDLGAPSIDESIDEWHITE/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_CDLGAPSIDESIDEWHITE_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_CDLGRAVESTONEDOJI(int iters) {
    const char *nm = "CDLGRAVESTONEDOJI";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_CDLGRAVESTONEDOJI_Stream *st = NULL;
    TA_CDLGRAVESTONEDOJI_Stream *stf = NULL;
    int iv0 = 0;

    ICOUNT_ZERO();
    rc = TA_CDLGRAVESTONEDOJI(0, g_nPoints - 1, g_open, g_high, g_low, g_close, &outBegIdx, &outNBElement, g_outIntBuf0);
    ICOUNT_DUMP("CDLGRAVESTONEDOJI/batch");
    icount_row(nm, "batch", 1, rc);
    acc += (double)g_outIntBuf0[0];

    ICOUNT_ZERO();
    rc = TA_CDLGRAVESTONEDOJI_OpenAndFill(&stf, g_open, g_high, g_low, g_close, g_nPoints, &outBegIdx, &outNBElement, g_outIntBuf0);
    ICOUNT_DUMP("CDLGRAVESTONEDOJI/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += (double)g_outIntBuf0[0];
    if( stf ) TA_CDLGRAVESTONEDOJI_Close(stf);

    ICOUNT_ZERO();
    rc = TA_CDLGRAVESTONEDOJI_Open(&st, g_open, g_high, g_low, g_close, g_nPoints, &iv0);
    ICOUNT_DUMP("CDLGRAVESTONEDOJI/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_CDLGRAVESTONEDOJI_Update(st, g_open[it & ICOUNT_MASK], g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &iv0);
            acc += (double)iv0;
        }
        ICOUNT_DUMP("CDLGRAVESTONEDOJI/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_CDLGRAVESTONEDOJI_Peek(st, g_open[it & ICOUNT_MASK], g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &iv0);
            acc += (double)iv0;
        }
        ICOUNT_DUMP("CDLGRAVESTONEDOJI/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_CDLGRAVESTONEDOJI_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_CDLHAMMER(int iters) {
    const char *nm = "CDLHAMMER";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_CDLHAMMER_Stream *st = NULL;
    TA_CDLHAMMER_Stream *stf = NULL;
    int iv0 = 0;

    ICOUNT_ZERO();
    rc = TA_CDLHAMMER(0, g_nPoints - 1, g_open, g_high, g_low, g_close, &outBegIdx, &outNBElement, g_outIntBuf0);
    ICOUNT_DUMP("CDLHAMMER/batch");
    icount_row(nm, "batch", 1, rc);
    acc += (double)g_outIntBuf0[0];

    ICOUNT_ZERO();
    rc = TA_CDLHAMMER_OpenAndFill(&stf, g_open, g_high, g_low, g_close, g_nPoints, &outBegIdx, &outNBElement, g_outIntBuf0);
    ICOUNT_DUMP("CDLHAMMER/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += (double)g_outIntBuf0[0];
    if( stf ) TA_CDLHAMMER_Close(stf);

    ICOUNT_ZERO();
    rc = TA_CDLHAMMER_Open(&st, g_open, g_high, g_low, g_close, g_nPoints, &iv0);
    ICOUNT_DUMP("CDLHAMMER/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_CDLHAMMER_Update(st, g_open[it & ICOUNT_MASK], g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &iv0);
            acc += (double)iv0;
        }
        ICOUNT_DUMP("CDLHAMMER/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_CDLHAMMER_Peek(st, g_open[it & ICOUNT_MASK], g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &iv0);
            acc += (double)iv0;
        }
        ICOUNT_DUMP("CDLHAMMER/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_CDLHAMMER_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_CDLHANGINGMAN(int iters) {
    const char *nm = "CDLHANGINGMAN";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_CDLHANGINGMAN_Stream *st = NULL;
    TA_CDLHANGINGMAN_Stream *stf = NULL;
    int iv0 = 0;

    ICOUNT_ZERO();
    rc = TA_CDLHANGINGMAN(0, g_nPoints - 1, g_open, g_high, g_low, g_close, &outBegIdx, &outNBElement, g_outIntBuf0);
    ICOUNT_DUMP("CDLHANGINGMAN/batch");
    icount_row(nm, "batch", 1, rc);
    acc += (double)g_outIntBuf0[0];

    ICOUNT_ZERO();
    rc = TA_CDLHANGINGMAN_OpenAndFill(&stf, g_open, g_high, g_low, g_close, g_nPoints, &outBegIdx, &outNBElement, g_outIntBuf0);
    ICOUNT_DUMP("CDLHANGINGMAN/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += (double)g_outIntBuf0[0];
    if( stf ) TA_CDLHANGINGMAN_Close(stf);

    ICOUNT_ZERO();
    rc = TA_CDLHANGINGMAN_Open(&st, g_open, g_high, g_low, g_close, g_nPoints, &iv0);
    ICOUNT_DUMP("CDLHANGINGMAN/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_CDLHANGINGMAN_Update(st, g_open[it & ICOUNT_MASK], g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &iv0);
            acc += (double)iv0;
        }
        ICOUNT_DUMP("CDLHANGINGMAN/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_CDLHANGINGMAN_Peek(st, g_open[it & ICOUNT_MASK], g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &iv0);
            acc += (double)iv0;
        }
        ICOUNT_DUMP("CDLHANGINGMAN/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_CDLHANGINGMAN_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_CDLHARAMI(int iters) {
    const char *nm = "CDLHARAMI";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_CDLHARAMI_Stream *st = NULL;
    TA_CDLHARAMI_Stream *stf = NULL;
    int iv0 = 0;

    ICOUNT_ZERO();
    rc = TA_CDLHARAMI(0, g_nPoints - 1, g_open, g_high, g_low, g_close, &outBegIdx, &outNBElement, g_outIntBuf0);
    ICOUNT_DUMP("CDLHARAMI/batch");
    icount_row(nm, "batch", 1, rc);
    acc += (double)g_outIntBuf0[0];

    ICOUNT_ZERO();
    rc = TA_CDLHARAMI_OpenAndFill(&stf, g_open, g_high, g_low, g_close, g_nPoints, &outBegIdx, &outNBElement, g_outIntBuf0);
    ICOUNT_DUMP("CDLHARAMI/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += (double)g_outIntBuf0[0];
    if( stf ) TA_CDLHARAMI_Close(stf);

    ICOUNT_ZERO();
    rc = TA_CDLHARAMI_Open(&st, g_open, g_high, g_low, g_close, g_nPoints, &iv0);
    ICOUNT_DUMP("CDLHARAMI/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_CDLHARAMI_Update(st, g_open[it & ICOUNT_MASK], g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &iv0);
            acc += (double)iv0;
        }
        ICOUNT_DUMP("CDLHARAMI/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_CDLHARAMI_Peek(st, g_open[it & ICOUNT_MASK], g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &iv0);
            acc += (double)iv0;
        }
        ICOUNT_DUMP("CDLHARAMI/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_CDLHARAMI_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_CDLHARAMICROSS(int iters) {
    const char *nm = "CDLHARAMICROSS";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_CDLHARAMICROSS_Stream *st = NULL;
    TA_CDLHARAMICROSS_Stream *stf = NULL;
    int iv0 = 0;

    ICOUNT_ZERO();
    rc = TA_CDLHARAMICROSS(0, g_nPoints - 1, g_open, g_high, g_low, g_close, &outBegIdx, &outNBElement, g_outIntBuf0);
    ICOUNT_DUMP("CDLHARAMICROSS/batch");
    icount_row(nm, "batch", 1, rc);
    acc += (double)g_outIntBuf0[0];

    ICOUNT_ZERO();
    rc = TA_CDLHARAMICROSS_OpenAndFill(&stf, g_open, g_high, g_low, g_close, g_nPoints, &outBegIdx, &outNBElement, g_outIntBuf0);
    ICOUNT_DUMP("CDLHARAMICROSS/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += (double)g_outIntBuf0[0];
    if( stf ) TA_CDLHARAMICROSS_Close(stf);

    ICOUNT_ZERO();
    rc = TA_CDLHARAMICROSS_Open(&st, g_open, g_high, g_low, g_close, g_nPoints, &iv0);
    ICOUNT_DUMP("CDLHARAMICROSS/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_CDLHARAMICROSS_Update(st, g_open[it & ICOUNT_MASK], g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &iv0);
            acc += (double)iv0;
        }
        ICOUNT_DUMP("CDLHARAMICROSS/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_CDLHARAMICROSS_Peek(st, g_open[it & ICOUNT_MASK], g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &iv0);
            acc += (double)iv0;
        }
        ICOUNT_DUMP("CDLHARAMICROSS/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_CDLHARAMICROSS_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_CDLHIGHWAVE(int iters) {
    const char *nm = "CDLHIGHWAVE";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_CDLHIGHWAVE_Stream *st = NULL;
    TA_CDLHIGHWAVE_Stream *stf = NULL;
    int iv0 = 0;

    ICOUNT_ZERO();
    rc = TA_CDLHIGHWAVE(0, g_nPoints - 1, g_open, g_high, g_low, g_close, &outBegIdx, &outNBElement, g_outIntBuf0);
    ICOUNT_DUMP("CDLHIGHWAVE/batch");
    icount_row(nm, "batch", 1, rc);
    acc += (double)g_outIntBuf0[0];

    ICOUNT_ZERO();
    rc = TA_CDLHIGHWAVE_OpenAndFill(&stf, g_open, g_high, g_low, g_close, g_nPoints, &outBegIdx, &outNBElement, g_outIntBuf0);
    ICOUNT_DUMP("CDLHIGHWAVE/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += (double)g_outIntBuf0[0];
    if( stf ) TA_CDLHIGHWAVE_Close(stf);

    ICOUNT_ZERO();
    rc = TA_CDLHIGHWAVE_Open(&st, g_open, g_high, g_low, g_close, g_nPoints, &iv0);
    ICOUNT_DUMP("CDLHIGHWAVE/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_CDLHIGHWAVE_Update(st, g_open[it & ICOUNT_MASK], g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &iv0);
            acc += (double)iv0;
        }
        ICOUNT_DUMP("CDLHIGHWAVE/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_CDLHIGHWAVE_Peek(st, g_open[it & ICOUNT_MASK], g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &iv0);
            acc += (double)iv0;
        }
        ICOUNT_DUMP("CDLHIGHWAVE/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_CDLHIGHWAVE_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_CDLHIKKAKE(int iters) {
    const char *nm = "CDLHIKKAKE";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_CDLHIKKAKE_Stream *st = NULL;
    TA_CDLHIKKAKE_Stream *stf = NULL;
    int iv0 = 0;

    ICOUNT_ZERO();
    rc = TA_CDLHIKKAKE(0, g_nPoints - 1, g_open, g_high, g_low, g_close, &outBegIdx, &outNBElement, g_outIntBuf0);
    ICOUNT_DUMP("CDLHIKKAKE/batch");
    icount_row(nm, "batch", 1, rc);
    acc += (double)g_outIntBuf0[0];

    ICOUNT_ZERO();
    rc = TA_CDLHIKKAKE_OpenAndFill(&stf, g_open, g_high, g_low, g_close, g_nPoints, &outBegIdx, &outNBElement, g_outIntBuf0);
    ICOUNT_DUMP("CDLHIKKAKE/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += (double)g_outIntBuf0[0];
    if( stf ) TA_CDLHIKKAKE_Close(stf);

    ICOUNT_ZERO();
    rc = TA_CDLHIKKAKE_Open(&st, g_open, g_high, g_low, g_close, g_nPoints, &iv0);
    ICOUNT_DUMP("CDLHIKKAKE/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_CDLHIKKAKE_Update(st, g_open[it & ICOUNT_MASK], g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &iv0);
            acc += (double)iv0;
        }
        ICOUNT_DUMP("CDLHIKKAKE/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_CDLHIKKAKE_Peek(st, g_open[it & ICOUNT_MASK], g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &iv0);
            acc += (double)iv0;
        }
        ICOUNT_DUMP("CDLHIKKAKE/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_CDLHIKKAKE_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_CDLHIKKAKEMOD(int iters) {
    const char *nm = "CDLHIKKAKEMOD";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_CDLHIKKAKEMOD_Stream *st = NULL;
    TA_CDLHIKKAKEMOD_Stream *stf = NULL;
    int iv0 = 0;

    ICOUNT_ZERO();
    rc = TA_CDLHIKKAKEMOD(0, g_nPoints - 1, g_open, g_high, g_low, g_close, &outBegIdx, &outNBElement, g_outIntBuf0);
    ICOUNT_DUMP("CDLHIKKAKEMOD/batch");
    icount_row(nm, "batch", 1, rc);
    acc += (double)g_outIntBuf0[0];

    ICOUNT_ZERO();
    rc = TA_CDLHIKKAKEMOD_OpenAndFill(&stf, g_open, g_high, g_low, g_close, g_nPoints, &outBegIdx, &outNBElement, g_outIntBuf0);
    ICOUNT_DUMP("CDLHIKKAKEMOD/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += (double)g_outIntBuf0[0];
    if( stf ) TA_CDLHIKKAKEMOD_Close(stf);

    ICOUNT_ZERO();
    rc = TA_CDLHIKKAKEMOD_Open(&st, g_open, g_high, g_low, g_close, g_nPoints, &iv0);
    ICOUNT_DUMP("CDLHIKKAKEMOD/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_CDLHIKKAKEMOD_Update(st, g_open[it & ICOUNT_MASK], g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &iv0);
            acc += (double)iv0;
        }
        ICOUNT_DUMP("CDLHIKKAKEMOD/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_CDLHIKKAKEMOD_Peek(st, g_open[it & ICOUNT_MASK], g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &iv0);
            acc += (double)iv0;
        }
        ICOUNT_DUMP("CDLHIKKAKEMOD/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_CDLHIKKAKEMOD_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_CDLHOMINGPIGEON(int iters) {
    const char *nm = "CDLHOMINGPIGEON";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_CDLHOMINGPIGEON_Stream *st = NULL;
    TA_CDLHOMINGPIGEON_Stream *stf = NULL;
    int iv0 = 0;

    ICOUNT_ZERO();
    rc = TA_CDLHOMINGPIGEON(0, g_nPoints - 1, g_open, g_high, g_low, g_close, &outBegIdx, &outNBElement, g_outIntBuf0);
    ICOUNT_DUMP("CDLHOMINGPIGEON/batch");
    icount_row(nm, "batch", 1, rc);
    acc += (double)g_outIntBuf0[0];

    ICOUNT_ZERO();
    rc = TA_CDLHOMINGPIGEON_OpenAndFill(&stf, g_open, g_high, g_low, g_close, g_nPoints, &outBegIdx, &outNBElement, g_outIntBuf0);
    ICOUNT_DUMP("CDLHOMINGPIGEON/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += (double)g_outIntBuf0[0];
    if( stf ) TA_CDLHOMINGPIGEON_Close(stf);

    ICOUNT_ZERO();
    rc = TA_CDLHOMINGPIGEON_Open(&st, g_open, g_high, g_low, g_close, g_nPoints, &iv0);
    ICOUNT_DUMP("CDLHOMINGPIGEON/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_CDLHOMINGPIGEON_Update(st, g_open[it & ICOUNT_MASK], g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &iv0);
            acc += (double)iv0;
        }
        ICOUNT_DUMP("CDLHOMINGPIGEON/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_CDLHOMINGPIGEON_Peek(st, g_open[it & ICOUNT_MASK], g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &iv0);
            acc += (double)iv0;
        }
        ICOUNT_DUMP("CDLHOMINGPIGEON/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_CDLHOMINGPIGEON_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_CDLIDENTICAL3CROWS(int iters) {
    const char *nm = "CDLIDENTICAL3CROWS";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_CDLIDENTICAL3CROWS_Stream *st = NULL;
    TA_CDLIDENTICAL3CROWS_Stream *stf = NULL;
    int iv0 = 0;

    ICOUNT_ZERO();
    rc = TA_CDLIDENTICAL3CROWS(0, g_nPoints - 1, g_open, g_high, g_low, g_close, &outBegIdx, &outNBElement, g_outIntBuf0);
    ICOUNT_DUMP("CDLIDENTICAL3CROWS/batch");
    icount_row(nm, "batch", 1, rc);
    acc += (double)g_outIntBuf0[0];

    ICOUNT_ZERO();
    rc = TA_CDLIDENTICAL3CROWS_OpenAndFill(&stf, g_open, g_high, g_low, g_close, g_nPoints, &outBegIdx, &outNBElement, g_outIntBuf0);
    ICOUNT_DUMP("CDLIDENTICAL3CROWS/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += (double)g_outIntBuf0[0];
    if( stf ) TA_CDLIDENTICAL3CROWS_Close(stf);

    ICOUNT_ZERO();
    rc = TA_CDLIDENTICAL3CROWS_Open(&st, g_open, g_high, g_low, g_close, g_nPoints, &iv0);
    ICOUNT_DUMP("CDLIDENTICAL3CROWS/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_CDLIDENTICAL3CROWS_Update(st, g_open[it & ICOUNT_MASK], g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &iv0);
            acc += (double)iv0;
        }
        ICOUNT_DUMP("CDLIDENTICAL3CROWS/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_CDLIDENTICAL3CROWS_Peek(st, g_open[it & ICOUNT_MASK], g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &iv0);
            acc += (double)iv0;
        }
        ICOUNT_DUMP("CDLIDENTICAL3CROWS/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_CDLIDENTICAL3CROWS_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_CDLINNECK(int iters) {
    const char *nm = "CDLINNECK";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_CDLINNECK_Stream *st = NULL;
    TA_CDLINNECK_Stream *stf = NULL;
    int iv0 = 0;

    ICOUNT_ZERO();
    rc = TA_CDLINNECK(0, g_nPoints - 1, g_open, g_high, g_low, g_close, &outBegIdx, &outNBElement, g_outIntBuf0);
    ICOUNT_DUMP("CDLINNECK/batch");
    icount_row(nm, "batch", 1, rc);
    acc += (double)g_outIntBuf0[0];

    ICOUNT_ZERO();
    rc = TA_CDLINNECK_OpenAndFill(&stf, g_open, g_high, g_low, g_close, g_nPoints, &outBegIdx, &outNBElement, g_outIntBuf0);
    ICOUNT_DUMP("CDLINNECK/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += (double)g_outIntBuf0[0];
    if( stf ) TA_CDLINNECK_Close(stf);

    ICOUNT_ZERO();
    rc = TA_CDLINNECK_Open(&st, g_open, g_high, g_low, g_close, g_nPoints, &iv0);
    ICOUNT_DUMP("CDLINNECK/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_CDLINNECK_Update(st, g_open[it & ICOUNT_MASK], g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &iv0);
            acc += (double)iv0;
        }
        ICOUNT_DUMP("CDLINNECK/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_CDLINNECK_Peek(st, g_open[it & ICOUNT_MASK], g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &iv0);
            acc += (double)iv0;
        }
        ICOUNT_DUMP("CDLINNECK/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_CDLINNECK_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_CDLINVERTEDHAMMER(int iters) {
    const char *nm = "CDLINVERTEDHAMMER";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_CDLINVERTEDHAMMER_Stream *st = NULL;
    TA_CDLINVERTEDHAMMER_Stream *stf = NULL;
    int iv0 = 0;

    ICOUNT_ZERO();
    rc = TA_CDLINVERTEDHAMMER(0, g_nPoints - 1, g_open, g_high, g_low, g_close, &outBegIdx, &outNBElement, g_outIntBuf0);
    ICOUNT_DUMP("CDLINVERTEDHAMMER/batch");
    icount_row(nm, "batch", 1, rc);
    acc += (double)g_outIntBuf0[0];

    ICOUNT_ZERO();
    rc = TA_CDLINVERTEDHAMMER_OpenAndFill(&stf, g_open, g_high, g_low, g_close, g_nPoints, &outBegIdx, &outNBElement, g_outIntBuf0);
    ICOUNT_DUMP("CDLINVERTEDHAMMER/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += (double)g_outIntBuf0[0];
    if( stf ) TA_CDLINVERTEDHAMMER_Close(stf);

    ICOUNT_ZERO();
    rc = TA_CDLINVERTEDHAMMER_Open(&st, g_open, g_high, g_low, g_close, g_nPoints, &iv0);
    ICOUNT_DUMP("CDLINVERTEDHAMMER/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_CDLINVERTEDHAMMER_Update(st, g_open[it & ICOUNT_MASK], g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &iv0);
            acc += (double)iv0;
        }
        ICOUNT_DUMP("CDLINVERTEDHAMMER/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_CDLINVERTEDHAMMER_Peek(st, g_open[it & ICOUNT_MASK], g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &iv0);
            acc += (double)iv0;
        }
        ICOUNT_DUMP("CDLINVERTEDHAMMER/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_CDLINVERTEDHAMMER_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_CDLKICKING(int iters) {
    const char *nm = "CDLKICKING";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_CDLKICKING_Stream *st = NULL;
    TA_CDLKICKING_Stream *stf = NULL;
    int iv0 = 0;

    ICOUNT_ZERO();
    rc = TA_CDLKICKING(0, g_nPoints - 1, g_open, g_high, g_low, g_close, &outBegIdx, &outNBElement, g_outIntBuf0);
    ICOUNT_DUMP("CDLKICKING/batch");
    icount_row(nm, "batch", 1, rc);
    acc += (double)g_outIntBuf0[0];

    ICOUNT_ZERO();
    rc = TA_CDLKICKING_OpenAndFill(&stf, g_open, g_high, g_low, g_close, g_nPoints, &outBegIdx, &outNBElement, g_outIntBuf0);
    ICOUNT_DUMP("CDLKICKING/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += (double)g_outIntBuf0[0];
    if( stf ) TA_CDLKICKING_Close(stf);

    ICOUNT_ZERO();
    rc = TA_CDLKICKING_Open(&st, g_open, g_high, g_low, g_close, g_nPoints, &iv0);
    ICOUNT_DUMP("CDLKICKING/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_CDLKICKING_Update(st, g_open[it & ICOUNT_MASK], g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &iv0);
            acc += (double)iv0;
        }
        ICOUNT_DUMP("CDLKICKING/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_CDLKICKING_Peek(st, g_open[it & ICOUNT_MASK], g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &iv0);
            acc += (double)iv0;
        }
        ICOUNT_DUMP("CDLKICKING/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_CDLKICKING_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_CDLKICKINGBYLENGTH(int iters) {
    const char *nm = "CDLKICKINGBYLENGTH";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_CDLKICKINGBYLENGTH_Stream *st = NULL;
    TA_CDLKICKINGBYLENGTH_Stream *stf = NULL;
    int iv0 = 0;

    ICOUNT_ZERO();
    rc = TA_CDLKICKINGBYLENGTH(0, g_nPoints - 1, g_open, g_high, g_low, g_close, &outBegIdx, &outNBElement, g_outIntBuf0);
    ICOUNT_DUMP("CDLKICKINGBYLENGTH/batch");
    icount_row(nm, "batch", 1, rc);
    acc += (double)g_outIntBuf0[0];

    ICOUNT_ZERO();
    rc = TA_CDLKICKINGBYLENGTH_OpenAndFill(&stf, g_open, g_high, g_low, g_close, g_nPoints, &outBegIdx, &outNBElement, g_outIntBuf0);
    ICOUNT_DUMP("CDLKICKINGBYLENGTH/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += (double)g_outIntBuf0[0];
    if( stf ) TA_CDLKICKINGBYLENGTH_Close(stf);

    ICOUNT_ZERO();
    rc = TA_CDLKICKINGBYLENGTH_Open(&st, g_open, g_high, g_low, g_close, g_nPoints, &iv0);
    ICOUNT_DUMP("CDLKICKINGBYLENGTH/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_CDLKICKINGBYLENGTH_Update(st, g_open[it & ICOUNT_MASK], g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &iv0);
            acc += (double)iv0;
        }
        ICOUNT_DUMP("CDLKICKINGBYLENGTH/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_CDLKICKINGBYLENGTH_Peek(st, g_open[it & ICOUNT_MASK], g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &iv0);
            acc += (double)iv0;
        }
        ICOUNT_DUMP("CDLKICKINGBYLENGTH/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_CDLKICKINGBYLENGTH_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_CDLLADDERBOTTOM(int iters) {
    const char *nm = "CDLLADDERBOTTOM";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_CDLLADDERBOTTOM_Stream *st = NULL;
    TA_CDLLADDERBOTTOM_Stream *stf = NULL;
    int iv0 = 0;

    ICOUNT_ZERO();
    rc = TA_CDLLADDERBOTTOM(0, g_nPoints - 1, g_open, g_high, g_low, g_close, &outBegIdx, &outNBElement, g_outIntBuf0);
    ICOUNT_DUMP("CDLLADDERBOTTOM/batch");
    icount_row(nm, "batch", 1, rc);
    acc += (double)g_outIntBuf0[0];

    ICOUNT_ZERO();
    rc = TA_CDLLADDERBOTTOM_OpenAndFill(&stf, g_open, g_high, g_low, g_close, g_nPoints, &outBegIdx, &outNBElement, g_outIntBuf0);
    ICOUNT_DUMP("CDLLADDERBOTTOM/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += (double)g_outIntBuf0[0];
    if( stf ) TA_CDLLADDERBOTTOM_Close(stf);

    ICOUNT_ZERO();
    rc = TA_CDLLADDERBOTTOM_Open(&st, g_open, g_high, g_low, g_close, g_nPoints, &iv0);
    ICOUNT_DUMP("CDLLADDERBOTTOM/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_CDLLADDERBOTTOM_Update(st, g_open[it & ICOUNT_MASK], g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &iv0);
            acc += (double)iv0;
        }
        ICOUNT_DUMP("CDLLADDERBOTTOM/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_CDLLADDERBOTTOM_Peek(st, g_open[it & ICOUNT_MASK], g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &iv0);
            acc += (double)iv0;
        }
        ICOUNT_DUMP("CDLLADDERBOTTOM/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_CDLLADDERBOTTOM_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_CDLLONGLEGGEDDOJI(int iters) {
    const char *nm = "CDLLONGLEGGEDDOJI";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_CDLLONGLEGGEDDOJI_Stream *st = NULL;
    TA_CDLLONGLEGGEDDOJI_Stream *stf = NULL;
    int iv0 = 0;

    ICOUNT_ZERO();
    rc = TA_CDLLONGLEGGEDDOJI(0, g_nPoints - 1, g_open, g_high, g_low, g_close, &outBegIdx, &outNBElement, g_outIntBuf0);
    ICOUNT_DUMP("CDLLONGLEGGEDDOJI/batch");
    icount_row(nm, "batch", 1, rc);
    acc += (double)g_outIntBuf0[0];

    ICOUNT_ZERO();
    rc = TA_CDLLONGLEGGEDDOJI_OpenAndFill(&stf, g_open, g_high, g_low, g_close, g_nPoints, &outBegIdx, &outNBElement, g_outIntBuf0);
    ICOUNT_DUMP("CDLLONGLEGGEDDOJI/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += (double)g_outIntBuf0[0];
    if( stf ) TA_CDLLONGLEGGEDDOJI_Close(stf);

    ICOUNT_ZERO();
    rc = TA_CDLLONGLEGGEDDOJI_Open(&st, g_open, g_high, g_low, g_close, g_nPoints, &iv0);
    ICOUNT_DUMP("CDLLONGLEGGEDDOJI/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_CDLLONGLEGGEDDOJI_Update(st, g_open[it & ICOUNT_MASK], g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &iv0);
            acc += (double)iv0;
        }
        ICOUNT_DUMP("CDLLONGLEGGEDDOJI/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_CDLLONGLEGGEDDOJI_Peek(st, g_open[it & ICOUNT_MASK], g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &iv0);
            acc += (double)iv0;
        }
        ICOUNT_DUMP("CDLLONGLEGGEDDOJI/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_CDLLONGLEGGEDDOJI_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_CDLLONGLINE(int iters) {
    const char *nm = "CDLLONGLINE";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_CDLLONGLINE_Stream *st = NULL;
    TA_CDLLONGLINE_Stream *stf = NULL;
    int iv0 = 0;

    ICOUNT_ZERO();
    rc = TA_CDLLONGLINE(0, g_nPoints - 1, g_open, g_high, g_low, g_close, &outBegIdx, &outNBElement, g_outIntBuf0);
    ICOUNT_DUMP("CDLLONGLINE/batch");
    icount_row(nm, "batch", 1, rc);
    acc += (double)g_outIntBuf0[0];

    ICOUNT_ZERO();
    rc = TA_CDLLONGLINE_OpenAndFill(&stf, g_open, g_high, g_low, g_close, g_nPoints, &outBegIdx, &outNBElement, g_outIntBuf0);
    ICOUNT_DUMP("CDLLONGLINE/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += (double)g_outIntBuf0[0];
    if( stf ) TA_CDLLONGLINE_Close(stf);

    ICOUNT_ZERO();
    rc = TA_CDLLONGLINE_Open(&st, g_open, g_high, g_low, g_close, g_nPoints, &iv0);
    ICOUNT_DUMP("CDLLONGLINE/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_CDLLONGLINE_Update(st, g_open[it & ICOUNT_MASK], g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &iv0);
            acc += (double)iv0;
        }
        ICOUNT_DUMP("CDLLONGLINE/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_CDLLONGLINE_Peek(st, g_open[it & ICOUNT_MASK], g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &iv0);
            acc += (double)iv0;
        }
        ICOUNT_DUMP("CDLLONGLINE/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_CDLLONGLINE_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_CDLMARUBOZU(int iters) {
    const char *nm = "CDLMARUBOZU";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_CDLMARUBOZU_Stream *st = NULL;
    TA_CDLMARUBOZU_Stream *stf = NULL;
    int iv0 = 0;

    ICOUNT_ZERO();
    rc = TA_CDLMARUBOZU(0, g_nPoints - 1, g_open, g_high, g_low, g_close, &outBegIdx, &outNBElement, g_outIntBuf0);
    ICOUNT_DUMP("CDLMARUBOZU/batch");
    icount_row(nm, "batch", 1, rc);
    acc += (double)g_outIntBuf0[0];

    ICOUNT_ZERO();
    rc = TA_CDLMARUBOZU_OpenAndFill(&stf, g_open, g_high, g_low, g_close, g_nPoints, &outBegIdx, &outNBElement, g_outIntBuf0);
    ICOUNT_DUMP("CDLMARUBOZU/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += (double)g_outIntBuf0[0];
    if( stf ) TA_CDLMARUBOZU_Close(stf);

    ICOUNT_ZERO();
    rc = TA_CDLMARUBOZU_Open(&st, g_open, g_high, g_low, g_close, g_nPoints, &iv0);
    ICOUNT_DUMP("CDLMARUBOZU/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_CDLMARUBOZU_Update(st, g_open[it & ICOUNT_MASK], g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &iv0);
            acc += (double)iv0;
        }
        ICOUNT_DUMP("CDLMARUBOZU/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_CDLMARUBOZU_Peek(st, g_open[it & ICOUNT_MASK], g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &iv0);
            acc += (double)iv0;
        }
        ICOUNT_DUMP("CDLMARUBOZU/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_CDLMARUBOZU_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_CDLMATCHINGLOW(int iters) {
    const char *nm = "CDLMATCHINGLOW";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_CDLMATCHINGLOW_Stream *st = NULL;
    TA_CDLMATCHINGLOW_Stream *stf = NULL;
    int iv0 = 0;

    ICOUNT_ZERO();
    rc = TA_CDLMATCHINGLOW(0, g_nPoints - 1, g_open, g_high, g_low, g_close, &outBegIdx, &outNBElement, g_outIntBuf0);
    ICOUNT_DUMP("CDLMATCHINGLOW/batch");
    icount_row(nm, "batch", 1, rc);
    acc += (double)g_outIntBuf0[0];

    ICOUNT_ZERO();
    rc = TA_CDLMATCHINGLOW_OpenAndFill(&stf, g_open, g_high, g_low, g_close, g_nPoints, &outBegIdx, &outNBElement, g_outIntBuf0);
    ICOUNT_DUMP("CDLMATCHINGLOW/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += (double)g_outIntBuf0[0];
    if( stf ) TA_CDLMATCHINGLOW_Close(stf);

    ICOUNT_ZERO();
    rc = TA_CDLMATCHINGLOW_Open(&st, g_open, g_high, g_low, g_close, g_nPoints, &iv0);
    ICOUNT_DUMP("CDLMATCHINGLOW/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_CDLMATCHINGLOW_Update(st, g_open[it & ICOUNT_MASK], g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &iv0);
            acc += (double)iv0;
        }
        ICOUNT_DUMP("CDLMATCHINGLOW/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_CDLMATCHINGLOW_Peek(st, g_open[it & ICOUNT_MASK], g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &iv0);
            acc += (double)iv0;
        }
        ICOUNT_DUMP("CDLMATCHINGLOW/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_CDLMATCHINGLOW_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_CDLMATHOLD(int iters) {
    const char *nm = "CDLMATHOLD";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_CDLMATHOLD_Stream *st = NULL;
    TA_CDLMATHOLD_Stream *stf = NULL;
    int iv0 = 0;

    ICOUNT_ZERO();
    rc = TA_CDLMATHOLD(0, g_nPoints - 1, g_open, g_high, g_low, g_close, 0.500000000000000, &outBegIdx, &outNBElement, g_outIntBuf0);
    ICOUNT_DUMP("CDLMATHOLD/batch");
    icount_row(nm, "batch", 1, rc);
    acc += (double)g_outIntBuf0[0];

    ICOUNT_ZERO();
    rc = TA_CDLMATHOLD_OpenAndFill(&stf, g_open, g_high, g_low, g_close, g_nPoints, 0.500000000000000, &outBegIdx, &outNBElement, g_outIntBuf0);
    ICOUNT_DUMP("CDLMATHOLD/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += (double)g_outIntBuf0[0];
    if( stf ) TA_CDLMATHOLD_Close(stf);

    ICOUNT_ZERO();
    rc = TA_CDLMATHOLD_Open(&st, g_open, g_high, g_low, g_close, g_nPoints, 0.500000000000000, &iv0);
    ICOUNT_DUMP("CDLMATHOLD/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_CDLMATHOLD_Update(st, g_open[it & ICOUNT_MASK], g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &iv0);
            acc += (double)iv0;
        }
        ICOUNT_DUMP("CDLMATHOLD/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_CDLMATHOLD_Peek(st, g_open[it & ICOUNT_MASK], g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &iv0);
            acc += (double)iv0;
        }
        ICOUNT_DUMP("CDLMATHOLD/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_CDLMATHOLD_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_CDLMORNINGDOJISTAR(int iters) {
    const char *nm = "CDLMORNINGDOJISTAR";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_CDLMORNINGDOJISTAR_Stream *st = NULL;
    TA_CDLMORNINGDOJISTAR_Stream *stf = NULL;
    int iv0 = 0;

    ICOUNT_ZERO();
    rc = TA_CDLMORNINGDOJISTAR(0, g_nPoints - 1, g_open, g_high, g_low, g_close, 0.300000000000000, &outBegIdx, &outNBElement, g_outIntBuf0);
    ICOUNT_DUMP("CDLMORNINGDOJISTAR/batch");
    icount_row(nm, "batch", 1, rc);
    acc += (double)g_outIntBuf0[0];

    ICOUNT_ZERO();
    rc = TA_CDLMORNINGDOJISTAR_OpenAndFill(&stf, g_open, g_high, g_low, g_close, g_nPoints, 0.300000000000000, &outBegIdx, &outNBElement, g_outIntBuf0);
    ICOUNT_DUMP("CDLMORNINGDOJISTAR/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += (double)g_outIntBuf0[0];
    if( stf ) TA_CDLMORNINGDOJISTAR_Close(stf);

    ICOUNT_ZERO();
    rc = TA_CDLMORNINGDOJISTAR_Open(&st, g_open, g_high, g_low, g_close, g_nPoints, 0.300000000000000, &iv0);
    ICOUNT_DUMP("CDLMORNINGDOJISTAR/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_CDLMORNINGDOJISTAR_Update(st, g_open[it & ICOUNT_MASK], g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &iv0);
            acc += (double)iv0;
        }
        ICOUNT_DUMP("CDLMORNINGDOJISTAR/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_CDLMORNINGDOJISTAR_Peek(st, g_open[it & ICOUNT_MASK], g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &iv0);
            acc += (double)iv0;
        }
        ICOUNT_DUMP("CDLMORNINGDOJISTAR/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_CDLMORNINGDOJISTAR_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_CDLMORNINGSTAR(int iters) {
    const char *nm = "CDLMORNINGSTAR";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_CDLMORNINGSTAR_Stream *st = NULL;
    TA_CDLMORNINGSTAR_Stream *stf = NULL;
    int iv0 = 0;

    ICOUNT_ZERO();
    rc = TA_CDLMORNINGSTAR(0, g_nPoints - 1, g_open, g_high, g_low, g_close, 0.300000000000000, &outBegIdx, &outNBElement, g_outIntBuf0);
    ICOUNT_DUMP("CDLMORNINGSTAR/batch");
    icount_row(nm, "batch", 1, rc);
    acc += (double)g_outIntBuf0[0];

    ICOUNT_ZERO();
    rc = TA_CDLMORNINGSTAR_OpenAndFill(&stf, g_open, g_high, g_low, g_close, g_nPoints, 0.300000000000000, &outBegIdx, &outNBElement, g_outIntBuf0);
    ICOUNT_DUMP("CDLMORNINGSTAR/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += (double)g_outIntBuf0[0];
    if( stf ) TA_CDLMORNINGSTAR_Close(stf);

    ICOUNT_ZERO();
    rc = TA_CDLMORNINGSTAR_Open(&st, g_open, g_high, g_low, g_close, g_nPoints, 0.300000000000000, &iv0);
    ICOUNT_DUMP("CDLMORNINGSTAR/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_CDLMORNINGSTAR_Update(st, g_open[it & ICOUNT_MASK], g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &iv0);
            acc += (double)iv0;
        }
        ICOUNT_DUMP("CDLMORNINGSTAR/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_CDLMORNINGSTAR_Peek(st, g_open[it & ICOUNT_MASK], g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &iv0);
            acc += (double)iv0;
        }
        ICOUNT_DUMP("CDLMORNINGSTAR/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_CDLMORNINGSTAR_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_CDLONNECK(int iters) {
    const char *nm = "CDLONNECK";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_CDLONNECK_Stream *st = NULL;
    TA_CDLONNECK_Stream *stf = NULL;
    int iv0 = 0;

    ICOUNT_ZERO();
    rc = TA_CDLONNECK(0, g_nPoints - 1, g_open, g_high, g_low, g_close, &outBegIdx, &outNBElement, g_outIntBuf0);
    ICOUNT_DUMP("CDLONNECK/batch");
    icount_row(nm, "batch", 1, rc);
    acc += (double)g_outIntBuf0[0];

    ICOUNT_ZERO();
    rc = TA_CDLONNECK_OpenAndFill(&stf, g_open, g_high, g_low, g_close, g_nPoints, &outBegIdx, &outNBElement, g_outIntBuf0);
    ICOUNT_DUMP("CDLONNECK/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += (double)g_outIntBuf0[0];
    if( stf ) TA_CDLONNECK_Close(stf);

    ICOUNT_ZERO();
    rc = TA_CDLONNECK_Open(&st, g_open, g_high, g_low, g_close, g_nPoints, &iv0);
    ICOUNT_DUMP("CDLONNECK/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_CDLONNECK_Update(st, g_open[it & ICOUNT_MASK], g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &iv0);
            acc += (double)iv0;
        }
        ICOUNT_DUMP("CDLONNECK/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_CDLONNECK_Peek(st, g_open[it & ICOUNT_MASK], g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &iv0);
            acc += (double)iv0;
        }
        ICOUNT_DUMP("CDLONNECK/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_CDLONNECK_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_CDLPIERCING(int iters) {
    const char *nm = "CDLPIERCING";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_CDLPIERCING_Stream *st = NULL;
    TA_CDLPIERCING_Stream *stf = NULL;
    int iv0 = 0;

    ICOUNT_ZERO();
    rc = TA_CDLPIERCING(0, g_nPoints - 1, g_open, g_high, g_low, g_close, &outBegIdx, &outNBElement, g_outIntBuf0);
    ICOUNT_DUMP("CDLPIERCING/batch");
    icount_row(nm, "batch", 1, rc);
    acc += (double)g_outIntBuf0[0];

    ICOUNT_ZERO();
    rc = TA_CDLPIERCING_OpenAndFill(&stf, g_open, g_high, g_low, g_close, g_nPoints, &outBegIdx, &outNBElement, g_outIntBuf0);
    ICOUNT_DUMP("CDLPIERCING/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += (double)g_outIntBuf0[0];
    if( stf ) TA_CDLPIERCING_Close(stf);

    ICOUNT_ZERO();
    rc = TA_CDLPIERCING_Open(&st, g_open, g_high, g_low, g_close, g_nPoints, &iv0);
    ICOUNT_DUMP("CDLPIERCING/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_CDLPIERCING_Update(st, g_open[it & ICOUNT_MASK], g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &iv0);
            acc += (double)iv0;
        }
        ICOUNT_DUMP("CDLPIERCING/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_CDLPIERCING_Peek(st, g_open[it & ICOUNT_MASK], g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &iv0);
            acc += (double)iv0;
        }
        ICOUNT_DUMP("CDLPIERCING/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_CDLPIERCING_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_CDLRICKSHAWMAN(int iters) {
    const char *nm = "CDLRICKSHAWMAN";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_CDLRICKSHAWMAN_Stream *st = NULL;
    TA_CDLRICKSHAWMAN_Stream *stf = NULL;
    int iv0 = 0;

    ICOUNT_ZERO();
    rc = TA_CDLRICKSHAWMAN(0, g_nPoints - 1, g_open, g_high, g_low, g_close, &outBegIdx, &outNBElement, g_outIntBuf0);
    ICOUNT_DUMP("CDLRICKSHAWMAN/batch");
    icount_row(nm, "batch", 1, rc);
    acc += (double)g_outIntBuf0[0];

    ICOUNT_ZERO();
    rc = TA_CDLRICKSHAWMAN_OpenAndFill(&stf, g_open, g_high, g_low, g_close, g_nPoints, &outBegIdx, &outNBElement, g_outIntBuf0);
    ICOUNT_DUMP("CDLRICKSHAWMAN/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += (double)g_outIntBuf0[0];
    if( stf ) TA_CDLRICKSHAWMAN_Close(stf);

    ICOUNT_ZERO();
    rc = TA_CDLRICKSHAWMAN_Open(&st, g_open, g_high, g_low, g_close, g_nPoints, &iv0);
    ICOUNT_DUMP("CDLRICKSHAWMAN/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_CDLRICKSHAWMAN_Update(st, g_open[it & ICOUNT_MASK], g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &iv0);
            acc += (double)iv0;
        }
        ICOUNT_DUMP("CDLRICKSHAWMAN/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_CDLRICKSHAWMAN_Peek(st, g_open[it & ICOUNT_MASK], g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &iv0);
            acc += (double)iv0;
        }
        ICOUNT_DUMP("CDLRICKSHAWMAN/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_CDLRICKSHAWMAN_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_CDLRISEFALL3METHODS(int iters) {
    const char *nm = "CDLRISEFALL3METHODS";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_CDLRISEFALL3METHODS_Stream *st = NULL;
    TA_CDLRISEFALL3METHODS_Stream *stf = NULL;
    int iv0 = 0;

    ICOUNT_ZERO();
    rc = TA_CDLRISEFALL3METHODS(0, g_nPoints - 1, g_open, g_high, g_low, g_close, &outBegIdx, &outNBElement, g_outIntBuf0);
    ICOUNT_DUMP("CDLRISEFALL3METHODS/batch");
    icount_row(nm, "batch", 1, rc);
    acc += (double)g_outIntBuf0[0];

    ICOUNT_ZERO();
    rc = TA_CDLRISEFALL3METHODS_OpenAndFill(&stf, g_open, g_high, g_low, g_close, g_nPoints, &outBegIdx, &outNBElement, g_outIntBuf0);
    ICOUNT_DUMP("CDLRISEFALL3METHODS/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += (double)g_outIntBuf0[0];
    if( stf ) TA_CDLRISEFALL3METHODS_Close(stf);

    ICOUNT_ZERO();
    rc = TA_CDLRISEFALL3METHODS_Open(&st, g_open, g_high, g_low, g_close, g_nPoints, &iv0);
    ICOUNT_DUMP("CDLRISEFALL3METHODS/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_CDLRISEFALL3METHODS_Update(st, g_open[it & ICOUNT_MASK], g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &iv0);
            acc += (double)iv0;
        }
        ICOUNT_DUMP("CDLRISEFALL3METHODS/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_CDLRISEFALL3METHODS_Peek(st, g_open[it & ICOUNT_MASK], g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &iv0);
            acc += (double)iv0;
        }
        ICOUNT_DUMP("CDLRISEFALL3METHODS/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_CDLRISEFALL3METHODS_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_CDLSEPARATINGLINES(int iters) {
    const char *nm = "CDLSEPARATINGLINES";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_CDLSEPARATINGLINES_Stream *st = NULL;
    TA_CDLSEPARATINGLINES_Stream *stf = NULL;
    int iv0 = 0;

    ICOUNT_ZERO();
    rc = TA_CDLSEPARATINGLINES(0, g_nPoints - 1, g_open, g_high, g_low, g_close, &outBegIdx, &outNBElement, g_outIntBuf0);
    ICOUNT_DUMP("CDLSEPARATINGLINES/batch");
    icount_row(nm, "batch", 1, rc);
    acc += (double)g_outIntBuf0[0];

    ICOUNT_ZERO();
    rc = TA_CDLSEPARATINGLINES_OpenAndFill(&stf, g_open, g_high, g_low, g_close, g_nPoints, &outBegIdx, &outNBElement, g_outIntBuf0);
    ICOUNT_DUMP("CDLSEPARATINGLINES/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += (double)g_outIntBuf0[0];
    if( stf ) TA_CDLSEPARATINGLINES_Close(stf);

    ICOUNT_ZERO();
    rc = TA_CDLSEPARATINGLINES_Open(&st, g_open, g_high, g_low, g_close, g_nPoints, &iv0);
    ICOUNT_DUMP("CDLSEPARATINGLINES/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_CDLSEPARATINGLINES_Update(st, g_open[it & ICOUNT_MASK], g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &iv0);
            acc += (double)iv0;
        }
        ICOUNT_DUMP("CDLSEPARATINGLINES/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_CDLSEPARATINGLINES_Peek(st, g_open[it & ICOUNT_MASK], g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &iv0);
            acc += (double)iv0;
        }
        ICOUNT_DUMP("CDLSEPARATINGLINES/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_CDLSEPARATINGLINES_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_CDLSHOOTINGSTAR(int iters) {
    const char *nm = "CDLSHOOTINGSTAR";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_CDLSHOOTINGSTAR_Stream *st = NULL;
    TA_CDLSHOOTINGSTAR_Stream *stf = NULL;
    int iv0 = 0;

    ICOUNT_ZERO();
    rc = TA_CDLSHOOTINGSTAR(0, g_nPoints - 1, g_open, g_high, g_low, g_close, &outBegIdx, &outNBElement, g_outIntBuf0);
    ICOUNT_DUMP("CDLSHOOTINGSTAR/batch");
    icount_row(nm, "batch", 1, rc);
    acc += (double)g_outIntBuf0[0];

    ICOUNT_ZERO();
    rc = TA_CDLSHOOTINGSTAR_OpenAndFill(&stf, g_open, g_high, g_low, g_close, g_nPoints, &outBegIdx, &outNBElement, g_outIntBuf0);
    ICOUNT_DUMP("CDLSHOOTINGSTAR/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += (double)g_outIntBuf0[0];
    if( stf ) TA_CDLSHOOTINGSTAR_Close(stf);

    ICOUNT_ZERO();
    rc = TA_CDLSHOOTINGSTAR_Open(&st, g_open, g_high, g_low, g_close, g_nPoints, &iv0);
    ICOUNT_DUMP("CDLSHOOTINGSTAR/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_CDLSHOOTINGSTAR_Update(st, g_open[it & ICOUNT_MASK], g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &iv0);
            acc += (double)iv0;
        }
        ICOUNT_DUMP("CDLSHOOTINGSTAR/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_CDLSHOOTINGSTAR_Peek(st, g_open[it & ICOUNT_MASK], g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &iv0);
            acc += (double)iv0;
        }
        ICOUNT_DUMP("CDLSHOOTINGSTAR/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_CDLSHOOTINGSTAR_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_CDLSHORTLINE(int iters) {
    const char *nm = "CDLSHORTLINE";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_CDLSHORTLINE_Stream *st = NULL;
    TA_CDLSHORTLINE_Stream *stf = NULL;
    int iv0 = 0;

    ICOUNT_ZERO();
    rc = TA_CDLSHORTLINE(0, g_nPoints - 1, g_open, g_high, g_low, g_close, &outBegIdx, &outNBElement, g_outIntBuf0);
    ICOUNT_DUMP("CDLSHORTLINE/batch");
    icount_row(nm, "batch", 1, rc);
    acc += (double)g_outIntBuf0[0];

    ICOUNT_ZERO();
    rc = TA_CDLSHORTLINE_OpenAndFill(&stf, g_open, g_high, g_low, g_close, g_nPoints, &outBegIdx, &outNBElement, g_outIntBuf0);
    ICOUNT_DUMP("CDLSHORTLINE/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += (double)g_outIntBuf0[0];
    if( stf ) TA_CDLSHORTLINE_Close(stf);

    ICOUNT_ZERO();
    rc = TA_CDLSHORTLINE_Open(&st, g_open, g_high, g_low, g_close, g_nPoints, &iv0);
    ICOUNT_DUMP("CDLSHORTLINE/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_CDLSHORTLINE_Update(st, g_open[it & ICOUNT_MASK], g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &iv0);
            acc += (double)iv0;
        }
        ICOUNT_DUMP("CDLSHORTLINE/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_CDLSHORTLINE_Peek(st, g_open[it & ICOUNT_MASK], g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &iv0);
            acc += (double)iv0;
        }
        ICOUNT_DUMP("CDLSHORTLINE/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_CDLSHORTLINE_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_CDLSPINNINGTOP(int iters) {
    const char *nm = "CDLSPINNINGTOP";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_CDLSPINNINGTOP_Stream *st = NULL;
    TA_CDLSPINNINGTOP_Stream *stf = NULL;
    int iv0 = 0;

    ICOUNT_ZERO();
    rc = TA_CDLSPINNINGTOP(0, g_nPoints - 1, g_open, g_high, g_low, g_close, &outBegIdx, &outNBElement, g_outIntBuf0);
    ICOUNT_DUMP("CDLSPINNINGTOP/batch");
    icount_row(nm, "batch", 1, rc);
    acc += (double)g_outIntBuf0[0];

    ICOUNT_ZERO();
    rc = TA_CDLSPINNINGTOP_OpenAndFill(&stf, g_open, g_high, g_low, g_close, g_nPoints, &outBegIdx, &outNBElement, g_outIntBuf0);
    ICOUNT_DUMP("CDLSPINNINGTOP/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += (double)g_outIntBuf0[0];
    if( stf ) TA_CDLSPINNINGTOP_Close(stf);

    ICOUNT_ZERO();
    rc = TA_CDLSPINNINGTOP_Open(&st, g_open, g_high, g_low, g_close, g_nPoints, &iv0);
    ICOUNT_DUMP("CDLSPINNINGTOP/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_CDLSPINNINGTOP_Update(st, g_open[it & ICOUNT_MASK], g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &iv0);
            acc += (double)iv0;
        }
        ICOUNT_DUMP("CDLSPINNINGTOP/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_CDLSPINNINGTOP_Peek(st, g_open[it & ICOUNT_MASK], g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &iv0);
            acc += (double)iv0;
        }
        ICOUNT_DUMP("CDLSPINNINGTOP/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_CDLSPINNINGTOP_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_CDLSTALLEDPATTERN(int iters) {
    const char *nm = "CDLSTALLEDPATTERN";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_CDLSTALLEDPATTERN_Stream *st = NULL;
    TA_CDLSTALLEDPATTERN_Stream *stf = NULL;
    int iv0 = 0;

    ICOUNT_ZERO();
    rc = TA_CDLSTALLEDPATTERN(0, g_nPoints - 1, g_open, g_high, g_low, g_close, &outBegIdx, &outNBElement, g_outIntBuf0);
    ICOUNT_DUMP("CDLSTALLEDPATTERN/batch");
    icount_row(nm, "batch", 1, rc);
    acc += (double)g_outIntBuf0[0];

    ICOUNT_ZERO();
    rc = TA_CDLSTALLEDPATTERN_OpenAndFill(&stf, g_open, g_high, g_low, g_close, g_nPoints, &outBegIdx, &outNBElement, g_outIntBuf0);
    ICOUNT_DUMP("CDLSTALLEDPATTERN/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += (double)g_outIntBuf0[0];
    if( stf ) TA_CDLSTALLEDPATTERN_Close(stf);

    ICOUNT_ZERO();
    rc = TA_CDLSTALLEDPATTERN_Open(&st, g_open, g_high, g_low, g_close, g_nPoints, &iv0);
    ICOUNT_DUMP("CDLSTALLEDPATTERN/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_CDLSTALLEDPATTERN_Update(st, g_open[it & ICOUNT_MASK], g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &iv0);
            acc += (double)iv0;
        }
        ICOUNT_DUMP("CDLSTALLEDPATTERN/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_CDLSTALLEDPATTERN_Peek(st, g_open[it & ICOUNT_MASK], g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &iv0);
            acc += (double)iv0;
        }
        ICOUNT_DUMP("CDLSTALLEDPATTERN/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_CDLSTALLEDPATTERN_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_CDLSTICKSANDWICH(int iters) {
    const char *nm = "CDLSTICKSANDWICH";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_CDLSTICKSANDWICH_Stream *st = NULL;
    TA_CDLSTICKSANDWICH_Stream *stf = NULL;
    int iv0 = 0;

    ICOUNT_ZERO();
    rc = TA_CDLSTICKSANDWICH(0, g_nPoints - 1, g_open, g_high, g_low, g_close, &outBegIdx, &outNBElement, g_outIntBuf0);
    ICOUNT_DUMP("CDLSTICKSANDWICH/batch");
    icount_row(nm, "batch", 1, rc);
    acc += (double)g_outIntBuf0[0];

    ICOUNT_ZERO();
    rc = TA_CDLSTICKSANDWICH_OpenAndFill(&stf, g_open, g_high, g_low, g_close, g_nPoints, &outBegIdx, &outNBElement, g_outIntBuf0);
    ICOUNT_DUMP("CDLSTICKSANDWICH/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += (double)g_outIntBuf0[0];
    if( stf ) TA_CDLSTICKSANDWICH_Close(stf);

    ICOUNT_ZERO();
    rc = TA_CDLSTICKSANDWICH_Open(&st, g_open, g_high, g_low, g_close, g_nPoints, &iv0);
    ICOUNT_DUMP("CDLSTICKSANDWICH/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_CDLSTICKSANDWICH_Update(st, g_open[it & ICOUNT_MASK], g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &iv0);
            acc += (double)iv0;
        }
        ICOUNT_DUMP("CDLSTICKSANDWICH/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_CDLSTICKSANDWICH_Peek(st, g_open[it & ICOUNT_MASK], g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &iv0);
            acc += (double)iv0;
        }
        ICOUNT_DUMP("CDLSTICKSANDWICH/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_CDLSTICKSANDWICH_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_CDLTAKURI(int iters) {
    const char *nm = "CDLTAKURI";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_CDLTAKURI_Stream *st = NULL;
    TA_CDLTAKURI_Stream *stf = NULL;
    int iv0 = 0;

    ICOUNT_ZERO();
    rc = TA_CDLTAKURI(0, g_nPoints - 1, g_open, g_high, g_low, g_close, &outBegIdx, &outNBElement, g_outIntBuf0);
    ICOUNT_DUMP("CDLTAKURI/batch");
    icount_row(nm, "batch", 1, rc);
    acc += (double)g_outIntBuf0[0];

    ICOUNT_ZERO();
    rc = TA_CDLTAKURI_OpenAndFill(&stf, g_open, g_high, g_low, g_close, g_nPoints, &outBegIdx, &outNBElement, g_outIntBuf0);
    ICOUNT_DUMP("CDLTAKURI/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += (double)g_outIntBuf0[0];
    if( stf ) TA_CDLTAKURI_Close(stf);

    ICOUNT_ZERO();
    rc = TA_CDLTAKURI_Open(&st, g_open, g_high, g_low, g_close, g_nPoints, &iv0);
    ICOUNT_DUMP("CDLTAKURI/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_CDLTAKURI_Update(st, g_open[it & ICOUNT_MASK], g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &iv0);
            acc += (double)iv0;
        }
        ICOUNT_DUMP("CDLTAKURI/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_CDLTAKURI_Peek(st, g_open[it & ICOUNT_MASK], g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &iv0);
            acc += (double)iv0;
        }
        ICOUNT_DUMP("CDLTAKURI/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_CDLTAKURI_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_CDLTASUKIGAP(int iters) {
    const char *nm = "CDLTASUKIGAP";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_CDLTASUKIGAP_Stream *st = NULL;
    TA_CDLTASUKIGAP_Stream *stf = NULL;
    int iv0 = 0;

    ICOUNT_ZERO();
    rc = TA_CDLTASUKIGAP(0, g_nPoints - 1, g_open, g_high, g_low, g_close, &outBegIdx, &outNBElement, g_outIntBuf0);
    ICOUNT_DUMP("CDLTASUKIGAP/batch");
    icount_row(nm, "batch", 1, rc);
    acc += (double)g_outIntBuf0[0];

    ICOUNT_ZERO();
    rc = TA_CDLTASUKIGAP_OpenAndFill(&stf, g_open, g_high, g_low, g_close, g_nPoints, &outBegIdx, &outNBElement, g_outIntBuf0);
    ICOUNT_DUMP("CDLTASUKIGAP/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += (double)g_outIntBuf0[0];
    if( stf ) TA_CDLTASUKIGAP_Close(stf);

    ICOUNT_ZERO();
    rc = TA_CDLTASUKIGAP_Open(&st, g_open, g_high, g_low, g_close, g_nPoints, &iv0);
    ICOUNT_DUMP("CDLTASUKIGAP/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_CDLTASUKIGAP_Update(st, g_open[it & ICOUNT_MASK], g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &iv0);
            acc += (double)iv0;
        }
        ICOUNT_DUMP("CDLTASUKIGAP/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_CDLTASUKIGAP_Peek(st, g_open[it & ICOUNT_MASK], g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &iv0);
            acc += (double)iv0;
        }
        ICOUNT_DUMP("CDLTASUKIGAP/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_CDLTASUKIGAP_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_CDLTHRUSTING(int iters) {
    const char *nm = "CDLTHRUSTING";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_CDLTHRUSTING_Stream *st = NULL;
    TA_CDLTHRUSTING_Stream *stf = NULL;
    int iv0 = 0;

    ICOUNT_ZERO();
    rc = TA_CDLTHRUSTING(0, g_nPoints - 1, g_open, g_high, g_low, g_close, &outBegIdx, &outNBElement, g_outIntBuf0);
    ICOUNT_DUMP("CDLTHRUSTING/batch");
    icount_row(nm, "batch", 1, rc);
    acc += (double)g_outIntBuf0[0];

    ICOUNT_ZERO();
    rc = TA_CDLTHRUSTING_OpenAndFill(&stf, g_open, g_high, g_low, g_close, g_nPoints, &outBegIdx, &outNBElement, g_outIntBuf0);
    ICOUNT_DUMP("CDLTHRUSTING/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += (double)g_outIntBuf0[0];
    if( stf ) TA_CDLTHRUSTING_Close(stf);

    ICOUNT_ZERO();
    rc = TA_CDLTHRUSTING_Open(&st, g_open, g_high, g_low, g_close, g_nPoints, &iv0);
    ICOUNT_DUMP("CDLTHRUSTING/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_CDLTHRUSTING_Update(st, g_open[it & ICOUNT_MASK], g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &iv0);
            acc += (double)iv0;
        }
        ICOUNT_DUMP("CDLTHRUSTING/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_CDLTHRUSTING_Peek(st, g_open[it & ICOUNT_MASK], g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &iv0);
            acc += (double)iv0;
        }
        ICOUNT_DUMP("CDLTHRUSTING/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_CDLTHRUSTING_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_CDLTRISTAR(int iters) {
    const char *nm = "CDLTRISTAR";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_CDLTRISTAR_Stream *st = NULL;
    TA_CDLTRISTAR_Stream *stf = NULL;
    int iv0 = 0;

    ICOUNT_ZERO();
    rc = TA_CDLTRISTAR(0, g_nPoints - 1, g_open, g_high, g_low, g_close, &outBegIdx, &outNBElement, g_outIntBuf0);
    ICOUNT_DUMP("CDLTRISTAR/batch");
    icount_row(nm, "batch", 1, rc);
    acc += (double)g_outIntBuf0[0];

    ICOUNT_ZERO();
    rc = TA_CDLTRISTAR_OpenAndFill(&stf, g_open, g_high, g_low, g_close, g_nPoints, &outBegIdx, &outNBElement, g_outIntBuf0);
    ICOUNT_DUMP("CDLTRISTAR/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += (double)g_outIntBuf0[0];
    if( stf ) TA_CDLTRISTAR_Close(stf);

    ICOUNT_ZERO();
    rc = TA_CDLTRISTAR_Open(&st, g_open, g_high, g_low, g_close, g_nPoints, &iv0);
    ICOUNT_DUMP("CDLTRISTAR/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_CDLTRISTAR_Update(st, g_open[it & ICOUNT_MASK], g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &iv0);
            acc += (double)iv0;
        }
        ICOUNT_DUMP("CDLTRISTAR/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_CDLTRISTAR_Peek(st, g_open[it & ICOUNT_MASK], g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &iv0);
            acc += (double)iv0;
        }
        ICOUNT_DUMP("CDLTRISTAR/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_CDLTRISTAR_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_CDLUNIQUE3RIVER(int iters) {
    const char *nm = "CDLUNIQUE3RIVER";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_CDLUNIQUE3RIVER_Stream *st = NULL;
    TA_CDLUNIQUE3RIVER_Stream *stf = NULL;
    int iv0 = 0;

    ICOUNT_ZERO();
    rc = TA_CDLUNIQUE3RIVER(0, g_nPoints - 1, g_open, g_high, g_low, g_close, &outBegIdx, &outNBElement, g_outIntBuf0);
    ICOUNT_DUMP("CDLUNIQUE3RIVER/batch");
    icount_row(nm, "batch", 1, rc);
    acc += (double)g_outIntBuf0[0];

    ICOUNT_ZERO();
    rc = TA_CDLUNIQUE3RIVER_OpenAndFill(&stf, g_open, g_high, g_low, g_close, g_nPoints, &outBegIdx, &outNBElement, g_outIntBuf0);
    ICOUNT_DUMP("CDLUNIQUE3RIVER/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += (double)g_outIntBuf0[0];
    if( stf ) TA_CDLUNIQUE3RIVER_Close(stf);

    ICOUNT_ZERO();
    rc = TA_CDLUNIQUE3RIVER_Open(&st, g_open, g_high, g_low, g_close, g_nPoints, &iv0);
    ICOUNT_DUMP("CDLUNIQUE3RIVER/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_CDLUNIQUE3RIVER_Update(st, g_open[it & ICOUNT_MASK], g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &iv0);
            acc += (double)iv0;
        }
        ICOUNT_DUMP("CDLUNIQUE3RIVER/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_CDLUNIQUE3RIVER_Peek(st, g_open[it & ICOUNT_MASK], g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &iv0);
            acc += (double)iv0;
        }
        ICOUNT_DUMP("CDLUNIQUE3RIVER/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_CDLUNIQUE3RIVER_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_CDLUPSIDEGAP2CROWS(int iters) {
    const char *nm = "CDLUPSIDEGAP2CROWS";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_CDLUPSIDEGAP2CROWS_Stream *st = NULL;
    TA_CDLUPSIDEGAP2CROWS_Stream *stf = NULL;
    int iv0 = 0;

    ICOUNT_ZERO();
    rc = TA_CDLUPSIDEGAP2CROWS(0, g_nPoints - 1, g_open, g_high, g_low, g_close, &outBegIdx, &outNBElement, g_outIntBuf0);
    ICOUNT_DUMP("CDLUPSIDEGAP2CROWS/batch");
    icount_row(nm, "batch", 1, rc);
    acc += (double)g_outIntBuf0[0];

    ICOUNT_ZERO();
    rc = TA_CDLUPSIDEGAP2CROWS_OpenAndFill(&stf, g_open, g_high, g_low, g_close, g_nPoints, &outBegIdx, &outNBElement, g_outIntBuf0);
    ICOUNT_DUMP("CDLUPSIDEGAP2CROWS/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += (double)g_outIntBuf0[0];
    if( stf ) TA_CDLUPSIDEGAP2CROWS_Close(stf);

    ICOUNT_ZERO();
    rc = TA_CDLUPSIDEGAP2CROWS_Open(&st, g_open, g_high, g_low, g_close, g_nPoints, &iv0);
    ICOUNT_DUMP("CDLUPSIDEGAP2CROWS/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_CDLUPSIDEGAP2CROWS_Update(st, g_open[it & ICOUNT_MASK], g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &iv0);
            acc += (double)iv0;
        }
        ICOUNT_DUMP("CDLUPSIDEGAP2CROWS/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_CDLUPSIDEGAP2CROWS_Peek(st, g_open[it & ICOUNT_MASK], g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &iv0);
            acc += (double)iv0;
        }
        ICOUNT_DUMP("CDLUPSIDEGAP2CROWS/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_CDLUPSIDEGAP2CROWS_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_CDLXSIDEGAP3METHODS(int iters) {
    const char *nm = "CDLXSIDEGAP3METHODS";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_CDLXSIDEGAP3METHODS_Stream *st = NULL;
    TA_CDLXSIDEGAP3METHODS_Stream *stf = NULL;
    int iv0 = 0;

    ICOUNT_ZERO();
    rc = TA_CDLXSIDEGAP3METHODS(0, g_nPoints - 1, g_open, g_high, g_low, g_close, &outBegIdx, &outNBElement, g_outIntBuf0);
    ICOUNT_DUMP("CDLXSIDEGAP3METHODS/batch");
    icount_row(nm, "batch", 1, rc);
    acc += (double)g_outIntBuf0[0];

    ICOUNT_ZERO();
    rc = TA_CDLXSIDEGAP3METHODS_OpenAndFill(&stf, g_open, g_high, g_low, g_close, g_nPoints, &outBegIdx, &outNBElement, g_outIntBuf0);
    ICOUNT_DUMP("CDLXSIDEGAP3METHODS/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += (double)g_outIntBuf0[0];
    if( stf ) TA_CDLXSIDEGAP3METHODS_Close(stf);

    ICOUNT_ZERO();
    rc = TA_CDLXSIDEGAP3METHODS_Open(&st, g_open, g_high, g_low, g_close, g_nPoints, &iv0);
    ICOUNT_DUMP("CDLXSIDEGAP3METHODS/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_CDLXSIDEGAP3METHODS_Update(st, g_open[it & ICOUNT_MASK], g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &iv0);
            acc += (double)iv0;
        }
        ICOUNT_DUMP("CDLXSIDEGAP3METHODS/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_CDLXSIDEGAP3METHODS_Peek(st, g_open[it & ICOUNT_MASK], g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &iv0);
            acc += (double)iv0;
        }
        ICOUNT_DUMP("CDLXSIDEGAP3METHODS/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_CDLXSIDEGAP3METHODS_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_CEIL(int iters) {
    const char *nm = "CEIL";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_CEIL_Stream *st = NULL;
    TA_CEIL_Stream *stf = NULL;
    double v0 = 0.0;

    ICOUNT_ZERO();
    rc = TA_CEIL(0, g_nPoints - 1, g_close, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("CEIL/batch");
    icount_row(nm, "batch", 1, rc);
    acc += g_outBuf0[0];

    ICOUNT_ZERO();
    rc = TA_CEIL_OpenAndFill(&stf, g_close, g_nPoints, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("CEIL/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += g_outBuf0[0];
    if( stf ) TA_CEIL_Close(stf);

    ICOUNT_ZERO();
    rc = TA_CEIL_Open(&st, g_close, g_nPoints, &v0);
    ICOUNT_DUMP("CEIL/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_CEIL_Update(st, g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("CEIL/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_CEIL_Peek(st, g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("CEIL/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_CEIL_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_CMF(int iters) {
    const char *nm = "CMF";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_CMF_Stream *st = NULL;
    TA_CMF_Stream *stf = NULL;
    double v0 = 0.0;

    ICOUNT_ZERO();
    rc = TA_CMF(0, g_nPoints - 1, g_high, g_low, g_close, g_volume, 20, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("CMF/batch");
    icount_row(nm, "batch", 1, rc);
    acc += g_outBuf0[0];

    ICOUNT_ZERO();
    rc = TA_CMF_OpenAndFill(&stf, g_high, g_low, g_close, g_volume, g_nPoints, 20, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("CMF/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += g_outBuf0[0];
    if( stf ) TA_CMF_Close(stf);

    ICOUNT_ZERO();
    rc = TA_CMF_Open(&st, g_high, g_low, g_close, g_volume, g_nPoints, 20, &v0);
    ICOUNT_DUMP("CMF/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_CMF_Update(st, g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], g_volume[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("CMF/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_CMF_Peek(st, g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], g_volume[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("CMF/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_CMF_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_CMO(int iters) {
    const char *nm = "CMO";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_CMO_Stream *st = NULL;
    TA_CMO_Stream *stf = NULL;
    double v0 = 0.0;

    ICOUNT_ZERO();
    rc = TA_CMO(0, g_nPoints - 1, g_close, 14, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("CMO/batch");
    icount_row(nm, "batch", 1, rc);
    acc += g_outBuf0[0];

    ICOUNT_ZERO();
    rc = TA_CMO_OpenAndFill(&stf, g_close, g_nPoints, 14, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("CMO/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += g_outBuf0[0];
    if( stf ) TA_CMO_Close(stf);

    ICOUNT_ZERO();
    rc = TA_CMO_Open(&st, g_close, g_nPoints, 14, &v0);
    ICOUNT_DUMP("CMO/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_CMO_Update(st, g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("CMO/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_CMO_Peek(st, g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("CMO/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_CMO_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_CMOU(int iters) {
    const char *nm = "CMOU";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_CMOU_Stream *st = NULL;
    TA_CMOU_Stream *stf = NULL;
    double v0 = 0.0;

    ICOUNT_ZERO();
    rc = TA_CMOU(0, g_nPoints - 1, g_close, 14, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("CMOU/batch");
    icount_row(nm, "batch", 1, rc);
    acc += g_outBuf0[0];

    ICOUNT_ZERO();
    rc = TA_CMOU_OpenAndFill(&stf, g_close, g_nPoints, 14, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("CMOU/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += g_outBuf0[0];
    if( stf ) TA_CMOU_Close(stf);

    ICOUNT_ZERO();
    rc = TA_CMOU_Open(&st, g_close, g_nPoints, 14, &v0);
    ICOUNT_DUMP("CMOU/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_CMOU_Update(st, g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("CMOU/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_CMOU_Peek(st, g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("CMOU/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_CMOU_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_COPPOCK(int iters) {
    const char *nm = "COPPOCK";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_COPPOCK_Stream *st = NULL;
    TA_COPPOCK_Stream *stf = NULL;
    double v0 = 0.0;

    ICOUNT_ZERO();
    rc = TA_COPPOCK(0, g_nPoints - 1, g_close, 10, 11, 14, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("COPPOCK/batch");
    icount_row(nm, "batch", 1, rc);
    acc += g_outBuf0[0];

    ICOUNT_ZERO();
    rc = TA_COPPOCK_OpenAndFill(&stf, g_close, g_nPoints, 10, 11, 14, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("COPPOCK/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += g_outBuf0[0];
    if( stf ) TA_COPPOCK_Close(stf);

    ICOUNT_ZERO();
    rc = TA_COPPOCK_Open(&st, g_close, g_nPoints, 10, 11, 14, &v0);
    ICOUNT_DUMP("COPPOCK/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_COPPOCK_Update(st, g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("COPPOCK/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_COPPOCK_Peek(st, g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("COPPOCK/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_COPPOCK_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_CORREL(int iters) {
    const char *nm = "CORREL";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_CORREL_Stream *st = NULL;
    TA_CORREL_Stream *stf = NULL;
    double v0 = 0.0;

    ICOUNT_ZERO();
    rc = TA_CORREL(0, g_nPoints - 1, g_close, g_high, 30, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("CORREL/batch");
    icount_row(nm, "batch", 1, rc);
    acc += g_outBuf0[0];

    ICOUNT_ZERO();
    rc = TA_CORREL_OpenAndFill(&stf, g_close, g_high, g_nPoints, 30, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("CORREL/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += g_outBuf0[0];
    if( stf ) TA_CORREL_Close(stf);

    ICOUNT_ZERO();
    rc = TA_CORREL_Open(&st, g_close, g_high, g_nPoints, 30, &v0);
    ICOUNT_DUMP("CORREL/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_CORREL_Update(st, g_close[it & ICOUNT_MASK], g_high[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("CORREL/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_CORREL_Peek(st, g_close[it & ICOUNT_MASK], g_high[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("CORREL/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_CORREL_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_COS(int iters) {
    const char *nm = "COS";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_COS_Stream *st = NULL;
    TA_COS_Stream *stf = NULL;
    double v0 = 0.0;

    ICOUNT_ZERO();
    rc = TA_COS(0, g_nPoints - 1, g_close, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("COS/batch");
    icount_row(nm, "batch", 1, rc);
    acc += g_outBuf0[0];

    ICOUNT_ZERO();
    rc = TA_COS_OpenAndFill(&stf, g_close, g_nPoints, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("COS/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += g_outBuf0[0];
    if( stf ) TA_COS_Close(stf);

    ICOUNT_ZERO();
    rc = TA_COS_Open(&st, g_close, g_nPoints, &v0);
    ICOUNT_DUMP("COS/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_COS_Update(st, g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("COS/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_COS_Peek(st, g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("COS/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_COS_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_COSH(int iters) {
    const char *nm = "COSH";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_COSH_Stream *st = NULL;
    TA_COSH_Stream *stf = NULL;
    double v0 = 0.0;

    ICOUNT_ZERO();
    rc = TA_COSH(0, g_nPoints - 1, g_close, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("COSH/batch");
    icount_row(nm, "batch", 1, rc);
    acc += g_outBuf0[0];

    ICOUNT_ZERO();
    rc = TA_COSH_OpenAndFill(&stf, g_close, g_nPoints, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("COSH/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += g_outBuf0[0];
    if( stf ) TA_COSH_Close(stf);

    ICOUNT_ZERO();
    rc = TA_COSH_Open(&st, g_close, g_nPoints, &v0);
    ICOUNT_DUMP("COSH/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_COSH_Update(st, g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("COSH/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_COSH_Peek(st, g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("COSH/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_COSH_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_CUMSUM(int iters) {
    const char *nm = "CUMSUM";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_CUMSUM_Stream *st = NULL;
    TA_CUMSUM_Stream *stf = NULL;
    double v0 = 0.0;

    ICOUNT_ZERO();
    rc = TA_CUMSUM(0, g_nPoints - 1, g_close, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("CUMSUM/batch");
    icount_row(nm, "batch", 1, rc);
    acc += g_outBuf0[0];

    ICOUNT_ZERO();
    rc = TA_CUMSUM_OpenAndFill(&stf, g_close, g_nPoints, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("CUMSUM/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += g_outBuf0[0];
    if( stf ) TA_CUMSUM_Close(stf);

    ICOUNT_ZERO();
    rc = TA_CUMSUM_Open(&st, g_close, g_nPoints, &v0);
    ICOUNT_DUMP("CUMSUM/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_CUMSUM_Update(st, g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("CUMSUM/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_CUMSUM_Peek(st, g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("CUMSUM/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_CUMSUM_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_CVI(int iters) {
    const char *nm = "CVI";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_CVI_Stream *st = NULL;
    TA_CVI_Stream *stf = NULL;
    double v0 = 0.0;

    ICOUNT_ZERO();
    rc = TA_CVI(0, g_nPoints - 1, g_high, g_low, 10, 10, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("CVI/batch");
    icount_row(nm, "batch", 1, rc);
    acc += g_outBuf0[0];

    ICOUNT_ZERO();
    rc = TA_CVI_OpenAndFill(&stf, g_high, g_low, g_nPoints, 10, 10, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("CVI/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += g_outBuf0[0];
    if( stf ) TA_CVI_Close(stf);

    ICOUNT_ZERO();
    rc = TA_CVI_Open(&st, g_high, g_low, g_nPoints, 10, 10, &v0);
    ICOUNT_DUMP("CVI/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_CVI_Update(st, g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("CVI/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_CVI_Peek(st, g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("CVI/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_CVI_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_DEMA(int iters) {
    const char *nm = "DEMA";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_DEMA_Stream *st = NULL;
    TA_DEMA_Stream *stf = NULL;
    double v0 = 0.0;

    ICOUNT_ZERO();
    rc = TA_DEMA(0, g_nPoints - 1, g_close, 30, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("DEMA/batch");
    icount_row(nm, "batch", 1, rc);
    acc += g_outBuf0[0];

    ICOUNT_ZERO();
    rc = TA_DEMA_OpenAndFill(&stf, g_close, g_nPoints, 30, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("DEMA/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += g_outBuf0[0];
    if( stf ) TA_DEMA_Close(stf);

    ICOUNT_ZERO();
    rc = TA_DEMA_Open(&st, g_close, g_nPoints, 30, &v0);
    ICOUNT_DUMP("DEMA/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_DEMA_Update(st, g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("DEMA/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_DEMA_Peek(st, g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("DEMA/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_DEMA_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_DIV(int iters) {
    const char *nm = "DIV";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_DIV_Stream *st = NULL;
    TA_DIV_Stream *stf = NULL;
    double v0 = 0.0;

    ICOUNT_ZERO();
    rc = TA_DIV(0, g_nPoints - 1, g_close, g_high, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("DIV/batch");
    icount_row(nm, "batch", 1, rc);
    acc += g_outBuf0[0];

    ICOUNT_ZERO();
    rc = TA_DIV_OpenAndFill(&stf, g_close, g_high, g_nPoints, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("DIV/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += g_outBuf0[0];
    if( stf ) TA_DIV_Close(stf);

    ICOUNT_ZERO();
    rc = TA_DIV_Open(&st, g_close, g_high, g_nPoints, &v0);
    ICOUNT_DUMP("DIV/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_DIV_Update(st, g_close[it & ICOUNT_MASK], g_high[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("DIV/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_DIV_Peek(st, g_close[it & ICOUNT_MASK], g_high[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("DIV/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_DIV_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_DONCHIAN(int iters) {
    const char *nm = "DONCHIAN";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_DONCHIAN_Stream *st = NULL;
    TA_DONCHIAN_Stream *stf = NULL;
    double v0 = 0.0;
    double v1 = 0.0;
    double v2 = 0.0;

    ICOUNT_ZERO();
    rc = TA_DONCHIAN(0, g_nPoints - 1, g_high, g_low, 20, &outBegIdx, &outNBElement, g_outBuf0, g_outBuf1, g_outBuf2);
    ICOUNT_DUMP("DONCHIAN/batch");
    icount_row(nm, "batch", 1, rc);
    acc += g_outBuf0[0];
    acc += g_outBuf1[0];
    acc += g_outBuf2[0];

    ICOUNT_ZERO();
    rc = TA_DONCHIAN_OpenAndFill(&stf, g_high, g_low, g_nPoints, 20, &outBegIdx, &outNBElement, g_outBuf0, g_outBuf1, g_outBuf2);
    ICOUNT_DUMP("DONCHIAN/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += g_outBuf0[0];
    acc += g_outBuf1[0];
    acc += g_outBuf2[0];
    if( stf ) TA_DONCHIAN_Close(stf);

    ICOUNT_ZERO();
    rc = TA_DONCHIAN_Open(&st, g_high, g_low, g_nPoints, 20, &v0, &v1, &v2);
    ICOUNT_DUMP("DONCHIAN/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_DONCHIAN_Update(st, g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], &v0, &v1, &v2);
            acc += v0;
            acc += v1;
            acc += v2;
        }
        ICOUNT_DUMP("DONCHIAN/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_DONCHIAN_Peek(st, g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], &v0, &v1, &v2);
            acc += v0;
            acc += v1;
            acc += v2;
        }
        ICOUNT_DUMP("DONCHIAN/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_DONCHIAN_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_DPO(int iters) {
    const char *nm = "DPO";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_DPO_Stream *st = NULL;
    TA_DPO_Stream *stf = NULL;
    double v0 = 0.0;

    ICOUNT_ZERO();
    rc = TA_DPO(0, g_nPoints - 1, g_close, 20, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("DPO/batch");
    icount_row(nm, "batch", 1, rc);
    acc += g_outBuf0[0];

    ICOUNT_ZERO();
    rc = TA_DPO_OpenAndFill(&stf, g_close, g_nPoints, 20, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("DPO/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += g_outBuf0[0];
    if( stf ) TA_DPO_Close(stf);

    ICOUNT_ZERO();
    rc = TA_DPO_Open(&st, g_close, g_nPoints, 20, &v0);
    ICOUNT_DUMP("DPO/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_DPO_Update(st, g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("DPO/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_DPO_Peek(st, g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("DPO/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_DPO_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_DX(int iters) {
    const char *nm = "DX";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_DX_Stream *st = NULL;
    TA_DX_Stream *stf = NULL;
    double v0 = 0.0;

    ICOUNT_ZERO();
    rc = TA_DX(0, g_nPoints - 1, g_high, g_low, g_close, 14, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("DX/batch");
    icount_row(nm, "batch", 1, rc);
    acc += g_outBuf0[0];

    ICOUNT_ZERO();
    rc = TA_DX_OpenAndFill(&stf, g_high, g_low, g_close, g_nPoints, 14, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("DX/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += g_outBuf0[0];
    if( stf ) TA_DX_Close(stf);

    ICOUNT_ZERO();
    rc = TA_DX_Open(&st, g_high, g_low, g_close, g_nPoints, 14, &v0);
    ICOUNT_DUMP("DX/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_DX_Update(st, g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("DX/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_DX_Peek(st, g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("DX/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_DX_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_EFI(int iters) {
    const char *nm = "EFI";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_EFI_Stream *st = NULL;
    TA_EFI_Stream *stf = NULL;
    double v0 = 0.0;

    ICOUNT_ZERO();
    rc = TA_EFI(0, g_nPoints - 1, g_close, g_volume, 13, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("EFI/batch");
    icount_row(nm, "batch", 1, rc);
    acc += g_outBuf0[0];

    ICOUNT_ZERO();
    rc = TA_EFI_OpenAndFill(&stf, g_close, g_volume, g_nPoints, 13, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("EFI/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += g_outBuf0[0];
    if( stf ) TA_EFI_Close(stf);

    ICOUNT_ZERO();
    rc = TA_EFI_Open(&st, g_close, g_volume, g_nPoints, 13, &v0);
    ICOUNT_DUMP("EFI/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_EFI_Update(st, g_close[it & ICOUNT_MASK], g_volume[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("EFI/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_EFI_Peek(st, g_close[it & ICOUNT_MASK], g_volume[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("EFI/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_EFI_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_EMA(int iters) {
    const char *nm = "EMA";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_EMA_Stream *st = NULL;
    TA_EMA_Stream *stf = NULL;
    double v0 = 0.0;

    ICOUNT_ZERO();
    rc = TA_EMA(0, g_nPoints - 1, g_close, 30, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("EMA/batch");
    icount_row(nm, "batch", 1, rc);
    acc += g_outBuf0[0];

    ICOUNT_ZERO();
    rc = TA_EMA_OpenAndFill(&stf, g_close, g_nPoints, 30, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("EMA/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += g_outBuf0[0];
    if( stf ) TA_EMA_Close(stf);

    ICOUNT_ZERO();
    rc = TA_EMA_Open(&st, g_close, g_nPoints, 30, &v0);
    ICOUNT_DUMP("EMA/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_EMA_Update(st, g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("EMA/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_EMA_Peek(st, g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("EMA/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_EMA_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_ER(int iters) {
    const char *nm = "ER";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_ER_Stream *st = NULL;
    TA_ER_Stream *stf = NULL;
    double v0 = 0.0;

    ICOUNT_ZERO();
    rc = TA_ER(0, g_nPoints - 1, g_close, 10, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("ER/batch");
    icount_row(nm, "batch", 1, rc);
    acc += g_outBuf0[0];

    ICOUNT_ZERO();
    rc = TA_ER_OpenAndFill(&stf, g_close, g_nPoints, 10, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("ER/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += g_outBuf0[0];
    if( stf ) TA_ER_Close(stf);

    ICOUNT_ZERO();
    rc = TA_ER_Open(&st, g_close, g_nPoints, 10, &v0);
    ICOUNT_DUMP("ER/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_ER_Update(st, g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("ER/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_ER_Peek(st, g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("ER/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_ER_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_ERI(int iters) {
    const char *nm = "ERI";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_ERI_Stream *st = NULL;
    TA_ERI_Stream *stf = NULL;
    double v0 = 0.0;
    double v1 = 0.0;

    ICOUNT_ZERO();
    rc = TA_ERI(0, g_nPoints - 1, g_high, g_low, g_close, 13, &outBegIdx, &outNBElement, g_outBuf0, g_outBuf1);
    ICOUNT_DUMP("ERI/batch");
    icount_row(nm, "batch", 1, rc);
    acc += g_outBuf0[0];
    acc += g_outBuf1[0];

    ICOUNT_ZERO();
    rc = TA_ERI_OpenAndFill(&stf, g_high, g_low, g_close, g_nPoints, 13, &outBegIdx, &outNBElement, g_outBuf0, g_outBuf1);
    ICOUNT_DUMP("ERI/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += g_outBuf0[0];
    acc += g_outBuf1[0];
    if( stf ) TA_ERI_Close(stf);

    ICOUNT_ZERO();
    rc = TA_ERI_Open(&st, g_high, g_low, g_close, g_nPoints, 13, &v0, &v1);
    ICOUNT_DUMP("ERI/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_ERI_Update(st, g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &v0, &v1);
            acc += v0;
            acc += v1;
        }
        ICOUNT_DUMP("ERI/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_ERI_Peek(st, g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &v0, &v1);
            acc += v0;
            acc += v1;
        }
        ICOUNT_DUMP("ERI/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_ERI_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_EXP(int iters) {
    const char *nm = "EXP";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_EXP_Stream *st = NULL;
    TA_EXP_Stream *stf = NULL;
    double v0 = 0.0;

    ICOUNT_ZERO();
    rc = TA_EXP(0, g_nPoints - 1, g_close, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("EXP/batch");
    icount_row(nm, "batch", 1, rc);
    acc += g_outBuf0[0];

    ICOUNT_ZERO();
    rc = TA_EXP_OpenAndFill(&stf, g_close, g_nPoints, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("EXP/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += g_outBuf0[0];
    if( stf ) TA_EXP_Close(stf);

    ICOUNT_ZERO();
    rc = TA_EXP_Open(&st, g_close, g_nPoints, &v0);
    ICOUNT_DUMP("EXP/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_EXP_Update(st, g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("EXP/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_EXP_Peek(st, g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("EXP/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_EXP_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_FLOOR(int iters) {
    const char *nm = "FLOOR";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_FLOOR_Stream *st = NULL;
    TA_FLOOR_Stream *stf = NULL;
    double v0 = 0.0;

    ICOUNT_ZERO();
    rc = TA_FLOOR(0, g_nPoints - 1, g_close, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("FLOOR/batch");
    icount_row(nm, "batch", 1, rc);
    acc += g_outBuf0[0];

    ICOUNT_ZERO();
    rc = TA_FLOOR_OpenAndFill(&stf, g_close, g_nPoints, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("FLOOR/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += g_outBuf0[0];
    if( stf ) TA_FLOOR_Close(stf);

    ICOUNT_ZERO();
    rc = TA_FLOOR_Open(&st, g_close, g_nPoints, &v0);
    ICOUNT_DUMP("FLOOR/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_FLOOR_Update(st, g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("FLOOR/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_FLOOR_Peek(st, g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("FLOOR/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_FLOOR_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_FOSC(int iters) {
    const char *nm = "FOSC";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_FOSC_Stream *st = NULL;
    TA_FOSC_Stream *stf = NULL;
    double v0 = 0.0;

    ICOUNT_ZERO();
    rc = TA_FOSC(0, g_nPoints - 1, g_close, 5, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("FOSC/batch");
    icount_row(nm, "batch", 1, rc);
    acc += g_outBuf0[0];

    ICOUNT_ZERO();
    rc = TA_FOSC_OpenAndFill(&stf, g_close, g_nPoints, 5, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("FOSC/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += g_outBuf0[0];
    if( stf ) TA_FOSC_Close(stf);

    ICOUNT_ZERO();
    rc = TA_FOSC_Open(&st, g_close, g_nPoints, 5, &v0);
    ICOUNT_DUMP("FOSC/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_FOSC_Update(st, g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("FOSC/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_FOSC_Peek(st, g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("FOSC/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_FOSC_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_FRACTAL(int iters) {
    const char *nm = "FRACTAL";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_FRACTAL_Stream *st = NULL;
    TA_FRACTAL_Stream *stf = NULL;
    int iv0 = 0;
    int iv1 = 0;

    ICOUNT_ZERO();
    rc = TA_FRACTAL(0, g_nPoints - 1, g_high, g_low, 2, 2, &outBegIdx, &outNBElement, g_outIntBuf0, g_outIntBuf1);
    ICOUNT_DUMP("FRACTAL/batch");
    icount_row(nm, "batch", 1, rc);
    acc += (double)g_outIntBuf0[0];
    acc += (double)g_outIntBuf1[0];

    ICOUNT_ZERO();
    rc = TA_FRACTAL_OpenAndFill(&stf, g_high, g_low, g_nPoints, 2, 2, &outBegIdx, &outNBElement, g_outIntBuf0, g_outIntBuf1);
    ICOUNT_DUMP("FRACTAL/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += (double)g_outIntBuf0[0];
    acc += (double)g_outIntBuf1[0];
    if( stf ) TA_FRACTAL_Close(stf);

    ICOUNT_ZERO();
    rc = TA_FRACTAL_Open(&st, g_high, g_low, g_nPoints, 2, 2, &iv0, &iv1);
    ICOUNT_DUMP("FRACTAL/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_FRACTAL_Update(st, g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], &iv0, &iv1);
            acc += (double)iv0;
            acc += (double)iv1;
        }
        ICOUNT_DUMP("FRACTAL/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_FRACTAL_Peek(st, g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], &iv0, &iv1);
            acc += (double)iv0;
            acc += (double)iv1;
        }
        ICOUNT_DUMP("FRACTAL/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_FRACTAL_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_HA(int iters) {
    const char *nm = "HA";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_HA_Stream *st = NULL;
    TA_HA_Stream *stf = NULL;
    double v0 = 0.0;
    double v1 = 0.0;
    double v2 = 0.0;
    double v3 = 0.0;

    ICOUNT_ZERO();
    rc = TA_HA(0, g_nPoints - 1, g_open, g_high, g_low, g_close, &outBegIdx, &outNBElement, g_outBuf0, g_outBuf1, g_outBuf2, g_outBuf3);
    ICOUNT_DUMP("HA/batch");
    icount_row(nm, "batch", 1, rc);
    acc += g_outBuf0[0];
    acc += g_outBuf1[0];
    acc += g_outBuf2[0];
    acc += g_outBuf3[0];

    ICOUNT_ZERO();
    rc = TA_HA_OpenAndFill(&stf, g_open, g_high, g_low, g_close, g_nPoints, &outBegIdx, &outNBElement, g_outBuf0, g_outBuf1, g_outBuf2, g_outBuf3);
    ICOUNT_DUMP("HA/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += g_outBuf0[0];
    acc += g_outBuf1[0];
    acc += g_outBuf2[0];
    acc += g_outBuf3[0];
    if( stf ) TA_HA_Close(stf);

    ICOUNT_ZERO();
    rc = TA_HA_Open(&st, g_open, g_high, g_low, g_close, g_nPoints, &v0, &v1, &v2, &v3);
    ICOUNT_DUMP("HA/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_HA_Update(st, g_open[it & ICOUNT_MASK], g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &v0, &v1, &v2, &v3);
            acc += v0;
            acc += v1;
            acc += v2;
            acc += v3;
        }
        ICOUNT_DUMP("HA/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_HA_Peek(st, g_open[it & ICOUNT_MASK], g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &v0, &v1, &v2, &v3);
            acc += v0;
            acc += v1;
            acc += v2;
            acc += v3;
        }
        ICOUNT_DUMP("HA/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_HA_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_HMA(int iters) {
    const char *nm = "HMA";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_HMA_Stream *st = NULL;
    TA_HMA_Stream *stf = NULL;
    double v0 = 0.0;

    ICOUNT_ZERO();
    rc = TA_HMA(0, g_nPoints - 1, g_close, 20, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("HMA/batch");
    icount_row(nm, "batch", 1, rc);
    acc += g_outBuf0[0];

    ICOUNT_ZERO();
    rc = TA_HMA_OpenAndFill(&stf, g_close, g_nPoints, 20, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("HMA/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += g_outBuf0[0];
    if( stf ) TA_HMA_Close(stf);

    ICOUNT_ZERO();
    rc = TA_HMA_Open(&st, g_close, g_nPoints, 20, &v0);
    ICOUNT_DUMP("HMA/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_HMA_Update(st, g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("HMA/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_HMA_Peek(st, g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("HMA/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_HMA_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_HT_DCPERIOD(int iters) {
    const char *nm = "HT_DCPERIOD";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_HT_DCPERIOD_Stream *st = NULL;
    TA_HT_DCPERIOD_Stream *stf = NULL;
    double v0 = 0.0;

    ICOUNT_ZERO();
    rc = TA_HT_DCPERIOD(0, g_nPoints - 1, g_close, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("HT_DCPERIOD/batch");
    icount_row(nm, "batch", 1, rc);
    acc += g_outBuf0[0];

    ICOUNT_ZERO();
    rc = TA_HT_DCPERIOD_OpenAndFill(&stf, g_close, g_nPoints, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("HT_DCPERIOD/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += g_outBuf0[0];
    if( stf ) TA_HT_DCPERIOD_Close(stf);

    ICOUNT_ZERO();
    rc = TA_HT_DCPERIOD_Open(&st, g_close, g_nPoints, &v0);
    ICOUNT_DUMP("HT_DCPERIOD/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_HT_DCPERIOD_Update(st, g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("HT_DCPERIOD/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_HT_DCPERIOD_Peek(st, g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("HT_DCPERIOD/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_HT_DCPERIOD_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_HT_DCPHASE(int iters) {
    const char *nm = "HT_DCPHASE";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_HT_DCPHASE_Stream *st = NULL;
    TA_HT_DCPHASE_Stream *stf = NULL;
    double v0 = 0.0;

    ICOUNT_ZERO();
    rc = TA_HT_DCPHASE(0, g_nPoints - 1, g_close, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("HT_DCPHASE/batch");
    icount_row(nm, "batch", 1, rc);
    acc += g_outBuf0[0];

    ICOUNT_ZERO();
    rc = TA_HT_DCPHASE_OpenAndFill(&stf, g_close, g_nPoints, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("HT_DCPHASE/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += g_outBuf0[0];
    if( stf ) TA_HT_DCPHASE_Close(stf);

    ICOUNT_ZERO();
    rc = TA_HT_DCPHASE_Open(&st, g_close, g_nPoints, &v0);
    ICOUNT_DUMP("HT_DCPHASE/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_HT_DCPHASE_Update(st, g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("HT_DCPHASE/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_HT_DCPHASE_Peek(st, g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("HT_DCPHASE/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_HT_DCPHASE_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_HT_PHASOR(int iters) {
    const char *nm = "HT_PHASOR";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_HT_PHASOR_Stream *st = NULL;
    TA_HT_PHASOR_Stream *stf = NULL;
    double v0 = 0.0;
    double v1 = 0.0;

    ICOUNT_ZERO();
    rc = TA_HT_PHASOR(0, g_nPoints - 1, g_close, &outBegIdx, &outNBElement, g_outBuf0, g_outBuf1);
    ICOUNT_DUMP("HT_PHASOR/batch");
    icount_row(nm, "batch", 1, rc);
    acc += g_outBuf0[0];
    acc += g_outBuf1[0];

    ICOUNT_ZERO();
    rc = TA_HT_PHASOR_OpenAndFill(&stf, g_close, g_nPoints, &outBegIdx, &outNBElement, g_outBuf0, g_outBuf1);
    ICOUNT_DUMP("HT_PHASOR/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += g_outBuf0[0];
    acc += g_outBuf1[0];
    if( stf ) TA_HT_PHASOR_Close(stf);

    ICOUNT_ZERO();
    rc = TA_HT_PHASOR_Open(&st, g_close, g_nPoints, &v0, &v1);
    ICOUNT_DUMP("HT_PHASOR/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_HT_PHASOR_Update(st, g_close[it & ICOUNT_MASK], &v0, &v1);
            acc += v0;
            acc += v1;
        }
        ICOUNT_DUMP("HT_PHASOR/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_HT_PHASOR_Peek(st, g_close[it & ICOUNT_MASK], &v0, &v1);
            acc += v0;
            acc += v1;
        }
        ICOUNT_DUMP("HT_PHASOR/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_HT_PHASOR_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_HT_SINE(int iters) {
    const char *nm = "HT_SINE";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_HT_SINE_Stream *st = NULL;
    TA_HT_SINE_Stream *stf = NULL;
    double v0 = 0.0;
    double v1 = 0.0;

    ICOUNT_ZERO();
    rc = TA_HT_SINE(0, g_nPoints - 1, g_close, &outBegIdx, &outNBElement, g_outBuf0, g_outBuf1);
    ICOUNT_DUMP("HT_SINE/batch");
    icount_row(nm, "batch", 1, rc);
    acc += g_outBuf0[0];
    acc += g_outBuf1[0];

    ICOUNT_ZERO();
    rc = TA_HT_SINE_OpenAndFill(&stf, g_close, g_nPoints, &outBegIdx, &outNBElement, g_outBuf0, g_outBuf1);
    ICOUNT_DUMP("HT_SINE/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += g_outBuf0[0];
    acc += g_outBuf1[0];
    if( stf ) TA_HT_SINE_Close(stf);

    ICOUNT_ZERO();
    rc = TA_HT_SINE_Open(&st, g_close, g_nPoints, &v0, &v1);
    ICOUNT_DUMP("HT_SINE/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_HT_SINE_Update(st, g_close[it & ICOUNT_MASK], &v0, &v1);
            acc += v0;
            acc += v1;
        }
        ICOUNT_DUMP("HT_SINE/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_HT_SINE_Peek(st, g_close[it & ICOUNT_MASK], &v0, &v1);
            acc += v0;
            acc += v1;
        }
        ICOUNT_DUMP("HT_SINE/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_HT_SINE_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_HT_TRENDLINE(int iters) {
    const char *nm = "HT_TRENDLINE";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_HT_TRENDLINE_Stream *st = NULL;
    TA_HT_TRENDLINE_Stream *stf = NULL;
    double v0 = 0.0;

    ICOUNT_ZERO();
    rc = TA_HT_TRENDLINE(0, g_nPoints - 1, g_close, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("HT_TRENDLINE/batch");
    icount_row(nm, "batch", 1, rc);
    acc += g_outBuf0[0];

    ICOUNT_ZERO();
    rc = TA_HT_TRENDLINE_OpenAndFill(&stf, g_close, g_nPoints, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("HT_TRENDLINE/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += g_outBuf0[0];
    if( stf ) TA_HT_TRENDLINE_Close(stf);

    ICOUNT_ZERO();
    rc = TA_HT_TRENDLINE_Open(&st, g_close, g_nPoints, &v0);
    ICOUNT_DUMP("HT_TRENDLINE/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_HT_TRENDLINE_Update(st, g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("HT_TRENDLINE/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_HT_TRENDLINE_Peek(st, g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("HT_TRENDLINE/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_HT_TRENDLINE_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_HT_TRENDMODE(int iters) {
    const char *nm = "HT_TRENDMODE";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_HT_TRENDMODE_Stream *st = NULL;
    TA_HT_TRENDMODE_Stream *stf = NULL;
    int iv0 = 0;

    ICOUNT_ZERO();
    rc = TA_HT_TRENDMODE(0, g_nPoints - 1, g_close, &outBegIdx, &outNBElement, g_outIntBuf0);
    ICOUNT_DUMP("HT_TRENDMODE/batch");
    icount_row(nm, "batch", 1, rc);
    acc += (double)g_outIntBuf0[0];

    ICOUNT_ZERO();
    rc = TA_HT_TRENDMODE_OpenAndFill(&stf, g_close, g_nPoints, &outBegIdx, &outNBElement, g_outIntBuf0);
    ICOUNT_DUMP("HT_TRENDMODE/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += (double)g_outIntBuf0[0];
    if( stf ) TA_HT_TRENDMODE_Close(stf);

    ICOUNT_ZERO();
    rc = TA_HT_TRENDMODE_Open(&st, g_close, g_nPoints, &iv0);
    ICOUNT_DUMP("HT_TRENDMODE/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_HT_TRENDMODE_Update(st, g_close[it & ICOUNT_MASK], &iv0);
            acc += (double)iv0;
        }
        ICOUNT_DUMP("HT_TRENDMODE/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_HT_TRENDMODE_Peek(st, g_close[it & ICOUNT_MASK], &iv0);
            acc += (double)iv0;
        }
        ICOUNT_DUMP("HT_TRENDMODE/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_HT_TRENDMODE_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_IMI(int iters) {
    const char *nm = "IMI";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_IMI_Stream *st = NULL;
    TA_IMI_Stream *stf = NULL;
    double v0 = 0.0;

    ICOUNT_ZERO();
    rc = TA_IMI(0, g_nPoints - 1, g_open, g_close, 14, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("IMI/batch");
    icount_row(nm, "batch", 1, rc);
    acc += g_outBuf0[0];

    ICOUNT_ZERO();
    rc = TA_IMI_OpenAndFill(&stf, g_open, g_close, g_nPoints, 14, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("IMI/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += g_outBuf0[0];
    if( stf ) TA_IMI_Close(stf);

    ICOUNT_ZERO();
    rc = TA_IMI_Open(&st, g_open, g_close, g_nPoints, 14, &v0);
    ICOUNT_DUMP("IMI/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_IMI_Update(st, g_open[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("IMI/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_IMI_Peek(st, g_open[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("IMI/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_IMI_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_KAMA(int iters) {
    const char *nm = "KAMA";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_KAMA_Stream *st = NULL;
    TA_KAMA_Stream *stf = NULL;
    double v0 = 0.0;

    ICOUNT_ZERO();
    rc = TA_KAMA(0, g_nPoints - 1, g_close, 30, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("KAMA/batch");
    icount_row(nm, "batch", 1, rc);
    acc += g_outBuf0[0];

    ICOUNT_ZERO();
    rc = TA_KAMA_OpenAndFill(&stf, g_close, g_nPoints, 30, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("KAMA/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += g_outBuf0[0];
    if( stf ) TA_KAMA_Close(stf);

    ICOUNT_ZERO();
    rc = TA_KAMA_Open(&st, g_close, g_nPoints, 30, &v0);
    ICOUNT_DUMP("KAMA/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_KAMA_Update(st, g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("KAMA/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_KAMA_Peek(st, g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("KAMA/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_KAMA_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_KC(int iters) {
    const char *nm = "KC";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_KC_Stream *st = NULL;
    TA_KC_Stream *stf = NULL;
    double v0 = 0.0;
    double v1 = 0.0;
    double v2 = 0.0;

    ICOUNT_ZERO();
    rc = TA_KC(0, g_nPoints - 1, g_high, g_low, g_close, 20, 10, 2.000000000000000, &outBegIdx, &outNBElement, g_outBuf0, g_outBuf1, g_outBuf2);
    ICOUNT_DUMP("KC/batch");
    icount_row(nm, "batch", 1, rc);
    acc += g_outBuf0[0];
    acc += g_outBuf1[0];
    acc += g_outBuf2[0];

    ICOUNT_ZERO();
    rc = TA_KC_OpenAndFill(&stf, g_high, g_low, g_close, g_nPoints, 20, 10, 2.000000000000000, &outBegIdx, &outNBElement, g_outBuf0, g_outBuf1, g_outBuf2);
    ICOUNT_DUMP("KC/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += g_outBuf0[0];
    acc += g_outBuf1[0];
    acc += g_outBuf2[0];
    if( stf ) TA_KC_Close(stf);

    ICOUNT_ZERO();
    rc = TA_KC_Open(&st, g_high, g_low, g_close, g_nPoints, 20, 10, 2.000000000000000, &v0, &v1, &v2);
    ICOUNT_DUMP("KC/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_KC_Update(st, g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &v0, &v1, &v2);
            acc += v0;
            acc += v1;
            acc += v2;
        }
        ICOUNT_DUMP("KC/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_KC_Peek(st, g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &v0, &v1, &v2);
            acc += v0;
            acc += v1;
            acc += v2;
        }
        ICOUNT_DUMP("KC/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_KC_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_KDJ(int iters) {
    const char *nm = "KDJ";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_KDJ_Stream *st = NULL;
    TA_KDJ_Stream *stf = NULL;
    double v0 = 0.0;
    double v1 = 0.0;
    double v2 = 0.0;

    ICOUNT_ZERO();
    rc = TA_KDJ(0, g_nPoints - 1, g_high, g_low, g_close, 9, 3, 13, 3, 13, &outBegIdx, &outNBElement, g_outBuf0, g_outBuf1, g_outBuf2);
    ICOUNT_DUMP("KDJ/batch");
    icount_row(nm, "batch", 1, rc);
    acc += g_outBuf0[0];
    acc += g_outBuf1[0];
    acc += g_outBuf2[0];

    ICOUNT_ZERO();
    rc = TA_KDJ_OpenAndFill(&stf, g_high, g_low, g_close, g_nPoints, 9, 3, 13, 3, 13, &outBegIdx, &outNBElement, g_outBuf0, g_outBuf1, g_outBuf2);
    ICOUNT_DUMP("KDJ/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += g_outBuf0[0];
    acc += g_outBuf1[0];
    acc += g_outBuf2[0];
    if( stf ) TA_KDJ_Close(stf);

    ICOUNT_ZERO();
    rc = TA_KDJ_Open(&st, g_high, g_low, g_close, g_nPoints, 9, 3, 13, 3, 13, &v0, &v1, &v2);
    ICOUNT_DUMP("KDJ/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_KDJ_Update(st, g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &v0, &v1, &v2);
            acc += v0;
            acc += v1;
            acc += v2;
        }
        ICOUNT_DUMP("KDJ/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_KDJ_Peek(st, g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &v0, &v1, &v2);
            acc += v0;
            acc += v1;
            acc += v2;
        }
        ICOUNT_DUMP("KDJ/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_KDJ_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_LINEARREG(int iters) {
    const char *nm = "LINEARREG";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_LINEARREG_Stream *st = NULL;
    TA_LINEARREG_Stream *stf = NULL;
    double v0 = 0.0;

    ICOUNT_ZERO();
    rc = TA_LINEARREG(0, g_nPoints - 1, g_close, 14, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("LINEARREG/batch");
    icount_row(nm, "batch", 1, rc);
    acc += g_outBuf0[0];

    ICOUNT_ZERO();
    rc = TA_LINEARREG_OpenAndFill(&stf, g_close, g_nPoints, 14, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("LINEARREG/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += g_outBuf0[0];
    if( stf ) TA_LINEARREG_Close(stf);

    ICOUNT_ZERO();
    rc = TA_LINEARREG_Open(&st, g_close, g_nPoints, 14, &v0);
    ICOUNT_DUMP("LINEARREG/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_LINEARREG_Update(st, g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("LINEARREG/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_LINEARREG_Peek(st, g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("LINEARREG/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_LINEARREG_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_LINEARREG_ANGLE(int iters) {
    const char *nm = "LINEARREG_ANGLE";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_LINEARREG_ANGLE_Stream *st = NULL;
    TA_LINEARREG_ANGLE_Stream *stf = NULL;
    double v0 = 0.0;

    ICOUNT_ZERO();
    rc = TA_LINEARREG_ANGLE(0, g_nPoints - 1, g_close, 14, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("LINEARREG_ANGLE/batch");
    icount_row(nm, "batch", 1, rc);
    acc += g_outBuf0[0];

    ICOUNT_ZERO();
    rc = TA_LINEARREG_ANGLE_OpenAndFill(&stf, g_close, g_nPoints, 14, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("LINEARREG_ANGLE/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += g_outBuf0[0];
    if( stf ) TA_LINEARREG_ANGLE_Close(stf);

    ICOUNT_ZERO();
    rc = TA_LINEARREG_ANGLE_Open(&st, g_close, g_nPoints, 14, &v0);
    ICOUNT_DUMP("LINEARREG_ANGLE/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_LINEARREG_ANGLE_Update(st, g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("LINEARREG_ANGLE/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_LINEARREG_ANGLE_Peek(st, g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("LINEARREG_ANGLE/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_LINEARREG_ANGLE_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_LINEARREG_INTERCEPT(int iters) {
    const char *nm = "LINEARREG_INTERCEPT";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_LINEARREG_INTERCEPT_Stream *st = NULL;
    TA_LINEARREG_INTERCEPT_Stream *stf = NULL;
    double v0 = 0.0;

    ICOUNT_ZERO();
    rc = TA_LINEARREG_INTERCEPT(0, g_nPoints - 1, g_close, 14, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("LINEARREG_INTERCEPT/batch");
    icount_row(nm, "batch", 1, rc);
    acc += g_outBuf0[0];

    ICOUNT_ZERO();
    rc = TA_LINEARREG_INTERCEPT_OpenAndFill(&stf, g_close, g_nPoints, 14, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("LINEARREG_INTERCEPT/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += g_outBuf0[0];
    if( stf ) TA_LINEARREG_INTERCEPT_Close(stf);

    ICOUNT_ZERO();
    rc = TA_LINEARREG_INTERCEPT_Open(&st, g_close, g_nPoints, 14, &v0);
    ICOUNT_DUMP("LINEARREG_INTERCEPT/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_LINEARREG_INTERCEPT_Update(st, g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("LINEARREG_INTERCEPT/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_LINEARREG_INTERCEPT_Peek(st, g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("LINEARREG_INTERCEPT/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_LINEARREG_INTERCEPT_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_LINEARREG_SLOPE(int iters) {
    const char *nm = "LINEARREG_SLOPE";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_LINEARREG_SLOPE_Stream *st = NULL;
    TA_LINEARREG_SLOPE_Stream *stf = NULL;
    double v0 = 0.0;

    ICOUNT_ZERO();
    rc = TA_LINEARREG_SLOPE(0, g_nPoints - 1, g_close, 14, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("LINEARREG_SLOPE/batch");
    icount_row(nm, "batch", 1, rc);
    acc += g_outBuf0[0];

    ICOUNT_ZERO();
    rc = TA_LINEARREG_SLOPE_OpenAndFill(&stf, g_close, g_nPoints, 14, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("LINEARREG_SLOPE/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += g_outBuf0[0];
    if( stf ) TA_LINEARREG_SLOPE_Close(stf);

    ICOUNT_ZERO();
    rc = TA_LINEARREG_SLOPE_Open(&st, g_close, g_nPoints, 14, &v0);
    ICOUNT_DUMP("LINEARREG_SLOPE/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_LINEARREG_SLOPE_Update(st, g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("LINEARREG_SLOPE/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_LINEARREG_SLOPE_Peek(st, g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("LINEARREG_SLOPE/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_LINEARREG_SLOPE_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_LN(int iters) {
    const char *nm = "LN";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_LN_Stream *st = NULL;
    TA_LN_Stream *stf = NULL;
    double v0 = 0.0;

    ICOUNT_ZERO();
    rc = TA_LN(0, g_nPoints - 1, g_close, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("LN/batch");
    icount_row(nm, "batch", 1, rc);
    acc += g_outBuf0[0];

    ICOUNT_ZERO();
    rc = TA_LN_OpenAndFill(&stf, g_close, g_nPoints, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("LN/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += g_outBuf0[0];
    if( stf ) TA_LN_Close(stf);

    ICOUNT_ZERO();
    rc = TA_LN_Open(&st, g_close, g_nPoints, &v0);
    ICOUNT_DUMP("LN/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_LN_Update(st, g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("LN/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_LN_Peek(st, g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("LN/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_LN_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_LOG10(int iters) {
    const char *nm = "LOG10";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_LOG10_Stream *st = NULL;
    TA_LOG10_Stream *stf = NULL;
    double v0 = 0.0;

    ICOUNT_ZERO();
    rc = TA_LOG10(0, g_nPoints - 1, g_close, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("LOG10/batch");
    icount_row(nm, "batch", 1, rc);
    acc += g_outBuf0[0];

    ICOUNT_ZERO();
    rc = TA_LOG10_OpenAndFill(&stf, g_close, g_nPoints, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("LOG10/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += g_outBuf0[0];
    if( stf ) TA_LOG10_Close(stf);

    ICOUNT_ZERO();
    rc = TA_LOG10_Open(&st, g_close, g_nPoints, &v0);
    ICOUNT_DUMP("LOG10/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_LOG10_Update(st, g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("LOG10/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_LOG10_Peek(st, g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("LOG10/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_LOG10_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_MA(int iters) {
    const char *nm = "MA";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_MA_Stream *st = NULL;
    TA_MA_Stream *stf = NULL;
    double v0 = 0.0;

    ICOUNT_ZERO();
    rc = TA_MA(0, g_nPoints - 1, g_close, 30, 0, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("MA/batch");
    icount_row(nm, "batch", 1, rc);
    acc += g_outBuf0[0];

    ICOUNT_ZERO();
    rc = TA_MA_OpenAndFill(&stf, g_close, g_nPoints, 30, 0, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("MA/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += g_outBuf0[0];
    if( stf ) TA_MA_Close(stf);

    ICOUNT_ZERO();
    rc = TA_MA_Open(&st, g_close, g_nPoints, 30, 0, &v0);
    ICOUNT_DUMP("MA/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_MA_Update(st, g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("MA/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_MA_Peek(st, g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("MA/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_MA_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_MACD(int iters) {
    const char *nm = "MACD";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_MACD_Stream *st = NULL;
    TA_MACD_Stream *stf = NULL;
    double v0 = 0.0;
    double v1 = 0.0;
    double v2 = 0.0;

    ICOUNT_ZERO();
    rc = TA_MACD(0, g_nPoints - 1, g_close, 12, 26, 9, &outBegIdx, &outNBElement, g_outBuf0, g_outBuf1, g_outBuf2);
    ICOUNT_DUMP("MACD/batch");
    icount_row(nm, "batch", 1, rc);
    acc += g_outBuf0[0];
    acc += g_outBuf1[0];
    acc += g_outBuf2[0];

    ICOUNT_ZERO();
    rc = TA_MACD_OpenAndFill(&stf, g_close, g_nPoints, 12, 26, 9, &outBegIdx, &outNBElement, g_outBuf0, g_outBuf1, g_outBuf2);
    ICOUNT_DUMP("MACD/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += g_outBuf0[0];
    acc += g_outBuf1[0];
    acc += g_outBuf2[0];
    if( stf ) TA_MACD_Close(stf);

    ICOUNT_ZERO();
    rc = TA_MACD_Open(&st, g_close, g_nPoints, 12, 26, 9, &v0, &v1, &v2);
    ICOUNT_DUMP("MACD/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_MACD_Update(st, g_close[it & ICOUNT_MASK], &v0, &v1, &v2);
            acc += v0;
            acc += v1;
            acc += v2;
        }
        ICOUNT_DUMP("MACD/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_MACD_Peek(st, g_close[it & ICOUNT_MASK], &v0, &v1, &v2);
            acc += v0;
            acc += v1;
            acc += v2;
        }
        ICOUNT_DUMP("MACD/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_MACD_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_MACDEXT(int iters) {
    const char *nm = "MACDEXT";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_MACDEXT_Stream *st = NULL;
    TA_MACDEXT_Stream *stf = NULL;
    double v0 = 0.0;
    double v1 = 0.0;
    double v2 = 0.0;

    ICOUNT_ZERO();
    rc = TA_MACDEXT(0, g_nPoints - 1, g_close, 12, 0, 26, 0, 9, 0, &outBegIdx, &outNBElement, g_outBuf0, g_outBuf1, g_outBuf2);
    ICOUNT_DUMP("MACDEXT/batch");
    icount_row(nm, "batch", 1, rc);
    acc += g_outBuf0[0];
    acc += g_outBuf1[0];
    acc += g_outBuf2[0];

    ICOUNT_ZERO();
    rc = TA_MACDEXT_OpenAndFill(&stf, g_close, g_nPoints, 12, 0, 26, 0, 9, 0, &outBegIdx, &outNBElement, g_outBuf0, g_outBuf1, g_outBuf2);
    ICOUNT_DUMP("MACDEXT/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += g_outBuf0[0];
    acc += g_outBuf1[0];
    acc += g_outBuf2[0];
    if( stf ) TA_MACDEXT_Close(stf);

    ICOUNT_ZERO();
    rc = TA_MACDEXT_Open(&st, g_close, g_nPoints, 12, 0, 26, 0, 9, 0, &v0, &v1, &v2);
    ICOUNT_DUMP("MACDEXT/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_MACDEXT_Update(st, g_close[it & ICOUNT_MASK], &v0, &v1, &v2);
            acc += v0;
            acc += v1;
            acc += v2;
        }
        ICOUNT_DUMP("MACDEXT/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_MACDEXT_Peek(st, g_close[it & ICOUNT_MASK], &v0, &v1, &v2);
            acc += v0;
            acc += v1;
            acc += v2;
        }
        ICOUNT_DUMP("MACDEXT/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_MACDEXT_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_MACDFIX(int iters) {
    const char *nm = "MACDFIX";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_MACDFIX_Stream *st = NULL;
    TA_MACDFIX_Stream *stf = NULL;
    double v0 = 0.0;
    double v1 = 0.0;
    double v2 = 0.0;

    ICOUNT_ZERO();
    rc = TA_MACDFIX(0, g_nPoints - 1, g_close, 9, &outBegIdx, &outNBElement, g_outBuf0, g_outBuf1, g_outBuf2);
    ICOUNT_DUMP("MACDFIX/batch");
    icount_row(nm, "batch", 1, rc);
    acc += g_outBuf0[0];
    acc += g_outBuf1[0];
    acc += g_outBuf2[0];

    ICOUNT_ZERO();
    rc = TA_MACDFIX_OpenAndFill(&stf, g_close, g_nPoints, 9, &outBegIdx, &outNBElement, g_outBuf0, g_outBuf1, g_outBuf2);
    ICOUNT_DUMP("MACDFIX/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += g_outBuf0[0];
    acc += g_outBuf1[0];
    acc += g_outBuf2[0];
    if( stf ) TA_MACDFIX_Close(stf);

    ICOUNT_ZERO();
    rc = TA_MACDFIX_Open(&st, g_close, g_nPoints, 9, &v0, &v1, &v2);
    ICOUNT_DUMP("MACDFIX/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_MACDFIX_Update(st, g_close[it & ICOUNT_MASK], &v0, &v1, &v2);
            acc += v0;
            acc += v1;
            acc += v2;
        }
        ICOUNT_DUMP("MACDFIX/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_MACDFIX_Peek(st, g_close[it & ICOUNT_MASK], &v0, &v1, &v2);
            acc += v0;
            acc += v1;
            acc += v2;
        }
        ICOUNT_DUMP("MACDFIX/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_MACDFIX_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_MAMA(int iters) {
    const char *nm = "MAMA";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_MAMA_Stream *st = NULL;
    TA_MAMA_Stream *stf = NULL;
    double v0 = 0.0;
    double v1 = 0.0;

    ICOUNT_ZERO();
    rc = TA_MAMA(0, g_nPoints - 1, g_close, 0.500000000000000, 0.050000000000000, &outBegIdx, &outNBElement, g_outBuf0, g_outBuf1);
    ICOUNT_DUMP("MAMA/batch");
    icount_row(nm, "batch", 1, rc);
    acc += g_outBuf0[0];
    acc += g_outBuf1[0];

    ICOUNT_ZERO();
    rc = TA_MAMA_OpenAndFill(&stf, g_close, g_nPoints, 0.500000000000000, 0.050000000000000, &outBegIdx, &outNBElement, g_outBuf0, g_outBuf1);
    ICOUNT_DUMP("MAMA/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += g_outBuf0[0];
    acc += g_outBuf1[0];
    if( stf ) TA_MAMA_Close(stf);

    ICOUNT_ZERO();
    rc = TA_MAMA_Open(&st, g_close, g_nPoints, 0.500000000000000, 0.050000000000000, &v0, &v1);
    ICOUNT_DUMP("MAMA/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_MAMA_Update(st, g_close[it & ICOUNT_MASK], &v0, &v1);
            acc += v0;
            acc += v1;
        }
        ICOUNT_DUMP("MAMA/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_MAMA_Peek(st, g_close[it & ICOUNT_MASK], &v0, &v1);
            acc += v0;
            acc += v1;
        }
        ICOUNT_DUMP("MAMA/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_MAMA_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_MARKETFI(int iters) {
    const char *nm = "MARKETFI";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_MARKETFI_Stream *st = NULL;
    TA_MARKETFI_Stream *stf = NULL;
    double v0 = 0.0;

    ICOUNT_ZERO();
    rc = TA_MARKETFI(0, g_nPoints - 1, g_high, g_low, g_volume, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("MARKETFI/batch");
    icount_row(nm, "batch", 1, rc);
    acc += g_outBuf0[0];

    ICOUNT_ZERO();
    rc = TA_MARKETFI_OpenAndFill(&stf, g_high, g_low, g_volume, g_nPoints, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("MARKETFI/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += g_outBuf0[0];
    if( stf ) TA_MARKETFI_Close(stf);

    ICOUNT_ZERO();
    rc = TA_MARKETFI_Open(&st, g_high, g_low, g_volume, g_nPoints, &v0);
    ICOUNT_DUMP("MARKETFI/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_MARKETFI_Update(st, g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_volume[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("MARKETFI/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_MARKETFI_Peek(st, g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_volume[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("MARKETFI/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_MARKETFI_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_MASSI(int iters) {
    const char *nm = "MASSI";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_MASSI_Stream *st = NULL;
    TA_MASSI_Stream *stf = NULL;
    double v0 = 0.0;

    ICOUNT_ZERO();
    rc = TA_MASSI(0, g_nPoints - 1, g_high, g_low, 9, 25, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("MASSI/batch");
    icount_row(nm, "batch", 1, rc);
    acc += g_outBuf0[0];

    ICOUNT_ZERO();
    rc = TA_MASSI_OpenAndFill(&stf, g_high, g_low, g_nPoints, 9, 25, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("MASSI/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += g_outBuf0[0];
    if( stf ) TA_MASSI_Close(stf);

    ICOUNT_ZERO();
    rc = TA_MASSI_Open(&st, g_high, g_low, g_nPoints, 9, 25, &v0);
    ICOUNT_DUMP("MASSI/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_MASSI_Update(st, g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("MASSI/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_MASSI_Peek(st, g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("MASSI/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_MASSI_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_MAVP(int iters) {
    const char *nm = "MAVP";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_MAVP_Stream *st = NULL;
    TA_MAVP_Stream *stf = NULL;
    double v0 = 0.0;

    ICOUNT_ZERO();
    rc = TA_MAVP(0, g_nPoints - 1, g_close, g_periods, 2, 30, 0, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("MAVP/batch");
    icount_row(nm, "batch", 1, rc);
    acc += g_outBuf0[0];

    ICOUNT_ZERO();
    rc = TA_MAVP_OpenAndFill(&stf, g_close, g_periods, g_nPoints, 2, 30, 0, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("MAVP/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += g_outBuf0[0];
    if( stf ) TA_MAVP_Close(stf);

    ICOUNT_ZERO();
    rc = TA_MAVP_Open(&st, g_close, g_periods, g_nPoints, 2, 30, 0, &v0);
    ICOUNT_DUMP("MAVP/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_MAVP_Update(st, g_close[it & ICOUNT_MASK], g_periods[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("MAVP/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_MAVP_Peek(st, g_close[it & ICOUNT_MASK], g_periods[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("MAVP/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_MAVP_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_MAX(int iters) {
    const char *nm = "MAX";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_MAX_Stream *st = NULL;
    TA_MAX_Stream *stf = NULL;
    double v0 = 0.0;

    ICOUNT_ZERO();
    rc = TA_MAX(0, g_nPoints - 1, g_close, 30, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("MAX/batch");
    icount_row(nm, "batch", 1, rc);
    acc += g_outBuf0[0];

    ICOUNT_ZERO();
    rc = TA_MAX_OpenAndFill(&stf, g_close, g_nPoints, 30, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("MAX/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += g_outBuf0[0];
    if( stf ) TA_MAX_Close(stf);

    ICOUNT_ZERO();
    rc = TA_MAX_Open(&st, g_close, g_nPoints, 30, &v0);
    ICOUNT_DUMP("MAX/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_MAX_Update(st, g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("MAX/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_MAX_Peek(st, g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("MAX/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_MAX_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_MAXINDEX(int iters) {
    const char *nm = "MAXINDEX";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_MAXINDEX_Stream *st = NULL;
    TA_MAXINDEX_Stream *stf = NULL;
    int iv0 = 0;

    ICOUNT_ZERO();
    rc = TA_MAXINDEX(0, g_nPoints - 1, g_close, 30, &outBegIdx, &outNBElement, g_outIntBuf0);
    ICOUNT_DUMP("MAXINDEX/batch");
    icount_row(nm, "batch", 1, rc);
    acc += (double)g_outIntBuf0[0];

    ICOUNT_ZERO();
    rc = TA_MAXINDEX_OpenAndFill(&stf, g_close, g_nPoints, 30, &outBegIdx, &outNBElement, g_outIntBuf0);
    ICOUNT_DUMP("MAXINDEX/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += (double)g_outIntBuf0[0];
    if( stf ) TA_MAXINDEX_Close(stf);

    ICOUNT_ZERO();
    rc = TA_MAXINDEX_Open(&st, g_close, g_nPoints, 30, &iv0);
    ICOUNT_DUMP("MAXINDEX/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_MAXINDEX_Update(st, g_close[it & ICOUNT_MASK], &iv0);
            acc += (double)iv0;
        }
        ICOUNT_DUMP("MAXINDEX/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_MAXINDEX_Peek(st, g_close[it & ICOUNT_MASK], &iv0);
            acc += (double)iv0;
        }
        ICOUNT_DUMP("MAXINDEX/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_MAXINDEX_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_MEDPRICE(int iters) {
    const char *nm = "MEDPRICE";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_MEDPRICE_Stream *st = NULL;
    TA_MEDPRICE_Stream *stf = NULL;
    double v0 = 0.0;

    ICOUNT_ZERO();
    rc = TA_MEDPRICE(0, g_nPoints - 1, g_high, g_low, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("MEDPRICE/batch");
    icount_row(nm, "batch", 1, rc);
    acc += g_outBuf0[0];

    ICOUNT_ZERO();
    rc = TA_MEDPRICE_OpenAndFill(&stf, g_high, g_low, g_nPoints, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("MEDPRICE/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += g_outBuf0[0];
    if( stf ) TA_MEDPRICE_Close(stf);

    ICOUNT_ZERO();
    rc = TA_MEDPRICE_Open(&st, g_high, g_low, g_nPoints, &v0);
    ICOUNT_DUMP("MEDPRICE/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_MEDPRICE_Update(st, g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("MEDPRICE/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_MEDPRICE_Peek(st, g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("MEDPRICE/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_MEDPRICE_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_MFI(int iters) {
    const char *nm = "MFI";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_MFI_Stream *st = NULL;
    TA_MFI_Stream *stf = NULL;
    double v0 = 0.0;

    ICOUNT_ZERO();
    rc = TA_MFI(0, g_nPoints - 1, g_high, g_low, g_close, g_volume, 14, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("MFI/batch");
    icount_row(nm, "batch", 1, rc);
    acc += g_outBuf0[0];

    ICOUNT_ZERO();
    rc = TA_MFI_OpenAndFill(&stf, g_high, g_low, g_close, g_volume, g_nPoints, 14, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("MFI/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += g_outBuf0[0];
    if( stf ) TA_MFI_Close(stf);

    ICOUNT_ZERO();
    rc = TA_MFI_Open(&st, g_high, g_low, g_close, g_volume, g_nPoints, 14, &v0);
    ICOUNT_DUMP("MFI/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_MFI_Update(st, g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], g_volume[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("MFI/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_MFI_Peek(st, g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], g_volume[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("MFI/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_MFI_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_MIDPOINT(int iters) {
    const char *nm = "MIDPOINT";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_MIDPOINT_Stream *st = NULL;
    TA_MIDPOINT_Stream *stf = NULL;
    double v0 = 0.0;

    ICOUNT_ZERO();
    rc = TA_MIDPOINT(0, g_nPoints - 1, g_close, 14, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("MIDPOINT/batch");
    icount_row(nm, "batch", 1, rc);
    acc += g_outBuf0[0];

    ICOUNT_ZERO();
    rc = TA_MIDPOINT_OpenAndFill(&stf, g_close, g_nPoints, 14, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("MIDPOINT/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += g_outBuf0[0];
    if( stf ) TA_MIDPOINT_Close(stf);

    ICOUNT_ZERO();
    rc = TA_MIDPOINT_Open(&st, g_close, g_nPoints, 14, &v0);
    ICOUNT_DUMP("MIDPOINT/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_MIDPOINT_Update(st, g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("MIDPOINT/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_MIDPOINT_Peek(st, g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("MIDPOINT/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_MIDPOINT_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_MIDPRICE(int iters) {
    const char *nm = "MIDPRICE";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_MIDPRICE_Stream *st = NULL;
    TA_MIDPRICE_Stream *stf = NULL;
    double v0 = 0.0;

    ICOUNT_ZERO();
    rc = TA_MIDPRICE(0, g_nPoints - 1, g_high, g_low, 14, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("MIDPRICE/batch");
    icount_row(nm, "batch", 1, rc);
    acc += g_outBuf0[0];

    ICOUNT_ZERO();
    rc = TA_MIDPRICE_OpenAndFill(&stf, g_high, g_low, g_nPoints, 14, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("MIDPRICE/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += g_outBuf0[0];
    if( stf ) TA_MIDPRICE_Close(stf);

    ICOUNT_ZERO();
    rc = TA_MIDPRICE_Open(&st, g_high, g_low, g_nPoints, 14, &v0);
    ICOUNT_DUMP("MIDPRICE/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_MIDPRICE_Update(st, g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("MIDPRICE/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_MIDPRICE_Peek(st, g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("MIDPRICE/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_MIDPRICE_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_MIN(int iters) {
    const char *nm = "MIN";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_MIN_Stream *st = NULL;
    TA_MIN_Stream *stf = NULL;
    double v0 = 0.0;

    ICOUNT_ZERO();
    rc = TA_MIN(0, g_nPoints - 1, g_close, 30, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("MIN/batch");
    icount_row(nm, "batch", 1, rc);
    acc += g_outBuf0[0];

    ICOUNT_ZERO();
    rc = TA_MIN_OpenAndFill(&stf, g_close, g_nPoints, 30, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("MIN/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += g_outBuf0[0];
    if( stf ) TA_MIN_Close(stf);

    ICOUNT_ZERO();
    rc = TA_MIN_Open(&st, g_close, g_nPoints, 30, &v0);
    ICOUNT_DUMP("MIN/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_MIN_Update(st, g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("MIN/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_MIN_Peek(st, g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("MIN/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_MIN_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_MININDEX(int iters) {
    const char *nm = "MININDEX";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_MININDEX_Stream *st = NULL;
    TA_MININDEX_Stream *stf = NULL;
    int iv0 = 0;

    ICOUNT_ZERO();
    rc = TA_MININDEX(0, g_nPoints - 1, g_close, 30, &outBegIdx, &outNBElement, g_outIntBuf0);
    ICOUNT_DUMP("MININDEX/batch");
    icount_row(nm, "batch", 1, rc);
    acc += (double)g_outIntBuf0[0];

    ICOUNT_ZERO();
    rc = TA_MININDEX_OpenAndFill(&stf, g_close, g_nPoints, 30, &outBegIdx, &outNBElement, g_outIntBuf0);
    ICOUNT_DUMP("MININDEX/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += (double)g_outIntBuf0[0];
    if( stf ) TA_MININDEX_Close(stf);

    ICOUNT_ZERO();
    rc = TA_MININDEX_Open(&st, g_close, g_nPoints, 30, &iv0);
    ICOUNT_DUMP("MININDEX/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_MININDEX_Update(st, g_close[it & ICOUNT_MASK], &iv0);
            acc += (double)iv0;
        }
        ICOUNT_DUMP("MININDEX/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_MININDEX_Peek(st, g_close[it & ICOUNT_MASK], &iv0);
            acc += (double)iv0;
        }
        ICOUNT_DUMP("MININDEX/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_MININDEX_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_MINMAX(int iters) {
    const char *nm = "MINMAX";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_MINMAX_Stream *st = NULL;
    TA_MINMAX_Stream *stf = NULL;
    double v0 = 0.0;
    double v1 = 0.0;

    ICOUNT_ZERO();
    rc = TA_MINMAX(0, g_nPoints - 1, g_close, 30, &outBegIdx, &outNBElement, g_outBuf0, g_outBuf1);
    ICOUNT_DUMP("MINMAX/batch");
    icount_row(nm, "batch", 1, rc);
    acc += g_outBuf0[0];
    acc += g_outBuf1[0];

    ICOUNT_ZERO();
    rc = TA_MINMAX_OpenAndFill(&stf, g_close, g_nPoints, 30, &outBegIdx, &outNBElement, g_outBuf0, g_outBuf1);
    ICOUNT_DUMP("MINMAX/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += g_outBuf0[0];
    acc += g_outBuf1[0];
    if( stf ) TA_MINMAX_Close(stf);

    ICOUNT_ZERO();
    rc = TA_MINMAX_Open(&st, g_close, g_nPoints, 30, &v0, &v1);
    ICOUNT_DUMP("MINMAX/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_MINMAX_Update(st, g_close[it & ICOUNT_MASK], &v0, &v1);
            acc += v0;
            acc += v1;
        }
        ICOUNT_DUMP("MINMAX/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_MINMAX_Peek(st, g_close[it & ICOUNT_MASK], &v0, &v1);
            acc += v0;
            acc += v1;
        }
        ICOUNT_DUMP("MINMAX/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_MINMAX_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_MINMAXINDEX(int iters) {
    const char *nm = "MINMAXINDEX";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_MINMAXINDEX_Stream *st = NULL;
    TA_MINMAXINDEX_Stream *stf = NULL;
    int iv0 = 0;
    int iv1 = 0;

    ICOUNT_ZERO();
    rc = TA_MINMAXINDEX(0, g_nPoints - 1, g_close, 30, &outBegIdx, &outNBElement, g_outIntBuf0, g_outIntBuf1);
    ICOUNT_DUMP("MINMAXINDEX/batch");
    icount_row(nm, "batch", 1, rc);
    acc += (double)g_outIntBuf0[0];
    acc += (double)g_outIntBuf1[0];

    ICOUNT_ZERO();
    rc = TA_MINMAXINDEX_OpenAndFill(&stf, g_close, g_nPoints, 30, &outBegIdx, &outNBElement, g_outIntBuf0, g_outIntBuf1);
    ICOUNT_DUMP("MINMAXINDEX/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += (double)g_outIntBuf0[0];
    acc += (double)g_outIntBuf1[0];
    if( stf ) TA_MINMAXINDEX_Close(stf);

    ICOUNT_ZERO();
    rc = TA_MINMAXINDEX_Open(&st, g_close, g_nPoints, 30, &iv0, &iv1);
    ICOUNT_DUMP("MINMAXINDEX/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_MINMAXINDEX_Update(st, g_close[it & ICOUNT_MASK], &iv0, &iv1);
            acc += (double)iv0;
            acc += (double)iv1;
        }
        ICOUNT_DUMP("MINMAXINDEX/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_MINMAXINDEX_Peek(st, g_close[it & ICOUNT_MASK], &iv0, &iv1);
            acc += (double)iv0;
            acc += (double)iv1;
        }
        ICOUNT_DUMP("MINMAXINDEX/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_MINMAXINDEX_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_MINUS_DI(int iters) {
    const char *nm = "MINUS_DI";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_MINUS_DI_Stream *st = NULL;
    TA_MINUS_DI_Stream *stf = NULL;
    double v0 = 0.0;

    ICOUNT_ZERO();
    rc = TA_MINUS_DI(0, g_nPoints - 1, g_high, g_low, g_close, 14, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("MINUS_DI/batch");
    icount_row(nm, "batch", 1, rc);
    acc += g_outBuf0[0];

    ICOUNT_ZERO();
    rc = TA_MINUS_DI_OpenAndFill(&stf, g_high, g_low, g_close, g_nPoints, 14, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("MINUS_DI/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += g_outBuf0[0];
    if( stf ) TA_MINUS_DI_Close(stf);

    ICOUNT_ZERO();
    rc = TA_MINUS_DI_Open(&st, g_high, g_low, g_close, g_nPoints, 14, &v0);
    ICOUNT_DUMP("MINUS_DI/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_MINUS_DI_Update(st, g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("MINUS_DI/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_MINUS_DI_Peek(st, g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("MINUS_DI/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_MINUS_DI_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_MINUS_DM(int iters) {
    const char *nm = "MINUS_DM";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_MINUS_DM_Stream *st = NULL;
    TA_MINUS_DM_Stream *stf = NULL;
    double v0 = 0.0;

    ICOUNT_ZERO();
    rc = TA_MINUS_DM(0, g_nPoints - 1, g_high, g_low, 14, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("MINUS_DM/batch");
    icount_row(nm, "batch", 1, rc);
    acc += g_outBuf0[0];

    ICOUNT_ZERO();
    rc = TA_MINUS_DM_OpenAndFill(&stf, g_high, g_low, g_nPoints, 14, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("MINUS_DM/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += g_outBuf0[0];
    if( stf ) TA_MINUS_DM_Close(stf);

    ICOUNT_ZERO();
    rc = TA_MINUS_DM_Open(&st, g_high, g_low, g_nPoints, 14, &v0);
    ICOUNT_DUMP("MINUS_DM/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_MINUS_DM_Update(st, g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("MINUS_DM/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_MINUS_DM_Peek(st, g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("MINUS_DM/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_MINUS_DM_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_MOM(int iters) {
    const char *nm = "MOM";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_MOM_Stream *st = NULL;
    TA_MOM_Stream *stf = NULL;
    double v0 = 0.0;

    ICOUNT_ZERO();
    rc = TA_MOM(0, g_nPoints - 1, g_close, 10, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("MOM/batch");
    icount_row(nm, "batch", 1, rc);
    acc += g_outBuf0[0];

    ICOUNT_ZERO();
    rc = TA_MOM_OpenAndFill(&stf, g_close, g_nPoints, 10, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("MOM/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += g_outBuf0[0];
    if( stf ) TA_MOM_Close(stf);

    ICOUNT_ZERO();
    rc = TA_MOM_Open(&st, g_close, g_nPoints, 10, &v0);
    ICOUNT_DUMP("MOM/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_MOM_Update(st, g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("MOM/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_MOM_Peek(st, g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("MOM/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_MOM_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_MULT(int iters) {
    const char *nm = "MULT";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_MULT_Stream *st = NULL;
    TA_MULT_Stream *stf = NULL;
    double v0 = 0.0;

    ICOUNT_ZERO();
    rc = TA_MULT(0, g_nPoints - 1, g_close, g_high, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("MULT/batch");
    icount_row(nm, "batch", 1, rc);
    acc += g_outBuf0[0];

    ICOUNT_ZERO();
    rc = TA_MULT_OpenAndFill(&stf, g_close, g_high, g_nPoints, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("MULT/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += g_outBuf0[0];
    if( stf ) TA_MULT_Close(stf);

    ICOUNT_ZERO();
    rc = TA_MULT_Open(&st, g_close, g_high, g_nPoints, &v0);
    ICOUNT_DUMP("MULT/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_MULT_Update(st, g_close[it & ICOUNT_MASK], g_high[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("MULT/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_MULT_Peek(st, g_close[it & ICOUNT_MASK], g_high[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("MULT/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_MULT_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_NATR(int iters) {
    const char *nm = "NATR";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_NATR_Stream *st = NULL;
    TA_NATR_Stream *stf = NULL;
    double v0 = 0.0;

    ICOUNT_ZERO();
    rc = TA_NATR(0, g_nPoints - 1, g_high, g_low, g_close, 14, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("NATR/batch");
    icount_row(nm, "batch", 1, rc);
    acc += g_outBuf0[0];

    ICOUNT_ZERO();
    rc = TA_NATR_OpenAndFill(&stf, g_high, g_low, g_close, g_nPoints, 14, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("NATR/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += g_outBuf0[0];
    if( stf ) TA_NATR_Close(stf);

    ICOUNT_ZERO();
    rc = TA_NATR_Open(&st, g_high, g_low, g_close, g_nPoints, 14, &v0);
    ICOUNT_DUMP("NATR/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_NATR_Update(st, g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("NATR/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_NATR_Peek(st, g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("NATR/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_NATR_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_NVI(int iters) {
    const char *nm = "NVI";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_NVI_Stream *st = NULL;
    TA_NVI_Stream *stf = NULL;
    double v0 = 0.0;

    ICOUNT_ZERO();
    rc = TA_NVI(0, g_nPoints - 1, g_close, g_volume, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("NVI/batch");
    icount_row(nm, "batch", 1, rc);
    acc += g_outBuf0[0];

    ICOUNT_ZERO();
    rc = TA_NVI_OpenAndFill(&stf, g_close, g_volume, g_nPoints, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("NVI/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += g_outBuf0[0];
    if( stf ) TA_NVI_Close(stf);

    ICOUNT_ZERO();
    rc = TA_NVI_Open(&st, g_close, g_volume, g_nPoints, &v0);
    ICOUNT_DUMP("NVI/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_NVI_Update(st, g_close[it & ICOUNT_MASK], g_volume[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("NVI/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_NVI_Peek(st, g_close[it & ICOUNT_MASK], g_volume[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("NVI/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_NVI_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_OBV(int iters) {
    const char *nm = "OBV";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_OBV_Stream *st = NULL;
    TA_OBV_Stream *stf = NULL;
    double v0 = 0.0;

    ICOUNT_ZERO();
    rc = TA_OBV(0, g_nPoints - 1, g_close, g_volume, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("OBV/batch");
    icount_row(nm, "batch", 1, rc);
    acc += g_outBuf0[0];

    ICOUNT_ZERO();
    rc = TA_OBV_OpenAndFill(&stf, g_close, g_volume, g_nPoints, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("OBV/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += g_outBuf0[0];
    if( stf ) TA_OBV_Close(stf);

    ICOUNT_ZERO();
    rc = TA_OBV_Open(&st, g_close, g_volume, g_nPoints, &v0);
    ICOUNT_DUMP("OBV/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_OBV_Update(st, g_close[it & ICOUNT_MASK], g_volume[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("OBV/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_OBV_Peek(st, g_close[it & ICOUNT_MASK], g_volume[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("OBV/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_OBV_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_PERCENTILE(int iters) {
    const char *nm = "PERCENTILE";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_PERCENTILE_Stream *st = NULL;
    TA_PERCENTILE_Stream *stf = NULL;
    double v0 = 0.0;

    ICOUNT_ZERO();
    rc = TA_PERCENTILE(0, g_nPoints - 1, g_close, 30, 50.000000000000000, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("PERCENTILE/batch");
    icount_row(nm, "batch", 1, rc);
    acc += g_outBuf0[0];

    ICOUNT_ZERO();
    rc = TA_PERCENTILE_OpenAndFill(&stf, g_close, g_nPoints, 30, 50.000000000000000, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("PERCENTILE/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += g_outBuf0[0];
    if( stf ) TA_PERCENTILE_Close(stf);

    ICOUNT_ZERO();
    rc = TA_PERCENTILE_Open(&st, g_close, g_nPoints, 30, 50.000000000000000, &v0);
    ICOUNT_DUMP("PERCENTILE/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_PERCENTILE_Update(st, g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("PERCENTILE/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_PERCENTILE_Peek(st, g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("PERCENTILE/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_PERCENTILE_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_PERCENTRANK(int iters) {
    const char *nm = "PERCENTRANK";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_PERCENTRANK_Stream *st = NULL;
    TA_PERCENTRANK_Stream *stf = NULL;
    double v0 = 0.0;

    ICOUNT_ZERO();
    rc = TA_PERCENTRANK(0, g_nPoints - 1, g_close, 100, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("PERCENTRANK/batch");
    icount_row(nm, "batch", 1, rc);
    acc += g_outBuf0[0];

    ICOUNT_ZERO();
    rc = TA_PERCENTRANK_OpenAndFill(&stf, g_close, g_nPoints, 100, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("PERCENTRANK/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += g_outBuf0[0];
    if( stf ) TA_PERCENTRANK_Close(stf);

    ICOUNT_ZERO();
    rc = TA_PERCENTRANK_Open(&st, g_close, g_nPoints, 100, &v0);
    ICOUNT_DUMP("PERCENTRANK/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_PERCENTRANK_Update(st, g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("PERCENTRANK/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_PERCENTRANK_Peek(st, g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("PERCENTRANK/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_PERCENTRANK_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_PLUS_DI(int iters) {
    const char *nm = "PLUS_DI";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_PLUS_DI_Stream *st = NULL;
    TA_PLUS_DI_Stream *stf = NULL;
    double v0 = 0.0;

    ICOUNT_ZERO();
    rc = TA_PLUS_DI(0, g_nPoints - 1, g_high, g_low, g_close, 14, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("PLUS_DI/batch");
    icount_row(nm, "batch", 1, rc);
    acc += g_outBuf0[0];

    ICOUNT_ZERO();
    rc = TA_PLUS_DI_OpenAndFill(&stf, g_high, g_low, g_close, g_nPoints, 14, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("PLUS_DI/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += g_outBuf0[0];
    if( stf ) TA_PLUS_DI_Close(stf);

    ICOUNT_ZERO();
    rc = TA_PLUS_DI_Open(&st, g_high, g_low, g_close, g_nPoints, 14, &v0);
    ICOUNT_DUMP("PLUS_DI/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_PLUS_DI_Update(st, g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("PLUS_DI/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_PLUS_DI_Peek(st, g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("PLUS_DI/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_PLUS_DI_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_PLUS_DM(int iters) {
    const char *nm = "PLUS_DM";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_PLUS_DM_Stream *st = NULL;
    TA_PLUS_DM_Stream *stf = NULL;
    double v0 = 0.0;

    ICOUNT_ZERO();
    rc = TA_PLUS_DM(0, g_nPoints - 1, g_high, g_low, 14, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("PLUS_DM/batch");
    icount_row(nm, "batch", 1, rc);
    acc += g_outBuf0[0];

    ICOUNT_ZERO();
    rc = TA_PLUS_DM_OpenAndFill(&stf, g_high, g_low, g_nPoints, 14, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("PLUS_DM/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += g_outBuf0[0];
    if( stf ) TA_PLUS_DM_Close(stf);

    ICOUNT_ZERO();
    rc = TA_PLUS_DM_Open(&st, g_high, g_low, g_nPoints, 14, &v0);
    ICOUNT_DUMP("PLUS_DM/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_PLUS_DM_Update(st, g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("PLUS_DM/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_PLUS_DM_Peek(st, g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("PLUS_DM/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_PLUS_DM_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_PPO(int iters) {
    const char *nm = "PPO";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_PPO_Stream *st = NULL;
    TA_PPO_Stream *stf = NULL;
    double v0 = 0.0;

    ICOUNT_ZERO();
    rc = TA_PPO(0, g_nPoints - 1, g_close, 12, 26, 1, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("PPO/batch");
    icount_row(nm, "batch", 1, rc);
    acc += g_outBuf0[0];

    ICOUNT_ZERO();
    rc = TA_PPO_OpenAndFill(&stf, g_close, g_nPoints, 12, 26, 1, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("PPO/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += g_outBuf0[0];
    if( stf ) TA_PPO_Close(stf);

    ICOUNT_ZERO();
    rc = TA_PPO_Open(&st, g_close, g_nPoints, 12, 26, 1, &v0);
    ICOUNT_DUMP("PPO/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_PPO_Update(st, g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("PPO/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_PPO_Peek(st, g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("PPO/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_PPO_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_PVI(int iters) {
    const char *nm = "PVI";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_PVI_Stream *st = NULL;
    TA_PVI_Stream *stf = NULL;
    double v0 = 0.0;

    ICOUNT_ZERO();
    rc = TA_PVI(0, g_nPoints - 1, g_close, g_volume, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("PVI/batch");
    icount_row(nm, "batch", 1, rc);
    acc += g_outBuf0[0];

    ICOUNT_ZERO();
    rc = TA_PVI_OpenAndFill(&stf, g_close, g_volume, g_nPoints, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("PVI/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += g_outBuf0[0];
    if( stf ) TA_PVI_Close(stf);

    ICOUNT_ZERO();
    rc = TA_PVI_Open(&st, g_close, g_volume, g_nPoints, &v0);
    ICOUNT_DUMP("PVI/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_PVI_Update(st, g_close[it & ICOUNT_MASK], g_volume[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("PVI/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_PVI_Peek(st, g_close[it & ICOUNT_MASK], g_volume[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("PVI/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_PVI_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_PVO(int iters) {
    const char *nm = "PVO";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_PVO_Stream *st = NULL;
    TA_PVO_Stream *stf = NULL;
    double v0 = 0.0;

    ICOUNT_ZERO();
    rc = TA_PVO(0, g_nPoints - 1, g_volume, 12, 26, 1, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("PVO/batch");
    icount_row(nm, "batch", 1, rc);
    acc += g_outBuf0[0];

    ICOUNT_ZERO();
    rc = TA_PVO_OpenAndFill(&stf, g_volume, g_nPoints, 12, 26, 1, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("PVO/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += g_outBuf0[0];
    if( stf ) TA_PVO_Close(stf);

    ICOUNT_ZERO();
    rc = TA_PVO_Open(&st, g_volume, g_nPoints, 12, 26, 1, &v0);
    ICOUNT_DUMP("PVO/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_PVO_Update(st, g_volume[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("PVO/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_PVO_Peek(st, g_volume[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("PVO/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_PVO_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_PVT(int iters) {
    const char *nm = "PVT";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_PVT_Stream *st = NULL;
    TA_PVT_Stream *stf = NULL;
    double v0 = 0.0;

    ICOUNT_ZERO();
    rc = TA_PVT(0, g_nPoints - 1, g_close, g_volume, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("PVT/batch");
    icount_row(nm, "batch", 1, rc);
    acc += g_outBuf0[0];

    ICOUNT_ZERO();
    rc = TA_PVT_OpenAndFill(&stf, g_close, g_volume, g_nPoints, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("PVT/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += g_outBuf0[0];
    if( stf ) TA_PVT_Close(stf);

    ICOUNT_ZERO();
    rc = TA_PVT_Open(&st, g_close, g_volume, g_nPoints, &v0);
    ICOUNT_DUMP("PVT/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_PVT_Update(st, g_close[it & ICOUNT_MASK], g_volume[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("PVT/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_PVT_Peek(st, g_close[it & ICOUNT_MASK], g_volume[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("PVT/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_PVT_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_QSTICK(int iters) {
    const char *nm = "QSTICK";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_QSTICK_Stream *st = NULL;
    TA_QSTICK_Stream *stf = NULL;
    double v0 = 0.0;

    ICOUNT_ZERO();
    rc = TA_QSTICK(0, g_nPoints - 1, g_open, g_close, 10, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("QSTICK/batch");
    icount_row(nm, "batch", 1, rc);
    acc += g_outBuf0[0];

    ICOUNT_ZERO();
    rc = TA_QSTICK_OpenAndFill(&stf, g_open, g_close, g_nPoints, 10, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("QSTICK/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += g_outBuf0[0];
    if( stf ) TA_QSTICK_Close(stf);

    ICOUNT_ZERO();
    rc = TA_QSTICK_Open(&st, g_open, g_close, g_nPoints, 10, &v0);
    ICOUNT_DUMP("QSTICK/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_QSTICK_Update(st, g_open[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("QSTICK/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_QSTICK_Peek(st, g_open[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("QSTICK/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_QSTICK_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_RMA(int iters) {
    const char *nm = "RMA";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_RMA_Stream *st = NULL;
    TA_RMA_Stream *stf = NULL;
    double v0 = 0.0;

    ICOUNT_ZERO();
    rc = TA_RMA(0, g_nPoints - 1, g_close, 30, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("RMA/batch");
    icount_row(nm, "batch", 1, rc);
    acc += g_outBuf0[0];

    ICOUNT_ZERO();
    rc = TA_RMA_OpenAndFill(&stf, g_close, g_nPoints, 30, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("RMA/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += g_outBuf0[0];
    if( stf ) TA_RMA_Close(stf);

    ICOUNT_ZERO();
    rc = TA_RMA_Open(&st, g_close, g_nPoints, 30, &v0);
    ICOUNT_DUMP("RMA/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_RMA_Update(st, g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("RMA/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_RMA_Peek(st, g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("RMA/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_RMA_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_ROC(int iters) {
    const char *nm = "ROC";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_ROC_Stream *st = NULL;
    TA_ROC_Stream *stf = NULL;
    double v0 = 0.0;

    ICOUNT_ZERO();
    rc = TA_ROC(0, g_nPoints - 1, g_close, 10, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("ROC/batch");
    icount_row(nm, "batch", 1, rc);
    acc += g_outBuf0[0];

    ICOUNT_ZERO();
    rc = TA_ROC_OpenAndFill(&stf, g_close, g_nPoints, 10, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("ROC/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += g_outBuf0[0];
    if( stf ) TA_ROC_Close(stf);

    ICOUNT_ZERO();
    rc = TA_ROC_Open(&st, g_close, g_nPoints, 10, &v0);
    ICOUNT_DUMP("ROC/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_ROC_Update(st, g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("ROC/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_ROC_Peek(st, g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("ROC/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_ROC_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_ROCP(int iters) {
    const char *nm = "ROCP";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_ROCP_Stream *st = NULL;
    TA_ROCP_Stream *stf = NULL;
    double v0 = 0.0;

    ICOUNT_ZERO();
    rc = TA_ROCP(0, g_nPoints - 1, g_close, 10, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("ROCP/batch");
    icount_row(nm, "batch", 1, rc);
    acc += g_outBuf0[0];

    ICOUNT_ZERO();
    rc = TA_ROCP_OpenAndFill(&stf, g_close, g_nPoints, 10, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("ROCP/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += g_outBuf0[0];
    if( stf ) TA_ROCP_Close(stf);

    ICOUNT_ZERO();
    rc = TA_ROCP_Open(&st, g_close, g_nPoints, 10, &v0);
    ICOUNT_DUMP("ROCP/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_ROCP_Update(st, g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("ROCP/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_ROCP_Peek(st, g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("ROCP/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_ROCP_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_ROCR(int iters) {
    const char *nm = "ROCR";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_ROCR_Stream *st = NULL;
    TA_ROCR_Stream *stf = NULL;
    double v0 = 0.0;

    ICOUNT_ZERO();
    rc = TA_ROCR(0, g_nPoints - 1, g_close, 10, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("ROCR/batch");
    icount_row(nm, "batch", 1, rc);
    acc += g_outBuf0[0];

    ICOUNT_ZERO();
    rc = TA_ROCR_OpenAndFill(&stf, g_close, g_nPoints, 10, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("ROCR/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += g_outBuf0[0];
    if( stf ) TA_ROCR_Close(stf);

    ICOUNT_ZERO();
    rc = TA_ROCR_Open(&st, g_close, g_nPoints, 10, &v0);
    ICOUNT_DUMP("ROCR/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_ROCR_Update(st, g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("ROCR/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_ROCR_Peek(st, g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("ROCR/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_ROCR_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_ROCR100(int iters) {
    const char *nm = "ROCR100";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_ROCR100_Stream *st = NULL;
    TA_ROCR100_Stream *stf = NULL;
    double v0 = 0.0;

    ICOUNT_ZERO();
    rc = TA_ROCR100(0, g_nPoints - 1, g_close, 10, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("ROCR100/batch");
    icount_row(nm, "batch", 1, rc);
    acc += g_outBuf0[0];

    ICOUNT_ZERO();
    rc = TA_ROCR100_OpenAndFill(&stf, g_close, g_nPoints, 10, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("ROCR100/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += g_outBuf0[0];
    if( stf ) TA_ROCR100_Close(stf);

    ICOUNT_ZERO();
    rc = TA_ROCR100_Open(&st, g_close, g_nPoints, 10, &v0);
    ICOUNT_DUMP("ROCR100/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_ROCR100_Update(st, g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("ROCR100/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_ROCR100_Peek(st, g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("ROCR100/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_ROCR100_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_RSI(int iters) {
    const char *nm = "RSI";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_RSI_Stream *st = NULL;
    TA_RSI_Stream *stf = NULL;
    double v0 = 0.0;

    ICOUNT_ZERO();
    rc = TA_RSI(0, g_nPoints - 1, g_close, 14, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("RSI/batch");
    icount_row(nm, "batch", 1, rc);
    acc += g_outBuf0[0];

    ICOUNT_ZERO();
    rc = TA_RSI_OpenAndFill(&stf, g_close, g_nPoints, 14, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("RSI/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += g_outBuf0[0];
    if( stf ) TA_RSI_Close(stf);

    ICOUNT_ZERO();
    rc = TA_RSI_Open(&st, g_close, g_nPoints, 14, &v0);
    ICOUNT_DUMP("RSI/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_RSI_Update(st, g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("RSI/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_RSI_Peek(st, g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("RSI/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_RSI_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_RVI(int iters) {
    const char *nm = "RVI";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_RVI_Stream *st = NULL;
    TA_RVI_Stream *stf = NULL;
    double v0 = 0.0;

    ICOUNT_ZERO();
    rc = TA_RVI(0, g_nPoints - 1, g_close, 14, 10, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("RVI/batch");
    icount_row(nm, "batch", 1, rc);
    acc += g_outBuf0[0];

    ICOUNT_ZERO();
    rc = TA_RVI_OpenAndFill(&stf, g_close, g_nPoints, 14, 10, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("RVI/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += g_outBuf0[0];
    if( stf ) TA_RVI_Close(stf);

    ICOUNT_ZERO();
    rc = TA_RVI_Open(&st, g_close, g_nPoints, 14, 10, &v0);
    ICOUNT_DUMP("RVI/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_RVI_Update(st, g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("RVI/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_RVI_Peek(st, g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("RVI/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_RVI_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_RVOL(int iters) {
    const char *nm = "RVOL";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_RVOL_Stream *st = NULL;
    TA_RVOL_Stream *stf = NULL;
    double v0 = 0.0;

    ICOUNT_ZERO();
    rc = TA_RVOL(0, g_nPoints - 1, g_volume, 20, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("RVOL/batch");
    icount_row(nm, "batch", 1, rc);
    acc += g_outBuf0[0];

    ICOUNT_ZERO();
    rc = TA_RVOL_OpenAndFill(&stf, g_volume, g_nPoints, 20, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("RVOL/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += g_outBuf0[0];
    if( stf ) TA_RVOL_Close(stf);

    ICOUNT_ZERO();
    rc = TA_RVOL_Open(&st, g_volume, g_nPoints, 20, &v0);
    ICOUNT_DUMP("RVOL/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_RVOL_Update(st, g_volume[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("RVOL/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_RVOL_Peek(st, g_volume[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("RVOL/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_RVOL_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_SAR(int iters) {
    const char *nm = "SAR";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_SAR_Stream *st = NULL;
    TA_SAR_Stream *stf = NULL;
    double v0 = 0.0;

    ICOUNT_ZERO();
    rc = TA_SAR(0, g_nPoints - 1, g_high, g_low, 0.020000000000000, 0.200000000000000, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("SAR/batch");
    icount_row(nm, "batch", 1, rc);
    acc += g_outBuf0[0];

    ICOUNT_ZERO();
    rc = TA_SAR_OpenAndFill(&stf, g_high, g_low, g_nPoints, 0.020000000000000, 0.200000000000000, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("SAR/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += g_outBuf0[0];
    if( stf ) TA_SAR_Close(stf);

    ICOUNT_ZERO();
    rc = TA_SAR_Open(&st, g_high, g_low, g_nPoints, 0.020000000000000, 0.200000000000000, &v0);
    ICOUNT_DUMP("SAR/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_SAR_Update(st, g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("SAR/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_SAR_Peek(st, g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("SAR/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_SAR_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_SAREXT(int iters) {
    const char *nm = "SAREXT";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_SAREXT_Stream *st = NULL;
    TA_SAREXT_Stream *stf = NULL;
    double v0 = 0.0;

    ICOUNT_ZERO();
    rc = TA_SAREXT(0, g_nPoints - 1, g_high, g_low, 0.000000000000000, 0.000000000000000, 0.020000000000000, 0.020000000000000, 0.200000000000000, 0.020000000000000, 0.020000000000000, 0.200000000000000, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("SAREXT/batch");
    icount_row(nm, "batch", 1, rc);
    acc += g_outBuf0[0];

    ICOUNT_ZERO();
    rc = TA_SAREXT_OpenAndFill(&stf, g_high, g_low, g_nPoints, 0.000000000000000, 0.000000000000000, 0.020000000000000, 0.020000000000000, 0.200000000000000, 0.020000000000000, 0.020000000000000, 0.200000000000000, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("SAREXT/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += g_outBuf0[0];
    if( stf ) TA_SAREXT_Close(stf);

    ICOUNT_ZERO();
    rc = TA_SAREXT_Open(&st, g_high, g_low, g_nPoints, 0.000000000000000, 0.000000000000000, 0.020000000000000, 0.020000000000000, 0.200000000000000, 0.020000000000000, 0.020000000000000, 0.200000000000000, &v0);
    ICOUNT_DUMP("SAREXT/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_SAREXT_Update(st, g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("SAREXT/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_SAREXT_Peek(st, g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("SAREXT/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_SAREXT_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_SIN(int iters) {
    const char *nm = "SIN";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_SIN_Stream *st = NULL;
    TA_SIN_Stream *stf = NULL;
    double v0 = 0.0;

    ICOUNT_ZERO();
    rc = TA_SIN(0, g_nPoints - 1, g_close, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("SIN/batch");
    icount_row(nm, "batch", 1, rc);
    acc += g_outBuf0[0];

    ICOUNT_ZERO();
    rc = TA_SIN_OpenAndFill(&stf, g_close, g_nPoints, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("SIN/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += g_outBuf0[0];
    if( stf ) TA_SIN_Close(stf);

    ICOUNT_ZERO();
    rc = TA_SIN_Open(&st, g_close, g_nPoints, &v0);
    ICOUNT_DUMP("SIN/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_SIN_Update(st, g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("SIN/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_SIN_Peek(st, g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("SIN/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_SIN_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_SINH(int iters) {
    const char *nm = "SINH";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_SINH_Stream *st = NULL;
    TA_SINH_Stream *stf = NULL;
    double v0 = 0.0;

    ICOUNT_ZERO();
    rc = TA_SINH(0, g_nPoints - 1, g_close, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("SINH/batch");
    icount_row(nm, "batch", 1, rc);
    acc += g_outBuf0[0];

    ICOUNT_ZERO();
    rc = TA_SINH_OpenAndFill(&stf, g_close, g_nPoints, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("SINH/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += g_outBuf0[0];
    if( stf ) TA_SINH_Close(stf);

    ICOUNT_ZERO();
    rc = TA_SINH_Open(&st, g_close, g_nPoints, &v0);
    ICOUNT_DUMP("SINH/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_SINH_Update(st, g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("SINH/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_SINH_Peek(st, g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("SINH/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_SINH_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_SMA(int iters) {
    const char *nm = "SMA";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_SMA_Stream *st = NULL;
    TA_SMA_Stream *stf = NULL;
    double v0 = 0.0;

    ICOUNT_ZERO();
    rc = TA_SMA(0, g_nPoints - 1, g_close, 30, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("SMA/batch");
    icount_row(nm, "batch", 1, rc);
    acc += g_outBuf0[0];

    ICOUNT_ZERO();
    rc = TA_SMA_OpenAndFill(&stf, g_close, g_nPoints, 30, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("SMA/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += g_outBuf0[0];
    if( stf ) TA_SMA_Close(stf);

    ICOUNT_ZERO();
    rc = TA_SMA_Open(&st, g_close, g_nPoints, 30, &v0);
    ICOUNT_DUMP("SMA/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_SMA_Update(st, g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("SMA/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_SMA_Peek(st, g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("SMA/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_SMA_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_SMI(int iters) {
    const char *nm = "SMI";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_SMI_Stream *st = NULL;
    TA_SMI_Stream *stf = NULL;
    double v0 = 0.0;
    double v1 = 0.0;

    ICOUNT_ZERO();
    rc = TA_SMI(0, g_nPoints - 1, g_high, g_low, g_close, 13, 2, 25, 9, &outBegIdx, &outNBElement, g_outBuf0, g_outBuf1);
    ICOUNT_DUMP("SMI/batch");
    icount_row(nm, "batch", 1, rc);
    acc += g_outBuf0[0];
    acc += g_outBuf1[0];

    ICOUNT_ZERO();
    rc = TA_SMI_OpenAndFill(&stf, g_high, g_low, g_close, g_nPoints, 13, 2, 25, 9, &outBegIdx, &outNBElement, g_outBuf0, g_outBuf1);
    ICOUNT_DUMP("SMI/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += g_outBuf0[0];
    acc += g_outBuf1[0];
    if( stf ) TA_SMI_Close(stf);

    ICOUNT_ZERO();
    rc = TA_SMI_Open(&st, g_high, g_low, g_close, g_nPoints, 13, 2, 25, 9, &v0, &v1);
    ICOUNT_DUMP("SMI/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_SMI_Update(st, g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &v0, &v1);
            acc += v0;
            acc += v1;
        }
        ICOUNT_DUMP("SMI/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_SMI_Peek(st, g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &v0, &v1);
            acc += v0;
            acc += v1;
        }
        ICOUNT_DUMP("SMI/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_SMI_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_SQRT(int iters) {
    const char *nm = "SQRT";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_SQRT_Stream *st = NULL;
    TA_SQRT_Stream *stf = NULL;
    double v0 = 0.0;

    ICOUNT_ZERO();
    rc = TA_SQRT(0, g_nPoints - 1, g_close, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("SQRT/batch");
    icount_row(nm, "batch", 1, rc);
    acc += g_outBuf0[0];

    ICOUNT_ZERO();
    rc = TA_SQRT_OpenAndFill(&stf, g_close, g_nPoints, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("SQRT/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += g_outBuf0[0];
    if( stf ) TA_SQRT_Close(stf);

    ICOUNT_ZERO();
    rc = TA_SQRT_Open(&st, g_close, g_nPoints, &v0);
    ICOUNT_DUMP("SQRT/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_SQRT_Update(st, g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("SQRT/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_SQRT_Peek(st, g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("SQRT/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_SQRT_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_STDDEV(int iters) {
    const char *nm = "STDDEV";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_STDDEV_Stream *st = NULL;
    TA_STDDEV_Stream *stf = NULL;
    double v0 = 0.0;

    ICOUNT_ZERO();
    rc = TA_STDDEV(0, g_nPoints - 1, g_close, 5, 1.000000000000000, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("STDDEV/batch");
    icount_row(nm, "batch", 1, rc);
    acc += g_outBuf0[0];

    ICOUNT_ZERO();
    rc = TA_STDDEV_OpenAndFill(&stf, g_close, g_nPoints, 5, 1.000000000000000, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("STDDEV/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += g_outBuf0[0];
    if( stf ) TA_STDDEV_Close(stf);

    ICOUNT_ZERO();
    rc = TA_STDDEV_Open(&st, g_close, g_nPoints, 5, 1.000000000000000, &v0);
    ICOUNT_DUMP("STDDEV/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_STDDEV_Update(st, g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("STDDEV/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_STDDEV_Peek(st, g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("STDDEV/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_STDDEV_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_STOCH(int iters) {
    const char *nm = "STOCH";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_STOCH_Stream *st = NULL;
    TA_STOCH_Stream *stf = NULL;
    double v0 = 0.0;
    double v1 = 0.0;

    ICOUNT_ZERO();
    rc = TA_STOCH(0, g_nPoints - 1, g_high, g_low, g_close, 5, 3, 0, 3, 0, &outBegIdx, &outNBElement, g_outBuf0, g_outBuf1);
    ICOUNT_DUMP("STOCH/batch");
    icount_row(nm, "batch", 1, rc);
    acc += g_outBuf0[0];
    acc += g_outBuf1[0];

    ICOUNT_ZERO();
    rc = TA_STOCH_OpenAndFill(&stf, g_high, g_low, g_close, g_nPoints, 5, 3, 0, 3, 0, &outBegIdx, &outNBElement, g_outBuf0, g_outBuf1);
    ICOUNT_DUMP("STOCH/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += g_outBuf0[0];
    acc += g_outBuf1[0];
    if( stf ) TA_STOCH_Close(stf);

    ICOUNT_ZERO();
    rc = TA_STOCH_Open(&st, g_high, g_low, g_close, g_nPoints, 5, 3, 0, 3, 0, &v0, &v1);
    ICOUNT_DUMP("STOCH/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_STOCH_Update(st, g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &v0, &v1);
            acc += v0;
            acc += v1;
        }
        ICOUNT_DUMP("STOCH/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_STOCH_Peek(st, g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &v0, &v1);
            acc += v0;
            acc += v1;
        }
        ICOUNT_DUMP("STOCH/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_STOCH_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_STOCHF(int iters) {
    const char *nm = "STOCHF";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_STOCHF_Stream *st = NULL;
    TA_STOCHF_Stream *stf = NULL;
    double v0 = 0.0;
    double v1 = 0.0;

    ICOUNT_ZERO();
    rc = TA_STOCHF(0, g_nPoints - 1, g_high, g_low, g_close, 5, 3, 0, &outBegIdx, &outNBElement, g_outBuf0, g_outBuf1);
    ICOUNT_DUMP("STOCHF/batch");
    icount_row(nm, "batch", 1, rc);
    acc += g_outBuf0[0];
    acc += g_outBuf1[0];

    ICOUNT_ZERO();
    rc = TA_STOCHF_OpenAndFill(&stf, g_high, g_low, g_close, g_nPoints, 5, 3, 0, &outBegIdx, &outNBElement, g_outBuf0, g_outBuf1);
    ICOUNT_DUMP("STOCHF/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += g_outBuf0[0];
    acc += g_outBuf1[0];
    if( stf ) TA_STOCHF_Close(stf);

    ICOUNT_ZERO();
    rc = TA_STOCHF_Open(&st, g_high, g_low, g_close, g_nPoints, 5, 3, 0, &v0, &v1);
    ICOUNT_DUMP("STOCHF/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_STOCHF_Update(st, g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &v0, &v1);
            acc += v0;
            acc += v1;
        }
        ICOUNT_DUMP("STOCHF/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_STOCHF_Peek(st, g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &v0, &v1);
            acc += v0;
            acc += v1;
        }
        ICOUNT_DUMP("STOCHF/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_STOCHF_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_STOCHRSI(int iters) {
    const char *nm = "STOCHRSI";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_STOCHRSI_Stream *st = NULL;
    TA_STOCHRSI_Stream *stf = NULL;
    double v0 = 0.0;
    double v1 = 0.0;

    ICOUNT_ZERO();
    rc = TA_STOCHRSI(0, g_nPoints - 1, g_close, 14, 5, 3, 0, &outBegIdx, &outNBElement, g_outBuf0, g_outBuf1);
    ICOUNT_DUMP("STOCHRSI/batch");
    icount_row(nm, "batch", 1, rc);
    acc += g_outBuf0[0];
    acc += g_outBuf1[0];

    ICOUNT_ZERO();
    rc = TA_STOCHRSI_OpenAndFill(&stf, g_close, g_nPoints, 14, 5, 3, 0, &outBegIdx, &outNBElement, g_outBuf0, g_outBuf1);
    ICOUNT_DUMP("STOCHRSI/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += g_outBuf0[0];
    acc += g_outBuf1[0];
    if( stf ) TA_STOCHRSI_Close(stf);

    ICOUNT_ZERO();
    rc = TA_STOCHRSI_Open(&st, g_close, g_nPoints, 14, 5, 3, 0, &v0, &v1);
    ICOUNT_DUMP("STOCHRSI/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_STOCHRSI_Update(st, g_close[it & ICOUNT_MASK], &v0, &v1);
            acc += v0;
            acc += v1;
        }
        ICOUNT_DUMP("STOCHRSI/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_STOCHRSI_Peek(st, g_close[it & ICOUNT_MASK], &v0, &v1);
            acc += v0;
            acc += v1;
        }
        ICOUNT_DUMP("STOCHRSI/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_STOCHRSI_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_SUB(int iters) {
    const char *nm = "SUB";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_SUB_Stream *st = NULL;
    TA_SUB_Stream *stf = NULL;
    double v0 = 0.0;

    ICOUNT_ZERO();
    rc = TA_SUB(0, g_nPoints - 1, g_close, g_high, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("SUB/batch");
    icount_row(nm, "batch", 1, rc);
    acc += g_outBuf0[0];

    ICOUNT_ZERO();
    rc = TA_SUB_OpenAndFill(&stf, g_close, g_high, g_nPoints, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("SUB/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += g_outBuf0[0];
    if( stf ) TA_SUB_Close(stf);

    ICOUNT_ZERO();
    rc = TA_SUB_Open(&st, g_close, g_high, g_nPoints, &v0);
    ICOUNT_DUMP("SUB/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_SUB_Update(st, g_close[it & ICOUNT_MASK], g_high[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("SUB/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_SUB_Peek(st, g_close[it & ICOUNT_MASK], g_high[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("SUB/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_SUB_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_SUM(int iters) {
    const char *nm = "SUM";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_SUM_Stream *st = NULL;
    TA_SUM_Stream *stf = NULL;
    double v0 = 0.0;

    ICOUNT_ZERO();
    rc = TA_SUM(0, g_nPoints - 1, g_close, 30, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("SUM/batch");
    icount_row(nm, "batch", 1, rc);
    acc += g_outBuf0[0];

    ICOUNT_ZERO();
    rc = TA_SUM_OpenAndFill(&stf, g_close, g_nPoints, 30, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("SUM/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += g_outBuf0[0];
    if( stf ) TA_SUM_Close(stf);

    ICOUNT_ZERO();
    rc = TA_SUM_Open(&st, g_close, g_nPoints, 30, &v0);
    ICOUNT_DUMP("SUM/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_SUM_Update(st, g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("SUM/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_SUM_Peek(st, g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("SUM/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_SUM_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_SUPERTREND(int iters) {
    const char *nm = "SUPERTREND";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_SUPERTREND_Stream *st = NULL;
    TA_SUPERTREND_Stream *stf = NULL;
    double v0 = 0.0;
    int iv0 = 0;

    ICOUNT_ZERO();
    rc = TA_SUPERTREND(0, g_nPoints - 1, g_high, g_low, g_close, 10, 3.000000000000000, &outBegIdx, &outNBElement, g_outBuf0, g_outIntBuf0);
    ICOUNT_DUMP("SUPERTREND/batch");
    icount_row(nm, "batch", 1, rc);
    acc += g_outBuf0[0];
    acc += (double)g_outIntBuf0[0];

    ICOUNT_ZERO();
    rc = TA_SUPERTREND_OpenAndFill(&stf, g_high, g_low, g_close, g_nPoints, 10, 3.000000000000000, &outBegIdx, &outNBElement, g_outBuf0, g_outIntBuf0);
    ICOUNT_DUMP("SUPERTREND/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += g_outBuf0[0];
    acc += (double)g_outIntBuf0[0];
    if( stf ) TA_SUPERTREND_Close(stf);

    ICOUNT_ZERO();
    rc = TA_SUPERTREND_Open(&st, g_high, g_low, g_close, g_nPoints, 10, 3.000000000000000, &v0, &iv0);
    ICOUNT_DUMP("SUPERTREND/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_SUPERTREND_Update(st, g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &v0, &iv0);
            acc += v0;
            acc += (double)iv0;
        }
        ICOUNT_DUMP("SUPERTREND/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_SUPERTREND_Peek(st, g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &v0, &iv0);
            acc += v0;
            acc += (double)iv0;
        }
        ICOUNT_DUMP("SUPERTREND/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_SUPERTREND_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_T3(int iters) {
    const char *nm = "T3";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_T3_Stream *st = NULL;
    TA_T3_Stream *stf = NULL;
    double v0 = 0.0;

    ICOUNT_ZERO();
    rc = TA_T3(0, g_nPoints - 1, g_close, 5, 0.700000000000000, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("T3/batch");
    icount_row(nm, "batch", 1, rc);
    acc += g_outBuf0[0];

    ICOUNT_ZERO();
    rc = TA_T3_OpenAndFill(&stf, g_close, g_nPoints, 5, 0.700000000000000, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("T3/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += g_outBuf0[0];
    if( stf ) TA_T3_Close(stf);

    ICOUNT_ZERO();
    rc = TA_T3_Open(&st, g_close, g_nPoints, 5, 0.700000000000000, &v0);
    ICOUNT_DUMP("T3/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_T3_Update(st, g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("T3/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_T3_Peek(st, g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("T3/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_T3_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_TAN(int iters) {
    const char *nm = "TAN";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_TAN_Stream *st = NULL;
    TA_TAN_Stream *stf = NULL;
    double v0 = 0.0;

    ICOUNT_ZERO();
    rc = TA_TAN(0, g_nPoints - 1, g_close, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("TAN/batch");
    icount_row(nm, "batch", 1, rc);
    acc += g_outBuf0[0];

    ICOUNT_ZERO();
    rc = TA_TAN_OpenAndFill(&stf, g_close, g_nPoints, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("TAN/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += g_outBuf0[0];
    if( stf ) TA_TAN_Close(stf);

    ICOUNT_ZERO();
    rc = TA_TAN_Open(&st, g_close, g_nPoints, &v0);
    ICOUNT_DUMP("TAN/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_TAN_Update(st, g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("TAN/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_TAN_Peek(st, g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("TAN/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_TAN_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_TANH(int iters) {
    const char *nm = "TANH";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_TANH_Stream *st = NULL;
    TA_TANH_Stream *stf = NULL;
    double v0 = 0.0;

    ICOUNT_ZERO();
    rc = TA_TANH(0, g_nPoints - 1, g_close, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("TANH/batch");
    icount_row(nm, "batch", 1, rc);
    acc += g_outBuf0[0];

    ICOUNT_ZERO();
    rc = TA_TANH_OpenAndFill(&stf, g_close, g_nPoints, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("TANH/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += g_outBuf0[0];
    if( stf ) TA_TANH_Close(stf);

    ICOUNT_ZERO();
    rc = TA_TANH_Open(&st, g_close, g_nPoints, &v0);
    ICOUNT_DUMP("TANH/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_TANH_Update(st, g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("TANH/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_TANH_Peek(st, g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("TANH/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_TANH_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_TEMA(int iters) {
    const char *nm = "TEMA";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_TEMA_Stream *st = NULL;
    TA_TEMA_Stream *stf = NULL;
    double v0 = 0.0;

    ICOUNT_ZERO();
    rc = TA_TEMA(0, g_nPoints - 1, g_close, 30, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("TEMA/batch");
    icount_row(nm, "batch", 1, rc);
    acc += g_outBuf0[0];

    ICOUNT_ZERO();
    rc = TA_TEMA_OpenAndFill(&stf, g_close, g_nPoints, 30, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("TEMA/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += g_outBuf0[0];
    if( stf ) TA_TEMA_Close(stf);

    ICOUNT_ZERO();
    rc = TA_TEMA_Open(&st, g_close, g_nPoints, 30, &v0);
    ICOUNT_DUMP("TEMA/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_TEMA_Update(st, g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("TEMA/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_TEMA_Peek(st, g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("TEMA/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_TEMA_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_TRANGE(int iters) {
    const char *nm = "TRANGE";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_TRANGE_Stream *st = NULL;
    TA_TRANGE_Stream *stf = NULL;
    double v0 = 0.0;

    ICOUNT_ZERO();
    rc = TA_TRANGE(0, g_nPoints - 1, g_high, g_low, g_close, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("TRANGE/batch");
    icount_row(nm, "batch", 1, rc);
    acc += g_outBuf0[0];

    ICOUNT_ZERO();
    rc = TA_TRANGE_OpenAndFill(&stf, g_high, g_low, g_close, g_nPoints, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("TRANGE/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += g_outBuf0[0];
    if( stf ) TA_TRANGE_Close(stf);

    ICOUNT_ZERO();
    rc = TA_TRANGE_Open(&st, g_high, g_low, g_close, g_nPoints, &v0);
    ICOUNT_DUMP("TRANGE/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_TRANGE_Update(st, g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("TRANGE/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_TRANGE_Peek(st, g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("TRANGE/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_TRANGE_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_TRIMA(int iters) {
    const char *nm = "TRIMA";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_TRIMA_Stream *st = NULL;
    TA_TRIMA_Stream *stf = NULL;
    double v0 = 0.0;

    ICOUNT_ZERO();
    rc = TA_TRIMA(0, g_nPoints - 1, g_close, 30, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("TRIMA/batch");
    icount_row(nm, "batch", 1, rc);
    acc += g_outBuf0[0];

    ICOUNT_ZERO();
    rc = TA_TRIMA_OpenAndFill(&stf, g_close, g_nPoints, 30, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("TRIMA/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += g_outBuf0[0];
    if( stf ) TA_TRIMA_Close(stf);

    ICOUNT_ZERO();
    rc = TA_TRIMA_Open(&st, g_close, g_nPoints, 30, &v0);
    ICOUNT_DUMP("TRIMA/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_TRIMA_Update(st, g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("TRIMA/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_TRIMA_Peek(st, g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("TRIMA/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_TRIMA_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_TRIX(int iters) {
    const char *nm = "TRIX";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_TRIX_Stream *st = NULL;
    TA_TRIX_Stream *stf = NULL;
    double v0 = 0.0;

    ICOUNT_ZERO();
    rc = TA_TRIX(0, g_nPoints - 1, g_close, 30, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("TRIX/batch");
    icount_row(nm, "batch", 1, rc);
    acc += g_outBuf0[0];

    ICOUNT_ZERO();
    rc = TA_TRIX_OpenAndFill(&stf, g_close, g_nPoints, 30, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("TRIX/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += g_outBuf0[0];
    if( stf ) TA_TRIX_Close(stf);

    ICOUNT_ZERO();
    rc = TA_TRIX_Open(&st, g_close, g_nPoints, 30, &v0);
    ICOUNT_DUMP("TRIX/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_TRIX_Update(st, g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("TRIX/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_TRIX_Peek(st, g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("TRIX/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_TRIX_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_TSF(int iters) {
    const char *nm = "TSF";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_TSF_Stream *st = NULL;
    TA_TSF_Stream *stf = NULL;
    double v0 = 0.0;

    ICOUNT_ZERO();
    rc = TA_TSF(0, g_nPoints - 1, g_close, 14, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("TSF/batch");
    icount_row(nm, "batch", 1, rc);
    acc += g_outBuf0[0];

    ICOUNT_ZERO();
    rc = TA_TSF_OpenAndFill(&stf, g_close, g_nPoints, 14, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("TSF/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += g_outBuf0[0];
    if( stf ) TA_TSF_Close(stf);

    ICOUNT_ZERO();
    rc = TA_TSF_Open(&st, g_close, g_nPoints, 14, &v0);
    ICOUNT_DUMP("TSF/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_TSF_Update(st, g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("TSF/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_TSF_Peek(st, g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("TSF/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_TSF_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_TSI(int iters) {
    const char *nm = "TSI";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_TSI_Stream *st = NULL;
    TA_TSI_Stream *stf = NULL;
    double v0 = 0.0;

    ICOUNT_ZERO();
    rc = TA_TSI(0, g_nPoints - 1, g_close, 25, 13, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("TSI/batch");
    icount_row(nm, "batch", 1, rc);
    acc += g_outBuf0[0];

    ICOUNT_ZERO();
    rc = TA_TSI_OpenAndFill(&stf, g_close, g_nPoints, 25, 13, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("TSI/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += g_outBuf0[0];
    if( stf ) TA_TSI_Close(stf);

    ICOUNT_ZERO();
    rc = TA_TSI_Open(&st, g_close, g_nPoints, 25, 13, &v0);
    ICOUNT_DUMP("TSI/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_TSI_Update(st, g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("TSI/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_TSI_Peek(st, g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("TSI/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_TSI_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_TYPPRICE(int iters) {
    const char *nm = "TYPPRICE";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_TYPPRICE_Stream *st = NULL;
    TA_TYPPRICE_Stream *stf = NULL;
    double v0 = 0.0;

    ICOUNT_ZERO();
    rc = TA_TYPPRICE(0, g_nPoints - 1, g_high, g_low, g_close, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("TYPPRICE/batch");
    icount_row(nm, "batch", 1, rc);
    acc += g_outBuf0[0];

    ICOUNT_ZERO();
    rc = TA_TYPPRICE_OpenAndFill(&stf, g_high, g_low, g_close, g_nPoints, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("TYPPRICE/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += g_outBuf0[0];
    if( stf ) TA_TYPPRICE_Close(stf);

    ICOUNT_ZERO();
    rc = TA_TYPPRICE_Open(&st, g_high, g_low, g_close, g_nPoints, &v0);
    ICOUNT_DUMP("TYPPRICE/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_TYPPRICE_Update(st, g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("TYPPRICE/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_TYPPRICE_Peek(st, g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("TYPPRICE/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_TYPPRICE_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_ULTOSC(int iters) {
    const char *nm = "ULTOSC";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_ULTOSC_Stream *st = NULL;
    TA_ULTOSC_Stream *stf = NULL;
    double v0 = 0.0;

    ICOUNT_ZERO();
    rc = TA_ULTOSC(0, g_nPoints - 1, g_high, g_low, g_close, 7, 14, 28, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("ULTOSC/batch");
    icount_row(nm, "batch", 1, rc);
    acc += g_outBuf0[0];

    ICOUNT_ZERO();
    rc = TA_ULTOSC_OpenAndFill(&stf, g_high, g_low, g_close, g_nPoints, 7, 14, 28, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("ULTOSC/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += g_outBuf0[0];
    if( stf ) TA_ULTOSC_Close(stf);

    ICOUNT_ZERO();
    rc = TA_ULTOSC_Open(&st, g_high, g_low, g_close, g_nPoints, 7, 14, 28, &v0);
    ICOUNT_DUMP("ULTOSC/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_ULTOSC_Update(st, g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("ULTOSC/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_ULTOSC_Peek(st, g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("ULTOSC/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_ULTOSC_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_VAR(int iters) {
    const char *nm = "VAR";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_VAR_Stream *st = NULL;
    TA_VAR_Stream *stf = NULL;
    double v0 = 0.0;

    ICOUNT_ZERO();
    rc = TA_VAR(0, g_nPoints - 1, g_close, 5, 1.000000000000000, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("VAR/batch");
    icount_row(nm, "batch", 1, rc);
    acc += g_outBuf0[0];

    ICOUNT_ZERO();
    rc = TA_VAR_OpenAndFill(&stf, g_close, g_nPoints, 5, 1.000000000000000, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("VAR/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += g_outBuf0[0];
    if( stf ) TA_VAR_Close(stf);

    ICOUNT_ZERO();
    rc = TA_VAR_Open(&st, g_close, g_nPoints, 5, 1.000000000000000, &v0);
    ICOUNT_DUMP("VAR/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_VAR_Update(st, g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("VAR/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_VAR_Peek(st, g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("VAR/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_VAR_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_VHF(int iters) {
    const char *nm = "VHF";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_VHF_Stream *st = NULL;
    TA_VHF_Stream *stf = NULL;
    double v0 = 0.0;

    ICOUNT_ZERO();
    rc = TA_VHF(0, g_nPoints - 1, g_close, 28, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("VHF/batch");
    icount_row(nm, "batch", 1, rc);
    acc += g_outBuf0[0];

    ICOUNT_ZERO();
    rc = TA_VHF_OpenAndFill(&stf, g_close, g_nPoints, 28, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("VHF/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += g_outBuf0[0];
    if( stf ) TA_VHF_Close(stf);

    ICOUNT_ZERO();
    rc = TA_VHF_Open(&st, g_close, g_nPoints, 28, &v0);
    ICOUNT_DUMP("VHF/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_VHF_Update(st, g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("VHF/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_VHF_Peek(st, g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("VHF/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_VHF_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_VORTEX(int iters) {
    const char *nm = "VORTEX";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_VORTEX_Stream *st = NULL;
    TA_VORTEX_Stream *stf = NULL;
    double v0 = 0.0;
    double v1 = 0.0;

    ICOUNT_ZERO();
    rc = TA_VORTEX(0, g_nPoints - 1, g_high, g_low, g_close, 14, &outBegIdx, &outNBElement, g_outBuf0, g_outBuf1);
    ICOUNT_DUMP("VORTEX/batch");
    icount_row(nm, "batch", 1, rc);
    acc += g_outBuf0[0];
    acc += g_outBuf1[0];

    ICOUNT_ZERO();
    rc = TA_VORTEX_OpenAndFill(&stf, g_high, g_low, g_close, g_nPoints, 14, &outBegIdx, &outNBElement, g_outBuf0, g_outBuf1);
    ICOUNT_DUMP("VORTEX/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += g_outBuf0[0];
    acc += g_outBuf1[0];
    if( stf ) TA_VORTEX_Close(stf);

    ICOUNT_ZERO();
    rc = TA_VORTEX_Open(&st, g_high, g_low, g_close, g_nPoints, 14, &v0, &v1);
    ICOUNT_DUMP("VORTEX/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_VORTEX_Update(st, g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &v0, &v1);
            acc += v0;
            acc += v1;
        }
        ICOUNT_DUMP("VORTEX/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_VORTEX_Peek(st, g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &v0, &v1);
            acc += v0;
            acc += v1;
        }
        ICOUNT_DUMP("VORTEX/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_VORTEX_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_VWAP(int iters) {
    const char *nm = "VWAP";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_VWAP_Stream *st = NULL;
    TA_VWAP_Stream *stf = NULL;
    double v0 = 0.0;

    ICOUNT_ZERO();
    rc = TA_VWAP(0, g_nPoints - 1, g_high, g_low, g_close, g_volume, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("VWAP/batch");
    icount_row(nm, "batch", 1, rc);
    acc += g_outBuf0[0];

    ICOUNT_ZERO();
    rc = TA_VWAP_OpenAndFill(&stf, g_high, g_low, g_close, g_volume, g_nPoints, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("VWAP/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += g_outBuf0[0];
    if( stf ) TA_VWAP_Close(stf);

    ICOUNT_ZERO();
    rc = TA_VWAP_Open(&st, g_high, g_low, g_close, g_volume, g_nPoints, &v0);
    ICOUNT_DUMP("VWAP/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_VWAP_Update(st, g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], g_volume[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("VWAP/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_VWAP_Peek(st, g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], g_volume[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("VWAP/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_VWAP_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_VWMA(int iters) {
    const char *nm = "VWMA";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_VWMA_Stream *st = NULL;
    TA_VWMA_Stream *stf = NULL;
    double v0 = 0.0;

    ICOUNT_ZERO();
    rc = TA_VWMA(0, g_nPoints - 1, g_close, g_volume, 30, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("VWMA/batch");
    icount_row(nm, "batch", 1, rc);
    acc += g_outBuf0[0];

    ICOUNT_ZERO();
    rc = TA_VWMA_OpenAndFill(&stf, g_close, g_volume, g_nPoints, 30, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("VWMA/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += g_outBuf0[0];
    if( stf ) TA_VWMA_Close(stf);

    ICOUNT_ZERO();
    rc = TA_VWMA_Open(&st, g_close, g_volume, g_nPoints, 30, &v0);
    ICOUNT_DUMP("VWMA/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_VWMA_Update(st, g_close[it & ICOUNT_MASK], g_volume[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("VWMA/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_VWMA_Peek(st, g_close[it & ICOUNT_MASK], g_volume[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("VWMA/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_VWMA_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_WAD(int iters) {
    const char *nm = "WAD";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_WAD_Stream *st = NULL;
    TA_WAD_Stream *stf = NULL;
    double v0 = 0.0;

    ICOUNT_ZERO();
    rc = TA_WAD(0, g_nPoints - 1, g_high, g_low, g_close, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("WAD/batch");
    icount_row(nm, "batch", 1, rc);
    acc += g_outBuf0[0];

    ICOUNT_ZERO();
    rc = TA_WAD_OpenAndFill(&stf, g_high, g_low, g_close, g_nPoints, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("WAD/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += g_outBuf0[0];
    if( stf ) TA_WAD_Close(stf);

    ICOUNT_ZERO();
    rc = TA_WAD_Open(&st, g_high, g_low, g_close, g_nPoints, &v0);
    ICOUNT_DUMP("WAD/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_WAD_Update(st, g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("WAD/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_WAD_Peek(st, g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("WAD/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_WAD_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_WCLPRICE(int iters) {
    const char *nm = "WCLPRICE";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_WCLPRICE_Stream *st = NULL;
    TA_WCLPRICE_Stream *stf = NULL;
    double v0 = 0.0;

    ICOUNT_ZERO();
    rc = TA_WCLPRICE(0, g_nPoints - 1, g_high, g_low, g_close, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("WCLPRICE/batch");
    icount_row(nm, "batch", 1, rc);
    acc += g_outBuf0[0];

    ICOUNT_ZERO();
    rc = TA_WCLPRICE_OpenAndFill(&stf, g_high, g_low, g_close, g_nPoints, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("WCLPRICE/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += g_outBuf0[0];
    if( stf ) TA_WCLPRICE_Close(stf);

    ICOUNT_ZERO();
    rc = TA_WCLPRICE_Open(&st, g_high, g_low, g_close, g_nPoints, &v0);
    ICOUNT_DUMP("WCLPRICE/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_WCLPRICE_Update(st, g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("WCLPRICE/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_WCLPRICE_Peek(st, g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("WCLPRICE/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_WCLPRICE_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_WILLR(int iters) {
    const char *nm = "WILLR";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_WILLR_Stream *st = NULL;
    TA_WILLR_Stream *stf = NULL;
    double v0 = 0.0;

    ICOUNT_ZERO();
    rc = TA_WILLR(0, g_nPoints - 1, g_high, g_low, g_close, 14, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("WILLR/batch");
    icount_row(nm, "batch", 1, rc);
    acc += g_outBuf0[0];

    ICOUNT_ZERO();
    rc = TA_WILLR_OpenAndFill(&stf, g_high, g_low, g_close, g_nPoints, 14, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("WILLR/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += g_outBuf0[0];
    if( stf ) TA_WILLR_Close(stf);

    ICOUNT_ZERO();
    rc = TA_WILLR_Open(&st, g_high, g_low, g_close, g_nPoints, 14, &v0);
    ICOUNT_DUMP("WILLR/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_WILLR_Update(st, g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("WILLR/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_WILLR_Peek(st, g_high[it & ICOUNT_MASK], g_low[it & ICOUNT_MASK], g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("WILLR/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_WILLR_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_WMA(int iters) {
    const char *nm = "WMA";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_WMA_Stream *st = NULL;
    TA_WMA_Stream *stf = NULL;
    double v0 = 0.0;

    ICOUNT_ZERO();
    rc = TA_WMA(0, g_nPoints - 1, g_close, 30, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("WMA/batch");
    icount_row(nm, "batch", 1, rc);
    acc += g_outBuf0[0];

    ICOUNT_ZERO();
    rc = TA_WMA_OpenAndFill(&stf, g_close, g_nPoints, 30, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("WMA/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += g_outBuf0[0];
    if( stf ) TA_WMA_Close(stf);

    ICOUNT_ZERO();
    rc = TA_WMA_Open(&st, g_close, g_nPoints, 30, &v0);
    ICOUNT_DUMP("WMA/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_WMA_Update(st, g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("WMA/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_WMA_Peek(st, g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("WMA/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_WMA_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_ZLEMA(int iters) {
    const char *nm = "ZLEMA";
    int outBegIdx = 0, outNBElement = 0;
    double acc = 0.0;
    TA_RetCode rc;
    TA_ZLEMA_Stream *st = NULL;
    TA_ZLEMA_Stream *stf = NULL;
    double v0 = 0.0;

    ICOUNT_ZERO();
    rc = TA_ZLEMA(0, g_nPoints - 1, g_close, 30, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("ZLEMA/batch");
    icount_row(nm, "batch", 1, rc);
    acc += g_outBuf0[0];

    ICOUNT_ZERO();
    rc = TA_ZLEMA_OpenAndFill(&stf, g_close, g_nPoints, 30, &outBegIdx, &outNBElement, g_outBuf0);
    ICOUNT_DUMP("ZLEMA/openfill");
    icount_row(nm, "openfill", 1, rc);
    acc += g_outBuf0[0];
    if( stf ) TA_ZLEMA_Close(stf);

    ICOUNT_ZERO();
    rc = TA_ZLEMA_Open(&st, g_close, g_nPoints, 30, &v0);
    ICOUNT_DUMP("ZLEMA/open");
    icount_row(nm, "open", 1, rc);

    if( rc == TA_SUCCESS && st ) {
        TA_RetCode src = TA_SUCCESS;
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_ZLEMA_Update(st, g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("ZLEMA/update");
        icount_row(nm, "update", 1, src);
        ICOUNT_ZERO();
        for( int it = 0; it < iters; it++ ) {
            src = TA_ZLEMA_Peek(st, g_close[it & ICOUNT_MASK], &v0);
            acc += v0;
        }
        ICOUNT_DUMP("ZLEMA/peek");
        icount_row(nm, "peek", 1, src);
    } else {
        icount_row(nm, "update", 0, rc);
        icount_row(nm, "peek", 0, rc);
    }
    if( st ) TA_ZLEMA_Close(st);
    g_sink += (int)acc + outNBElement;
}

static void icount_all(const char *filter, int iters) {
    if( func_matches(filter, "AC") ) { icount_AC(iters); fflush(stdout); }
    if( func_matches(filter, "ACCBANDS") ) { icount_ACCBANDS(iters); fflush(stdout); }
    if( func_matches(filter, "ACOS") ) { icount_ACOS(iters); fflush(stdout); }
    if( func_matches(filter, "AD") ) { icount_AD(iters); fflush(stdout); }
    if( func_matches(filter, "ADD") ) { icount_ADD(iters); fflush(stdout); }
    if( func_matches(filter, "ADOSC") ) { icount_ADOSC(iters); fflush(stdout); }
    if( func_matches(filter, "ADR") ) { icount_ADR(iters); fflush(stdout); }
    if( func_matches(filter, "ADX") ) { icount_ADX(iters); fflush(stdout); }
    if( func_matches(filter, "ADXR") ) { icount_ADXR(iters); fflush(stdout); }
    if( func_matches(filter, "AO") ) { icount_AO(iters); fflush(stdout); }
    if( func_matches(filter, "APO") ) { icount_APO(iters); fflush(stdout); }
    if( func_matches(filter, "AROON") ) { icount_AROON(iters); fflush(stdout); }
    if( func_matches(filter, "AROONOSC") ) { icount_AROONOSC(iters); fflush(stdout); }
    if( func_matches(filter, "ASIN") ) { icount_ASIN(iters); fflush(stdout); }
    if( func_matches(filter, "ATAN") ) { icount_ATAN(iters); fflush(stdout); }
    if( func_matches(filter, "ATR") ) { icount_ATR(iters); fflush(stdout); }
    if( func_matches(filter, "AVGDEV") ) { icount_AVGDEV(iters); fflush(stdout); }
    if( func_matches(filter, "AVGPRICE") ) { icount_AVGPRICE(iters); fflush(stdout); }
    if( func_matches(filter, "BBANDS") ) { icount_BBANDS(iters); fflush(stdout); }
    if( func_matches(filter, "BETA") ) { icount_BETA(iters); fflush(stdout); }
    if( func_matches(filter, "BOP") ) { icount_BOP(iters); fflush(stdout); }
    if( func_matches(filter, "CCI") ) { icount_CCI(iters); fflush(stdout); }
    if( func_matches(filter, "CDL2CROWS") ) { icount_CDL2CROWS(iters); fflush(stdout); }
    if( func_matches(filter, "CDL3BLACKCROWS") ) { icount_CDL3BLACKCROWS(iters); fflush(stdout); }
    if( func_matches(filter, "CDL3INSIDE") ) { icount_CDL3INSIDE(iters); fflush(stdout); }
    if( func_matches(filter, "CDL3LINESTRIKE") ) { icount_CDL3LINESTRIKE(iters); fflush(stdout); }
    if( func_matches(filter, "CDL3OUTSIDE") ) { icount_CDL3OUTSIDE(iters); fflush(stdout); }
    if( func_matches(filter, "CDL3STARSINSOUTH") ) { icount_CDL3STARSINSOUTH(iters); fflush(stdout); }
    if( func_matches(filter, "CDL3WHITESOLDIERS") ) { icount_CDL3WHITESOLDIERS(iters); fflush(stdout); }
    if( func_matches(filter, "CDLABANDONEDBABY") ) { icount_CDLABANDONEDBABY(iters); fflush(stdout); }
    if( func_matches(filter, "CDLADVANCEBLOCK") ) { icount_CDLADVANCEBLOCK(iters); fflush(stdout); }
    if( func_matches(filter, "CDLBELTHOLD") ) { icount_CDLBELTHOLD(iters); fflush(stdout); }
    if( func_matches(filter, "CDLBREAKAWAY") ) { icount_CDLBREAKAWAY(iters); fflush(stdout); }
    if( func_matches(filter, "CDLCLOSINGMARUBOZU") ) { icount_CDLCLOSINGMARUBOZU(iters); fflush(stdout); }
    if( func_matches(filter, "CDLCONCEALBABYSWALL") ) { icount_CDLCONCEALBABYSWALL(iters); fflush(stdout); }
    if( func_matches(filter, "CDLCOUNTERATTACK") ) { icount_CDLCOUNTERATTACK(iters); fflush(stdout); }
    if( func_matches(filter, "CDLDARKCLOUDCOVER") ) { icount_CDLDARKCLOUDCOVER(iters); fflush(stdout); }
    if( func_matches(filter, "CDLDOJI") ) { icount_CDLDOJI(iters); fflush(stdout); }
    if( func_matches(filter, "CDLDOJISTAR") ) { icount_CDLDOJISTAR(iters); fflush(stdout); }
    if( func_matches(filter, "CDLDRAGONFLYDOJI") ) { icount_CDLDRAGONFLYDOJI(iters); fflush(stdout); }
    if( func_matches(filter, "CDLENGULFING") ) { icount_CDLENGULFING(iters); fflush(stdout); }
    if( func_matches(filter, "CDLEVENINGDOJISTAR") ) { icount_CDLEVENINGDOJISTAR(iters); fflush(stdout); }
    if( func_matches(filter, "CDLEVENINGSTAR") ) { icount_CDLEVENINGSTAR(iters); fflush(stdout); }
    if( func_matches(filter, "CDLGAPSIDESIDEWHITE") ) { icount_CDLGAPSIDESIDEWHITE(iters); fflush(stdout); }
    if( func_matches(filter, "CDLGRAVESTONEDOJI") ) { icount_CDLGRAVESTONEDOJI(iters); fflush(stdout); }
    if( func_matches(filter, "CDLHAMMER") ) { icount_CDLHAMMER(iters); fflush(stdout); }
    if( func_matches(filter, "CDLHANGINGMAN") ) { icount_CDLHANGINGMAN(iters); fflush(stdout); }
    if( func_matches(filter, "CDLHARAMI") ) { icount_CDLHARAMI(iters); fflush(stdout); }
    if( func_matches(filter, "CDLHARAMICROSS") ) { icount_CDLHARAMICROSS(iters); fflush(stdout); }
    if( func_matches(filter, "CDLHIGHWAVE") ) { icount_CDLHIGHWAVE(iters); fflush(stdout); }
    if( func_matches(filter, "CDLHIKKAKE") ) { icount_CDLHIKKAKE(iters); fflush(stdout); }
    if( func_matches(filter, "CDLHIKKAKEMOD") ) { icount_CDLHIKKAKEMOD(iters); fflush(stdout); }
    if( func_matches(filter, "CDLHOMINGPIGEON") ) { icount_CDLHOMINGPIGEON(iters); fflush(stdout); }
    if( func_matches(filter, "CDLIDENTICAL3CROWS") ) { icount_CDLIDENTICAL3CROWS(iters); fflush(stdout); }
    if( func_matches(filter, "CDLINNECK") ) { icount_CDLINNECK(iters); fflush(stdout); }
    if( func_matches(filter, "CDLINVERTEDHAMMER") ) { icount_CDLINVERTEDHAMMER(iters); fflush(stdout); }
    if( func_matches(filter, "CDLKICKING") ) { icount_CDLKICKING(iters); fflush(stdout); }
    if( func_matches(filter, "CDLKICKINGBYLENGTH") ) { icount_CDLKICKINGBYLENGTH(iters); fflush(stdout); }
    if( func_matches(filter, "CDLLADDERBOTTOM") ) { icount_CDLLADDERBOTTOM(iters); fflush(stdout); }
    if( func_matches(filter, "CDLLONGLEGGEDDOJI") ) { icount_CDLLONGLEGGEDDOJI(iters); fflush(stdout); }
    if( func_matches(filter, "CDLLONGLINE") ) { icount_CDLLONGLINE(iters); fflush(stdout); }
    if( func_matches(filter, "CDLMARUBOZU") ) { icount_CDLMARUBOZU(iters); fflush(stdout); }
    if( func_matches(filter, "CDLMATCHINGLOW") ) { icount_CDLMATCHINGLOW(iters); fflush(stdout); }
    if( func_matches(filter, "CDLMATHOLD") ) { icount_CDLMATHOLD(iters); fflush(stdout); }
    if( func_matches(filter, "CDLMORNINGDOJISTAR") ) { icount_CDLMORNINGDOJISTAR(iters); fflush(stdout); }
    if( func_matches(filter, "CDLMORNINGSTAR") ) { icount_CDLMORNINGSTAR(iters); fflush(stdout); }
    if( func_matches(filter, "CDLONNECK") ) { icount_CDLONNECK(iters); fflush(stdout); }
    if( func_matches(filter, "CDLPIERCING") ) { icount_CDLPIERCING(iters); fflush(stdout); }
    if( func_matches(filter, "CDLRICKSHAWMAN") ) { icount_CDLRICKSHAWMAN(iters); fflush(stdout); }
    if( func_matches(filter, "CDLRISEFALL3METHODS") ) { icount_CDLRISEFALL3METHODS(iters); fflush(stdout); }
    if( func_matches(filter, "CDLSEPARATINGLINES") ) { icount_CDLSEPARATINGLINES(iters); fflush(stdout); }
    if( func_matches(filter, "CDLSHOOTINGSTAR") ) { icount_CDLSHOOTINGSTAR(iters); fflush(stdout); }
    if( func_matches(filter, "CDLSHORTLINE") ) { icount_CDLSHORTLINE(iters); fflush(stdout); }
    if( func_matches(filter, "CDLSPINNINGTOP") ) { icount_CDLSPINNINGTOP(iters); fflush(stdout); }
    if( func_matches(filter, "CDLSTALLEDPATTERN") ) { icount_CDLSTALLEDPATTERN(iters); fflush(stdout); }
    if( func_matches(filter, "CDLSTICKSANDWICH") ) { icount_CDLSTICKSANDWICH(iters); fflush(stdout); }
    if( func_matches(filter, "CDLTAKURI") ) { icount_CDLTAKURI(iters); fflush(stdout); }
    if( func_matches(filter, "CDLTASUKIGAP") ) { icount_CDLTASUKIGAP(iters); fflush(stdout); }
    if( func_matches(filter, "CDLTHRUSTING") ) { icount_CDLTHRUSTING(iters); fflush(stdout); }
    if( func_matches(filter, "CDLTRISTAR") ) { icount_CDLTRISTAR(iters); fflush(stdout); }
    if( func_matches(filter, "CDLUNIQUE3RIVER") ) { icount_CDLUNIQUE3RIVER(iters); fflush(stdout); }
    if( func_matches(filter, "CDLUPSIDEGAP2CROWS") ) { icount_CDLUPSIDEGAP2CROWS(iters); fflush(stdout); }
    if( func_matches(filter, "CDLXSIDEGAP3METHODS") ) { icount_CDLXSIDEGAP3METHODS(iters); fflush(stdout); }
    if( func_matches(filter, "CEIL") ) { icount_CEIL(iters); fflush(stdout); }
    if( func_matches(filter, "CMF") ) { icount_CMF(iters); fflush(stdout); }
    if( func_matches(filter, "CMO") ) { icount_CMO(iters); fflush(stdout); }
    if( func_matches(filter, "CMOU") ) { icount_CMOU(iters); fflush(stdout); }
    if( func_matches(filter, "COPPOCK") ) { icount_COPPOCK(iters); fflush(stdout); }
    if( func_matches(filter, "CORREL") ) { icount_CORREL(iters); fflush(stdout); }
    if( func_matches(filter, "COS") ) { icount_COS(iters); fflush(stdout); }
    if( func_matches(filter, "COSH") ) { icount_COSH(iters); fflush(stdout); }
    if( func_matches(filter, "CUMSUM") ) { icount_CUMSUM(iters); fflush(stdout); }
    if( func_matches(filter, "CVI") ) { icount_CVI(iters); fflush(stdout); }
    if( func_matches(filter, "DEMA") ) { icount_DEMA(iters); fflush(stdout); }
    if( func_matches(filter, "DIV") ) { icount_DIV(iters); fflush(stdout); }
    if( func_matches(filter, "DONCHIAN") ) { icount_DONCHIAN(iters); fflush(stdout); }
    if( func_matches(filter, "DPO") ) { icount_DPO(iters); fflush(stdout); }
    if( func_matches(filter, "DX") ) { icount_DX(iters); fflush(stdout); }
    if( func_matches(filter, "EFI") ) { icount_EFI(iters); fflush(stdout); }
    if( func_matches(filter, "EMA") ) { icount_EMA(iters); fflush(stdout); }
    if( func_matches(filter, "ER") ) { icount_ER(iters); fflush(stdout); }
    if( func_matches(filter, "ERI") ) { icount_ERI(iters); fflush(stdout); }
    if( func_matches(filter, "EXP") ) { icount_EXP(iters); fflush(stdout); }
    if( func_matches(filter, "FLOOR") ) { icount_FLOOR(iters); fflush(stdout); }
    if( func_matches(filter, "FOSC") ) { icount_FOSC(iters); fflush(stdout); }
    if( func_matches(filter, "FRACTAL") ) { icount_FRACTAL(iters); fflush(stdout); }
    if( func_matches(filter, "HA") ) { icount_HA(iters); fflush(stdout); }
    if( func_matches(filter, "HMA") ) { icount_HMA(iters); fflush(stdout); }
    if( func_matches(filter, "HT_DCPERIOD") ) { icount_HT_DCPERIOD(iters); fflush(stdout); }
    if( func_matches(filter, "HT_DCPHASE") ) { icount_HT_DCPHASE(iters); fflush(stdout); }
    if( func_matches(filter, "HT_PHASOR") ) { icount_HT_PHASOR(iters); fflush(stdout); }
    if( func_matches(filter, "HT_SINE") ) { icount_HT_SINE(iters); fflush(stdout); }
    if( func_matches(filter, "HT_TRENDLINE") ) { icount_HT_TRENDLINE(iters); fflush(stdout); }
    if( func_matches(filter, "HT_TRENDMODE") ) { icount_HT_TRENDMODE(iters); fflush(stdout); }
    if( func_matches(filter, "IMI") ) { icount_IMI(iters); fflush(stdout); }
    if( func_matches(filter, "KAMA") ) { icount_KAMA(iters); fflush(stdout); }
    if( func_matches(filter, "KC") ) { icount_KC(iters); fflush(stdout); }
    if( func_matches(filter, "KDJ") ) { icount_KDJ(iters); fflush(stdout); }
    if( func_matches(filter, "LINEARREG") ) { icount_LINEARREG(iters); fflush(stdout); }
    if( func_matches(filter, "LINEARREG_ANGLE") ) { icount_LINEARREG_ANGLE(iters); fflush(stdout); }
    if( func_matches(filter, "LINEARREG_INTERCEPT") ) { icount_LINEARREG_INTERCEPT(iters); fflush(stdout); }
    if( func_matches(filter, "LINEARREG_SLOPE") ) { icount_LINEARREG_SLOPE(iters); fflush(stdout); }
    if( func_matches(filter, "LN") ) { icount_LN(iters); fflush(stdout); }
    if( func_matches(filter, "LOG10") ) { icount_LOG10(iters); fflush(stdout); }
    if( func_matches(filter, "MA") ) { icount_MA(iters); fflush(stdout); }
    if( func_matches(filter, "MACD") ) { icount_MACD(iters); fflush(stdout); }
    if( func_matches(filter, "MACDEXT") ) { icount_MACDEXT(iters); fflush(stdout); }
    if( func_matches(filter, "MACDFIX") ) { icount_MACDFIX(iters); fflush(stdout); }
    if( func_matches(filter, "MAMA") ) { icount_MAMA(iters); fflush(stdout); }
    if( func_matches(filter, "MARKETFI") ) { icount_MARKETFI(iters); fflush(stdout); }
    if( func_matches(filter, "MASSI") ) { icount_MASSI(iters); fflush(stdout); }
    if( func_matches(filter, "MAVP") ) { icount_MAVP(iters); fflush(stdout); }
    if( func_matches(filter, "MAX") ) { icount_MAX(iters); fflush(stdout); }
    if( func_matches(filter, "MAXINDEX") ) { icount_MAXINDEX(iters); fflush(stdout); }
    if( func_matches(filter, "MEDPRICE") ) { icount_MEDPRICE(iters); fflush(stdout); }
    if( func_matches(filter, "MFI") ) { icount_MFI(iters); fflush(stdout); }
    if( func_matches(filter, "MIDPOINT") ) { icount_MIDPOINT(iters); fflush(stdout); }
    if( func_matches(filter, "MIDPRICE") ) { icount_MIDPRICE(iters); fflush(stdout); }
    if( func_matches(filter, "MIN") ) { icount_MIN(iters); fflush(stdout); }
    if( func_matches(filter, "MININDEX") ) { icount_MININDEX(iters); fflush(stdout); }
    if( func_matches(filter, "MINMAX") ) { icount_MINMAX(iters); fflush(stdout); }
    if( func_matches(filter, "MINMAXINDEX") ) { icount_MINMAXINDEX(iters); fflush(stdout); }
    if( func_matches(filter, "MINUS_DI") ) { icount_MINUS_DI(iters); fflush(stdout); }
    if( func_matches(filter, "MINUS_DM") ) { icount_MINUS_DM(iters); fflush(stdout); }
    if( func_matches(filter, "MOM") ) { icount_MOM(iters); fflush(stdout); }
    if( func_matches(filter, "MULT") ) { icount_MULT(iters); fflush(stdout); }
    if( func_matches(filter, "NATR") ) { icount_NATR(iters); fflush(stdout); }
    if( func_matches(filter, "NVI") ) { icount_NVI(iters); fflush(stdout); }
    if( func_matches(filter, "OBV") ) { icount_OBV(iters); fflush(stdout); }
    if( func_matches(filter, "PERCENTILE") ) { icount_PERCENTILE(iters); fflush(stdout); }
    if( func_matches(filter, "PERCENTRANK") ) { icount_PERCENTRANK(iters); fflush(stdout); }
    if( func_matches(filter, "PLUS_DI") ) { icount_PLUS_DI(iters); fflush(stdout); }
    if( func_matches(filter, "PLUS_DM") ) { icount_PLUS_DM(iters); fflush(stdout); }
    if( func_matches(filter, "PPO") ) { icount_PPO(iters); fflush(stdout); }
    if( func_matches(filter, "PVI") ) { icount_PVI(iters); fflush(stdout); }
    if( func_matches(filter, "PVO") ) { icount_PVO(iters); fflush(stdout); }
    if( func_matches(filter, "PVT") ) { icount_PVT(iters); fflush(stdout); }
    if( func_matches(filter, "QSTICK") ) { icount_QSTICK(iters); fflush(stdout); }
    if( func_matches(filter, "RMA") ) { icount_RMA(iters); fflush(stdout); }
    if( func_matches(filter, "ROC") ) { icount_ROC(iters); fflush(stdout); }
    if( func_matches(filter, "ROCP") ) { icount_ROCP(iters); fflush(stdout); }
    if( func_matches(filter, "ROCR") ) { icount_ROCR(iters); fflush(stdout); }
    if( func_matches(filter, "ROCR100") ) { icount_ROCR100(iters); fflush(stdout); }
    if( func_matches(filter, "RSI") ) { icount_RSI(iters); fflush(stdout); }
    if( func_matches(filter, "RVI") ) { icount_RVI(iters); fflush(stdout); }
    if( func_matches(filter, "RVOL") ) { icount_RVOL(iters); fflush(stdout); }
    if( func_matches(filter, "SAR") ) { icount_SAR(iters); fflush(stdout); }
    if( func_matches(filter, "SAREXT") ) { icount_SAREXT(iters); fflush(stdout); }
    if( func_matches(filter, "SIN") ) { icount_SIN(iters); fflush(stdout); }
    if( func_matches(filter, "SINH") ) { icount_SINH(iters); fflush(stdout); }
    if( func_matches(filter, "SMA") ) { icount_SMA(iters); fflush(stdout); }
    if( func_matches(filter, "SMI") ) { icount_SMI(iters); fflush(stdout); }
    if( func_matches(filter, "SQRT") ) { icount_SQRT(iters); fflush(stdout); }
    if( func_matches(filter, "STDDEV") ) { icount_STDDEV(iters); fflush(stdout); }
    if( func_matches(filter, "STOCH") ) { icount_STOCH(iters); fflush(stdout); }
    if( func_matches(filter, "STOCHF") ) { icount_STOCHF(iters); fflush(stdout); }
    if( func_matches(filter, "STOCHRSI") ) { icount_STOCHRSI(iters); fflush(stdout); }
    if( func_matches(filter, "SUB") ) { icount_SUB(iters); fflush(stdout); }
    if( func_matches(filter, "SUM") ) { icount_SUM(iters); fflush(stdout); }
    if( func_matches(filter, "SUPERTREND") ) { icount_SUPERTREND(iters); fflush(stdout); }
    if( func_matches(filter, "T3") ) { icount_T3(iters); fflush(stdout); }
    if( func_matches(filter, "TAN") ) { icount_TAN(iters); fflush(stdout); }
    if( func_matches(filter, "TANH") ) { icount_TANH(iters); fflush(stdout); }
    if( func_matches(filter, "TEMA") ) { icount_TEMA(iters); fflush(stdout); }
    if( func_matches(filter, "TRANGE") ) { icount_TRANGE(iters); fflush(stdout); }
    if( func_matches(filter, "TRIMA") ) { icount_TRIMA(iters); fflush(stdout); }
    if( func_matches(filter, "TRIX") ) { icount_TRIX(iters); fflush(stdout); }
    if( func_matches(filter, "TSF") ) { icount_TSF(iters); fflush(stdout); }
    if( func_matches(filter, "TSI") ) { icount_TSI(iters); fflush(stdout); }
    if( func_matches(filter, "TYPPRICE") ) { icount_TYPPRICE(iters); fflush(stdout); }
    if( func_matches(filter, "ULTOSC") ) { icount_ULTOSC(iters); fflush(stdout); }
    if( func_matches(filter, "VAR") ) { icount_VAR(iters); fflush(stdout); }
    if( func_matches(filter, "VHF") ) { icount_VHF(iters); fflush(stdout); }
    if( func_matches(filter, "VORTEX") ) { icount_VORTEX(iters); fflush(stdout); }
    if( func_matches(filter, "VWAP") ) { icount_VWAP(iters); fflush(stdout); }
    if( func_matches(filter, "VWMA") ) { icount_VWMA(iters); fflush(stdout); }
    if( func_matches(filter, "WAD") ) { icount_WAD(iters); fflush(stdout); }
    if( func_matches(filter, "WCLPRICE") ) { icount_WCLPRICE(iters); fflush(stdout); }
    if( func_matches(filter, "WILLR") ) { icount_WILLR(iters); fflush(stdout); }
    if( func_matches(filter, "WMA") ) { icount_WMA(iters); fflush(stdout); }
    if( func_matches(filter, "ZLEMA") ) { icount_ZLEMA(iters); fflush(stdout); }
}

int main(int argc, char *argv[]) {
    int n_points = 20000;
    int n_iters = 1024;
    int verify_corpus = 0;
    int dry_run = 0;
    const char *func_filter = NULL;
    TA_Initialize();
    bench_corpus_defaults(&g_corpus);
    for( int i = 1; i < argc; i++ ) {
        if( strncmp(argv[i], "--points=", 9) == 0 )    n_points = atoi(argv[i]+9);
        else if( strncmp(argv[i], "--stream-iters=", 15) == 0 ) n_iters = atoi(argv[i]+15);
        else if( strncmp(argv[i], "--function=", 11) == 0 ) func_filter = argv[i]+11;
        /* Prints the rows without measuring, so the row set can be checked
           where valgrind is not installed. Never used by the comparator. */
        else if( strcmp(argv[i], "--dry-run") == 0 ) dry_run = 1;
        else if( strncmp(argv[i], "--shape=", 8) == 0 ) {
            g_corpus.shape = bench_shape_id(argv[i]+8);
            if( g_corpus.shape < 0 ) {
                printf("unknown --shape=%s\n\n", argv[i]+8);
                bench_shape_list();
                return 1;
            }
        }
        else if( strncmp(argv[i], "--seed=", 7) == 0 )   g_corpus.seed = atoi(argv[i]+7);
        else if( strncmp(argv[i], "--regime-period=", 16) == 0 ) g_corpus.refPeriod = atoi(argv[i]+16);
        else if( strncmp(argv[i], "--trend-strength=", 17) == 0 ) g_corpus.trendStrength = atof(argv[i]+17);
        else if( strcmp(argv[i], "--list-shapes") == 0 ) { bench_shape_list(); return 0; }
        else if( strcmp(argv[i], "--verify-corpus") == 0 ) verify_corpus = 1;
        else {
            /* Reject rather than ignore. ta_bench_direct forwards the corpus
             * flags to this binary unconditionally, so a silently-dropped flag
             * makes it time two DIFFERENT input classes and print the ratio as
             * if they matched — a wrong answer with no diagnostic. */
            fprintf(stderr, "%s: unknown option '%s'\n", argv[0], argv[i]);
            return 2;
        }
    }
    if( n_points > MAX_POINTS ) n_points = MAX_POINTS;
    if( n_points < ICOUNT_MASK + 1 ) n_points = ICOUNT_MASK + 1;
    if( n_iters < 1 ) n_iters = 1;
    if( verify_corpus ) return bench_corpus_selfcheck(n_points, &g_corpus) ? 1 : 0;
    if( !dry_run ) {
#ifndef ICOUNT_HAVE_CALLGRIND
        fprintf(stderr, "ta_bench_icount: built without <valgrind/callgrind.h>;"
                        " install valgrind and rebuild, or pass --dry-run\n");
        return 3;
#endif
        if( !ICOUNT_UNDER_VG() ) {
            fprintf(stderr, "ta_bench_icount: not running under valgrind; invoke as\n"
                            "  valgrind --tool=callgrind --combine-dumps=yes ./ta_bench_icount\n");
            return 3;
        }
    }
    generate_price_data(n_points);
    printf("# ta_bench_icount points=%d stream_iters=%d shape=%s seed=%d regime_period=%d trend_strength=%.6f\n",
           n_points, n_iters, bench_shape_name(g_corpus.shape), g_corpus.seed,
           g_corpus.refPeriod, g_corpus.trendStrength);
    fflush(stdout);
    icount_all(func_filter, n_iters);
    printf("# rows=%d measured=%d skipped=%d sink=%d\n", g_rows, g_measured, g_skipped, g_sink);
    free(g_open); free(g_high); free(g_low); free(g_close); free(g_volume); free(g_oi); free(g_periods);
    return 0;
}
