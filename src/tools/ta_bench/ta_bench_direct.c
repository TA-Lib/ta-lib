/* ta_bench_direct — Zero-overhead direct-call benchmark.
 *
 * Reference: TA_CallFunc via libta-lib.a (separate TU, extern globals)
 * Codegen:   ta_bench_cg binary (#include single TU, static globals)
 *
 * No JSON-RPC, no pipes on the hot path.
 *
 * Usage:
 *   ./ta_bench_direct [--points=N] [--iters=N] [--function=RSI,SMA]
 *                     [--shape=NAME] [--seed=N] [--regime-period=N] [--trend-strength=F]
 *                     [--list-shapes] [--verify-corpus]
 *
 * --shape picks the input class from the corpus in bench_corpus.h; the default
 * reproduces the series this tool has always used. --list-shapes prints them.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#ifdef __APPLE__
#include <mach/mach_time.h>
#endif
#if defined(WIN32) || defined(_WIN32)
#include <windows.h>
/* MSVC portability: popen/pclose are prefixed, strcasestr is a GNU
 * extension with no Windows equivalent. */
#define popen  _popen
#define pclose _pclose
static char *win_strcasestr(const char *haystack, const char *needle)
{
    size_t nlen = strlen(needle);
    if( nlen == 0 ) return (char *)haystack;
    for( ; *haystack; haystack++ )
    {
        if( _strnicmp(haystack, needle, nlen) == 0 )
            return (char *)haystack;
    }
    return NULL;
}
#define strcasestr win_strcasestr
#endif

#include "ta_libc.h"
#include "bench_corpus.h"

/* ---- Configuration ---- */

#define MAX_POINTS     200000
#define DEFAULT_POINTS 100000
#define DEFAULT_ITERS  200
#define MAX_FUNCTIONS  200
#define BENCH_PASSES   3

/* ---- Timing ---- */

static long long get_nanotime(void) {
#ifdef __APPLE__
    static mach_timebase_info_data_t info = {0, 0};
    if( info.denom == 0 ) mach_timebase_info(&info);
    uint64_t t = mach_absolute_time();
    return (long long)(t * info.numer / info.denom);
#elif defined(WIN32) || defined(_WIN32)
    static LARGE_INTEGER freq = {0};
    LARGE_INTEGER t;
    if( freq.QuadPart == 0 ) QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&t);
    return (t.QuadPart / freq.QuadPart) * 1000000000LL
         + (t.QuadPart % freq.QuadPart) * 1000000000LL / freq.QuadPart;
#else
    struct timespec ts;
    if( clock_gettime(CLOCK_MONOTONIC, &ts) == 0 )
        return (long long)ts.tv_sec * 1000000000LL + (long long)ts.tv_nsec;
    return 0;
#endif
}

/* ---- Test data (same corpus as ta_bench / ta_bench_cg — bench_corpus.h) ---- */

static TA_Real *g_open, *g_high, *g_low, *g_close, *g_volume, *g_oi;
/* MAVP's per-bar period series, in its [2,30] range. Kept separate from the
 * price arrays: it is the only TA_Input_Real that is not a price. */
static TA_Real *g_periods;
static int g_nPoints;

static void generate_price_data(int n, const BenchCorpusCfg *corpus) {
    g_nPoints = n;
    g_open   = calloc(n, sizeof(TA_Real));
    g_high   = calloc(n, sizeof(TA_Real));
    g_low    = calloc(n, sizeof(TA_Real));
    g_close  = calloc(n, sizeof(TA_Real));
    g_volume = calloc(n, sizeof(TA_Real));
    g_oi     = calloc(n, sizeof(TA_Real));
    g_periods = calloc(n, sizeof(TA_Real));
    bench_corpus_gen(corpus, n, g_open, g_high, g_low, g_close, g_volume, g_oi,
                     g_periods);
}

/* ---- Output buffers ---- */

static TA_Real g_outReal0[MAX_POINTS];
static TA_Real g_outReal1[MAX_POINTS];
static TA_Real g_outReal2[MAX_POINTS];
static TA_Integer g_outInt0[MAX_POINTS];
static TA_Integer g_outInt1[MAX_POINTS];

/* ---- Function filter ---- */

static int func_matches(const char *filter, const char *name) {
    if( !filter ) return 1;
    char buf[512]; strncpy(buf, filter, sizeof(buf)-1); buf[sizeof(buf)-1]='\0';
    for( char *tok = strtok(buf, ","); tok; tok = strtok(NULL, ",") )
        if( strcasestr(name, tok) ) return 1;
    return 0;
}

/* ---- Reference timing storage ---- */

typedef struct {
    char name[64];
    long long ref_ns;
    long long cg_ns;
} BenchResult;

static BenchResult g_results[MAX_FUNCTIONS];
static int g_nResults = 0;

/* ---- Callback context ---- */

typedef struct {
    const char *filter;
    int iters;
} BenchCallbackCtx;

