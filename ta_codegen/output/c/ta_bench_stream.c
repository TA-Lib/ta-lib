/* Auto-generated streaming benchmark for ta_codegen C output.
 * Per streamable function: batch@last vs Update vs Peek (ns) + handle bytes.
 * Output: `NAME batch_last update peek lookback handle_bytes` per line.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#ifdef __APPLE__
#include <mach/mach_time.h>
#endif

#include "ta_func_unguarded.h"
#include "ta_func/ta_func_private.h"

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

#include "ta_ACCBANDS.c"
#include "ta_ACOS.c"
#include "ta_AD.c"
#include "ta_ADD.c"
#include "ta_ADOSC.c"
#include "ta_ADX.c"
#include "ta_ADXR.c"
#include "ta_APO.c"
#include "ta_AROON.c"
#include "ta_AROONOSC.c"
#include "ta_ASIN.c"
#include "ta_ATAN.c"
#include "ta_ATR.c"
#include "ta_AVGDEV.c"
#include "ta_AVGPRICE.c"
#include "ta_BBANDS.c"
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
#include "ta_CMO.c"
#include "ta_CORREL.c"
#include "ta_COS.c"
#include "ta_COSH.c"
#include "ta_DEMA.c"
#include "ta_DIV.c"
#include "ta_DX.c"
#include "ta_EMA.c"
#include "ta_EXP.c"
#include "ta_FLOOR.c"
#include "ta_HT_DCPERIOD.c"
#include "ta_HT_DCPHASE.c"
#include "ta_HT_PHASOR.c"
#include "ta_HT_SINE.c"
#include "ta_HT_TRENDLINE.c"
#include "ta_HT_TRENDMODE.c"
#include "ta_IMI.c"
#include "ta_KAMA.c"
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
#include "ta_MAVP.c"
#include "ta_MAX.c"
#include "ta_MAXINDEX.c"
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
#include "ta_OBV.c"
#include "ta_PLUS_DI.c"
#include "ta_PLUS_DM.c"
#include "ta_PPO.c"
#include "ta_ROC.c"
#include "ta_ROCP.c"
#include "ta_ROCR.c"
#include "ta_ROCR100.c"
#include "ta_RSI.c"
#include "ta_SAR.c"
#include "ta_SAREXT.c"
#include "ta_SIN.c"
#include "ta_SINH.c"
#include "ta_SMA.c"
#include "ta_SQRT.c"
#include "ta_STDDEV.c"
#include "ta_STOCH.c"
#include "ta_STOCHF.c"
#include "ta_STOCHRSI.c"
#include "ta_SUB.c"
#include "ta_SUM.c"
#include "ta_T3.c"
#include "ta_TAN.c"
#include "ta_TANH.c"
#include "ta_TEMA.c"
#include "ta_TRANGE.c"
#include "ta_TRIMA.c"
#include "ta_TRIX.c"
#include "ta_TSF.c"
#include "ta_TYPPRICE.c"
#include "ta_ULTOSC.c"
#include "ta_VAR.c"
#include "ta_WCLPRICE.c"
#include "ta_WILLR.c"
#include "ta_WMA.c"
#include "ta_MA.c"


static long long get_nanotime(void) {
#ifdef __APPLE__
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


static double *g_open, *g_high, *g_low, *g_close, *g_volume, *g_oi;
static int g_nPoints;

static void generate_price_data(int n) {
    g_nPoints = n;
    g_open   = calloc(n, sizeof(double));
    g_high   = calloc(n, sizeof(double));
    g_low    = calloc(n, sizeof(double));
    g_close  = calloc(n, sizeof(double));
    g_volume = calloc(n, sizeof(double));
    g_oi     = calloc(n, sizeof(double));
    unsigned int seed = 42;
    double price = 100.0;
    for( int i = 0; i < n; i++ ) {
        seed = seed * 1103515245 + 12345;
        double r = ((double)(seed >> 16) / 32768.0) - 1.0;
        double o = price, c = price + r * 2.0;
        double h = fmax(o, c) + fabs(r) * 0.5;
        double l = fmin(o, c) - fabs(r) * 0.5;
        if( l < 1.0 ) l = 1.0;
        g_open[i] = o; g_high[i] = h; g_low[i] = l; g_close[i] = c;
        g_volume[i] = 1000000.0 + r * 500000.0;
        price = c; if( price < 1.0 ) price = 1.0;
    }
}

#define MAX_POINTS 200000
static double g_outBuf0[MAX_POINTS];
static double g_outBuf1[MAX_POINTS];
static double g_outBuf2[MAX_POINTS];
static int g_outIntBuf0[MAX_POINTS];
static int g_outIntBuf1[MAX_POINTS];


static int func_matches(const char *filter, const char *name) {
    if( !filter || !*filter ) return 1;
    char buf[512]; strncpy(buf, filter, sizeof(buf)-1); buf[sizeof(buf)-1]='\0';
    char *saveptr = NULL;
    for( char *tok = strtok_r(buf, ",", &saveptr); tok; tok = strtok_r(NULL, ",", &saveptr) )
        if( strcasestr(name, tok) ) return 1;
    return 0;
}

static volatile int g_sink = 0;

#define BENCH_MASK 4095

static void bench_stream_all(const char *filter, int iters) {
    printf("# func batch_last_ns update_ns peek_ns lookback handle_bytes\n");
    fflush(stdout);
    if( func_matches(filter, "ACCBANDS") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_ACCBANDS(e, e, g_high, g_low, g_close, 20, &begIdx, &nb, g_outBuf0, g_outBuf1, g_outBuf2);
                acc += g_outBuf0[0];
                acc += g_outBuf1[0];
                acc += g_outBuf2[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_ACCBANDS_Lookback(20);
        TA_ACCBANDS_Stream *st = NULL;
            double v0 = 0.0;
            double v1 = 0.0;
            double v2 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_ACCBANDS_Open(20, g_high, g_low, g_close, g_nPoints, &st, &v0, &v1, &v2);
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
            printf("ACCBANDS %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_ACCBANDS_Close(st); }
            printf("ACCBANDS %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "ACOS") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_ACOS(e, e, g_close, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_ACOS_Lookback();
        TA_ACOS_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_ACOS_Open(g_close, g_nPoints, &st, &v0);
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
            printf("ACOS %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_ACOS_Close(st); }
            printf("ACOS %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "AD") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_AD(e, e, g_high, g_low, g_close, g_volume, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_AD_Lookback();
        TA_AD_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_AD_Open(g_high, g_low, g_close, g_volume, g_nPoints, &st, &v0);
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
            printf("AD %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_AD_Close(st); }
            printf("AD %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "ADD") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_ADD(e, e, g_close, g_high, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_ADD_Lookback();
        TA_ADD_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_ADD_Open(g_close, g_high, g_nPoints, &st, &v0);
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
            printf("ADD %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_ADD_Close(st); }
            printf("ADD %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "ADOSC") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_ADOSC(e, e, g_high, g_low, g_close, g_volume, 3, 10, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_ADOSC_Lookback(3, 10);
        TA_ADOSC_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_ADOSC_Open(3, 10, g_high, g_low, g_close, g_volume, g_nPoints, &st, &v0);
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
            printf("ADOSC %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_ADOSC_Close(st); }
            printf("ADOSC %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "ADX") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_ADX(e, e, g_high, g_low, g_close, 14, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_ADX_Lookback(14);
        TA_ADX_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_ADX_Open(14, g_high, g_low, g_close, g_nPoints, &st, &v0);
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
            printf("ADX %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_ADX_Close(st); }
            printf("ADX %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "ADXR") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_ADXR(e, e, g_high, g_low, g_close, 14, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_ADXR_Lookback(14);
        TA_ADXR_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_ADXR_Open(14, g_high, g_low, g_close, g_nPoints, &st, &v0);
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
            printf("ADXR %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_ADXR_Close(st); }
            printf("ADXR %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "APO") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_APO(e, e, g_close, 12, 26, 0, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_APO_Lookback(12, 26, 0);
        TA_APO_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_APO_Open(12, 26, 0, g_close, g_nPoints, &st, &v0);
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
            printf("APO %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_APO_Close(st); }
            printf("APO %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "AROON") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_AROON(e, e, g_high, g_low, 14, &begIdx, &nb, g_outBuf0, g_outBuf1);
                acc += g_outBuf0[0];
                acc += g_outBuf1[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_AROON_Lookback(14);
        TA_AROON_Stream *st = NULL;
            double v0 = 0.0;
            double v1 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_AROON_Open(14, g_high, g_low, g_nPoints, &st, &v0, &v1);
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
            printf("AROON %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_AROON_Close(st); }
            printf("AROON %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "AROONOSC") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_AROONOSC(e, e, g_high, g_low, 14, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_AROONOSC_Lookback(14);
        TA_AROONOSC_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_AROONOSC_Open(14, g_high, g_low, g_nPoints, &st, &v0);
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
            printf("AROONOSC %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_AROONOSC_Close(st); }
            printf("AROONOSC %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "ASIN") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_ASIN(e, e, g_close, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_ASIN_Lookback();
        TA_ASIN_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_ASIN_Open(g_close, g_nPoints, &st, &v0);
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
            printf("ASIN %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_ASIN_Close(st); }
            printf("ASIN %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "ATAN") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_ATAN(e, e, g_close, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_ATAN_Lookback();
        TA_ATAN_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_ATAN_Open(g_close, g_nPoints, &st, &v0);
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
            printf("ATAN %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_ATAN_Close(st); }
            printf("ATAN %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "ATR") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_ATR(e, e, g_high, g_low, g_close, 14, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_ATR_Lookback(14);
        TA_ATR_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_ATR_Open(14, g_high, g_low, g_close, g_nPoints, &st, &v0);
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
            printf("ATR %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_ATR_Close(st); }
            printf("ATR %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "AVGDEV") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_AVGDEV(e, e, g_close, 14, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_AVGDEV_Lookback(14);
        TA_AVGDEV_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_AVGDEV_Open(14, g_close, g_nPoints, &st, &v0);
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
            printf("AVGDEV %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_AVGDEV_Close(st); }
            printf("AVGDEV %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "AVGPRICE") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_AVGPRICE(e, e, g_open, g_high, g_low, g_close, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_AVGPRICE_Lookback();
        TA_AVGPRICE_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_AVGPRICE_Open(g_open, g_high, g_low, g_close, g_nPoints, &st, &v0);
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
            printf("AVGPRICE %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_AVGPRICE_Close(st); }
            printf("AVGPRICE %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "BBANDS") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_BBANDS(e, e, g_close, 5, 2.000000000000000, 2.000000000000000, 0, &begIdx, &nb, g_outBuf0, g_outBuf1, g_outBuf2);
                acc += g_outBuf0[0];
                acc += g_outBuf1[0];
                acc += g_outBuf2[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_BBANDS_Lookback(5, 2.000000000000000, 2.000000000000000, 0);
        TA_BBANDS_Stream *st = NULL;
            double v0 = 0.0;
            double v1 = 0.0;
            double v2 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_BBANDS_Open(5, 2.000000000000000, 2.000000000000000, 0, g_close, g_nPoints, &st, &v0, &v1, &v2);
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
            printf("BBANDS %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_BBANDS_Close(st); }
            printf("BBANDS %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "BETA") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_BETA(e, e, g_close, g_high, 5, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_BETA_Lookback(5);
        TA_BETA_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_BETA_Open(5, g_close, g_high, g_nPoints, &st, &v0);
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
            printf("BETA %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_BETA_Close(st); }
            printf("BETA %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "BOP") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_BOP(e, e, g_open, g_high, g_low, g_close, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_BOP_Lookback();
        TA_BOP_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_BOP_Open(g_open, g_high, g_low, g_close, g_nPoints, &st, &v0);
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
            printf("BOP %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_BOP_Close(st); }
            printf("BOP %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CCI") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_CCI(e, e, g_high, g_low, g_close, 14, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_CCI_Lookback(14);
        TA_CCI_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CCI_Open(14, g_high, g_low, g_close, g_nPoints, &st, &v0);
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
            printf("CCI %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CCI_Close(st); }
            printf("CCI %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDL2CROWS") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_CDL2CROWS(e, e, g_open, g_high, g_low, g_close, &begIdx, &nb, g_outIntBuf0);
                acc += (double)g_outIntBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_CDL2CROWS_Lookback();
        TA_CDL2CROWS_Stream *st = NULL;
            int iv0 = 0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CDL2CROWS_Open(g_open, g_high, g_low, g_close, g_nPoints, &st, &iv0);
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
            printf("CDL2CROWS %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDL2CROWS_Close(st); }
            printf("CDL2CROWS %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDL3BLACKCROWS") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_CDL3BLACKCROWS(e, e, g_open, g_high, g_low, g_close, &begIdx, &nb, g_outIntBuf0);
                acc += (double)g_outIntBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_CDL3BLACKCROWS_Lookback();
        TA_CDL3BLACKCROWS_Stream *st = NULL;
            int iv0 = 0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CDL3BLACKCROWS_Open(g_open, g_high, g_low, g_close, g_nPoints, &st, &iv0);
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
            printf("CDL3BLACKCROWS %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDL3BLACKCROWS_Close(st); }
            printf("CDL3BLACKCROWS %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDL3INSIDE") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_CDL3INSIDE(e, e, g_open, g_high, g_low, g_close, &begIdx, &nb, g_outIntBuf0);
                acc += (double)g_outIntBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_CDL3INSIDE_Lookback();
        TA_CDL3INSIDE_Stream *st = NULL;
            int iv0 = 0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CDL3INSIDE_Open(g_open, g_high, g_low, g_close, g_nPoints, &st, &iv0);
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
            printf("CDL3INSIDE %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDL3INSIDE_Close(st); }
            printf("CDL3INSIDE %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDL3LINESTRIKE") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_CDL3LINESTRIKE(e, e, g_open, g_high, g_low, g_close, &begIdx, &nb, g_outIntBuf0);
                acc += (double)g_outIntBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_CDL3LINESTRIKE_Lookback();
        TA_CDL3LINESTRIKE_Stream *st = NULL;
            int iv0 = 0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CDL3LINESTRIKE_Open(g_open, g_high, g_low, g_close, g_nPoints, &st, &iv0);
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
            printf("CDL3LINESTRIKE %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDL3LINESTRIKE_Close(st); }
            printf("CDL3LINESTRIKE %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDL3OUTSIDE") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_CDL3OUTSIDE(e, e, g_open, g_high, g_low, g_close, &begIdx, &nb, g_outIntBuf0);
                acc += (double)g_outIntBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_CDL3OUTSIDE_Lookback();
        TA_CDL3OUTSIDE_Stream *st = NULL;
            int iv0 = 0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CDL3OUTSIDE_Open(g_open, g_high, g_low, g_close, g_nPoints, &st, &iv0);
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
            printf("CDL3OUTSIDE %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDL3OUTSIDE_Close(st); }
            printf("CDL3OUTSIDE %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDL3STARSINSOUTH") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_CDL3STARSINSOUTH(e, e, g_open, g_high, g_low, g_close, &begIdx, &nb, g_outIntBuf0);
                acc += (double)g_outIntBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_CDL3STARSINSOUTH_Lookback();
        TA_CDL3STARSINSOUTH_Stream *st = NULL;
            int iv0 = 0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CDL3STARSINSOUTH_Open(g_open, g_high, g_low, g_close, g_nPoints, &st, &iv0);
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
            printf("CDL3STARSINSOUTH %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDL3STARSINSOUTH_Close(st); }
            printf("CDL3STARSINSOUTH %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDL3WHITESOLDIERS") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_CDL3WHITESOLDIERS(e, e, g_open, g_high, g_low, g_close, &begIdx, &nb, g_outIntBuf0);
                acc += (double)g_outIntBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_CDL3WHITESOLDIERS_Lookback();
        TA_CDL3WHITESOLDIERS_Stream *st = NULL;
            int iv0 = 0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CDL3WHITESOLDIERS_Open(g_open, g_high, g_low, g_close, g_nPoints, &st, &iv0);
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
            printf("CDL3WHITESOLDIERS %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDL3WHITESOLDIERS_Close(st); }
            printf("CDL3WHITESOLDIERS %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLABANDONEDBABY") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_CDLABANDONEDBABY(e, e, g_open, g_high, g_low, g_close, 0.300000000000000, &begIdx, &nb, g_outIntBuf0);
                acc += (double)g_outIntBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_CDLABANDONEDBABY_Lookback(0.300000000000000);
        TA_CDLABANDONEDBABY_Stream *st = NULL;
            int iv0 = 0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CDLABANDONEDBABY_Open(0.300000000000000, g_open, g_high, g_low, g_close, g_nPoints, &st, &iv0);
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
            printf("CDLABANDONEDBABY %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLABANDONEDBABY_Close(st); }
            printf("CDLABANDONEDBABY %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLADVANCEBLOCK") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_CDLADVANCEBLOCK(e, e, g_open, g_high, g_low, g_close, &begIdx, &nb, g_outIntBuf0);
                acc += (double)g_outIntBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_CDLADVANCEBLOCK_Lookback();
        TA_CDLADVANCEBLOCK_Stream *st = NULL;
            int iv0 = 0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CDLADVANCEBLOCK_Open(g_open, g_high, g_low, g_close, g_nPoints, &st, &iv0);
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
            printf("CDLADVANCEBLOCK %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLADVANCEBLOCK_Close(st); }
            printf("CDLADVANCEBLOCK %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLBELTHOLD") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_CDLBELTHOLD(e, e, g_open, g_high, g_low, g_close, &begIdx, &nb, g_outIntBuf0);
                acc += (double)g_outIntBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_CDLBELTHOLD_Lookback();
        TA_CDLBELTHOLD_Stream *st = NULL;
            int iv0 = 0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CDLBELTHOLD_Open(g_open, g_high, g_low, g_close, g_nPoints, &st, &iv0);
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
            printf("CDLBELTHOLD %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLBELTHOLD_Close(st); }
            printf("CDLBELTHOLD %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLBREAKAWAY") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_CDLBREAKAWAY(e, e, g_open, g_high, g_low, g_close, &begIdx, &nb, g_outIntBuf0);
                acc += (double)g_outIntBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_CDLBREAKAWAY_Lookback();
        TA_CDLBREAKAWAY_Stream *st = NULL;
            int iv0 = 0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CDLBREAKAWAY_Open(g_open, g_high, g_low, g_close, g_nPoints, &st, &iv0);
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
            printf("CDLBREAKAWAY %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLBREAKAWAY_Close(st); }
            printf("CDLBREAKAWAY %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLCLOSINGMARUBOZU") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_CDLCLOSINGMARUBOZU(e, e, g_open, g_high, g_low, g_close, &begIdx, &nb, g_outIntBuf0);
                acc += (double)g_outIntBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_CDLCLOSINGMARUBOZU_Lookback();
        TA_CDLCLOSINGMARUBOZU_Stream *st = NULL;
            int iv0 = 0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CDLCLOSINGMARUBOZU_Open(g_open, g_high, g_low, g_close, g_nPoints, &st, &iv0);
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
            printf("CDLCLOSINGMARUBOZU %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLCLOSINGMARUBOZU_Close(st); }
            printf("CDLCLOSINGMARUBOZU %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLCONCEALBABYSWALL") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_CDLCONCEALBABYSWALL(e, e, g_open, g_high, g_low, g_close, &begIdx, &nb, g_outIntBuf0);
                acc += (double)g_outIntBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_CDLCONCEALBABYSWALL_Lookback();
        TA_CDLCONCEALBABYSWALL_Stream *st = NULL;
            int iv0 = 0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CDLCONCEALBABYSWALL_Open(g_open, g_high, g_low, g_close, g_nPoints, &st, &iv0);
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
            printf("CDLCONCEALBABYSWALL %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLCONCEALBABYSWALL_Close(st); }
            printf("CDLCONCEALBABYSWALL %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLCOUNTERATTACK") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_CDLCOUNTERATTACK(e, e, g_open, g_high, g_low, g_close, &begIdx, &nb, g_outIntBuf0);
                acc += (double)g_outIntBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_CDLCOUNTERATTACK_Lookback();
        TA_CDLCOUNTERATTACK_Stream *st = NULL;
            int iv0 = 0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CDLCOUNTERATTACK_Open(g_open, g_high, g_low, g_close, g_nPoints, &st, &iv0);
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
            printf("CDLCOUNTERATTACK %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLCOUNTERATTACK_Close(st); }
            printf("CDLCOUNTERATTACK %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLDARKCLOUDCOVER") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_CDLDARKCLOUDCOVER(e, e, g_open, g_high, g_low, g_close, 0.500000000000000, &begIdx, &nb, g_outIntBuf0);
                acc += (double)g_outIntBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_CDLDARKCLOUDCOVER_Lookback(0.500000000000000);
        TA_CDLDARKCLOUDCOVER_Stream *st = NULL;
            int iv0 = 0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CDLDARKCLOUDCOVER_Open(0.500000000000000, g_open, g_high, g_low, g_close, g_nPoints, &st, &iv0);
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
            printf("CDLDARKCLOUDCOVER %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLDARKCLOUDCOVER_Close(st); }
            printf("CDLDARKCLOUDCOVER %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLDOJI") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_CDLDOJI(e, e, g_open, g_high, g_low, g_close, &begIdx, &nb, g_outIntBuf0);
                acc += (double)g_outIntBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_CDLDOJI_Lookback();
        TA_CDLDOJI_Stream *st = NULL;
            int iv0 = 0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CDLDOJI_Open(g_open, g_high, g_low, g_close, g_nPoints, &st, &iv0);
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
            printf("CDLDOJI %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLDOJI_Close(st); }
            printf("CDLDOJI %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLDOJISTAR") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_CDLDOJISTAR(e, e, g_open, g_high, g_low, g_close, &begIdx, &nb, g_outIntBuf0);
                acc += (double)g_outIntBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_CDLDOJISTAR_Lookback();
        TA_CDLDOJISTAR_Stream *st = NULL;
            int iv0 = 0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CDLDOJISTAR_Open(g_open, g_high, g_low, g_close, g_nPoints, &st, &iv0);
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
            printf("CDLDOJISTAR %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLDOJISTAR_Close(st); }
            printf("CDLDOJISTAR %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLDRAGONFLYDOJI") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_CDLDRAGONFLYDOJI(e, e, g_open, g_high, g_low, g_close, &begIdx, &nb, g_outIntBuf0);
                acc += (double)g_outIntBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_CDLDRAGONFLYDOJI_Lookback();
        TA_CDLDRAGONFLYDOJI_Stream *st = NULL;
            int iv0 = 0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CDLDRAGONFLYDOJI_Open(g_open, g_high, g_low, g_close, g_nPoints, &st, &iv0);
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
            printf("CDLDRAGONFLYDOJI %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLDRAGONFLYDOJI_Close(st); }
            printf("CDLDRAGONFLYDOJI %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLENGULFING") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_CDLENGULFING(e, e, g_open, g_high, g_low, g_close, &begIdx, &nb, g_outIntBuf0);
                acc += (double)g_outIntBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_CDLENGULFING_Lookback();
        TA_CDLENGULFING_Stream *st = NULL;
            int iv0 = 0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CDLENGULFING_Open(g_open, g_high, g_low, g_close, g_nPoints, &st, &iv0);
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
            printf("CDLENGULFING %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLENGULFING_Close(st); }
            printf("CDLENGULFING %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLEVENINGDOJISTAR") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_CDLEVENINGDOJISTAR(e, e, g_open, g_high, g_low, g_close, 0.300000000000000, &begIdx, &nb, g_outIntBuf0);
                acc += (double)g_outIntBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_CDLEVENINGDOJISTAR_Lookback(0.300000000000000);
        TA_CDLEVENINGDOJISTAR_Stream *st = NULL;
            int iv0 = 0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CDLEVENINGDOJISTAR_Open(0.300000000000000, g_open, g_high, g_low, g_close, g_nPoints, &st, &iv0);
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
            printf("CDLEVENINGDOJISTAR %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLEVENINGDOJISTAR_Close(st); }
            printf("CDLEVENINGDOJISTAR %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLEVENINGSTAR") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_CDLEVENINGSTAR(e, e, g_open, g_high, g_low, g_close, 0.300000000000000, &begIdx, &nb, g_outIntBuf0);
                acc += (double)g_outIntBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_CDLEVENINGSTAR_Lookback(0.300000000000000);
        TA_CDLEVENINGSTAR_Stream *st = NULL;
            int iv0 = 0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CDLEVENINGSTAR_Open(0.300000000000000, g_open, g_high, g_low, g_close, g_nPoints, &st, &iv0);
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
            printf("CDLEVENINGSTAR %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLEVENINGSTAR_Close(st); }
            printf("CDLEVENINGSTAR %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLGAPSIDESIDEWHITE") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_CDLGAPSIDESIDEWHITE(e, e, g_open, g_high, g_low, g_close, &begIdx, &nb, g_outIntBuf0);
                acc += (double)g_outIntBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_CDLGAPSIDESIDEWHITE_Lookback();
        TA_CDLGAPSIDESIDEWHITE_Stream *st = NULL;
            int iv0 = 0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CDLGAPSIDESIDEWHITE_Open(g_open, g_high, g_low, g_close, g_nPoints, &st, &iv0);
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
            printf("CDLGAPSIDESIDEWHITE %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLGAPSIDESIDEWHITE_Close(st); }
            printf("CDLGAPSIDESIDEWHITE %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLGRAVESTONEDOJI") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_CDLGRAVESTONEDOJI(e, e, g_open, g_high, g_low, g_close, &begIdx, &nb, g_outIntBuf0);
                acc += (double)g_outIntBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_CDLGRAVESTONEDOJI_Lookback();
        TA_CDLGRAVESTONEDOJI_Stream *st = NULL;
            int iv0 = 0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CDLGRAVESTONEDOJI_Open(g_open, g_high, g_low, g_close, g_nPoints, &st, &iv0);
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
            printf("CDLGRAVESTONEDOJI %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLGRAVESTONEDOJI_Close(st); }
            printf("CDLGRAVESTONEDOJI %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLHAMMER") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_CDLHAMMER(e, e, g_open, g_high, g_low, g_close, &begIdx, &nb, g_outIntBuf0);
                acc += (double)g_outIntBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_CDLHAMMER_Lookback();
        TA_CDLHAMMER_Stream *st = NULL;
            int iv0 = 0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CDLHAMMER_Open(g_open, g_high, g_low, g_close, g_nPoints, &st, &iv0);
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
            printf("CDLHAMMER %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLHAMMER_Close(st); }
            printf("CDLHAMMER %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLHANGINGMAN") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_CDLHANGINGMAN(e, e, g_open, g_high, g_low, g_close, &begIdx, &nb, g_outIntBuf0);
                acc += (double)g_outIntBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_CDLHANGINGMAN_Lookback();
        TA_CDLHANGINGMAN_Stream *st = NULL;
            int iv0 = 0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CDLHANGINGMAN_Open(g_open, g_high, g_low, g_close, g_nPoints, &st, &iv0);
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
            printf("CDLHANGINGMAN %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLHANGINGMAN_Close(st); }
            printf("CDLHANGINGMAN %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLHARAMI") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_CDLHARAMI(e, e, g_open, g_high, g_low, g_close, &begIdx, &nb, g_outIntBuf0);
                acc += (double)g_outIntBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_CDLHARAMI_Lookback();
        TA_CDLHARAMI_Stream *st = NULL;
            int iv0 = 0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CDLHARAMI_Open(g_open, g_high, g_low, g_close, g_nPoints, &st, &iv0);
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
            printf("CDLHARAMI %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLHARAMI_Close(st); }
            printf("CDLHARAMI %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLHARAMICROSS") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_CDLHARAMICROSS(e, e, g_open, g_high, g_low, g_close, &begIdx, &nb, g_outIntBuf0);
                acc += (double)g_outIntBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_CDLHARAMICROSS_Lookback();
        TA_CDLHARAMICROSS_Stream *st = NULL;
            int iv0 = 0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CDLHARAMICROSS_Open(g_open, g_high, g_low, g_close, g_nPoints, &st, &iv0);
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
            printf("CDLHARAMICROSS %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLHARAMICROSS_Close(st); }
            printf("CDLHARAMICROSS %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLHIGHWAVE") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_CDLHIGHWAVE(e, e, g_open, g_high, g_low, g_close, &begIdx, &nb, g_outIntBuf0);
                acc += (double)g_outIntBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_CDLHIGHWAVE_Lookback();
        TA_CDLHIGHWAVE_Stream *st = NULL;
            int iv0 = 0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CDLHIGHWAVE_Open(g_open, g_high, g_low, g_close, g_nPoints, &st, &iv0);
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
            printf("CDLHIGHWAVE %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLHIGHWAVE_Close(st); }
            printf("CDLHIGHWAVE %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLHIKKAKE") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_CDLHIKKAKE(e, e, g_open, g_high, g_low, g_close, &begIdx, &nb, g_outIntBuf0);
                acc += (double)g_outIntBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_CDLHIKKAKE_Lookback();
        TA_CDLHIKKAKE_Stream *st = NULL;
            int iv0 = 0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CDLHIKKAKE_Open(g_open, g_high, g_low, g_close, g_nPoints, &st, &iv0);
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
            printf("CDLHIKKAKE %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLHIKKAKE_Close(st); }
            printf("CDLHIKKAKE %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLHIKKAKEMOD") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_CDLHIKKAKEMOD(e, e, g_open, g_high, g_low, g_close, &begIdx, &nb, g_outIntBuf0);
                acc += (double)g_outIntBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_CDLHIKKAKEMOD_Lookback();
        TA_CDLHIKKAKEMOD_Stream *st = NULL;
            int iv0 = 0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CDLHIKKAKEMOD_Open(g_open, g_high, g_low, g_close, g_nPoints, &st, &iv0);
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
            printf("CDLHIKKAKEMOD %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLHIKKAKEMOD_Close(st); }
            printf("CDLHIKKAKEMOD %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLHOMINGPIGEON") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_CDLHOMINGPIGEON(e, e, g_open, g_high, g_low, g_close, &begIdx, &nb, g_outIntBuf0);
                acc += (double)g_outIntBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_CDLHOMINGPIGEON_Lookback();
        TA_CDLHOMINGPIGEON_Stream *st = NULL;
            int iv0 = 0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CDLHOMINGPIGEON_Open(g_open, g_high, g_low, g_close, g_nPoints, &st, &iv0);
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
            printf("CDLHOMINGPIGEON %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLHOMINGPIGEON_Close(st); }
            printf("CDLHOMINGPIGEON %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLIDENTICAL3CROWS") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_CDLIDENTICAL3CROWS(e, e, g_open, g_high, g_low, g_close, &begIdx, &nb, g_outIntBuf0);
                acc += (double)g_outIntBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_CDLIDENTICAL3CROWS_Lookback();
        TA_CDLIDENTICAL3CROWS_Stream *st = NULL;
            int iv0 = 0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CDLIDENTICAL3CROWS_Open(g_open, g_high, g_low, g_close, g_nPoints, &st, &iv0);
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
            printf("CDLIDENTICAL3CROWS %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLIDENTICAL3CROWS_Close(st); }
            printf("CDLIDENTICAL3CROWS %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLINNECK") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_CDLINNECK(e, e, g_open, g_high, g_low, g_close, &begIdx, &nb, g_outIntBuf0);
                acc += (double)g_outIntBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_CDLINNECK_Lookback();
        TA_CDLINNECK_Stream *st = NULL;
            int iv0 = 0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CDLINNECK_Open(g_open, g_high, g_low, g_close, g_nPoints, &st, &iv0);
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
            printf("CDLINNECK %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLINNECK_Close(st); }
            printf("CDLINNECK %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLINVERTEDHAMMER") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_CDLINVERTEDHAMMER(e, e, g_open, g_high, g_low, g_close, &begIdx, &nb, g_outIntBuf0);
                acc += (double)g_outIntBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_CDLINVERTEDHAMMER_Lookback();
        TA_CDLINVERTEDHAMMER_Stream *st = NULL;
            int iv0 = 0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CDLINVERTEDHAMMER_Open(g_open, g_high, g_low, g_close, g_nPoints, &st, &iv0);
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
            printf("CDLINVERTEDHAMMER %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLINVERTEDHAMMER_Close(st); }
            printf("CDLINVERTEDHAMMER %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLKICKING") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_CDLKICKING(e, e, g_open, g_high, g_low, g_close, &begIdx, &nb, g_outIntBuf0);
                acc += (double)g_outIntBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_CDLKICKING_Lookback();
        TA_CDLKICKING_Stream *st = NULL;
            int iv0 = 0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CDLKICKING_Open(g_open, g_high, g_low, g_close, g_nPoints, &st, &iv0);
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
            printf("CDLKICKING %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLKICKING_Close(st); }
            printf("CDLKICKING %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLKICKINGBYLENGTH") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_CDLKICKINGBYLENGTH(e, e, g_open, g_high, g_low, g_close, &begIdx, &nb, g_outIntBuf0);
                acc += (double)g_outIntBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_CDLKICKINGBYLENGTH_Lookback();
        TA_CDLKICKINGBYLENGTH_Stream *st = NULL;
            int iv0 = 0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CDLKICKINGBYLENGTH_Open(g_open, g_high, g_low, g_close, g_nPoints, &st, &iv0);
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
            printf("CDLKICKINGBYLENGTH %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLKICKINGBYLENGTH_Close(st); }
            printf("CDLKICKINGBYLENGTH %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLLADDERBOTTOM") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_CDLLADDERBOTTOM(e, e, g_open, g_high, g_low, g_close, &begIdx, &nb, g_outIntBuf0);
                acc += (double)g_outIntBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_CDLLADDERBOTTOM_Lookback();
        TA_CDLLADDERBOTTOM_Stream *st = NULL;
            int iv0 = 0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CDLLADDERBOTTOM_Open(g_open, g_high, g_low, g_close, g_nPoints, &st, &iv0);
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
            printf("CDLLADDERBOTTOM %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLLADDERBOTTOM_Close(st); }
            printf("CDLLADDERBOTTOM %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLLONGLEGGEDDOJI") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_CDLLONGLEGGEDDOJI(e, e, g_open, g_high, g_low, g_close, &begIdx, &nb, g_outIntBuf0);
                acc += (double)g_outIntBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_CDLLONGLEGGEDDOJI_Lookback();
        TA_CDLLONGLEGGEDDOJI_Stream *st = NULL;
            int iv0 = 0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CDLLONGLEGGEDDOJI_Open(g_open, g_high, g_low, g_close, g_nPoints, &st, &iv0);
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
            printf("CDLLONGLEGGEDDOJI %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLLONGLEGGEDDOJI_Close(st); }
            printf("CDLLONGLEGGEDDOJI %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLLONGLINE") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_CDLLONGLINE(e, e, g_open, g_high, g_low, g_close, &begIdx, &nb, g_outIntBuf0);
                acc += (double)g_outIntBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_CDLLONGLINE_Lookback();
        TA_CDLLONGLINE_Stream *st = NULL;
            int iv0 = 0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CDLLONGLINE_Open(g_open, g_high, g_low, g_close, g_nPoints, &st, &iv0);
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
            printf("CDLLONGLINE %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLLONGLINE_Close(st); }
            printf("CDLLONGLINE %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLMARUBOZU") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_CDLMARUBOZU(e, e, g_open, g_high, g_low, g_close, &begIdx, &nb, g_outIntBuf0);
                acc += (double)g_outIntBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_CDLMARUBOZU_Lookback();
        TA_CDLMARUBOZU_Stream *st = NULL;
            int iv0 = 0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CDLMARUBOZU_Open(g_open, g_high, g_low, g_close, g_nPoints, &st, &iv0);
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
            printf("CDLMARUBOZU %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLMARUBOZU_Close(st); }
            printf("CDLMARUBOZU %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLMATCHINGLOW") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_CDLMATCHINGLOW(e, e, g_open, g_high, g_low, g_close, &begIdx, &nb, g_outIntBuf0);
                acc += (double)g_outIntBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_CDLMATCHINGLOW_Lookback();
        TA_CDLMATCHINGLOW_Stream *st = NULL;
            int iv0 = 0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CDLMATCHINGLOW_Open(g_open, g_high, g_low, g_close, g_nPoints, &st, &iv0);
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
            printf("CDLMATCHINGLOW %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLMATCHINGLOW_Close(st); }
            printf("CDLMATCHINGLOW %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLMATHOLD") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_CDLMATHOLD(e, e, g_open, g_high, g_low, g_close, 0.500000000000000, &begIdx, &nb, g_outIntBuf0);
                acc += (double)g_outIntBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_CDLMATHOLD_Lookback(0.500000000000000);
        TA_CDLMATHOLD_Stream *st = NULL;
            int iv0 = 0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CDLMATHOLD_Open(0.500000000000000, g_open, g_high, g_low, g_close, g_nPoints, &st, &iv0);
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
            printf("CDLMATHOLD %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLMATHOLD_Close(st); }
            printf("CDLMATHOLD %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLMORNINGDOJISTAR") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_CDLMORNINGDOJISTAR(e, e, g_open, g_high, g_low, g_close, 0.300000000000000, &begIdx, &nb, g_outIntBuf0);
                acc += (double)g_outIntBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_CDLMORNINGDOJISTAR_Lookback(0.300000000000000);
        TA_CDLMORNINGDOJISTAR_Stream *st = NULL;
            int iv0 = 0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CDLMORNINGDOJISTAR_Open(0.300000000000000, g_open, g_high, g_low, g_close, g_nPoints, &st, &iv0);
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
            printf("CDLMORNINGDOJISTAR %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLMORNINGDOJISTAR_Close(st); }
            printf("CDLMORNINGDOJISTAR %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLMORNINGSTAR") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_CDLMORNINGSTAR(e, e, g_open, g_high, g_low, g_close, 0.300000000000000, &begIdx, &nb, g_outIntBuf0);
                acc += (double)g_outIntBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_CDLMORNINGSTAR_Lookback(0.300000000000000);
        TA_CDLMORNINGSTAR_Stream *st = NULL;
            int iv0 = 0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CDLMORNINGSTAR_Open(0.300000000000000, g_open, g_high, g_low, g_close, g_nPoints, &st, &iv0);
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
            printf("CDLMORNINGSTAR %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLMORNINGSTAR_Close(st); }
            printf("CDLMORNINGSTAR %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLONNECK") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_CDLONNECK(e, e, g_open, g_high, g_low, g_close, &begIdx, &nb, g_outIntBuf0);
                acc += (double)g_outIntBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_CDLONNECK_Lookback();
        TA_CDLONNECK_Stream *st = NULL;
            int iv0 = 0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CDLONNECK_Open(g_open, g_high, g_low, g_close, g_nPoints, &st, &iv0);
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
            printf("CDLONNECK %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLONNECK_Close(st); }
            printf("CDLONNECK %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLPIERCING") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_CDLPIERCING(e, e, g_open, g_high, g_low, g_close, &begIdx, &nb, g_outIntBuf0);
                acc += (double)g_outIntBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_CDLPIERCING_Lookback();
        TA_CDLPIERCING_Stream *st = NULL;
            int iv0 = 0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CDLPIERCING_Open(g_open, g_high, g_low, g_close, g_nPoints, &st, &iv0);
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
            printf("CDLPIERCING %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLPIERCING_Close(st); }
            printf("CDLPIERCING %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLRICKSHAWMAN") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_CDLRICKSHAWMAN(e, e, g_open, g_high, g_low, g_close, &begIdx, &nb, g_outIntBuf0);
                acc += (double)g_outIntBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_CDLRICKSHAWMAN_Lookback();
        TA_CDLRICKSHAWMAN_Stream *st = NULL;
            int iv0 = 0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CDLRICKSHAWMAN_Open(g_open, g_high, g_low, g_close, g_nPoints, &st, &iv0);
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
            printf("CDLRICKSHAWMAN %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLRICKSHAWMAN_Close(st); }
            printf("CDLRICKSHAWMAN %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLRISEFALL3METHODS") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_CDLRISEFALL3METHODS(e, e, g_open, g_high, g_low, g_close, &begIdx, &nb, g_outIntBuf0);
                acc += (double)g_outIntBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_CDLRISEFALL3METHODS_Lookback();
        TA_CDLRISEFALL3METHODS_Stream *st = NULL;
            int iv0 = 0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CDLRISEFALL3METHODS_Open(g_open, g_high, g_low, g_close, g_nPoints, &st, &iv0);
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
            printf("CDLRISEFALL3METHODS %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLRISEFALL3METHODS_Close(st); }
            printf("CDLRISEFALL3METHODS %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLSEPARATINGLINES") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_CDLSEPARATINGLINES(e, e, g_open, g_high, g_low, g_close, &begIdx, &nb, g_outIntBuf0);
                acc += (double)g_outIntBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_CDLSEPARATINGLINES_Lookback();
        TA_CDLSEPARATINGLINES_Stream *st = NULL;
            int iv0 = 0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CDLSEPARATINGLINES_Open(g_open, g_high, g_low, g_close, g_nPoints, &st, &iv0);
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
            printf("CDLSEPARATINGLINES %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLSEPARATINGLINES_Close(st); }
            printf("CDLSEPARATINGLINES %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLSHOOTINGSTAR") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_CDLSHOOTINGSTAR(e, e, g_open, g_high, g_low, g_close, &begIdx, &nb, g_outIntBuf0);
                acc += (double)g_outIntBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_CDLSHOOTINGSTAR_Lookback();
        TA_CDLSHOOTINGSTAR_Stream *st = NULL;
            int iv0 = 0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CDLSHOOTINGSTAR_Open(g_open, g_high, g_low, g_close, g_nPoints, &st, &iv0);
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
            printf("CDLSHOOTINGSTAR %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLSHOOTINGSTAR_Close(st); }
            printf("CDLSHOOTINGSTAR %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLSHORTLINE") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_CDLSHORTLINE(e, e, g_open, g_high, g_low, g_close, &begIdx, &nb, g_outIntBuf0);
                acc += (double)g_outIntBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_CDLSHORTLINE_Lookback();
        TA_CDLSHORTLINE_Stream *st = NULL;
            int iv0 = 0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CDLSHORTLINE_Open(g_open, g_high, g_low, g_close, g_nPoints, &st, &iv0);
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
            printf("CDLSHORTLINE %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLSHORTLINE_Close(st); }
            printf("CDLSHORTLINE %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLSPINNINGTOP") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_CDLSPINNINGTOP(e, e, g_open, g_high, g_low, g_close, &begIdx, &nb, g_outIntBuf0);
                acc += (double)g_outIntBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_CDLSPINNINGTOP_Lookback();
        TA_CDLSPINNINGTOP_Stream *st = NULL;
            int iv0 = 0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CDLSPINNINGTOP_Open(g_open, g_high, g_low, g_close, g_nPoints, &st, &iv0);
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
            printf("CDLSPINNINGTOP %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLSPINNINGTOP_Close(st); }
            printf("CDLSPINNINGTOP %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLSTALLEDPATTERN") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_CDLSTALLEDPATTERN(e, e, g_open, g_high, g_low, g_close, &begIdx, &nb, g_outIntBuf0);
                acc += (double)g_outIntBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_CDLSTALLEDPATTERN_Lookback();
        TA_CDLSTALLEDPATTERN_Stream *st = NULL;
            int iv0 = 0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CDLSTALLEDPATTERN_Open(g_open, g_high, g_low, g_close, g_nPoints, &st, &iv0);
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
            printf("CDLSTALLEDPATTERN %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLSTALLEDPATTERN_Close(st); }
            printf("CDLSTALLEDPATTERN %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLSTICKSANDWICH") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_CDLSTICKSANDWICH(e, e, g_open, g_high, g_low, g_close, &begIdx, &nb, g_outIntBuf0);
                acc += (double)g_outIntBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_CDLSTICKSANDWICH_Lookback();
        TA_CDLSTICKSANDWICH_Stream *st = NULL;
            int iv0 = 0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CDLSTICKSANDWICH_Open(g_open, g_high, g_low, g_close, g_nPoints, &st, &iv0);
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
            printf("CDLSTICKSANDWICH %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLSTICKSANDWICH_Close(st); }
            printf("CDLSTICKSANDWICH %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLTAKURI") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_CDLTAKURI(e, e, g_open, g_high, g_low, g_close, &begIdx, &nb, g_outIntBuf0);
                acc += (double)g_outIntBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_CDLTAKURI_Lookback();
        TA_CDLTAKURI_Stream *st = NULL;
            int iv0 = 0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CDLTAKURI_Open(g_open, g_high, g_low, g_close, g_nPoints, &st, &iv0);
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
            printf("CDLTAKURI %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLTAKURI_Close(st); }
            printf("CDLTAKURI %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLTASUKIGAP") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_CDLTASUKIGAP(e, e, g_open, g_high, g_low, g_close, &begIdx, &nb, g_outIntBuf0);
                acc += (double)g_outIntBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_CDLTASUKIGAP_Lookback();
        TA_CDLTASUKIGAP_Stream *st = NULL;
            int iv0 = 0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CDLTASUKIGAP_Open(g_open, g_high, g_low, g_close, g_nPoints, &st, &iv0);
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
            printf("CDLTASUKIGAP %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLTASUKIGAP_Close(st); }
            printf("CDLTASUKIGAP %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLTHRUSTING") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_CDLTHRUSTING(e, e, g_open, g_high, g_low, g_close, &begIdx, &nb, g_outIntBuf0);
                acc += (double)g_outIntBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_CDLTHRUSTING_Lookback();
        TA_CDLTHRUSTING_Stream *st = NULL;
            int iv0 = 0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CDLTHRUSTING_Open(g_open, g_high, g_low, g_close, g_nPoints, &st, &iv0);
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
            printf("CDLTHRUSTING %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLTHRUSTING_Close(st); }
            printf("CDLTHRUSTING %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLTRISTAR") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_CDLTRISTAR(e, e, g_open, g_high, g_low, g_close, &begIdx, &nb, g_outIntBuf0);
                acc += (double)g_outIntBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_CDLTRISTAR_Lookback();
        TA_CDLTRISTAR_Stream *st = NULL;
            int iv0 = 0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CDLTRISTAR_Open(g_open, g_high, g_low, g_close, g_nPoints, &st, &iv0);
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
            printf("CDLTRISTAR %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLTRISTAR_Close(st); }
            printf("CDLTRISTAR %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLUNIQUE3RIVER") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_CDLUNIQUE3RIVER(e, e, g_open, g_high, g_low, g_close, &begIdx, &nb, g_outIntBuf0);
                acc += (double)g_outIntBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_CDLUNIQUE3RIVER_Lookback();
        TA_CDLUNIQUE3RIVER_Stream *st = NULL;
            int iv0 = 0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CDLUNIQUE3RIVER_Open(g_open, g_high, g_low, g_close, g_nPoints, &st, &iv0);
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
            printf("CDLUNIQUE3RIVER %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLUNIQUE3RIVER_Close(st); }
            printf("CDLUNIQUE3RIVER %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLUPSIDEGAP2CROWS") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_CDLUPSIDEGAP2CROWS(e, e, g_open, g_high, g_low, g_close, &begIdx, &nb, g_outIntBuf0);
                acc += (double)g_outIntBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_CDLUPSIDEGAP2CROWS_Lookback();
        TA_CDLUPSIDEGAP2CROWS_Stream *st = NULL;
            int iv0 = 0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CDLUPSIDEGAP2CROWS_Open(g_open, g_high, g_low, g_close, g_nPoints, &st, &iv0);
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
            printf("CDLUPSIDEGAP2CROWS %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLUPSIDEGAP2CROWS_Close(st); }
            printf("CDLUPSIDEGAP2CROWS %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLXSIDEGAP3METHODS") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_CDLXSIDEGAP3METHODS(e, e, g_open, g_high, g_low, g_close, &begIdx, &nb, g_outIntBuf0);
                acc += (double)g_outIntBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_CDLXSIDEGAP3METHODS_Lookback();
        TA_CDLXSIDEGAP3METHODS_Stream *st = NULL;
            int iv0 = 0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CDLXSIDEGAP3METHODS_Open(g_open, g_high, g_low, g_close, g_nPoints, &st, &iv0);
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
            printf("CDLXSIDEGAP3METHODS %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLXSIDEGAP3METHODS_Close(st); }
            printf("CDLXSIDEGAP3METHODS %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CEIL") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_CEIL(e, e, g_close, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_CEIL_Lookback();
        TA_CEIL_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CEIL_Open(g_close, g_nPoints, &st, &v0);
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
            printf("CEIL %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CEIL_Close(st); }
            printf("CEIL %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CMO") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_CMO(e, e, g_close, 14, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_CMO_Lookback(14);
        TA_CMO_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CMO_Open(14, g_close, g_nPoints, &st, &v0);
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
            printf("CMO %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CMO_Close(st); }
            printf("CMO %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CORREL") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_CORREL(e, e, g_close, g_high, 30, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_CORREL_Lookback(30);
        TA_CORREL_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CORREL_Open(30, g_close, g_high, g_nPoints, &st, &v0);
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
            printf("CORREL %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CORREL_Close(st); }
            printf("CORREL %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "COS") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_COS(e, e, g_close, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_COS_Lookback();
        TA_COS_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_COS_Open(g_close, g_nPoints, &st, &v0);
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
            printf("COS %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_COS_Close(st); }
            printf("COS %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "COSH") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_COSH(e, e, g_close, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_COSH_Lookback();
        TA_COSH_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_COSH_Open(g_close, g_nPoints, &st, &v0);
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
            printf("COSH %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_COSH_Close(st); }
            printf("COSH %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "DEMA") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_DEMA(e, e, g_close, 30, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_DEMA_Lookback(30);
        TA_DEMA_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_DEMA_Open(30, g_close, g_nPoints, &st, &v0);
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
            printf("DEMA %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_DEMA_Close(st); }
            printf("DEMA %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "DIV") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_DIV(e, e, g_close, g_high, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_DIV_Lookback();
        TA_DIV_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_DIV_Open(g_close, g_high, g_nPoints, &st, &v0);
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
            printf("DIV %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_DIV_Close(st); }
            printf("DIV %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "DX") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_DX(e, e, g_high, g_low, g_close, 14, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_DX_Lookback(14);
        TA_DX_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_DX_Open(14, g_high, g_low, g_close, g_nPoints, &st, &v0);
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
            printf("DX %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_DX_Close(st); }
            printf("DX %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "EMA") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_EMA(e, e, g_close, 30, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_EMA_Lookback(30);
        TA_EMA_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_EMA_Open(30, g_close, g_nPoints, &st, &v0);
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
            printf("EMA %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_EMA_Close(st); }
            printf("EMA %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "EXP") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_EXP(e, e, g_close, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_EXP_Lookback();
        TA_EXP_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_EXP_Open(g_close, g_nPoints, &st, &v0);
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
            printf("EXP %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_EXP_Close(st); }
            printf("EXP %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "FLOOR") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_FLOOR(e, e, g_close, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_FLOOR_Lookback();
        TA_FLOOR_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_FLOOR_Open(g_close, g_nPoints, &st, &v0);
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
            printf("FLOOR %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_FLOOR_Close(st); }
            printf("FLOOR %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "HT_DCPERIOD") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_HT_DCPERIOD(e, e, g_close, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_HT_DCPERIOD_Lookback();
        TA_HT_DCPERIOD_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_HT_DCPERIOD_Open(g_close, g_nPoints, &st, &v0);
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
            printf("HT_DCPERIOD %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_HT_DCPERIOD_Close(st); }
            printf("HT_DCPERIOD %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "HT_DCPHASE") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_HT_DCPHASE(e, e, g_close, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_HT_DCPHASE_Lookback();
        TA_HT_DCPHASE_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_HT_DCPHASE_Open(g_close, g_nPoints, &st, &v0);
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
            printf("HT_DCPHASE %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_HT_DCPHASE_Close(st); }
            printf("HT_DCPHASE %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "HT_PHASOR") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_HT_PHASOR(e, e, g_close, &begIdx, &nb, g_outBuf0, g_outBuf1);
                acc += g_outBuf0[0];
                acc += g_outBuf1[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_HT_PHASOR_Lookback();
        TA_HT_PHASOR_Stream *st = NULL;
            double v0 = 0.0;
            double v1 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_HT_PHASOR_Open(g_close, g_nPoints, &st, &v0, &v1);
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
            printf("HT_PHASOR %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_HT_PHASOR_Close(st); }
            printf("HT_PHASOR %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "HT_SINE") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_HT_SINE(e, e, g_close, &begIdx, &nb, g_outBuf0, g_outBuf1);
                acc += g_outBuf0[0];
                acc += g_outBuf1[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_HT_SINE_Lookback();
        TA_HT_SINE_Stream *st = NULL;
            double v0 = 0.0;
            double v1 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_HT_SINE_Open(g_close, g_nPoints, &st, &v0, &v1);
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
            printf("HT_SINE %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_HT_SINE_Close(st); }
            printf("HT_SINE %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "HT_TRENDLINE") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_HT_TRENDLINE(e, e, g_close, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_HT_TRENDLINE_Lookback();
        TA_HT_TRENDLINE_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_HT_TRENDLINE_Open(g_close, g_nPoints, &st, &v0);
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
            printf("HT_TRENDLINE %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_HT_TRENDLINE_Close(st); }
            printf("HT_TRENDLINE %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "HT_TRENDMODE") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_HT_TRENDMODE(e, e, g_close, &begIdx, &nb, g_outIntBuf0);
                acc += (double)g_outIntBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_HT_TRENDMODE_Lookback();
        TA_HT_TRENDMODE_Stream *st = NULL;
            int iv0 = 0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_HT_TRENDMODE_Open(g_close, g_nPoints, &st, &iv0);
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
            printf("HT_TRENDMODE %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_HT_TRENDMODE_Close(st); }
            printf("HT_TRENDMODE %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "IMI") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_IMI(e, e, g_open, g_close, 14, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_IMI_Lookback(14);
        TA_IMI_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_IMI_Open(14, g_open, g_close, g_nPoints, &st, &v0);
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
            printf("IMI %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_IMI_Close(st); }
            printf("IMI %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "KAMA") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_KAMA(e, e, g_close, 30, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_KAMA_Lookback(30);
        TA_KAMA_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_KAMA_Open(30, g_close, g_nPoints, &st, &v0);
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
            printf("KAMA %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_KAMA_Close(st); }
            printf("KAMA %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "LINEARREG") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_LINEARREG(e, e, g_close, 14, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_LINEARREG_Lookback(14);
        TA_LINEARREG_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_LINEARREG_Open(14, g_close, g_nPoints, &st, &v0);
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
            printf("LINEARREG %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_LINEARREG_Close(st); }
            printf("LINEARREG %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "LINEARREG_ANGLE") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_LINEARREG_ANGLE(e, e, g_close, 14, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_LINEARREG_ANGLE_Lookback(14);
        TA_LINEARREG_ANGLE_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_LINEARREG_ANGLE_Open(14, g_close, g_nPoints, &st, &v0);
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
            printf("LINEARREG_ANGLE %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_LINEARREG_ANGLE_Close(st); }
            printf("LINEARREG_ANGLE %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "LINEARREG_INTERCEPT") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_LINEARREG_INTERCEPT(e, e, g_close, 14, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_LINEARREG_INTERCEPT_Lookback(14);
        TA_LINEARREG_INTERCEPT_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_LINEARREG_INTERCEPT_Open(14, g_close, g_nPoints, &st, &v0);
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
            printf("LINEARREG_INTERCEPT %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_LINEARREG_INTERCEPT_Close(st); }
            printf("LINEARREG_INTERCEPT %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "LINEARREG_SLOPE") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_LINEARREG_SLOPE(e, e, g_close, 14, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_LINEARREG_SLOPE_Lookback(14);
        TA_LINEARREG_SLOPE_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_LINEARREG_SLOPE_Open(14, g_close, g_nPoints, &st, &v0);
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
            printf("LINEARREG_SLOPE %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_LINEARREG_SLOPE_Close(st); }
            printf("LINEARREG_SLOPE %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "LN") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_LN(e, e, g_close, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_LN_Lookback();
        TA_LN_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_LN_Open(g_close, g_nPoints, &st, &v0);
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
            printf("LN %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_LN_Close(st); }
            printf("LN %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "LOG10") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_LOG10(e, e, g_close, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_LOG10_Lookback();
        TA_LOG10_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_LOG10_Open(g_close, g_nPoints, &st, &v0);
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
            printf("LOG10 %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_LOG10_Close(st); }
            printf("LOG10 %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "MA") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_MA(e, e, g_close, 30, 0, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_MA_Lookback(30, 0);
        TA_MA_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_MA_Open(30, 0, g_close, g_nPoints, &st, &v0);
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
            printf("MA %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_MA_Close(st); }
            printf("MA %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "MACD") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_MACD(e, e, g_close, 12, 26, 9, &begIdx, &nb, g_outBuf0, g_outBuf1, g_outBuf2);
                acc += g_outBuf0[0];
                acc += g_outBuf1[0];
                acc += g_outBuf2[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_MACD_Lookback(12, 26, 9);
        TA_MACD_Stream *st = NULL;
            double v0 = 0.0;
            double v1 = 0.0;
            double v2 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_MACD_Open(12, 26, 9, g_close, g_nPoints, &st, &v0, &v1, &v2);
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
            printf("MACD %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_MACD_Close(st); }
            printf("MACD %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "MACDEXT") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_MACDEXT(e, e, g_close, 12, 0, 26, 0, 9, 0, &begIdx, &nb, g_outBuf0, g_outBuf1, g_outBuf2);
                acc += g_outBuf0[0];
                acc += g_outBuf1[0];
                acc += g_outBuf2[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_MACDEXT_Lookback(12, 0, 26, 0, 9, 0);
        TA_MACDEXT_Stream *st = NULL;
            double v0 = 0.0;
            double v1 = 0.0;
            double v2 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_MACDEXT_Open(12, 0, 26, 0, 9, 0, g_close, g_nPoints, &st, &v0, &v1, &v2);
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
            printf("MACDEXT %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_MACDEXT_Close(st); }
            printf("MACDEXT %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "MACDFIX") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_MACDFIX(e, e, g_close, 9, &begIdx, &nb, g_outBuf0, g_outBuf1, g_outBuf2);
                acc += g_outBuf0[0];
                acc += g_outBuf1[0];
                acc += g_outBuf2[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_MACDFIX_Lookback(9);
        TA_MACDFIX_Stream *st = NULL;
            double v0 = 0.0;
            double v1 = 0.0;
            double v2 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_MACDFIX_Open(9, g_close, g_nPoints, &st, &v0, &v1, &v2);
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
            printf("MACDFIX %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_MACDFIX_Close(st); }
            printf("MACDFIX %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "MAMA") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_MAMA(e, e, g_close, 0.500000000000000, 0.050000000000000, &begIdx, &nb, g_outBuf0, g_outBuf1);
                acc += g_outBuf0[0];
                acc += g_outBuf1[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_MAMA_Lookback(0.500000000000000, 0.050000000000000);
        TA_MAMA_Stream *st = NULL;
            double v0 = 0.0;
            double v1 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_MAMA_Open(0.500000000000000, 0.050000000000000, g_close, g_nPoints, &st, &v0, &v1);
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
            printf("MAMA %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_MAMA_Close(st); }
            printf("MAMA %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "MAVP") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_MAVP(e, e, g_close, g_high, 2, 30, 0, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_MAVP_Lookback(2, 30, 0);
        TA_MAVP_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_MAVP_Open(2, 30, 0, g_close, g_high, g_nPoints, &st, &v0);
        g_ta_track = 0; handle_bytes = g_ta_live_bytes;
        if( orc == TA_SUCCESS && st ) {
            int blk = (iters >= 64) ? 32 : 1;
            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;
            for( int pass = 0; pass < 3; pass++ ) {
                long long t0 = get_nanotime();
                for( int it = 0; it < iters; it++ ) {
                    TA_MAVP_Update(st, g_close[it & BENCH_MASK], g_high[it & BENCH_MASK], &v0);
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
                        TA_MAVP_Peek(st, g_close[it & BENCH_MASK], g_high[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                    tp += get_nanotime() - t0;
                    for( int j = 0; j < blk; j++ ) {
                        int it = b * blk + j;
                        TA_MAVP_Update(st, g_close[it & BENCH_MASK], g_high[it & BENCH_MASK], &v0);
                        acc += v0;
                    }
                }
                if( best_p < 0 || tp < best_p ) best_p = tp;
            }
            g_sink += (int)acc + nb;
            TA_MAVP_Close(st);
            printf("MAVP %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_MAVP_Close(st); }
            printf("MAVP %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "MAX") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_MAX(e, e, g_close, 30, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_MAX_Lookback(30);
        TA_MAX_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_MAX_Open(30, g_close, g_nPoints, &st, &v0);
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
            printf("MAX %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_MAX_Close(st); }
            printf("MAX %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "MAXINDEX") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_MAXINDEX(e, e, g_close, 30, &begIdx, &nb, g_outIntBuf0);
                acc += (double)g_outIntBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_MAXINDEX_Lookback(30);
        TA_MAXINDEX_Stream *st = NULL;
            int iv0 = 0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_MAXINDEX_Open(30, g_close, g_nPoints, &st, &iv0);
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
            printf("MAXINDEX %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_MAXINDEX_Close(st); }
            printf("MAXINDEX %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "MEDPRICE") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_MEDPRICE(e, e, g_high, g_low, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_MEDPRICE_Lookback();
        TA_MEDPRICE_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_MEDPRICE_Open(g_high, g_low, g_nPoints, &st, &v0);
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
            printf("MEDPRICE %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_MEDPRICE_Close(st); }
            printf("MEDPRICE %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "MFI") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_MFI(e, e, g_high, g_low, g_close, g_volume, 14, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_MFI_Lookback(14);
        TA_MFI_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_MFI_Open(14, g_high, g_low, g_close, g_volume, g_nPoints, &st, &v0);
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
            printf("MFI %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_MFI_Close(st); }
            printf("MFI %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "MIDPOINT") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_MIDPOINT(e, e, g_close, 14, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_MIDPOINT_Lookback(14);
        TA_MIDPOINT_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_MIDPOINT_Open(14, g_close, g_nPoints, &st, &v0);
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
            printf("MIDPOINT %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_MIDPOINT_Close(st); }
            printf("MIDPOINT %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "MIDPRICE") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_MIDPRICE(e, e, g_high, g_low, 14, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_MIDPRICE_Lookback(14);
        TA_MIDPRICE_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_MIDPRICE_Open(14, g_high, g_low, g_nPoints, &st, &v0);
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
            printf("MIDPRICE %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_MIDPRICE_Close(st); }
            printf("MIDPRICE %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "MIN") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_MIN(e, e, g_close, 30, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_MIN_Lookback(30);
        TA_MIN_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_MIN_Open(30, g_close, g_nPoints, &st, &v0);
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
            printf("MIN %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_MIN_Close(st); }
            printf("MIN %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "MININDEX") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_MININDEX(e, e, g_close, 30, &begIdx, &nb, g_outIntBuf0);
                acc += (double)g_outIntBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_MININDEX_Lookback(30);
        TA_MININDEX_Stream *st = NULL;
            int iv0 = 0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_MININDEX_Open(30, g_close, g_nPoints, &st, &iv0);
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
            printf("MININDEX %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_MININDEX_Close(st); }
            printf("MININDEX %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "MINMAX") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_MINMAX(e, e, g_close, 30, &begIdx, &nb, g_outBuf0, g_outBuf1);
                acc += g_outBuf0[0];
                acc += g_outBuf1[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_MINMAX_Lookback(30);
        TA_MINMAX_Stream *st = NULL;
            double v0 = 0.0;
            double v1 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_MINMAX_Open(30, g_close, g_nPoints, &st, &v0, &v1);
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
            printf("MINMAX %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_MINMAX_Close(st); }
            printf("MINMAX %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "MINMAXINDEX") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_MINMAXINDEX(e, e, g_close, 30, &begIdx, &nb, g_outIntBuf0, g_outIntBuf1);
                acc += (double)g_outIntBuf0[0];
                acc += (double)g_outIntBuf1[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_MINMAXINDEX_Lookback(30);
        TA_MINMAXINDEX_Stream *st = NULL;
            int iv0 = 0;
            int iv1 = 0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_MINMAXINDEX_Open(30, g_close, g_nPoints, &st, &iv0, &iv1);
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
            printf("MINMAXINDEX %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_MINMAXINDEX_Close(st); }
            printf("MINMAXINDEX %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "MINUS_DI") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_MINUS_DI(e, e, g_high, g_low, g_close, 14, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_MINUS_DI_Lookback(14);
        TA_MINUS_DI_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_MINUS_DI_Open(14, g_high, g_low, g_close, g_nPoints, &st, &v0);
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
            printf("MINUS_DI %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_MINUS_DI_Close(st); }
            printf("MINUS_DI %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "MINUS_DM") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_MINUS_DM(e, e, g_high, g_low, 14, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_MINUS_DM_Lookback(14);
        TA_MINUS_DM_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_MINUS_DM_Open(14, g_high, g_low, g_nPoints, &st, &v0);
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
            printf("MINUS_DM %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_MINUS_DM_Close(st); }
            printf("MINUS_DM %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "MOM") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_MOM(e, e, g_close, 10, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_MOM_Lookback(10);
        TA_MOM_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_MOM_Open(10, g_close, g_nPoints, &st, &v0);
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
            printf("MOM %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_MOM_Close(st); }
            printf("MOM %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "MULT") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_MULT(e, e, g_close, g_high, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_MULT_Lookback();
        TA_MULT_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_MULT_Open(g_close, g_high, g_nPoints, &st, &v0);
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
            printf("MULT %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_MULT_Close(st); }
            printf("MULT %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "NATR") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_NATR(e, e, g_high, g_low, g_close, 14, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_NATR_Lookback(14);
        TA_NATR_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_NATR_Open(14, g_high, g_low, g_close, g_nPoints, &st, &v0);
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
            printf("NATR %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_NATR_Close(st); }
            printf("NATR %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "OBV") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_OBV(e, e, g_close, g_volume, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_OBV_Lookback();
        TA_OBV_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_OBV_Open(g_close, g_volume, g_nPoints, &st, &v0);
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
            printf("OBV %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_OBV_Close(st); }
            printf("OBV %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "PLUS_DI") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_PLUS_DI(e, e, g_high, g_low, g_close, 14, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_PLUS_DI_Lookback(14);
        TA_PLUS_DI_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_PLUS_DI_Open(14, g_high, g_low, g_close, g_nPoints, &st, &v0);
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
            printf("PLUS_DI %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_PLUS_DI_Close(st); }
            printf("PLUS_DI %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "PLUS_DM") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_PLUS_DM(e, e, g_high, g_low, 14, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_PLUS_DM_Lookback(14);
        TA_PLUS_DM_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_PLUS_DM_Open(14, g_high, g_low, g_nPoints, &st, &v0);
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
            printf("PLUS_DM %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_PLUS_DM_Close(st); }
            printf("PLUS_DM %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "PPO") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_PPO(e, e, g_close, 12, 26, 0, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_PPO_Lookback(12, 26, 0);
        TA_PPO_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_PPO_Open(12, 26, 0, g_close, g_nPoints, &st, &v0);
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
            printf("PPO %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_PPO_Close(st); }
            printf("PPO %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "ROC") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_ROC(e, e, g_close, 10, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_ROC_Lookback(10);
        TA_ROC_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_ROC_Open(10, g_close, g_nPoints, &st, &v0);
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
            printf("ROC %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_ROC_Close(st); }
            printf("ROC %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "ROCP") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_ROCP(e, e, g_close, 10, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_ROCP_Lookback(10);
        TA_ROCP_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_ROCP_Open(10, g_close, g_nPoints, &st, &v0);
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
            printf("ROCP %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_ROCP_Close(st); }
            printf("ROCP %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "ROCR") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_ROCR(e, e, g_close, 10, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_ROCR_Lookback(10);
        TA_ROCR_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_ROCR_Open(10, g_close, g_nPoints, &st, &v0);
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
            printf("ROCR %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_ROCR_Close(st); }
            printf("ROCR %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "ROCR100") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_ROCR100(e, e, g_close, 10, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_ROCR100_Lookback(10);
        TA_ROCR100_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_ROCR100_Open(10, g_close, g_nPoints, &st, &v0);
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
            printf("ROCR100 %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_ROCR100_Close(st); }
            printf("ROCR100 %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "RSI") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_RSI(e, e, g_close, 14, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_RSI_Lookback(14);
        TA_RSI_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_RSI_Open(14, g_close, g_nPoints, &st, &v0);
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
            printf("RSI %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_RSI_Close(st); }
            printf("RSI %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "SAR") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_SAR(e, e, g_high, g_low, 0.020000000000000, 0.200000000000000, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_SAR_Lookback(0.020000000000000, 0.200000000000000);
        TA_SAR_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_SAR_Open(0.020000000000000, 0.200000000000000, g_high, g_low, g_nPoints, &st, &v0);
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
            printf("SAR %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_SAR_Close(st); }
            printf("SAR %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "SAREXT") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_SAREXT(e, e, g_high, g_low, 0.000000000000000, 0.000000000000000, 0.020000000000000, 0.020000000000000, 0.200000000000000, 0.020000000000000, 0.020000000000000, 0.200000000000000, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_SAREXT_Lookback(0.000000000000000, 0.000000000000000, 0.020000000000000, 0.020000000000000, 0.200000000000000, 0.020000000000000, 0.020000000000000, 0.200000000000000);
        TA_SAREXT_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_SAREXT_Open(0.000000000000000, 0.000000000000000, 0.020000000000000, 0.020000000000000, 0.200000000000000, 0.020000000000000, 0.020000000000000, 0.200000000000000, g_high, g_low, g_nPoints, &st, &v0);
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
            printf("SAREXT %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_SAREXT_Close(st); }
            printf("SAREXT %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "SIN") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_SIN(e, e, g_close, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_SIN_Lookback();
        TA_SIN_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_SIN_Open(g_close, g_nPoints, &st, &v0);
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
            printf("SIN %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_SIN_Close(st); }
            printf("SIN %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "SINH") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_SINH(e, e, g_close, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_SINH_Lookback();
        TA_SINH_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_SINH_Open(g_close, g_nPoints, &st, &v0);
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
            printf("SINH %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_SINH_Close(st); }
            printf("SINH %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "SMA") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_SMA(e, e, g_close, 30, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_SMA_Lookback(30);
        TA_SMA_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_SMA_Open(30, g_close, g_nPoints, &st, &v0);
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
            printf("SMA %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_SMA_Close(st); }
            printf("SMA %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "SQRT") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_SQRT(e, e, g_close, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_SQRT_Lookback();
        TA_SQRT_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_SQRT_Open(g_close, g_nPoints, &st, &v0);
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
            printf("SQRT %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_SQRT_Close(st); }
            printf("SQRT %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "STDDEV") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_STDDEV(e, e, g_close, 5, 1.000000000000000, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_STDDEV_Lookback(5, 1.000000000000000);
        TA_STDDEV_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_STDDEV_Open(5, 1.000000000000000, g_close, g_nPoints, &st, &v0);
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
            printf("STDDEV %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_STDDEV_Close(st); }
            printf("STDDEV %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "STOCH") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_STOCH(e, e, g_high, g_low, g_close, 5, 3, 0, 3, 0, &begIdx, &nb, g_outBuf0, g_outBuf1);
                acc += g_outBuf0[0];
                acc += g_outBuf1[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_STOCH_Lookback(5, 3, 0, 3, 0);
        TA_STOCH_Stream *st = NULL;
            double v0 = 0.0;
            double v1 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_STOCH_Open(5, 3, 0, 3, 0, g_high, g_low, g_close, g_nPoints, &st, &v0, &v1);
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
            printf("STOCH %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_STOCH_Close(st); }
            printf("STOCH %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "STOCHF") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_STOCHF(e, e, g_high, g_low, g_close, 5, 3, 0, &begIdx, &nb, g_outBuf0, g_outBuf1);
                acc += g_outBuf0[0];
                acc += g_outBuf1[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_STOCHF_Lookback(5, 3, 0);
        TA_STOCHF_Stream *st = NULL;
            double v0 = 0.0;
            double v1 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_STOCHF_Open(5, 3, 0, g_high, g_low, g_close, g_nPoints, &st, &v0, &v1);
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
            printf("STOCHF %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_STOCHF_Close(st); }
            printf("STOCHF %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "STOCHRSI") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_STOCHRSI(e, e, g_close, 14, 5, 3, 0, &begIdx, &nb, g_outBuf0, g_outBuf1);
                acc += g_outBuf0[0];
                acc += g_outBuf1[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_STOCHRSI_Lookback(14, 5, 3, 0);
        TA_STOCHRSI_Stream *st = NULL;
            double v0 = 0.0;
            double v1 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_STOCHRSI_Open(14, 5, 3, 0, g_close, g_nPoints, &st, &v0, &v1);
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
            printf("STOCHRSI %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_STOCHRSI_Close(st); }
            printf("STOCHRSI %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "SUB") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_SUB(e, e, g_close, g_high, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_SUB_Lookback();
        TA_SUB_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_SUB_Open(g_close, g_high, g_nPoints, &st, &v0);
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
            printf("SUB %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_SUB_Close(st); }
            printf("SUB %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "SUM") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_SUM(e, e, g_close, 30, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_SUM_Lookback(30);
        TA_SUM_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_SUM_Open(30, g_close, g_nPoints, &st, &v0);
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
            printf("SUM %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_SUM_Close(st); }
            printf("SUM %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "T3") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_T3(e, e, g_close, 5, 0.700000000000000, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_T3_Lookback(5, 0.700000000000000);
        TA_T3_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_T3_Open(5, 0.700000000000000, g_close, g_nPoints, &st, &v0);
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
            printf("T3 %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_T3_Close(st); }
            printf("T3 %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "TAN") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_TAN(e, e, g_close, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_TAN_Lookback();
        TA_TAN_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_TAN_Open(g_close, g_nPoints, &st, &v0);
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
            printf("TAN %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_TAN_Close(st); }
            printf("TAN %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "TANH") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_TANH(e, e, g_close, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_TANH_Lookback();
        TA_TANH_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_TANH_Open(g_close, g_nPoints, &st, &v0);
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
            printf("TANH %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_TANH_Close(st); }
            printf("TANH %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "TEMA") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_TEMA(e, e, g_close, 30, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_TEMA_Lookback(30);
        TA_TEMA_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_TEMA_Open(30, g_close, g_nPoints, &st, &v0);
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
            printf("TEMA %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_TEMA_Close(st); }
            printf("TEMA %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "TRANGE") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_TRANGE(e, e, g_high, g_low, g_close, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_TRANGE_Lookback();
        TA_TRANGE_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_TRANGE_Open(g_high, g_low, g_close, g_nPoints, &st, &v0);
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
            printf("TRANGE %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_TRANGE_Close(st); }
            printf("TRANGE %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "TRIMA") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_TRIMA(e, e, g_close, 30, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_TRIMA_Lookback(30);
        TA_TRIMA_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_TRIMA_Open(30, g_close, g_nPoints, &st, &v0);
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
            printf("TRIMA %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_TRIMA_Close(st); }
            printf("TRIMA %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "TRIX") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_TRIX(e, e, g_close, 30, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_TRIX_Lookback(30);
        TA_TRIX_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_TRIX_Open(30, g_close, g_nPoints, &st, &v0);
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
            printf("TRIX %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_TRIX_Close(st); }
            printf("TRIX %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "TSF") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_TSF(e, e, g_close, 14, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_TSF_Lookback(14);
        TA_TSF_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_TSF_Open(14, g_close, g_nPoints, &st, &v0);
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
            printf("TSF %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_TSF_Close(st); }
            printf("TSF %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "TYPPRICE") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_TYPPRICE(e, e, g_high, g_low, g_close, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_TYPPRICE_Lookback();
        TA_TYPPRICE_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_TYPPRICE_Open(g_high, g_low, g_close, g_nPoints, &st, &v0);
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
            printf("TYPPRICE %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_TYPPRICE_Close(st); }
            printf("TYPPRICE %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "ULTOSC") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_ULTOSC(e, e, g_high, g_low, g_close, 7, 14, 28, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_ULTOSC_Lookback(7, 14, 28);
        TA_ULTOSC_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_ULTOSC_Open(7, 14, 28, g_high, g_low, g_close, g_nPoints, &st, &v0);
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
            printf("ULTOSC %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_ULTOSC_Close(st); }
            printf("ULTOSC %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "VAR") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_VAR(e, e, g_close, 5, 1.000000000000000, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_VAR_Lookback(5, 1.000000000000000);
        TA_VAR_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_VAR_Open(5, 1.000000000000000, g_close, g_nPoints, &st, &v0);
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
            printf("VAR %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_VAR_Close(st); }
            printf("VAR %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "WCLPRICE") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_WCLPRICE(e, e, g_high, g_low, g_close, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_WCLPRICE_Lookback();
        TA_WCLPRICE_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_WCLPRICE_Open(g_high, g_low, g_close, g_nPoints, &st, &v0);
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
            printf("WCLPRICE %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_WCLPRICE_Close(st); }
            printf("WCLPRICE %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "WILLR") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_WILLR(e, e, g_high, g_low, g_close, 14, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_WILLR_Lookback(14);
        TA_WILLR_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_WILLR_Open(14, g_high, g_low, g_close, g_nPoints, &st, &v0);
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
            printf("WILLR %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_WILLR_Close(st); }
            printf("WILLR %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "WMA") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        for( int pass = 0; pass < 3; pass++ ) {
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                int e = (g_nPoints - 1) - (it & BENCH_MASK);
                TA_WMA(e, e, g_close, 30, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        int lb = TA_WMA_Lookback(30);
        TA_WMA_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_WMA_Open(30, g_close, g_nPoints, &st, &v0);
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
            printf("WMA %lld %lld %lld %d %zu\n", best_b/iters, best_u/iters, best_p/npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_WMA_Close(st); }
            printf("WMA %lld -1 -1 %d 0\n", best_b/iters, lb);
        }
        fflush(stdout);
    }
}


int main(int argc, char *argv[]) {
    TA_Initialize();
    int n_points = 100000;
    int n_iters = 500;
    const char *func_filter = NULL;
    for( int i = 1; i < argc; i++ ) {
        if( strncmp(argv[i], "--points=", 9) == 0 )    n_points = atoi(argv[i]+9);
        else if( strncmp(argv[i], "--iters=", 8) == 0 ) n_iters = atoi(argv[i]+8);
        else if( strncmp(argv[i], "--function=", 11) == 0 ) func_filter = argv[i]+11;
    }
    if( n_points > MAX_POINTS ) n_points = MAX_POINTS;
    generate_price_data(n_points);
    bench_stream_all(func_filter, n_iters);
    free(g_open); free(g_high); free(g_low); free(g_close); free(g_volume); free(g_oi);
    return 0;
}
