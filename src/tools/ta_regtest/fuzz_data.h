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
    FUZZ_CANDLE,        /* inside-bar cascades + breakouts + confirmations —    */
                        /* pattern-rich so CDLHIKKAKE(MOD) and the other inside */
                        /* -bar candlesticks actually FIRE (detection AND the   */
                        /* +/-200 confirmation), instead of all-zero (vacuous)  */
    FUZZ_NSHAPES
};

/* ---- FUZZ_CANDLE: deterministic, pattern-rich inside-bar OHLC ------------ */
/* Pure geometry (no TA calls) so it compiles byte-identically into both sides
 * of the fuzz-064 oracle and the stream server. Lays a catalog of hand-designed
 * hikkake / modified-hikkake windows (bull & bear detection, confirmation in and
 * out of the 3-bar window, and single-predicate-broken near-misses) separated by
 * FLAT filler so the confirmation countdown expires between windows. "Close near
 * low/high" for a modified-hikkake 2nd candle is forced by putting Close exactly
 * on the Low/High (<= Low+avg for any avg>=0), robust to candle settings. */
static int fuzz_cdl_bar(double *o,double *h,double *l,double *c,double *v,double *oi,
                        int p,int n,double O,double H,double L,double Cl)
{
    double hi=H, lo=L;
    if(hi<O)hi=O; if(hi<Cl)hi=Cl;   /* clamp to a valid candle */
    if(lo>O)lo=O; if(lo>Cl)lo=Cl;
    if(p<n){ o[p]=O; h[p]=hi; l[p]=lo; c[p]=Cl; v[p]=1000.0; oi[p]=100.0; p++; }
    return p;
}
static int fuzz_cdl_flat(double *o,double *h,double *l,double *c,double *v,double *oi,
                         int p,int n,int k,double base)
{
    while(k-->0) p=fuzz_cdl_bar(o,h,l,c,v,oi,p,n,base,base+1.0,base-1.0,base);
    return p;
}
/* Hikkake window: 3 bars + optional confirm. dir +1 bull/-1 bear;
 * brk 0=intact,1=break P1,2=break P2,3/4=break the breakout; conf
 * 0=none,1=confirm next bar,2=confirm one bar too late (expired). */
