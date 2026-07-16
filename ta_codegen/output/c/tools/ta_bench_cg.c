/* Auto-generated direct-call benchmark for ta_codegen C output.
 * No JSON-RPC -- generates its own data, calls functions directly.
 * Output: FUNCNAME timing_ns (one per line)
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
#include "ta_PVO.c"
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

static void bench_all(const char *filter, int iters) {
    if( func_matches(filter, "ACCBANDS") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_ACCBANDS(0, g_nPoints - 1, g_high, g_low, g_close, 20, &outBegIdx, &outNBElement, g_outBuf0, g_outBuf1, g_outBuf2);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += (int)g_outBuf0[0];
            g_sink += (int)g_outBuf1[0];
            g_sink += (int)g_outBuf2[0];
        }
        printf("ACCBANDS %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "ACOS") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_ACOS(0, g_nPoints - 1, g_close, &outBegIdx, &outNBElement, g_outBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += (int)g_outBuf0[0];
        }
        printf("ACOS %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "AD") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_AD(0, g_nPoints - 1, g_high, g_low, g_close, g_volume, &outBegIdx, &outNBElement, g_outBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += (int)g_outBuf0[0];
        }
        printf("AD %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "ADD") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_ADD(0, g_nPoints - 1, g_close, g_high, &outBegIdx, &outNBElement, g_outBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += (int)g_outBuf0[0];
        }
        printf("ADD %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "ADOSC") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_ADOSC(0, g_nPoints - 1, g_high, g_low, g_close, g_volume, 3, 10, &outBegIdx, &outNBElement, g_outBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += (int)g_outBuf0[0];
        }
        printf("ADOSC %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "ADX") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_ADX(0, g_nPoints - 1, g_high, g_low, g_close, 14, &outBegIdx, &outNBElement, g_outBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += (int)g_outBuf0[0];
        }
        printf("ADX %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "ADXR") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_ADXR(0, g_nPoints - 1, g_high, g_low, g_close, 14, &outBegIdx, &outNBElement, g_outBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += (int)g_outBuf0[0];
        }
        printf("ADXR %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "APO") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_APO(0, g_nPoints - 1, g_close, 12, 26, 1, &outBegIdx, &outNBElement, g_outBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += (int)g_outBuf0[0];
        }
        printf("APO %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "AROON") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_AROON(0, g_nPoints - 1, g_high, g_low, 14, &outBegIdx, &outNBElement, g_outBuf0, g_outBuf1);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += (int)g_outBuf0[0];
            g_sink += (int)g_outBuf1[0];
        }
        printf("AROON %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "AROONOSC") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_AROONOSC(0, g_nPoints - 1, g_high, g_low, 14, &outBegIdx, &outNBElement, g_outBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += (int)g_outBuf0[0];
        }
        printf("AROONOSC %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "ASIN") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_ASIN(0, g_nPoints - 1, g_close, &outBegIdx, &outNBElement, g_outBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += (int)g_outBuf0[0];
        }
        printf("ASIN %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "ATAN") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_ATAN(0, g_nPoints - 1, g_close, &outBegIdx, &outNBElement, g_outBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += (int)g_outBuf0[0];
        }
        printf("ATAN %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "ATR") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_ATR(0, g_nPoints - 1, g_high, g_low, g_close, 14, &outBegIdx, &outNBElement, g_outBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += (int)g_outBuf0[0];
        }
        printf("ATR %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "AVGDEV") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_AVGDEV(0, g_nPoints - 1, g_close, 14, &outBegIdx, &outNBElement, g_outBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += (int)g_outBuf0[0];
        }
        printf("AVGDEV %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "AVGPRICE") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_AVGPRICE(0, g_nPoints - 1, g_open, g_high, g_low, g_close, &outBegIdx, &outNBElement, g_outBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += (int)g_outBuf0[0];
        }
        printf("AVGPRICE %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "BBANDS") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_BBANDS(0, g_nPoints - 1, g_close, 5, 2.000000000000000, 2.000000000000000, 0, &outBegIdx, &outNBElement, g_outBuf0, g_outBuf1, g_outBuf2);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += (int)g_outBuf0[0];
            g_sink += (int)g_outBuf1[0];
            g_sink += (int)g_outBuf2[0];
        }
        printf("BBANDS %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "BETA") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_BETA(0, g_nPoints - 1, g_close, g_high, 5, &outBegIdx, &outNBElement, g_outBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += (int)g_outBuf0[0];
        }
        printf("BETA %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "BOP") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_BOP(0, g_nPoints - 1, g_open, g_high, g_low, g_close, &outBegIdx, &outNBElement, g_outBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += (int)g_outBuf0[0];
        }
        printf("BOP %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "CCI") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_CCI(0, g_nPoints - 1, g_high, g_low, g_close, 14, &outBegIdx, &outNBElement, g_outBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += (int)g_outBuf0[0];
        }
        printf("CCI %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "CDL2CROWS") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_CDL2CROWS(0, g_nPoints - 1, g_open, g_high, g_low, g_close, &outBegIdx, &outNBElement, g_outIntBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += g_outIntBuf0[0];
        }
        printf("CDL2CROWS %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "CDL3BLACKCROWS") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_CDL3BLACKCROWS(0, g_nPoints - 1, g_open, g_high, g_low, g_close, &outBegIdx, &outNBElement, g_outIntBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += g_outIntBuf0[0];
        }
        printf("CDL3BLACKCROWS %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "CDL3INSIDE") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_CDL3INSIDE(0, g_nPoints - 1, g_open, g_high, g_low, g_close, &outBegIdx, &outNBElement, g_outIntBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += g_outIntBuf0[0];
        }
        printf("CDL3INSIDE %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "CDL3LINESTRIKE") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_CDL3LINESTRIKE(0, g_nPoints - 1, g_open, g_high, g_low, g_close, &outBegIdx, &outNBElement, g_outIntBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += g_outIntBuf0[0];
        }
        printf("CDL3LINESTRIKE %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "CDL3OUTSIDE") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_CDL3OUTSIDE(0, g_nPoints - 1, g_open, g_high, g_low, g_close, &outBegIdx, &outNBElement, g_outIntBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += g_outIntBuf0[0];
        }
        printf("CDL3OUTSIDE %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "CDL3STARSINSOUTH") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_CDL3STARSINSOUTH(0, g_nPoints - 1, g_open, g_high, g_low, g_close, &outBegIdx, &outNBElement, g_outIntBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += g_outIntBuf0[0];
        }
        printf("CDL3STARSINSOUTH %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "CDL3WHITESOLDIERS") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_CDL3WHITESOLDIERS(0, g_nPoints - 1, g_open, g_high, g_low, g_close, &outBegIdx, &outNBElement, g_outIntBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += g_outIntBuf0[0];
        }
        printf("CDL3WHITESOLDIERS %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "CDLABANDONEDBABY") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_CDLABANDONEDBABY(0, g_nPoints - 1, g_open, g_high, g_low, g_close, 0.300000000000000, &outBegIdx, &outNBElement, g_outIntBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += g_outIntBuf0[0];
        }
        printf("CDLABANDONEDBABY %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "CDLADVANCEBLOCK") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_CDLADVANCEBLOCK(0, g_nPoints - 1, g_open, g_high, g_low, g_close, &outBegIdx, &outNBElement, g_outIntBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += g_outIntBuf0[0];
        }
        printf("CDLADVANCEBLOCK %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "CDLBELTHOLD") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_CDLBELTHOLD(0, g_nPoints - 1, g_open, g_high, g_low, g_close, &outBegIdx, &outNBElement, g_outIntBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += g_outIntBuf0[0];
        }
        printf("CDLBELTHOLD %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "CDLBREAKAWAY") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_CDLBREAKAWAY(0, g_nPoints - 1, g_open, g_high, g_low, g_close, &outBegIdx, &outNBElement, g_outIntBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += g_outIntBuf0[0];
        }
        printf("CDLBREAKAWAY %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "CDLCLOSINGMARUBOZU") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_CDLCLOSINGMARUBOZU(0, g_nPoints - 1, g_open, g_high, g_low, g_close, &outBegIdx, &outNBElement, g_outIntBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += g_outIntBuf0[0];
        }
        printf("CDLCLOSINGMARUBOZU %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "CDLCONCEALBABYSWALL") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_CDLCONCEALBABYSWALL(0, g_nPoints - 1, g_open, g_high, g_low, g_close, &outBegIdx, &outNBElement, g_outIntBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += g_outIntBuf0[0];
        }
        printf("CDLCONCEALBABYSWALL %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "CDLCOUNTERATTACK") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_CDLCOUNTERATTACK(0, g_nPoints - 1, g_open, g_high, g_low, g_close, &outBegIdx, &outNBElement, g_outIntBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += g_outIntBuf0[0];
        }
        printf("CDLCOUNTERATTACK %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "CDLDARKCLOUDCOVER") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_CDLDARKCLOUDCOVER(0, g_nPoints - 1, g_open, g_high, g_low, g_close, 0.500000000000000, &outBegIdx, &outNBElement, g_outIntBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += g_outIntBuf0[0];
        }
        printf("CDLDARKCLOUDCOVER %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "CDLDOJI") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_CDLDOJI(0, g_nPoints - 1, g_open, g_high, g_low, g_close, &outBegIdx, &outNBElement, g_outIntBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += g_outIntBuf0[0];
        }
        printf("CDLDOJI %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "CDLDOJISTAR") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_CDLDOJISTAR(0, g_nPoints - 1, g_open, g_high, g_low, g_close, &outBegIdx, &outNBElement, g_outIntBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += g_outIntBuf0[0];
        }
        printf("CDLDOJISTAR %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "CDLDRAGONFLYDOJI") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_CDLDRAGONFLYDOJI(0, g_nPoints - 1, g_open, g_high, g_low, g_close, &outBegIdx, &outNBElement, g_outIntBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += g_outIntBuf0[0];
        }
        printf("CDLDRAGONFLYDOJI %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "CDLENGULFING") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_CDLENGULFING(0, g_nPoints - 1, g_open, g_high, g_low, g_close, &outBegIdx, &outNBElement, g_outIntBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += g_outIntBuf0[0];
        }
        printf("CDLENGULFING %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "CDLEVENINGDOJISTAR") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_CDLEVENINGDOJISTAR(0, g_nPoints - 1, g_open, g_high, g_low, g_close, 0.300000000000000, &outBegIdx, &outNBElement, g_outIntBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += g_outIntBuf0[0];
        }
        printf("CDLEVENINGDOJISTAR %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "CDLEVENINGSTAR") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_CDLEVENINGSTAR(0, g_nPoints - 1, g_open, g_high, g_low, g_close, 0.300000000000000, &outBegIdx, &outNBElement, g_outIntBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += g_outIntBuf0[0];
        }
        printf("CDLEVENINGSTAR %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "CDLGAPSIDESIDEWHITE") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_CDLGAPSIDESIDEWHITE(0, g_nPoints - 1, g_open, g_high, g_low, g_close, &outBegIdx, &outNBElement, g_outIntBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += g_outIntBuf0[0];
        }
        printf("CDLGAPSIDESIDEWHITE %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "CDLGRAVESTONEDOJI") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_CDLGRAVESTONEDOJI(0, g_nPoints - 1, g_open, g_high, g_low, g_close, &outBegIdx, &outNBElement, g_outIntBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += g_outIntBuf0[0];
        }
        printf("CDLGRAVESTONEDOJI %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "CDLHAMMER") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_CDLHAMMER(0, g_nPoints - 1, g_open, g_high, g_low, g_close, &outBegIdx, &outNBElement, g_outIntBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += g_outIntBuf0[0];
        }
        printf("CDLHAMMER %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "CDLHANGINGMAN") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_CDLHANGINGMAN(0, g_nPoints - 1, g_open, g_high, g_low, g_close, &outBegIdx, &outNBElement, g_outIntBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += g_outIntBuf0[0];
        }
        printf("CDLHANGINGMAN %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "CDLHARAMI") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_CDLHARAMI(0, g_nPoints - 1, g_open, g_high, g_low, g_close, &outBegIdx, &outNBElement, g_outIntBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += g_outIntBuf0[0];
        }
        printf("CDLHARAMI %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "CDLHARAMICROSS") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_CDLHARAMICROSS(0, g_nPoints - 1, g_open, g_high, g_low, g_close, &outBegIdx, &outNBElement, g_outIntBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += g_outIntBuf0[0];
        }
        printf("CDLHARAMICROSS %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "CDLHIGHWAVE") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_CDLHIGHWAVE(0, g_nPoints - 1, g_open, g_high, g_low, g_close, &outBegIdx, &outNBElement, g_outIntBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += g_outIntBuf0[0];
        }
        printf("CDLHIGHWAVE %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "CDLHIKKAKE") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_CDLHIKKAKE(0, g_nPoints - 1, g_open, g_high, g_low, g_close, &outBegIdx, &outNBElement, g_outIntBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += g_outIntBuf0[0];
        }
        printf("CDLHIKKAKE %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "CDLHIKKAKEMOD") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_CDLHIKKAKEMOD(0, g_nPoints - 1, g_open, g_high, g_low, g_close, &outBegIdx, &outNBElement, g_outIntBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += g_outIntBuf0[0];
        }
        printf("CDLHIKKAKEMOD %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "CDLHOMINGPIGEON") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_CDLHOMINGPIGEON(0, g_nPoints - 1, g_open, g_high, g_low, g_close, &outBegIdx, &outNBElement, g_outIntBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += g_outIntBuf0[0];
        }
        printf("CDLHOMINGPIGEON %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "CDLIDENTICAL3CROWS") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_CDLIDENTICAL3CROWS(0, g_nPoints - 1, g_open, g_high, g_low, g_close, &outBegIdx, &outNBElement, g_outIntBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += g_outIntBuf0[0];
        }
        printf("CDLIDENTICAL3CROWS %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "CDLINNECK") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_CDLINNECK(0, g_nPoints - 1, g_open, g_high, g_low, g_close, &outBegIdx, &outNBElement, g_outIntBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += g_outIntBuf0[0];
        }
        printf("CDLINNECK %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "CDLINVERTEDHAMMER") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_CDLINVERTEDHAMMER(0, g_nPoints - 1, g_open, g_high, g_low, g_close, &outBegIdx, &outNBElement, g_outIntBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += g_outIntBuf0[0];
        }
        printf("CDLINVERTEDHAMMER %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "CDLKICKING") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_CDLKICKING(0, g_nPoints - 1, g_open, g_high, g_low, g_close, &outBegIdx, &outNBElement, g_outIntBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += g_outIntBuf0[0];
        }
        printf("CDLKICKING %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "CDLKICKINGBYLENGTH") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_CDLKICKINGBYLENGTH(0, g_nPoints - 1, g_open, g_high, g_low, g_close, &outBegIdx, &outNBElement, g_outIntBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += g_outIntBuf0[0];
        }
        printf("CDLKICKINGBYLENGTH %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "CDLLADDERBOTTOM") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_CDLLADDERBOTTOM(0, g_nPoints - 1, g_open, g_high, g_low, g_close, &outBegIdx, &outNBElement, g_outIntBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += g_outIntBuf0[0];
        }
        printf("CDLLADDERBOTTOM %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "CDLLONGLEGGEDDOJI") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_CDLLONGLEGGEDDOJI(0, g_nPoints - 1, g_open, g_high, g_low, g_close, &outBegIdx, &outNBElement, g_outIntBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += g_outIntBuf0[0];
        }
        printf("CDLLONGLEGGEDDOJI %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "CDLLONGLINE") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_CDLLONGLINE(0, g_nPoints - 1, g_open, g_high, g_low, g_close, &outBegIdx, &outNBElement, g_outIntBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += g_outIntBuf0[0];
        }
        printf("CDLLONGLINE %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "CDLMARUBOZU") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_CDLMARUBOZU(0, g_nPoints - 1, g_open, g_high, g_low, g_close, &outBegIdx, &outNBElement, g_outIntBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += g_outIntBuf0[0];
        }
        printf("CDLMARUBOZU %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "CDLMATCHINGLOW") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_CDLMATCHINGLOW(0, g_nPoints - 1, g_open, g_high, g_low, g_close, &outBegIdx, &outNBElement, g_outIntBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += g_outIntBuf0[0];
        }
        printf("CDLMATCHINGLOW %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "CDLMATHOLD") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_CDLMATHOLD(0, g_nPoints - 1, g_open, g_high, g_low, g_close, 0.500000000000000, &outBegIdx, &outNBElement, g_outIntBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += g_outIntBuf0[0];
        }
        printf("CDLMATHOLD %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "CDLMORNINGDOJISTAR") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_CDLMORNINGDOJISTAR(0, g_nPoints - 1, g_open, g_high, g_low, g_close, 0.300000000000000, &outBegIdx, &outNBElement, g_outIntBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += g_outIntBuf0[0];
        }
        printf("CDLMORNINGDOJISTAR %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "CDLMORNINGSTAR") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_CDLMORNINGSTAR(0, g_nPoints - 1, g_open, g_high, g_low, g_close, 0.300000000000000, &outBegIdx, &outNBElement, g_outIntBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += g_outIntBuf0[0];
        }
        printf("CDLMORNINGSTAR %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "CDLONNECK") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_CDLONNECK(0, g_nPoints - 1, g_open, g_high, g_low, g_close, &outBegIdx, &outNBElement, g_outIntBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += g_outIntBuf0[0];
        }
        printf("CDLONNECK %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "CDLPIERCING") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_CDLPIERCING(0, g_nPoints - 1, g_open, g_high, g_low, g_close, &outBegIdx, &outNBElement, g_outIntBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += g_outIntBuf0[0];
        }
        printf("CDLPIERCING %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "CDLRICKSHAWMAN") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_CDLRICKSHAWMAN(0, g_nPoints - 1, g_open, g_high, g_low, g_close, &outBegIdx, &outNBElement, g_outIntBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += g_outIntBuf0[0];
        }
        printf("CDLRICKSHAWMAN %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "CDLRISEFALL3METHODS") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_CDLRISEFALL3METHODS(0, g_nPoints - 1, g_open, g_high, g_low, g_close, &outBegIdx, &outNBElement, g_outIntBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += g_outIntBuf0[0];
        }
        printf("CDLRISEFALL3METHODS %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "CDLSEPARATINGLINES") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_CDLSEPARATINGLINES(0, g_nPoints - 1, g_open, g_high, g_low, g_close, &outBegIdx, &outNBElement, g_outIntBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += g_outIntBuf0[0];
        }
        printf("CDLSEPARATINGLINES %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "CDLSHOOTINGSTAR") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_CDLSHOOTINGSTAR(0, g_nPoints - 1, g_open, g_high, g_low, g_close, &outBegIdx, &outNBElement, g_outIntBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += g_outIntBuf0[0];
        }
        printf("CDLSHOOTINGSTAR %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "CDLSHORTLINE") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_CDLSHORTLINE(0, g_nPoints - 1, g_open, g_high, g_low, g_close, &outBegIdx, &outNBElement, g_outIntBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += g_outIntBuf0[0];
        }
        printf("CDLSHORTLINE %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "CDLSPINNINGTOP") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_CDLSPINNINGTOP(0, g_nPoints - 1, g_open, g_high, g_low, g_close, &outBegIdx, &outNBElement, g_outIntBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += g_outIntBuf0[0];
        }
        printf("CDLSPINNINGTOP %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "CDLSTALLEDPATTERN") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_CDLSTALLEDPATTERN(0, g_nPoints - 1, g_open, g_high, g_low, g_close, &outBegIdx, &outNBElement, g_outIntBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += g_outIntBuf0[0];
        }
        printf("CDLSTALLEDPATTERN %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "CDLSTICKSANDWICH") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_CDLSTICKSANDWICH(0, g_nPoints - 1, g_open, g_high, g_low, g_close, &outBegIdx, &outNBElement, g_outIntBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += g_outIntBuf0[0];
        }
        printf("CDLSTICKSANDWICH %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "CDLTAKURI") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_CDLTAKURI(0, g_nPoints - 1, g_open, g_high, g_low, g_close, &outBegIdx, &outNBElement, g_outIntBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += g_outIntBuf0[0];
        }
        printf("CDLTAKURI %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "CDLTASUKIGAP") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_CDLTASUKIGAP(0, g_nPoints - 1, g_open, g_high, g_low, g_close, &outBegIdx, &outNBElement, g_outIntBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += g_outIntBuf0[0];
        }
        printf("CDLTASUKIGAP %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "CDLTHRUSTING") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_CDLTHRUSTING(0, g_nPoints - 1, g_open, g_high, g_low, g_close, &outBegIdx, &outNBElement, g_outIntBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += g_outIntBuf0[0];
        }
        printf("CDLTHRUSTING %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "CDLTRISTAR") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_CDLTRISTAR(0, g_nPoints - 1, g_open, g_high, g_low, g_close, &outBegIdx, &outNBElement, g_outIntBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += g_outIntBuf0[0];
        }
        printf("CDLTRISTAR %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "CDLUNIQUE3RIVER") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_CDLUNIQUE3RIVER(0, g_nPoints - 1, g_open, g_high, g_low, g_close, &outBegIdx, &outNBElement, g_outIntBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += g_outIntBuf0[0];
        }
        printf("CDLUNIQUE3RIVER %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "CDLUPSIDEGAP2CROWS") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_CDLUPSIDEGAP2CROWS(0, g_nPoints - 1, g_open, g_high, g_low, g_close, &outBegIdx, &outNBElement, g_outIntBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += g_outIntBuf0[0];
        }
        printf("CDLUPSIDEGAP2CROWS %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "CDLXSIDEGAP3METHODS") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_CDLXSIDEGAP3METHODS(0, g_nPoints - 1, g_open, g_high, g_low, g_close, &outBegIdx, &outNBElement, g_outIntBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += g_outIntBuf0[0];
        }
        printf("CDLXSIDEGAP3METHODS %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "CEIL") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_CEIL(0, g_nPoints - 1, g_close, &outBegIdx, &outNBElement, g_outBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += (int)g_outBuf0[0];
        }
        printf("CEIL %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "CMO") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_CMO(0, g_nPoints - 1, g_close, 14, &outBegIdx, &outNBElement, g_outBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += (int)g_outBuf0[0];
        }
        printf("CMO %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "CORREL") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_CORREL(0, g_nPoints - 1, g_close, g_high, 30, &outBegIdx, &outNBElement, g_outBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += (int)g_outBuf0[0];
        }
        printf("CORREL %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "COS") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_COS(0, g_nPoints - 1, g_close, &outBegIdx, &outNBElement, g_outBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += (int)g_outBuf0[0];
        }
        printf("COS %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "COSH") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_COSH(0, g_nPoints - 1, g_close, &outBegIdx, &outNBElement, g_outBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += (int)g_outBuf0[0];
        }
        printf("COSH %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "DEMA") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_DEMA(0, g_nPoints - 1, g_close, 30, &outBegIdx, &outNBElement, g_outBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += (int)g_outBuf0[0];
        }
        printf("DEMA %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "DIV") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_DIV(0, g_nPoints - 1, g_close, g_high, &outBegIdx, &outNBElement, g_outBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += (int)g_outBuf0[0];
        }
        printf("DIV %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "DX") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_DX(0, g_nPoints - 1, g_high, g_low, g_close, 14, &outBegIdx, &outNBElement, g_outBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += (int)g_outBuf0[0];
        }
        printf("DX %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "EMA") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_EMA(0, g_nPoints - 1, g_close, 30, &outBegIdx, &outNBElement, g_outBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += (int)g_outBuf0[0];
        }
        printf("EMA %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "EXP") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_EXP(0, g_nPoints - 1, g_close, &outBegIdx, &outNBElement, g_outBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += (int)g_outBuf0[0];
        }
        printf("EXP %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "FLOOR") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_FLOOR(0, g_nPoints - 1, g_close, &outBegIdx, &outNBElement, g_outBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += (int)g_outBuf0[0];
        }
        printf("FLOOR %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "HT_DCPERIOD") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_HT_DCPERIOD(0, g_nPoints - 1, g_close, &outBegIdx, &outNBElement, g_outBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += (int)g_outBuf0[0];
        }
        printf("HT_DCPERIOD %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "HT_DCPHASE") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_HT_DCPHASE(0, g_nPoints - 1, g_close, &outBegIdx, &outNBElement, g_outBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += (int)g_outBuf0[0];
        }
        printf("HT_DCPHASE %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "HT_PHASOR") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_HT_PHASOR(0, g_nPoints - 1, g_close, &outBegIdx, &outNBElement, g_outBuf0, g_outBuf1);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += (int)g_outBuf0[0];
            g_sink += (int)g_outBuf1[0];
        }
        printf("HT_PHASOR %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "HT_SINE") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_HT_SINE(0, g_nPoints - 1, g_close, &outBegIdx, &outNBElement, g_outBuf0, g_outBuf1);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += (int)g_outBuf0[0];
            g_sink += (int)g_outBuf1[0];
        }
        printf("HT_SINE %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "HT_TRENDLINE") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_HT_TRENDLINE(0, g_nPoints - 1, g_close, &outBegIdx, &outNBElement, g_outBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += (int)g_outBuf0[0];
        }
        printf("HT_TRENDLINE %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "HT_TRENDMODE") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_HT_TRENDMODE(0, g_nPoints - 1, g_close, &outBegIdx, &outNBElement, g_outIntBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += g_outIntBuf0[0];
        }
        printf("HT_TRENDMODE %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "IMI") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_IMI(0, g_nPoints - 1, g_open, g_close, 14, &outBegIdx, &outNBElement, g_outBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += (int)g_outBuf0[0];
        }
        printf("IMI %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "KAMA") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_KAMA(0, g_nPoints - 1, g_close, 30, &outBegIdx, &outNBElement, g_outBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += (int)g_outBuf0[0];
        }
        printf("KAMA %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "LINEARREG") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_LINEARREG(0, g_nPoints - 1, g_close, 14, &outBegIdx, &outNBElement, g_outBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += (int)g_outBuf0[0];
        }
        printf("LINEARREG %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "LINEARREG_ANGLE") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_LINEARREG_ANGLE(0, g_nPoints - 1, g_close, 14, &outBegIdx, &outNBElement, g_outBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += (int)g_outBuf0[0];
        }
        printf("LINEARREG_ANGLE %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "LINEARREG_INTERCEPT") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_LINEARREG_INTERCEPT(0, g_nPoints - 1, g_close, 14, &outBegIdx, &outNBElement, g_outBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += (int)g_outBuf0[0];
        }
        printf("LINEARREG_INTERCEPT %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "LINEARREG_SLOPE") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_LINEARREG_SLOPE(0, g_nPoints - 1, g_close, 14, &outBegIdx, &outNBElement, g_outBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += (int)g_outBuf0[0];
        }
        printf("LINEARREG_SLOPE %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "LN") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_LN(0, g_nPoints - 1, g_close, &outBegIdx, &outNBElement, g_outBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += (int)g_outBuf0[0];
        }
        printf("LN %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "LOG10") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_LOG10(0, g_nPoints - 1, g_close, &outBegIdx, &outNBElement, g_outBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += (int)g_outBuf0[0];
        }
        printf("LOG10 %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "MA") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_MA(0, g_nPoints - 1, g_close, 30, 0, &outBegIdx, &outNBElement, g_outBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += (int)g_outBuf0[0];
        }
        printf("MA %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "MACD") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_MACD(0, g_nPoints - 1, g_close, 12, 26, 9, &outBegIdx, &outNBElement, g_outBuf0, g_outBuf1, g_outBuf2);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += (int)g_outBuf0[0];
            g_sink += (int)g_outBuf1[0];
            g_sink += (int)g_outBuf2[0];
        }
        printf("MACD %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "MACDEXT") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_MACDEXT(0, g_nPoints - 1, g_close, 12, 0, 26, 0, 9, 0, &outBegIdx, &outNBElement, g_outBuf0, g_outBuf1, g_outBuf2);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += (int)g_outBuf0[0];
            g_sink += (int)g_outBuf1[0];
            g_sink += (int)g_outBuf2[0];
        }
        printf("MACDEXT %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "MACDFIX") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_MACDFIX(0, g_nPoints - 1, g_close, 9, &outBegIdx, &outNBElement, g_outBuf0, g_outBuf1, g_outBuf2);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += (int)g_outBuf0[0];
            g_sink += (int)g_outBuf1[0];
            g_sink += (int)g_outBuf2[0];
        }
        printf("MACDFIX %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "MAMA") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_MAMA(0, g_nPoints - 1, g_close, 0.500000000000000, 0.050000000000000, &outBegIdx, &outNBElement, g_outBuf0, g_outBuf1);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += (int)g_outBuf0[0];
            g_sink += (int)g_outBuf1[0];
        }
        printf("MAMA %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "MAVP") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_MAVP(0, g_nPoints - 1, g_close, g_high, 2, 30, 0, &outBegIdx, &outNBElement, g_outBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += (int)g_outBuf0[0];
        }
        printf("MAVP %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "MAX") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_MAX(0, g_nPoints - 1, g_close, 30, &outBegIdx, &outNBElement, g_outBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += (int)g_outBuf0[0];
        }
        printf("MAX %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "MAXINDEX") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_MAXINDEX(0, g_nPoints - 1, g_close, 30, &outBegIdx, &outNBElement, g_outIntBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += g_outIntBuf0[0];
        }
        printf("MAXINDEX %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "MEDPRICE") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_MEDPRICE(0, g_nPoints - 1, g_high, g_low, &outBegIdx, &outNBElement, g_outBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += (int)g_outBuf0[0];
        }
        printf("MEDPRICE %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "MFI") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_MFI(0, g_nPoints - 1, g_high, g_low, g_close, g_volume, 14, &outBegIdx, &outNBElement, g_outBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += (int)g_outBuf0[0];
        }
        printf("MFI %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "MIDPOINT") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_MIDPOINT(0, g_nPoints - 1, g_close, 14, &outBegIdx, &outNBElement, g_outBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += (int)g_outBuf0[0];
        }
        printf("MIDPOINT %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "MIDPRICE") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_MIDPRICE(0, g_nPoints - 1, g_high, g_low, 14, &outBegIdx, &outNBElement, g_outBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += (int)g_outBuf0[0];
        }
        printf("MIDPRICE %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "MIN") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_MIN(0, g_nPoints - 1, g_close, 30, &outBegIdx, &outNBElement, g_outBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += (int)g_outBuf0[0];
        }
        printf("MIN %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "MININDEX") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_MININDEX(0, g_nPoints - 1, g_close, 30, &outBegIdx, &outNBElement, g_outIntBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += g_outIntBuf0[0];
        }
        printf("MININDEX %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "MINMAX") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_MINMAX(0, g_nPoints - 1, g_close, 30, &outBegIdx, &outNBElement, g_outBuf0, g_outBuf1);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += (int)g_outBuf0[0];
            g_sink += (int)g_outBuf1[0];
        }
        printf("MINMAX %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "MINMAXINDEX") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_MINMAXINDEX(0, g_nPoints - 1, g_close, 30, &outBegIdx, &outNBElement, g_outIntBuf0, g_outIntBuf1);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += g_outIntBuf0[0];
            g_sink += g_outIntBuf1[0];
        }
        printf("MINMAXINDEX %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "MINUS_DI") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_MINUS_DI(0, g_nPoints - 1, g_high, g_low, g_close, 14, &outBegIdx, &outNBElement, g_outBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += (int)g_outBuf0[0];
        }
        printf("MINUS_DI %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "MINUS_DM") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_MINUS_DM(0, g_nPoints - 1, g_high, g_low, 14, &outBegIdx, &outNBElement, g_outBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += (int)g_outBuf0[0];
        }
        printf("MINUS_DM %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "MOM") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_MOM(0, g_nPoints - 1, g_close, 10, &outBegIdx, &outNBElement, g_outBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += (int)g_outBuf0[0];
        }
        printf("MOM %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "MULT") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_MULT(0, g_nPoints - 1, g_close, g_high, &outBegIdx, &outNBElement, g_outBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += (int)g_outBuf0[0];
        }
        printf("MULT %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "NATR") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_NATR(0, g_nPoints - 1, g_high, g_low, g_close, 14, &outBegIdx, &outNBElement, g_outBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += (int)g_outBuf0[0];
        }
        printf("NATR %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "OBV") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_OBV(0, g_nPoints - 1, g_close, g_volume, &outBegIdx, &outNBElement, g_outBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += (int)g_outBuf0[0];
        }
        printf("OBV %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "PLUS_DI") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_PLUS_DI(0, g_nPoints - 1, g_high, g_low, g_close, 14, &outBegIdx, &outNBElement, g_outBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += (int)g_outBuf0[0];
        }
        printf("PLUS_DI %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "PLUS_DM") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_PLUS_DM(0, g_nPoints - 1, g_high, g_low, 14, &outBegIdx, &outNBElement, g_outBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += (int)g_outBuf0[0];
        }
        printf("PLUS_DM %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "PPO") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_PPO(0, g_nPoints - 1, g_close, 12, 26, 1, &outBegIdx, &outNBElement, g_outBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += (int)g_outBuf0[0];
        }
        printf("PPO %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "PVO") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_PVO(0, g_nPoints - 1, g_volume, 12, 26, 1, &outBegIdx, &outNBElement, g_outBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += (int)g_outBuf0[0];
        }
        printf("PVO %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "ROC") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_ROC(0, g_nPoints - 1, g_close, 10, &outBegIdx, &outNBElement, g_outBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += (int)g_outBuf0[0];
        }
        printf("ROC %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "ROCP") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_ROCP(0, g_nPoints - 1, g_close, 10, &outBegIdx, &outNBElement, g_outBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += (int)g_outBuf0[0];
        }
        printf("ROCP %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "ROCR") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_ROCR(0, g_nPoints - 1, g_close, 10, &outBegIdx, &outNBElement, g_outBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += (int)g_outBuf0[0];
        }
        printf("ROCR %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "ROCR100") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_ROCR100(0, g_nPoints - 1, g_close, 10, &outBegIdx, &outNBElement, g_outBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += (int)g_outBuf0[0];
        }
        printf("ROCR100 %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "RSI") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_RSI(0, g_nPoints - 1, g_close, 14, &outBegIdx, &outNBElement, g_outBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += (int)g_outBuf0[0];
        }
        printf("RSI %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "SAR") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_SAR(0, g_nPoints - 1, g_high, g_low, 0.020000000000000, 0.200000000000000, &outBegIdx, &outNBElement, g_outBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += (int)g_outBuf0[0];
        }
        printf("SAR %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "SAREXT") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_SAREXT(0, g_nPoints - 1, g_high, g_low, 0.000000000000000, 0.000000000000000, 0.020000000000000, 0.020000000000000, 0.200000000000000, 0.020000000000000, 0.020000000000000, 0.200000000000000, &outBegIdx, &outNBElement, g_outBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += (int)g_outBuf0[0];
        }
        printf("SAREXT %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "SIN") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_SIN(0, g_nPoints - 1, g_close, &outBegIdx, &outNBElement, g_outBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += (int)g_outBuf0[0];
        }
        printf("SIN %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "SINH") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_SINH(0, g_nPoints - 1, g_close, &outBegIdx, &outNBElement, g_outBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += (int)g_outBuf0[0];
        }
        printf("SINH %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "SMA") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_SMA(0, g_nPoints - 1, g_close, 30, &outBegIdx, &outNBElement, g_outBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += (int)g_outBuf0[0];
        }
        printf("SMA %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "SQRT") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_SQRT(0, g_nPoints - 1, g_close, &outBegIdx, &outNBElement, g_outBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += (int)g_outBuf0[0];
        }
        printf("SQRT %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "STDDEV") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_STDDEV(0, g_nPoints - 1, g_close, 5, 1.000000000000000, &outBegIdx, &outNBElement, g_outBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += (int)g_outBuf0[0];
        }
        printf("STDDEV %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "STOCH") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_STOCH(0, g_nPoints - 1, g_high, g_low, g_close, 5, 3, 0, 3, 0, &outBegIdx, &outNBElement, g_outBuf0, g_outBuf1);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += (int)g_outBuf0[0];
            g_sink += (int)g_outBuf1[0];
        }
        printf("STOCH %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "STOCHF") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_STOCHF(0, g_nPoints - 1, g_high, g_low, g_close, 5, 3, 0, &outBegIdx, &outNBElement, g_outBuf0, g_outBuf1);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += (int)g_outBuf0[0];
            g_sink += (int)g_outBuf1[0];
        }
        printf("STOCHF %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "STOCHRSI") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_STOCHRSI(0, g_nPoints - 1, g_close, 14, 5, 3, 0, &outBegIdx, &outNBElement, g_outBuf0, g_outBuf1);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += (int)g_outBuf0[0];
            g_sink += (int)g_outBuf1[0];
        }
        printf("STOCHRSI %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "SUB") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_SUB(0, g_nPoints - 1, g_close, g_high, &outBegIdx, &outNBElement, g_outBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += (int)g_outBuf0[0];
        }
        printf("SUB %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "SUM") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_SUM(0, g_nPoints - 1, g_close, 30, &outBegIdx, &outNBElement, g_outBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += (int)g_outBuf0[0];
        }
        printf("SUM %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "T3") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_T3(0, g_nPoints - 1, g_close, 5, 0.700000000000000, &outBegIdx, &outNBElement, g_outBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += (int)g_outBuf0[0];
        }
        printf("T3 %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "TAN") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_TAN(0, g_nPoints - 1, g_close, &outBegIdx, &outNBElement, g_outBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += (int)g_outBuf0[0];
        }
        printf("TAN %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "TANH") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_TANH(0, g_nPoints - 1, g_close, &outBegIdx, &outNBElement, g_outBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += (int)g_outBuf0[0];
        }
        printf("TANH %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "TEMA") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_TEMA(0, g_nPoints - 1, g_close, 30, &outBegIdx, &outNBElement, g_outBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += (int)g_outBuf0[0];
        }
        printf("TEMA %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "TRANGE") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_TRANGE(0, g_nPoints - 1, g_high, g_low, g_close, &outBegIdx, &outNBElement, g_outBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += (int)g_outBuf0[0];
        }
        printf("TRANGE %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "TRIMA") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_TRIMA(0, g_nPoints - 1, g_close, 30, &outBegIdx, &outNBElement, g_outBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += (int)g_outBuf0[0];
        }
        printf("TRIMA %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "TRIX") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_TRIX(0, g_nPoints - 1, g_close, 30, &outBegIdx, &outNBElement, g_outBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += (int)g_outBuf0[0];
        }
        printf("TRIX %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "TSF") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_TSF(0, g_nPoints - 1, g_close, 14, &outBegIdx, &outNBElement, g_outBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += (int)g_outBuf0[0];
        }
        printf("TSF %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "TYPPRICE") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_TYPPRICE(0, g_nPoints - 1, g_high, g_low, g_close, &outBegIdx, &outNBElement, g_outBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += (int)g_outBuf0[0];
        }
        printf("TYPPRICE %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "ULTOSC") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_ULTOSC(0, g_nPoints - 1, g_high, g_low, g_close, 7, 14, 28, &outBegIdx, &outNBElement, g_outBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += (int)g_outBuf0[0];
        }
        printf("ULTOSC %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "VAR") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_VAR(0, g_nPoints - 1, g_close, 5, 1.000000000000000, &outBegIdx, &outNBElement, g_outBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += (int)g_outBuf0[0];
        }
        printf("VAR %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "WCLPRICE") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_WCLPRICE(0, g_nPoints - 1, g_high, g_low, g_close, &outBegIdx, &outNBElement, g_outBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += (int)g_outBuf0[0];
        }
        printf("WCLPRICE %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "WILLR") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_WILLR(0, g_nPoints - 1, g_high, g_low, g_close, 14, &outBegIdx, &outNBElement, g_outBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += (int)g_outBuf0[0];
        }
        printf("WILLR %lld\n", best / iters);
        fflush(stdout);
    }
    if( func_matches(filter, "WMA") ) {
        long long best = 0;
        for( int pass = 0; pass < 3; pass++ ) {
            int outBegIdx, outNBElement;
            long long t0 = get_nanotime();
            for( int it = 0; it < iters; it++ ) {
                TA_WMA(0, g_nPoints - 1, g_close, 30, &outBegIdx, &outNBElement, g_outBuf0);
            }
            long long elapsed = get_nanotime() - t0;
            if( !best || elapsed < best ) best = elapsed;
            g_sink += outNBElement;
            g_sink += (int)g_outBuf0[0];
        }
        printf("WMA %lld\n", best / iters);
        fflush(stdout);
    }
}


int main(int argc, char *argv[]) {
    TA_Initialize();
    int n_points = 100000;
    int n_iters = 200;
    const char *func_filter = NULL;
    for( int i = 1; i < argc; i++ ) {
        if( strncmp(argv[i], "--points=", 9) == 0 )    n_points = atoi(argv[i]+9);
        else if( strncmp(argv[i], "--iters=", 8) == 0 ) n_iters = atoi(argv[i]+8);
        else if( strncmp(argv[i], "--function=", 11) == 0 ) func_filter = argv[i]+11;
    }
    if( n_points > MAX_POINTS ) n_points = MAX_POINTS;
    generate_price_data(n_points);
    bench_all(func_filter, n_iters);
    free(g_open); free(g_high); free(g_low); free(g_close); free(g_volume); free(g_oi);
    return 0;
}
