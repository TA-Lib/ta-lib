/* Auto-generated streaming benchmark for ta_codegen C output.
 * Per streamable function: batch@last vs Update vs Peek (ns) + handle bytes.
 * Output: `NAME batch_last update peek lookback handle_bytes` per line.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <ctype.h>
#include <limits.h>
#ifdef _WIN32
#include <windows.h>
#endif
#ifdef __APPLE__
#include <mach/mach_time.h>
#endif

#include "bench_corpus.h"

#include "tools/ta_alloc_check.h"

#include "ta_func/ta_func_stream_private.h"

#include "ta_common/ta_global.c"
#include "ta_func/ta_utility.c"
#include "ta_common/ta_version.c"
#include "ta_common/ta_retcode.c"


/* --- retained-handle allocation tracking (indicator TUs only) --- */
#define TRK_MAX 65536
static void  *g_trk_ptr[TRK_MAX];
static size_t g_trk_sz[TRK_MAX];
static int    g_trk_n = 0;
static size_t g_ta_live_bytes = 0;
static int    g_ta_track = 0;
static void g_trk_reset(void) { g_trk_n = 0; g_ta_live_bytes = 0; }
static void *bench_tracked_malloc(size_t n) {
    void *p = malloc(n);
    if( p && g_ta_track ) {
        g_ta_live_bytes += n;
        if( g_trk_n < TRK_MAX ) { g_trk_ptr[g_trk_n] = p; g_trk_sz[g_trk_n] = n; g_trk_n++; }
    }
    return p;
}
static void bench_tracked_free(void *p) {
    if( p && g_ta_track ) {
        for( int i = g_trk_n - 1; i >= 0; i-- ) {
            if( g_trk_ptr[i] == p ) {
                g_ta_live_bytes -= g_trk_sz[i];
                g_trk_ptr[i] = g_trk_ptr[--g_trk_n];
                g_trk_sz[i]  = g_trk_sz[g_trk_n];
                break;
            }
        }
    }
    free(p);
}
#undef TA_Malloc
#undef TA_Free
#define TA_Malloc(a) bench_tracked_malloc(a)
#define TA_Free(a)   bench_tracked_free(a)

#include "ta_AC.c"
#include "ta_ACCBANDS.c"
#include "ta_ACOS.c"
#include "ta_AD.c"
#include "ta_ADD.c"
#include "ta_ADOSC.c"
#include "ta_ADR.c"
#include "ta_ADX.c"
#include "ta_ADXR.c"
#include "ta_AO.c"
#include "ta_APO.c"
#include "ta_AROON.c"
#include "ta_AROONOSC.c"
#include "ta_ASIN.c"
#include "ta_ATAN.c"
#include "ta_ATR.c"
#include "ta_AVGDEV.c"
#include "ta_AVGPRICE.c"
#include "ta_BBANDS.c"
#include "ta_BBW.c"
#include "ta_BETA.c"
#include "ta_BOP.c"
#include "ta_CCI.c"
#include "ta_CDL2CROWS.c"
#include "ta_CDL3BLACKCROWS.c"
#include "ta_CDL3INSIDE.c"
#include "ta_CDL3LINESTRIKE.c"
#include "ta_CDL3OUTSIDE.c"
#include "ta_CDL3STARSINSOUTH.c"
#include "ta_CDL3WHITESOLDIERS.c"
#include "ta_CDLABANDONEDBABY.c"
#include "ta_CDLADVANCEBLOCK.c"
#include "ta_CDLBELTHOLD.c"
#include "ta_CDLBREAKAWAY.c"
#include "ta_CDLCLOSINGMARUBOZU.c"
#include "ta_CDLCONCEALBABYSWALL.c"
#include "ta_CDLCOUNTERATTACK.c"
#include "ta_CDLDARKCLOUDCOVER.c"
#include "ta_CDLDOJI.c"
#include "ta_CDLDOJISTAR.c"
#include "ta_CDLDRAGONFLYDOJI.c"
#include "ta_CDLENGULFING.c"
#include "ta_CDLEVENINGDOJISTAR.c"
#include "ta_CDLEVENINGSTAR.c"
#include "ta_CDLGAPSIDESIDEWHITE.c"
#include "ta_CDLGRAVESTONEDOJI.c"
#include "ta_CDLHAMMER.c"
#include "ta_CDLHANGINGMAN.c"
#include "ta_CDLHARAMI.c"
#include "ta_CDLHARAMICROSS.c"
#include "ta_CDLHIGHWAVE.c"
#include "ta_CDLHIKKAKE.c"
#include "ta_CDLHIKKAKEMOD.c"
#include "ta_CDLHOMINGPIGEON.c"
#include "ta_CDLIDENTICAL3CROWS.c"
#include "ta_CDLINNECK.c"
#include "ta_CDLINVERTEDHAMMER.c"
#include "ta_CDLKICKING.c"
#include "ta_CDLKICKINGBYLENGTH.c"
#include "ta_CDLLADDERBOTTOM.c"
#include "ta_CDLLONGLEGGEDDOJI.c"
#include "ta_CDLLONGLINE.c"
#include "ta_CDLMARUBOZU.c"
#include "ta_CDLMATCHINGLOW.c"
#include "ta_CDLMATHOLD.c"
#include "ta_CDLMORNINGDOJISTAR.c"
#include "ta_CDLMORNINGSTAR.c"
#include "ta_CDLONNECK.c"
#include "ta_CDLPIERCING.c"
#include "ta_CDLRICKSHAWMAN.c"
#include "ta_CDLRISEFALL3METHODS.c"
#include "ta_CDLSEPARATINGLINES.c"
#include "ta_CDLSHOOTINGSTAR.c"
#include "ta_CDLSHORTLINE.c"
#include "ta_CDLSPINNINGTOP.c"
#include "ta_CDLSTALLEDPATTERN.c"
#include "ta_CDLSTICKSANDWICH.c"
#include "ta_CDLTAKURI.c"
#include "ta_CDLTASUKIGAP.c"
#include "ta_CDLTHRUSTING.c"
#include "ta_CDLTRISTAR.c"
#include "ta_CDLUNIQUE3RIVER.c"
#include "ta_CDLUPSIDEGAP2CROWS.c"
#include "ta_CDLXSIDEGAP3METHODS.c"
#include "ta_CEIL.c"
#include "ta_CG.c"
#include "ta_CMF.c"
#include "ta_CMO.c"
#include "ta_CMOU.c"
#include "ta_COPPOCK.c"
#include "ta_CORREL.c"
#include "ta_COS.c"
#include "ta_COSH.c"
#include "ta_CRSI.c"
#include "ta_CTI.c"
#include "ta_CUMSUM.c"
#include "ta_CVI.c"
#include "ta_DEMA.c"
#include "ta_DIV.c"
#include "ta_DONCHIAN.c"
#include "ta_DPO.c"
#include "ta_DX.c"
#include "ta_EFI.c"
#include "ta_EMA.c"
#include "ta_ER.c"
#include "ta_ERI.c"
#include "ta_EXP.c"
#include "ta_FLOOR.c"
#include "ta_FOSC.c"
#include "ta_FRACTAL.c"
#include "ta_HA.c"
#include "ta_HMA.c"
#include "ta_HT_DCPERIOD.c"
#include "ta_HT_DCPHASE.c"
#include "ta_HT_PHASOR.c"
#include "ta_HT_SINE.c"
#include "ta_HT_TRENDLINE.c"
#include "ta_HT_TRENDMODE.c"
#include "ta_IMI.c"
#include "ta_KAMA.c"
#include "ta_KC.c"
#include "ta_KDJ.c"
#include "ta_KURTOSIS.c"
#include "ta_LINEARREG.c"
#include "ta_LINEARREG_ANGLE.c"
#include "ta_LINEARREG_INTERCEPT.c"
#include "ta_LINEARREG_SLOPE.c"
#include "ta_LN.c"
#include "ta_LOG10.c"
#include "ta_MACD.c"
#include "ta_MACDEXT.c"
#include "ta_MACDFIX.c"
#include "ta_MAMA.c"
#include "ta_MARKETFI.c"
#include "ta_MASSI.c"
#include "ta_MAVP.c"
#include "ta_MAX.c"
#include "ta_MAXINDEX.c"
#include "ta_MEDIAN.c"
#include "ta_MEDPRICE.c"
#include "ta_MFI.c"
#include "ta_MIDPOINT.c"
#include "ta_MIDPRICE.c"
#include "ta_MIN.c"
#include "ta_MININDEX.c"
#include "ta_MINMAX.c"
#include "ta_MINMAXINDEX.c"
#include "ta_MINUS_DI.c"
#include "ta_MINUS_DM.c"
#include "ta_MOM.c"
#include "ta_MULT.c"
#include "ta_NATR.c"
#include "ta_NVI.c"
#include "ta_OBV.c"
#include "ta_PERCENTB.c"
#include "ta_PERCENTILE.c"
#include "ta_PERCENTRANK.c"
#include "ta_PLUS_DI.c"
#include "ta_PLUS_DM.c"
#include "ta_PPO.c"
#include "ta_PVI.c"
#include "ta_PVO.c"
#include "ta_PVT.c"
#include "ta_QSTICK.c"
#include "ta_RMA.c"
#include "ta_ROC.c"
#include "ta_ROCP.c"
#include "ta_ROCR.c"
#include "ta_ROCR100.c"
#include "ta_RSI.c"
#include "ta_RVI.c"
#include "ta_RVIR.c"
#include "ta_RVOL.c"
#include "ta_SAR.c"
#include "ta_SAREXT.c"
#include "ta_SIN.c"
#include "ta_SINH.c"
#include "ta_SMA.c"
#include "ta_SMI.c"
#include "ta_SQRT.c"
#include "ta_STDDEV.c"
#include "ta_STOCH.c"
#include "ta_STOCHF.c"
#include "ta_STOCHRSI.c"
#include "ta_SUB.c"
#include "ta_SUM.c"
#include "ta_SUPERTREND.c"
#include "ta_T3.c"
#include "ta_TAN.c"
#include "ta_TANH.c"
#include "ta_TEMA.c"
#include "ta_TRANGE.c"
#include "ta_TRIMA.c"
#include "ta_TRIX.c"
#include "ta_TSF.c"
#include "ta_TSI.c"
#include "ta_TYPPRICE.c"
#include "ta_ULTOSC.c"
#include "ta_VAR.c"
#include "ta_VHF.c"
#include "ta_VORTEX.c"
#include "ta_VWAP.c"
#include "ta_VWMA.c"
#include "ta_WAD.c"
#include "ta_WCLPRICE.c"
#include "ta_WILLR.c"
#include "ta_WMA.c"
#include "ta_ZLEMA.c"
#include "ta_MA.c"


static long long get_nanotime(void) {
#if defined(_WIN32)
    /* No clock_gettime in the MSVC CRT. QueryPerformanceCounter is the
     * monotonic counter here; its frequency is fixed for the lifetime of the
     * process, so one query is enough. Scaling ticks->ns as (t/f)*1e9 would
     * truncate to whole seconds, and t*1e9 overflows a signed 64-bit at
     * ~9.2e9 ticks (roughly an hour at a 10 MHz QPC), so split the tick count
     * into whole seconds plus a remainder before scaling.
     */
    static LARGE_INTEGER freq = {0};
    LARGE_INTEGER now;
    if( freq.QuadPart == 0 ) QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&now);
    return (long long)((now.QuadPart / freq.QuadPart) * 1000000000LL
         + ((now.QuadPart % freq.QuadPart) * 1000000000LL) / freq.QuadPart);
#elif defined(__APPLE__)
    static mach_timebase_info_data_t info = {0, 0};
    if( info.denom == 0 ) mach_timebase_info(&info);
    uint64_t t = mach_absolute_time();
    return (long long)(t * info.numer / info.denom);
#else
    struct timespec ts;
    if( clock_gettime(CLOCK_MONOTONIC, &ts) == 0 )
        return (long long)ts.tv_sec * 1000000000LL + (long long)ts.tv_nsec;
    return 0;
#endif
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

static double *g_rt_open, *g_rt_high, *g_rt_low, *g_rt_close, *g_rt_volume, *g_rt_oi, *g_rt_periods;
static int g_rtCap;


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

static volatile int g_sink = 0;

#define BENCH_MASK 4095

static double g_min_ratio = 0.0;   /* 0 = report only, no gate */
static int    g_period = 0;        /* --period; 0 = every optInTimePeriod at its default */
static double g_worst_ratio = -1.0;
static char   g_worst_name[64] = "";
static int    g_rows = 0, g_slow = 0, g_below = 0, g_reject = 0, g_short = 0;

/* Every param reaches Lookback, batch and Open through one of these: a literal
   folds into the inlined batch body and times a cheaper call than the
   library's, with nothing to show it. */
static int    bench_opaque_int(int v)       { volatile int x = v; return x; }
static double bench_opaque_double(double v) { volatile double x = v; return x; }

/* batch@last appends from index lb, so the history must hold lb + iters bars;
   a long --period outgrows the headroom main reserves. */
static void bench_rt_reserve(long long need) {
    double **rt[7] = { &g_rt_open, &g_rt_high, &g_rt_low, &g_rt_close,
                       &g_rt_volume, &g_rt_oi, &g_rt_periods };
    const double *src[7] = { g_open, g_high, g_low, g_close, g_volume, g_oi, g_periods };
    if( need <= g_rtCap ) return;
    if( need > INT_MAX ) TA_TOOL_OOM("the batch@last history");
    for( int k = 0; k < 7; k++ ) {
        double *p = realloc(*rt[k], sizeof(double) * (size_t)need);
        if( !p ) TA_TOOL_OOM("the batch@last history");
        for( int i = g_rtCap; i < (int)need; i++ ) p[i] = src[k][i % g_nPoints];
        *rt[k] = p;
    }
    g_rtCap = (int)need;
}

static void bench_stream_row(const char *name, TA_RetCode orc, double b, double u,
                             double p, int lb, size_t hb)
{
    if( u <= 0.0 && orc == TA_INSUFFICIENT_HISTORY ) {   /* lb >= --points */
        printf("%s %.3f -1 -1 %d 0 short\n", name, b, lb);
        g_short++;
        return;
    }
    if( u <= 0.0 ) {   /* Open rejected the params */
        /* lb < 0: Lookback refused them too, so batch_last timed rejections. */
        if( lb < 0 ) printf("%s -1 -1 -1 %d 0 -1\n", name, lb);
        else         printf("%s %.3f -1 -1 %d 0 -1\n", name, b, lb);
        g_reject++;
        return;
    }
    double r = b / u;
    printf("%s %.3f %.3f %.3f %d %zu %.3f\n", name, b, u, p, lb, hb, r);
    g_rows++;
    if( r < 1.0 ) g_slow++;
    if( g_min_ratio > 0.0 && r < g_min_ratio ) g_below++;
    if( g_worst_ratio < 0.0 || r < g_worst_ratio ) {
        g_worst_ratio = r;
        snprintf(g_worst_name, sizeof(g_worst_name), "%s", name);
    }
}

/* Returns the process exit code: non-zero only when --min-ratio is set and
   something came in under it, so this can gate a nightly. */
static int bench_stream_summary(void)
{
    printf("# %d timed, %d rejected, %d short (lookback >= --points); "
           "%d slower than batch@last; worst %s %.2fx\n",
           g_rows, g_reject, g_short, g_slow,
           g_worst_name[0] ? g_worst_name : "-", g_worst_ratio);
    if( g_min_ratio > 0.0 ) {
        printf("# --min-ratio=%.2f: %d below -> %s\n",
               g_min_ratio, g_below, g_below ? "FAIL" : "PASS");
        return g_below ? 1 : 0;
    }
    return 0;
}

