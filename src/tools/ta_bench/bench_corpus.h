#ifndef BENCH_CORPUS_H
#define BENCH_CORPUS_H

/* bench_corpus.h — the benchmark input corpus: the synthetic price series the
 * ta_bench binaries measure on.
 *
 * One shared header so ta_bench, ta_bench_direct and the two generated benches
 * (ta_bench_cg, ta_bench_stream) measure the SAME series; before this each
 * carried its own copy of the same seed-42 walk.
 *
 * Every series is a pure function of its BenchCorpusCfg and n — no time(), no
 * rand(), no locale, no platform state — so the same selection reproduces byte
 * for byte on re-run (`--verify-corpus` checks exactly that). Unlike
 * fuzz_data.h these arrays are never hashed or compared against an oracle, only
 * timed, so no cross-toolchain bit-identity is required and the
 * -ffp-contract=off constraint of fuzz_data.h (#150) does not apply here.
 *
 * There is a WEAKER property one caller does depend on, though. ta_bench_direct
 * prints a ratio between its own in-process run (CMake, -O3, no LTO) and a
 * ta_bench_cg subprocess (gcc -O3 -flto, single TU) — two independently
 * compiled copies of this header. That ratio is only meaningful if both halves
 * generate the SAME workload, so keep the generators here free of anything a
 * build flag can reorder: no reassociable reductions, no fast-math-sensitive
 * idioms. The current shapes satisfy this; the walk family is immune outright
 * (its scale factors are exact powers of two), and the rest are strict
 * sequential recurrences.
 *
 * WHY MORE THAN ONE SHAPE — the rolling min/max shared by MIN, MAX, MINMAX,
 * MIDPOINT, MIDPRICE, WILLR, STOCH and STOCHF caches the window extremum with
 * its index and rescans the whole window whenever that extremum falls out of
 * it, so its cost is input-dependent. What a corpus has to vary is how often
 * that rescan fires, i.e. how often the window extremum is its OLDEST bar:
 *
 *   - on a driftless walk the rate decays as ~1/sqrt(period), because the
 *     extremum of a long driftless window is seldom at its start: measured
 *     33%/23%/9% of bars (either extremum) at period 14/30/200;
 *   - on a trending leg the rate is set by the per-bar drift/noise ratio and is
 *     nearly independent of the period: 36%/30%/27% for trend-chop-1p at the
 *     default --trend-strength (~43% of the bars inside a trend leg itself).
 *
 * So the two classes separate further the longer the window — 1.1x the rescan
 * rate at period 14, 3x at period 200 — and a lone zero-drift walk cannot see
 * it. Note the trend case is a MAJORITY-quiet one, not a pathological one: it
 * is the minority of bars that rescan, just far more of them than on the walk.
 * Within an uptrend it is the low that is pinned near the oldest bar while the
 * high refreshes almost every bar, so the dual-extremum functions degrade on
 * either trend direction and the single-extremum ones on one direction each.
 * Real markets alternate trending and range-bound legs, which is why
 * `trend-chop-*` is the input class this corpus was missing (issue #147).
 *
 * THE TAIL SHAPES ARE NOT PEERS — `constant` is the worst case in the family,
 * at 2*(period-1) inner comparisons per bar, exactly TWICE `mono-up`/`mono-down`
 * (period-1). The reason is a tie-break, not the trend mechanism above: the
 * rescan loop compares with strict `>` / `<`, so on equal values it leaves
 * highestIdx/lowestIdx parked on trailingIdx, the `< trailingIdx` guard refires
 * on the very next bar, and the `>= highest` / `<= lowest` fast-path arms — the
 * ones that would otherwise move the cached index onto the newest bar and end
 * the rescans — never execute at all. Flat input does this to BOTH extrema
 * indefinitely; a monotone ramp pins only one of the two (in an up-ramp the high
 * is always the newest bar), which is precisely why it costs half as much.
 *
 * `randwalk` stays the default and reproduces the pre-existing seed-42 series
 * bit for bit, so numbers taken before this corpus existed remain comparable.
 * Shapes are opt-in via --shape=NAME: a benchmark run costs the same as before
 * unless you ask for another class.
 */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Historical benchmark seed. Kept as the default so --shape=randwalk with no
 * --seed reproduces the series every earlier measurement used. */