static int fuzz_cdl_hikkake(double *o,double *h,double *l,double *c,double *v,double *oi,
                            int p,int n,double base,double w,int dir,int brk,int conf)
{
    double H1=base+w,      L1=base-w;
    double H2=(brk==1)? base+w+0.5 : base+0.6*w;
    double L2=(brk==2)? base-w-0.5 : base-0.6*w;
    double H3,L3,cc;
    if(dir>0){ H3=(brk==3)? H2+0.4*w : H2-0.4*w; L3=L2-0.4*w; }
    else     { H3=(brk==4)? H2-0.4*w : H2+0.4*w; L3=L2+0.4*w; }
    p=fuzz_cdl_bar(o,h,l,c,v,oi,p,n,base,H1,L1,base);
    p=fuzz_cdl_bar(o,h,l,c,v,oi,p,n,base,H2,L2,base);
    p=fuzz_cdl_bar(o,h,l,c,v,oi,p,n,base,H3,L3,base);
    if(conf==2) p=fuzz_cdl_flat(o,h,l,c,v,oi,p,n,3,base);
    if(conf){ cc = dir>0 ? H2+w : L2-w; p=fuzz_cdl_bar(o,h,l,c,v,oi,p,n,cc,cc+0.5,cc-0.5,cc); }
    return p;
}
/* Modified-hikkake window: 4 nested/breakout bars + optional confirm. */
static int fuzz_cdl_hikkakemod(double *o,double *h,double *l,double *c,double *v,double *oi,
                               int p,int n,double base,double w,int dir,int brk,int conf)
{
    double H1=base+w,      L1=base-w;
    double H2=(brk==1)? base+w+0.5 : base+0.7*w;
    double L2=(brk==1)? base-w-0.5 : base-0.7*w;
    double H3=(brk==2)? H2+0.3*w   : base+0.45*w;
    double L3=(brk==2)? L2-0.3*w   : base-0.45*w;
    double H4,L4,C2,cc;
    if(dir>0){ H4=(brk==3)? H3+0.3*w : H3-0.3*w; L4=L3-0.3*w; }
    else     { H4=(brk==3)? H3-0.3*w : H3+0.3*w; L4=L3+0.3*w; }
    C2 = (brk==4)? base : (dir>0 ? L2 : H2);   /* 2nd close near low/high */
    p=fuzz_cdl_bar(o,h,l,c,v,oi,p,n,base,H1,L1,base);
    p=fuzz_cdl_bar(o,h,l,c,v,oi,p,n,base,H2,L2,C2);
    p=fuzz_cdl_bar(o,h,l,c,v,oi,p,n,base,H3,L3,base);
    p=fuzz_cdl_bar(o,h,l,c,v,oi,p,n,base,H4,L4,base);
    if(conf){ cc = dir>0 ? H3+w : L3-w; p=fuzz_cdl_bar(o,h,l,c,v,oi,p,n,cc,cc+0.5,cc-0.5,cc); }
    return p;
}
static void fuzz_candle_gen(int seed, int n,
                            double *o, double *h, double *l, double *c,
                            double *v, double *oi)
{
    unsigned long long s =
        0x243F6A8885A308D3ULL
        ^ ((unsigned long long)(unsigned int)seed * 0xD1B54A32D192ED03ULL);
    int p = fuzz_cdl_flat(o,h,l,c,v,oi,0,n,6,100.0);
    while( p < n-16 )
    {
        double base = 100.0 + (fuzz_sm_unit(&s)-0.5)*40.0;
        double w    = 8.0 + fuzz_sm_unit(&s)*20.0;
        int variant = (int)(fuzz_sm_unit(&s)*10.0);
        int dir = (fuzz_sm_next(&s)&1) ? 1 : -1;
        switch( variant )
        {
        case 0:  p=fuzz_cdl_hikkake(o,h,l,c,v,oi,p,n,base,w,dir,0,1); break;
        case 1:  p=fuzz_cdl_hikkake(o,h,l,c,v,oi,p,n,base,w,dir,0,2); break;
        case 2:  p=fuzz_cdl_hikkake(o,h,l,c,v,oi,p,n,base,w,dir,1,0); break;
        case 3:  p=fuzz_cdl_hikkake(o,h,l,c,v,oi,p,n,base,w,dir,2,0); break;
        case 4:  p=fuzz_cdl_hikkake(o,h,l,c,v,oi,p,n,base,w,dir,dir>0?3:4,0); break;
        case 5:  p=fuzz_cdl_hikkakemod(o,h,l,c,v,oi,p,n,base,w,dir,0,1); break;
        case 6:  p=fuzz_cdl_hikkakemod(o,h,l,c,v,oi,p,n,base,w,dir,0,0); break;
        case 7:  p=fuzz_cdl_hikkakemod(o,h,l,c,v,oi,p,n,base,w,dir,1,0); break;
        case 8:  p=fuzz_cdl_hikkakemod(o,h,l,c,v,oi,p,n,base,w,dir,3,0); break;
        default: p=fuzz_cdl_hikkakemod(o,h,l,c,v,oi,p,n,base,w,dir,4,1); break;
        }
        p=fuzz_cdl_flat(o,h,l,c,v,oi,p,n,6,base);
    }
    while( p < n ) p=fuzz_cdl_flat(o,h,l,c,v,oi,p,n,1,100.0);
}

/* Fill OHLCV+OI arrays (length n) from (shape,seed). high>=max(o,c),
 * low<=min(o,c). Mul/add split into statements so nothing contracts. */
static void fuzz_gen(int shape, int seed, int n,
                     double *o, double *h, double *l, double *c,
                     double *v, double *oi)
{
    if( shape == FUZZ_CANDLE ) { fuzz_candle_gen(seed, n, o, h, l, c, v, oi); return; }
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
