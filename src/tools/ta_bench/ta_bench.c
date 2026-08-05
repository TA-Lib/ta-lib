/* ta_bench — Generic performance benchmark for ALL TA-Lib indicators.
 *
 * Uses ta_abstract (TA_ForEachFunc, TA_CallFunc) to iterate all indicators
 * generically. Compares C-ref (direct call) against codegen servers
 * (JSON-RPC with load_data + use_preloaded + server-side iteration).
 *
 * Usage:
 *   ./ta_bench [--points=N] [--iters=N] [--language=c,rust] [--function=RSI,SMA]
 *              [--shape=NAME] [--seed=N] [--regime-period=N] [--trend-strength=F]
 *              [--list-shapes] [--verify-corpus]
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
#include "codegen_pipe.h"
#include "bench_corpus.h"

/* ---- Configuration ---- */

#define MAX_POINTS        200000
#define DEFAULT_POINTS    100000
#define DEFAULT_ITERS     100
#define MAX_FUNCTIONS     200
#define JSON_BUF_SIZE     (32 * 1024 * 1024)

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

/* ---- Test data (corpus shapes live in bench_corpus.h) ---- */

static TA_Real *g_open, *g_high, *g_low, *g_close, *g_volume, *g_oi;
static int g_nPoints;

static void generate_price_data(int n, const BenchCorpusCfg *corpus) {
    g_nPoints = n;
    g_open   = calloc(n, sizeof(TA_Real));
    g_high   = calloc(n, sizeof(TA_Real));
    g_low    = calloc(n, sizeof(TA_Real));
    g_close  = calloc(n, sizeof(TA_Real));
    g_volume = calloc(n, sizeof(TA_Real));
    g_oi     = calloc(n, sizeof(TA_Real));
    bench_corpus_gen(corpus, n, g_open, g_high, g_low, g_close, g_volume, g_oi, NULL);
}

/* ---- JSON helpers ---- */

static int json_write_double_array(char *buf, int sz, int pos, const TA_Real *arr, int n) {
    pos = codegen_appendc(buf, sz, pos, '[');
    for( int i = 0; i < n; i++ ) {
        if( i > 0 ) pos = codegen_appendc(buf, sz, pos, ',');
        pos = codegen_appendf(buf, sz, pos, "%.10g", arr[i]);
    }
    return codegen_appendc(buf, sz, pos, ']');
}

static const char *json_find_field(const char *json, const char *field, int *len) {
    char pat[128]; snprintf(pat, sizeof(pat), "\"%s\":", field);
    const char *p = strstr(json, pat);
    if( !p ) return NULL;
    p += strlen(pat); while( *p == ' ' ) p++;
    const char *end = p;
    if( *end == '[' ) { int d=1; end++; while(d>0&&*end){if(*end=='[')d++;if(*end==']')d--;end++;} }
    else { while(*end&&*end!=','&&*end!='}')end++; }
    *len = (int)(end - p); return p;
}

/* ---- Language servers ---- */

typedef struct {
    const char *name;
    const char *display;
    const char *const *argv;
    CodegenPipe cp;
    int active;
    int optional;   /* 1 = only run when explicitly named in --language
                       (e.g. an opt-in third-party comparison server) */
} BenchLanguage;

static const char *const argv_cref[]   = {"./ta_ref_serve", NULL};
static const char *const argv_c[]      = {"./ta_codegen_serve_c", NULL};
static const char *const argv_rust[]   = {"./ta_codegen_serve_rust", NULL};
static const char *const argv_java[]   = {"java", "-cp", "ta_codegen_java", "TaCodegenServe", NULL};
static const char *const argv_csharp[] = {"dotnet", "ta_codegen_csharp/TaCodegenServe.dll", NULL};

static BenchLanguage LANGUAGES[] = {
    {"cref",     "C-ref",    argv_cref,     {0}, 0, 0},
    {"c",        "C",        argv_c,        {0}, 0, 0},
    {"rust",     "Rust",     argv_rust,     {0}, 0, 0},
    {"java",     "Java",     argv_java,     {0}, 0, 0},
    {"csharp",   "C#",       argv_csharp,   {0}, 0, 0},
};
#define NUM_LANGUAGES (sizeof(LANGUAGES)/sizeof(LANGUAGES[0]))

/* ---- Send load_data ---- */