#define BENCH_CORPUS_SEED 42

/* Default reference period for the regime-length of the trend/chop shapes:
 * the default optInTimePeriod of the dual-extremum functions in #147
 * (MIDPOINT, MIDPRICE, WILLR). See the regime-ratio note below. */
#define BENCH_CORPUS_PERIOD 14

/* Default trend-leg drift, in units of the per-bar standard deviation.
 *
 * This is the corpus knob that decides how trending a trend leg is, so it is
 * worth being explicit about. At 0.5 a bar's expected move is half its noise:
 * 31% of the bars in an "uptrend" still close lower, and at the 1%/bar
 * volatility these legs use (~16%/yr) a 63-bar leg gains ~37% — an ordinary
 * strong leg in a real name, not a monotone series in disguise. It also
 * matters mechanically: the window low sits on the
 * oldest bar exactly when the leg has not traded back below where the window
 * opened, and that probability is set by this ratio — measured inside an
 * uptrend leg, 25% at 0.25, 43% at 0.5, 64% at 1.0 — NOT by the period, and
 * NOT by how long the leg runs. Sweep it with --trend-strength to
 * see how the rolling min/max cost responds; 0 degenerates to a chop-only
 * series and large values approach the monotone tail shapes. */
#define BENCH_CORPUS_TREND 0.5

/* Input classes. Keep BENCH_NSHAPES last; the names below are the --shape=
 * tokens, lowercase-hyphen as in fuzz_data.h's shape list. */
enum {
    BENCH_RANDWALK = 0,     /* seed-42 additive walk around 100 — the historical
                               default; zero drift, so the rescan rate decays as
                               ~1/sqrt(period) and is lowest where it matters most */
    BENCH_RANDWALK_LO,      /* same walk, 1/4 the step size                      */
    BENCH_RANDWALK_HI,      /* same walk, 4x the step size                       */
    BENCH_GBM,              /* geometric Brownian motion, 20%/yr vol on daily
                               bars — the realistic-input acceptance gate        */
    BENCH_TREND_CHOP_HALF,  /* alternating trend/chop, regime = period/2         */
    BENCH_TREND_CHOP_1,     /* alternating trend/chop, regime = period           */
    BENCH_TREND_CHOP_2,     /* alternating trend/chop, regime = 2 x period       */
    BENCH_TREND_CHOP_4,     /* alternating trend/chop, regime = 4 x period       */
    BENCH_MONO_UP,          /* strictly increasing — degenerate tail case        */
    BENCH_MONO_DOWN,        /* strictly decreasing — degenerate tail case        */
    BENCH_CONSTANT,         /* flat O=H=L=C — plateau, pins BOTH extrema         */
    BENCH_NSHAPES
};

/* Generator family behind each shape. */
enum {
    BENCH_GEN_WALK = 0,     /* legacy LCG additive walk, param = step scale      */
    BENCH_GEN_GBM,          /* lognormal, param = annualized volatility          */
    BENCH_GEN_TRENDCHOP,    /* alternating regimes, param = regime/period ratio  */
    BENCH_GEN_RAMP,         /* linear, param = +1 up / -1 down                   */
    BENCH_GEN_FLAT          /* constant                                          */
};

/* What a shape is FOR. The rolling-extremum rescan rate depends only on the
 * RANK ORDER of the bars, and three shapes cannot move it — measured within 1%
 * of randwalk at period 14/30/200 — but for TWO DIFFERENT reasons, which is
 * worth keeping straight:
 *
 *   - randwalk-lo / randwalk-hi are the SAME path rescaled. They share the LCG
 *     stream and their scale factors are exact powers of two, so the bar
 *     ordering is identical by construction (measured: 7 and 18 close-direction
 *     differences in 99999, all downstream of the 1.0 price floor).
 *   - gbm is an INDEPENDENT path — a different generator entirely, disagreeing
 *     with randwalk on the direction of 50172 of 99999 bars, i.e. a coin flip.
 *     It matches on the rescan rate because it is also driftless, not because
 *     it retraces the same walk.
 *
 * Either way they are controls, not stressors, and the list says so rather than
 * implying otherwise with a "low/high volatility" label. They still earn their
 * place: they vary magnitude and (for gbm) additive-vs-multiplicative scaling,
 * which is what a numerical-conditioning question needs — deadbands,
 * cancellation, ratio-based indicators — just not this one. */
