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
#include "../ta_alloc_check.h"

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
    if( !g_open || !g_high || !g_low || !g_close || !g_volume || !g_oi || !g_periods )
        TA_TOOL_OOM("the price data arrays");
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

/* Keep every sample, not just the winner. A single min hides whether the box
 * was quiet enough for the number to mean anything -- three identical runs of
 * this tool have been observed moving 120 of 168 rows across the +-10% colour
 * boundary, so the verdict was reporting noise. */
#define MAX_SAMPLES (BENCH_PASSES * 8)

typedef struct {
    char name[64];
    long long ref_ns;          /* median of ref_s */
    long long cg_ns;           /* median of cg_s  */
    long long ref_s[MAX_SAMPLES];
    long long cg_s[MAX_SAMPLES];
    int nref, ncg;
    double ref_spread, cg_spread;   /* (max-min)/median; -1 when < 2 samples */
} BenchResult;

static BenchResult g_results[MAX_FUNCTIONS];
static int g_nResults = 0;

static int ll_cmp(const void *a, const void *b) {
    long long x = *(const long long *)a, y = *(const long long *)b;
    return (x > y) - (x < y);
}

/* Median + relative spread over n samples. Returns the median, sets *spread to
 * (max-min)/median, or -1 when a single sample makes spread unknowable. */
static long long samples_stat(long long *s, int n, double *spread)
{
    if( n <= 0 ) { *spread = -1.0; return 0; }
    qsort(s, (size_t)n, sizeof(s[0]), ll_cmp);
    long long med = s[n / 2];
    *spread = (n < 2 || med <= 0) ? -1.0
            : (double)(s[n - 1] - s[0]) / (double)med;
    return med;
}

static void result_add(BenchResult *r, long long ref, long long cg)
{
    if( ref > 0 && r->nref < MAX_SAMPLES ) r->ref_s[r->nref++] = ref;
    if( cg  > 0 && r->ncg  < MAX_SAMPLES ) r->cg_s[r->ncg++]  = cg;
}

static int d_cmp(const void *a, const void *b) {
    double x = *(const double *)a, y = *(const double *)b;
    return (x > y) - (x < y);
}

/* Median of the per-row spreads: one number for "was the box quiet". */
static double samples_stat_d(double *s, int n)
{
    if( n <= 0 ) return 0.0;
    qsort(s, (size_t)n, sizeof(s[0]), d_cmp);
    return s[n / 2];
}

/* Rows accumulate across repetitions, so a second pass must land on the same
   row rather than appending a duplicate. */
static BenchResult *result_row(const char *name)
{
    for( int i = 0; i < g_nResults; i++ )
        if( strcmp(g_results[i].name, name) == 0 ) return &g_results[i];
    if( g_nResults >= MAX_FUNCTIONS ) return NULL;
    BenchResult *r = &g_results[g_nResults++];
    memset(r, 0, sizeof(*r));
    strncpy(r->name, name, sizeof(r->name) - 1);
    return r;
}

/* Appended, one object per run, same shape as ta_regtest_timing.jsonl
   (timestamp + git_sha + results) so both can be tracked by one reader. */
