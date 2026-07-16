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

static double *g_rt_open, *g_rt_high, *g_rt_low, *g_rt_close, *g_rt_volume, *g_rt_oi;
static int g_rtCap;


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
        int lb = TA_ACCBANDS_Lookback(20);
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_high[t] = g_high[it & BENCH_MASK];
                g_rt_low[t] = g_low[it & BENCH_MASK];
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_ACCBANDS(t, t, g_rt_high, g_rt_low, g_rt_close, 20, &begIdx, &nb, g_outBuf0, g_outBuf1, g_outBuf2);
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
        TA_RetCode orc = TA_ACCBANDS_Open(&st, g_high, g_low, g_close, g_nPoints, 20, &v0, &v1, &v2);
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
            printf("ACCBANDS %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_ACCBANDS_Close(st); }
            printf("ACCBANDS %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "ACOS") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_ACOS_Lookback();
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
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
            printf("ACOS %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_ACOS_Close(st); }
            printf("ACOS %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "AD") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_AD_Lookback();
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
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
            printf("AD %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_AD_Close(st); }
            printf("AD %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "ADD") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_ADD_Lookback();
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
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
            printf("ADD %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_ADD_Close(st); }
            printf("ADD %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "ADOSC") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_ADOSC_Lookback(3, 10);
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_high[t] = g_high[it & BENCH_MASK];
                g_rt_low[t] = g_low[it & BENCH_MASK];
                g_rt_close[t] = g_close[it & BENCH_MASK];
                g_rt_volume[t] = g_volume[it & BENCH_MASK];
                TA_ADOSC(t, t, g_rt_high, g_rt_low, g_rt_close, g_rt_volume, 3, 10, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_ADOSC_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_ADOSC_Open(&st, g_high, g_low, g_close, g_volume, g_nPoints, 3, 10, &v0);
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
            printf("ADOSC %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_ADOSC_Close(st); }
            printf("ADOSC %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "ADX") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_ADX_Lookback(14);
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_high[t] = g_high[it & BENCH_MASK];
                g_rt_low[t] = g_low[it & BENCH_MASK];
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_ADX(t, t, g_rt_high, g_rt_low, g_rt_close, 14, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_ADX_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_ADX_Open(&st, g_high, g_low, g_close, g_nPoints, 14, &v0);
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
            printf("ADX %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_ADX_Close(st); }
            printf("ADX %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "ADXR") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_ADXR_Lookback(14);
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_high[t] = g_high[it & BENCH_MASK];
                g_rt_low[t] = g_low[it & BENCH_MASK];
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_ADXR(t, t, g_rt_high, g_rt_low, g_rt_close, 14, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_ADXR_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_ADXR_Open(&st, g_high, g_low, g_close, g_nPoints, 14, &v0);
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
            printf("ADXR %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_ADXR_Close(st); }
            printf("ADXR %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "APO") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_APO_Lookback(12, 26, 0);
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_APO(t, t, g_rt_close, 12, 26, 0, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_APO_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_APO_Open(&st, g_close, g_nPoints, 12, 26, 0, &v0);
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
            printf("APO %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_APO_Close(st); }
            printf("APO %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "AROON") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_AROON_Lookback(14);
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_high[t] = g_high[it & BENCH_MASK];
                g_rt_low[t] = g_low[it & BENCH_MASK];
                TA_AROON(t, t, g_rt_high, g_rt_low, 14, &begIdx, &nb, g_outBuf0, g_outBuf1);
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
        TA_RetCode orc = TA_AROON_Open(&st, g_high, g_low, g_nPoints, 14, &v0, &v1);
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
            printf("AROON %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_AROON_Close(st); }
            printf("AROON %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "AROONOSC") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_AROONOSC_Lookback(14);
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_high[t] = g_high[it & BENCH_MASK];
                g_rt_low[t] = g_low[it & BENCH_MASK];
                TA_AROONOSC(t, t, g_rt_high, g_rt_low, 14, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_AROONOSC_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_AROONOSC_Open(&st, g_high, g_low, g_nPoints, 14, &v0);
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
            printf("AROONOSC %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_AROONOSC_Close(st); }
            printf("AROONOSC %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "ASIN") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_ASIN_Lookback();
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
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
            printf("ASIN %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_ASIN_Close(st); }
            printf("ASIN %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "ATAN") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_ATAN_Lookback();
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
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
            printf("ATAN %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_ATAN_Close(st); }
            printf("ATAN %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "ATR") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_ATR_Lookback(14);
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_high[t] = g_high[it & BENCH_MASK];
                g_rt_low[t] = g_low[it & BENCH_MASK];
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_ATR(t, t, g_rt_high, g_rt_low, g_rt_close, 14, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_ATR_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_ATR_Open(&st, g_high, g_low, g_close, g_nPoints, 14, &v0);
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
            printf("ATR %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_ATR_Close(st); }
            printf("ATR %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "AVGDEV") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_AVGDEV_Lookback(14);
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_AVGDEV(t, t, g_rt_close, 14, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_AVGDEV_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_AVGDEV_Open(&st, g_close, g_nPoints, 14, &v0);
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
            printf("AVGDEV %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_AVGDEV_Close(st); }
            printf("AVGDEV %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "AVGPRICE") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_AVGPRICE_Lookback();
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
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
            printf("AVGPRICE %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_AVGPRICE_Close(st); }
            printf("AVGPRICE %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "BBANDS") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_BBANDS_Lookback(5, 2.000000000000000, 2.000000000000000, 0);
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_BBANDS(t, t, g_rt_close, 5, 2.000000000000000, 2.000000000000000, 0, &begIdx, &nb, g_outBuf0, g_outBuf1, g_outBuf2);
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
        TA_RetCode orc = TA_BBANDS_Open(&st, g_close, g_nPoints, 5, 2.000000000000000, 2.000000000000000, 0, &v0, &v1, &v2);
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
            printf("BBANDS %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_BBANDS_Close(st); }
            printf("BBANDS %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "BETA") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_BETA_Lookback(5);
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_close[t] = g_close[it & BENCH_MASK];
                g_rt_high[t] = g_high[it & BENCH_MASK];
                TA_BETA(t, t, g_rt_close, g_rt_high, 5, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_BETA_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_BETA_Open(&st, g_close, g_high, g_nPoints, 5, &v0);
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
            printf("BETA %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_BETA_Close(st); }
            printf("BETA %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "BOP") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_BOP_Lookback();
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
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
            printf("BOP %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_BOP_Close(st); }
            printf("BOP %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CCI") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_CCI_Lookback(14);
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_high[t] = g_high[it & BENCH_MASK];
                g_rt_low[t] = g_low[it & BENCH_MASK];
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_CCI(t, t, g_rt_high, g_rt_low, g_rt_close, 14, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_CCI_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CCI_Open(&st, g_high, g_low, g_close, g_nPoints, 14, &v0);
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
            printf("CCI %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CCI_Close(st); }
            printf("CCI %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDL2CROWS") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_CDL2CROWS_Lookback();
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
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
            printf("CDL2CROWS %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDL2CROWS_Close(st); }
            printf("CDL2CROWS %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDL3BLACKCROWS") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_CDL3BLACKCROWS_Lookback();
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
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
            printf("CDL3BLACKCROWS %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDL3BLACKCROWS_Close(st); }
            printf("CDL3BLACKCROWS %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDL3INSIDE") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_CDL3INSIDE_Lookback();
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
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
            printf("CDL3INSIDE %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDL3INSIDE_Close(st); }
            printf("CDL3INSIDE %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDL3LINESTRIKE") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_CDL3LINESTRIKE_Lookback();
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
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
            printf("CDL3LINESTRIKE %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDL3LINESTRIKE_Close(st); }
            printf("CDL3LINESTRIKE %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDL3OUTSIDE") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_CDL3OUTSIDE_Lookback();
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
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
            printf("CDL3OUTSIDE %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDL3OUTSIDE_Close(st); }
            printf("CDL3OUTSIDE %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDL3STARSINSOUTH") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_CDL3STARSINSOUTH_Lookback();
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
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
            printf("CDL3STARSINSOUTH %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDL3STARSINSOUTH_Close(st); }
            printf("CDL3STARSINSOUTH %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDL3WHITESOLDIERS") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_CDL3WHITESOLDIERS_Lookback();
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
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
            printf("CDL3WHITESOLDIERS %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDL3WHITESOLDIERS_Close(st); }
            printf("CDL3WHITESOLDIERS %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLABANDONEDBABY") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_CDLABANDONEDBABY_Lookback(0.300000000000000);
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_open[t] = g_open[it & BENCH_MASK];
                g_rt_high[t] = g_high[it & BENCH_MASK];
                g_rt_low[t] = g_low[it & BENCH_MASK];
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_CDLABANDONEDBABY(t, t, g_rt_open, g_rt_high, g_rt_low, g_rt_close, 0.300000000000000, &begIdx, &nb, g_outIntBuf0);
                acc += (double)g_outIntBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_CDLABANDONEDBABY_Stream *st = NULL;
            int iv0 = 0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CDLABANDONEDBABY_Open(&st, g_open, g_high, g_low, g_close, g_nPoints, 0.300000000000000, &iv0);
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
            printf("CDLABANDONEDBABY %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLABANDONEDBABY_Close(st); }
            printf("CDLABANDONEDBABY %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLADVANCEBLOCK") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_CDLADVANCEBLOCK_Lookback();
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
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
            printf("CDLADVANCEBLOCK %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLADVANCEBLOCK_Close(st); }
            printf("CDLADVANCEBLOCK %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLBELTHOLD") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_CDLBELTHOLD_Lookback();
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
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
            printf("CDLBELTHOLD %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLBELTHOLD_Close(st); }
            printf("CDLBELTHOLD %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLBREAKAWAY") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_CDLBREAKAWAY_Lookback();
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
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
            printf("CDLBREAKAWAY %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLBREAKAWAY_Close(st); }
            printf("CDLBREAKAWAY %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLCLOSINGMARUBOZU") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_CDLCLOSINGMARUBOZU_Lookback();
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
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
            printf("CDLCLOSINGMARUBOZU %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLCLOSINGMARUBOZU_Close(st); }
            printf("CDLCLOSINGMARUBOZU %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLCONCEALBABYSWALL") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_CDLCONCEALBABYSWALL_Lookback();
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
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
            printf("CDLCONCEALBABYSWALL %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLCONCEALBABYSWALL_Close(st); }
            printf("CDLCONCEALBABYSWALL %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLCOUNTERATTACK") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_CDLCOUNTERATTACK_Lookback();
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
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
            printf("CDLCOUNTERATTACK %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLCOUNTERATTACK_Close(st); }
            printf("CDLCOUNTERATTACK %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLDARKCLOUDCOVER") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_CDLDARKCLOUDCOVER_Lookback(0.500000000000000);
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_open[t] = g_open[it & BENCH_MASK];
                g_rt_high[t] = g_high[it & BENCH_MASK];
                g_rt_low[t] = g_low[it & BENCH_MASK];
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_CDLDARKCLOUDCOVER(t, t, g_rt_open, g_rt_high, g_rt_low, g_rt_close, 0.500000000000000, &begIdx, &nb, g_outIntBuf0);
                acc += (double)g_outIntBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_CDLDARKCLOUDCOVER_Stream *st = NULL;
            int iv0 = 0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CDLDARKCLOUDCOVER_Open(&st, g_open, g_high, g_low, g_close, g_nPoints, 0.500000000000000, &iv0);
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
            printf("CDLDARKCLOUDCOVER %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLDARKCLOUDCOVER_Close(st); }
            printf("CDLDARKCLOUDCOVER %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLDOJI") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_CDLDOJI_Lookback();
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
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
            printf("CDLDOJI %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLDOJI_Close(st); }
            printf("CDLDOJI %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLDOJISTAR") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_CDLDOJISTAR_Lookback();
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
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
            printf("CDLDOJISTAR %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLDOJISTAR_Close(st); }
            printf("CDLDOJISTAR %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLDRAGONFLYDOJI") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_CDLDRAGONFLYDOJI_Lookback();
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
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
            printf("CDLDRAGONFLYDOJI %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLDRAGONFLYDOJI_Close(st); }
            printf("CDLDRAGONFLYDOJI %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLENGULFING") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_CDLENGULFING_Lookback();
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
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
            printf("CDLENGULFING %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLENGULFING_Close(st); }
            printf("CDLENGULFING %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLEVENINGDOJISTAR") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_CDLEVENINGDOJISTAR_Lookback(0.300000000000000);
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_open[t] = g_open[it & BENCH_MASK];
                g_rt_high[t] = g_high[it & BENCH_MASK];
                g_rt_low[t] = g_low[it & BENCH_MASK];
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_CDLEVENINGDOJISTAR(t, t, g_rt_open, g_rt_high, g_rt_low, g_rt_close, 0.300000000000000, &begIdx, &nb, g_outIntBuf0);
                acc += (double)g_outIntBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_CDLEVENINGDOJISTAR_Stream *st = NULL;
            int iv0 = 0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CDLEVENINGDOJISTAR_Open(&st, g_open, g_high, g_low, g_close, g_nPoints, 0.300000000000000, &iv0);
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
            printf("CDLEVENINGDOJISTAR %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLEVENINGDOJISTAR_Close(st); }
            printf("CDLEVENINGDOJISTAR %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLEVENINGSTAR") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_CDLEVENINGSTAR_Lookback(0.300000000000000);
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_open[t] = g_open[it & BENCH_MASK];
                g_rt_high[t] = g_high[it & BENCH_MASK];
                g_rt_low[t] = g_low[it & BENCH_MASK];
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_CDLEVENINGSTAR(t, t, g_rt_open, g_rt_high, g_rt_low, g_rt_close, 0.300000000000000, &begIdx, &nb, g_outIntBuf0);
                acc += (double)g_outIntBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_CDLEVENINGSTAR_Stream *st = NULL;
            int iv0 = 0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CDLEVENINGSTAR_Open(&st, g_open, g_high, g_low, g_close, g_nPoints, 0.300000000000000, &iv0);
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
            printf("CDLEVENINGSTAR %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLEVENINGSTAR_Close(st); }
            printf("CDLEVENINGSTAR %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLGAPSIDESIDEWHITE") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_CDLGAPSIDESIDEWHITE_Lookback();
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
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
            printf("CDLGAPSIDESIDEWHITE %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLGAPSIDESIDEWHITE_Close(st); }
            printf("CDLGAPSIDESIDEWHITE %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLGRAVESTONEDOJI") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_CDLGRAVESTONEDOJI_Lookback();
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
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
            printf("CDLGRAVESTONEDOJI %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLGRAVESTONEDOJI_Close(st); }
            printf("CDLGRAVESTONEDOJI %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLHAMMER") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_CDLHAMMER_Lookback();
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
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
            printf("CDLHAMMER %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLHAMMER_Close(st); }
            printf("CDLHAMMER %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLHANGINGMAN") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_CDLHANGINGMAN_Lookback();
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
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
            printf("CDLHANGINGMAN %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLHANGINGMAN_Close(st); }
            printf("CDLHANGINGMAN %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLHARAMI") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_CDLHARAMI_Lookback();
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
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
            printf("CDLHARAMI %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLHARAMI_Close(st); }
            printf("CDLHARAMI %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLHARAMICROSS") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_CDLHARAMICROSS_Lookback();
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
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
            printf("CDLHARAMICROSS %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLHARAMICROSS_Close(st); }
            printf("CDLHARAMICROSS %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLHIGHWAVE") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_CDLHIGHWAVE_Lookback();
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
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
            printf("CDLHIGHWAVE %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLHIGHWAVE_Close(st); }
            printf("CDLHIGHWAVE %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLHIKKAKE") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_CDLHIKKAKE_Lookback();
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
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
            printf("CDLHIKKAKE %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLHIKKAKE_Close(st); }
            printf("CDLHIKKAKE %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLHIKKAKEMOD") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_CDLHIKKAKEMOD_Lookback();
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
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
            printf("CDLHIKKAKEMOD %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLHIKKAKEMOD_Close(st); }
            printf("CDLHIKKAKEMOD %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLHOMINGPIGEON") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_CDLHOMINGPIGEON_Lookback();
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
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
            printf("CDLHOMINGPIGEON %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLHOMINGPIGEON_Close(st); }
            printf("CDLHOMINGPIGEON %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLIDENTICAL3CROWS") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_CDLIDENTICAL3CROWS_Lookback();
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
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
            printf("CDLIDENTICAL3CROWS %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLIDENTICAL3CROWS_Close(st); }
            printf("CDLIDENTICAL3CROWS %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLINNECK") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_CDLINNECK_Lookback();
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
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
            printf("CDLINNECK %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLINNECK_Close(st); }
            printf("CDLINNECK %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLINVERTEDHAMMER") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_CDLINVERTEDHAMMER_Lookback();
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
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
            printf("CDLINVERTEDHAMMER %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLINVERTEDHAMMER_Close(st); }
            printf("CDLINVERTEDHAMMER %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLKICKING") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_CDLKICKING_Lookback();
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
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
            printf("CDLKICKING %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLKICKING_Close(st); }
            printf("CDLKICKING %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLKICKINGBYLENGTH") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_CDLKICKINGBYLENGTH_Lookback();
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
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
            printf("CDLKICKINGBYLENGTH %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLKICKINGBYLENGTH_Close(st); }
            printf("CDLKICKINGBYLENGTH %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLLADDERBOTTOM") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_CDLLADDERBOTTOM_Lookback();
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
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
            printf("CDLLADDERBOTTOM %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLLADDERBOTTOM_Close(st); }
            printf("CDLLADDERBOTTOM %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLLONGLEGGEDDOJI") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_CDLLONGLEGGEDDOJI_Lookback();
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
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
            printf("CDLLONGLEGGEDDOJI %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLLONGLEGGEDDOJI_Close(st); }
            printf("CDLLONGLEGGEDDOJI %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLLONGLINE") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_CDLLONGLINE_Lookback();
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
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
            printf("CDLLONGLINE %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLLONGLINE_Close(st); }
            printf("CDLLONGLINE %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLMARUBOZU") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_CDLMARUBOZU_Lookback();
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
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
            printf("CDLMARUBOZU %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLMARUBOZU_Close(st); }
            printf("CDLMARUBOZU %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLMATCHINGLOW") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_CDLMATCHINGLOW_Lookback();
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
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
            printf("CDLMATCHINGLOW %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLMATCHINGLOW_Close(st); }
            printf("CDLMATCHINGLOW %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLMATHOLD") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_CDLMATHOLD_Lookback(0.500000000000000);
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_open[t] = g_open[it & BENCH_MASK];
                g_rt_high[t] = g_high[it & BENCH_MASK];
                g_rt_low[t] = g_low[it & BENCH_MASK];
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_CDLMATHOLD(t, t, g_rt_open, g_rt_high, g_rt_low, g_rt_close, 0.500000000000000, &begIdx, &nb, g_outIntBuf0);
                acc += (double)g_outIntBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_CDLMATHOLD_Stream *st = NULL;
            int iv0 = 0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CDLMATHOLD_Open(&st, g_open, g_high, g_low, g_close, g_nPoints, 0.500000000000000, &iv0);
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
            printf("CDLMATHOLD %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLMATHOLD_Close(st); }
            printf("CDLMATHOLD %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLMORNINGDOJISTAR") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_CDLMORNINGDOJISTAR_Lookback(0.300000000000000);
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_open[t] = g_open[it & BENCH_MASK];
                g_rt_high[t] = g_high[it & BENCH_MASK];
                g_rt_low[t] = g_low[it & BENCH_MASK];
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_CDLMORNINGDOJISTAR(t, t, g_rt_open, g_rt_high, g_rt_low, g_rt_close, 0.300000000000000, &begIdx, &nb, g_outIntBuf0);
                acc += (double)g_outIntBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_CDLMORNINGDOJISTAR_Stream *st = NULL;
            int iv0 = 0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CDLMORNINGDOJISTAR_Open(&st, g_open, g_high, g_low, g_close, g_nPoints, 0.300000000000000, &iv0);
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
            printf("CDLMORNINGDOJISTAR %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLMORNINGDOJISTAR_Close(st); }
            printf("CDLMORNINGDOJISTAR %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLMORNINGSTAR") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_CDLMORNINGSTAR_Lookback(0.300000000000000);
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_open[t] = g_open[it & BENCH_MASK];
                g_rt_high[t] = g_high[it & BENCH_MASK];
                g_rt_low[t] = g_low[it & BENCH_MASK];
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_CDLMORNINGSTAR(t, t, g_rt_open, g_rt_high, g_rt_low, g_rt_close, 0.300000000000000, &begIdx, &nb, g_outIntBuf0);
                acc += (double)g_outIntBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_CDLMORNINGSTAR_Stream *st = NULL;
            int iv0 = 0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CDLMORNINGSTAR_Open(&st, g_open, g_high, g_low, g_close, g_nPoints, 0.300000000000000, &iv0);
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
            printf("CDLMORNINGSTAR %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLMORNINGSTAR_Close(st); }
            printf("CDLMORNINGSTAR %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLONNECK") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_CDLONNECK_Lookback();
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
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
            printf("CDLONNECK %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLONNECK_Close(st); }
            printf("CDLONNECK %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLPIERCING") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_CDLPIERCING_Lookback();
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
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
            printf("CDLPIERCING %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLPIERCING_Close(st); }
            printf("CDLPIERCING %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLRICKSHAWMAN") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_CDLRICKSHAWMAN_Lookback();
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
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
            printf("CDLRICKSHAWMAN %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLRICKSHAWMAN_Close(st); }
            printf("CDLRICKSHAWMAN %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLRISEFALL3METHODS") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_CDLRISEFALL3METHODS_Lookback();
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
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
            printf("CDLRISEFALL3METHODS %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLRISEFALL3METHODS_Close(st); }
            printf("CDLRISEFALL3METHODS %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLSEPARATINGLINES") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_CDLSEPARATINGLINES_Lookback();
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
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
            printf("CDLSEPARATINGLINES %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLSEPARATINGLINES_Close(st); }
            printf("CDLSEPARATINGLINES %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLSHOOTINGSTAR") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_CDLSHOOTINGSTAR_Lookback();
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
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
            printf("CDLSHOOTINGSTAR %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLSHOOTINGSTAR_Close(st); }
            printf("CDLSHOOTINGSTAR %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLSHORTLINE") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_CDLSHORTLINE_Lookback();
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
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
            printf("CDLSHORTLINE %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLSHORTLINE_Close(st); }
            printf("CDLSHORTLINE %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLSPINNINGTOP") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_CDLSPINNINGTOP_Lookback();
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
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
            printf("CDLSPINNINGTOP %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLSPINNINGTOP_Close(st); }
            printf("CDLSPINNINGTOP %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLSTALLEDPATTERN") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_CDLSTALLEDPATTERN_Lookback();
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
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
            printf("CDLSTALLEDPATTERN %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLSTALLEDPATTERN_Close(st); }
            printf("CDLSTALLEDPATTERN %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLSTICKSANDWICH") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_CDLSTICKSANDWICH_Lookback();
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
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
            printf("CDLSTICKSANDWICH %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLSTICKSANDWICH_Close(st); }
            printf("CDLSTICKSANDWICH %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLTAKURI") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_CDLTAKURI_Lookback();
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
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
            printf("CDLTAKURI %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLTAKURI_Close(st); }
            printf("CDLTAKURI %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLTASUKIGAP") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_CDLTASUKIGAP_Lookback();
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
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
            printf("CDLTASUKIGAP %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLTASUKIGAP_Close(st); }
            printf("CDLTASUKIGAP %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLTHRUSTING") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_CDLTHRUSTING_Lookback();
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
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
            printf("CDLTHRUSTING %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLTHRUSTING_Close(st); }
            printf("CDLTHRUSTING %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLTRISTAR") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_CDLTRISTAR_Lookback();
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
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
            printf("CDLTRISTAR %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLTRISTAR_Close(st); }
            printf("CDLTRISTAR %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLUNIQUE3RIVER") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_CDLUNIQUE3RIVER_Lookback();
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
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
            printf("CDLUNIQUE3RIVER %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLUNIQUE3RIVER_Close(st); }
            printf("CDLUNIQUE3RIVER %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLUPSIDEGAP2CROWS") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_CDLUPSIDEGAP2CROWS_Lookback();
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
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
            printf("CDLUPSIDEGAP2CROWS %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLUPSIDEGAP2CROWS_Close(st); }
            printf("CDLUPSIDEGAP2CROWS %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CDLXSIDEGAP3METHODS") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_CDLXSIDEGAP3METHODS_Lookback();
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
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
            printf("CDLXSIDEGAP3METHODS %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CDLXSIDEGAP3METHODS_Close(st); }
            printf("CDLXSIDEGAP3METHODS %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CEIL") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_CEIL_Lookback();
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
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
            printf("CEIL %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CEIL_Close(st); }
            printf("CEIL %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CMO") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_CMO_Lookback(14);
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_CMO(t, t, g_rt_close, 14, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_CMO_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CMO_Open(&st, g_close, g_nPoints, 14, &v0);
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
            printf("CMO %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CMO_Close(st); }
            printf("CMO %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "CORREL") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_CORREL_Lookback(30);
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_close[t] = g_close[it & BENCH_MASK];
                g_rt_high[t] = g_high[it & BENCH_MASK];
                TA_CORREL(t, t, g_rt_close, g_rt_high, 30, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_CORREL_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_CORREL_Open(&st, g_close, g_high, g_nPoints, 30, &v0);
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
            printf("CORREL %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_CORREL_Close(st); }
            printf("CORREL %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "COS") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_COS_Lookback();
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
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
            printf("COS %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_COS_Close(st); }
            printf("COS %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "COSH") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_COSH_Lookback();
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
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
            printf("COSH %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_COSH_Close(st); }
            printf("COSH %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "DEMA") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_DEMA_Lookback(30);
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_DEMA(t, t, g_rt_close, 30, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_DEMA_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_DEMA_Open(&st, g_close, g_nPoints, 30, &v0);
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
            printf("DEMA %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_DEMA_Close(st); }
            printf("DEMA %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "DIV") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_DIV_Lookback();
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
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
            printf("DIV %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_DIV_Close(st); }
            printf("DIV %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "DX") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_DX_Lookback(14);
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_high[t] = g_high[it & BENCH_MASK];
                g_rt_low[t] = g_low[it & BENCH_MASK];
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_DX(t, t, g_rt_high, g_rt_low, g_rt_close, 14, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_DX_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_DX_Open(&st, g_high, g_low, g_close, g_nPoints, 14, &v0);
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
            printf("DX %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_DX_Close(st); }
            printf("DX %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "EMA") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_EMA_Lookback(30);
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_EMA(t, t, g_rt_close, 30, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_EMA_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_EMA_Open(&st, g_close, g_nPoints, 30, &v0);
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
            printf("EMA %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_EMA_Close(st); }
            printf("EMA %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "EXP") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_EXP_Lookback();
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
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
            printf("EXP %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_EXP_Close(st); }
            printf("EXP %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "FLOOR") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_FLOOR_Lookback();
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
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
            printf("FLOOR %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_FLOOR_Close(st); }
            printf("FLOOR %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "HT_DCPERIOD") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_HT_DCPERIOD_Lookback();
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
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
            printf("HT_DCPERIOD %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_HT_DCPERIOD_Close(st); }
            printf("HT_DCPERIOD %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "HT_DCPHASE") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_HT_DCPHASE_Lookback();
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
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
            printf("HT_DCPHASE %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_HT_DCPHASE_Close(st); }
            printf("HT_DCPHASE %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "HT_PHASOR") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_HT_PHASOR_Lookback();
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
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
            printf("HT_PHASOR %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_HT_PHASOR_Close(st); }
            printf("HT_PHASOR %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "HT_SINE") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_HT_SINE_Lookback();
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
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
            printf("HT_SINE %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_HT_SINE_Close(st); }
            printf("HT_SINE %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "HT_TRENDLINE") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_HT_TRENDLINE_Lookback();
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
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
            printf("HT_TRENDLINE %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_HT_TRENDLINE_Close(st); }
            printf("HT_TRENDLINE %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "HT_TRENDMODE") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_HT_TRENDMODE_Lookback();
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
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
            printf("HT_TRENDMODE %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_HT_TRENDMODE_Close(st); }
            printf("HT_TRENDMODE %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "IMI") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_IMI_Lookback(14);
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_open[t] = g_open[it & BENCH_MASK];
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_IMI(t, t, g_rt_open, g_rt_close, 14, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_IMI_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_IMI_Open(&st, g_open, g_close, g_nPoints, 14, &v0);
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
            printf("IMI %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_IMI_Close(st); }
            printf("IMI %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "KAMA") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_KAMA_Lookback(30);
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_KAMA(t, t, g_rt_close, 30, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_KAMA_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_KAMA_Open(&st, g_close, g_nPoints, 30, &v0);
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
            printf("KAMA %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_KAMA_Close(st); }
            printf("KAMA %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "LINEARREG") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_LINEARREG_Lookback(14);
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_LINEARREG(t, t, g_rt_close, 14, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_LINEARREG_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_LINEARREG_Open(&st, g_close, g_nPoints, 14, &v0);
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
            printf("LINEARREG %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_LINEARREG_Close(st); }
            printf("LINEARREG %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "LINEARREG_ANGLE") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_LINEARREG_ANGLE_Lookback(14);
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_LINEARREG_ANGLE(t, t, g_rt_close, 14, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_LINEARREG_ANGLE_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_LINEARREG_ANGLE_Open(&st, g_close, g_nPoints, 14, &v0);
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
            printf("LINEARREG_ANGLE %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_LINEARREG_ANGLE_Close(st); }
            printf("LINEARREG_ANGLE %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "LINEARREG_INTERCEPT") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_LINEARREG_INTERCEPT_Lookback(14);
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_LINEARREG_INTERCEPT(t, t, g_rt_close, 14, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_LINEARREG_INTERCEPT_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_LINEARREG_INTERCEPT_Open(&st, g_close, g_nPoints, 14, &v0);
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
            printf("LINEARREG_INTERCEPT %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_LINEARREG_INTERCEPT_Close(st); }
            printf("LINEARREG_INTERCEPT %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "LINEARREG_SLOPE") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_LINEARREG_SLOPE_Lookback(14);
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_LINEARREG_SLOPE(t, t, g_rt_close, 14, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_LINEARREG_SLOPE_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_LINEARREG_SLOPE_Open(&st, g_close, g_nPoints, 14, &v0);
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
            printf("LINEARREG_SLOPE %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_LINEARREG_SLOPE_Close(st); }
            printf("LINEARREG_SLOPE %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "LN") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_LN_Lookback();
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
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
            printf("LN %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_LN_Close(st); }
            printf("LN %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "LOG10") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_LOG10_Lookback();
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
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
            printf("LOG10 %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_LOG10_Close(st); }
            printf("LOG10 %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "MA") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_MA_Lookback(30, 0);
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_MA(t, t, g_rt_close, 30, 0, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_MA_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_MA_Open(&st, g_close, g_nPoints, 30, 0, &v0);
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
            printf("MA %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_MA_Close(st); }
            printf("MA %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "MACD") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_MACD_Lookback(12, 26, 9);
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_MACD(t, t, g_rt_close, 12, 26, 9, &begIdx, &nb, g_outBuf0, g_outBuf1, g_outBuf2);
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
        TA_RetCode orc = TA_MACD_Open(&st, g_close, g_nPoints, 12, 26, 9, &v0, &v1, &v2);
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
            printf("MACD %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_MACD_Close(st); }
            printf("MACD %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "MACDEXT") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_MACDEXT_Lookback(12, 0, 26, 0, 9, 0);
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_MACDEXT(t, t, g_rt_close, 12, 0, 26, 0, 9, 0, &begIdx, &nb, g_outBuf0, g_outBuf1, g_outBuf2);
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
        TA_RetCode orc = TA_MACDEXT_Open(&st, g_close, g_nPoints, 12, 0, 26, 0, 9, 0, &v0, &v1, &v2);
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
            printf("MACDEXT %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_MACDEXT_Close(st); }
            printf("MACDEXT %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "MACDFIX") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_MACDFIX_Lookback(9);
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_MACDFIX(t, t, g_rt_close, 9, &begIdx, &nb, g_outBuf0, g_outBuf1, g_outBuf2);
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
        TA_RetCode orc = TA_MACDFIX_Open(&st, g_close, g_nPoints, 9, &v0, &v1, &v2);
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
            printf("MACDFIX %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_MACDFIX_Close(st); }
            printf("MACDFIX %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "MAMA") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_MAMA_Lookback(0.500000000000000, 0.050000000000000);
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_MAMA(t, t, g_rt_close, 0.500000000000000, 0.050000000000000, &begIdx, &nb, g_outBuf0, g_outBuf1);
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
        TA_RetCode orc = TA_MAMA_Open(&st, g_close, g_nPoints, 0.500000000000000, 0.050000000000000, &v0, &v1);
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
            printf("MAMA %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_MAMA_Close(st); }
            printf("MAMA %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "MAVP") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_MAVP_Lookback(2, 30, 0);
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_close[t] = g_close[it & BENCH_MASK];
                g_rt_high[t] = g_high[it & BENCH_MASK];
                TA_MAVP(t, t, g_rt_close, g_rt_high, 2, 30, 0, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_MAVP_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_MAVP_Open(&st, g_close, g_high, g_nPoints, 2, 30, 0, &v0);
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
            printf("MAVP %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_MAVP_Close(st); }
            printf("MAVP %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "MAX") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_MAX_Lookback(30);
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_MAX(t, t, g_rt_close, 30, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_MAX_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_MAX_Open(&st, g_close, g_nPoints, 30, &v0);
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
            printf("MAX %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_MAX_Close(st); }
            printf("MAX %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "MAXINDEX") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_MAXINDEX_Lookback(30);
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_MAXINDEX(t, t, g_rt_close, 30, &begIdx, &nb, g_outIntBuf0);
                acc += (double)g_outIntBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_MAXINDEX_Stream *st = NULL;
            int iv0 = 0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_MAXINDEX_Open(&st, g_close, g_nPoints, 30, &iv0);
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
            printf("MAXINDEX %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_MAXINDEX_Close(st); }
            printf("MAXINDEX %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "MEDPRICE") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_MEDPRICE_Lookback();
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
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
            printf("MEDPRICE %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_MEDPRICE_Close(st); }
            printf("MEDPRICE %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "MFI") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_MFI_Lookback(14);
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_high[t] = g_high[it & BENCH_MASK];
                g_rt_low[t] = g_low[it & BENCH_MASK];
                g_rt_close[t] = g_close[it & BENCH_MASK];
                g_rt_volume[t] = g_volume[it & BENCH_MASK];
                TA_MFI(t, t, g_rt_high, g_rt_low, g_rt_close, g_rt_volume, 14, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_MFI_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_MFI_Open(&st, g_high, g_low, g_close, g_volume, g_nPoints, 14, &v0);
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
            printf("MFI %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_MFI_Close(st); }
            printf("MFI %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "MIDPOINT") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_MIDPOINT_Lookback(14);
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_MIDPOINT(t, t, g_rt_close, 14, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_MIDPOINT_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_MIDPOINT_Open(&st, g_close, g_nPoints, 14, &v0);
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
            printf("MIDPOINT %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_MIDPOINT_Close(st); }
            printf("MIDPOINT %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "MIDPRICE") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_MIDPRICE_Lookback(14);
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_high[t] = g_high[it & BENCH_MASK];
                g_rt_low[t] = g_low[it & BENCH_MASK];
                TA_MIDPRICE(t, t, g_rt_high, g_rt_low, 14, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_MIDPRICE_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_MIDPRICE_Open(&st, g_high, g_low, g_nPoints, 14, &v0);
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
            printf("MIDPRICE %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_MIDPRICE_Close(st); }
            printf("MIDPRICE %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "MIN") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_MIN_Lookback(30);
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_MIN(t, t, g_rt_close, 30, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_MIN_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_MIN_Open(&st, g_close, g_nPoints, 30, &v0);
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
            printf("MIN %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_MIN_Close(st); }
            printf("MIN %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "MININDEX") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_MININDEX_Lookback(30);
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_MININDEX(t, t, g_rt_close, 30, &begIdx, &nb, g_outIntBuf0);
                acc += (double)g_outIntBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_MININDEX_Stream *st = NULL;
            int iv0 = 0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_MININDEX_Open(&st, g_close, g_nPoints, 30, &iv0);
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
            printf("MININDEX %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_MININDEX_Close(st); }
            printf("MININDEX %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "MINMAX") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_MINMAX_Lookback(30);
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_MINMAX(t, t, g_rt_close, 30, &begIdx, &nb, g_outBuf0, g_outBuf1);
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
        TA_RetCode orc = TA_MINMAX_Open(&st, g_close, g_nPoints, 30, &v0, &v1);
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
            printf("MINMAX %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_MINMAX_Close(st); }
            printf("MINMAX %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "MINMAXINDEX") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_MINMAXINDEX_Lookback(30);
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_MINMAXINDEX(t, t, g_rt_close, 30, &begIdx, &nb, g_outIntBuf0, g_outIntBuf1);
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
        TA_RetCode orc = TA_MINMAXINDEX_Open(&st, g_close, g_nPoints, 30, &iv0, &iv1);
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
            printf("MINMAXINDEX %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_MINMAXINDEX_Close(st); }
            printf("MINMAXINDEX %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "MINUS_DI") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_MINUS_DI_Lookback(14);
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_high[t] = g_high[it & BENCH_MASK];
                g_rt_low[t] = g_low[it & BENCH_MASK];
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_MINUS_DI(t, t, g_rt_high, g_rt_low, g_rt_close, 14, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_MINUS_DI_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_MINUS_DI_Open(&st, g_high, g_low, g_close, g_nPoints, 14, &v0);
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
            printf("MINUS_DI %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_MINUS_DI_Close(st); }
            printf("MINUS_DI %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "MINUS_DM") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_MINUS_DM_Lookback(14);
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_high[t] = g_high[it & BENCH_MASK];
                g_rt_low[t] = g_low[it & BENCH_MASK];
                TA_MINUS_DM(t, t, g_rt_high, g_rt_low, 14, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_MINUS_DM_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_MINUS_DM_Open(&st, g_high, g_low, g_nPoints, 14, &v0);
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
            printf("MINUS_DM %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_MINUS_DM_Close(st); }
            printf("MINUS_DM %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "MOM") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_MOM_Lookback(10);
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_MOM(t, t, g_rt_close, 10, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_MOM_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_MOM_Open(&st, g_close, g_nPoints, 10, &v0);
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
            printf("MOM %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_MOM_Close(st); }
            printf("MOM %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "MULT") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_MULT_Lookback();
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
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
            printf("MULT %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_MULT_Close(st); }
            printf("MULT %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "NATR") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_NATR_Lookback(14);
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_high[t] = g_high[it & BENCH_MASK];
                g_rt_low[t] = g_low[it & BENCH_MASK];
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_NATR(t, t, g_rt_high, g_rt_low, g_rt_close, 14, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_NATR_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_NATR_Open(&st, g_high, g_low, g_close, g_nPoints, 14, &v0);
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
            printf("NATR %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_NATR_Close(st); }
            printf("NATR %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "OBV") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_OBV_Lookback();
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
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
            printf("OBV %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_OBV_Close(st); }
            printf("OBV %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "PLUS_DI") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_PLUS_DI_Lookback(14);
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_high[t] = g_high[it & BENCH_MASK];
                g_rt_low[t] = g_low[it & BENCH_MASK];
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_PLUS_DI(t, t, g_rt_high, g_rt_low, g_rt_close, 14, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_PLUS_DI_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_PLUS_DI_Open(&st, g_high, g_low, g_close, g_nPoints, 14, &v0);
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
            printf("PLUS_DI %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_PLUS_DI_Close(st); }
            printf("PLUS_DI %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "PLUS_DM") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_PLUS_DM_Lookback(14);
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_high[t] = g_high[it & BENCH_MASK];
                g_rt_low[t] = g_low[it & BENCH_MASK];
                TA_PLUS_DM(t, t, g_rt_high, g_rt_low, 14, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_PLUS_DM_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_PLUS_DM_Open(&st, g_high, g_low, g_nPoints, 14, &v0);
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
            printf("PLUS_DM %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_PLUS_DM_Close(st); }
            printf("PLUS_DM %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "PPO") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_PPO_Lookback(12, 26, 0);
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_PPO(t, t, g_rt_close, 12, 26, 0, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_PPO_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_PPO_Open(&st, g_close, g_nPoints, 12, 26, 0, &v0);
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
            printf("PPO %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_PPO_Close(st); }
            printf("PPO %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "ROC") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_ROC_Lookback(10);
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_ROC(t, t, g_rt_close, 10, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_ROC_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_ROC_Open(&st, g_close, g_nPoints, 10, &v0);
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
            printf("ROC %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_ROC_Close(st); }
            printf("ROC %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "ROCP") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_ROCP_Lookback(10);
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_ROCP(t, t, g_rt_close, 10, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_ROCP_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_ROCP_Open(&st, g_close, g_nPoints, 10, &v0);
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
            printf("ROCP %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_ROCP_Close(st); }
            printf("ROCP %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "ROCR") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_ROCR_Lookback(10);
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_ROCR(t, t, g_rt_close, 10, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_ROCR_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_ROCR_Open(&st, g_close, g_nPoints, 10, &v0);
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
            printf("ROCR %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_ROCR_Close(st); }
            printf("ROCR %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "ROCR100") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_ROCR100_Lookback(10);
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_ROCR100(t, t, g_rt_close, 10, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_ROCR100_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_ROCR100_Open(&st, g_close, g_nPoints, 10, &v0);
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
            printf("ROCR100 %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_ROCR100_Close(st); }
            printf("ROCR100 %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "RSI") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_RSI_Lookback(14);
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_RSI(t, t, g_rt_close, 14, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_RSI_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_RSI_Open(&st, g_close, g_nPoints, 14, &v0);
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
            printf("RSI %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_RSI_Close(st); }
            printf("RSI %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "SAR") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_SAR_Lookback(0.020000000000000, 0.200000000000000);
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_high[t] = g_high[it & BENCH_MASK];
                g_rt_low[t] = g_low[it & BENCH_MASK];
                TA_SAR(t, t, g_rt_high, g_rt_low, 0.020000000000000, 0.200000000000000, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_SAR_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_SAR_Open(&st, g_high, g_low, g_nPoints, 0.020000000000000, 0.200000000000000, &v0);
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
            printf("SAR %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_SAR_Close(st); }
            printf("SAR %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "SAREXT") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_SAREXT_Lookback(0.000000000000000, 0.000000000000000, 0.020000000000000, 0.020000000000000, 0.200000000000000, 0.020000000000000, 0.020000000000000, 0.200000000000000);
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_high[t] = g_high[it & BENCH_MASK];
                g_rt_low[t] = g_low[it & BENCH_MASK];
                TA_SAREXT(t, t, g_rt_high, g_rt_low, 0.000000000000000, 0.000000000000000, 0.020000000000000, 0.020000000000000, 0.200000000000000, 0.020000000000000, 0.020000000000000, 0.200000000000000, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_SAREXT_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_SAREXT_Open(&st, g_high, g_low, g_nPoints, 0.000000000000000, 0.000000000000000, 0.020000000000000, 0.020000000000000, 0.200000000000000, 0.020000000000000, 0.020000000000000, 0.200000000000000, &v0);
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
            printf("SAREXT %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_SAREXT_Close(st); }
            printf("SAREXT %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "SIN") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_SIN_Lookback();
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
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
            printf("SIN %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_SIN_Close(st); }
            printf("SIN %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "SINH") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_SINH_Lookback();
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
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
            printf("SINH %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_SINH_Close(st); }
            printf("SINH %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "SMA") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_SMA_Lookback(30);
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_SMA(t, t, g_rt_close, 30, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_SMA_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_SMA_Open(&st, g_close, g_nPoints, 30, &v0);
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
            printf("SMA %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_SMA_Close(st); }
            printf("SMA %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "SQRT") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_SQRT_Lookback();
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
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
            printf("SQRT %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_SQRT_Close(st); }
            printf("SQRT %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "STDDEV") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_STDDEV_Lookback(5, 1.000000000000000);
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_STDDEV(t, t, g_rt_close, 5, 1.000000000000000, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_STDDEV_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_STDDEV_Open(&st, g_close, g_nPoints, 5, 1.000000000000000, &v0);
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
            printf("STDDEV %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_STDDEV_Close(st); }
            printf("STDDEV %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "STOCH") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_STOCH_Lookback(5, 3, 0, 3, 0);
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_high[t] = g_high[it & BENCH_MASK];
                g_rt_low[t] = g_low[it & BENCH_MASK];
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_STOCH(t, t, g_rt_high, g_rt_low, g_rt_close, 5, 3, 0, 3, 0, &begIdx, &nb, g_outBuf0, g_outBuf1);
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
        TA_RetCode orc = TA_STOCH_Open(&st, g_high, g_low, g_close, g_nPoints, 5, 3, 0, 3, 0, &v0, &v1);
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
            printf("STOCH %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_STOCH_Close(st); }
            printf("STOCH %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "STOCHF") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_STOCHF_Lookback(5, 3, 0);
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_high[t] = g_high[it & BENCH_MASK];
                g_rt_low[t] = g_low[it & BENCH_MASK];
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_STOCHF(t, t, g_rt_high, g_rt_low, g_rt_close, 5, 3, 0, &begIdx, &nb, g_outBuf0, g_outBuf1);
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
        TA_RetCode orc = TA_STOCHF_Open(&st, g_high, g_low, g_close, g_nPoints, 5, 3, 0, &v0, &v1);
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
            printf("STOCHF %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_STOCHF_Close(st); }
            printf("STOCHF %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "STOCHRSI") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_STOCHRSI_Lookback(14, 5, 3, 0);
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_STOCHRSI(t, t, g_rt_close, 14, 5, 3, 0, &begIdx, &nb, g_outBuf0, g_outBuf1);
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
        TA_RetCode orc = TA_STOCHRSI_Open(&st, g_close, g_nPoints, 14, 5, 3, 0, &v0, &v1);
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
            printf("STOCHRSI %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_STOCHRSI_Close(st); }
            printf("STOCHRSI %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "SUB") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_SUB_Lookback();
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
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
            printf("SUB %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_SUB_Close(st); }
            printf("SUB %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "SUM") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_SUM_Lookback(30);
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_SUM(t, t, g_rt_close, 30, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_SUM_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_SUM_Open(&st, g_close, g_nPoints, 30, &v0);
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
            printf("SUM %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_SUM_Close(st); }
            printf("SUM %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "T3") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_T3_Lookback(5, 0.700000000000000);
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_T3(t, t, g_rt_close, 5, 0.700000000000000, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_T3_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_T3_Open(&st, g_close, g_nPoints, 5, 0.700000000000000, &v0);
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
            printf("T3 %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_T3_Close(st); }
            printf("T3 %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "TAN") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_TAN_Lookback();
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
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
            printf("TAN %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_TAN_Close(st); }
            printf("TAN %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "TANH") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_TANH_Lookback();
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
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
            printf("TANH %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_TANH_Close(st); }
            printf("TANH %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "TEMA") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_TEMA_Lookback(30);
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_TEMA(t, t, g_rt_close, 30, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_TEMA_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_TEMA_Open(&st, g_close, g_nPoints, 30, &v0);
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
            printf("TEMA %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_TEMA_Close(st); }
            printf("TEMA %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "TRANGE") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_TRANGE_Lookback();
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
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
            printf("TRANGE %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_TRANGE_Close(st); }
            printf("TRANGE %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "TRIMA") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_TRIMA_Lookback(30);
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_TRIMA(t, t, g_rt_close, 30, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_TRIMA_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_TRIMA_Open(&st, g_close, g_nPoints, 30, &v0);
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
            printf("TRIMA %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_TRIMA_Close(st); }
            printf("TRIMA %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "TRIX") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_TRIX_Lookback(30);
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_TRIX(t, t, g_rt_close, 30, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_TRIX_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_TRIX_Open(&st, g_close, g_nPoints, 30, &v0);
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
            printf("TRIX %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_TRIX_Close(st); }
            printf("TRIX %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "TSF") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_TSF_Lookback(14);
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_TSF(t, t, g_rt_close, 14, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_TSF_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_TSF_Open(&st, g_close, g_nPoints, 14, &v0);
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
            printf("TSF %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_TSF_Close(st); }
            printf("TSF %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "TYPPRICE") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_TYPPRICE_Lookback();
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
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
            printf("TYPPRICE %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_TYPPRICE_Close(st); }
            printf("TYPPRICE %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "ULTOSC") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_ULTOSC_Lookback(7, 14, 28);
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_high[t] = g_high[it & BENCH_MASK];
                g_rt_low[t] = g_low[it & BENCH_MASK];
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_ULTOSC(t, t, g_rt_high, g_rt_low, g_rt_close, 7, 14, 28, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_ULTOSC_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_ULTOSC_Open(&st, g_high, g_low, g_close, g_nPoints, 7, 14, 28, &v0);
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
            printf("ULTOSC %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_ULTOSC_Close(st); }
            printf("ULTOSC %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "VAR") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_VAR_Lookback(5, 1.000000000000000);
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_VAR(t, t, g_rt_close, 5, 1.000000000000000, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_VAR_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_VAR_Open(&st, g_close, g_nPoints, 5, 1.000000000000000, &v0);
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
            printf("VAR %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_VAR_Close(st); }
            printf("VAR %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "WCLPRICE") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_WCLPRICE_Lookback();
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
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
            printf("WCLPRICE %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_WCLPRICE_Close(st); }
            printf("WCLPRICE %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "WILLR") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_WILLR_Lookback(14);
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_high[t] = g_high[it & BENCH_MASK];
                g_rt_low[t] = g_low[it & BENCH_MASK];
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_WILLR(t, t, g_rt_high, g_rt_low, g_rt_close, 14, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_WILLR_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_WILLR_Open(&st, g_high, g_low, g_close, g_nPoints, 14, &v0);
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
            printf("WILLR %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_WILLR_Close(st); }
            printf("WILLR %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
        }
        fflush(stdout);
    }
    if( func_matches(filter, "WMA") ) {
        long long best_b = 0, best_u = -1, best_p = -1;
        int begIdx = 0, nb = 0;
        size_t handle_bytes = 0;
        double acc = 0.0;
        int lb = TA_WMA_Lookback(30);
        if( lb < 0 ) lb = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int t = lb;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                g_rt_close[t] = g_close[it & BENCH_MASK];
                TA_WMA(t, t, g_rt_close, 30, &begIdx, &nb, g_outBuf0);
                acc += g_outBuf0[0];
                t++;
            }
            long long el = get_nanotime() - t0;
            if( !best_b || el < best_b ) best_b = el;
        }
        TA_WMA_Stream *st = NULL;
            double v0 = 0.0;
        g_trk_reset(); g_ta_track = 1;
        TA_RetCode orc = TA_WMA_Open(&st, g_close, g_nPoints, 30, &v0);
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
            printf("WMA %.3f %.3f %.3f %d %zu\n", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);
        } else {
            g_sink += (int)acc + nb;
            if( st ) { g_ta_track = 0; TA_WMA_Close(st); }
            printf("WMA %.3f -1 -1 %d 0\n", best_b/(double)iters, lb);
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
    if( n_points < BENCH_MASK + 1 ) n_points = BENCH_MASK + 1; /* the bar feed indexes it & BENCH_MASK */
    if( n_iters < 1 ) n_iters = 1;
    generate_price_data(n_points);
    /* Growing history for batch@last: one buffer sized to hold the whole run
       (n_iters appended bars + lookback headroom) so it never recycles within a pass. */
    g_rtCap = n_iters + 8192;
    g_rt_open   = malloc(sizeof(double) * (size_t)g_rtCap);
    g_rt_high   = malloc(sizeof(double) * (size_t)g_rtCap);
    g_rt_low    = malloc(sizeof(double) * (size_t)g_rtCap);
    g_rt_close  = malloc(sizeof(double) * (size_t)g_rtCap);
    g_rt_volume = malloc(sizeof(double) * (size_t)g_rtCap);
    g_rt_oi     = malloc(sizeof(double) * (size_t)g_rtCap);
    if( g_rtCap <= 0 || !g_rt_open || !g_rt_high || !g_rt_low || !g_rt_close || !g_rt_volume || !g_rt_oi ) {
        fprintf( stderr, "ta_bench_stream: allocation failed (try a smaller --iters)\n" );
        return 1;
    }
    for( int i = 0; i < g_rtCap; i++ ) {
        int j = i % g_nPoints;
        g_rt_open[i]=g_open[j]; g_rt_high[i]=g_high[j]; g_rt_low[i]=g_low[j];
        g_rt_close[i]=g_close[j]; g_rt_volume[i]=g_volume[j]; g_rt_oi[i]=g_oi[j];
    }
    bench_stream_all(func_filter, n_iters);
    free(g_rt_open); free(g_rt_high); free(g_rt_low); free(g_rt_close); free(g_rt_volume); free(g_rt_oi);
    free(g_open); free(g_high); free(g_low); free(g_close); free(g_volume); free(g_oi);
    return 0;
}