enum {
    BENCH_GRP_CONTROL = 0,  /* rank-order preserving: same rescan rate as randwalk */
    BENCH_GRP_STRESS,       /* varies how often the window extremum is its oldest bar */
    BENCH_GRP_TAIL          /* analytic degenerate cases, not the target class     */
};

typedef struct {
    const char *name;       /* --shape= token                                   */
    int         gen;        /* BENCH_GEN_*                                      */
    double      param;      /* meaning depends on gen (see above)               */
    int         group;      /* BENCH_GRP_*                                      */
    const char *note;       /* one line for --list-shapes                       */
} BenchShape;

/* Regime ratios: the trend/chop regime length N is set relative to the rolling
 * window P (--regime-period, default 14) because the degradation is a function
 * of N/P, not of N. N < P means every window straddles a regime boundary, so
 * the pinned extremum is repeatedly displaced by the chop and the cache still
 * helps; N >= P means a window sits inside one leg for most of its life and the
 * rescan rate saturates — measured at P=200, 0.5p rescans on 17% of bars while
 * 1p/2p/4p all land within a point of 27%. 0.5/1/2/4 brackets that transition,
 * and shows where it flattens. With the default P=14 the regimes are 7/14/28/56,
 * which against MIN/MAX/MINMAX's own default window of 30 are 0.23/0.47/0.93/
 * 1.87 x — so one sweep covers roughly 0.25p..4p across both default periods in
 * the family. */
static const BenchShape BENCH_SHAPES[BENCH_NSHAPES] = {
    { "randwalk",         BENCH_GEN_WALK,      1.00, BENCH_GRP_CONTROL,
      "zero-drift additive walk, seed 42 (historical default; the control)" },
    { "randwalk-lo",      BENCH_GEN_WALK,      0.25, BENCH_GRP_CONTROL,
      "randwalk at 1/4 step: same rank order, smaller magnitudes" },
    { "randwalk-hi",      BENCH_GEN_WALK,      4.00, BENCH_GRP_CONTROL,
      "randwalk at 4x step: same rank order, larger magnitudes" },
    { "gbm",              BENCH_GEN_GBM,       0.20, BENCH_GRP_CONTROL,
      "multiplicative walk, 20%/yr: moves scale with the price level" },
    { "trend-chop-0.5p",  BENCH_GEN_TRENDCHOP, 0.50, BENCH_GRP_STRESS,
      "alternating trend/chop legs, regime = period/2" },
    { "trend-chop-1p",    BENCH_GEN_TRENDCHOP, 1.00, BENCH_GRP_STRESS,
      "alternating trend/chop legs, regime = period" },
    { "trend-chop-2p",    BENCH_GEN_TRENDCHOP, 2.00, BENCH_GRP_STRESS,
      "alternating trend/chop legs, regime = 2x period" },
    { "trend-chop-4p",    BENCH_GEN_TRENDCHOP, 4.00, BENCH_GRP_STRESS,
      "alternating trend/chop legs, regime = 4x period" },
    { "mono-up",          BENCH_GEN_RAMP,      1.00, BENCH_GRP_TAIL,
      "strictly increasing ramp: pins the low only, period-1 per bar" },
    { "mono-down",        BENCH_GEN_RAMP,     -1.00, BENCH_GRP_TAIL,
      "strictly decreasing ramp: pins the high only, period-1 per bar" },
    { "constant",         BENCH_GEN_FLAT,      0.00, BENCH_GRP_TAIL,
      "flat O=H=L=C: WORST case, pins both, 2*(period-1) per bar" }
};

/* ---- PRNGs ---------------------------------------------------------------
 * Two, deliberately. The `randwalk*` family keeps the original glibc-style LCG
 * so `randwalk` stays bit-identical to the pre-corpus series (and so the low/hi
 * variants are the same process at a different step scale, not a different
 * process). Everything added here uses splitmix64, the same generator
 * fuzz_data.h uses. */

static unsigned long long bench_sm_next(unsigned long long *s)
{
    unsigned long long z = (*s += 0x9E3779B97F4A7C15ULL);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
}

/* Uniform in [0,1) from the top 53 bits. */
static double bench_sm_unit(unsigned long long *s)
{
    return (double)(bench_sm_next(s) >> 11) * (1.0 / 9007199254740992.0);
}