/* ---- Bench one reference function via TA_CallFunc ---- */

static void bench_ref_func(const TA_FuncInfo *fi, void *opaque) {
    BenchCallbackCtx *ctx = (BenchCallbackCtx *)opaque;
    if( !func_matches(ctx->filter, fi->name) ) return;
    if( g_nResults >= MAX_FUNCTIONS ) return;

    TA_ParamHolder *params = NULL;
    TA_ParamHolderAlloc(fi->handle, &params);

    /* Set inputs */
    for( unsigned int i = 0; i < fi->nbInput; i++ ) {
        const TA_InputParameterInfo *info;
        TA_GetInputParameterInfo(fi->handle, i, &info);
        if( info->type == TA_Input_Price ) {
            TA_SetInputParamPricePtr(params, i, g_open, g_high, g_low, g_close, g_volume, g_oi);
        } else {
            /* TA_Input_Real — close for the first, high for the second, EXCEPT
             * MAVP's inPeriods, which is a per-bar period series and not a
             * price. Feeding it g_high ran MAVP with periods in the hundreds
             * while ta_bench_cg used the corpus's [2,30] series, so the ratio
             * printed below compared two different workloads (the known ~24x
             * false MAVP regression). Matched by name, as bench_gen.rs does. */
            const TA_Real *src = (i == 0) ? g_close : g_high;
            if( info->paramName && strcmp(info->paramName, "inPeriods") == 0 )
                src = g_periods;
            TA_SetInputParamRealPtr(params, i, src);
        }
    }

    /* Set outputs */
    unsigned int realIdx = 0, intIdx = 0;
    for( unsigned int i = 0; i < fi->nbOutput; i++ ) {
        const TA_OutputParameterInfo *info;
        TA_GetOutputParameterInfo(fi->handle, i, &info);
        if( info->type == TA_Output_Integer ) {
            TA_SetOutputParamIntegerPtr(params, i, intIdx == 0 ? g_outInt0 : g_outInt1);
            intIdx++;
        } else {
            TA_Real *buf = realIdx == 0 ? g_outReal0 : (realIdx == 1 ? g_outReal1 : g_outReal2);
            TA_SetOutputParamRealPtr(params, i, buf);
            realIdx++;
        }
    }

    /* Benchmark: 3 passes, keep minimum */
    TA_Integer outBegIdx, outNbElement;
    long long best = 0;
    for( int pass = 0; pass < BENCH_PASSES; pass++ ) {
        long long t0 = get_nanotime();
        for( int it = 0; it < ctx->iters; it++ ) {
            TA_CallFunc(params, 0, g_nPoints - 1, &outBegIdx, &outNbElement);
        }
        long long elapsed = get_nanotime() - t0;
        if( !best || elapsed < best ) best = elapsed;
    }

    TA_ParamHolderFree(params);

    strncpy(g_results[g_nResults].name, fi->name, 63);
    g_results[g_nResults].name[63] = '\0';
    g_results[g_nResults].ref_ns = best / ctx->iters;
    g_results[g_nResults].cg_ns = 0;
    g_nResults++;
}

/* ---- Main ---- */

