/* Writes fma test cases with libm's answer for ts_fma_check.py.
 *
 *   golden --families
 *   golden <seed> <worker> <workers> <hard-per-family> <random>
 *
 * Each case is a function of (seed, family, index) alone, so the case set does
 * not depend on how many workers split it. A record is 40 bytes, every field
 * little-endian whatever the host: a, b, c, fma(a,b,c) as IEEE bits, the family
 * id (u32), and flags (u32; bit 0 = the twice-rounded a*b+c differs). Build
 * with -ffp-contract=off, or `a * b + c` below is itself fused. */
#include <float.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
    F_TIES, F_TIES_STICKY, F_CANCEL, F_OVF_FINITE, F_OVF_INF, F_OVF_BOUNDARY,
    F_SUBNORMAL, F_ZERO_SIGNS, F_INF_ZERO, F_INF_C, F_NAN,
    F_RANDOM_BITS, F_RANDOM_REAL, F_COUNT
};
#define FIRST_RANDOM F_RANDOM_BITS

static const char *const FAMILY_NAMES[F_COUNT] = {
    "ties", "ties-sticky", "cancel", "ovf-finite", "ovf-inf", "ovf-boundary",
    "subnormal", "zero-signs", "inf-zero", "inf-c", "nan",
    "random-bits", "random-real",
};

static uint64_t d2u(double x) { uint64_t u; memcpy(&u, &x, 8); return u; }
static double u2d(uint64_t u) { double x; memcpy(&x, &u, 8); return x; }

static uint64_t mix64(uint64_t z) {
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
    return z ^ (z >> 31);
}

typedef struct { uint64_t s; } rng_t;

static uint64_t next64(rng_t *r) { return mix64(r->s += 0x9E3779B97F4A7C15ull); }
static int below(rng_t *r, int n) { return (int)(next64(r) % (uint64_t)n); }
static int range(rng_t *r, int lo, int hi) { return lo + below(r, hi - lo + 1); }
static double sign(rng_t *r) { return (next64(r) & 1) ? -1.0 : 1.0; }
static double unit(rng_t *r) { return (double)(next64(r) >> 11) * 0x1p-53; }

/* A significand in [1, 2) with exactly `bits` significant bits, the last one set. */
static double mant_bits(rng_t *r, int bits) {
    if (bits <= 1) return 1.0;
    uint64_t m = (1ull << (bits - 1)) | (next64(r) & ((1ull << (bits - 1)) - 1)) | 1ull;
    return ldexp((double)m, 1 - bits);
}

static double mant53(rng_t *r) { return mant_bits(r, 53); }

/* A random value whose binade is 2^e. */
static double at_exp(rng_t *r, int e) { return sign(r) * ldexp(mant53(r), e); }

static double step_ulps(double x, int k) {
    for (; k > 0; k--) x = nextafter(x, INFINITY);
    for (; k < 0; k++) x = nextafter(x, -INFINITY);
    return x;
}

static double ulp_of(double x) {
    x = fabs(x);
    return nextafter(x, INFINITY) - x;
}

/* A product exponent: across the normal range half the time, else near 0. */
static int product_exp(rng_t *r, int lo, int hi) {
    return (next64(r) & 1) ? range(r, lo, hi) : range(r, -40, 40);
}

/* a and b with a*b in binade 2^ep, the split between them random. */
static void split_product(rng_t *r, int ep, int na, int nb, double *a, double *b) {
    int lo = ep - 1020 > -1060 ? ep - 1020 : -1060;
    int hi = ep + 1060 < 1020 ? ep + 1060 : 1020;
    int ea = (next64(r) & 3) ? ep / 2 + range(r, -60, 60) : range(r, lo, hi);
    if (ea < lo) ea = lo;
    if (ea > hi) ea = hi;
    *a = sign(r) * ldexp(mant_bits(r, na), ea);
    *b = sign(r) * ldexp(mant_bits(r, nb), ep - ea);
}

static double specials(rng_t *r) {
    static const double v[] = { 0.0, -0.0, 1.0, -1.0, 0x1p-1074, -0x1p-1074, DBL_MIN, -DBL_MIN,
                                DBL_MAX, -DBL_MAX, INFINITY, -INFINITY, 0.5, 3.0, 1e300, 1e-300 };
    int i = below(r, (int)(sizeof v / sizeof v[0]) + 2);
    if (i == (int)(sizeof v / sizeof v[0])) return u2d(next64(r));
    if (i > (int)(sizeof v / sizeof v[0])) return at_exp(r, range(r, -1074, 1023));
    return v[i];
}

