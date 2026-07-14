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
    FUZZ_ZEROSUM,       /* symmetric high=-low bars: high+low == 0 EXACTLY, the */
                        /* only shape landing high+low in the 1e-14 TA_IS_ZERO  */
                        /* band -> exercises the ACCBANDS degenerate else branch */
                        /* (no divide by high+low) in both fuzz-064 and stream   */
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
/* ---- FUZZ_CANDLE deterministic pattern catalog (issue #109) --------------- */
/* Beyond the seed-driven hikkake windows below, lay one hand-built firing
 * instance of each otherwise-vacuous multi-candle pattern so the fuzz-064 and
 * stream_verify gates exercise its real decision logic instead of comparing
 * all-zero output to all-zero. Each window self-primes with a short neutral run
 * so its candle-setting averages (BodyLong/Short/Doji, shadows, Near/Far, ...)
 * don't depend on the neighbouring windows. Pure geometry (no TA calls), so it
 * stays byte-identical across the v0.6.4 oracle boundary. The windows are a
 * fixed, seed-independent prefix, so every seed fires every pattern.
 * Each window was validated against the shipped library to produce the exact
 * expected output at the expected bar. */

/* Neutral primer: k alternating small-body bars (real body ~bd, extra half-range
 * ~hr beyond the body) around base. Sets the BodyLong/BodyShort averages to ~bd,
 * so a following long body (>> bd) or short body (<< bd) qualifies with margin. */
static int fuzz_cdl_primer(double *o,double *h,double *l,double *c,double *v,double *oi,
                           int p,int n,int k,double base,double bd,double hr)
{
    int i;
    for(i=0;i<k;i++){
        double O = (i&1)? base : base+bd;
        double C = (i&1)? base+bd : base;
        p=fuzz_cdl_bar(o,h,l,c,v,oi,p,n,O,base+bd+hr,base-hr,C);
    }
    return p;
}

/* CDL2CROWS (bearish, -100 on the 3rd candle): a long white body; a black candle
 * gapping up entirely above it; then a black candle opening inside the 2nd body
 * and closing inside the 1st body. */
static int fuzz_cdl_2crows(double *o,double *h,double *l,double *c,double *v,double *oi,
                           int p,int n,double base)
{
    p=fuzz_cdl_primer(o,h,l,c,v,oi,p,n,12,base,2.0,1.0);
    p=fuzz_cdl_bar(o,h,l,c,v,oi,p,n, base,      base+12.5, base-0.5,  base+12.0); /* 1st white long   */
    p=fuzz_cdl_bar(o,h,l,c,v,oi,p,n, base+18.0, base+18.5, base+13.5, base+14.0); /* 2nd black gap up */
    p=fuzz_cdl_bar(o,h,l,c,v,oi,p,n, base+16.0, base+16.5, base+5.5,  base+6.0);  /* 3rd black inside */
    return p;
}

/* CDL3BLACKCROWS (bearish, -100 on the 3rd crow): a white long body, then three
 * black candles each opening inside the prior black's body with declining
 * closes and near-zero lower shadows. */
static int fuzz_cdl_3blackcrows(double *o,double *h,double *l,double *c,double *v,double *oi,
                                int p,int n,double base)
{
    p=fuzz_cdl_primer(o,h,l,c,v,oi,p,n,13,base,2.0,1.0);
    p=fuzz_cdl_bar(o,h,l,c,v,oi,p,n, base,    base+11, base-1, base+10); /* i-3: white long body   */
    p=fuzz_cdl_bar(o,h,l,c,v,oi,p,n, base+8,  base+8,  base,   base);    /* i-2: 1st black          */
    p=fuzz_cdl_bar(o,h,l,c,v,oi,p,n, base+5,  base+5,  base-5, base-5);  /* i-1: 2nd black inside   */
    p=fuzz_cdl_bar(o,h,l,c,v,oi,p,n, base+2,  base+2,  base-8, base-8);  /* i:   3rd black inside   */
    return p;
}

/* CDL3WHITESOLDIERS (bullish, +100 on the 3rd soldier): three white candles with
 * climbing opens and closes, each opening inside the prior body and with a tiny
 * upper shadow (very-short-shadow), bodies not short. */