int main(int argc, char *argv[]) {
    int n_points = DEFAULT_POINTS;
    int n_iters  = DEFAULT_ITERS;
    const char *func_filter = NULL;
    const char *shape_name = NULL;
    int verify_corpus = 0;
    BenchCorpusCfg corpus;
    int seed = BENCH_CORPUS_SEED;
    double trend_strength = BENCH_CORPUS_TREND;
    int regime_period = BENCH_CORPUS_PERIOD;
    int shape;

    for( int i = 1; i < argc; i++ ) {
        if( strncmp(argv[i], "--points=", 9) == 0 )       n_points = atoi(argv[i]+9);
        else if( strncmp(argv[i], "--iters=", 8) == 0 )    n_iters = atoi(argv[i]+8);
        else if( strncmp(argv[i], "--function=", 11) == 0 ) func_filter = argv[i]+11;
        else if( strncmp(argv[i], "--shape=", 8) == 0 )     shape_name = argv[i]+8;
        else if( strncmp(argv[i], "--seed=", 7) == 0 )      seed = atoi(argv[i]+7);
        else if( strncmp(argv[i], "--regime-period=", 16) == 0 ) regime_period = atoi(argv[i]+16);
        else if( strncmp(argv[i], "--trend-strength=", 17) == 0 ) trend_strength = atof(argv[i]+17);
        else if( strcmp(argv[i], "--list-shapes") == 0 )  { bench_shape_list(); return 0; }
        else if( strcmp(argv[i], "--verify-corpus") == 0 ) verify_corpus = 1;
        else {
            /* Reject rather than ignore: this binary forwards the corpus flags
             * to ta_bench_cg, so an unrecognised flag here would desync the two
             * halves of the ratio it prints. */
            fprintf(stderr, "ta_bench_direct: unknown option '%s'\n", argv[i]);
            return 2;
        }
    }
    if( n_points > MAX_POINTS ) n_points = MAX_POINTS;

    shape = bench_shape_id(shape_name);
    if( shape < 0 ) {
        printf("ta_bench_direct: unknown --shape=%s\n\n", shape_name);
        bench_shape_list();
        return 1;
    }

    bench_corpus_defaults(&corpus);
    corpus.shape         = shape;
    corpus.seed          = seed;
    corpus.refPeriod     = regime_period;
    corpus.trendStrength = trend_strength;

    /* At the n actually benchmarked — the walk family's floor artefacts only
     * appear around n=12000, so a small fixed n cannot see them. */
    if( verify_corpus )
        return bench_corpus_selfcheck(n_points, &corpus) ? 1 : 0;

    TA_Initialize();
    generate_price_data(n_points, &corpus);

    printf("ta_bench_direct: %d points, %d iters, shape=%s seed=%d regime-period=%d"
           " trend-strength=%.2f (direct calls)\n\n",
           n_points, n_iters, bench_shape_name(shape), seed, regime_period, trend_strength);

    /* Phase 1: Reference timing via TA_CallFunc */
    printf("  Running reference (libta-lib.a)...\n");

    BenchCallbackCtx cb = { .filter = func_filter, .iters = n_iters };
    TA_ForEachFunc(bench_ref_func, &cb);
    printf("  %d functions timed\n", g_nResults);

    /* Phase 2: Codegen timing via ta_bench_cg subprocess */
    int cg_status = 0, cg_failed = 0;
    printf("  Running codegen (ta_bench_cg)...\n");
    char cmd[1024];
    snprintf(cmd, sizeof(cmd),
             /* %.17g, not %.6g: the child must reconstruct the SAME double, or
                the two halves of the ratio below measure different series. */
             "./ta_bench_cg --points=%d --iters=%d --shape=%s --seed=%d"
             " --regime-period=%d --trend-strength=%.17g",
             n_points, n_iters, bench_shape_name(shape), seed, regime_period, trend_strength);
    if( func_filter )
        snprintf(cmd + strlen(cmd), sizeof(cmd) - strlen(cmd), " --function=%s", func_filter);

    FILE *fp = popen(cmd, "r");
    if( !fp ) {
        fprintf(stderr, "Failed to start ta_bench_cg\n");
    } else {
        char line[256];
        while( fgets(line, sizeof(line), fp) ) {
            char fname[64];
            long long ns;
            if( sscanf(line, "%63s %lld", fname, &ns) == 2 ) {
                /* Find matching result */
                for( int i = 0; i < g_nResults; i++ ) {
                    if( strcmp(g_results[i].name, fname) == 0 ) {
                        g_results[i].cg_ns = ns;
                        break;
                    }
                }
            }
        }
        /* The child's exit status is the ONLY signal that it rejected a flag we
         * forwarded (ta_bench_cg returns 2 on unknown argv). Dropping it here
         * would leave a dead or mis-invoked child looking like a 0.00x row. */
        cg_status = pclose(fp);
    }
    if( cg_status != 0 ) {
        fprintf(stderr,
                "ta_bench_direct: ta_bench_cg failed (status %d) — "
                "the codegen column below is not a measurement.\n", cg_status);
        cg_failed = 1;
    }

    /* Phase 3: Print comparison table */
    printf("\n%-20s %10s %10s %8s\n", "Function", "C-ref", "C", "Ratio");
    printf("%-20s %10s %10s %8s\n", "--------", "------", "------", "-----");

    for( int i = 0; i < g_nResults; i++ ) {
        double ratio = (g_results[i].ref_ns > 0 && g_results[i].cg_ns > 0)
            ? (double)g_results[i].cg_ns / (double)g_results[i].ref_ns
            : 0.0;
        /* No codegen number is not a 0.00x measurement — say so, and never
         * colour it, so a missing row cannot read as a spectacular win. */
        if( ratio <= 0.0 ) {
            printf("%-20s %10lld %10s %8s\n",
                   g_results[i].name, g_results[i].ref_ns, "--", "--");
            continue;
        }
        const char *clr = (ratio > 1.10) ? "\033[31m" : (ratio < 0.90) ? "\033[32m" : "";
        const char *rst = (*clr) ? "\033[0m" : "";
        printf("%-20s %10lld %s%10lld%s %7.2fx\n",
               g_results[i].name, g_results[i].ref_ns,
               clr, g_results[i].cg_ns, rst, ratio);
    }

    printf("\n%d indicators benchmarked (%d points, %d iters, shape=%s, direct calls)\n",
           g_nResults, n_points, n_iters, bench_shape_name(shape));
    printf("(red >10%% slower, green >10%% faster than C-ref)\n");

    free(g_open); free(g_high); free(g_low); free(g_close); free(g_volume); free(g_oi);
    free(g_periods);
    TA_Shutdown();
    return cg_failed ? 1 : 0;
}