static void write_jsonl(const char *path, int points, int iters, int reps,
                        const char *shape, double med_spread)
{
    FILE *f = fopen(path, "a");
    if( !f ) {
        fprintf(stderr, "ta_bench_direct: cannot append to %s\n", path);
        return;
    }
    char ts[32] = "";
    time_t now = time(NULL);
    struct tm tmv;
#if defined(WIN32) || defined(_WIN32)
    if( gmtime_s(&tmv, &now) == 0 )
#else
    if( gmtime_r(&now, &tmv) )
#endif
        strftime(ts, sizeof(ts), "%Y-%m-%dT%H:%M:%SZ", &tmv);

    /* Same source as ta_regtest's report, so rows from the two line up. */
    char sha[32] = "";
    FILE *git = popen("git rev-parse --short HEAD 2>/dev/null", "r");
    if( git ) {
        if( fgets(sha, sizeof(sha), git) ) sha[strcspn(sha, "\r\n")] = '\0';
        pclose(git);
    }
    fprintf(f, "{\"timestamp\":\"%s\",\"git_sha\":\"%s\",\"tool\":\"ta_bench_direct\","
               "\"points\":%d,\"iters\":%d,\"reps\":%d,\"shape\":\"%s\","
               "\"median_spread\":%.4f,\"results\":{",
            ts, sha, points, iters, reps, shape, med_spread);
    for( int i = 0; i < g_nResults; i++ ) {
        const BenchResult *r = &g_results[i];
        fprintf(f, "%s\"%s\":{\"ref_ns\":%lld,\"cg_ns\":%lld,"
                   "\"ref_spread\":%.4f,\"cg_spread\":%.4f,\"n\":%d}",
                i ? "," : "", r->name, r->ref_ns, r->cg_ns,
                r->ref_spread, r->cg_spread, r->nref);
    }
    fprintf(f, "}}\n");
    fclose(f);
}

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

    /* Every pass is kept: the spread across them is what says whether the
     * median below is a measurement or a mood of the machine. */
    TA_Integer outBegIdx, outNbElement;
    BenchResult *r = result_row(fi->name);
    if( !r ) { TA_ParamHolderFree(params); return; }
    for( int pass = 0; pass < BENCH_PASSES; pass++ ) {
        long long t0 = get_nanotime();
        for( int it = 0; it < ctx->iters; it++ ) {
            TA_CallFunc(params, 0, g_nPoints - 1, &outBegIdx, &outNbElement);
        }
        long long elapsed = get_nanotime() - t0;
        result_add(r, elapsed / ctx->iters, 0);
    }

    TA_ParamHolderFree(params);
}

/* ---- Main ---- */