/* Standard normal, Box-Muller. Cached second deviate, so the stream advances
 * two uniforms per pair and stays reproducible for a given (shape,seed,n). */
typedef struct {
    unsigned long long s;
    double spare;
    int    hasSpare;
} BenchRng;

/* The stream depends on the seed ONLY — deliberately not on the shape.
 * Mixing the table index in would make a --shape sweep vary the regime length
 * and the random path at the same time, so trend-chop-0.5p..4p would not be a
 * controlled comparison. It would also tie every series to its position in
 * BENCH_SHAPES, so inserting or removing a shape would silently change the
 * numbers of every other one. */
static void bench_rng_init(BenchRng *r, int seed)
{
    r->s = 0x243F6A8885A308D3ULL
         ^ ((unsigned long long)(unsigned int)seed * 0xD1B54A32D192ED03ULL);
    r->spare = 0.0;
    r->hasSpare = 0;
}

static double bench_rng_normal(BenchRng *r)
{
    double u1, u2, mag;
    if( r->hasSpare ) { r->hasSpare = 0; return r->spare; }
    /* Guard the log against an exact 0 from the uniform. */
    do { u1 = bench_sm_unit(&r->s); } while( u1 <= 1.0e-300 );
    u2 = bench_sm_unit(&r->s);
    mag = sqrt(-2.0 * log(u1));
    r->spare = mag * sin(6.283185307179586476925286766559 * u2);
    r->hasSpare = 1;
    return mag * cos(6.283185307179586476925286766559 * u2);
}

/* ---- Corpus selection --------------------------------------------------- */

typedef struct {
    int    shape;          /* BENCH_* input class                              */
    int    seed;           /* PRNG stream                                      */
    int    refPeriod;      /* rolling window the regime length is relative to  */
    double trendStrength;  /* trend-leg drift, in per-bar standard deviations  */
} BenchCorpusCfg;

static void bench_corpus_defaults(BenchCorpusCfg *cfg)
{
    cfg->shape         = BENCH_RANDWALK;
    cfg->seed          = BENCH_CORPUS_SEED;
    cfg->refPeriod     = BENCH_CORPUS_PERIOD;
    cfg->trendStrength = BENCH_CORPUS_TREND;
}

/* ---- Shape lookup ------------------------------------------------------- */

/* Shape id for a --shape= token, or -1 when unknown. */
static int bench_shape_id(const char *name)
{
    int i;
    if( !name || !*name ) return BENCH_RANDWALK;
    for( i = 0; i < BENCH_NSHAPES; i++ )
        if( strcmp(BENCH_SHAPES[i].name, name) == 0 ) return i;
    return -1;
}

static const char *bench_shape_name(int shape)
{
    if( shape < 0 || shape >= BENCH_NSHAPES ) return "?";
    return BENCH_SHAPES[shape].name;
}

/* One line per input class, grouped by what the class is for, for
 * --list-shapes. The grouping is the point: it stops a reader assuming the
 * rank-order controls stress the rescan rate. */
static void bench_shape_list(void)
{
    static const char *const GROUP_HDR[] = {
        "Controls -- do NOT move the rolling-extremum rescan rate (within 1% of randwalk).\n"
        "randwalk-lo/-hi are the same path rescaled; gbm is an independent driftless path.\n"
        "Useful for magnitude/conditioning questions, not for the rescan rate.",
        "Rescan-rate stressors -- vary how often the window extremum is its oldest bar.",
        "Degenerate tail -- analytic worst cases, not the class this corpus exists for."
    };
    int g, i;
    printf("Benchmark input corpus (--shape=NAME, default %s):\n",
           BENCH_SHAPES[BENCH_RANDWALK].name);
    for( g = BENCH_GRP_CONTROL; g <= BENCH_GRP_TAIL; g++ )
    {
        printf("\n%s\n", GROUP_HDR[g]);
        for( i = 0; i < BENCH_NSHAPES; i++ )
            if( BENCH_SHAPES[i].group == g )
                printf("  %-16s %s\n", BENCH_SHAPES[i].name, BENCH_SHAPES[i].note);
    }
    printf("\nTrend/chop regime length is --regime-period (default %d) times the ratio\n"
           "in the shape name; --trend-strength (default %.2f) is the trend-leg drift in\n"
           "per-bar standard deviations; --seed (default %d) picks the stream.\n",
           BENCH_CORPUS_PERIOD, BENCH_CORPUS_TREND, BENCH_CORPUS_SEED);
}

