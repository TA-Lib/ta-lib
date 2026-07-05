#ifndef FUZZ_DATA_H
#define FUZZ_DATA_H

/* fuzz_data.h — deterministic input generator + output hasher, compiled
 * byte-identically into both ta_regtest and ta_064_serve so a (shape,seed,n)
 * tuple reproduces the same inputs on each side. See CLAUDE.md (--fuzz-064).
 * FP_CONTRACT off so the generator can't be FMA-fused on one side only. */

#pragma STDC FP_CONTRACT OFF
#include <math.h>

/* ---- splitmix64 PRNG (deterministic, self-contained) ---- */
static unsigned long long fuzz_sm_next(unsigned long long *s)
{
    unsigned long long z = (*s += 0x9E3779B97F4A7C15ULL);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
}

/* Uniform double in [0,1) from the top 53 bits. */
static double fuzz_sm_unit(unsigned long long *s)
{
    return (double)(fuzz_sm_next(s) >> 11) * (1.0 / 9007199254740992.0);
}

/* Data shapes. Keep FUZZ_NSHAPES last. */
enum {
    FUZZ_RANDWALK = 0,  /* geometric random walk around 100 (typical prices)   */
    FUZZ_MONO_UP,       /* strictly increasing                                 */
    FUZZ_MONO_DOWN,     /* strictly decreasing                                 */
    FUZZ_CONSTANT,      /* flat O=H=L=C (degenerate: hh==ll, zero variance)    */
    FUZZ_TIE_HEAVY,     /* small integer set — many equal values / ties        */
    FUZZ_EXTREME,       /* alternating huge (1e9) / tiny (1e-7) magnitudes     */
    FUZZ_WITH_ZEROS,    /* sprinkled 0.0 / -0.0 and small signed values        */
    FUZZ_NSHAPES
};

/* Fill OHLCV+OI arrays (length n) from (shape,seed). high>=max(o,c),
 * low<=min(o,c). Mul/add split into statements so nothing contracts. */
static void fuzz_gen(int shape, int seed, int n,
                     double *o, double *h, double *l, double *c,
                     double *v, double *oi)
{
    unsigned long long s =
        0x243F6A8885A308D3ULL
        ^ ((unsigned long long)(unsigned int)seed  * 0xD1B54A32D192ED03ULL)
        ^ ((unsigned long long)(unsigned int)shape << 32);
    double walk = 100.0;
    int i;
    for( i = 0; i < n; i++ )
    {
        double close, open, hi, lo, mag, t;

        if( shape == FUZZ_CONSTANT )
        {
            o[i] = h[i] = l[i] = c[i] = 42.0;
            v[i] = 1000000.0; oi[i] = 10000.0;
            continue;
        }
        if( shape == FUZZ_TIE_HEAVY )
        {
            close = (double)(3 + (int)(fuzz_sm_unit(&s) * 5.0)); /* {3..7} */
            o[i] = close;
            c[i] = close;
            h[i] = close + (double)(fuzz_sm_next(&s) & 1);
            l[i] = close - (double)(fuzz_sm_next(&s) & 1);
            v[i] = (double)(1 + (int)(fuzz_sm_unit(&s) * 4.0)) * 1000.0;
            oi[i] = 1000.0;
            continue;
        }

        switch( shape )
        {
        case FUZZ_MONO_UP:
            t = (double)i * 0.5; close = 10.0 + t; break;
        case FUZZ_MONO_DOWN:
            t = (double)i * 0.25; close = 500.0 - t; break;
        case FUZZ_EXTREME:
            t = fuzz_sm_unit(&s);
            close = (fuzz_sm_next(&s) & 1) ? (1.0 + t) * 1.0e9
                                          : (1.0 + t) * 1.0e-7;
            break;
        case FUZZ_WITH_ZEROS: {
            double r = fuzz_sm_unit(&s);
            if( r < 0.15 )      close = 0.0;
            else if( r < 0.30 ) close = -0.0;
            else              { t = r - 0.5; close = t * 8.0; }
            break;
        }
        case FUZZ_RANDWALK:
        default:
            t = fuzz_sm_unit(&s) - 0.5;
            t = t * 0.04;
            walk = walk * (1.0 + t);
            close = walk;
            break;
        }

        mag = fabs(close) * 0.01 + 0.001;
        t = fuzz_sm_unit(&s) - 0.5;
        t = t * mag;
        open = close + t;
        hi = (open > close) ? open : close;
        t = fuzz_sm_unit(&s) * mag;
        hi = hi + t;
        lo = (open < close) ? open : close;
        t = fuzz_sm_unit(&s) * mag;
        lo = lo - t;

        o[i] = open; h[i] = hi; l[i] = lo; c[i] = close;
        t = fuzz_sm_unit(&s) * 1.0e6;
        v[i] = 1000.0 + t;
        t = fuzz_sm_unit(&s) * 1.0e4;
        oi[i] = 100.0 + t;
    }
}

/* ---- 64-bit output hash (FNV-1a over raw bytes + murmur finalizer). ---- */
static unsigned long long fuzz_hash_init(void)
{
    return 1469598103934665603ULL; /* FNV-1a 64-bit offset basis */
}
static unsigned long long fuzz_hash_bytes(unsigned long long hsh,
                                          const void *p, unsigned long len)
{
    const unsigned char *b = (const unsigned char *)p;
    unsigned long i;
    for( i = 0; i < len; i++ )
    {
        hsh ^= (unsigned long long)b[i];
        hsh *= 1099511628211ULL; /* FNV-1a 64-bit prime */
    }
    return hsh;
}
static unsigned long long fuzz_hash_fin(unsigned long long hsh)
{
    hsh ^= hsh >> 33;
    hsh *= 0xFF51AFD7ED558CCDULL;
    hsh ^= hsh >> 33;
    hsh *= 0xC4CEB9FE1A85EC53ULL;
    hsh ^= hsh >> 33;
    return hsh;
}

#endif /* FUZZ_DATA_H */