int main(int argc, char *argv[]) {
    int n_points = DEFAULT_POINTS;
    int n_iters  = DEFAULT_ITERS;
    const char *func_filter = NULL;
    const char *shape_name = NULL;
    int verify_corpus = 0;
    int n_reps = 1;
    /* Defaults chosen from measurement on this tree, not taste: the ratio's
       own run-to-run scatter sits around 20%, so a 1.20x band is the smallest
       honest one, and a median row spread above 25% means the box is too busy
       for any of it to mean anything. */
    double no_signal = 1.20;
    double max_spread = 0.25;
    const char *jsonl_path = NULL;
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
        else if( strncmp(argv[i], "--reps=", 7) == 0 )        n_reps = atoi(argv[i]+7);
        else if( strncmp(argv[i], "--max-spread=", 13) == 0 ) max_spread = atof(argv[i]+13)/100.0;
        else if( strncmp(argv[i], "--no-signal=", 12) == 0 )  no_signal = atof(argv[i]+12);
        else if( strncmp(argv[i], "--jsonl=", 8) == 0 )       jsonl_path = argv[i]+8;
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

    int cg_status = 0, cg_failed = 0;
    /* Interleaved, not phase-1-then-phase-2 N times: a thermal or scheduling
       drift over the run then hits both arms alike instead of biasing the
       ratio toward whichever arm ran while the box was quiet. */
    for( int rep = 0; rep < n_reps; rep++ ) {
    if( n_reps > 1 ) printf("  rep %d/%d\n", rep + 1, n_reps);

    /* Phase 1: Reference timing via TA_CallFunc */
    printf("  Running reference (libta-lib.a)...\n");

    BenchCallbackCtx cb = { .filter = func_filter, .iters = n_iters };
    TA_ForEachFunc(bench_ref_func, &cb);
    printf("  %d functions timed\n", g_nResults);

    /* Phase 2: Codegen timing via ta_bench_cg subprocess */
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
                BenchResult *r = result_row(fname);
                if( r ) result_add(r, 0, ns);
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
        break;
    }
    } /* rep */

    /* Collapse the samples once, before anything reads ref_ns/cg_ns. */
    for( int i = 0; i < g_nResults; i++ ) {
        BenchResult *r = &g_results[i];
        r->ref_ns = samples_stat(r->ref_s, r->nref, &r->ref_spread);
        r->cg_ns  = samples_stat(r->cg_s,  r->ncg,  &r->cg_spread);
    }

    /* Phase 3: Print comparison table.
     *
     * The two arms are DIFFERENT BUILDS of the same generated source -- C-ref is
     * libta-lib.a (separate TUs, no LTO), C is ta_bench_cg (one TU, -flto). The
     * ratio measures that, not an algorithm difference, and binary layout alone
     * moves it by more than the old +-10% colour band. So colour only outside a
     * stated no-signal band, and never colour a row whose own samples are
     * spread wider than the effect being claimed. */
    printf("\n%-20s %10s %6s %10s %6s %8s\n",
           "Function", "C-ref", "+-", "C", "+-", "Ratio");
    printf("%-20s %10s %6s %10s %6s %8s\n",
           "--------", "------", "-----", "------", "-----", "-----");

    double spreads[MAX_FUNCTIONS];
    int nspread = 0, n_noisy = 0;

    for( int i = 0; i < g_nResults; i++ ) {
        BenchResult *r = &g_results[i];
        char rs[16], cs[16];
        if( r->ref_spread >= 0.0 ) snprintf(rs, sizeof(rs), "%.0f%%", r->ref_spread * 100.0);
        else                       snprintf(rs, sizeof(rs), "%s", "?");
        if( r->cg_spread >= 0.0 )  snprintf(cs, sizeof(cs), "%.0f%%", r->cg_spread * 100.0);
        else                       snprintf(cs, sizeof(cs), "%s", "?");
        if( r->ref_spread >= 0.0 ) spreads[nspread++] = r->ref_spread;

        double ratio = (r->ref_ns > 0 && r->cg_ns > 0)
            ? (double)r->cg_ns / (double)r->ref_ns : 0.0;
        /* No codegen number is not a 0.00x measurement — say so, and never
         * colour it, so a missing row cannot read as a spectacular win. */
        if( ratio <= 0.0 ) {
            printf("%-20s %10lld %6s %10s %6s %8s\n", r->name, r->ref_ns, rs, "--", "--", "--");
            continue;
        }
        /* Widest known spread on the row. A single cg sample is not "unknown
           noise" -- ta_bench_cg already reports the min of its own 3 passes, so
           it is at least as quiet as the ref arm; fall back to the ref spread
           rather than refusing to call anything at --reps=1. */
        double noise = (r->ref_spread >= 0.0) ? r->ref_spread : 1.0e9;
        if( r->cg_spread > noise ) noise = r->cg_spread;

        int resolved = (ratio > no_signal || ratio < 1.0 / no_signal)
                    && (noise < no_signal - 1.0);
        if( !resolved && (ratio > no_signal || ratio < 1.0 / no_signal) ) n_noisy++;

        const char *clr = !resolved ? ""
                        : (ratio > 1.0) ? "\033[31m" : "\033[32m";
        const char *rst = (*clr) ? "\033[0m" : "";
        printf("%-20s %10lld %6s %10lld %6s %s%7.2fx%s\n",
               r->name, r->ref_ns, rs, r->cg_ns, cs, clr, ratio, rst);
    }

    double med_spread = samples_stat_d(spreads, nspread);
    printf("\n%d indicators benchmarked (%d points, %d iters, %d rep%s, shape=%s, direct calls)\n",
           g_nResults, n_points, n_iters, n_reps, n_reps == 1 ? "" : "s",
           bench_shape_name(shape));
    printf("Ratio = ta_bench_cg (single TU, -flto) / libta-lib.a (separate TUs, no LTO):\n"
           "a build-configuration difference, not an algorithm one. Coloured only\n"
           "outside %.2fx and only when the row's own spread is narrower than that.\n",
           no_signal);
    printf("Median per-row spread %.0f%%", med_spread * 100.0);
    if( n_noisy )
        printf("; %d row(s) exceeded %.2fx but were too noisy to call", n_noisy, no_signal);
    printf(".\n");
    if( n_reps == 1 )
        printf("Only the C column is repeated (%d passes); --reps=3 samples both arms.\n",
               BENCH_PASSES);

    int too_noisy = (max_spread > 0.0 && med_spread > max_spread);
    if( too_noisy )
        fprintf(stderr,
                "ta_bench_direct: median spread %.0f%% exceeds --max-spread=%.0f%% — "
                "these numbers are not trustworthy; quiet the machine or raise --iters.\n",
                med_spread * 100.0, max_spread * 100.0);

    if( jsonl_path ) write_jsonl(jsonl_path, n_points, n_iters, n_reps,
                                 bench_shape_name(shape), med_spread);

    free(g_open); free(g_high); free(g_low); free(g_close); free(g_volume); free(g_oi);
    free(g_periods);
    TA_Shutdown();
    return (cg_failed || too_noisy) ? 1 : 0;
}