static double any_nan(rng_t *r) {
    uint64_t m = next64(r) & 0x000FFFFFFFFFFFFFull;
    if (m == 0) m = 1;
    return u2d((next64(r) & 0x8000000000000000ull) | 0x7FF0000000000000ull | m);
}

static void gen_ties(rng_t *r, double *a, double *b, double *c) {
    /* a*b = odd * 2^s exactly (<= 53 bits); c on the 2^(s+1) grid, so c + a*b
     * sits on a midpoint unless the sum leaves c's binade. */
    int na = range(r, 1, 26), nb = range(r, 1, 27);
    int s = (below(r, 4) == 0) ? -1075 : product_exp(r, -1070, 960);
    int sa = s / 2 + range(r, -30, 30);
    *a = sign(r) * ldexp(mant_bits(r, na), sa + na - 1);
    *b = sign(r) * ldexp(mant_bits(r, nb), s - sa + nb - 1);
    if (s == -1075) *c = sign(r) * ldexp((double)(next64(r) & 0x000FFFFFFFFFFFFFull), -1074);
    else *c = sign(r) * ldexp(mant53(r), s + 53);
}

static void gen_ties_sticky(rng_t *r, double *a, double *b, double *c) {
    int ep = product_exp(r, -960, 960);
    if (below(r, 2)) {
        /* Few-bit operands a few ulps off put the product's error term far
         * below ph's lowest set bit: the shape only round-to-odd gets right. */
        split_product(r, ep, range(r, 1, 3), range(r, 1, 3), a, b);
        *a = step_ulps(*a, range(r, -3, 3));
        *b = step_ulps(*b, range(r, -3, 3));
    } else {
        int na = range(r, 1, 53);
        split_product(r, ep, na, range(r, 54 - na > 1 ? 54 - na : 1, 53), a, b);
    }
    double ph = *a * *b;
    switch (below(r, 3)) {
    case 0: {
        /* c's grid is twice the lowest set bit of ph: c + ph is a midpoint and
         * the product's error term alone decides the rounding. */
        uint64_t m = d2u(ph) & 0x000FFFFFFFFFFFFFull;
        int low = 0;
        if (m) while (!((m >> low) & 1)) low++;
        else low = 52;
        int t = ilogb(ph) - 52 + low + 1;
        *c = sign(r) * ldexp(mant53(r), t + 52 + range(r, 0, 1));
        break;
    }
    case 1: *c = sign(r) * ulp_of(ph) * 0.5; break;
    default: *c = sign(r) * ulp_of(ph) * 0.5 * (1.0 + range(r, -4, 4) * 0x1p-52); break;
    }
}

static void gen_cancel(rng_t *r, double *a, double *b, double *c) {
    int exact = below(r, 4) == 0;
    int na = exact ? range(r, 1, 26) : 53, nb = exact ? range(r, 1, 27) : 53;
    split_product(r, product_exp(r, -1000, 1000), na, nb, a, b);
    double ph = *a * *b;
    *c = step_ulps(-ph, exact ? 0 : range(r, -8, 8));
}

static void gen_ovf_finite(rng_t *r, double *a, double *b, double *c) {
    split_product(r, range(r, 1020, 1024), 53, 53, a, b);
    double ph = *a * *b;
    double s = ph > 0 ? 1.0 : -1.0;
    switch (below(r, 4)) {
    case 0: *c = -s * ldexp(1.0 + unit(r), 1023) * (unit(r) < 0.5 ? 1.0 : 0.999); break;
    case 1: *c = -s * ldexp(mant53(r), range(r, 960, 1023)); break;
    case 2: *c = s * ldexp(mant53(r), range(r, 900, 1010)); break;
    default: *c = s * ldexp(mant53(r), range(r, 1018, 1023)); break; /* c + a*b may overflow */
    }
}

static void gen_ovf_inf(rng_t *r, double *a, double *b, double *c) {
    split_product(r, range(r, 1026, 2000), 53, 53, a, b);
    *c = (below(r, 4) == 0) ? specials(r) : at_exp(r, range(r, -1074, 1023));
    if (isinf(*c) || isnan(*c)) *c = DBL_MAX * sign(r);
}