static int send_load_data(BenchLanguage *lang, char *buf, int sz, char *resp, int rsz) {
    int pos = codegen_appendf(buf, sz, 0, "{\"method\":\"load_data\",\"params\":{\"open\":");
    pos = json_write_double_array(buf, sz, pos, g_open, g_nPoints);
    pos = codegen_appendf(buf, sz, pos, ",\"high\":");
    pos = json_write_double_array(buf, sz, pos, g_high, g_nPoints);
    pos = codegen_appendf(buf, sz, pos, ",\"low\":");
    pos = json_write_double_array(buf, sz, pos, g_low, g_nPoints);
    pos = codegen_appendf(buf, sz, pos, ",\"close\":");
    pos = json_write_double_array(buf, sz, pos, g_close, g_nPoints);
    pos = codegen_appendf(buf, sz, pos, ",\"volume\":");
    pos = json_write_double_array(buf, sz, pos, g_volume, g_nPoints);
    pos = codegen_appendf(buf, sz, pos, ",\"openInterest\":");
    pos = json_write_double_array(buf, sz, pos, g_oi, g_nPoints);
    pos = codegen_appendf(buf, sz, pos, "}}");
    if( codegen_pipe_call(&lang->cp, buf, resp, rsz) != TA_TEST_PASS ) return -1;
    return (strstr(resp, "\"ok\"") != NULL) ? 0 : -1;
}

/* ---- Build server request (use_preloaded, no inline data) ---- */

/* When >0, overrides any integer optInTimePeriod param (diagnostic period sweep). */
static int g_period_override = 0;

static int build_bench_request(char *buf, int sz, const TA_FuncInfo *fi,
                                int startIdx, int endIdx, int iters) {
    int pos = codegen_appendf(buf, sz, 0,
        /* no_output: only timing_ns is read here, and serialising the output
           arrays costs far more than the call being timed. */
        "{\"method\":\"TA_%s\",\"params\":{\"startIdx\":%d,\"endIdx\":%d,"
        "\"use_preloaded\":1,\"no_output\":1,\"iters\":%d",
        fi->name, startIdx, endIdx, iters);

    /* Add optional params with default values */
    for( unsigned int i = 0; i < fi->nbOptInput; i++ ) {
        const TA_OptInputParameterInfo *optInfo;
        TA_GetOptInputParameterInfo(fi->handle, i, &optInfo);
        if( optInfo->type == TA_OptInput_RealRange ) {
            pos = codegen_appendf(buf, sz, pos, ",\"%s\":%.15g",
                            optInfo->paramName, optInfo->defaultValue);
        } else {
            int val = (int)optInfo->defaultValue;
            if( g_period_override > 0 && strcmp(optInfo->paramName, "optInTimePeriod") == 0 )
                val = g_period_override;
            pos = codegen_appendf(buf, sz, pos, ",\"%s\":%d",
                            optInfo->paramName, val);
        }
    }
    pos = codegen_appendf(buf, sz, pos, "}}");
    return pos;
}

/* ---- Thermal canary ---- */

/* Run SMA on the C-ref server as a thermal probe.
 * Returns the timing in ns, or 0 on error. */
static long long g_canary_baseline = 0;
/* endIdx is filled from the run's --points: the literal 99999 silently probed a
 * different workload than the one being measured (and read past a smaller run). */
static char g_canary_req[256];

static long long run_canary(char *respBuf, int respSz) {
    /* Find the C-ref server (index 0) */
    if( !LANGUAGES[0].active ) return 0;
    if( codegen_pipe_call(&LANGUAGES[0].cp, g_canary_req, respBuf, respSz) != TA_TEST_PASS )
        return 0;
    int len;
    const char *t = json_find_field(respBuf, "timing_ns", &len);
    return t ? strtoll(t, NULL, 10) : 0;
}

static void thermal_wait(char *respBuf, int respSz) {
    if( g_canary_baseline <= 0 ) return;
    long long threshold = (long long)(g_canary_baseline * 1.05);
    for( int attempt = 0; attempt < 20; attempt++ ) {
        long long t = run_canary(respBuf, respSz);
        if( t > 0 && t <= threshold ) return;
        /* Still hot — the canary run itself is a gentle cooldown */
    }
}

/* Spread of the cref arm across BENCH_PASSES, accumulated over all rows. The
 * ratio columns below are only as meaningful as this is small. */