static int fuzz_cdl_3whitesoldiers(double *o,double *h,double *l,double *c,double *v,double *oi,
                                   int p,int n,double base)
{
    p=fuzz_cdl_primer(o,h,l,c,v,oi,p,n,12,base,2.0,1.0);
    p=fuzz_cdl_bar(o,h,l,c,v,oi,p,n, base,     base+10.2, base-0.2, base+10.0); /* 1st white long */
    p=fuzz_cdl_bar(o,h,l,c,v,oi,p,n, base+5.0, base+15.2, base+4.8, base+15.0); /* 2nd white       */
    p=fuzz_cdl_bar(o,h,l,c,v,oi,p,n, base+10.0,base+20.2, base+9.8, base+20.0); /* 3rd white       */
    return p;
}

/* CDL3STARSINSOUTH (bullish, +100 on the 3rd candle): a long black candle with a
 * long lower shadow; a smaller black candle opening higher but making a lower low
 * within the 1st range; a tiny black marubozu engulfed by the 2nd. */
static int fuzz_cdl_3starsinsouth(double *o,double *h,double *l,double *c,double *v,double *oi,
                                  int p,int n,double base)
{
    p=fuzz_cdl_primer(o,h,l,c,v,oi,p,n,12,base,2.0,1.0);
    p=fuzz_cdl_bar(o,h,l,c,v,oi,p,n, base+8.0, base+8.0, base-12.0, base);      /* 1st black long + long lower shadow */
    p=fuzz_cdl_bar(o,h,l,c,v,oi,p,n, base+6.0, base+6.0, base-4.0,  base+2.0);  /* 2nd black smaller body            */
    p=fuzz_cdl_bar(o,h,l,c,v,oi,p,n, base+1.0, base+1.0, base,      base);      /* 3rd black tiny marubozu           */
    return p;
}

/* CDL3LINESTRIKE (three-white branch, +100 on the strike): three white candles
 * with climbing closes each opening inside the prior body, then a black candle
 * opening above the 3rd close and closing below the 1st open. */
static int fuzz_cdl_3linestrike(double *o,double *h,double *l,double *c,double *v,double *oi,
                                int p,int n,double base)
{
    p=fuzz_cdl_primer(o,h,l,c,v,oi,p,n,12,base,2.0,1.0);
    p=fuzz_cdl_bar(o,h,l,c,v,oi,p,n, base,      base+5.0,  base-1.0, base+4.0);  /* i-3: 1st white soldier */
    p=fuzz_cdl_bar(o,h,l,c,v,oi,p,n, base+2.0,  base+7.0,  base+1.0, base+6.0);  /* i-2: 2nd white         */
    p=fuzz_cdl_bar(o,h,l,c,v,oi,p,n, base+4.0,  base+9.0,  base+3.0, base+8.0);  /* i-1: 3rd white         */
    p=fuzz_cdl_bar(o,h,l,c,v,oi,p,n, base+10.0, base+11.0, base-3.0, base-2.0);  /* i:   4th black strike  */
    return p;
}

/* CDLCONCEALBABYSWALL (bullish, +100 on the 4th candle): two black marubozu, a
 * black candle gapping down with an upper shadow poking into the 2nd body, then a
 * black candle engulfing the 3rd including its shadows. */
static int fuzz_cdl_concealbabyswall(double *o,double *h,double *l,double *c,double *v,double *oi,
                                     int p,int n,double base)
{
    p=fuzz_cdl_primer(o,h,l,c,v,oi,p,n,12,base,2.0,1.0);
    p=fuzz_cdl_bar(o,h,l,c,v,oi,p,n, base,     base,     base-3.0,  base-3.0);  /* 1st black marubozu       */
    p=fuzz_cdl_bar(o,h,l,c,v,oi,p,n, base-4.0, base-4.0, base-7.0,  base-7.0);  /* 2nd black marubozu       */
    p=fuzz_cdl_bar(o,h,l,c,v,oi,p,n, base-9.0, base-6.0, base-12.0, base-11.0); /* 3rd black gapdown+shadow */
    p=fuzz_cdl_bar(o,h,l,c,v,oi,p,n, base-5.0, base-4.0, base-16.0, base-15.0); /* 4th black engulfs 3rd    */
    return p;
}

/* CDLMATHOLD (bullish, +100 on the 5th candle): a long white candle, a short
 * black candle gapping up above it, two more short candles holding within the 1st
 * range with falling tops, then a white candle opening above the 4th close and
 * closing above the highest reaction high. */