static void gen_ovf_boundary(rng_t *r, double *a, double *b, double *c) {
    /* c = DBL_MAX - k ulp; the rounding boundary DBL_MAX + ulp/2 is then
     * reached by a*b = (2k+1) * 2^970. Perturb a*b around that. */
    int k = range(r, 0, 7);
    double s = sign(r);
    *c = s * step_ulps(DBL_MAX, -k);
    double target = (2 * k + 1) * 0x1p970;
    int e = range(r, -200, 200);
    switch (below(r, 3)) {
    case 0:
        *a = s * ldexp(step_ulps(target, range(r, -3, 3)), -e);
        *b = ldexp(1.0, e);
        break;
    case 1:
        *a = s * ldexp(target, -e);
        *b = ldexp(step_ulps(1.0, range(r, -3, 3)), e);
        break;
    default:
        *a = s * ldexp(step_ulps(target, range(r, -2, 2)), -e);
        *b = ldexp(step_ulps(1.0, range(r, -2, 2)), e);
        break;
    }
    if (below(r, 2)) { double t = *a; *a = *b; *b = t; }
}

static void gen_subnormal(rng_t *r, double *a, double *b, double *c) {
    switch (below(r, 4)) {
    case 0: /* the result, and usually the product, is subnormal */
        split_product(r, range(r, -1100, -960), 53, 53, a, b);
        switch (below(r, 4)) {
        case 0: *c = 0.0 * sign(r); break;
        case 1: *c = sign(r) * ldexp((double)(next64(r) & 0x000FFFFFFFFFFFFFull), -1074); break;
        case 2: *c = at_exp(r, range(r, -1022, -1000)); break;
        default: *c = step_ulps(-(*a * *b), range(r, -4, 4)); break;
        }
        break;
    case 1: /* a subnormal operand */
        *a = u2d((next64(r) & 0x800FFFFFFFFFFFFFull) | 1);
        *b = at_exp(r, range(r, 0, 1023));
        *c = (below(r, 2)) ? at_exp(r, range(r, -1074, -900)) : step_ulps(-(*a * *b), range(r, -4, 4));
        break;
    default: /* the product's error term is subnormal */
        split_product(r, range(r, -1022, -960), 53, 53, a, b);
        *c = (below(r, 2)) ? step_ulps(-(*a * *b), range(r, -4, 4))
                           : at_exp(r, ilogb(*a * *b) + range(r, -60, 3));
        break;
    }
    if (below(r, 2)) { double t = *a; *a = *b; *b = t; }
}

static void gen_zero_signs(rng_t *r, double *a, double *b, double *c) {
    static const double others[] = { 0.0, -0.0, 1.0, -1.0, 0x1p-1074, -0x1p-1074, DBL_MAX, -DBL_MAX, 2.5 };
    switch (below(r, 4)) {
    case 0:
    case 1:
        *a = 0.0 * sign(r);
        *b = (below(r, 3) == 0) ? at_exp(r, range(r, -1074, 1023))
                                : others[below(r, (int)(sizeof others / sizeof others[0]))];
        *c = 0.0 * sign(r);
        break;
    case 2: /* a nonzero product that rounds to zero in double */
        split_product(r, range(r, -1300, -1076), 53, 53, a, b);
        *c = 0.0 * sign(r);
        break;
    default: /* exact cancellation to zero */
        split_product(r, product_exp(r, -900, 900), range(r, 1, 26), range(r, 1, 27), a, b);
        *c = -(*a * *b);
        break;
    }
    if (below(r, 2)) { double t = *a; *a = *b; *b = t; }
}

static void gen_inf_zero(rng_t *r, double *a, double *b, double *c) {
    *a = INFINITY * sign(r);
    *b = 0.0 * sign(r);
    *c = specials(r);
    if (below(r, 8) == 0) *c = any_nan(r);
    if (below(r, 2)) { double t = *a; *a = *b; *b = t; }
}

static void gen_inf_c(rng_t *r, double *a, double *b, double *c) {
    *c = INFINITY * sign(r);
    if (below(r, 3) == 0) {
        *a = specials(r);
        *b = specials(r);
    } else {
        split_product(r, range(r, -1100, 2000), 53, 53, a, b);
    }
}

static void gen_nan(rng_t *r, double *a, double *b, double *c) {
    *a = specials(r);
    *b = specials(r);
    *c = specials(r);
    int m = range(r, 1, 7);
    if (m & 1) *a = any_nan(r);
    if (m & 2) *b = any_nan(r);
    if (m & 4) *c = any_nan(r);
}

static void gen_random_bits(rng_t *r, double *a, double *b, double *c) {
    *a = u2d(next64(r));
    *b = u2d(next64(r));
    *c = u2d(next64(r));
}