static double g_spread_sum = 0.0, g_spread_worst = 0.0;
static int    g_spread_n = 0;

/* ---- Per-indicator benchmark callback ---- */

typedef struct {
    const char *functionFilter;
    const char *langFilter;
    char *reqBuf;
    char *respBuf;
    int iters;
    int count;
} BenchContext;

/* Exact-token match, not substring: "csharp" contains "c", so a substring test
 * would silently run the C row for --language=csharp. */
static int lang_matches(const char *filter, const char *name) {
    char filterCopy[1024];
    char *token;
    if( !filter ) return 1;
    strncpy(filterCopy, filter, sizeof(filterCopy) - 1);
    filterCopy[sizeof(filterCopy) - 1] = '\0';
    token = strtok(filterCopy, ",");
    while( token != NULL )
    {
        if( strcmp(name, token) == 0 ) return 1;
        token = strtok(NULL, ",");
    }
    return 0;
}

static int func_matches(const char *filter, const char *name) {
    if( !filter ) return 1;
    /* Comma-separated substring match */
    char buf[512]; strncpy(buf, filter, sizeof(buf)-1); buf[sizeof(buf)-1]='\0';
    for( char *tok = strtok(buf, ","); tok; tok = strtok(NULL, ",") )
        if( strcasestr(name, tok) ) return 1;
    return 0;
}

static void bench_one_function(const TA_FuncInfo *fi, void *opaque) {
    BenchContext *ctx = (BenchContext *)opaque;
    if( !func_matches(ctx->functionFilter, fi->name) ) return;

    /* Wait for thermal equilibrium before each indicator */
    thermal_wait(ctx->respBuf, JSON_BUF_SIZE);

    int startIdx = 0;
    int endIdx = g_nPoints - 1;

    build_bench_request(ctx->reqBuf, JSON_BUF_SIZE, fi, startIdx, endIdx, ctx->iters);

    /* Collect timing from all active servers.
     * Run 3 passes and keep the minimum per server — eliminates icache noise
     * from running all 161 indicators back-to-back in one binary. */
    long long ref_ns = 0;
    long long timings[16] = {0};
    long long t_max[16] = {0};
    int has_timing[16] = {0};


    #define BENCH_PASSES 3
    for( int pass = 0; pass < BENCH_PASSES; pass++ ) {
        if( pass > 0 ) thermal_wait(ctx->respBuf, JSON_BUF_SIZE);
        for( unsigned int li = 0; li < NUM_LANGUAGES; li++ ) {
            if( !LANGUAGES[li].active ) continue;
            if( codegen_pipe_call(&LANGUAGES[li].cp, ctx->reqBuf, ctx->respBuf, JSON_BUF_SIZE) != TA_TEST_PASS )
                continue;
            int len;
            const char *t = json_find_field(ctx->respBuf, "timing_ns", &len);
            if( t ) {
                long long ns = strtoll(t, NULL, 10);
                /* Error responses carry timing_ns 0 — not a measurement.
                 * Without this guard an errored call would show up as a
                 * (green) 0 ns row instead of ERR. */
                if( ns > 0 ) {
                    if( !has_timing[li] || ns < timings[li] )
                        timings[li] = ns;
                    /* Widest and narrowest too: min alone cannot say whether
                       the box was quiet enough for the row to mean anything. */
                    if( !has_timing[li] || ns > t_max[li] ) t_max[li] = ns;
                    has_timing[li] = 1;
                }
            }
        }
    }

    /* Extract ref timing for ratio coloring */
    double ref_spread = -1.0;
    for( unsigned int li = 0; li < NUM_LANGUAGES; li++ ) {
        if( has_timing[li] && strcmp(LANGUAGES[li].name, "cref") == 0 ) {
            ref_ns = timings[li];
            if( ref_ns > 0 )
                ref_spread = (double)(t_max[li] - timings[li]) / (double)ref_ns;
        }
    }
    /* Track the worst row so the footer can say whether the run was quiet. */
    if( ref_spread >= 0.0 ) {
        g_spread_sum += ref_spread;
        g_spread_n++;
        if( ref_spread > g_spread_worst ) g_spread_worst = ref_spread;
    }

    /* Print row */
    printf("%-20s", fi->name);
    for( unsigned int li = 0; li < NUM_LANGUAGES; li++ ) {
        if( !LANGUAGES[li].active ) continue;
        int is_cref = (strcmp(LANGUAGES[li].name, "cref") == 0);
        if( !has_timing[li] ) {
            printf(" %10s", "ERR");
        } else if( is_cref ) {
            printf(" %10lld", timings[li]);
        } else {
            double ratio = (ref_ns > 0) ? (double)timings[li] / (double)ref_ns : 0.0;
            const char *clr = (ratio > 1.10) ? "\033[31m" : (ratio < 0.90) ? "\033[32m" : "";
            const char *rst = (*clr) ? "\033[0m" : "";
            printf(" %s%10lld%s", clr, timings[li], rst);
        }
    }
    printf("\n");
    ctx->count++;
}