static int fuzz_cdl_mathold(double *o,double *h,double *l,double *c,double *v,double *oi,
                            int p,int n,double base)
{
    p=fuzz_cdl_primer(o,h,l,c,v,oi,p,n,14,base,2.0,1.0);
    p=fuzz_cdl_bar(o,h,l,c,v,oi,p,n, base,      base+21.0, base-1.0,  base+20.0); /* c1 white long        */
    p=fuzz_cdl_bar(o,h,l,c,v,oi,p,n, base+31.0, base+32.0, base+29.0, base+30.0); /* c2 short black gap up */
    p=fuzz_cdl_bar(o,h,l,c,v,oi,p,n, base+16.0, base+17.0, base+14.0, base+15.0); /* c3 short, in 1st range*/
    p=fuzz_cdl_bar(o,h,l,c,v,oi,p,n, base+14.0, base+15.0, base+12.0, base+13.0); /* c4 short, falling     */
    p=fuzz_cdl_bar(o,h,l,c,v,oi,p,n, base+35.0, base+41.0, base+34.0, base+40.0); /* c5 white breakout     */
    return p;
}

/* CDLRISEFALL3METHODS (rising branch, +100 on the 5th candle): a long white
 * candle, three small falling black candles holding within the 1st range, then a
 * long white candle opening above the 4th close and closing above the 1st close. */
static int fuzz_cdl_risefall3methods(double *o,double *h,double *l,double *c,double *v,double *oi,
                                     int p,int n,double base)
{
    p=fuzz_cdl_primer(o,h,l,c,v,oi,p,n,12,base,2.0,1.0);
    p=fuzz_cdl_bar(o,h,l,c,v,oi,p,n, base,     base+12.5, base-0.5,  base+12.0); /* 1st long white          */
    p=fuzz_cdl_bar(o,h,l,c,v,oi,p,n, base+10.0,base+10.5, base+8.5,  base+9.0);  /* 2nd small black         */
    p=fuzz_cdl_bar(o,h,l,c,v,oi,p,n, base+8.0, base+8.5,  base+6.5,  base+7.0);  /* 3rd small black falling */
    p=fuzz_cdl_bar(o,h,l,c,v,oi,p,n, base+6.0, base+6.5,  base+4.5,  base+5.0);  /* 4th small black falling */
    p=fuzz_cdl_bar(o,h,l,c,v,oi,p,n, base+6.0, base+20.5, base+5.5,  base+20.0); /* 5th long white breakout */
    return p;
}

/* CDLADVANCEBLOCK (bearish, -100 on the 3rd candle): three white candles with
 * climbing closes but weakening bodies (1st long, then progressively shorter),
 * each opening within/near the prior body, tiny upper shadows. */
static int fuzz_cdl_advanceblock(double *o,double *h,double *l,double *c,double *v,double *oi,
                                 int p,int n,double base)
{
    p=fuzz_cdl_primer(o,h,l,c,v,oi,p,n,12,base,2.0,1.0);
    p=fuzz_cdl_bar(o,h,l,c,v,oi,p,n, base,     base+6.0, base,     base+6.0); /* 1st white long body */
    p=fuzz_cdl_bar(o,h,l,c,v,oi,p,n, base+5.0, base+7.0, base+5.0, base+7.0); /* 2nd white, shorter  */
    p=fuzz_cdl_bar(o,h,l,c,v,oi,p,n, base+7.0, base+8.0, base+7.0, base+8.0); /* 3rd white, shortest */
    return p;
}

/* CDLINNECK (bearish, -100 on the 2nd candle): a long black candle, then a white
 * candle opening below the 1st low and closing just into the 1st body (close
 * within the Equal band above the 1st close). Wider primer (hr=6) so the Equal
 * threshold (0.05*avg high-low) is a comfortable margin. */
static int fuzz_cdl_inneck(double *o,double *h,double *l,double *c,double *v,double *oi,
                           int p,int n,double base)
{
    p=fuzz_cdl_primer(o,h,l,c,v,oi,p,n,12,base,2.0,6.0);
    p=fuzz_cdl_bar(o,h,l,c,v,oi,p,n, base+10.0, base+12.0, base-1.0, base);       /* 1st long black         */
    p=fuzz_cdl_bar(o,h,l,c,v,oi,p,n, base-5.0,  base+1.0,  base-6.0, base+0.35);  /* 2nd white into neck    */
    return p;
}

/* Lay the deterministic per-family catalog. Appended to as each family's window
 * lands (issue #109); one entry per otherwise-vacuous pattern. */