/* ---- Corpus generator ---------------------------------------------------
 * Fills OHLCV+OI (length n) for one input class. Invariants held by every
 * shape: all values finite, high >= max(open,close), low <= min(open,close),
 * low > 0. The output is a pure function of (cfg, n).
 *
 * periods may be NULL — when given it receives MAVP's per-bar period series in
 * its default [2..30] range.
 */
static void bench_corpus_gen(const BenchCorpusCfg *cfg, int n,
                             double *o, double *h, double *l, double *c,
                             double *v, double *oi, double *periods)
{
    const BenchShape *sh;
    BenchRng rng;
    double param;
    int i, period = 16;
    int shape = cfg->shape;
    int seed = cfg->seed;
    int refPeriod = cfg->refPeriod;

    if( shape < 0 || shape >= BENCH_NSHAPES ) shape = BENCH_RANDWALK;
    if( refPeriod < 2 ) refPeriod = BENCH_CORPUS_PERIOD;
    sh = &BENCH_SHAPES[shape];
    param = sh->param;
    bench_rng_init(&rng, seed);

    if( sh->gen == BENCH_GEN_WALK )
    {
        /* The pre-corpus generator, with the step size factored out. At
         * param 1.0 amp/hw are exactly 2.0/0.5 and every bar is bit-identical
         * to the series ta_bench measured before this header existed. */
        unsigned int lcg = (unsigned int)seed;
        double price = 100.0;
        double amp = 2.0 * param, hw = 0.5 * param;
        for( i = 0; i < n; i++ ) {
            double r, oo, cc;
            lcg = lcg * 1103515245 + 12345;
            r = ((double)(lcg >> 16) / 32768.0) - 1.0;
            oo = price; cc = price + r * amp;
            o[i] = oo; c[i] = cc;
            h[i] = fmax(oo, cc) + fabs(r) * hw;
            l[i] = fmin(oo, cc) - fabs(r) * hw;
            if( l[i] < 1.0 ) l[i] = 1.0;
            v[i] = 1000000.0 + r * 500000.0;
            oi[i] = 0.0;
            price = cc; if( price < 1.0 ) price = 1.0;
            if( periods ) {
                /* Wandering period series in MAVP's default [2..30] range:
                 * exercises the multi-period grouping, not just the
                 * single-period fast path. */
                period += (int)((lcg >> 8) % 7) - 3;
                if( period < 2 ) period = 2;
                if( period > 30 ) period = 30;
                periods[i] = (double)period;
            }
        }
        return;
    }

    if( sh->gen == BENCH_GEN_FLAT )
    {
        for( i = 0; i < n; i++ ) {
            o[i] = h[i] = l[i] = c[i] = 100.0;
            v[i] = 1000000.0;
            oi[i] = 0.0;
            if( periods ) periods[i] = 14.0;
        }
        return;
    }

    if( sh->gen == BENCH_GEN_RAMP )
    {
        /* 100..1000 (or the reverse) over the whole series, so the ramp is
         * strictly monotone in open/high/low/close at any n without running
         * into non-positive prices. */
        double span = (n > 1) ? 900.0 / (double)(n - 1) : 0.0;
        for( i = 0; i < n; i++ ) {
            double t = (param > 0.0) ? (double)i : (double)(n - 1 - i);
            double cc = 100.0 + span * t;
            double oo = cc - 0.5 * span * ((param > 0.0) ? 1.0 : -1.0);
            o[i] = oo; c[i] = cc;
            h[i] = fmax(oo, cc) + 0.25 * span + 1.0e-6;
            l[i] = fmin(oo, cc) - 0.25 * span - 1.0e-6;
            if( l[i] < 1.0 ) l[i] = 1.0;
            v[i] = 1000000.0;
            oi[i] = 0.0;
            if( periods ) periods[i] = 14.0;
        }
        return;
    }

    /* ---- Stochastic shapes: build a close series, then an intrabar envelope. */
    {
        /* Per-bar log-return sd. GBM: annualized vol over 252 daily bars.
         * Trend/chop: 1%/bar, ordinary daily equity volatility. */
        double sigma = (sh->gen == BENCH_GEN_GBM)
                     ? param / sqrt(252.0)
                     : 0.01;
        /* Ito-corrected 5%/yr drift for GBM; the trend/chop legs carry their
         * own drift below. */
        double mu = (sh->gen == BENCH_GEN_GBM)
                  ? 0.05 / 252.0 - 0.5 * sigma * sigma
                  : 0.0;
        /* Trend legs: drift cfg->trendStrength standard deviations per bar (see
         * BENCH_CORPUS_TREND for why the default is 0.5 and what it controls).
         * Chop legs: no drift, pulled back toward the level the leg opened at
         * (AR(1), phi = 0.10) so they are genuinely range-bound. */
        double trendDrift = cfg->trendStrength * sigma;
        double phi = 0.10;
        /* Regime length in bars, and the 4-leg cycle up/chop/down/chop. The
         * trend direction alternates so the level stays bounded and so the
         * single-extremum functions see both their favourable and their
         * degrading direction within one series. */
        /* Clamped before the cast: (int) of an out-of-range double is UB, and
         * an overflowing --regime-period used to wrap to INT_MIN and then clamp
         * UP to a 2-bar regime — the exact inverse of what was asked for. */
        double regimeF = (sh->gen == BENCH_GEN_TRENDCHOP)
                       ? param * (double)refPeriod + 0.5
                       : 0.0;
        int regime;
        if( regimeF > (double)(n > 2 ? n : 2) ) regimeF = (double)(n > 2 ? n : 2);
        if( regimeF < 0.0 ) regimeF = 0.0;
        regime = (int)regimeF;
        double logp = log(100.0), anchor = logp;
        int leg = 0, legBar = 0;

        if( sh->gen == BENCH_GEN_TRENDCHOP && regime < 2 ) regime = 2;

        for( i = 0; i < n; i++ )
        {
            double z = bench_rng_normal(&rng);
            double cc, oo, hi, lo, w;

            if( sh->gen == BENCH_GEN_TRENDCHOP )
            {
                if( legBar >= regime ) { legBar = 0; leg = (leg + 1) & 3; anchor = logp; }
                switch( leg ) {
                case 0: logp += trendDrift + sigma * z; break;              /* up   */
                case 2: logp += -trendDrift + sigma * z; break;             /* down */
                default: logp += -phi * (logp - anchor) + sigma * z; break; /* chop */
                }
                legBar++;
            }
            else
            {
                logp += mu + sigma * z;
            }

            cc = exp(logp);
            /* Intrabar: open a fraction of a bar's move away from the close,
             * high/low a fraction of one sd outside the body. */
            oo = cc * (1.0 + sigma * 0.5 * bench_sm_unit(&rng.s) - sigma * 0.25);
            hi = fmax(oo, cc); lo = fmin(oo, cc);
            w  = sigma * 0.5 * bench_sm_unit(&rng.s);
            hi = hi * (1.0 + w);
            w  = sigma * 0.5 * bench_sm_unit(&rng.s);
            lo = lo * (1.0 - w);
            o[i] = oo; c[i] = cc; h[i] = hi; l[i] = lo;
            if( l[i] < 1.0e-6 ) l[i] = 1.0e-6;
            v[i] = 1000000.0 + 500000.0 * (bench_sm_unit(&rng.s) - 0.5);
            oi[i] = 0.0;
            if( periods ) {
                period += (int)(bench_sm_next(&rng.s) % 7) - 3;
                if( period < 2 ) period = 2;
                if( period > 30 ) period = 30;
                periods[i] = (double)period;
            }
        }
    }
}