static void bench_stream_all(const char *filter, int iters) {
    if( g_period > 0 )
        printf("# --period=%d: every optInTimePeriod; all other params at their defaults\n", g_period);
    printf("# func batch_last_ns update_ns peek_ns lookback handle_bytes speedup\n");
    fflush(stdout);
    if( func_matches(filter, "AC") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        const int optInFastPeriod = bench_opaque_int(5);
        const int optInSlowPeriod = bench_opaque_int(34);
        const int optInSignalPeriod = bench_opaque_int(5);
        int lb = TA_AC_Lookback(optInFastPeriod, optInSlowPeriod, optInSignalPeriod);
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_high[t] = g_high[it & BENCH_MASK];
                g_rt_low[t] = g_low[it & BENCH_MASK];
                TA_AC(t, t, g_rt_high, g_rt_low, optInFastPeriod, optInSlowPeriod, optInSignalPeriod, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_AC_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_AC_Open(&st, g_high, g_low, g_nPoints, optInFastPeriod, optInSlowPeriod, optInSignalPeriod, &v0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_AC_Update(st, g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], &v0);
                    acc += v0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_AC_Peek(st, g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_AC_Update(st, g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_AC_Close(st);
            bench_stream_row("AC", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_AC_Close(st); }
            bench_stream_row("AC", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "ACCBANDS") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        const int optInTimePeriod = bench_opaque_int(g_period > 0 ? g_period : 20);
        int lb = TA_ACCBANDS_Lookback(optInTimePeriod);
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_high[t] = g_high[it & BENCH_MASK];
                g_rt_low[t] = g_low[it & BENCH_MASK];
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_ACCBANDS(t, t, g_rt_high, g_rt_low, g_rt_close, optInTimePeriod, &begIdx, &nb, g_outBuf0, g_outBuf1, g_outBuf2);
                acc += g_outBuf0[0];
                acc += g_outBuf1[0];
                acc += g_outBuf2[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_ACCBANDS_Stream *st = NULL;
            double v0 = 0.0;
            double v1 = 0.0;
            double v2 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_ACCBANDS_Open(&st, g_high, g_low, g_close, g_nPoints, optInTimePeriod, &v0, &v1, &v2);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_ACCBANDS_Update(st, g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &v0, &v1, &v2);
                    acc += v0;
                    acc += v1;
                    acc += v2;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_ACCBANDS_Peek(st, g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &v0, &v1, &v2);
                        acc += v0;
                        acc += v1;
                        acc += v2;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_ACCBANDS_Update(st, g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &v0, &v1, &v2);
                        acc += v0;
                        acc += v1;
                        acc += v2;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_ACCBANDS_Close(st);
            bench_stream_row("ACCBANDS", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_ACCBANDS_Close(st); }
            bench_stream_row("ACCBANDS", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "ACOS") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_ACOS_Lookback();
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_ACOS(t, t, g_rt_close, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_ACOS_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_ACOS_Open(&st, g_close, g_nPoints, &v0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_ACOS_Update(st, g_close[it & BENCH_MASK], &v0);
                    acc += v0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_ACOS_Peek(st, g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_ACOS_Update(st, g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_ACOS_Close(st);
            bench_stream_row("ACOS", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_ACOS_Close(st); }
            bench_stream_row("ACOS", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "AD") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_AD_Lookback();
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_high[t] = g_high[it & BENCH_MASK];
                g_rt_low[t] = g_low[it & BENCH_MASK];
                g_rt_close[t] = g_close[it & BENCH_MASK];
                g_rt_volume[t] = g_volume[it & BENCH_MASK];
                TA_AD(t, t, g_rt_high, g_rt_low, g_rt_close, g_rt_volume, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_AD_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_AD_Open(&st, g_high, g_low, g_close, g_volume, g_nPoints, &v0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_AD_Update(st, g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], g_volume[it & BENCH_MASK], &v0);
                    acc += v0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_AD_Peek(st, g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], g_volume[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_AD_Update(st, g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], g_volume[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_AD_Close(st);
            bench_stream_row("AD", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_AD_Close(st); }
            bench_stream_row("AD", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "ADD") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_ADD_Lookback();
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_close[t] = g_close[it & BENCH_MASK];
                g_rt_high[t] = g_high[it & BENCH_MASK];
                TA_ADD(t, t, g_rt_close, g_rt_high, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_ADD_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_ADD_Open(&st, g_close, g_high, g_nPoints, &v0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_ADD_Update(st, g_close[it & BENCH_MASK], g_high[it & BENCH_MASK], &v0);
                    acc += v0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_ADD_Peek(st, g_close[it & BENCH_MASK], g_high[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_ADD_Update(st, g_close[it & BENCH_MASK], g_high[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_ADD_Close(st);
            bench_stream_row("ADD", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_ADD_Close(st); }
            bench_stream_row("ADD", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "ADOSC") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        const int optInFastPeriod = bench_opaque_int(3);
        const int optInSlowPeriod = bench_opaque_int(10);
        int lb = TA_ADOSC_Lookback(optInFastPeriod, optInSlowPeriod);
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_high[t] = g_high[it & BENCH_MASK];
                g_rt_low[t] = g_low[it & BENCH_MASK];
                g_rt_close[t] = g_close[it & BENCH_MASK];
                g_rt_volume[t] = g_volume[it & BENCH_MASK];
                TA_ADOSC(t, t, g_rt_high, g_rt_low, g_rt_close, g_rt_volume, optInFastPeriod, optInSlowPeriod, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_ADOSC_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_ADOSC_Open(&st, g_high, g_low, g_close, g_volume, g_nPoints, optInFastPeriod, optInSlowPeriod, &v0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_ADOSC_Update(st, g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], g_volume[it & BENCH_MASK], &v0);
                    acc += v0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_ADOSC_Peek(st, g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], g_volume[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_ADOSC_Update(st, g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], g_volume[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_ADOSC_Close(st);
            bench_stream_row("ADOSC", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_ADOSC_Close(st); }
            bench_stream_row("ADOSC", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "ADR") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        const int optInTimePeriod = bench_opaque_int(g_period > 0 ? g_period : 14);
        int lb = TA_ADR_Lookback(optInTimePeriod);
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_high[t] = g_high[it & BENCH_MASK];
                g_rt_low[t] = g_low[it & BENCH_MASK];
                TA_ADR(t, t, g_rt_high, g_rt_low, optInTimePeriod, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_ADR_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_ADR_Open(&st, g_high, g_low, g_nPoints, optInTimePeriod, &v0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_ADR_Update(st, g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], &v0);
                    acc += v0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_ADR_Peek(st, g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_ADR_Update(st, g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_ADR_Close(st);
            bench_stream_row("ADR", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_ADR_Close(st); }
            bench_stream_row("ADR", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "ADX") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        const int optInTimePeriod = bench_opaque_int(g_period > 0 ? g_period : 14);
        int lb = TA_ADX_Lookback(optInTimePeriod);
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_high[t] = g_high[it & BENCH_MASK];
                g_rt_low[t] = g_low[it & BENCH_MASK];
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_ADX(t, t, g_rt_high, g_rt_low, g_rt_close, optInTimePeriod, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_ADX_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_ADX_Open(&st, g_high, g_low, g_close, g_nPoints, optInTimePeriod, &v0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_ADX_Update(st, g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &v0);
                    acc += v0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_ADX_Peek(st, g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_ADX_Update(st, g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_ADX_Close(st);
            bench_stream_row("ADX", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_ADX_Close(st); }
            bench_stream_row("ADX", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "ADXR") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        const int optInTimePeriod = bench_opaque_int(g_period > 0 ? g_period : 14);
        int lb = TA_ADXR_Lookback(optInTimePeriod);
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_high[t] = g_high[it & BENCH_MASK];
                g_rt_low[t] = g_low[it & BENCH_MASK];
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_ADXR(t, t, g_rt_high, g_rt_low, g_rt_close, optInTimePeriod, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_ADXR_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_ADXR_Open(&st, g_high, g_low, g_close, g_nPoints, optInTimePeriod, &v0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_ADXR_Update(st, g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &v0);
                    acc += v0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_ADXR_Peek(st, g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_ADXR_Update(st, g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_ADXR_Close(st);
            bench_stream_row("ADXR", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_ADXR_Close(st); }
            bench_stream_row("ADXR", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "AO") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        const int optInFastPeriod = bench_opaque_int(5);
        const int optInSlowPeriod = bench_opaque_int(34);
        int lb = TA_AO_Lookback(optInFastPeriod, optInSlowPeriod);
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_high[t] = g_high[it & BENCH_MASK];
                g_rt_low[t] = g_low[it & BENCH_MASK];
                TA_AO(t, t, g_rt_high, g_rt_low, optInFastPeriod, optInSlowPeriod, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_AO_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_AO_Open(&st, g_high, g_low, g_nPoints, optInFastPeriod, optInSlowPeriod, &v0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_AO_Update(st, g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], &v0);
                    acc += v0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_AO_Peek(st, g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_AO_Update(st, g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_AO_Close(st);
            bench_stream_row("AO", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_AO_Close(st); }
            bench_stream_row("AO", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "APO") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        const int optInFastPeriod = bench_opaque_int(12);
        const int optInSlowPeriod = bench_opaque_int(26);
        const int optInMAType = bench_opaque_int(1);
        int lb = TA_APO_Lookback(optInFastPeriod, optInSlowPeriod, optInMAType);
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_APO(t, t, g_rt_close, optInFastPeriod, optInSlowPeriod, optInMAType, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_APO_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_APO_Open(&st, g_close, g_nPoints, optInFastPeriod, optInSlowPeriod, optInMAType, &v0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_APO_Update(st, g_close[it & BENCH_MASK], &v0);
                    acc += v0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_APO_Peek(st, g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_APO_Update(st, g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_APO_Close(st);
            bench_stream_row("APO", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_APO_Close(st); }
            bench_stream_row("APO", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "AROON") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        const int optInTimePeriod = bench_opaque_int(g_period > 0 ? g_period : 14);
        int lb = TA_AROON_Lookback(optInTimePeriod);
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_high[t] = g_high[it & BENCH_MASK];
                g_rt_low[t] = g_low[it & BENCH_MASK];
                TA_AROON(t, t, g_rt_high, g_rt_low, optInTimePeriod, &begIdx, &nb, g_outBuf0, g_outBuf1);
                acc += g_outBuf0[0];
                acc += g_outBuf1[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_AROON_Stream *st = NULL;
            double v0 = 0.0;
            double v1 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_AROON_Open(&st, g_high, g_low, g_nPoints, optInTimePeriod, &v0, &v1);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_AROON_Update(st, g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], &v0, &v1);
                    acc += v0;
                    acc += v1;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_AROON_Peek(st, g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], &v0, &v1);
                        acc += v0;
                        acc += v1;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_AROON_Update(st, g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], &v0, &v1);
                        acc += v0;
                        acc += v1;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_AROON_Close(st);
            bench_stream_row("AROON", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_AROON_Close(st); }
            bench_stream_row("AROON", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "AROONOSC") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        const int optInTimePeriod = bench_opaque_int(g_period > 0 ? g_period : 14);
        int lb = TA_AROONOSC_Lookback(optInTimePeriod);
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_high[t] = g_high[it & BENCH_MASK];
                g_rt_low[t] = g_low[it & BENCH_MASK];
                TA_AROONOSC(t, t, g_rt_high, g_rt_low, optInTimePeriod, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_AROONOSC_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_AROONOSC_Open(&st, g_high, g_low, g_nPoints, optInTimePeriod, &v0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_AROONOSC_Update(st, g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], &v0);
                    acc += v0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_AROONOSC_Peek(st, g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_AROONOSC_Update(st, g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_AROONOSC_Close(st);
            bench_stream_row("AROONOSC", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_AROONOSC_Close(st); }
            bench_stream_row("AROONOSC", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "ASIN") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_ASIN_Lookback();
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_ASIN(t, t, g_rt_close, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_ASIN_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_ASIN_Open(&st, g_close, g_nPoints, &v0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_ASIN_Update(st, g_close[it & BENCH_MASK], &v0);
                    acc += v0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_ASIN_Peek(st, g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_ASIN_Update(st, g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_ASIN_Close(st);
            bench_stream_row("ASIN", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_ASIN_Close(st); }
            bench_stream_row("ASIN", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "ATAN") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_ATAN_Lookback();
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_ATAN(t, t, g_rt_close, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_ATAN_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_ATAN_Open(&st, g_close, g_nPoints, &v0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_ATAN_Update(st, g_close[it & BENCH_MASK], &v0);
                    acc += v0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_ATAN_Peek(st, g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_ATAN_Update(st, g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_ATAN_Close(st);
            bench_stream_row("ATAN", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_ATAN_Close(st); }
            bench_stream_row("ATAN", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "ATR") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        const int optInTimePeriod = bench_opaque_int(g_period > 0 ? g_period : 14);
        int lb = TA_ATR_Lookback(optInTimePeriod);
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_high[t] = g_high[it & BENCH_MASK];
                g_rt_low[t] = g_low[it & BENCH_MASK];
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_ATR(t, t, g_rt_high, g_rt_low, g_rt_close, optInTimePeriod, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_ATR_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_ATR_Open(&st, g_high, g_low, g_close, g_nPoints, optInTimePeriod, &v0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_ATR_Update(st, g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &v0);
                    acc += v0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_ATR_Peek(st, g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_ATR_Update(st, g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_ATR_Close(st);
            bench_stream_row("ATR", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_ATR_Close(st); }
            bench_stream_row("ATR", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "AVGDEV") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        const int optInTimePeriod = bench_opaque_int(g_period > 0 ? g_period : 14);
        int lb = TA_AVGDEV_Lookback(optInTimePeriod);
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_AVGDEV(t, t, g_rt_close, optInTimePeriod, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_AVGDEV_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_AVGDEV_Open(&st, g_close, g_nPoints, optInTimePeriod, &v0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_AVGDEV_Update(st, g_close[it & BENCH_MASK], &v0);
                    acc += v0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_AVGDEV_Peek(st, g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_AVGDEV_Update(st, g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_AVGDEV_Close(st);
            bench_stream_row("AVGDEV", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_AVGDEV_Close(st); }
            bench_stream_row("AVGDEV", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "AVGPRICE") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_AVGPRICE_Lookback();
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_open[t] = g_open[it & BENCH_MASK];
                g_rt_high[t] = g_high[it & BENCH_MASK];
                g_rt_low[t] = g_low[it & BENCH_MASK];
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_AVGPRICE(t, t, g_rt_open, g_rt_high, g_rt_low, g_rt_close, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_AVGPRICE_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_AVGPRICE_Open(&st, g_open, g_high, g_low, g_close, g_nPoints, &v0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_AVGPRICE_Update(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &v0);
                    acc += v0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_AVGPRICE_Peek(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_AVGPRICE_Update(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_AVGPRICE_Close(st);
            bench_stream_row("AVGPRICE", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_AVGPRICE_Close(st); }
            bench_stream_row("AVGPRICE", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "BBANDS") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        const int optInTimePeriod = bench_opaque_int(g_period > 0 ? g_period : 20);
        const double optInNbDevUp = bench_opaque_double(2.000000000000000);
        const double optInNbDevDn = bench_opaque_double(2.000000000000000);
        const int optInMAType = bench_opaque_int(0);
        int lb = TA_BBANDS_Lookback(optInTimePeriod, optInNbDevUp, optInNbDevDn, optInMAType);
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_BBANDS(t, t, g_rt_close, optInTimePeriod, optInNbDevUp, optInNbDevDn, optInMAType, &begIdx, &nb, g_outBuf0, g_outBuf1, g_outBuf2);
                acc += g_outBuf0[0];
                acc += g_outBuf1[0];
                acc += g_outBuf2[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_BBANDS_Stream *st = NULL;
            double v0 = 0.0;
            double v1 = 0.0;
            double v2 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_BBANDS_Open(&st, g_close, g_nPoints, optInTimePeriod, optInNbDevUp, optInNbDevDn, optInMAType, &v0, &v1, &v2);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_BBANDS_Update(st, g_close[it & BENCH_MASK], &v0, &v1, &v2);
                    acc += v0;
                    acc += v1;
                    acc += v2;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_BBANDS_Peek(st, g_close[it & BENCH_MASK], &v0, &v1, &v2);
                        acc += v0;
                        acc += v1;
                        acc += v2;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_BBANDS_Update(st, g_close[it & BENCH_MASK], &v0, &v1, &v2);
                        acc += v0;
                        acc += v1;
                        acc += v2;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_BBANDS_Close(st);
            bench_stream_row("BBANDS", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_BBANDS_Close(st); }
            bench_stream_row("BBANDS", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "BBW") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        const int optInTimePeriod = bench_opaque_int(g_period > 0 ? g_period : 20);
        const double optInNbDevUp = bench_opaque_double(2.000000000000000);
        const double optInNbDevDn = bench_opaque_double(2.000000000000000);
        const int optInMAType = bench_opaque_int(0);
        int lb = TA_BBW_Lookback(optInTimePeriod, optInNbDevUp, optInNbDevDn, optInMAType);
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_BBW(t, t, g_rt_close, optInTimePeriod, optInNbDevUp, optInNbDevDn, optInMAType, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_BBW_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_BBW_Open(&st, g_close, g_nPoints, optInTimePeriod, optInNbDevUp, optInNbDevDn, optInMAType, &v0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_BBW_Update(st, g_close[it & BENCH_MASK], &v0);
                    acc += v0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_BBW_Peek(st, g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_BBW_Update(st, g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_BBW_Close(st);
            bench_stream_row("BBW", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_BBW_Close(st); }
            bench_stream_row("BBW", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "BETA") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        const int optInTimePeriod = bench_opaque_int(g_period > 0 ? g_period : 5);
        int lb = TA_BETA_Lookback(optInTimePeriod);
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_close[t] = g_close[it & BENCH_MASK];
                g_rt_high[t] = g_high[it & BENCH_MASK];
                TA_BETA(t, t, g_rt_close, g_rt_high, optInTimePeriod, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_BETA_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_BETA_Open(&st, g_close, g_high, g_nPoints, optInTimePeriod, &v0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_BETA_Update(st, g_close[it & BENCH_MASK], g_high[it & BENCH_MASK], &v0);
                    acc += v0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_BETA_Peek(st, g_close[it & BENCH_MASK], g_high[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_BETA_Update(st, g_close[it & BENCH_MASK], g_high[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_BETA_Close(st);
            bench_stream_row("BETA", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_BETA_Close(st); }
            bench_stream_row("BETA", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "BOP") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_BOP_Lookback();
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_open[t] = g_open[it & BENCH_MASK];
                g_rt_high[t] = g_high[it & BENCH_MASK];
                g_rt_low[t] = g_low[it & BENCH_MASK];
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_BOP(t, t, g_rt_open, g_rt_high, g_rt_low, g_rt_close, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_BOP_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_BOP_Open(&st, g_open, g_high, g_low, g_close, g_nPoints, &v0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_BOP_Update(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &v0);
                    acc += v0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_BOP_Peek(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_BOP_Update(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_BOP_Close(st);
            bench_stream_row("BOP", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_BOP_Close(st); }
            bench_stream_row("BOP", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CCI") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        const int optInTimePeriod = bench_opaque_int(g_period > 0 ? g_period : 14);
        int lb = TA_CCI_Lookback(optInTimePeriod);
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_high[t] = g_high[it & BENCH_MASK];
                g_rt_low[t] = g_low[it & BENCH_MASK];
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_CCI(t, t, g_rt_high, g_rt_low, g_rt_close, optInTimePeriod, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_CCI_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CCI_Open(&st, g_high, g_low, g_close, g_nPoints, optInTimePeriod, &v0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_CCI_Update(st, g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &v0);
                    acc += v0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_CCI_Peek(st, g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_CCI_Update(st, g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_CCI_Close(st);
            bench_stream_row("CCI", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CCI_Close(st); }
            bench_stream_row("CCI", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDL2CROWS") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_CDL2CROWS_Lookback();
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_open[t] = g_open[it & BENCH_MASK];
                g_rt_high[t] = g_high[it & BENCH_MASK];
                g_rt_low[t] = g_low[it & BENCH_MASK];
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_CDL2CROWS(t, t, g_rt_open, g_rt_high, g_rt_low, g_rt_close, &begIdx, &nb, g_outIntBuf0);
                acc += (double)g_outIntBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_CDL2CROWS_Stream *st = NULL;
            int iv0 = 0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CDL2CROWS_Open(&st, g_open, g_high, g_low, g_close, g_nPoints, &iv0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_CDL2CROWS_Update(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                    acc += (double)iv0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_CDL2CROWS_Peek(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                        acc += (double)iv0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_CDL2CROWS_Update(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                        acc += (double)iv0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_CDL2CROWS_Close(st);
            bench_stream_row("CDL2CROWS", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDL2CROWS_Close(st); }
            bench_stream_row("CDL2CROWS", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDL3BLACKCROWS") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_CDL3BLACKCROWS_Lookback();
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_open[t] = g_open[it & BENCH_MASK];
                g_rt_high[t] = g_high[it & BENCH_MASK];
                g_rt_low[t] = g_low[it & BENCH_MASK];
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_CDL3BLACKCROWS(t, t, g_rt_open, g_rt_high, g_rt_low, g_rt_close, &begIdx, &nb, g_outIntBuf0);
                acc += (double)g_outIntBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_CDL3BLACKCROWS_Stream *st = NULL;
            int iv0 = 0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CDL3BLACKCROWS_Open(&st, g_open, g_high, g_low, g_close, g_nPoints, &iv0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_CDL3BLACKCROWS_Update(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                    acc += (double)iv0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_CDL3BLACKCROWS_Peek(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                        acc += (double)iv0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_CDL3BLACKCROWS_Update(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                        acc += (double)iv0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_CDL3BLACKCROWS_Close(st);
            bench_stream_row("CDL3BLACKCROWS", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDL3BLACKCROWS_Close(st); }
            bench_stream_row("CDL3BLACKCROWS", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDL3INSIDE") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_CDL3INSIDE_Lookback();
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_open[t] = g_open[it & BENCH_MASK];
                g_rt_high[t] = g_high[it & BENCH_MASK];
                g_rt_low[t] = g_low[it & BENCH_MASK];
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_CDL3INSIDE(t, t, g_rt_open, g_rt_high, g_rt_low, g_rt_close, &begIdx, &nb, g_outIntBuf0);
                acc += (double)g_outIntBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_CDL3INSIDE_Stream *st = NULL;
            int iv0 = 0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CDL3INSIDE_Open(&st, g_open, g_high, g_low, g_close, g_nPoints, &iv0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_CDL3INSIDE_Update(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                    acc += (double)iv0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_CDL3INSIDE_Peek(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                        acc += (double)iv0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_CDL3INSIDE_Update(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                        acc += (double)iv0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_CDL3INSIDE_Close(st);
            bench_stream_row("CDL3INSIDE", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDL3INSIDE_Close(st); }
            bench_stream_row("CDL3INSIDE", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDL3LINESTRIKE") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_CDL3LINESTRIKE_Lookback();
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_open[t] = g_open[it & BENCH_MASK];
                g_rt_high[t] = g_high[it & BENCH_MASK];
                g_rt_low[t] = g_low[it & BENCH_MASK];
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_CDL3LINESTRIKE(t, t, g_rt_open, g_rt_high, g_rt_low, g_rt_close, &begIdx, &nb, g_outIntBuf0);
                acc += (double)g_outIntBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_CDL3LINESTRIKE_Stream *st = NULL;
            int iv0 = 0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CDL3LINESTRIKE_Open(&st, g_open, g_high, g_low, g_close, g_nPoints, &iv0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_CDL3LINESTRIKE_Update(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                    acc += (double)iv0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_CDL3LINESTRIKE_Peek(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                        acc += (double)iv0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_CDL3LINESTRIKE_Update(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                        acc += (double)iv0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_CDL3LINESTRIKE_Close(st);
            bench_stream_row("CDL3LINESTRIKE", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDL3LINESTRIKE_Close(st); }
            bench_stream_row("CDL3LINESTRIKE", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDL3OUTSIDE") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_CDL3OUTSIDE_Lookback();
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_open[t] = g_open[it & BENCH_MASK];
                g_rt_high[t] = g_high[it & BENCH_MASK];
                g_rt_low[t] = g_low[it & BENCH_MASK];
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_CDL3OUTSIDE(t, t, g_rt_open, g_rt_high, g_rt_low, g_rt_close, &begIdx, &nb, g_outIntBuf0);
                acc += (double)g_outIntBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_CDL3OUTSIDE_Stream *st = NULL;
            int iv0 = 0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CDL3OUTSIDE_Open(&st, g_open, g_high, g_low, g_close, g_nPoints, &iv0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_CDL3OUTSIDE_Update(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                    acc += (double)iv0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_CDL3OUTSIDE_Peek(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                        acc += (double)iv0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_CDL3OUTSIDE_Update(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                        acc += (double)iv0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_CDL3OUTSIDE_Close(st);
            bench_stream_row("CDL3OUTSIDE", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDL3OUTSIDE_Close(st); }
            bench_stream_row("CDL3OUTSIDE", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDL3STARSINSOUTH") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_CDL3STARSINSOUTH_Lookback();
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_open[t] = g_open[it & BENCH_MASK];
                g_rt_high[t] = g_high[it & BENCH_MASK];
                g_rt_low[t] = g_low[it & BENCH_MASK];
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_CDL3STARSINSOUTH(t, t, g_rt_open, g_rt_high, g_rt_low, g_rt_close, &begIdx, &nb, g_outIntBuf0);
                acc += (double)g_outIntBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_CDL3STARSINSOUTH_Stream *st = NULL;
            int iv0 = 0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CDL3STARSINSOUTH_Open(&st, g_open, g_high, g_low, g_close, g_nPoints, &iv0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_CDL3STARSINSOUTH_Update(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                    acc += (double)iv0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_CDL3STARSINSOUTH_Peek(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                        acc += (double)iv0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_CDL3STARSINSOUTH_Update(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                        acc += (double)iv0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_CDL3STARSINSOUTH_Close(st);
            bench_stream_row("CDL3STARSINSOUTH", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDL3STARSINSOUTH_Close(st); }
            bench_stream_row("CDL3STARSINSOUTH", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDL3WHITESOLDIERS") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_CDL3WHITESOLDIERS_Lookback();
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_open[t] = g_open[it & BENCH_MASK];
                g_rt_high[t] = g_high[it & BENCH_MASK];
                g_rt_low[t] = g_low[it & BENCH_MASK];
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_CDL3WHITESOLDIERS(t, t, g_rt_open, g_rt_high, g_rt_low, g_rt_close, &begIdx, &nb, g_outIntBuf0);
                acc += (double)g_outIntBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_CDL3WHITESOLDIERS_Stream *st = NULL;
            int iv0 = 0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CDL3WHITESOLDIERS_Open(&st, g_open, g_high, g_low, g_close, g_nPoints, &iv0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_CDL3WHITESOLDIERS_Update(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                    acc += (double)iv0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_CDL3WHITESOLDIERS_Peek(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                        acc += (double)iv0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_CDL3WHITESOLDIERS_Update(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                        acc += (double)iv0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_CDL3WHITESOLDIERS_Close(st);
            bench_stream_row("CDL3WHITESOLDIERS", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDL3WHITESOLDIERS_Close(st); }
            bench_stream_row("CDL3WHITESOLDIERS", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLABANDONEDBABY") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        const double optInPenetration = bench_opaque_double(0.300000000000000);
        int lb = TA_CDLABANDONEDBABY_Lookback(optInPenetration);
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_open[t] = g_open[it & BENCH_MASK];
                g_rt_high[t] = g_high[it & BENCH_MASK];
                g_rt_low[t] = g_low[it & BENCH_MASK];
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_CDLABANDONEDBABY(t, t, g_rt_open, g_rt_high, g_rt_low, g_rt_close, optInPenetration, &begIdx, &nb, g_outIntBuf0);
                acc += (double)g_outIntBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_CDLABANDONEDBABY_Stream *st = NULL;
            int iv0 = 0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CDLABANDONEDBABY_Open(&st, g_open, g_high, g_low, g_close, g_nPoints, optInPenetration, &iv0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_CDLABANDONEDBABY_Update(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                    acc += (double)iv0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_CDLABANDONEDBABY_Peek(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                        acc += (double)iv0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_CDLABANDONEDBABY_Update(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                        acc += (double)iv0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_CDLABANDONEDBABY_Close(st);
            bench_stream_row("CDLABANDONEDBABY", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLABANDONEDBABY_Close(st); }
            bench_stream_row("CDLABANDONEDBABY", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLADVANCEBLOCK") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_CDLADVANCEBLOCK_Lookback();
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_open[t] = g_open[it & BENCH_MASK];
                g_rt_high[t] = g_high[it & BENCH_MASK];
                g_rt_low[t] = g_low[it & BENCH_MASK];
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_CDLADVANCEBLOCK(t, t, g_rt_open, g_rt_high, g_rt_low, g_rt_close, &begIdx, &nb, g_outIntBuf0);
                acc += (double)g_outIntBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_CDLADVANCEBLOCK_Stream *st = NULL;
            int iv0 = 0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CDLADVANCEBLOCK_Open(&st, g_open, g_high, g_low, g_close, g_nPoints, &iv0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_CDLADVANCEBLOCK_Update(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                    acc += (double)iv0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_CDLADVANCEBLOCK_Peek(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                        acc += (double)iv0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_CDLADVANCEBLOCK_Update(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                        acc += (double)iv0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_CDLADVANCEBLOCK_Close(st);
            bench_stream_row("CDLADVANCEBLOCK", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLADVANCEBLOCK_Close(st); }
            bench_stream_row("CDLADVANCEBLOCK", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLBELTHOLD") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_CDLBELTHOLD_Lookback();
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_open[t] = g_open[it & BENCH_MASK];
                g_rt_high[t] = g_high[it & BENCH_MASK];
                g_rt_low[t] = g_low[it & BENCH_MASK];
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_CDLBELTHOLD(t, t, g_rt_open, g_rt_high, g_rt_low, g_rt_close, &begIdx, &nb, g_outIntBuf0);
                acc += (double)g_outIntBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_CDLBELTHOLD_Stream *st = NULL;
            int iv0 = 0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CDLBELTHOLD_Open(&st, g_open, g_high, g_low, g_close, g_nPoints, &iv0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_CDLBELTHOLD_Update(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                    acc += (double)iv0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_CDLBELTHOLD_Peek(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                        acc += (double)iv0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_CDLBELTHOLD_Update(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                        acc += (double)iv0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_CDLBELTHOLD_Close(st);
            bench_stream_row("CDLBELTHOLD", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLBELTHOLD_Close(st); }
            bench_stream_row("CDLBELTHOLD", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLBREAKAWAY") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_CDLBREAKAWAY_Lookback();
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_open[t] = g_open[it & BENCH_MASK];
                g_rt_high[t] = g_high[it & BENCH_MASK];
                g_rt_low[t] = g_low[it & BENCH_MASK];
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_CDLBREAKAWAY(t, t, g_rt_open, g_rt_high, g_rt_low, g_rt_close, &begIdx, &nb, g_outIntBuf0);
                acc += (double)g_outIntBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_CDLBREAKAWAY_Stream *st = NULL;
            int iv0 = 0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CDLBREAKAWAY_Open(&st, g_open, g_high, g_low, g_close, g_nPoints, &iv0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_CDLBREAKAWAY_Update(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                    acc += (double)iv0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_CDLBREAKAWAY_Peek(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                        acc += (double)iv0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_CDLBREAKAWAY_Update(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                        acc += (double)iv0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_CDLBREAKAWAY_Close(st);
            bench_stream_row("CDLBREAKAWAY", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLBREAKAWAY_Close(st); }
            bench_stream_row("CDLBREAKAWAY", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLCLOSINGMARUBOZU") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_CDLCLOSINGMARUBOZU_Lookback();
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_open[t] = g_open[it & BENCH_MASK];
                g_rt_high[t] = g_high[it & BENCH_MASK];
                g_rt_low[t] = g_low[it & BENCH_MASK];
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_CDLCLOSINGMARUBOZU(t, t, g_rt_open, g_rt_high, g_rt_low, g_rt_close, &begIdx, &nb, g_outIntBuf0);
                acc += (double)g_outIntBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_CDLCLOSINGMARUBOZU_Stream *st = NULL;
            int iv0 = 0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CDLCLOSINGMARUBOZU_Open(&st, g_open, g_high, g_low, g_close, g_nPoints, &iv0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_CDLCLOSINGMARUBOZU_Update(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                    acc += (double)iv0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_CDLCLOSINGMARUBOZU_Peek(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                        acc += (double)iv0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_CDLCLOSINGMARUBOZU_Update(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                        acc += (double)iv0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_CDLCLOSINGMARUBOZU_Close(st);
            bench_stream_row("CDLCLOSINGMARUBOZU", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLCLOSINGMARUBOZU_Close(st); }
            bench_stream_row("CDLCLOSINGMARUBOZU", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLCONCEALBABYSWALL") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_CDLCONCEALBABYSWALL_Lookback();
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_open[t] = g_open[it & BENCH_MASK];
                g_rt_high[t] = g_high[it & BENCH_MASK];
                g_rt_low[t] = g_low[it & BENCH_MASK];
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_CDLCONCEALBABYSWALL(t, t, g_rt_open, g_rt_high, g_rt_low, g_rt_close, &begIdx, &nb, g_outIntBuf0);
                acc += (double)g_outIntBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_CDLCONCEALBABYSWALL_Stream *st = NULL;
            int iv0 = 0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CDLCONCEALBABYSWALL_Open(&st, g_open, g_high, g_low, g_close, g_nPoints, &iv0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_CDLCONCEALBABYSWALL_Update(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                    acc += (double)iv0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_CDLCONCEALBABYSWALL_Peek(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                        acc += (double)iv0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_CDLCONCEALBABYSWALL_Update(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                        acc += (double)iv0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_CDLCONCEALBABYSWALL_Close(st);
            bench_stream_row("CDLCONCEALBABYSWALL", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLCONCEALBABYSWALL_Close(st); }
            bench_stream_row("CDLCONCEALBABYSWALL", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLCOUNTERATTACK") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_CDLCOUNTERATTACK_Lookback();
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_open[t] = g_open[it & BENCH_MASK];
                g_rt_high[t] = g_high[it & BENCH_MASK];
                g_rt_low[t] = g_low[it & BENCH_MASK];
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_CDLCOUNTERATTACK(t, t, g_rt_open, g_rt_high, g_rt_low, g_rt_close, &begIdx, &nb, g_outIntBuf0);
                acc += (double)g_outIntBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_CDLCOUNTERATTACK_Stream *st = NULL;
            int iv0 = 0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CDLCOUNTERATTACK_Open(&st, g_open, g_high, g_low, g_close, g_nPoints, &iv0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_CDLCOUNTERATTACK_Update(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                    acc += (double)iv0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_CDLCOUNTERATTACK_Peek(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                        acc += (double)iv0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_CDLCOUNTERATTACK_Update(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                        acc += (double)iv0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_CDLCOUNTERATTACK_Close(st);
            bench_stream_row("CDLCOUNTERATTACK", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLCOUNTERATTACK_Close(st); }
            bench_stream_row("CDLCOUNTERATTACK", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLDARKCLOUDCOVER") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        const double optInPenetration = bench_opaque_double(0.500000000000000);
        int lb = TA_CDLDARKCLOUDCOVER_Lookback(optInPenetration);
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_open[t] = g_open[it & BENCH_MASK];
                g_rt_high[t] = g_high[it & BENCH_MASK];
                g_rt_low[t] = g_low[it & BENCH_MASK];
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_CDLDARKCLOUDCOVER(t, t, g_rt_open, g_rt_high, g_rt_low, g_rt_close, optInPenetration, &begIdx, &nb, g_outIntBuf0);
                acc += (double)g_outIntBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_CDLDARKCLOUDCOVER_Stream *st = NULL;
            int iv0 = 0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CDLDARKCLOUDCOVER_Open(&st, g_open, g_high, g_low, g_close, g_nPoints, optInPenetration, &iv0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_CDLDARKCLOUDCOVER_Update(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                    acc += (double)iv0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_CDLDARKCLOUDCOVER_Peek(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                        acc += (double)iv0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_CDLDARKCLOUDCOVER_Update(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                        acc += (double)iv0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_CDLDARKCLOUDCOVER_Close(st);
            bench_stream_row("CDLDARKCLOUDCOVER", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLDARKCLOUDCOVER_Close(st); }
            bench_stream_row("CDLDARKCLOUDCOVER", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLDOJI") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_CDLDOJI_Lookback();
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_open[t] = g_open[it & BENCH_MASK];
                g_rt_high[t] = g_high[it & BENCH_MASK];
                g_rt_low[t] = g_low[it & BENCH_MASK];
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_CDLDOJI(t, t, g_rt_open, g_rt_high, g_rt_low, g_rt_close, &begIdx, &nb, g_outIntBuf0);
                acc += (double)g_outIntBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_CDLDOJI_Stream *st = NULL;
            int iv0 = 0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CDLDOJI_Open(&st, g_open, g_high, g_low, g_close, g_nPoints, &iv0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_CDLDOJI_Update(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                    acc += (double)iv0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_CDLDOJI_Peek(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                        acc += (double)iv0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_CDLDOJI_Update(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                        acc += (double)iv0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_CDLDOJI_Close(st);
            bench_stream_row("CDLDOJI", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLDOJI_Close(st); }
            bench_stream_row("CDLDOJI", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLDOJISTAR") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_CDLDOJISTAR_Lookback();
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_open[t] = g_open[it & BENCH_MASK];
                g_rt_high[t] = g_high[it & BENCH_MASK];
                g_rt_low[t] = g_low[it & BENCH_MASK];
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_CDLDOJISTAR(t, t, g_rt_open, g_rt_high, g_rt_low, g_rt_close, &begIdx, &nb, g_outIntBuf0);
                acc += (double)g_outIntBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_CDLDOJISTAR_Stream *st = NULL;
            int iv0 = 0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CDLDOJISTAR_Open(&st, g_open, g_high, g_low, g_close, g_nPoints, &iv0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_CDLDOJISTAR_Update(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                    acc += (double)iv0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_CDLDOJISTAR_Peek(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                        acc += (double)iv0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_CDLDOJISTAR_Update(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                        acc += (double)iv0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_CDLDOJISTAR_Close(st);
            bench_stream_row("CDLDOJISTAR", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLDOJISTAR_Close(st); }
            bench_stream_row("CDLDOJISTAR", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLDRAGONFLYDOJI") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_CDLDRAGONFLYDOJI_Lookback();
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_open[t] = g_open[it & BENCH_MASK];
                g_rt_high[t] = g_high[it & BENCH_MASK];
                g_rt_low[t] = g_low[it & BENCH_MASK];
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_CDLDRAGONFLYDOJI(t, t, g_rt_open, g_rt_high, g_rt_low, g_rt_close, &begIdx, &nb, g_outIntBuf0);
                acc += (double)g_outIntBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_CDLDRAGONFLYDOJI_Stream *st = NULL;
            int iv0 = 0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CDLDRAGONFLYDOJI_Open(&st, g_open, g_high, g_low, g_close, g_nPoints, &iv0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_CDLDRAGONFLYDOJI_Update(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                    acc += (double)iv0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_CDLDRAGONFLYDOJI_Peek(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                        acc += (double)iv0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_CDLDRAGONFLYDOJI_Update(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                        acc += (double)iv0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_CDLDRAGONFLYDOJI_Close(st);
            bench_stream_row("CDLDRAGONFLYDOJI", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLDRAGONFLYDOJI_Close(st); }
            bench_stream_row("CDLDRAGONFLYDOJI", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLENGULFING") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_CDLENGULFING_Lookback();
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_open[t] = g_open[it & BENCH_MASK];
                g_rt_high[t] = g_high[it & BENCH_MASK];
                g_rt_low[t] = g_low[it & BENCH_MASK];
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_CDLENGULFING(t, t, g_rt_open, g_rt_high, g_rt_low, g_rt_close, &begIdx, &nb, g_outIntBuf0);
                acc += (double)g_outIntBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_CDLENGULFING_Stream *st = NULL;
            int iv0 = 0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CDLENGULFING_Open(&st, g_open, g_high, g_low, g_close, g_nPoints, &iv0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_CDLENGULFING_Update(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                    acc += (double)iv0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_CDLENGULFING_Peek(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                        acc += (double)iv0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_CDLENGULFING_Update(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                        acc += (double)iv0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_CDLENGULFING_Close(st);
            bench_stream_row("CDLENGULFING", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLENGULFING_Close(st); }
            bench_stream_row("CDLENGULFING", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLEVENINGDOJISTAR") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        const double optInPenetration = bench_opaque_double(0.300000000000000);
        int lb = TA_CDLEVENINGDOJISTAR_Lookback(optInPenetration);
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_open[t] = g_open[it & BENCH_MASK];
                g_rt_high[t] = g_high[it & BENCH_MASK];
                g_rt_low[t] = g_low[it & BENCH_MASK];
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_CDLEVENINGDOJISTAR(t, t, g_rt_open, g_rt_high, g_rt_low, g_rt_close, optInPenetration, &begIdx, &nb, g_outIntBuf0);
                acc += (double)g_outIntBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_CDLEVENINGDOJISTAR_Stream *st = NULL;
            int iv0 = 0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CDLEVENINGDOJISTAR_Open(&st, g_open, g_high, g_low, g_close, g_nPoints, optInPenetration, &iv0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_CDLEVENINGDOJISTAR_Update(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                    acc += (double)iv0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_CDLEVENINGDOJISTAR_Peek(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                        acc += (double)iv0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_CDLEVENINGDOJISTAR_Update(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                        acc += (double)iv0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_CDLEVENINGDOJISTAR_Close(st);
            bench_stream_row("CDLEVENINGDOJISTAR", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLEVENINGDOJISTAR_Close(st); }
            bench_stream_row("CDLEVENINGDOJISTAR", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLEVENINGSTAR") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        const double optInPenetration = bench_opaque_double(0.300000000000000);
        int lb = TA_CDLEVENINGSTAR_Lookback(optInPenetration);
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_open[t] = g_open[it & BENCH_MASK];
                g_rt_high[t] = g_high[it & BENCH_MASK];
                g_rt_low[t] = g_low[it & BENCH_MASK];
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_CDLEVENINGSTAR(t, t, g_rt_open, g_rt_high, g_rt_low, g_rt_close, optInPenetration, &begIdx, &nb, g_outIntBuf0);
                acc += (double)g_outIntBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_CDLEVENINGSTAR_Stream *st = NULL;
            int iv0 = 0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CDLEVENINGSTAR_Open(&st, g_open, g_high, g_low, g_close, g_nPoints, optInPenetration, &iv0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_CDLEVENINGSTAR_Update(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                    acc += (double)iv0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_CDLEVENINGSTAR_Peek(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                        acc += (double)iv0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_CDLEVENINGSTAR_Update(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                        acc += (double)iv0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_CDLEVENINGSTAR_Close(st);
            bench_stream_row("CDLEVENINGSTAR", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLEVENINGSTAR_Close(st); }
            bench_stream_row("CDLEVENINGSTAR", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLGAPSIDESIDEWHITE") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_CDLGAPSIDESIDEWHITE_Lookback();
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_open[t] = g_open[it & BENCH_MASK];
                g_rt_high[t] = g_high[it & BENCH_MASK];
                g_rt_low[t] = g_low[it & BENCH_MASK];
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_CDLGAPSIDESIDEWHITE(t, t, g_rt_open, g_rt_high, g_rt_low, g_rt_close, &begIdx, &nb, g_outIntBuf0);
                acc += (double)g_outIntBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_CDLGAPSIDESIDEWHITE_Stream *st = NULL;
            int iv0 = 0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CDLGAPSIDESIDEWHITE_Open(&st, g_open, g_high, g_low, g_close, g_nPoints, &iv0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_CDLGAPSIDESIDEWHITE_Update(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                    acc += (double)iv0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_CDLGAPSIDESIDEWHITE_Peek(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                        acc += (double)iv0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_CDLGAPSIDESIDEWHITE_Update(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                        acc += (double)iv0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_CDLGAPSIDESIDEWHITE_Close(st);
            bench_stream_row("CDLGAPSIDESIDEWHITE", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLGAPSIDESIDEWHITE_Close(st); }
            bench_stream_row("CDLGAPSIDESIDEWHITE", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLGRAVESTONEDOJI") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_CDLGRAVESTONEDOJI_Lookback();
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_open[t] = g_open[it & BENCH_MASK];
                g_rt_high[t] = g_high[it & BENCH_MASK];
                g_rt_low[t] = g_low[it & BENCH_MASK];
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_CDLGRAVESTONEDOJI(t, t, g_rt_open, g_rt_high, g_rt_low, g_rt_close, &begIdx, &nb, g_outIntBuf0);
                acc += (double)g_outIntBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_CDLGRAVESTONEDOJI_Stream *st = NULL;
            int iv0 = 0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CDLGRAVESTONEDOJI_Open(&st, g_open, g_high, g_low, g_close, g_nPoints, &iv0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_CDLGRAVESTONEDOJI_Update(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                    acc += (double)iv0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_CDLGRAVESTONEDOJI_Peek(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                        acc += (double)iv0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_CDLGRAVESTONEDOJI_Update(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                        acc += (double)iv0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_CDLGRAVESTONEDOJI_Close(st);
            bench_stream_row("CDLGRAVESTONEDOJI", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLGRAVESTONEDOJI_Close(st); }
            bench_stream_row("CDLGRAVESTONEDOJI", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLHAMMER") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_CDLHAMMER_Lookback();
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_open[t] = g_open[it & BENCH_MASK];
                g_rt_high[t] = g_high[it & BENCH_MASK];
                g_rt_low[t] = g_low[it & BENCH_MASK];
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_CDLHAMMER(t, t, g_rt_open, g_rt_high, g_rt_low, g_rt_close, &begIdx, &nb, g_outIntBuf0);
                acc += (double)g_outIntBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_CDLHAMMER_Stream *st = NULL;
            int iv0 = 0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CDLHAMMER_Open(&st, g_open, g_high, g_low, g_close, g_nPoints, &iv0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_CDLHAMMER_Update(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                    acc += (double)iv0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_CDLHAMMER_Peek(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                        acc += (double)iv0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_CDLHAMMER_Update(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                        acc += (double)iv0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_CDLHAMMER_Close(st);
            bench_stream_row("CDLHAMMER", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLHAMMER_Close(st); }
            bench_stream_row("CDLHAMMER", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLHANGINGMAN") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_CDLHANGINGMAN_Lookback();
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_open[t] = g_open[it & BENCH_MASK];
                g_rt_high[t] = g_high[it & BENCH_MASK];
                g_rt_low[t] = g_low[it & BENCH_MASK];
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_CDLHANGINGMAN(t, t, g_rt_open, g_rt_high, g_rt_low, g_rt_close, &begIdx, &nb, g_outIntBuf0);
                acc += (double)g_outIntBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_CDLHANGINGMAN_Stream *st = NULL;
            int iv0 = 0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CDLHANGINGMAN_Open(&st, g_open, g_high, g_low, g_close, g_nPoints, &iv0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_CDLHANGINGMAN_Update(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                    acc += (double)iv0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_CDLHANGINGMAN_Peek(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                        acc += (double)iv0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_CDLHANGINGMAN_Update(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                        acc += (double)iv0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_CDLHANGINGMAN_Close(st);
            bench_stream_row("CDLHANGINGMAN", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLHANGINGMAN_Close(st); }
            bench_stream_row("CDLHANGINGMAN", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLHARAMI") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_CDLHARAMI_Lookback();
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_open[t] = g_open[it & BENCH_MASK];
                g_rt_high[t] = g_high[it & BENCH_MASK];
                g_rt_low[t] = g_low[it & BENCH_MASK];
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_CDLHARAMI(t, t, g_rt_open, g_rt_high, g_rt_low, g_rt_close, &begIdx, &nb, g_outIntBuf0);
                acc += (double)g_outIntBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_CDLHARAMI_Stream *st = NULL;
            int iv0 = 0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CDLHARAMI_Open(&st, g_open, g_high, g_low, g_close, g_nPoints, &iv0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_CDLHARAMI_Update(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                    acc += (double)iv0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_CDLHARAMI_Peek(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                        acc += (double)iv0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_CDLHARAMI_Update(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                        acc += (double)iv0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_CDLHARAMI_Close(st);
            bench_stream_row("CDLHARAMI", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLHARAMI_Close(st); }
            bench_stream_row("CDLHARAMI", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLHARAMICROSS") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_CDLHARAMICROSS_Lookback();
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_open[t] = g_open[it & BENCH_MASK];
                g_rt_high[t] = g_high[it & BENCH_MASK];
                g_rt_low[t] = g_low[it & BENCH_MASK];
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_CDLHARAMICROSS(t, t, g_rt_open, g_rt_high, g_rt_low, g_rt_close, &begIdx, &nb, g_outIntBuf0);
                acc += (double)g_outIntBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_CDLHARAMICROSS_Stream *st = NULL;
            int iv0 = 0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CDLHARAMICROSS_Open(&st, g_open, g_high, g_low, g_close, g_nPoints, &iv0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_CDLHARAMICROSS_Update(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                    acc += (double)iv0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_CDLHARAMICROSS_Peek(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                        acc += (double)iv0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_CDLHARAMICROSS_Update(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                        acc += (double)iv0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_CDLHARAMICROSS_Close(st);
            bench_stream_row("CDLHARAMICROSS", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLHARAMICROSS_Close(st); }
            bench_stream_row("CDLHARAMICROSS", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLHIGHWAVE") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_CDLHIGHWAVE_Lookback();
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_open[t] = g_open[it & BENCH_MASK];
                g_rt_high[t] = g_high[it & BENCH_MASK];
                g_rt_low[t] = g_low[it & BENCH_MASK];
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_CDLHIGHWAVE(t, t, g_rt_open, g_rt_high, g_rt_low, g_rt_close, &begIdx, &nb, g_outIntBuf0);
                acc += (double)g_outIntBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_CDLHIGHWAVE_Stream *st = NULL;
            int iv0 = 0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CDLHIGHWAVE_Open(&st, g_open, g_high, g_low, g_close, g_nPoints, &iv0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_CDLHIGHWAVE_Update(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                    acc += (double)iv0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_CDLHIGHWAVE_Peek(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                        acc += (double)iv0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_CDLHIGHWAVE_Update(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                        acc += (double)iv0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_CDLHIGHWAVE_Close(st);
            bench_stream_row("CDLHIGHWAVE", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLHIGHWAVE_Close(st); }
            bench_stream_row("CDLHIGHWAVE", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLHIKKAKE") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_CDLHIKKAKE_Lookback();
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_open[t] = g_open[it & BENCH_MASK];
                g_rt_high[t] = g_high[it & BENCH_MASK];
                g_rt_low[t] = g_low[it & BENCH_MASK];
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_CDLHIKKAKE(t, t, g_rt_open, g_rt_high, g_rt_low, g_rt_close, &begIdx, &nb, g_outIntBuf0);
                acc += (double)g_outIntBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_CDLHIKKAKE_Stream *st = NULL;
            int iv0 = 0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CDLHIKKAKE_Open(&st, g_open, g_high, g_low, g_close, g_nPoints, &iv0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_CDLHIKKAKE_Update(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                    acc += (double)iv0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_CDLHIKKAKE_Peek(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                        acc += (double)iv0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_CDLHIKKAKE_Update(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                        acc += (double)iv0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_CDLHIKKAKE_Close(st);
            bench_stream_row("CDLHIKKAKE", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLHIKKAKE_Close(st); }
            bench_stream_row("CDLHIKKAKE", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLHIKKAKEMOD") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_CDLHIKKAKEMOD_Lookback();
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_open[t] = g_open[it & BENCH_MASK];
                g_rt_high[t] = g_high[it & BENCH_MASK];
                g_rt_low[t] = g_low[it & BENCH_MASK];
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_CDLHIKKAKEMOD(t, t, g_rt_open, g_rt_high, g_rt_low, g_rt_close, &begIdx, &nb, g_outIntBuf0);
                acc += (double)g_outIntBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_CDLHIKKAKEMOD_Stream *st = NULL;
            int iv0 = 0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CDLHIKKAKEMOD_Open(&st, g_open, g_high, g_low, g_close, g_nPoints, &iv0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_CDLHIKKAKEMOD_Update(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                    acc += (double)iv0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_CDLHIKKAKEMOD_Peek(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                        acc += (double)iv0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_CDLHIKKAKEMOD_Update(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                        acc += (double)iv0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_CDLHIKKAKEMOD_Close(st);
            bench_stream_row("CDLHIKKAKEMOD", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLHIKKAKEMOD_Close(st); }
            bench_stream_row("CDLHIKKAKEMOD", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLHOMINGPIGEON") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_CDLHOMINGPIGEON_Lookback();
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_open[t] = g_open[it & BENCH_MASK];
                g_rt_high[t] = g_high[it & BENCH_MASK];
                g_rt_low[t] = g_low[it & BENCH_MASK];
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_CDLHOMINGPIGEON(t, t, g_rt_open, g_rt_high, g_rt_low, g_rt_close, &begIdx, &nb, g_outIntBuf0);
                acc += (double)g_outIntBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_CDLHOMINGPIGEON_Stream *st = NULL;
            int iv0 = 0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CDLHOMINGPIGEON_Open(&st, g_open, g_high, g_low, g_close, g_nPoints, &iv0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_CDLHOMINGPIGEON_Update(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                    acc += (double)iv0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_CDLHOMINGPIGEON_Peek(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                        acc += (double)iv0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_CDLHOMINGPIGEON_Update(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                        acc += (double)iv0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_CDLHOMINGPIGEON_Close(st);
            bench_stream_row("CDLHOMINGPIGEON", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLHOMINGPIGEON_Close(st); }
            bench_stream_row("CDLHOMINGPIGEON", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLIDENTICAL3CROWS") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_CDLIDENTICAL3CROWS_Lookback();
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_open[t] = g_open[it & BENCH_MASK];
                g_rt_high[t] = g_high[it & BENCH_MASK];
                g_rt_low[t] = g_low[it & BENCH_MASK];
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_CDLIDENTICAL3CROWS(t, t, g_rt_open, g_rt_high, g_rt_low, g_rt_close, &begIdx, &nb, g_outIntBuf0);
                acc += (double)g_outIntBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_CDLIDENTICAL3CROWS_Stream *st = NULL;
            int iv0 = 0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CDLIDENTICAL3CROWS_Open(&st, g_open, g_high, g_low, g_close, g_nPoints, &iv0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_CDLIDENTICAL3CROWS_Update(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                    acc += (double)iv0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_CDLIDENTICAL3CROWS_Peek(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                        acc += (double)iv0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_CDLIDENTICAL3CROWS_Update(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                        acc += (double)iv0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_CDLIDENTICAL3CROWS_Close(st);
            bench_stream_row("CDLIDENTICAL3CROWS", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLIDENTICAL3CROWS_Close(st); }
            bench_stream_row("CDLIDENTICAL3CROWS", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLINNECK") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_CDLINNECK_Lookback();
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_open[t] = g_open[it & BENCH_MASK];
                g_rt_high[t] = g_high[it & BENCH_MASK];
                g_rt_low[t] = g_low[it & BENCH_MASK];
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_CDLINNECK(t, t, g_rt_open, g_rt_high, g_rt_low, g_rt_close, &begIdx, &nb, g_outIntBuf0);
                acc += (double)g_outIntBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_CDLINNECK_Stream *st = NULL;
            int iv0 = 0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CDLINNECK_Open(&st, g_open, g_high, g_low, g_close, g_nPoints, &iv0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_CDLINNECK_Update(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                    acc += (double)iv0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_CDLINNECK_Peek(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                        acc += (double)iv0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_CDLINNECK_Update(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                        acc += (double)iv0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_CDLINNECK_Close(st);
            bench_stream_row("CDLINNECK", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLINNECK_Close(st); }
            bench_stream_row("CDLINNECK", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLINVERTEDHAMMER") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_CDLINVERTEDHAMMER_Lookback();
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_open[t] = g_open[it & BENCH_MASK];
                g_rt_high[t] = g_high[it & BENCH_MASK];
                g_rt_low[t] = g_low[it & BENCH_MASK];
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_CDLINVERTEDHAMMER(t, t, g_rt_open, g_rt_high, g_rt_low, g_rt_close, &begIdx, &nb, g_outIntBuf0);
                acc += (double)g_outIntBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_CDLINVERTEDHAMMER_Stream *st = NULL;
            int iv0 = 0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CDLINVERTEDHAMMER_Open(&st, g_open, g_high, g_low, g_close, g_nPoints, &iv0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_CDLINVERTEDHAMMER_Update(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                    acc += (double)iv0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_CDLINVERTEDHAMMER_Peek(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                        acc += (double)iv0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_CDLINVERTEDHAMMER_Update(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                        acc += (double)iv0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_CDLINVERTEDHAMMER_Close(st);
            bench_stream_row("CDLINVERTEDHAMMER", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLINVERTEDHAMMER_Close(st); }
            bench_stream_row("CDLINVERTEDHAMMER", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLKICKING") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_CDLKICKING_Lookback();
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_open[t] = g_open[it & BENCH_MASK];
                g_rt_high[t] = g_high[it & BENCH_MASK];
                g_rt_low[t] = g_low[it & BENCH_MASK];
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_CDLKICKING(t, t, g_rt_open, g_rt_high, g_rt_low, g_rt_close, &begIdx, &nb, g_outIntBuf0);
                acc += (double)g_outIntBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_CDLKICKING_Stream *st = NULL;
            int iv0 = 0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CDLKICKING_Open(&st, g_open, g_high, g_low, g_close, g_nPoints, &iv0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_CDLKICKING_Update(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                    acc += (double)iv0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_CDLKICKING_Peek(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                        acc += (double)iv0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_CDLKICKING_Update(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                        acc += (double)iv0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_CDLKICKING_Close(st);
            bench_stream_row("CDLKICKING", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLKICKING_Close(st); }
            bench_stream_row("CDLKICKING", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLKICKINGBYLENGTH") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_CDLKICKINGBYLENGTH_Lookback();
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_open[t] = g_open[it & BENCH_MASK];
                g_rt_high[t] = g_high[it & BENCH_MASK];
                g_rt_low[t] = g_low[it & BENCH_MASK];
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_CDLKICKINGBYLENGTH(t, t, g_rt_open, g_rt_high, g_rt_low, g_rt_close, &begIdx, &nb, g_outIntBuf0);
                acc += (double)g_outIntBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_CDLKICKINGBYLENGTH_Stream *st = NULL;
            int iv0 = 0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CDLKICKINGBYLENGTH_Open(&st, g_open, g_high, g_low, g_close, g_nPoints, &iv0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_CDLKICKINGBYLENGTH_Update(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                    acc += (double)iv0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_CDLKICKINGBYLENGTH_Peek(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                        acc += (double)iv0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_CDLKICKINGBYLENGTH_Update(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                        acc += (double)iv0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_CDLKICKINGBYLENGTH_Close(st);
            bench_stream_row("CDLKICKINGBYLENGTH", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLKICKINGBYLENGTH_Close(st); }
            bench_stream_row("CDLKICKINGBYLENGTH", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLLADDERBOTTOM") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_CDLLADDERBOTTOM_Lookback();
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_open[t] = g_open[it & BENCH_MASK];
                g_rt_high[t] = g_high[it & BENCH_MASK];
                g_rt_low[t] = g_low[it & BENCH_MASK];
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_CDLLADDERBOTTOM(t, t, g_rt_open, g_rt_high, g_rt_low, g_rt_close, &begIdx, &nb, g_outIntBuf0);
                acc += (double)g_outIntBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_CDLLADDERBOTTOM_Stream *st = NULL;
            int iv0 = 0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CDLLADDERBOTTOM_Open(&st, g_open, g_high, g_low, g_close, g_nPoints, &iv0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_CDLLADDERBOTTOM_Update(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                    acc += (double)iv0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_CDLLADDERBOTTOM_Peek(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                        acc += (double)iv0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_CDLLADDERBOTTOM_Update(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                        acc += (double)iv0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_CDLLADDERBOTTOM_Close(st);
            bench_stream_row("CDLLADDERBOTTOM", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLLADDERBOTTOM_Close(st); }
            bench_stream_row("CDLLADDERBOTTOM", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLLONGLEGGEDDOJI") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_CDLLONGLEGGEDDOJI_Lookback();
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_open[t] = g_open[it & BENCH_MASK];
                g_rt_high[t] = g_high[it & BENCH_MASK];
                g_rt_low[t] = g_low[it & BENCH_MASK];
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_CDLLONGLEGGEDDOJI(t, t, g_rt_open, g_rt_high, g_rt_low, g_rt_close, &begIdx, &nb, g_outIntBuf0);
                acc += (double)g_outIntBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_CDLLONGLEGGEDDOJI_Stream *st = NULL;
            int iv0 = 0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CDLLONGLEGGEDDOJI_Open(&st, g_open, g_high, g_low, g_close, g_nPoints, &iv0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_CDLLONGLEGGEDDOJI_Update(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                    acc += (double)iv0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_CDLLONGLEGGEDDOJI_Peek(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                        acc += (double)iv0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_CDLLONGLEGGEDDOJI_Update(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                        acc += (double)iv0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_CDLLONGLEGGEDDOJI_Close(st);
            bench_stream_row("CDLLONGLEGGEDDOJI", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLLONGLEGGEDDOJI_Close(st); }
            bench_stream_row("CDLLONGLEGGEDDOJI", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLLONGLINE") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_CDLLONGLINE_Lookback();
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_open[t] = g_open[it & BENCH_MASK];
                g_rt_high[t] = g_high[it & BENCH_MASK];
                g_rt_low[t] = g_low[it & BENCH_MASK];
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_CDLLONGLINE(t, t, g_rt_open, g_rt_high, g_rt_low, g_rt_close, &begIdx, &nb, g_outIntBuf0);
                acc += (double)g_outIntBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_CDLLONGLINE_Stream *st = NULL;
            int iv0 = 0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CDLLONGLINE_Open(&st, g_open, g_high, g_low, g_close, g_nPoints, &iv0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_CDLLONGLINE_Update(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                    acc += (double)iv0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_CDLLONGLINE_Peek(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                        acc += (double)iv0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_CDLLONGLINE_Update(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                        acc += (double)iv0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_CDLLONGLINE_Close(st);
            bench_stream_row("CDLLONGLINE", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLLONGLINE_Close(st); }
            bench_stream_row("CDLLONGLINE", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLMARUBOZU") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_CDLMARUBOZU_Lookback();
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_open[t] = g_open[it & BENCH_MASK];
                g_rt_high[t] = g_high[it & BENCH_MASK];
                g_rt_low[t] = g_low[it & BENCH_MASK];
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_CDLMARUBOZU(t, t, g_rt_open, g_rt_high, g_rt_low, g_rt_close, &begIdx, &nb, g_outIntBuf0);
                acc += (double)g_outIntBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_CDLMARUBOZU_Stream *st = NULL;
            int iv0 = 0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CDLMARUBOZU_Open(&st, g_open, g_high, g_low, g_close, g_nPoints, &iv0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_CDLMARUBOZU_Update(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                    acc += (double)iv0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_CDLMARUBOZU_Peek(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                        acc += (double)iv0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_CDLMARUBOZU_Update(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                        acc += (double)iv0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_CDLMARUBOZU_Close(st);
            bench_stream_row("CDLMARUBOZU", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLMARUBOZU_Close(st); }
            bench_stream_row("CDLMARUBOZU", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLMATCHINGLOW") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_CDLMATCHINGLOW_Lookback();
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_open[t] = g_open[it & BENCH_MASK];
                g_rt_high[t] = g_high[it & BENCH_MASK];
                g_rt_low[t] = g_low[it & BENCH_MASK];
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_CDLMATCHINGLOW(t, t, g_rt_open, g_rt_high, g_rt_low, g_rt_close, &begIdx, &nb, g_outIntBuf0);
                acc += (double)g_outIntBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_CDLMATCHINGLOW_Stream *st = NULL;
            int iv0 = 0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CDLMATCHINGLOW_Open(&st, g_open, g_high, g_low, g_close, g_nPoints, &iv0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_CDLMATCHINGLOW_Update(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                    acc += (double)iv0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_CDLMATCHINGLOW_Peek(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                        acc += (double)iv0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_CDLMATCHINGLOW_Update(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                        acc += (double)iv0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_CDLMATCHINGLOW_Close(st);
            bench_stream_row("CDLMATCHINGLOW", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLMATCHINGLOW_Close(st); }
            bench_stream_row("CDLMATCHINGLOW", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLMATHOLD") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        const double optInPenetration = bench_opaque_double(0.500000000000000);
        int lb = TA_CDLMATHOLD_Lookback(optInPenetration);
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_open[t] = g_open[it & BENCH_MASK];
                g_rt_high[t] = g_high[it & BENCH_MASK];
                g_rt_low[t] = g_low[it & BENCH_MASK];
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_CDLMATHOLD(t, t, g_rt_open, g_rt_high, g_rt_low, g_rt_close, optInPenetration, &begIdx, &nb, g_outIntBuf0);
                acc += (double)g_outIntBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_CDLMATHOLD_Stream *st = NULL;
            int iv0 = 0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CDLMATHOLD_Open(&st, g_open, g_high, g_low, g_close, g_nPoints, optInPenetration, &iv0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_CDLMATHOLD_Update(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                    acc += (double)iv0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_CDLMATHOLD_Peek(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                        acc += (double)iv0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_CDLMATHOLD_Update(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                        acc += (double)iv0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_CDLMATHOLD_Close(st);
            bench_stream_row("CDLMATHOLD", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLMATHOLD_Close(st); }
            bench_stream_row("CDLMATHOLD", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLMORNINGDOJISTAR") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        const double optInPenetration = bench_opaque_double(0.300000000000000);
        int lb = TA_CDLMORNINGDOJISTAR_Lookback(optInPenetration);
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_open[t] = g_open[it & BENCH_MASK];
                g_rt_high[t] = g_high[it & BENCH_MASK];
                g_rt_low[t] = g_low[it & BENCH_MASK];
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_CDLMORNINGDOJISTAR(t, t, g_rt_open, g_rt_high, g_rt_low, g_rt_close, optInPenetration, &begIdx, &nb, g_outIntBuf0);
                acc += (double)g_outIntBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_CDLMORNINGDOJISTAR_Stream *st = NULL;
            int iv0 = 0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CDLMORNINGDOJISTAR_Open(&st, g_open, g_high, g_low, g_close, g_nPoints, optInPenetration, &iv0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_CDLMORNINGDOJISTAR_Update(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                    acc += (double)iv0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_CDLMORNINGDOJISTAR_Peek(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                        acc += (double)iv0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_CDLMORNINGDOJISTAR_Update(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                        acc += (double)iv0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_CDLMORNINGDOJISTAR_Close(st);
            bench_stream_row("CDLMORNINGDOJISTAR", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLMORNINGDOJISTAR_Close(st); }
            bench_stream_row("CDLMORNINGDOJISTAR", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLMORNINGSTAR") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        const double optInPenetration = bench_opaque_double(0.300000000000000);
        int lb = TA_CDLMORNINGSTAR_Lookback(optInPenetration);
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_open[t] = g_open[it & BENCH_MASK];
                g_rt_high[t] = g_high[it & BENCH_MASK];
                g_rt_low[t] = g_low[it & BENCH_MASK];
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_CDLMORNINGSTAR(t, t, g_rt_open, g_rt_high, g_rt_low, g_rt_close, optInPenetration, &begIdx, &nb, g_outIntBuf0);
                acc += (double)g_outIntBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_CDLMORNINGSTAR_Stream *st = NULL;
            int iv0 = 0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CDLMORNINGSTAR_Open(&st, g_open, g_high, g_low, g_close, g_nPoints, optInPenetration, &iv0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_CDLMORNINGSTAR_Update(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                    acc += (double)iv0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_CDLMORNINGSTAR_Peek(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                        acc += (double)iv0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_CDLMORNINGSTAR_Update(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                        acc += (double)iv0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_CDLMORNINGSTAR_Close(st);
            bench_stream_row("CDLMORNINGSTAR", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLMORNINGSTAR_Close(st); }
            bench_stream_row("CDLMORNINGSTAR", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLONNECK") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_CDLONNECK_Lookback();
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_open[t] = g_open[it & BENCH_MASK];
                g_rt_high[t] = g_high[it & BENCH_MASK];
                g_rt_low[t] = g_low[it & BENCH_MASK];
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_CDLONNECK(t, t, g_rt_open, g_rt_high, g_rt_low, g_rt_close, &begIdx, &nb, g_outIntBuf0);
                acc += (double)g_outIntBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_CDLONNECK_Stream *st = NULL;
            int iv0 = 0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CDLONNECK_Open(&st, g_open, g_high, g_low, g_close, g_nPoints, &iv0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_CDLONNECK_Update(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                    acc += (double)iv0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_CDLONNECK_Peek(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                        acc += (double)iv0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_CDLONNECK_Update(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                        acc += (double)iv0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_CDLONNECK_Close(st);
            bench_stream_row("CDLONNECK", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLONNECK_Close(st); }
            bench_stream_row("CDLONNECK", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLPIERCING") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_CDLPIERCING_Lookback();
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_open[t] = g_open[it & BENCH_MASK];
                g_rt_high[t] = g_high[it & BENCH_MASK];
                g_rt_low[t] = g_low[it & BENCH_MASK];
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_CDLPIERCING(t, t, g_rt_open, g_rt_high, g_rt_low, g_rt_close, &begIdx, &nb, g_outIntBuf0);
                acc += (double)g_outIntBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_CDLPIERCING_Stream *st = NULL;
            int iv0 = 0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CDLPIERCING_Open(&st, g_open, g_high, g_low, g_close, g_nPoints, &iv0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_CDLPIERCING_Update(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                    acc += (double)iv0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_CDLPIERCING_Peek(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                        acc += (double)iv0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_CDLPIERCING_Update(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                        acc += (double)iv0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_CDLPIERCING_Close(st);
            bench_stream_row("CDLPIERCING", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLPIERCING_Close(st); }
            bench_stream_row("CDLPIERCING", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLRICKSHAWMAN") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_CDLRICKSHAWMAN_Lookback();
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_open[t] = g_open[it & BENCH_MASK];
                g_rt_high[t] = g_high[it & BENCH_MASK];
                g_rt_low[t] = g_low[it & BENCH_MASK];
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_CDLRICKSHAWMAN(t, t, g_rt_open, g_rt_high, g_rt_low, g_rt_close, &begIdx, &nb, g_outIntBuf0);
                acc += (double)g_outIntBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_CDLRICKSHAWMAN_Stream *st = NULL;
            int iv0 = 0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CDLRICKSHAWMAN_Open(&st, g_open, g_high, g_low, g_close, g_nPoints, &iv0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_CDLRICKSHAWMAN_Update(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                    acc += (double)iv0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_CDLRICKSHAWMAN_Peek(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                        acc += (double)iv0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_CDLRICKSHAWMAN_Update(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                        acc += (double)iv0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_CDLRICKSHAWMAN_Close(st);
            bench_stream_row("CDLRICKSHAWMAN", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLRICKSHAWMAN_Close(st); }
            bench_stream_row("CDLRICKSHAWMAN", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLRISEFALL3METHODS") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_CDLRISEFALL3METHODS_Lookback();
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_open[t] = g_open[it & BENCH_MASK];
                g_rt_high[t] = g_high[it & BENCH_MASK];
                g_rt_low[t] = g_low[it & BENCH_MASK];
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_CDLRISEFALL3METHODS(t, t, g_rt_open, g_rt_high, g_rt_low, g_rt_close, &begIdx, &nb, g_outIntBuf0);
                acc += (double)g_outIntBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_CDLRISEFALL3METHODS_Stream *st = NULL;
            int iv0 = 0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CDLRISEFALL3METHODS_Open(&st, g_open, g_high, g_low, g_close, g_nPoints, &iv0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_CDLRISEFALL3METHODS_Update(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                    acc += (double)iv0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_CDLRISEFALL3METHODS_Peek(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                        acc += (double)iv0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_CDLRISEFALL3METHODS_Update(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                        acc += (double)iv0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_CDLRISEFALL3METHODS_Close(st);
            bench_stream_row("CDLRISEFALL3METHODS", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLRISEFALL3METHODS_Close(st); }
            bench_stream_row("CDLRISEFALL3METHODS", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLSEPARATINGLINES") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_CDLSEPARATINGLINES_Lookback();
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_open[t] = g_open[it & BENCH_MASK];
                g_rt_high[t] = g_high[it & BENCH_MASK];
                g_rt_low[t] = g_low[it & BENCH_MASK];
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_CDLSEPARATINGLINES(t, t, g_rt_open, g_rt_high, g_rt_low, g_rt_close, &begIdx, &nb, g_outIntBuf0);
                acc += (double)g_outIntBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_CDLSEPARATINGLINES_Stream *st = NULL;
            int iv0 = 0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CDLSEPARATINGLINES_Open(&st, g_open, g_high, g_low, g_close, g_nPoints, &iv0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_CDLSEPARATINGLINES_Update(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                    acc += (double)iv0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_CDLSEPARATINGLINES_Peek(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                        acc += (double)iv0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_CDLSEPARATINGLINES_Update(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                        acc += (double)iv0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_CDLSEPARATINGLINES_Close(st);
            bench_stream_row("CDLSEPARATINGLINES", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLSEPARATINGLINES_Close(st); }
            bench_stream_row("CDLSEPARATINGLINES", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLSHOOTINGSTAR") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_CDLSHOOTINGSTAR_Lookback();
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_open[t] = g_open[it & BENCH_MASK];
                g_rt_high[t] = g_high[it & BENCH_MASK];
                g_rt_low[t] = g_low[it & BENCH_MASK];
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_CDLSHOOTINGSTAR(t, t, g_rt_open, g_rt_high, g_rt_low, g_rt_close, &begIdx, &nb, g_outIntBuf0);
                acc += (double)g_outIntBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_CDLSHOOTINGSTAR_Stream *st = NULL;
            int iv0 = 0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CDLSHOOTINGSTAR_Open(&st, g_open, g_high, g_low, g_close, g_nPoints, &iv0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_CDLSHOOTINGSTAR_Update(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                    acc += (double)iv0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_CDLSHOOTINGSTAR_Peek(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                        acc += (double)iv0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_CDLSHOOTINGSTAR_Update(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                        acc += (double)iv0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_CDLSHOOTINGSTAR_Close(st);
            bench_stream_row("CDLSHOOTINGSTAR", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLSHOOTINGSTAR_Close(st); }
            bench_stream_row("CDLSHOOTINGSTAR", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLSHORTLINE") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_CDLSHORTLINE_Lookback();
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_open[t] = g_open[it & BENCH_MASK];
                g_rt_high[t] = g_high[it & BENCH_MASK];
                g_rt_low[t] = g_low[it & BENCH_MASK];
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_CDLSHORTLINE(t, t, g_rt_open, g_rt_high, g_rt_low, g_rt_close, &begIdx, &nb, g_outIntBuf0);
                acc += (double)g_outIntBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_CDLSHORTLINE_Stream *st = NULL;
            int iv0 = 0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CDLSHORTLINE_Open(&st, g_open, g_high, g_low, g_close, g_nPoints, &iv0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_CDLSHORTLINE_Update(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                    acc += (double)iv0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_CDLSHORTLINE_Peek(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                        acc += (double)iv0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_CDLSHORTLINE_Update(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                        acc += (double)iv0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_CDLSHORTLINE_Close(st);
            bench_stream_row("CDLSHORTLINE", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLSHORTLINE_Close(st); }
            bench_stream_row("CDLSHORTLINE", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLSPINNINGTOP") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_CDLSPINNINGTOP_Lookback();
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_open[t] = g_open[it & BENCH_MASK];
                g_rt_high[t] = g_high[it & BENCH_MASK];
                g_rt_low[t] = g_low[it & BENCH_MASK];
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_CDLSPINNINGTOP(t, t, g_rt_open, g_rt_high, g_rt_low, g_rt_close, &begIdx, &nb, g_outIntBuf0);
                acc += (double)g_outIntBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_CDLSPINNINGTOP_Stream *st = NULL;
            int iv0 = 0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CDLSPINNINGTOP_Open(&st, g_open, g_high, g_low, g_close, g_nPoints, &iv0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_CDLSPINNINGTOP_Update(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                    acc += (double)iv0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_CDLSPINNINGTOP_Peek(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                        acc += (double)iv0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_CDLSPINNINGTOP_Update(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                        acc += (double)iv0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_CDLSPINNINGTOP_Close(st);
            bench_stream_row("CDLSPINNINGTOP", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLSPINNINGTOP_Close(st); }
            bench_stream_row("CDLSPINNINGTOP", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLSTALLEDPATTERN") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_CDLSTALLEDPATTERN_Lookback();
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_open[t] = g_open[it & BENCH_MASK];
                g_rt_high[t] = g_high[it & BENCH_MASK];
                g_rt_low[t] = g_low[it & BENCH_MASK];
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_CDLSTALLEDPATTERN(t, t, g_rt_open, g_rt_high, g_rt_low, g_rt_close, &begIdx, &nb, g_outIntBuf0);
                acc += (double)g_outIntBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_CDLSTALLEDPATTERN_Stream *st = NULL;
            int iv0 = 0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CDLSTALLEDPATTERN_Open(&st, g_open, g_high, g_low, g_close, g_nPoints, &iv0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_CDLSTALLEDPATTERN_Update(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                    acc += (double)iv0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_CDLSTALLEDPATTERN_Peek(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                        acc += (double)iv0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_CDLSTALLEDPATTERN_Update(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                        acc += (double)iv0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_CDLSTALLEDPATTERN_Close(st);
            bench_stream_row("CDLSTALLEDPATTERN", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLSTALLEDPATTERN_Close(st); }
            bench_stream_row("CDLSTALLEDPATTERN", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLSTICKSANDWICH") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_CDLSTICKSANDWICH_Lookback();
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_open[t] = g_open[it & BENCH_MASK];
                g_rt_high[t] = g_high[it & BENCH_MASK];
                g_rt_low[t] = g_low[it & BENCH_MASK];
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_CDLSTICKSANDWICH(t, t, g_rt_open, g_rt_high, g_rt_low, g_rt_close, &begIdx, &nb, g_outIntBuf0);
                acc += (double)g_outIntBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_CDLSTICKSANDWICH_Stream *st = NULL;
            int iv0 = 0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CDLSTICKSANDWICH_Open(&st, g_open, g_high, g_low, g_close, g_nPoints, &iv0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_CDLSTICKSANDWICH_Update(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                    acc += (double)iv0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_CDLSTICKSANDWICH_Peek(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                        acc += (double)iv0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_CDLSTICKSANDWICH_Update(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                        acc += (double)iv0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_CDLSTICKSANDWICH_Close(st);
            bench_stream_row("CDLSTICKSANDWICH", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLSTICKSANDWICH_Close(st); }
            bench_stream_row("CDLSTICKSANDWICH", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLTAKURI") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_CDLTAKURI_Lookback();
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_open[t] = g_open[it & BENCH_MASK];
                g_rt_high[t] = g_high[it & BENCH_MASK];
                g_rt_low[t] = g_low[it & BENCH_MASK];
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_CDLTAKURI(t, t, g_rt_open, g_rt_high, g_rt_low, g_rt_close, &begIdx, &nb, g_outIntBuf0);
                acc += (double)g_outIntBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_CDLTAKURI_Stream *st = NULL;
            int iv0 = 0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CDLTAKURI_Open(&st, g_open, g_high, g_low, g_close, g_nPoints, &iv0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_CDLTAKURI_Update(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                    acc += (double)iv0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_CDLTAKURI_Peek(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                        acc += (double)iv0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_CDLTAKURI_Update(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                        acc += (double)iv0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_CDLTAKURI_Close(st);
            bench_stream_row("CDLTAKURI", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLTAKURI_Close(st); }
            bench_stream_row("CDLTAKURI", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLTASUKIGAP") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_CDLTASUKIGAP_Lookback();
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_open[t] = g_open[it & BENCH_MASK];
                g_rt_high[t] = g_high[it & BENCH_MASK];
                g_rt_low[t] = g_low[it & BENCH_MASK];
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_CDLTASUKIGAP(t, t, g_rt_open, g_rt_high, g_rt_low, g_rt_close, &begIdx, &nb, g_outIntBuf0);
                acc += (double)g_outIntBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_CDLTASUKIGAP_Stream *st = NULL;
            int iv0 = 0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CDLTASUKIGAP_Open(&st, g_open, g_high, g_low, g_close, g_nPoints, &iv0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_CDLTASUKIGAP_Update(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                    acc += (double)iv0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_CDLTASUKIGAP_Peek(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                        acc += (double)iv0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_CDLTASUKIGAP_Update(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                        acc += (double)iv0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_CDLTASUKIGAP_Close(st);
            bench_stream_row("CDLTASUKIGAP", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLTASUKIGAP_Close(st); }
            bench_stream_row("CDLTASUKIGAP", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLTHRUSTING") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_CDLTHRUSTING_Lookback();
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_open[t] = g_open[it & BENCH_MASK];
                g_rt_high[t] = g_high[it & BENCH_MASK];
                g_rt_low[t] = g_low[it & BENCH_MASK];
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_CDLTHRUSTING(t, t, g_rt_open, g_rt_high, g_rt_low, g_rt_close, &begIdx, &nb, g_outIntBuf0);
                acc += (double)g_outIntBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_CDLTHRUSTING_Stream *st = NULL;
            int iv0 = 0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CDLTHRUSTING_Open(&st, g_open, g_high, g_low, g_close, g_nPoints, &iv0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_CDLTHRUSTING_Update(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                    acc += (double)iv0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_CDLTHRUSTING_Peek(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                        acc += (double)iv0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_CDLTHRUSTING_Update(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                        acc += (double)iv0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_CDLTHRUSTING_Close(st);
            bench_stream_row("CDLTHRUSTING", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLTHRUSTING_Close(st); }
            bench_stream_row("CDLTHRUSTING", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLTRISTAR") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_CDLTRISTAR_Lookback();
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_open[t] = g_open[it & BENCH_MASK];
                g_rt_high[t] = g_high[it & BENCH_MASK];
                g_rt_low[t] = g_low[it & BENCH_MASK];
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_CDLTRISTAR(t, t, g_rt_open, g_rt_high, g_rt_low, g_rt_close, &begIdx, &nb, g_outIntBuf0);
                acc += (double)g_outIntBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_CDLTRISTAR_Stream *st = NULL;
            int iv0 = 0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CDLTRISTAR_Open(&st, g_open, g_high, g_low, g_close, g_nPoints, &iv0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_CDLTRISTAR_Update(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                    acc += (double)iv0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_CDLTRISTAR_Peek(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                        acc += (double)iv0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_CDLTRISTAR_Update(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                        acc += (double)iv0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_CDLTRISTAR_Close(st);
            bench_stream_row("CDLTRISTAR", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLTRISTAR_Close(st); }
            bench_stream_row("CDLTRISTAR", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLUNIQUE3RIVER") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_CDLUNIQUE3RIVER_Lookback();
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_open[t] = g_open[it & BENCH_MASK];
                g_rt_high[t] = g_high[it & BENCH_MASK];
                g_rt_low[t] = g_low[it & BENCH_MASK];
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_CDLUNIQUE3RIVER(t, t, g_rt_open, g_rt_high, g_rt_low, g_rt_close, &begIdx, &nb, g_outIntBuf0);
                acc += (double)g_outIntBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_CDLUNIQUE3RIVER_Stream *st = NULL;
            int iv0 = 0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CDLUNIQUE3RIVER_Open(&st, g_open, g_high, g_low, g_close, g_nPoints, &iv0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_CDLUNIQUE3RIVER_Update(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                    acc += (double)iv0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_CDLUNIQUE3RIVER_Peek(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                        acc += (double)iv0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_CDLUNIQUE3RIVER_Update(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                        acc += (double)iv0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_CDLUNIQUE3RIVER_Close(st);
            bench_stream_row("CDLUNIQUE3RIVER", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLUNIQUE3RIVER_Close(st); }
            bench_stream_row("CDLUNIQUE3RIVER", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLUPSIDEGAP2CROWS") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_CDLUPSIDEGAP2CROWS_Lookback();
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_open[t] = g_open[it & BENCH_MASK];
                g_rt_high[t] = g_high[it & BENCH_MASK];
                g_rt_low[t] = g_low[it & BENCH_MASK];
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_CDLUPSIDEGAP2CROWS(t, t, g_rt_open, g_rt_high, g_rt_low, g_rt_close, &begIdx, &nb, g_outIntBuf0);
                acc += (double)g_outIntBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_CDLUPSIDEGAP2CROWS_Stream *st = NULL;
            int iv0 = 0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CDLUPSIDEGAP2CROWS_Open(&st, g_open, g_high, g_low, g_close, g_nPoints, &iv0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_CDLUPSIDEGAP2CROWS_Update(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                    acc += (double)iv0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_CDLUPSIDEGAP2CROWS_Peek(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                        acc += (double)iv0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_CDLUPSIDEGAP2CROWS_Update(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                        acc += (double)iv0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_CDLUPSIDEGAP2CROWS_Close(st);
            bench_stream_row("CDLUPSIDEGAP2CROWS", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLUPSIDEGAP2CROWS_Close(st); }
            bench_stream_row("CDLUPSIDEGAP2CROWS", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLXSIDEGAP3METHODS") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_CDLXSIDEGAP3METHODS_Lookback();
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_open[t] = g_open[it & BENCH_MASK];
                g_rt_high[t] = g_high[it & BENCH_MASK];
                g_rt_low[t] = g_low[it & BENCH_MASK];
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_CDLXSIDEGAP3METHODS(t, t, g_rt_open, g_rt_high, g_rt_low, g_rt_close, &begIdx, &nb, g_outIntBuf0);
                acc += (double)g_outIntBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_CDLXSIDEGAP3METHODS_Stream *st = NULL;
            int iv0 = 0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CDLXSIDEGAP3METHODS_Open(&st, g_open, g_high, g_low, g_close, g_nPoints, &iv0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_CDLXSIDEGAP3METHODS_Update(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                    acc += (double)iv0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_CDLXSIDEGAP3METHODS_Peek(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                        acc += (double)iv0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_CDLXSIDEGAP3METHODS_Update(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &iv0);
                        acc += (double)iv0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_CDLXSIDEGAP3METHODS_Close(st);
            bench_stream_row("CDLXSIDEGAP3METHODS", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLXSIDEGAP3METHODS_Close(st); }
            bench_stream_row("CDLXSIDEGAP3METHODS", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CEIL") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_CEIL_Lookback();
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_CEIL(t, t, g_rt_close, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_CEIL_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CEIL_Open(&st, g_close, g_nPoints, &v0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_CEIL_Update(st, g_close[it & BENCH_MASK], &v0);
                    acc += v0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_CEIL_Peek(st, g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_CEIL_Update(st, g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_CEIL_Close(st);
            bench_stream_row("CEIL", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CEIL_Close(st); }
            bench_stream_row("CEIL", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CG") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        const int optInTimePeriod = bench_opaque_int(g_period > 0 ? g_period : 10);
        int lb = TA_CG_Lookback(optInTimePeriod);
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_CG(t, t, g_rt_close, optInTimePeriod, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_CG_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CG_Open(&st, g_close, g_nPoints, optInTimePeriod, &v0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_CG_Update(st, g_close[it & BENCH_MASK], &v0);
                    acc += v0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_CG_Peek(st, g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_CG_Update(st, g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_CG_Close(st);
            bench_stream_row("CG", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CG_Close(st); }
            bench_stream_row("CG", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CMF") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        const int optInTimePeriod = bench_opaque_int(g_period > 0 ? g_period : 20);
        int lb = TA_CMF_Lookback(optInTimePeriod);
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_high[t] = g_high[it & BENCH_MASK];
                g_rt_low[t] = g_low[it & BENCH_MASK];
                g_rt_close[t] = g_close[it & BENCH_MASK];
                g_rt_volume[t] = g_volume[it & BENCH_MASK];
                TA_CMF(t, t, g_rt_high, g_rt_low, g_rt_close, g_rt_volume, optInTimePeriod, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_CMF_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CMF_Open(&st, g_high, g_low, g_close, g_volume, g_nPoints, optInTimePeriod, &v0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_CMF_Update(st, g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], g_volume[it & BENCH_MASK], &v0);
                    acc += v0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_CMF_Peek(st, g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], g_volume[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_CMF_Update(st, g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], g_volume[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_CMF_Close(st);
            bench_stream_row("CMF", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CMF_Close(st); }
            bench_stream_row("CMF", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CMO") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        const int optInTimePeriod = bench_opaque_int(g_period > 0 ? g_period : 14);
        int lb = TA_CMO_Lookback(optInTimePeriod);
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_CMO(t, t, g_rt_close, optInTimePeriod, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_CMO_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CMO_Open(&st, g_close, g_nPoints, optInTimePeriod, &v0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_CMO_Update(st, g_close[it & BENCH_MASK], &v0);
                    acc += v0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_CMO_Peek(st, g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_CMO_Update(st, g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_CMO_Close(st);
            bench_stream_row("CMO", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CMO_Close(st); }
            bench_stream_row("CMO", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CMOU") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        const int optInTimePeriod = bench_opaque_int(g_period > 0 ? g_period : 14);
        int lb = TA_CMOU_Lookback(optInTimePeriod);
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_CMOU(t, t, g_rt_close, optInTimePeriod, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_CMOU_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CMOU_Open(&st, g_close, g_nPoints, optInTimePeriod, &v0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_CMOU_Update(st, g_close[it & BENCH_MASK], &v0);
                    acc += v0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_CMOU_Peek(st, g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_CMOU_Update(st, g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_CMOU_Close(st);
            bench_stream_row("CMOU", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CMOU_Close(st); }
            bench_stream_row("CMOU", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "COPPOCK") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        const int optInWMAPeriod = bench_opaque_int(10);
        const int optInROC1Period = bench_opaque_int(11);
        const int optInROC2Period = bench_opaque_int(14);
        int lb = TA_COPPOCK_Lookback(optInWMAPeriod, optInROC1Period, optInROC2Period);
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_COPPOCK(t, t, g_rt_close, optInWMAPeriod, optInROC1Period, optInROC2Period, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_COPPOCK_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_COPPOCK_Open(&st, g_close, g_nPoints, optInWMAPeriod, optInROC1Period, optInROC2Period, &v0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_COPPOCK_Update(st, g_close[it & BENCH_MASK], &v0);
                    acc += v0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_COPPOCK_Peek(st, g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_COPPOCK_Update(st, g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_COPPOCK_Close(st);
            bench_stream_row("COPPOCK", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_COPPOCK_Close(st); }
            bench_stream_row("COPPOCK", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CORREL") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        const int optInTimePeriod = bench_opaque_int(g_period > 0 ? g_period : 30);
        int lb = TA_CORREL_Lookback(optInTimePeriod);
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_close[t] = g_close[it & BENCH_MASK];
                g_rt_high[t] = g_high[it & BENCH_MASK];
                TA_CORREL(t, t, g_rt_close, g_rt_high, optInTimePeriod, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_CORREL_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CORREL_Open(&st, g_close, g_high, g_nPoints, optInTimePeriod, &v0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_CORREL_Update(st, g_close[it & BENCH_MASK], g_high[it & BENCH_MASK], &v0);
                    acc += v0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_CORREL_Peek(st, g_close[it & BENCH_MASK], g_high[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_CORREL_Update(st, g_close[it & BENCH_MASK], g_high[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_CORREL_Close(st);
            bench_stream_row("CORREL", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CORREL_Close(st); }
            bench_stream_row("CORREL", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "COS") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_COS_Lookback();
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_COS(t, t, g_rt_close, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_COS_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_COS_Open(&st, g_close, g_nPoints, &v0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_COS_Update(st, g_close[it & BENCH_MASK], &v0);
                    acc += v0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_COS_Peek(st, g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_COS_Update(st, g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_COS_Close(st);
            bench_stream_row("COS", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_COS_Close(st); }
            bench_stream_row("COS", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "COSH") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_COSH_Lookback();
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_COSH(t, t, g_rt_close, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_COSH_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_COSH_Open(&st, g_close, g_nPoints, &v0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_COSH_Update(st, g_close[it & BENCH_MASK], &v0);
                    acc += v0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_COSH_Peek(st, g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_COSH_Update(st, g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_COSH_Close(st);
            bench_stream_row("COSH", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_COSH_Close(st); }
            bench_stream_row("COSH", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CRSI") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        const int optInTimePeriod = bench_opaque_int(g_period > 0 ? g_period : 3);
        const int optInStreakPeriod = bench_opaque_int(2);
        const int optInRankPeriod = bench_opaque_int(100);
        int lb = TA_CRSI_Lookback(optInTimePeriod, optInStreakPeriod, optInRankPeriod);
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_CRSI(t, t, g_rt_close, optInTimePeriod, optInStreakPeriod, optInRankPeriod, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_CRSI_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CRSI_Open(&st, g_close, g_nPoints, optInTimePeriod, optInStreakPeriod, optInRankPeriod, &v0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_CRSI_Update(st, g_close[it & BENCH_MASK], &v0);
                    acc += v0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_CRSI_Peek(st, g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_CRSI_Update(st, g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_CRSI_Close(st);
            bench_stream_row("CRSI", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CRSI_Close(st); }
            bench_stream_row("CRSI", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CTI") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        const int optInTimePeriod = bench_opaque_int(g_period > 0 ? g_period : 20);
        int lb = TA_CTI_Lookback(optInTimePeriod);
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_CTI(t, t, g_rt_close, optInTimePeriod, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_CTI_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CTI_Open(&st, g_close, g_nPoints, optInTimePeriod, &v0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_CTI_Update(st, g_close[it & BENCH_MASK], &v0);
                    acc += v0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_CTI_Peek(st, g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_CTI_Update(st, g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_CTI_Close(st);
            bench_stream_row("CTI", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CTI_Close(st); }
            bench_stream_row("CTI", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CUMSUM") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_CUMSUM_Lookback();
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_CUMSUM(t, t, g_rt_close, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_CUMSUM_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CUMSUM_Open(&st, g_close, g_nPoints, &v0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_CUMSUM_Update(st, g_close[it & BENCH_MASK], &v0);
                    acc += v0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_CUMSUM_Peek(st, g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_CUMSUM_Update(st, g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_CUMSUM_Close(st);
            bench_stream_row("CUMSUM", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CUMSUM_Close(st); }
            bench_stream_row("CUMSUM", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CVI") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        const int optInTimePeriod = bench_opaque_int(g_period > 0 ? g_period : 10);
        const int optInROCPeriod = bench_opaque_int(10);
        int lb = TA_CVI_Lookback(optInTimePeriod, optInROCPeriod);
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_high[t] = g_high[it & BENCH_MASK];
                g_rt_low[t] = g_low[it & BENCH_MASK];
                TA_CVI(t, t, g_rt_high, g_rt_low, optInTimePeriod, optInROCPeriod, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_CVI_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CVI_Open(&st, g_high, g_low, g_nPoints, optInTimePeriod, optInROCPeriod, &v0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_CVI_Update(st, g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], &v0);
                    acc += v0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_CVI_Peek(st, g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_CVI_Update(st, g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_CVI_Close(st);
            bench_stream_row("CVI", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CVI_Close(st); }
            bench_stream_row("CVI", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "DEMA") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        const int optInTimePeriod = bench_opaque_int(g_period > 0 ? g_period : 30);
        int lb = TA_DEMA_Lookback(optInTimePeriod);
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_DEMA(t, t, g_rt_close, optInTimePeriod, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_DEMA_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_DEMA_Open(&st, g_close, g_nPoints, optInTimePeriod, &v0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_DEMA_Update(st, g_close[it & BENCH_MASK], &v0);
                    acc += v0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_DEMA_Peek(st, g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_DEMA_Update(st, g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_DEMA_Close(st);
            bench_stream_row("DEMA", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_DEMA_Close(st); }
            bench_stream_row("DEMA", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "DIV") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_DIV_Lookback();
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_close[t] = g_close[it & BENCH_MASK];
                g_rt_high[t] = g_high[it & BENCH_MASK];
                TA_DIV(t, t, g_rt_close, g_rt_high, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_DIV_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_DIV_Open(&st, g_close, g_high, g_nPoints, &v0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_DIV_Update(st, g_close[it & BENCH_MASK], g_high[it & BENCH_MASK], &v0);
                    acc += v0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_DIV_Peek(st, g_close[it & BENCH_MASK], g_high[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_DIV_Update(st, g_close[it & BENCH_MASK], g_high[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_DIV_Close(st);
            bench_stream_row("DIV", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_DIV_Close(st); }
            bench_stream_row("DIV", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "DONCHIAN") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        const int optInTimePeriod = bench_opaque_int(g_period > 0 ? g_period : 20);
        int lb = TA_DONCHIAN_Lookback(optInTimePeriod);
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_high[t] = g_high[it & BENCH_MASK];
                g_rt_low[t] = g_low[it & BENCH_MASK];
                TA_DONCHIAN(t, t, g_rt_high, g_rt_low, optInTimePeriod, &begIdx, &nb, g_outBuf0, g_outBuf1, g_outBuf2);
                acc += g_outBuf0[0];
                acc += g_outBuf1[0];
                acc += g_outBuf2[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_DONCHIAN_Stream *st = NULL;
            double v0 = 0.0;
            double v1 = 0.0;
            double v2 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_DONCHIAN_Open(&st, g_high, g_low, g_nPoints, optInTimePeriod, &v0, &v1, &v2);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_DONCHIAN_Update(st, g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], &v0, &v1, &v2);
                    acc += v0;
                    acc += v1;
                    acc += v2;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_DONCHIAN_Peek(st, g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], &v0, &v1, &v2);
                        acc += v0;
                        acc += v1;
                        acc += v2;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_DONCHIAN_Update(st, g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], &v0, &v1, &v2);
                        acc += v0;
                        acc += v1;
                        acc += v2;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_DONCHIAN_Close(st);
            bench_stream_row("DONCHIAN", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_DONCHIAN_Close(st); }
            bench_stream_row("DONCHIAN", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "DPO") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        const int optInTimePeriod = bench_opaque_int(g_period > 0 ? g_period : 20);
        int lb = TA_DPO_Lookback(optInTimePeriod);
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_DPO(t, t, g_rt_close, optInTimePeriod, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_DPO_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_DPO_Open(&st, g_close, g_nPoints, optInTimePeriod, &v0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_DPO_Update(st, g_close[it & BENCH_MASK], &v0);
                    acc += v0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_DPO_Peek(st, g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_DPO_Update(st, g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_DPO_Close(st);
            bench_stream_row("DPO", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_DPO_Close(st); }
            bench_stream_row("DPO", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "DX") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        const int optInTimePeriod = bench_opaque_int(g_period > 0 ? g_period : 14);
        int lb = TA_DX_Lookback(optInTimePeriod);
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_high[t] = g_high[it & BENCH_MASK];
                g_rt_low[t] = g_low[it & BENCH_MASK];
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_DX(t, t, g_rt_high, g_rt_low, g_rt_close, optInTimePeriod, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_DX_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_DX_Open(&st, g_high, g_low, g_close, g_nPoints, optInTimePeriod, &v0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_DX_Update(st, g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &v0);
                    acc += v0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_DX_Peek(st, g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_DX_Update(st, g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_DX_Close(st);
            bench_stream_row("DX", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_DX_Close(st); }
            bench_stream_row("DX", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "EFI") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        const int optInTimePeriod = bench_opaque_int(g_period > 0 ? g_period : 13);
        int lb = TA_EFI_Lookback(optInTimePeriod);
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_close[t] = g_close[it & BENCH_MASK];
                g_rt_volume[t] = g_volume[it & BENCH_MASK];
                TA_EFI(t, t, g_rt_close, g_rt_volume, optInTimePeriod, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_EFI_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_EFI_Open(&st, g_close, g_volume, g_nPoints, optInTimePeriod, &v0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_EFI_Update(st, g_close[it & BENCH_MASK], g_volume[it & BENCH_MASK], &v0);
                    acc += v0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_EFI_Peek(st, g_close[it & BENCH_MASK], g_volume[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_EFI_Update(st, g_close[it & BENCH_MASK], g_volume[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_EFI_Close(st);
            bench_stream_row("EFI", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_EFI_Close(st); }
            bench_stream_row("EFI", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "EMA") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        const int optInTimePeriod = bench_opaque_int(g_period > 0 ? g_period : 30);
        int lb = TA_EMA_Lookback(optInTimePeriod);
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_EMA(t, t, g_rt_close, optInTimePeriod, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_EMA_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_EMA_Open(&st, g_close, g_nPoints, optInTimePeriod, &v0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_EMA_Update(st, g_close[it & BENCH_MASK], &v0);
                    acc += v0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_EMA_Peek(st, g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_EMA_Update(st, g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_EMA_Close(st);
            bench_stream_row("EMA", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_EMA_Close(st); }
            bench_stream_row("EMA", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "ER") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        const int optInTimePeriod = bench_opaque_int(g_period > 0 ? g_period : 10);
        int lb = TA_ER_Lookback(optInTimePeriod);
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_ER(t, t, g_rt_close, optInTimePeriod, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_ER_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_ER_Open(&st, g_close, g_nPoints, optInTimePeriod, &v0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_ER_Update(st, g_close[it & BENCH_MASK], &v0);
                    acc += v0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_ER_Peek(st, g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_ER_Update(st, g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_ER_Close(st);
            bench_stream_row("ER", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_ER_Close(st); }
            bench_stream_row("ER", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "ERI") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        const int optInTimePeriod = bench_opaque_int(g_period > 0 ? g_period : 13);
        int lb = TA_ERI_Lookback(optInTimePeriod);
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_high[t] = g_high[it & BENCH_MASK];
                g_rt_low[t] = g_low[it & BENCH_MASK];
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_ERI(t, t, g_rt_high, g_rt_low, g_rt_close, optInTimePeriod, &begIdx, &nb, g_outBuf0, g_outBuf1);
                acc += g_outBuf0[0];
                acc += g_outBuf1[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_ERI_Stream *st = NULL;
            double v0 = 0.0;
            double v1 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_ERI_Open(&st, g_high, g_low, g_close, g_nPoints, optInTimePeriod, &v0, &v1);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_ERI_Update(st, g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &v0, &v1);
                    acc += v0;
                    acc += v1;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_ERI_Peek(st, g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &v0, &v1);
                        acc += v0;
                        acc += v1;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_ERI_Update(st, g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &v0, &v1);
                        acc += v0;
                        acc += v1;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_ERI_Close(st);
            bench_stream_row("ERI", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_ERI_Close(st); }
            bench_stream_row("ERI", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "EXP") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_EXP_Lookback();
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_EXP(t, t, g_rt_close, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_EXP_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_EXP_Open(&st, g_close, g_nPoints, &v0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_EXP_Update(st, g_close[it & BENCH_MASK], &v0);
                    acc += v0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_EXP_Peek(st, g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_EXP_Update(st, g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_EXP_Close(st);
            bench_stream_row("EXP", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_EXP_Close(st); }
            bench_stream_row("EXP", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "FLOOR") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_FLOOR_Lookback();
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_FLOOR(t, t, g_rt_close, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_FLOOR_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_FLOOR_Open(&st, g_close, g_nPoints, &v0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_FLOOR_Update(st, g_close[it & BENCH_MASK], &v0);
                    acc += v0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_FLOOR_Peek(st, g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_FLOOR_Update(st, g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_FLOOR_Close(st);
            bench_stream_row("FLOOR", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_FLOOR_Close(st); }
            bench_stream_row("FLOOR", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "FOSC") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        const int optInTimePeriod = bench_opaque_int(g_period > 0 ? g_period : 5);
        int lb = TA_FOSC_Lookback(optInTimePeriod);
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_FOSC(t, t, g_rt_close, optInTimePeriod, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_FOSC_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_FOSC_Open(&st, g_close, g_nPoints, optInTimePeriod, &v0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_FOSC_Update(st, g_close[it & BENCH_MASK], &v0);
                    acc += v0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_FOSC_Peek(st, g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_FOSC_Update(st, g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_FOSC_Close(st);
            bench_stream_row("FOSC", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_FOSC_Close(st); }
            bench_stream_row("FOSC", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "FRACTAL") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        const int optInLeftBars = bench_opaque_int(2);
        const int optInRightBars = bench_opaque_int(2);
        int lb = TA_FRACTAL_Lookback(optInLeftBars, optInRightBars);
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_high[t] = g_high[it & BENCH_MASK];
                g_rt_low[t] = g_low[it & BENCH_MASK];
                TA_FRACTAL(t, t, g_rt_high, g_rt_low, optInLeftBars, optInRightBars, &begIdx, &nb, g_outIntBuf0, g_outIntBuf1);
                acc += (double)g_outIntBuf0[0];
                acc += (double)g_outIntBuf1[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_FRACTAL_Stream *st = NULL;
            int iv0 = 0;
            int iv1 = 0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_FRACTAL_Open(&st, g_high, g_low, g_nPoints, optInLeftBars, optInRightBars, &iv0, &iv1);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_FRACTAL_Update(st, g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], &iv0, &iv1);
                    acc += (double)iv0;
                    acc += (double)iv1;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_FRACTAL_Peek(st, g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], &iv0, &iv1);
                        acc += (double)iv0;
                        acc += (double)iv1;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_FRACTAL_Update(st, g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], &iv0, &iv1);
                        acc += (double)iv0;
                        acc += (double)iv1;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_FRACTAL_Close(st);
            bench_stream_row("FRACTAL", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_FRACTAL_Close(st); }
            bench_stream_row("FRACTAL", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "HA") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_HA_Lookback();
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_open[t] = g_open[it & BENCH_MASK];
                g_rt_high[t] = g_high[it & BENCH_MASK];
                g_rt_low[t] = g_low[it & BENCH_MASK];
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_HA(t, t, g_rt_open, g_rt_high, g_rt_low, g_rt_close, &begIdx, &nb, g_outBuf0, g_outBuf1, g_outBuf2, g_outBuf3);
                acc += g_outBuf0[0];
                acc += g_outBuf1[0];
                acc += g_outBuf2[0];
                acc += g_outBuf3[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_HA_Stream *st = NULL;
            double v0 = 0.0;
            double v1 = 0.0;
            double v2 = 0.0;
            double v3 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_HA_Open(&st, g_open, g_high, g_low, g_close, g_nPoints, &v0, &v1, &v2, &v3);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_HA_Update(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &v0, &v1, &v2, &v3);
                    acc += v0;
                    acc += v1;
                    acc += v2;
                    acc += v3;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_HA_Peek(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &v0, &v1, &v2, &v3);
                        acc += v0;
                        acc += v1;
                        acc += v2;
                        acc += v3;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_HA_Update(st, g_open[it & BENCH_MASK], g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &v0, &v1, &v2, &v3);
                        acc += v0;
                        acc += v1;
                        acc += v2;
                        acc += v3;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_HA_Close(st);
            bench_stream_row("HA", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_HA_Close(st); }
            bench_stream_row("HA", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "HMA") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        const int optInTimePeriod = bench_opaque_int(g_period > 0 ? g_period : 20);
        int lb = TA_HMA_Lookback(optInTimePeriod);
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_HMA(t, t, g_rt_close, optInTimePeriod, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_HMA_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_HMA_Open(&st, g_close, g_nPoints, optInTimePeriod, &v0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_HMA_Update(st, g_close[it & BENCH_MASK], &v0);
                    acc += v0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_HMA_Peek(st, g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_HMA_Update(st, g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_HMA_Close(st);
            bench_stream_row("HMA", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_HMA_Close(st); }
            bench_stream_row("HMA", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "HT_DCPERIOD") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_HT_DCPERIOD_Lookback();
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_HT_DCPERIOD(t, t, g_rt_close, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_HT_DCPERIOD_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_HT_DCPERIOD_Open(&st, g_close, g_nPoints, &v0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_HT_DCPERIOD_Update(st, g_close[it & BENCH_MASK], &v0);
                    acc += v0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_HT_DCPERIOD_Peek(st, g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_HT_DCPERIOD_Update(st, g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_HT_DCPERIOD_Close(st);
            bench_stream_row("HT_DCPERIOD", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_HT_DCPERIOD_Close(st); }
            bench_stream_row("HT_DCPERIOD", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "HT_DCPHASE") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_HT_DCPHASE_Lookback();
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_HT_DCPHASE(t, t, g_rt_close, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_HT_DCPHASE_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_HT_DCPHASE_Open(&st, g_close, g_nPoints, &v0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_HT_DCPHASE_Update(st, g_close[it & BENCH_MASK], &v0);
                    acc += v0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_HT_DCPHASE_Peek(st, g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_HT_DCPHASE_Update(st, g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_HT_DCPHASE_Close(st);
            bench_stream_row("HT_DCPHASE", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_HT_DCPHASE_Close(st); }
            bench_stream_row("HT_DCPHASE", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "HT_PHASOR") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_HT_PHASOR_Lookback();
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_HT_PHASOR(t, t, g_rt_close, &begIdx, &nb, g_outBuf0, g_outBuf1);
                acc += g_outBuf0[0];
                acc += g_outBuf1[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_HT_PHASOR_Stream *st = NULL;
            double v0 = 0.0;
            double v1 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_HT_PHASOR_Open(&st, g_close, g_nPoints, &v0, &v1);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_HT_PHASOR_Update(st, g_close[it & BENCH_MASK], &v0, &v1);
                    acc += v0;
                    acc += v1;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_HT_PHASOR_Peek(st, g_close[it & BENCH_MASK], &v0, &v1);
                        acc += v0;
                        acc += v1;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_HT_PHASOR_Update(st, g_close[it & BENCH_MASK], &v0, &v1);
                        acc += v0;
                        acc += v1;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_HT_PHASOR_Close(st);
            bench_stream_row("HT_PHASOR", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_HT_PHASOR_Close(st); }
            bench_stream_row("HT_PHASOR", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "HT_SINE") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_HT_SINE_Lookback();
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_HT_SINE(t, t, g_rt_close, &begIdx, &nb, g_outBuf0, g_outBuf1);
                acc += g_outBuf0[0];
                acc += g_outBuf1[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_HT_SINE_Stream *st = NULL;
            double v0 = 0.0;
            double v1 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_HT_SINE_Open(&st, g_close, g_nPoints, &v0, &v1);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_HT_SINE_Update(st, g_close[it & BENCH_MASK], &v0, &v1);
                    acc += v0;
                    acc += v1;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_HT_SINE_Peek(st, g_close[it & BENCH_MASK], &v0, &v1);
                        acc += v0;
                        acc += v1;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_HT_SINE_Update(st, g_close[it & BENCH_MASK], &v0, &v1);
                        acc += v0;
                        acc += v1;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_HT_SINE_Close(st);
            bench_stream_row("HT_SINE", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_HT_SINE_Close(st); }
            bench_stream_row("HT_SINE", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "HT_TRENDLINE") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_HT_TRENDLINE_Lookback();
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_HT_TRENDLINE(t, t, g_rt_close, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_HT_TRENDLINE_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_HT_TRENDLINE_Open(&st, g_close, g_nPoints, &v0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_HT_TRENDLINE_Update(st, g_close[it & BENCH_MASK], &v0);
                    acc += v0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_HT_TRENDLINE_Peek(st, g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_HT_TRENDLINE_Update(st, g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_HT_TRENDLINE_Close(st);
            bench_stream_row("HT_TRENDLINE", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_HT_TRENDLINE_Close(st); }
            bench_stream_row("HT_TRENDLINE", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "HT_TRENDMODE") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_HT_TRENDMODE_Lookback();
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_HT_TRENDMODE(t, t, g_rt_close, &begIdx, &nb, g_outIntBuf0);
                acc += (double)g_outIntBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_HT_TRENDMODE_Stream *st = NULL;
            int iv0 = 0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_HT_TRENDMODE_Open(&st, g_close, g_nPoints, &iv0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_HT_TRENDMODE_Update(st, g_close[it & BENCH_MASK], &iv0);
                    acc += (double)iv0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_HT_TRENDMODE_Peek(st, g_close[it & BENCH_MASK], &iv0);
                        acc += (double)iv0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_HT_TRENDMODE_Update(st, g_close[it & BENCH_MASK], &iv0);
                        acc += (double)iv0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_HT_TRENDMODE_Close(st);
            bench_stream_row("HT_TRENDMODE", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_HT_TRENDMODE_Close(st); }
            bench_stream_row("HT_TRENDMODE", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "IMI") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        const int optInTimePeriod = bench_opaque_int(g_period > 0 ? g_period : 14);
        int lb = TA_IMI_Lookback(optInTimePeriod);
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_open[t] = g_open[it & BENCH_MASK];
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_IMI(t, t, g_rt_open, g_rt_close, optInTimePeriod, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_IMI_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_IMI_Open(&st, g_open, g_close, g_nPoints, optInTimePeriod, &v0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_IMI_Update(st, g_open[it & BENCH_MASK], g_close[it & BENCH_MASK], &v0);
                    acc += v0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_IMI_Peek(st, g_open[it & BENCH_MASK], g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_IMI_Update(st, g_open[it & BENCH_MASK], g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_IMI_Close(st);
            bench_stream_row("IMI", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_IMI_Close(st); }
            bench_stream_row("IMI", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "KAMA") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        const int optInTimePeriod = bench_opaque_int(g_period > 0 ? g_period : 30);
        int lb = TA_KAMA_Lookback(optInTimePeriod);
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_KAMA(t, t, g_rt_close, optInTimePeriod, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_KAMA_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_KAMA_Open(&st, g_close, g_nPoints, optInTimePeriod, &v0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_KAMA_Update(st, g_close[it & BENCH_MASK], &v0);
                    acc += v0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_KAMA_Peek(st, g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_KAMA_Update(st, g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_KAMA_Close(st);
            bench_stream_row("KAMA", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_KAMA_Close(st); }
            bench_stream_row("KAMA", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "KC") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        const int optInTimePeriod = bench_opaque_int(g_period > 0 ? g_period : 20);
        const int optInATRPeriod = bench_opaque_int(10);
        const double optInNbDev = bench_opaque_double(2.000000000000000);
        int lb = TA_KC_Lookback(optInTimePeriod, optInATRPeriod, optInNbDev);
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_high[t] = g_high[it & BENCH_MASK];
                g_rt_low[t] = g_low[it & BENCH_MASK];
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_KC(t, t, g_rt_high, g_rt_low, g_rt_close, optInTimePeriod, optInATRPeriod, optInNbDev, &begIdx, &nb, g_outBuf0, g_outBuf1, g_outBuf2);
                acc += g_outBuf0[0];
                acc += g_outBuf1[0];
                acc += g_outBuf2[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_KC_Stream *st = NULL;
            double v0 = 0.0;
            double v1 = 0.0;
            double v2 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_KC_Open(&st, g_high, g_low, g_close, g_nPoints, optInTimePeriod, optInATRPeriod, optInNbDev, &v0, &v1, &v2);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_KC_Update(st, g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &v0, &v1, &v2);
                    acc += v0;
                    acc += v1;
                    acc += v2;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_KC_Peek(st, g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &v0, &v1, &v2);
                        acc += v0;
                        acc += v1;
                        acc += v2;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_KC_Update(st, g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &v0, &v1, &v2);
                        acc += v0;
                        acc += v1;
                        acc += v2;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_KC_Close(st);
            bench_stream_row("KC", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_KC_Close(st); }
            bench_stream_row("KC", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "KDJ") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        const int optInFastK_Period = bench_opaque_int(9);
        const int optInSlowK_Period = bench_opaque_int(3);
        const int optInSlowK_MAType = bench_opaque_int(13);
        const int optInSlowD_Period = bench_opaque_int(3);
        const int optInSlowD_MAType = bench_opaque_int(13);
        int lb = TA_KDJ_Lookback(optInFastK_Period, optInSlowK_Period, optInSlowK_MAType, optInSlowD_Period, optInSlowD_MAType);
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_high[t] = g_high[it & BENCH_MASK];
                g_rt_low[t] = g_low[it & BENCH_MASK];
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_KDJ(t, t, g_rt_high, g_rt_low, g_rt_close, optInFastK_Period, optInSlowK_Period, optInSlowK_MAType, optInSlowD_Period, optInSlowD_MAType, &begIdx, &nb, g_outBuf0, g_outBuf1, g_outBuf2);
                acc += g_outBuf0[0];
                acc += g_outBuf1[0];
                acc += g_outBuf2[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_KDJ_Stream *st = NULL;
            double v0 = 0.0;
            double v1 = 0.0;
            double v2 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_KDJ_Open(&st, g_high, g_low, g_close, g_nPoints, optInFastK_Period, optInSlowK_Period, optInSlowK_MAType, optInSlowD_Period, optInSlowD_MAType, &v0, &v1, &v2);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_KDJ_Update(st, g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &v0, &v1, &v2);
                    acc += v0;
                    acc += v1;
                    acc += v2;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_KDJ_Peek(st, g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &v0, &v1, &v2);
                        acc += v0;
                        acc += v1;
                        acc += v2;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_KDJ_Update(st, g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &v0, &v1, &v2);
                        acc += v0;
                        acc += v1;
                        acc += v2;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_KDJ_Close(st);
            bench_stream_row("KDJ", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_KDJ_Close(st); }
            bench_stream_row("KDJ", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "KURTOSIS") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        const int optInTimePeriod = bench_opaque_int(g_period > 0 ? g_period : 30);
        int lb = TA_KURTOSIS_Lookback(optInTimePeriod);
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_KURTOSIS(t, t, g_rt_close, optInTimePeriod, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_KURTOSIS_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_KURTOSIS_Open(&st, g_close, g_nPoints, optInTimePeriod, &v0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_KURTOSIS_Update(st, g_close[it & BENCH_MASK], &v0);
                    acc += v0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_KURTOSIS_Peek(st, g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_KURTOSIS_Update(st, g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_KURTOSIS_Close(st);
            bench_stream_row("KURTOSIS", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_KURTOSIS_Close(st); }
            bench_stream_row("KURTOSIS", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "LINEARREG") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        const int optInTimePeriod = bench_opaque_int(g_period > 0 ? g_period : 14);
        int lb = TA_LINEARREG_Lookback(optInTimePeriod);
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_LINEARREG(t, t, g_rt_close, optInTimePeriod, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_LINEARREG_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_LINEARREG_Open(&st, g_close, g_nPoints, optInTimePeriod, &v0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_LINEARREG_Update(st, g_close[it & BENCH_MASK], &v0);
                    acc += v0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_LINEARREG_Peek(st, g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_LINEARREG_Update(st, g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_LINEARREG_Close(st);
            bench_stream_row("LINEARREG", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_LINEARREG_Close(st); }
            bench_stream_row("LINEARREG", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "LINEARREG_ANGLE") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        const int optInTimePeriod = bench_opaque_int(g_period > 0 ? g_period : 14);
        int lb = TA_LINEARREG_ANGLE_Lookback(optInTimePeriod);
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_LINEARREG_ANGLE(t, t, g_rt_close, optInTimePeriod, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_LINEARREG_ANGLE_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_LINEARREG_ANGLE_Open(&st, g_close, g_nPoints, optInTimePeriod, &v0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_LINEARREG_ANGLE_Update(st, g_close[it & BENCH_MASK], &v0);
                    acc += v0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_LINEARREG_ANGLE_Peek(st, g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_LINEARREG_ANGLE_Update(st, g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_LINEARREG_ANGLE_Close(st);
            bench_stream_row("LINEARREG_ANGLE", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_LINEARREG_ANGLE_Close(st); }
            bench_stream_row("LINEARREG_ANGLE", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "LINEARREG_INTERCEPT") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        const int optInTimePeriod = bench_opaque_int(g_period > 0 ? g_period : 14);
        int lb = TA_LINEARREG_INTERCEPT_Lookback(optInTimePeriod);
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_LINEARREG_INTERCEPT(t, t, g_rt_close, optInTimePeriod, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_LINEARREG_INTERCEPT_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_LINEARREG_INTERCEPT_Open(&st, g_close, g_nPoints, optInTimePeriod, &v0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_LINEARREG_INTERCEPT_Update(st, g_close[it & BENCH_MASK], &v0);
                    acc += v0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_LINEARREG_INTERCEPT_Peek(st, g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_LINEARREG_INTERCEPT_Update(st, g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_LINEARREG_INTERCEPT_Close(st);
            bench_stream_row("LINEARREG_INTERCEPT", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_LINEARREG_INTERCEPT_Close(st); }
            bench_stream_row("LINEARREG_INTERCEPT", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "LINEARREG_SLOPE") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        const int optInTimePeriod = bench_opaque_int(g_period > 0 ? g_period : 14);
        int lb = TA_LINEARREG_SLOPE_Lookback(optInTimePeriod);
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_LINEARREG_SLOPE(t, t, g_rt_close, optInTimePeriod, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_LINEARREG_SLOPE_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_LINEARREG_SLOPE_Open(&st, g_close, g_nPoints, optInTimePeriod, &v0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_LINEARREG_SLOPE_Update(st, g_close[it & BENCH_MASK], &v0);
                    acc += v0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_LINEARREG_SLOPE_Peek(st, g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_LINEARREG_SLOPE_Update(st, g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_LINEARREG_SLOPE_Close(st);
            bench_stream_row("LINEARREG_SLOPE", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_LINEARREG_SLOPE_Close(st); }
            bench_stream_row("LINEARREG_SLOPE", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "LN") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_LN_Lookback();
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_LN(t, t, g_rt_close, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_LN_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_LN_Open(&st, g_close, g_nPoints, &v0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_LN_Update(st, g_close[it & BENCH_MASK], &v0);
                    acc += v0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_LN_Peek(st, g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_LN_Update(st, g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_LN_Close(st);
            bench_stream_row("LN", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_LN_Close(st); }
            bench_stream_row("LN", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "LOG10") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_LOG10_Lookback();
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_LOG10(t, t, g_rt_close, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_LOG10_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_LOG10_Open(&st, g_close, g_nPoints, &v0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_LOG10_Update(st, g_close[it & BENCH_MASK], &v0);
                    acc += v0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_LOG10_Peek(st, g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_LOG10_Update(st, g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_LOG10_Close(st);
            bench_stream_row("LOG10", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_LOG10_Close(st); }
            bench_stream_row("LOG10", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "MA") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        const int optInTimePeriod = bench_opaque_int(g_period > 0 ? g_period : 30);
        const int optInMAType = bench_opaque_int(0);
        int lb = TA_MA_Lookback(optInTimePeriod, optInMAType);
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_MA(t, t, g_rt_close, optInTimePeriod, optInMAType, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_MA_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_MA_Open(&st, g_close, g_nPoints, optInTimePeriod, optInMAType, &v0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_MA_Update(st, g_close[it & BENCH_MASK], &v0);
                    acc += v0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_MA_Peek(st, g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_MA_Update(st, g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_MA_Close(st);
            bench_stream_row("MA", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_MA_Close(st); }
            bench_stream_row("MA", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "MACD") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        const int optInFastPeriod = bench_opaque_int(12);
        const int optInSlowPeriod = bench_opaque_int(26);
        const int optInSignalPeriod = bench_opaque_int(9);
        int lb = TA_MACD_Lookback(optInFastPeriod, optInSlowPeriod, optInSignalPeriod);
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_MACD(t, t, g_rt_close, optInFastPeriod, optInSlowPeriod, optInSignalPeriod, &begIdx, &nb, g_outBuf0, g_outBuf1, g_outBuf2);
                acc += g_outBuf0[0];
                acc += g_outBuf1[0];
                acc += g_outBuf2[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_MACD_Stream *st = NULL;
            double v0 = 0.0;
            double v1 = 0.0;
            double v2 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_MACD_Open(&st, g_close, g_nPoints, optInFastPeriod, optInSlowPeriod, optInSignalPeriod, &v0, &v1, &v2);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_MACD_Update(st, g_close[it & BENCH_MASK], &v0, &v1, &v2);
                    acc += v0;
                    acc += v1;
                    acc += v2;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_MACD_Peek(st, g_close[it & BENCH_MASK], &v0, &v1, &v2);
                        acc += v0;
                        acc += v1;
                        acc += v2;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_MACD_Update(st, g_close[it & BENCH_MASK], &v0, &v1, &v2);
                        acc += v0;
                        acc += v1;
                        acc += v2;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_MACD_Close(st);
            bench_stream_row("MACD", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_MACD_Close(st); }
            bench_stream_row("MACD", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "MACDEXT") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        const int optInFastPeriod = bench_opaque_int(12);
        const int optInFastMAType = bench_opaque_int(0);
        const int optInSlowPeriod = bench_opaque_int(26);
        const int optInSlowMAType = bench_opaque_int(0);
        const int optInSignalPeriod = bench_opaque_int(9);
        const int optInSignalMAType = bench_opaque_int(0);
        int lb = TA_MACDEXT_Lookback(optInFastPeriod, optInFastMAType, optInSlowPeriod, optInSlowMAType, optInSignalPeriod, optInSignalMAType);
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_MACDEXT(t, t, g_rt_close, optInFastPeriod, optInFastMAType, optInSlowPeriod, optInSlowMAType, optInSignalPeriod, optInSignalMAType, &begIdx, &nb, g_outBuf0, g_outBuf1, g_outBuf2);
                acc += g_outBuf0[0];
                acc += g_outBuf1[0];
                acc += g_outBuf2[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_MACDEXT_Stream *st = NULL;
            double v0 = 0.0;
            double v1 = 0.0;
            double v2 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_MACDEXT_Open(&st, g_close, g_nPoints, optInFastPeriod, optInFastMAType, optInSlowPeriod, optInSlowMAType, optInSignalPeriod, optInSignalMAType, &v0, &v1, &v2);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_MACDEXT_Update(st, g_close[it & BENCH_MASK], &v0, &v1, &v2);
                    acc += v0;
                    acc += v1;
                    acc += v2;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_MACDEXT_Peek(st, g_close[it & BENCH_MASK], &v0, &v1, &v2);
                        acc += v0;
                        acc += v1;
                        acc += v2;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_MACDEXT_Update(st, g_close[it & BENCH_MASK], &v0, &v1, &v2);
                        acc += v0;
                        acc += v1;
                        acc += v2;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_MACDEXT_Close(st);
            bench_stream_row("MACDEXT", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_MACDEXT_Close(st); }
            bench_stream_row("MACDEXT", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "MACDFIX") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        const int optInSignalPeriod = bench_opaque_int(9);
        int lb = TA_MACDFIX_Lookback(optInSignalPeriod);
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_MACDFIX(t, t, g_rt_close, optInSignalPeriod, &begIdx, &nb, g_outBuf0, g_outBuf1, g_outBuf2);
                acc += g_outBuf0[0];
                acc += g_outBuf1[0];
                acc += g_outBuf2[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_MACDFIX_Stream *st = NULL;
            double v0 = 0.0;
            double v1 = 0.0;
            double v2 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_MACDFIX_Open(&st, g_close, g_nPoints, optInSignalPeriod, &v0, &v1, &v2);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_MACDFIX_Update(st, g_close[it & BENCH_MASK], &v0, &v1, &v2);
                    acc += v0;
                    acc += v1;
                    acc += v2;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_MACDFIX_Peek(st, g_close[it & BENCH_MASK], &v0, &v1, &v2);
                        acc += v0;
                        acc += v1;
                        acc += v2;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_MACDFIX_Update(st, g_close[it & BENCH_MASK], &v0, &v1, &v2);
                        acc += v0;
                        acc += v1;
                        acc += v2;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_MACDFIX_Close(st);
            bench_stream_row("MACDFIX", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_MACDFIX_Close(st); }
            bench_stream_row("MACDFIX", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "MAMA") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        const double optInFastLimit = bench_opaque_double(0.500000000000000);
        const double optInSlowLimit = bench_opaque_double(0.050000000000000);
        int lb = TA_MAMA_Lookback(optInFastLimit, optInSlowLimit);
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_MAMA(t, t, g_rt_close, optInFastLimit, optInSlowLimit, &begIdx, &nb, g_outBuf0, g_outBuf1);
                acc += g_outBuf0[0];
                acc += g_outBuf1[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_MAMA_Stream *st = NULL;
            double v0 = 0.0;
            double v1 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_MAMA_Open(&st, g_close, g_nPoints, optInFastLimit, optInSlowLimit, &v0, &v1);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_MAMA_Update(st, g_close[it & BENCH_MASK], &v0, &v1);
                    acc += v0;
                    acc += v1;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_MAMA_Peek(st, g_close[it & BENCH_MASK], &v0, &v1);
                        acc += v0;
                        acc += v1;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_MAMA_Update(st, g_close[it & BENCH_MASK], &v0, &v1);
                        acc += v0;
                        acc += v1;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_MAMA_Close(st);
            bench_stream_row("MAMA", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_MAMA_Close(st); }
            bench_stream_row("MAMA", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "MARKETFI") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_MARKETFI_Lookback();
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_high[t] = g_high[it & BENCH_MASK];
                g_rt_low[t] = g_low[it & BENCH_MASK];
                g_rt_volume[t] = g_volume[it & BENCH_MASK];
                TA_MARKETFI(t, t, g_rt_high, g_rt_low, g_rt_volume, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_MARKETFI_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_MARKETFI_Open(&st, g_high, g_low, g_volume, g_nPoints, &v0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_MARKETFI_Update(st, g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_volume[it & BENCH_MASK], &v0);
                    acc += v0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_MARKETFI_Peek(st, g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_volume[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_MARKETFI_Update(st, g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_volume[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_MARKETFI_Close(st);
            bench_stream_row("MARKETFI", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_MARKETFI_Close(st); }
            bench_stream_row("MARKETFI", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "MASSI") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        const int optInFastPeriod = bench_opaque_int(9);
        const int optInSlowPeriod = bench_opaque_int(25);
        int lb = TA_MASSI_Lookback(optInFastPeriod, optInSlowPeriod);
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_high[t] = g_high[it & BENCH_MASK];
                g_rt_low[t] = g_low[it & BENCH_MASK];
                TA_MASSI(t, t, g_rt_high, g_rt_low, optInFastPeriod, optInSlowPeriod, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_MASSI_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_MASSI_Open(&st, g_high, g_low, g_nPoints, optInFastPeriod, optInSlowPeriod, &v0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_MASSI_Update(st, g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], &v0);
                    acc += v0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_MASSI_Peek(st, g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_MASSI_Update(st, g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_MASSI_Close(st);
            bench_stream_row("MASSI", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_MASSI_Close(st); }
            bench_stream_row("MASSI", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "MAVP") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        const int optInMinPeriod = bench_opaque_int(2);
        const int optInMaxPeriod = bench_opaque_int(30);
        const int optInMAType = bench_opaque_int(0);
        int lb = TA_MAVP_Lookback(optInMinPeriod, optInMaxPeriod, optInMAType);
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_close[t] = g_close[it & BENCH_MASK];
                g_rt_periods[t] = g_periods[it & BENCH_MASK];
                TA_MAVP(t, t, g_rt_close, g_rt_periods, optInMinPeriod, optInMaxPeriod, optInMAType, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_MAVP_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_MAVP_Open(&st, g_close, g_periods, g_nPoints, optInMinPeriod, optInMaxPeriod, optInMAType, &v0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_MAVP_Update(st, g_close[it & BENCH_MASK], g_periods[it & BENCH_MASK], &v0);
                    acc += v0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_MAVP_Peek(st, g_close[it & BENCH_MASK], g_periods[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_MAVP_Update(st, g_close[it & BENCH_MASK], g_periods[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_MAVP_Close(st);
            bench_stream_row("MAVP", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_MAVP_Close(st); }
            bench_stream_row("MAVP", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "MAX") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        const int optInTimePeriod = bench_opaque_int(g_period > 0 ? g_period : 30);
        int lb = TA_MAX_Lookback(optInTimePeriod);
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_MAX(t, t, g_rt_close, optInTimePeriod, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_MAX_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_MAX_Open(&st, g_close, g_nPoints, optInTimePeriod, &v0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_MAX_Update(st, g_close[it & BENCH_MASK], &v0);
                    acc += v0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_MAX_Peek(st, g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_MAX_Update(st, g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_MAX_Close(st);
            bench_stream_row("MAX", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_MAX_Close(st); }
            bench_stream_row("MAX", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "MAXINDEX") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        const int optInTimePeriod = bench_opaque_int(g_period > 0 ? g_period : 30);
        int lb = TA_MAXINDEX_Lookback(optInTimePeriod);
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_MAXINDEX(t, t, g_rt_close, optInTimePeriod, &begIdx, &nb, g_outIntBuf0);
                acc += (double)g_outIntBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_MAXINDEX_Stream *st = NULL;
            int iv0 = 0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_MAXINDEX_Open(&st, g_close, g_nPoints, optInTimePeriod, &iv0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_MAXINDEX_Update(st, g_close[it & BENCH_MASK], &iv0);
                    acc += (double)iv0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_MAXINDEX_Peek(st, g_close[it & BENCH_MASK], &iv0);
                        acc += (double)iv0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_MAXINDEX_Update(st, g_close[it & BENCH_MASK], &iv0);
                        acc += (double)iv0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_MAXINDEX_Close(st);
            bench_stream_row("MAXINDEX", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_MAXINDEX_Close(st); }
            bench_stream_row("MAXINDEX", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "MEDIAN") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        const int optInTimePeriod = bench_opaque_int(g_period > 0 ? g_period : 30);
        int lb = TA_MEDIAN_Lookback(optInTimePeriod);
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_MEDIAN(t, t, g_rt_close, optInTimePeriod, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_MEDIAN_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_MEDIAN_Open(&st, g_close, g_nPoints, optInTimePeriod, &v0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_MEDIAN_Update(st, g_close[it & BENCH_MASK], &v0);
                    acc += v0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_MEDIAN_Peek(st, g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_MEDIAN_Update(st, g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_MEDIAN_Close(st);
            bench_stream_row("MEDIAN", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_MEDIAN_Close(st); }
            bench_stream_row("MEDIAN", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "MEDPRICE") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_MEDPRICE_Lookback();
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_high[t] = g_high[it & BENCH_MASK];
                g_rt_low[t] = g_low[it & BENCH_MASK];
                TA_MEDPRICE(t, t, g_rt_high, g_rt_low, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_MEDPRICE_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_MEDPRICE_Open(&st, g_high, g_low, g_nPoints, &v0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_MEDPRICE_Update(st, g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], &v0);
                    acc += v0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_MEDPRICE_Peek(st, g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_MEDPRICE_Update(st, g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_MEDPRICE_Close(st);
            bench_stream_row("MEDPRICE", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_MEDPRICE_Close(st); }
            bench_stream_row("MEDPRICE", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "MFI") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        const int optInTimePeriod = bench_opaque_int(g_period > 0 ? g_period : 14);
        int lb = TA_MFI_Lookback(optInTimePeriod);
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_high[t] = g_high[it & BENCH_MASK];
                g_rt_low[t] = g_low[it & BENCH_MASK];
                g_rt_close[t] = g_close[it & BENCH_MASK];
                g_rt_volume[t] = g_volume[it & BENCH_MASK];
                TA_MFI(t, t, g_rt_high, g_rt_low, g_rt_close, g_rt_volume, optInTimePeriod, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_MFI_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_MFI_Open(&st, g_high, g_low, g_close, g_volume, g_nPoints, optInTimePeriod, &v0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_MFI_Update(st, g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], g_volume[it & BENCH_MASK], &v0);
                    acc += v0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_MFI_Peek(st, g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], g_volume[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_MFI_Update(st, g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], g_volume[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_MFI_Close(st);
            bench_stream_row("MFI", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_MFI_Close(st); }
            bench_stream_row("MFI", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "MIDPOINT") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        const int optInTimePeriod = bench_opaque_int(g_period > 0 ? g_period : 14);
        int lb = TA_MIDPOINT_Lookback(optInTimePeriod);
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_MIDPOINT(t, t, g_rt_close, optInTimePeriod, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_MIDPOINT_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_MIDPOINT_Open(&st, g_close, g_nPoints, optInTimePeriod, &v0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_MIDPOINT_Update(st, g_close[it & BENCH_MASK], &v0);
                    acc += v0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_MIDPOINT_Peek(st, g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_MIDPOINT_Update(st, g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_MIDPOINT_Close(st);
            bench_stream_row("MIDPOINT", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_MIDPOINT_Close(st); }
            bench_stream_row("MIDPOINT", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "MIDPRICE") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        const int optInTimePeriod = bench_opaque_int(g_period > 0 ? g_period : 14);
        int lb = TA_MIDPRICE_Lookback(optInTimePeriod);
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_high[t] = g_high[it & BENCH_MASK];
                g_rt_low[t] = g_low[it & BENCH_MASK];
                TA_MIDPRICE(t, t, g_rt_high, g_rt_low, optInTimePeriod, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_MIDPRICE_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_MIDPRICE_Open(&st, g_high, g_low, g_nPoints, optInTimePeriod, &v0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_MIDPRICE_Update(st, g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], &v0);
                    acc += v0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_MIDPRICE_Peek(st, g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_MIDPRICE_Update(st, g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_MIDPRICE_Close(st);
            bench_stream_row("MIDPRICE", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_MIDPRICE_Close(st); }
            bench_stream_row("MIDPRICE", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "MIN") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        const int optInTimePeriod = bench_opaque_int(g_period > 0 ? g_period : 30);
        int lb = TA_MIN_Lookback(optInTimePeriod);
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_MIN(t, t, g_rt_close, optInTimePeriod, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_MIN_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_MIN_Open(&st, g_close, g_nPoints, optInTimePeriod, &v0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_MIN_Update(st, g_close[it & BENCH_MASK], &v0);
                    acc += v0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_MIN_Peek(st, g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_MIN_Update(st, g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_MIN_Close(st);
            bench_stream_row("MIN", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_MIN_Close(st); }
            bench_stream_row("MIN", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "MININDEX") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        const int optInTimePeriod = bench_opaque_int(g_period > 0 ? g_period : 30);
        int lb = TA_MININDEX_Lookback(optInTimePeriod);
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_MININDEX(t, t, g_rt_close, optInTimePeriod, &begIdx, &nb, g_outIntBuf0);
                acc += (double)g_outIntBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_MININDEX_Stream *st = NULL;
            int iv0 = 0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_MININDEX_Open(&st, g_close, g_nPoints, optInTimePeriod, &iv0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_MININDEX_Update(st, g_close[it & BENCH_MASK], &iv0);
                    acc += (double)iv0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_MININDEX_Peek(st, g_close[it & BENCH_MASK], &iv0);
                        acc += (double)iv0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_MININDEX_Update(st, g_close[it & BENCH_MASK], &iv0);
                        acc += (double)iv0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_MININDEX_Close(st);
            bench_stream_row("MININDEX", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_MININDEX_Close(st); }
            bench_stream_row("MININDEX", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "MINMAX") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        const int optInTimePeriod = bench_opaque_int(g_period > 0 ? g_period : 30);
        int lb = TA_MINMAX_Lookback(optInTimePeriod);
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_MINMAX(t, t, g_rt_close, optInTimePeriod, &begIdx, &nb, g_outBuf0, g_outBuf1);
                acc += g_outBuf0[0];
                acc += g_outBuf1[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_MINMAX_Stream *st = NULL;
            double v0 = 0.0;
            double v1 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_MINMAX_Open(&st, g_close, g_nPoints, optInTimePeriod, &v0, &v1);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_MINMAX_Update(st, g_close[it & BENCH_MASK], &v0, &v1);
                    acc += v0;
                    acc += v1;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_MINMAX_Peek(st, g_close[it & BENCH_MASK], &v0, &v1);
                        acc += v0;
                        acc += v1;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_MINMAX_Update(st, g_close[it & BENCH_MASK], &v0, &v1);
                        acc += v0;
                        acc += v1;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_MINMAX_Close(st);
            bench_stream_row("MINMAX", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_MINMAX_Close(st); }
            bench_stream_row("MINMAX", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "MINMAXINDEX") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        const int optInTimePeriod = bench_opaque_int(g_period > 0 ? g_period : 30);
        int lb = TA_MINMAXINDEX_Lookback(optInTimePeriod);
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_MINMAXINDEX(t, t, g_rt_close, optInTimePeriod, &begIdx, &nb, g_outIntBuf0, g_outIntBuf1);
                acc += (double)g_outIntBuf0[0];
                acc += (double)g_outIntBuf1[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_MINMAXINDEX_Stream *st = NULL;
            int iv0 = 0;
            int iv1 = 0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_MINMAXINDEX_Open(&st, g_close, g_nPoints, optInTimePeriod, &iv0, &iv1);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_MINMAXINDEX_Update(st, g_close[it & BENCH_MASK], &iv0, &iv1);
                    acc += (double)iv0;
                    acc += (double)iv1;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_MINMAXINDEX_Peek(st, g_close[it & BENCH_MASK], &iv0, &iv1);
                        acc += (double)iv0;
                        acc += (double)iv1;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_MINMAXINDEX_Update(st, g_close[it & BENCH_MASK], &iv0, &iv1);
                        acc += (double)iv0;
                        acc += (double)iv1;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_MINMAXINDEX_Close(st);
            bench_stream_row("MINMAXINDEX", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_MINMAXINDEX_Close(st); }
            bench_stream_row("MINMAXINDEX", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "MINUS_DI") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        const int optInTimePeriod = bench_opaque_int(g_period > 0 ? g_period : 14);
        int lb = TA_MINUS_DI_Lookback(optInTimePeriod);
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_high[t] = g_high[it & BENCH_MASK];
                g_rt_low[t] = g_low[it & BENCH_MASK];
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_MINUS_DI(t, t, g_rt_high, g_rt_low, g_rt_close, optInTimePeriod, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_MINUS_DI_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_MINUS_DI_Open(&st, g_high, g_low, g_close, g_nPoints, optInTimePeriod, &v0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_MINUS_DI_Update(st, g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &v0);
                    acc += v0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_MINUS_DI_Peek(st, g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_MINUS_DI_Update(st, g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_MINUS_DI_Close(st);
            bench_stream_row("MINUS_DI", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_MINUS_DI_Close(st); }
            bench_stream_row("MINUS_DI", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "MINUS_DM") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        const int optInTimePeriod = bench_opaque_int(g_period > 0 ? g_period : 14);
        int lb = TA_MINUS_DM_Lookback(optInTimePeriod);
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_high[t] = g_high[it & BENCH_MASK];
                g_rt_low[t] = g_low[it & BENCH_MASK];
                TA_MINUS_DM(t, t, g_rt_high, g_rt_low, optInTimePeriod, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_MINUS_DM_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_MINUS_DM_Open(&st, g_high, g_low, g_nPoints, optInTimePeriod, &v0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_MINUS_DM_Update(st, g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], &v0);
                    acc += v0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_MINUS_DM_Peek(st, g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_MINUS_DM_Update(st, g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_MINUS_DM_Close(st);
            bench_stream_row("MINUS_DM", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_MINUS_DM_Close(st); }
            bench_stream_row("MINUS_DM", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "MOM") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        const int optInTimePeriod = bench_opaque_int(g_period > 0 ? g_period : 10);
        int lb = TA_MOM_Lookback(optInTimePeriod);
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_MOM(t, t, g_rt_close, optInTimePeriod, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_MOM_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_MOM_Open(&st, g_close, g_nPoints, optInTimePeriod, &v0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_MOM_Update(st, g_close[it & BENCH_MASK], &v0);
                    acc += v0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_MOM_Peek(st, g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_MOM_Update(st, g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_MOM_Close(st);
            bench_stream_row("MOM", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_MOM_Close(st); }
            bench_stream_row("MOM", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "MULT") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_MULT_Lookback();
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_close[t] = g_close[it & BENCH_MASK];
                g_rt_high[t] = g_high[it & BENCH_MASK];
                TA_MULT(t, t, g_rt_close, g_rt_high, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_MULT_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_MULT_Open(&st, g_close, g_high, g_nPoints, &v0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_MULT_Update(st, g_close[it & BENCH_MASK], g_high[it & BENCH_MASK], &v0);
                    acc += v0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_MULT_Peek(st, g_close[it & BENCH_MASK], g_high[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_MULT_Update(st, g_close[it & BENCH_MASK], g_high[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_MULT_Close(st);
            bench_stream_row("MULT", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_MULT_Close(st); }
            bench_stream_row("MULT", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "NATR") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        const int optInTimePeriod = bench_opaque_int(g_period > 0 ? g_period : 14);
        int lb = TA_NATR_Lookback(optInTimePeriod);
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_high[t] = g_high[it & BENCH_MASK];
                g_rt_low[t] = g_low[it & BENCH_MASK];
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_NATR(t, t, g_rt_high, g_rt_low, g_rt_close, optInTimePeriod, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_NATR_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_NATR_Open(&st, g_high, g_low, g_close, g_nPoints, optInTimePeriod, &v0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_NATR_Update(st, g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &v0);
                    acc += v0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_NATR_Peek(st, g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_NATR_Update(st, g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_NATR_Close(st);
            bench_stream_row("NATR", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_NATR_Close(st); }
            bench_stream_row("NATR", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "NVI") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_NVI_Lookback();
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_close[t] = g_close[it & BENCH_MASK];
                g_rt_volume[t] = g_volume[it & BENCH_MASK];
                TA_NVI(t, t, g_rt_close, g_rt_volume, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_NVI_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_NVI_Open(&st, g_close, g_volume, g_nPoints, &v0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_NVI_Update(st, g_close[it & BENCH_MASK], g_volume[it & BENCH_MASK], &v0);
                    acc += v0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_NVI_Peek(st, g_close[it & BENCH_MASK], g_volume[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_NVI_Update(st, g_close[it & BENCH_MASK], g_volume[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_NVI_Close(st);
            bench_stream_row("NVI", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_NVI_Close(st); }
            bench_stream_row("NVI", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "OBV") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_OBV_Lookback();
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_close[t] = g_close[it & BENCH_MASK];
                g_rt_volume[t] = g_volume[it & BENCH_MASK];
                TA_OBV(t, t, g_rt_close, g_rt_volume, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_OBV_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_OBV_Open(&st, g_close, g_volume, g_nPoints, &v0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_OBV_Update(st, g_close[it & BENCH_MASK], g_volume[it & BENCH_MASK], &v0);
                    acc += v0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_OBV_Peek(st, g_close[it & BENCH_MASK], g_volume[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_OBV_Update(st, g_close[it & BENCH_MASK], g_volume[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_OBV_Close(st);
            bench_stream_row("OBV", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_OBV_Close(st); }
            bench_stream_row("OBV", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "PERCENTB") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        const int optInTimePeriod = bench_opaque_int(g_period > 0 ? g_period : 20);
        const double optInNbDevUp = bench_opaque_double(2.000000000000000);
        const double optInNbDevDn = bench_opaque_double(2.000000000000000);
        const int optInMAType = bench_opaque_int(0);
        int lb = TA_PERCENTB_Lookback(optInTimePeriod, optInNbDevUp, optInNbDevDn, optInMAType);
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_PERCENTB(t, t, g_rt_close, optInTimePeriod, optInNbDevUp, optInNbDevDn, optInMAType, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_PERCENTB_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_PERCENTB_Open(&st, g_close, g_nPoints, optInTimePeriod, optInNbDevUp, optInNbDevDn, optInMAType, &v0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_PERCENTB_Update(st, g_close[it & BENCH_MASK], &v0);
                    acc += v0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_PERCENTB_Peek(st, g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_PERCENTB_Update(st, g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_PERCENTB_Close(st);
            bench_stream_row("PERCENTB", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_PERCENTB_Close(st); }
            bench_stream_row("PERCENTB", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "PERCENTILE") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        const int optInTimePeriod = bench_opaque_int(g_period > 0 ? g_period : 100);
        const double optInPercentile = bench_opaque_double(50.000000000000000);
        int lb = TA_PERCENTILE_Lookback(optInTimePeriod, optInPercentile);
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_PERCENTILE(t, t, g_rt_close, optInTimePeriod, optInPercentile, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_PERCENTILE_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_PERCENTILE_Open(&st, g_close, g_nPoints, optInTimePeriod, optInPercentile, &v0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_PERCENTILE_Update(st, g_close[it & BENCH_MASK], &v0);
                    acc += v0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_PERCENTILE_Peek(st, g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_PERCENTILE_Update(st, g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_PERCENTILE_Close(st);
            bench_stream_row("PERCENTILE", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_PERCENTILE_Close(st); }
            bench_stream_row("PERCENTILE", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "PERCENTRANK") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        const int optInTimePeriod = bench_opaque_int(g_period > 0 ? g_period : 100);
        int lb = TA_PERCENTRANK_Lookback(optInTimePeriod);
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_PERCENTRANK(t, t, g_rt_close, optInTimePeriod, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_PERCENTRANK_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_PERCENTRANK_Open(&st, g_close, g_nPoints, optInTimePeriod, &v0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_PERCENTRANK_Update(st, g_close[it & BENCH_MASK], &v0);
                    acc += v0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_PERCENTRANK_Peek(st, g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_PERCENTRANK_Update(st, g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_PERCENTRANK_Close(st);
            bench_stream_row("PERCENTRANK", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_PERCENTRANK_Close(st); }
            bench_stream_row("PERCENTRANK", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "PLUS_DI") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        const int optInTimePeriod = bench_opaque_int(g_period > 0 ? g_period : 14);
        int lb = TA_PLUS_DI_Lookback(optInTimePeriod);
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_high[t] = g_high[it & BENCH_MASK];
                g_rt_low[t] = g_low[it & BENCH_MASK];
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_PLUS_DI(t, t, g_rt_high, g_rt_low, g_rt_close, optInTimePeriod, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_PLUS_DI_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_PLUS_DI_Open(&st, g_high, g_low, g_close, g_nPoints, optInTimePeriod, &v0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_PLUS_DI_Update(st, g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &v0);
                    acc += v0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_PLUS_DI_Peek(st, g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_PLUS_DI_Update(st, g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_PLUS_DI_Close(st);
            bench_stream_row("PLUS_DI", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_PLUS_DI_Close(st); }
            bench_stream_row("PLUS_DI", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "PLUS_DM") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        const int optInTimePeriod = bench_opaque_int(g_period > 0 ? g_period : 14);
        int lb = TA_PLUS_DM_Lookback(optInTimePeriod);
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_high[t] = g_high[it & BENCH_MASK];
                g_rt_low[t] = g_low[it & BENCH_MASK];
                TA_PLUS_DM(t, t, g_rt_high, g_rt_low, optInTimePeriod, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_PLUS_DM_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_PLUS_DM_Open(&st, g_high, g_low, g_nPoints, optInTimePeriod, &v0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_PLUS_DM_Update(st, g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], &v0);
                    acc += v0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_PLUS_DM_Peek(st, g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_PLUS_DM_Update(st, g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_PLUS_DM_Close(st);
            bench_stream_row("PLUS_DM", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_PLUS_DM_Close(st); }
            bench_stream_row("PLUS_DM", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "PPO") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        const int optInFastPeriod = bench_opaque_int(12);
        const int optInSlowPeriod = bench_opaque_int(26);
        const int optInMAType = bench_opaque_int(1);
        int lb = TA_PPO_Lookback(optInFastPeriod, optInSlowPeriod, optInMAType);
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_PPO(t, t, g_rt_close, optInFastPeriod, optInSlowPeriod, optInMAType, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_PPO_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_PPO_Open(&st, g_close, g_nPoints, optInFastPeriod, optInSlowPeriod, optInMAType, &v0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_PPO_Update(st, g_close[it & BENCH_MASK], &v0);
                    acc += v0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_PPO_Peek(st, g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_PPO_Update(st, g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_PPO_Close(st);
            bench_stream_row("PPO", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_PPO_Close(st); }
            bench_stream_row("PPO", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "PVI") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_PVI_Lookback();
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_close[t] = g_close[it & BENCH_MASK];
                g_rt_volume[t] = g_volume[it & BENCH_MASK];
                TA_PVI(t, t, g_rt_close, g_rt_volume, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_PVI_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_PVI_Open(&st, g_close, g_volume, g_nPoints, &v0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_PVI_Update(st, g_close[it & BENCH_MASK], g_volume[it & BENCH_MASK], &v0);
                    acc += v0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_PVI_Peek(st, g_close[it & BENCH_MASK], g_volume[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_PVI_Update(st, g_close[it & BENCH_MASK], g_volume[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_PVI_Close(st);
            bench_stream_row("PVI", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_PVI_Close(st); }
            bench_stream_row("PVI", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "PVO") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        const int optInFastPeriod = bench_opaque_int(12);
        const int optInSlowPeriod = bench_opaque_int(26);
        const int optInMAType = bench_opaque_int(1);
        int lb = TA_PVO_Lookback(optInFastPeriod, optInSlowPeriod, optInMAType);
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_volume[t] = g_volume[it & BENCH_MASK];
                TA_PVO(t, t, g_rt_volume, optInFastPeriod, optInSlowPeriod, optInMAType, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_PVO_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_PVO_Open(&st, g_volume, g_nPoints, optInFastPeriod, optInSlowPeriod, optInMAType, &v0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_PVO_Update(st, g_volume[it & BENCH_MASK], &v0);
                    acc += v0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_PVO_Peek(st, g_volume[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_PVO_Update(st, g_volume[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_PVO_Close(st);
            bench_stream_row("PVO", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_PVO_Close(st); }
            bench_stream_row("PVO", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "PVT") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_PVT_Lookback();
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_close[t] = g_close[it & BENCH_MASK];
                g_rt_volume[t] = g_volume[it & BENCH_MASK];
                TA_PVT(t, t, g_rt_close, g_rt_volume, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_PVT_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_PVT_Open(&st, g_close, g_volume, g_nPoints, &v0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_PVT_Update(st, g_close[it & BENCH_MASK], g_volume[it & BENCH_MASK], &v0);
                    acc += v0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_PVT_Peek(st, g_close[it & BENCH_MASK], g_volume[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_PVT_Update(st, g_close[it & BENCH_MASK], g_volume[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_PVT_Close(st);
            bench_stream_row("PVT", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_PVT_Close(st); }
            bench_stream_row("PVT", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "QSTICK") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        const int optInTimePeriod = bench_opaque_int(g_period > 0 ? g_period : 10);
        int lb = TA_QSTICK_Lookback(optInTimePeriod);
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_open[t] = g_open[it & BENCH_MASK];
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_QSTICK(t, t, g_rt_open, g_rt_close, optInTimePeriod, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_QSTICK_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_QSTICK_Open(&st, g_open, g_close, g_nPoints, optInTimePeriod, &v0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_QSTICK_Update(st, g_open[it & BENCH_MASK], g_close[it & BENCH_MASK], &v0);
                    acc += v0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_QSTICK_Peek(st, g_open[it & BENCH_MASK], g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_QSTICK_Update(st, g_open[it & BENCH_MASK], g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_QSTICK_Close(st);
            bench_stream_row("QSTICK", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_QSTICK_Close(st); }
            bench_stream_row("QSTICK", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "RMA") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        const int optInTimePeriod = bench_opaque_int(g_period > 0 ? g_period : 30);
        int lb = TA_RMA_Lookback(optInTimePeriod);
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_RMA(t, t, g_rt_close, optInTimePeriod, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_RMA_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_RMA_Open(&st, g_close, g_nPoints, optInTimePeriod, &v0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_RMA_Update(st, g_close[it & BENCH_MASK], &v0);
                    acc += v0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_RMA_Peek(st, g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_RMA_Update(st, g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_RMA_Close(st);
            bench_stream_row("RMA", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_RMA_Close(st); }
            bench_stream_row("RMA", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "ROC") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        const int optInTimePeriod = bench_opaque_int(g_period > 0 ? g_period : 10);
        int lb = TA_ROC_Lookback(optInTimePeriod);
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_ROC(t, t, g_rt_close, optInTimePeriod, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_ROC_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_ROC_Open(&st, g_close, g_nPoints, optInTimePeriod, &v0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_ROC_Update(st, g_close[it & BENCH_MASK], &v0);
                    acc += v0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_ROC_Peek(st, g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_ROC_Update(st, g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_ROC_Close(st);
            bench_stream_row("ROC", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_ROC_Close(st); }
            bench_stream_row("ROC", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "ROCP") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        const int optInTimePeriod = bench_opaque_int(g_period > 0 ? g_period : 10);
        int lb = TA_ROCP_Lookback(optInTimePeriod);
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_ROCP(t, t, g_rt_close, optInTimePeriod, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_ROCP_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_ROCP_Open(&st, g_close, g_nPoints, optInTimePeriod, &v0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_ROCP_Update(st, g_close[it & BENCH_MASK], &v0);
                    acc += v0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_ROCP_Peek(st, g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_ROCP_Update(st, g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_ROCP_Close(st);
            bench_stream_row("ROCP", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_ROCP_Close(st); }
            bench_stream_row("ROCP", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "ROCR") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        const int optInTimePeriod = bench_opaque_int(g_period > 0 ? g_period : 10);
        int lb = TA_ROCR_Lookback(optInTimePeriod);
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_ROCR(t, t, g_rt_close, optInTimePeriod, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_ROCR_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_ROCR_Open(&st, g_close, g_nPoints, optInTimePeriod, &v0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_ROCR_Update(st, g_close[it & BENCH_MASK], &v0);
                    acc += v0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_ROCR_Peek(st, g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_ROCR_Update(st, g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_ROCR_Close(st);
            bench_stream_row("ROCR", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_ROCR_Close(st); }
            bench_stream_row("ROCR", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "ROCR100") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        const int optInTimePeriod = bench_opaque_int(g_period > 0 ? g_period : 10);
        int lb = TA_ROCR100_Lookback(optInTimePeriod);
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_ROCR100(t, t, g_rt_close, optInTimePeriod, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_ROCR100_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_ROCR100_Open(&st, g_close, g_nPoints, optInTimePeriod, &v0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_ROCR100_Update(st, g_close[it & BENCH_MASK], &v0);
                    acc += v0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_ROCR100_Peek(st, g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_ROCR100_Update(st, g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_ROCR100_Close(st);
            bench_stream_row("ROCR100", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_ROCR100_Close(st); }
            bench_stream_row("ROCR100", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "RSI") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        const int optInTimePeriod = bench_opaque_int(g_period > 0 ? g_period : 14);
        int lb = TA_RSI_Lookback(optInTimePeriod);
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_RSI(t, t, g_rt_close, optInTimePeriod, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_RSI_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_RSI_Open(&st, g_close, g_nPoints, optInTimePeriod, &v0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_RSI_Update(st, g_close[it & BENCH_MASK], &v0);
                    acc += v0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_RSI_Peek(st, g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_RSI_Update(st, g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_RSI_Close(st);
            bench_stream_row("RSI", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_RSI_Close(st); }
            bench_stream_row("RSI", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "RVI") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        const int optInTimePeriod = bench_opaque_int(g_period > 0 ? g_period : 14);
        const int optInStdDevPeriod = bench_opaque_int(10);
        int lb = TA_RVI_Lookback(optInTimePeriod, optInStdDevPeriod);
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_RVI(t, t, g_rt_close, optInTimePeriod, optInStdDevPeriod, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_RVI_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_RVI_Open(&st, g_close, g_nPoints, optInTimePeriod, optInStdDevPeriod, &v0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_RVI_Update(st, g_close[it & BENCH_MASK], &v0);
                    acc += v0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_RVI_Peek(st, g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_RVI_Update(st, g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_RVI_Close(st);
            bench_stream_row("RVI", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_RVI_Close(st); }
            bench_stream_row("RVI", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "RVIR") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        const int optInTimePeriod = bench_opaque_int(g_period > 0 ? g_period : 14);
        const int optInStdDevPeriod = bench_opaque_int(10);
        int lb = TA_RVIR_Lookback(optInTimePeriod, optInStdDevPeriod);
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_high[t] = g_high[it & BENCH_MASK];
                g_rt_low[t] = g_low[it & BENCH_MASK];
                TA_RVIR(t, t, g_rt_high, g_rt_low, optInTimePeriod, optInStdDevPeriod, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_RVIR_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_RVIR_Open(&st, g_high, g_low, g_nPoints, optInTimePeriod, optInStdDevPeriod, &v0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_RVIR_Update(st, g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], &v0);
                    acc += v0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_RVIR_Peek(st, g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_RVIR_Update(st, g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_RVIR_Close(st);
            bench_stream_row("RVIR", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_RVIR_Close(st); }
            bench_stream_row("RVIR", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "RVOL") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        const int optInTimePeriod = bench_opaque_int(g_period > 0 ? g_period : 20);
        int lb = TA_RVOL_Lookback(optInTimePeriod);
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_volume[t] = g_volume[it & BENCH_MASK];
                TA_RVOL(t, t, g_rt_volume, optInTimePeriod, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_RVOL_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_RVOL_Open(&st, g_volume, g_nPoints, optInTimePeriod, &v0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_RVOL_Update(st, g_volume[it & BENCH_MASK], &v0);
                    acc += v0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_RVOL_Peek(st, g_volume[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_RVOL_Update(st, g_volume[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_RVOL_Close(st);
            bench_stream_row("RVOL", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_RVOL_Close(st); }
            bench_stream_row("RVOL", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "SAR") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        const double optInAcceleration = bench_opaque_double(0.020000000000000);
        const double optInMaximum = bench_opaque_double(0.200000000000000);
        int lb = TA_SAR_Lookback(optInAcceleration, optInMaximum);
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_high[t] = g_high[it & BENCH_MASK];
                g_rt_low[t] = g_low[it & BENCH_MASK];
                TA_SAR(t, t, g_rt_high, g_rt_low, optInAcceleration, optInMaximum, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_SAR_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_SAR_Open(&st, g_high, g_low, g_nPoints, optInAcceleration, optInMaximum, &v0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_SAR_Update(st, g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], &v0);
                    acc += v0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_SAR_Peek(st, g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_SAR_Update(st, g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_SAR_Close(st);
            bench_stream_row("SAR", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_SAR_Close(st); }
            bench_stream_row("SAR", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "SAREXT") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        const double optInStartValue = bench_opaque_double(0.000000000000000);
        const double optInOffsetOnReverse = bench_opaque_double(0.000000000000000);
        const double optInAccelerationInitLong = bench_opaque_double(0.020000000000000);
        const double optInAccelerationLong = bench_opaque_double(0.020000000000000);
        const double optInAccelerationMaxLong = bench_opaque_double(0.200000000000000);
        const double optInAccelerationInitShort = bench_opaque_double(0.020000000000000);
        const double optInAccelerationShort = bench_opaque_double(0.020000000000000);
        const double optInAccelerationMaxShort = bench_opaque_double(0.200000000000000);
        int lb = TA_SAREXT_Lookback(optInStartValue, optInOffsetOnReverse, optInAccelerationInitLong, optInAccelerationLong, optInAccelerationMaxLong, optInAccelerationInitShort, optInAccelerationShort, optInAccelerationMaxShort);
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_high[t] = g_high[it & BENCH_MASK];
                g_rt_low[t] = g_low[it & BENCH_MASK];
                TA_SAREXT(t, t, g_rt_high, g_rt_low, optInStartValue, optInOffsetOnReverse, optInAccelerationInitLong, optInAccelerationLong, optInAccelerationMaxLong, optInAccelerationInitShort, optInAccelerationShort, optInAccelerationMaxShort, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_SAREXT_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_SAREXT_Open(&st, g_high, g_low, g_nPoints, optInStartValue, optInOffsetOnReverse, optInAccelerationInitLong, optInAccelerationLong, optInAccelerationMaxLong, optInAccelerationInitShort, optInAccelerationShort, optInAccelerationMaxShort, &v0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_SAREXT_Update(st, g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], &v0);
                    acc += v0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_SAREXT_Peek(st, g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_SAREXT_Update(st, g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_SAREXT_Close(st);
            bench_stream_row("SAREXT", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_SAREXT_Close(st); }
            bench_stream_row("SAREXT", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "SIN") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_SIN_Lookback();
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_SIN(t, t, g_rt_close, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_SIN_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_SIN_Open(&st, g_close, g_nPoints, &v0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_SIN_Update(st, g_close[it & BENCH_MASK], &v0);
                    acc += v0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_SIN_Peek(st, g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_SIN_Update(st, g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_SIN_Close(st);
            bench_stream_row("SIN", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_SIN_Close(st); }
            bench_stream_row("SIN", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "SINH") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_SINH_Lookback();
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_SINH(t, t, g_rt_close, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_SINH_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_SINH_Open(&st, g_close, g_nPoints, &v0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_SINH_Update(st, g_close[it & BENCH_MASK], &v0);
                    acc += v0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_SINH_Peek(st, g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_SINH_Update(st, g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_SINH_Close(st);
            bench_stream_row("SINH", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_SINH_Close(st); }
            bench_stream_row("SINH", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "SMA") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        const int optInTimePeriod = bench_opaque_int(g_period > 0 ? g_period : 30);
        int lb = TA_SMA_Lookback(optInTimePeriod);
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_SMA(t, t, g_rt_close, optInTimePeriod, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_SMA_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_SMA_Open(&st, g_close, g_nPoints, optInTimePeriod, &v0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_SMA_Update(st, g_close[it & BENCH_MASK], &v0);
                    acc += v0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_SMA_Peek(st, g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_SMA_Update(st, g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_SMA_Close(st);
            bench_stream_row("SMA", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_SMA_Close(st); }
            bench_stream_row("SMA", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "SMI") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        const int optInTimePeriod = bench_opaque_int(g_period > 0 ? g_period : 13);
        const int optInFastPeriod = bench_opaque_int(2);
        const int optInSlowPeriod = bench_opaque_int(25);
        const int optInSignalPeriod = bench_opaque_int(9);
        int lb = TA_SMI_Lookback(optInTimePeriod, optInFastPeriod, optInSlowPeriod, optInSignalPeriod);
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_high[t] = g_high[it & BENCH_MASK];
                g_rt_low[t] = g_low[it & BENCH_MASK];
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_SMI(t, t, g_rt_high, g_rt_low, g_rt_close, optInTimePeriod, optInFastPeriod, optInSlowPeriod, optInSignalPeriod, &begIdx, &nb, g_outBuf0, g_outBuf1);
                acc += g_outBuf0[0];
                acc += g_outBuf1[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_SMI_Stream *st = NULL;
            double v0 = 0.0;
            double v1 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_SMI_Open(&st, g_high, g_low, g_close, g_nPoints, optInTimePeriod, optInFastPeriod, optInSlowPeriod, optInSignalPeriod, &v0, &v1);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_SMI_Update(st, g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &v0, &v1);
                    acc += v0;
                    acc += v1;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_SMI_Peek(st, g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &v0, &v1);
                        acc += v0;
                        acc += v1;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_SMI_Update(st, g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &v0, &v1);
                        acc += v0;
                        acc += v1;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_SMI_Close(st);
            bench_stream_row("SMI", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_SMI_Close(st); }
            bench_stream_row("SMI", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "SQRT") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_SQRT_Lookback();
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_SQRT(t, t, g_rt_close, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_SQRT_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_SQRT_Open(&st, g_close, g_nPoints, &v0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_SQRT_Update(st, g_close[it & BENCH_MASK], &v0);
                    acc += v0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_SQRT_Peek(st, g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_SQRT_Update(st, g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_SQRT_Close(st);
            bench_stream_row("SQRT", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_SQRT_Close(st); }
            bench_stream_row("SQRT", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "STDDEV") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        const int optInTimePeriod = bench_opaque_int(g_period > 0 ? g_period : 5);
        const double optInNbDev = bench_opaque_double(1.000000000000000);
        int lb = TA_STDDEV_Lookback(optInTimePeriod, optInNbDev);
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_STDDEV(t, t, g_rt_close, optInTimePeriod, optInNbDev, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_STDDEV_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_STDDEV_Open(&st, g_close, g_nPoints, optInTimePeriod, optInNbDev, &v0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_STDDEV_Update(st, g_close[it & BENCH_MASK], &v0);
                    acc += v0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_STDDEV_Peek(st, g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_STDDEV_Update(st, g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_STDDEV_Close(st);
            bench_stream_row("STDDEV", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_STDDEV_Close(st); }
            bench_stream_row("STDDEV", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "STOCH") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        const int optInFastK_Period = bench_opaque_int(5);
        const int optInSlowK_Period = bench_opaque_int(3);
        const int optInSlowK_MAType = bench_opaque_int(0);
        const int optInSlowD_Period = bench_opaque_int(3);
        const int optInSlowD_MAType = bench_opaque_int(0);
        int lb = TA_STOCH_Lookback(optInFastK_Period, optInSlowK_Period, optInSlowK_MAType, optInSlowD_Period, optInSlowD_MAType);
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_high[t] = g_high[it & BENCH_MASK];
                g_rt_low[t] = g_low[it & BENCH_MASK];
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_STOCH(t, t, g_rt_high, g_rt_low, g_rt_close, optInFastK_Period, optInSlowK_Period, optInSlowK_MAType, optInSlowD_Period, optInSlowD_MAType, &begIdx, &nb, g_outBuf0, g_outBuf1);
                acc += g_outBuf0[0];
                acc += g_outBuf1[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_STOCH_Stream *st = NULL;
            double v0 = 0.0;
            double v1 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_STOCH_Open(&st, g_high, g_low, g_close, g_nPoints, optInFastK_Period, optInSlowK_Period, optInSlowK_MAType, optInSlowD_Period, optInSlowD_MAType, &v0, &v1);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_STOCH_Update(st, g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &v0, &v1);
                    acc += v0;
                    acc += v1;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_STOCH_Peek(st, g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &v0, &v1);
                        acc += v0;
                        acc += v1;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_STOCH_Update(st, g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &v0, &v1);
                        acc += v0;
                        acc += v1;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_STOCH_Close(st);
            bench_stream_row("STOCH", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_STOCH_Close(st); }
            bench_stream_row("STOCH", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "STOCHF") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        const int optInFastK_Period = bench_opaque_int(5);
        const int optInFastD_Period = bench_opaque_int(3);
        const int optInFastD_MAType = bench_opaque_int(0);
        int lb = TA_STOCHF_Lookback(optInFastK_Period, optInFastD_Period, optInFastD_MAType);
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_high[t] = g_high[it & BENCH_MASK];
                g_rt_low[t] = g_low[it & BENCH_MASK];
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_STOCHF(t, t, g_rt_high, g_rt_low, g_rt_close, optInFastK_Period, optInFastD_Period, optInFastD_MAType, &begIdx, &nb, g_outBuf0, g_outBuf1);
                acc += g_outBuf0[0];
                acc += g_outBuf1[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_STOCHF_Stream *st = NULL;
            double v0 = 0.0;
            double v1 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_STOCHF_Open(&st, g_high, g_low, g_close, g_nPoints, optInFastK_Period, optInFastD_Period, optInFastD_MAType, &v0, &v1);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_STOCHF_Update(st, g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &v0, &v1);
                    acc += v0;
                    acc += v1;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_STOCHF_Peek(st, g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &v0, &v1);
                        acc += v0;
                        acc += v1;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_STOCHF_Update(st, g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &v0, &v1);
                        acc += v0;
                        acc += v1;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_STOCHF_Close(st);
            bench_stream_row("STOCHF", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_STOCHF_Close(st); }
            bench_stream_row("STOCHF", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "STOCHRSI") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        const int optInTimePeriod = bench_opaque_int(g_period > 0 ? g_period : 14);
        const int optInFastK_Period = bench_opaque_int(5);
        const int optInFastD_Period = bench_opaque_int(3);
        const int optInFastD_MAType = bench_opaque_int(0);
        int lb = TA_STOCHRSI_Lookback(optInTimePeriod, optInFastK_Period, optInFastD_Period, optInFastD_MAType);
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_STOCHRSI(t, t, g_rt_close, optInTimePeriod, optInFastK_Period, optInFastD_Period, optInFastD_MAType, &begIdx, &nb, g_outBuf0, g_outBuf1);
                acc += g_outBuf0[0];
                acc += g_outBuf1[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_STOCHRSI_Stream *st = NULL;
            double v0 = 0.0;
            double v1 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_STOCHRSI_Open(&st, g_close, g_nPoints, optInTimePeriod, optInFastK_Period, optInFastD_Period, optInFastD_MAType, &v0, &v1);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_STOCHRSI_Update(st, g_close[it & BENCH_MASK], &v0, &v1);
                    acc += v0;
                    acc += v1;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_STOCHRSI_Peek(st, g_close[it & BENCH_MASK], &v0, &v1);
                        acc += v0;
                        acc += v1;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_STOCHRSI_Update(st, g_close[it & BENCH_MASK], &v0, &v1);
                        acc += v0;
                        acc += v1;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_STOCHRSI_Close(st);
            bench_stream_row("STOCHRSI", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_STOCHRSI_Close(st); }
            bench_stream_row("STOCHRSI", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "SUB") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_SUB_Lookback();
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_close[t] = g_close[it & BENCH_MASK];
                g_rt_high[t] = g_high[it & BENCH_MASK];
                TA_SUB(t, t, g_rt_close, g_rt_high, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_SUB_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_SUB_Open(&st, g_close, g_high, g_nPoints, &v0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_SUB_Update(st, g_close[it & BENCH_MASK], g_high[it & BENCH_MASK], &v0);
                    acc += v0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_SUB_Peek(st, g_close[it & BENCH_MASK], g_high[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_SUB_Update(st, g_close[it & BENCH_MASK], g_high[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_SUB_Close(st);
            bench_stream_row("SUB", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_SUB_Close(st); }
            bench_stream_row("SUB", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "SUM") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        const int optInTimePeriod = bench_opaque_int(g_period > 0 ? g_period : 30);
        int lb = TA_SUM_Lookback(optInTimePeriod);
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_SUM(t, t, g_rt_close, optInTimePeriod, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_SUM_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_SUM_Open(&st, g_close, g_nPoints, optInTimePeriod, &v0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_SUM_Update(st, g_close[it & BENCH_MASK], &v0);
                    acc += v0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_SUM_Peek(st, g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_SUM_Update(st, g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_SUM_Close(st);
            bench_stream_row("SUM", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_SUM_Close(st); }
            bench_stream_row("SUM", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "SUPERTREND") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        const int optInTimePeriod = bench_opaque_int(g_period > 0 ? g_period : 10);
        const double optInMultiplier = bench_opaque_double(3.000000000000000);
        int lb = TA_SUPERTREND_Lookback(optInTimePeriod, optInMultiplier);
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_high[t] = g_high[it & BENCH_MASK];
                g_rt_low[t] = g_low[it & BENCH_MASK];
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_SUPERTREND(t, t, g_rt_high, g_rt_low, g_rt_close, optInTimePeriod, optInMultiplier, &begIdx, &nb, g_outBuf0, g_outIntBuf0);
                acc += g_outBuf0[0];
                acc += (double)g_outIntBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_SUPERTREND_Stream *st = NULL;
            double v0 = 0.0;
            int iv0 = 0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_SUPERTREND_Open(&st, g_high, g_low, g_close, g_nPoints, optInTimePeriod, optInMultiplier, &v0, &iv0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_SUPERTREND_Update(st, g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &v0, &iv0);
                    acc += v0;
                    acc += (double)iv0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_SUPERTREND_Peek(st, g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &v0, &iv0);
                        acc += v0;
                        acc += (double)iv0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_SUPERTREND_Update(st, g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &v0, &iv0);
                        acc += v0;
                        acc += (double)iv0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_SUPERTREND_Close(st);
            bench_stream_row("SUPERTREND", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_SUPERTREND_Close(st); }
            bench_stream_row("SUPERTREND", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "T3") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        const int optInTimePeriod = bench_opaque_int(g_period > 0 ? g_period : 5);
        const double optInVFactor = bench_opaque_double(0.700000000000000);
        int lb = TA_T3_Lookback(optInTimePeriod, optInVFactor);
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_T3(t, t, g_rt_close, optInTimePeriod, optInVFactor, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_T3_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_T3_Open(&st, g_close, g_nPoints, optInTimePeriod, optInVFactor, &v0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_T3_Update(st, g_close[it & BENCH_MASK], &v0);
                    acc += v0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_T3_Peek(st, g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_T3_Update(st, g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_T3_Close(st);
            bench_stream_row("T3", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_T3_Close(st); }
            bench_stream_row("T3", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "TAN") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_TAN_Lookback();
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_TAN(t, t, g_rt_close, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_TAN_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_TAN_Open(&st, g_close, g_nPoints, &v0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_TAN_Update(st, g_close[it & BENCH_MASK], &v0);
                    acc += v0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_TAN_Peek(st, g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_TAN_Update(st, g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_TAN_Close(st);
            bench_stream_row("TAN", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_TAN_Close(st); }
            bench_stream_row("TAN", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "TANH") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_TANH_Lookback();
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_TANH(t, t, g_rt_close, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_TANH_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_TANH_Open(&st, g_close, g_nPoints, &v0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_TANH_Update(st, g_close[it & BENCH_MASK], &v0);
                    acc += v0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_TANH_Peek(st, g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_TANH_Update(st, g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_TANH_Close(st);
            bench_stream_row("TANH", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_TANH_Close(st); }
            bench_stream_row("TANH", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "TEMA") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        const int optInTimePeriod = bench_opaque_int(g_period > 0 ? g_period : 30);
        int lb = TA_TEMA_Lookback(optInTimePeriod);
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_TEMA(t, t, g_rt_close, optInTimePeriod, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_TEMA_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_TEMA_Open(&st, g_close, g_nPoints, optInTimePeriod, &v0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_TEMA_Update(st, g_close[it & BENCH_MASK], &v0);
                    acc += v0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_TEMA_Peek(st, g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_TEMA_Update(st, g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_TEMA_Close(st);
            bench_stream_row("TEMA", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_TEMA_Close(st); }
            bench_stream_row("TEMA", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "TRANGE") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_TRANGE_Lookback();
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_high[t] = g_high[it & BENCH_MASK];
                g_rt_low[t] = g_low[it & BENCH_MASK];
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_TRANGE(t, t, g_rt_high, g_rt_low, g_rt_close, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_TRANGE_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_TRANGE_Open(&st, g_high, g_low, g_close, g_nPoints, &v0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_TRANGE_Update(st, g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &v0);
                    acc += v0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_TRANGE_Peek(st, g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_TRANGE_Update(st, g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_TRANGE_Close(st);
            bench_stream_row("TRANGE", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_TRANGE_Close(st); }
            bench_stream_row("TRANGE", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "TRIMA") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        const int optInTimePeriod = bench_opaque_int(g_period > 0 ? g_period : 30);
        int lb = TA_TRIMA_Lookback(optInTimePeriod);
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_TRIMA(t, t, g_rt_close, optInTimePeriod, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_TRIMA_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_TRIMA_Open(&st, g_close, g_nPoints, optInTimePeriod, &v0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_TRIMA_Update(st, g_close[it & BENCH_MASK], &v0);
                    acc += v0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_TRIMA_Peek(st, g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_TRIMA_Update(st, g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_TRIMA_Close(st);
            bench_stream_row("TRIMA", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_TRIMA_Close(st); }
            bench_stream_row("TRIMA", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "TRIX") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        const int optInTimePeriod = bench_opaque_int(g_period > 0 ? g_period : 30);
        int lb = TA_TRIX_Lookback(optInTimePeriod);
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_TRIX(t, t, g_rt_close, optInTimePeriod, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_TRIX_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_TRIX_Open(&st, g_close, g_nPoints, optInTimePeriod, &v0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_TRIX_Update(st, g_close[it & BENCH_MASK], &v0);
                    acc += v0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_TRIX_Peek(st, g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_TRIX_Update(st, g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_TRIX_Close(st);
            bench_stream_row("TRIX", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_TRIX_Close(st); }
            bench_stream_row("TRIX", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "TSF") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        const int optInTimePeriod = bench_opaque_int(g_period > 0 ? g_period : 14);
        int lb = TA_TSF_Lookback(optInTimePeriod);
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_TSF(t, t, g_rt_close, optInTimePeriod, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_TSF_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_TSF_Open(&st, g_close, g_nPoints, optInTimePeriod, &v0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_TSF_Update(st, g_close[it & BENCH_MASK], &v0);
                    acc += v0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_TSF_Peek(st, g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_TSF_Update(st, g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_TSF_Close(st);
            bench_stream_row("TSF", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_TSF_Close(st); }
            bench_stream_row("TSF", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "TSI") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        const int optInFirstPeriod = bench_opaque_int(25);
        const int optInSecondPeriod = bench_opaque_int(13);
        int lb = TA_TSI_Lookback(optInFirstPeriod, optInSecondPeriod);
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_TSI(t, t, g_rt_close, optInFirstPeriod, optInSecondPeriod, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_TSI_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_TSI_Open(&st, g_close, g_nPoints, optInFirstPeriod, optInSecondPeriod, &v0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_TSI_Update(st, g_close[it & BENCH_MASK], &v0);
                    acc += v0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_TSI_Peek(st, g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_TSI_Update(st, g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_TSI_Close(st);
            bench_stream_row("TSI", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_TSI_Close(st); }
            bench_stream_row("TSI", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "TYPPRICE") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_TYPPRICE_Lookback();
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_high[t] = g_high[it & BENCH_MASK];
                g_rt_low[t] = g_low[it & BENCH_MASK];
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_TYPPRICE(t, t, g_rt_high, g_rt_low, g_rt_close, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_TYPPRICE_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_TYPPRICE_Open(&st, g_high, g_low, g_close, g_nPoints, &v0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_TYPPRICE_Update(st, g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &v0);
                    acc += v0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_TYPPRICE_Peek(st, g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_TYPPRICE_Update(st, g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_TYPPRICE_Close(st);
            bench_stream_row("TYPPRICE", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_TYPPRICE_Close(st); }
            bench_stream_row("TYPPRICE", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "ULTOSC") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        const int optInTimePeriod1 = bench_opaque_int(7);
        const int optInTimePeriod2 = bench_opaque_int(14);
        const int optInTimePeriod3 = bench_opaque_int(28);
        int lb = TA_ULTOSC_Lookback(optInTimePeriod1, optInTimePeriod2, optInTimePeriod3);
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_high[t] = g_high[it & BENCH_MASK];
                g_rt_low[t] = g_low[it & BENCH_MASK];
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_ULTOSC(t, t, g_rt_high, g_rt_low, g_rt_close, optInTimePeriod1, optInTimePeriod2, optInTimePeriod3, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_ULTOSC_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_ULTOSC_Open(&st, g_high, g_low, g_close, g_nPoints, optInTimePeriod1, optInTimePeriod2, optInTimePeriod3, &v0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_ULTOSC_Update(st, g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &v0);
                    acc += v0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_ULTOSC_Peek(st, g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_ULTOSC_Update(st, g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_ULTOSC_Close(st);
            bench_stream_row("ULTOSC", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_ULTOSC_Close(st); }
            bench_stream_row("ULTOSC", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "VAR") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        const int optInTimePeriod = bench_opaque_int(g_period > 0 ? g_period : 5);
        const double optInNbDev = bench_opaque_double(1.000000000000000);
        int lb = TA_VAR_Lookback(optInTimePeriod, optInNbDev);
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_VAR(t, t, g_rt_close, optInTimePeriod, optInNbDev, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_VAR_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_VAR_Open(&st, g_close, g_nPoints, optInTimePeriod, optInNbDev, &v0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_VAR_Update(st, g_close[it & BENCH_MASK], &v0);
                    acc += v0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_VAR_Peek(st, g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_VAR_Update(st, g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_VAR_Close(st);
            bench_stream_row("VAR", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_VAR_Close(st); }
            bench_stream_row("VAR", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "VHF") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        const int optInTimePeriod = bench_opaque_int(g_period > 0 ? g_period : 28);
        int lb = TA_VHF_Lookback(optInTimePeriod);
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_VHF(t, t, g_rt_close, optInTimePeriod, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_VHF_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_VHF_Open(&st, g_close, g_nPoints, optInTimePeriod, &v0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_VHF_Update(st, g_close[it & BENCH_MASK], &v0);
                    acc += v0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_VHF_Peek(st, g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_VHF_Update(st, g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_VHF_Close(st);
            bench_stream_row("VHF", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_VHF_Close(st); }
            bench_stream_row("VHF", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "VORTEX") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        const int optInTimePeriod = bench_opaque_int(g_period > 0 ? g_period : 14);
        int lb = TA_VORTEX_Lookback(optInTimePeriod);
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_high[t] = g_high[it & BENCH_MASK];
                g_rt_low[t] = g_low[it & BENCH_MASK];
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_VORTEX(t, t, g_rt_high, g_rt_low, g_rt_close, optInTimePeriod, &begIdx, &nb, g_outBuf0, g_outBuf1);
                acc += g_outBuf0[0];
                acc += g_outBuf1[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_VORTEX_Stream *st = NULL;
            double v0 = 0.0;
            double v1 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_VORTEX_Open(&st, g_high, g_low, g_close, g_nPoints, optInTimePeriod, &v0, &v1);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_VORTEX_Update(st, g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &v0, &v1);
                    acc += v0;
                    acc += v1;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_VORTEX_Peek(st, g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &v0, &v1);
                        acc += v0;
                        acc += v1;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_VORTEX_Update(st, g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &v0, &v1);
                        acc += v0;
                        acc += v1;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_VORTEX_Close(st);
            bench_stream_row("VORTEX", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_VORTEX_Close(st); }
            bench_stream_row("VORTEX", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "VWAP") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_VWAP_Lookback();
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_high[t] = g_high[it & BENCH_MASK];
                g_rt_low[t] = g_low[it & BENCH_MASK];
                g_rt_close[t] = g_close[it & BENCH_MASK];
                g_rt_volume[t] = g_volume[it & BENCH_MASK];
                TA_VWAP(t, t, g_rt_high, g_rt_low, g_rt_close, g_rt_volume, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_VWAP_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_VWAP_Open(&st, g_high, g_low, g_close, g_volume, g_nPoints, &v0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_VWAP_Update(st, g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], g_volume[it & BENCH_MASK], &v0);
                    acc += v0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_VWAP_Peek(st, g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], g_volume[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_VWAP_Update(st, g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], g_volume[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_VWAP_Close(st);
            bench_stream_row("VWAP", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_VWAP_Close(st); }
            bench_stream_row("VWAP", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "VWMA") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        const int optInTimePeriod = bench_opaque_int(g_period > 0 ? g_period : 30);
        int lb = TA_VWMA_Lookback(optInTimePeriod);
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_close[t] = g_close[it & BENCH_MASK];
                g_rt_volume[t] = g_volume[it & BENCH_MASK];
                TA_VWMA(t, t, g_rt_close, g_rt_volume, optInTimePeriod, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_VWMA_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_VWMA_Open(&st, g_close, g_volume, g_nPoints, optInTimePeriod, &v0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_VWMA_Update(st, g_close[it & BENCH_MASK], g_volume[it & BENCH_MASK], &v0);
                    acc += v0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_VWMA_Peek(st, g_close[it & BENCH_MASK], g_volume[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_VWMA_Update(st, g_close[it & BENCH_MASK], g_volume[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_VWMA_Close(st);
            bench_stream_row("VWMA", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_VWMA_Close(st); }
            bench_stream_row("VWMA", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "WAD") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_WAD_Lookback();
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_high[t] = g_high[it & BENCH_MASK];
                g_rt_low[t] = g_low[it & BENCH_MASK];
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_WAD(t, t, g_rt_high, g_rt_low, g_rt_close, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_WAD_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_WAD_Open(&st, g_high, g_low, g_close, g_nPoints, &v0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_WAD_Update(st, g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &v0);
                    acc += v0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_WAD_Peek(st, g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_WAD_Update(st, g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_WAD_Close(st);
            bench_stream_row("WAD", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_WAD_Close(st); }
            bench_stream_row("WAD", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "WCLPRICE") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_WCLPRICE_Lookback();
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_high[t] = g_high[it & BENCH_MASK];
                g_rt_low[t] = g_low[it & BENCH_MASK];
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_WCLPRICE(t, t, g_rt_high, g_rt_low, g_rt_close, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_WCLPRICE_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_WCLPRICE_Open(&st, g_high, g_low, g_close, g_nPoints, &v0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_WCLPRICE_Update(st, g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &v0);
                    acc += v0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_WCLPRICE_Peek(st, g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_WCLPRICE_Update(st, g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_WCLPRICE_Close(st);
            bench_stream_row("WCLPRICE", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_WCLPRICE_Close(st); }
            bench_stream_row("WCLPRICE", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "WILLR") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        const int optInTimePeriod = bench_opaque_int(g_period > 0 ? g_period : 14);
        int lb = TA_WILLR_Lookback(optInTimePeriod);
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_high[t] = g_high[it & BENCH_MASK];
                g_rt_low[t] = g_low[it & BENCH_MASK];
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_WILLR(t, t, g_rt_high, g_rt_low, g_rt_close, optInTimePeriod, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_WILLR_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_WILLR_Open(&st, g_high, g_low, g_close, g_nPoints, optInTimePeriod, &v0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_WILLR_Update(st, g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &v0);
                    acc += v0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_WILLR_Peek(st, g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_WILLR_Update(st, g_high[it & BENCH_MASK], g_low[it & BENCH_MASK], g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_WILLR_Close(st);
            bench_stream_row("WILLR", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_WILLR_Close(st); }
            bench_stream_row("WILLR", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "WMA") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        const int optInTimePeriod = bench_opaque_int(g_period > 0 ? g_period : 30);
        int lb = TA_WMA_Lookback(optInTimePeriod);
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_WMA(t, t, g_rt_close, optInTimePeriod, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_WMA_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_WMA_Open(&st, g_close, g_nPoints, optInTimePeriod, &v0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_WMA_Update(st, g_close[it & BENCH_MASK], &v0);
                    acc += v0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_WMA_Peek(st, g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_WMA_Update(st, g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_WMA_Close(st);
            bench_stream_row("WMA", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_WMA_Close(st); }
            bench_stream_row("WMA", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "ZLEMA") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        const int optInTimePeriod = bench_opaque_int(g_period > 0 ? g_period : 30);
        int lb = TA_ZLEMA_Lookback(optInTimePeriod);
        bench_rt_reserve((long long)lb + iters);
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb < 0 ? 0 : lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_ZLEMA(t, t, g_rt_close, optInTimePeriod, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_ZLEMA_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_ZLEMA_Open(&st, g_close, g_nPoints, optInTimePeriod, &v0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_ZLEMA_Update(st, g_close[it & BENCH_MASK], &v0);
                    acc += v0;
                }
                long long tu = get_nanotime() - t0;
                if( best_u < 0 || tu < best_u ) best_u = tu;
            }
            for( int pass = 0; pass < 3; pass++ ) {
                long long tp = 0;
                for( int b = 0; b < nblk; b++ ) {
                    long long t0 = get_nanotime();
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_ZLEMA_Peek(st, g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_ZLEMA_Update(st, g_close[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_ZLEMA_Close(st);
            bench_stream_row("ZLEMA", orc, best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_ZLEMA_Close(st); }
            bench_stream_row("ZLEMA", orc, best_b/(double)iters, -1.0, -1.0, lb, 0);
        }
        fflush(stdout);
    }
}


int main(int argc, char *argv[]) {
    TA_Initialize();
    int n_points = 100000;
    int n_iters = 500;
    int verify_corpus = 0;
    const char *func_filter = NULL;
    g_corpus.refPeriod = 0;   /* 0 = derive after the loop */
    for( int i = 1; i < argc; i++ ) {
        if( strncmp(argv[i], "--points=", 9) == 0 )    n_points = atoi(argv[i]+9);
        else if( strncmp(argv[i], "--iters=", 8) == 0 ) n_iters = atoi(argv[i]+8);
        else if( strncmp(argv[i], "--function=", 11) == 0 ) func_filter = argv[i]+11;
        /* Gate: exit non-zero if any function streams slower than this multiple
           of its batch@last cost. Stream-bench only, hence not in CORPUS_ARGS. */
        else if( strncmp(argv[i], "--min-ratio=", 12) == 0 ) g_min_ratio = atof(argv[i]+12);
        else if( strncmp(argv[i], "--period=", 9) == 0 ) g_period = atoi(argv[i]+9);
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
    if( n_points < BENCH_MASK + 1 ) n_points = BENCH_MASK + 1; /* the bar feed indexes it & BENCH_MASK */
    if( n_iters < 1 ) n_iters = 1;
    /* The trend/chop regime length is relative to the window under test. */
    if( g_corpus.refPeriod <= 0 )
        g_corpus.refPeriod = (g_period > 0) ? g_period : BENCH_CORPUS_PERIOD;
    /* After the loop, so the check runs at the n actually benchmarked
       regardless of where --points sits in argv. */
    if( verify_corpus ) return bench_corpus_selfcheck(n_points, &g_corpus) ? 1 : 0;
    generate_price_data(n_points);
    /* Growing history for batch@last, sized so it never recycles within a pass. */
    bench_rt_reserve((long long)n_iters + 8192);
    bench_stream_all(func_filter, n_iters);
    int rc = bench_stream_summary();
    free(g_rt_open); free(g_rt_high); free(g_rt_low); free(g_rt_close); free(g_rt_volume); free(g_rt_oi); free(g_rt_periods);
    free(g_open); free(g_high); free(g_low); free(g_close); free(g_volume); free(g_oi); free(g_periods);
    return rc;
}