static int fuzz_cdl_catalog(double *o,double *h,double *l,double *c,double *v,double *oi,
                            int p,int n)
{
    p=fuzz_cdl_2crows(o,h,l,c,v,oi,p,n,100.0);
    p=fuzz_cdl_3blackcrows(o,h,l,c,v,oi,p,n,100.0);
    p=fuzz_cdl_3whitesoldiers(o,h,l,c,v,oi,p,n,100.0);
    p=fuzz_cdl_3starsinsouth(o,h,l,c,v,oi,p,n,100.0);
    p=fuzz_cdl_3linestrike(o,h,l,c,v,oi,p,n,100.0);
    p=fuzz_cdl_concealbabyswall(o,h,l,c,v,oi,p,n,100.0);
    p=fuzz_cdl_mathold(o,h,l,c,v,oi,p,n,100.0);
    p=fuzz_cdl_risefall3methods(o,h,l,c,v,oi,p,n,100.0);
    p=fuzz_cdl_advanceblock(o,h,l,c,v,oi,p,n,100.0);
    p=fuzz_cdl_inneck(o,h,l,c,v,oi,p,n,100.0);
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
    p = fuzz_cdl_catalog(o,h,l,c,v,oi,p,n);   /* deterministic per-family windows (#109) */
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

/* ---- FUZZ_ZEROSUM: bars whose high+low is exactly zero ------------------- */
/* Drives the ACCBANDS degenerate branch, TA_IS_ZERO(high+low) -> upper=high,
 * lower=low (skipping the 4*(high-low)/(high+low) divide). No other shape lands
 * high+low inside the 1e-14 TA_IS_ZERO band (all perturb high/low with
 * independent draws), so this is the sole oracle coverage — fuzz-064 vs v0.6.4
 * AND stream_verify vs batch — of that else branch. A symmetric bar high=+a,
 * low=-a gives high+low == +0.0 exactly for any finite a (IEEE x + (-x) == +0);
 * all-zero bars cover a==0. Interleaved with ordinary positive bars so the
 * degenerate bars both ENTER and LEAVE the moving-average window (the add-side
 * and subtract-side of the else branch). Pure geometry, no TA calls, so it
 * compiles byte-identically into both sides of the fuzz-064 oracle. */
static void fuzz_zerosum_gen(int seed, int n,
                             double *o, double *h, double *l, double *c,
                             double *v, double *oi)
{
    unsigned long long s =
        0x243F6A8885A308D3ULL
        ^ ((unsigned long long)(unsigned int)seed * 0xD1B54A32D192ED03ULL)
        ^ ((unsigned long long)FUZZ_ZEROSUM << 32);
    int i;
    for( i = 0; i < n; i++ )
    {
        double r = fuzz_sm_unit(&s);
        double open, hi, lo, close, a, base, w, t;
        if( r < 0.34 )
        {
            /* symmetric zero-sum bar: hi + lo == +0.0 exactly (else branch). */
            a = 1.0 + fuzz_sm_unit(&s) * 50.0;
            hi = a; lo = -a; open = 0.0; close = 0.0;
        }
        else if( r < 0.50 )
        {
            /* all-zero degenerate bar: hi + lo == 0 with hi == lo == 0. */
            hi = 0.0; lo = 0.0; open = 0.0; close = 0.0;
        }
        else
        {
            /* ordinary positive bar around 100 (hi + lo != 0 -> then branch). */
            base  = 90.0 + fuzz_sm_unit(&s) * 20.0;
            w     = 0.5 + fuzz_sm_unit(&s) * 5.0;
            close = base;
            t     = fuzz_sm_unit(&s) - 0.5;
            open  = base + t;
            hi = (open > close) ? open : close;
            hi = hi + w;
            lo = (open < close) ? open : close;
            lo = lo - w;
        }
        o[i] = open; h[i] = hi; l[i] = lo; c[i] = close;
        t = fuzz_sm_unit(&s) * 1000.0;
        v[i] = 1000.0 + t;
        t = fuzz_sm_unit(&s) * 100.0;
        oi[i] = 100.0 + t;
    }
}

/* Fill OHLCV+OI arrays (length n) from (shape,seed). high>=max(o,c),
 * low<=min(o,c). Mul/add split into statements so nothing contracts. */
static void fuzz_gen(int shape, int seed, int n,
                     double *o, double *h, double *l, double *c,
                     double *v, double *oi)
{
    if( shape == FUZZ_CANDLE ) { fuzz_candle_gen(seed, n, o, h, l, c, v, oi); return; }
    if( shape == FUZZ_ZEROSUM ) { fuzz_zerosum_gen(seed, n, o, h, l, c, v, oi); return; }
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