static double price(rng_t *r) {
    double hi = (below(r, 2)) ? 200.0 : 100000.0;
    return floor(unit(r) * hi * 100.0 + 1.0) / 100.0;
}

static void gen_random_real(rng_t *r, double *a, double *b, double *c) {
    switch (below(r, 4)) {
    case 0: { /* an EMA step */
        int n = range(r, 2, 400);
        double k = 2.0 / (n + 1);
        double x = price(r), prev = price(r);
        if (below(r, 2)) { *a = k; *b = x; *c = (1.0 - k) * prev; }
        else { *a = 1.0 - k; *b = prev; *c = k * x; }
        break;
    }
    case 1: /* prices and spreads, cancellation included */
        *a = price(r) * sign(r);
        *b = price(r) * sign(r);
        *c = (below(r, 2)) ? -(*a * *b) * (1.0 + (unit(r) - 0.5) * 1e-9) : price(r) * price(r) * sign(r);
        break;
    case 2:
        *a = at_exp(r, range(r, -30, 30));
        *b = at_exp(r, range(r, -30, 30));
        *c = at_exp(r, range(r, -60, 60));
        break;
    default:
        *a = sign(r) * unit(r);
        *b = price(r);
        *c = sign(r) * price(r);
        break;
    }
}

typedef void (*gen_fn)(rng_t *, double *, double *, double *);
static const gen_fn GENERATORS[F_COUNT] = {
    gen_ties, gen_ties_sticky, gen_cancel, gen_ovf_finite, gen_ovf_inf, gen_ovf_boundary,
    gen_subnormal, gen_zero_signs, gen_inf_zero, gen_inf_c, gen_nan,
    gen_random_bits, gen_random_real,
};

#define RECORD 40
#define BATCH 4096
static unsigned char out_buf[RECORD * BATCH];
static int out_n;

static void put64(unsigned char *p, uint64_t v) { for (int i = 0; i < 8; i++) p[i] = (unsigned char)(v >> (8 * i)); }
static void put32(unsigned char *p, uint32_t v) { for (int i = 0; i < 4; i++) p[i] = (unsigned char)(v >> (8 * i)); }

static void flush_out(void) {
    if (out_n && fwrite(out_buf, RECORD, (size_t)out_n, stdout) != (size_t)out_n) {
        perror("golden: write");
        exit(2);
    }
    out_n = 0;
}

static void emit(int family, double a, double b, double c) {
    volatile double va = a, vb = b, vc = c;
    double want = fma(va, vb, vc);
    double naive = va * vb + vc;
    uint32_t flags = (d2u(naive) != d2u(want) && !(isnan(naive) && isnan(want))) ? 1u : 0u;
    unsigned char *p = out_buf + RECORD * out_n;
    put64(p, d2u(a));
    put64(p + 8, d2u(b));
    put64(p + 16, d2u(c));
    put64(p + 24, d2u(want));
    put32(p + 32, (uint32_t)family);
    put32(p + 36, flags);
    if (++out_n == BATCH) flush_out();
}

int main(int argc, char **argv) {
    if (argc == 2 && strcmp(argv[1], "--families") == 0) {
        for (int f = 0; f < F_COUNT; f++) printf("%d %s\n", f, FAMILY_NAMES[f]);
        return 0;
    }
    if (argc != 6) {
        fprintf(stderr, "usage: golden --families | golden <seed> <worker> <workers> <hard-per-family> <random>\n");
        return 2;
    }
    uint64_t seed = strtoull(argv[1], NULL, 10);
    uint64_t worker = strtoull(argv[2], NULL, 10), workers = strtoull(argv[3], NULL, 10);
    uint64_t hard = strtoull(argv[4], NULL, 10), random_n = strtoull(argv[5], NULL, 10);
    if (workers == 0 || worker >= workers) {
        fprintf(stderr, "golden: bad worker split\n");
        return 2;
    }
    for (int f = 0; f < F_COUNT; f++) {
        uint64_t n = f < FIRST_RANDOM ? hard
                   : f == F_RANDOM_BITS ? random_n / 2 : random_n - random_n / 2;
        uint64_t base = mix64(mix64(seed) ^ ((uint64_t)f << 56));
        for (uint64_t j = worker; j < n; j += workers) {
            rng_t r = { mix64(base ^ j) };
            double a, b, c;
            GENERATORS[f](&r, &a, &b, &c);
            emit(f, a, b, c);
        }
    }
    flush_out();
    return fflush(stdout) == 0 ? 0 : 2;
}