/* ---- Self-check (--verify-corpus) ---------------------------------------
 * Two properties, for every shape:
 *   1. reproducible — two generations of the same (shape, seed, n) are
 *      byte-identical, so a measurement can be repeated exactly;
 *   2. valid OHLC   — all values finite, high >= low, high >= max(open,close),
 *      low > 0, low <= min(open,close), and the MAVP period series in [2,30].
 *
 * ONE DOCUMENTED EXEMPTION. `low <= min(open,close)` does NOT hold for the
 * BENCH_GEN_WALK family. The pre-corpus generator floors `low` at 1.0 but
 * leaves `close` unclamped, so a bar whose body trades below 1.0 ends up with
 * low > close — 32 bars of randwalk and 129 of randwalk-hi at n=100000, 11 and
 * 99 of them with a negative close. That is inherited behaviour, and clamping
 * `close` to repair it would break the byte-for-byte reproduction of the
 * historical seed-42 series, which is worth more than a tidy invariant on a
 * timing-only corpus. So the check exempts the walk family from that one
 * predicate BY GENERATOR, not by name, and enforces it on everything else —
 * a new shape still cannot regress. The other five predicates hold everywhere,
 * walk family included (verified at n up to 200000).
 *
 * Callers pass the n they will actually benchmark at: the walk violations only
 * begin around n=12000, so a check pinned at a small n cannot see them.
 * Returns the number of failing shapes; prints one line each. */