/* ---- Main ---- */

int main(int argc, char *argv[]) {
    int n_points = DEFAULT_POINTS;
    int n_iters  = DEFAULT_ITERS;
    const char *lang_filter = NULL;
    const char *func_filter = NULL;
    const char *shape_name = NULL;
    int verify_corpus = 0;
    /* 0 disables the gate; 25% matches ta_bench_direct. */
    double max_spread = 0.25;
    BenchCorpusCfg corpus;
    int seed = BENCH_CORPUS_SEED;
    double trend_strength = BENCH_CORPUS_TREND;
    int regime_period = 0;   /* 0 = derive (see below) */
    int shape;

    for( int i = 1; i < argc; i++ ) {
        if( strncmp(argv[i], "--points=", 9) == 0 )       n_points = atoi(argv[i]+9);
        else if( strncmp(argv[i], "--iters=", 8) == 0 )    n_iters = atoi(argv[i]+8);
        else if( strncmp(argv[i], "--language=", 11) == 0 ) lang_filter = argv[i]+11;
        else if( strncmp(argv[i], "--function=", 11) == 0 ) func_filter = argv[i]+11;
        else if( strncmp(argv[i], "--period=", 9) == 0 )    g_period_override = atoi(argv[i]+9);
        else if( strncmp(argv[i], "--shape=", 8) == 0 )     shape_name = argv[i]+8;
        else if( strncmp(argv[i], "--seed=", 7) == 0 )      seed = atoi(argv[i]+7);
        else if( strncmp(argv[i], "--regime-period=", 16) == 0 ) regime_period = atoi(argv[i]+16);
        else if( strncmp(argv[i], "--trend-strength=", 17) == 0 ) trend_strength = atof(argv[i]+17);
        else if( strcmp(argv[i], "--list-shapes") == 0 )  { bench_shape_list(); return 0; }
        else if( strcmp(argv[i], "--verify-corpus") == 0 ) verify_corpus = 1;
        else if( strncmp(argv[i], "--max-spread=", 13) == 0 ) max_spread = atof(argv[i]+13)/100.0;
        else {
            /* Reject rather than ignore: a mistyped --shape= would otherwise
             * silently benchmark the default class and report it as the one
             * asked for. */
            fprintf(stderr, "ta_bench: unknown option '%s'\n", argv[i]);
            return 2;
        }
    }
    if( n_points > MAX_POINTS ) n_points = MAX_POINTS;

    shape = bench_shape_id(shape_name);
    if( shape < 0 ) {
        printf("ta_bench: unknown --shape=%s\n\n", shape_name);
        bench_shape_list();
        return 1;
    }
    /* The regime length of the trend/chop shapes is relative to the rolling
     * window under test, so when --period pins the window use that; otherwise
     * fall back to the corpus default. */
    if( regime_period <= 0 )
        regime_period = (g_period_override > 0) ? g_period_override : BENCH_CORPUS_PERIOD;

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

    snprintf(g_canary_req, sizeof(g_canary_req),
             "{\"method\":\"TA_SMA\",\"params\":{\"startIdx\":0,\"endIdx\":%d,"
             "\"use_preloaded\":1,\"no_output\":1,\"iters\":50,"
             "\"optInTimePeriod\":30}}", n_points - 1);

    printf("ta_bench: %d points, %d iters, shape=%s seed=%d regime-period=%d"
           " trend-strength=%.2f (server-side)\n\n",
           n_points, n_iters, bench_shape_name(shape), seed, regime_period, trend_strength);

    /* Start servers + load data */
    char *reqBuf  = malloc(JSON_BUF_SIZE);
    char *respBuf = malloc(JSON_BUF_SIZE);

    for( unsigned int li = 0; li < NUM_LANGUAGES; li++ ) {
        if( LANGUAGES[li].optional && !lang_filter ) continue;
        if( !lang_matches(lang_filter, LANGUAGES[li].name) ) continue;
        if( codegen_pipe_open(&LANGUAGES[li].cp, LANGUAGES[li].argv) == TA_TEST_PASS ) {
            LANGUAGES[li].active = 1;
            printf("  Started %s server (pid %d)\n", LANGUAGES[li].display, LANGUAGES[li].cp.child_pid);
        } else {
            printf("  FAILED to start %s server\n", LANGUAGES[li].display);
        }
    }

    printf("  Loading %d points into servers...\n", n_points);
    for( unsigned int li = 0; li < NUM_LANGUAGES; li++ ) {
        if( !LANGUAGES[li].active ) continue;
        if( send_load_data(&LANGUAGES[li], reqBuf, JSON_BUF_SIZE, respBuf, JSON_BUF_SIZE) != 0 ) {
            printf("    %s: load_data FAILED\n", LANGUAGES[li].display);
            LANGUAGES[li].active = 0;
        } else {
            printf("    %s: ready\n", LANGUAGES[li].display);
        }
    }
    printf("\n");

    /* Establish thermal canary baseline (run SMA several times, take minimum) */
    {
        long long best = 0;
        for( int w = 0; w < 5; w++ ) {
            long long t = run_canary(respBuf, JSON_BUF_SIZE);
            if( t > 0 && (best == 0 || t < best) ) best = t;
        }
        g_canary_baseline = best;
        if( best > 0 )
            printf("  Thermal canary (SMA): %lld ns baseline\n\n", best);
    }

    /* Header */
    printf("%-20s", "Function");
    for( unsigned int li = 0; li < NUM_LANGUAGES; li++ ) {
        if( !LANGUAGES[li].active ) continue;
        printf(" %10s", LANGUAGES[li].display);
    }
    printf("\n");
    printf("%-20s", "--------");
    for( unsigned int li = 0; li < NUM_LANGUAGES; li++ ) {
        if( !LANGUAGES[li].active ) continue;
        printf(" %10s", "------");
    }
    printf("\n");

    /* Run all indicators */
    BenchContext ctx = {
        .functionFilter = func_filter,
        .langFilter     = lang_filter,
        .reqBuf         = reqBuf,
        .respBuf        = respBuf,
        .iters          = n_iters,
        .count          = 0,
    };
    TA_ForEachFunc(bench_one_function, &ctx);

    printf("\n%d indicators benchmarked (%d points, %d iters, shape=%s)\n",
           ctx.count, n_points, n_iters, bench_shape_name(shape));
    printf("(red >10%% slower, green >10%% faster than C-ref)\n");

    /* Say how quiet the box was. Without this the ratios above look equally
       authoritative whether the spread was 2% or 200%. */
    int too_noisy = 0;
    if( g_spread_n > 0 ) {
        double mean = g_spread_sum / (double)g_spread_n;
        printf("C-ref spread over %d passes: mean %.0f%%, worst %.0f%% (%d rows).\n",
               BENCH_PASSES, mean * 100.0, g_spread_worst * 100.0, g_spread_n);
        if( max_spread > 0.0 && mean > max_spread ) {
            fprintf(stderr,
                    "ta_bench: mean C-ref spread %.0f%% exceeds --max-spread=%.0f%% — "
                    "treat the ratios above as unresolved.\n",
                    mean * 100.0, max_spread * 100.0);
            too_noisy = 1;
        }
    } else if( !LANGUAGES[0].active ) {
        printf("No C-ref column: the ratio colours above are uncalibrated "
               "(add cref to --language).\n");
    }

    /* Cleanup */
    for( unsigned int li = 0; li < NUM_LANGUAGES; li++ )
        if( LANGUAGES[li].active )
            codegen_pipe_close(&LANGUAGES[li].cp);
    free(reqBuf); free(respBuf);
    free(g_open); free(g_high); free(g_low); free(g_close); free(g_volume); free(g_oi);
    TA_Shutdown();
    return too_noisy ? 1 : 0;
}