static int bench_corpus_selfcheck(int n, const BenchCorpusCfg *base)
{
    BenchCorpusCfg cfg = *base;
    double *a, *b;
    int shape, failures = 0;
    const int NARR = 7;

    if( n < 64 ) n = 64;
    a = (double *)malloc(sizeof(double) * (size_t)NARR * (size_t)n);
    b = (double *)malloc(sizeof(double) * (size_t)NARR * (size_t)n);
    if( !a || !b ) { printf("  corpus selfcheck: allocation failed\n"); free(a); free(b); return 1; }

    printf("Corpus self-check (n=%d seed=%d regime-period=%d trend-strength=%.2f)\n",
           n, base->seed, base->refPeriod, base->trendStrength);
    for( shape = 0; shape < BENCH_NSHAPES; shape++ )
    {
        const char *why = NULL;
        int i;
        double *ao = a, *ah = a+n, *al = a+2*n, *ac = a+3*n,
               *av = a+4*n, *ai = a+5*n, *ap = a+6*n;

        memset(a, 0xA5, sizeof(double) * (size_t)NARR * (size_t)n);
        memset(b, 0x5A, sizeof(double) * (size_t)NARR * (size_t)n);
        cfg.shape = shape;
        bench_corpus_gen(&cfg, n, ao, ah, al, ac, av, ai, ap);
        bench_corpus_gen(&cfg, n, b, b+n, b+2*n, b+3*n, b+4*n, b+5*n, b+6*n);

        if( memcmp(a, b, sizeof(double) * (size_t)NARR * (size_t)n) != 0 )
            why = "not reproducible";

        /* See the exemption note above: the walk family floors low at 1.0 but
         * not close, so low can sit above the body. Narrowed to low == 1.0
         * EXACTLY — the floor value — rather than exempting the family
         * wholesale, so the predicate still fires on a walk-family low that is
         * above the body for any OTHER reason. (Checked: all real violations
         * are exactly 1.0; a deliberate fmin/fmax swap in the walk generator is
         * caught by this form and was not caught by the blanket one.) */
        const int bodyExempt = (BENCH_SHAPES[shape].gen == BENCH_GEN_WALK);

        for( i = 0; i < n && !why; i++ )
        {
            double hi = (ao[i] > ac[i]) ? ao[i] : ac[i];
            double lo = (ao[i] < ac[i]) ? ao[i] : ac[i];
            if( !isfinite(ao[i]) || !isfinite(ah[i]) ||
                !isfinite(al[i]) || !isfinite(ac[i]) ||
                !isfinite(av[i]) || !isfinite(ai[i]) || !isfinite(ap[i]) )
                why = "non-finite value";
            else if( ah[i] < al[i] )         why = "high below low";
            else if( ah[i] < hi )            why = "high below body";
            else if( !(al[i] > 0.0) )        why = "non-positive low";
            else if( al[i] > lo && !(bodyExempt && al[i] == 1.0) )
                why = "low above body";
            else if( ap[i] < 2.0 || ap[i] > 30.0 ) why = "period out of [2,30]";
        }

        printf("  %-16s %s\n", BENCH_SHAPES[shape].name, why ? why : "ok");
        if( why ) failures++;
    }
    printf("%s (%d shape%s, %d failure%s)\n",
           failures ? "CORPUS SELF-CHECK FAILED" : "corpus self-check passed",
           BENCH_NSHAPES, (BENCH_NSHAPES == 1) ? "" : "s",
           failures, (failures == 1) ? "" : "s");
    free(a); free(b);
    return failures;
}

#endif /* BENCH_CORPUS_H */
