/* Probe for #548, never shipped: SQZMOM without series-length scratch.
 *
 * Variants, all with one signature and all compared bit for bit with the
 * shipped TA_SQZMOM (ref):
 *   blk   blocked over block-sized pads, SMA / VAR / LINEARREG hand-carried
 *         (the shipped batch loops with their loop-carried locals saved),
 *         MIDPRICE restarted on its own period grid, TRANGE on the sub-range
 *   five  hand copy of the generated composition, five mallocs (control)
 *   one   the same with one malloc sliced five ways
 *   strm  the public TA_SQZMOM stream: opened on lookbackTotal+1 bars, then
 *         one Update per later bar
 *   sblk  blocked, every stateful sub-calculation carried by its own public
 *         stream handle, Update loops grouped per handle over each block
 *   iblk  sblk with each per-handle Update loop replaced by one PROBE_X_Block:
 *         a loop over the callee's private StepImpl in the callee's own
 *         translation unit (inl_x.c, probe_blocks.h)
 *   iblkv iblk with STDDEV as VAR's block plus STDDEV's map, so no public
 *         Update is left in any hot loop
 *   hblk  iblkv with MIDPRICE and TRANGE as in blk (public batch calls per
 *         block), the step blocks kept for SMA x3, VAR and LINEARREG
 *   xblk  blk with the sub-calculations PROBE_XMASK names (default all) as
 *         step blocks: in-composition attribution of the step cost
 *   kblk  hblk with the five step blocks replaced by kernels derived
 *         mechanically from each step (kern_x.h, probe_blocks.h)
 *
 * Build: build.sh.  Run: probe check | probe long |
 *        probe time <points> <blockLen> <reps> [period] [fresh] |
 *        probe parts <points> <blockLen> <reps> [period]
 * parts times each sub-calculation alone over the whole series, block by
 * block, in its blk form, its step-block form and (SMA, VAR, LINEARREG, EMA)
 * its two mechanical kernels.
 */
#define _POSIX_C_SOURCE 200809L
#ifdef __APPLE__
#define _DARWIN_C_SOURCE 1   /* struct rusage's ru_minflt */
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <sys/resource.h>
#ifdef __APPLE__
#include <pthread.h>
#include <pthread/qos.h>
#endif
#include "ta_libc.h"
#include "probe_blocks.h"

/* Every block continuation a blocked variant makes; the check reads it
 * around each call so a block-edge count cannot be vacuous. */
static long long probeEdges;

/* The generated ta_SQZMOM.c squeeze expression, verbatim. */
#define SQZ_LEVEL(lo, up, centre, band, optInFactorWide, optInFactorNormal, optInFactorNarrow) \
   ((3) ^ (((((2) ^ (((((1) ^ (((((-1) & -(int)(((lo < centre - optInFactorWide * band) & (up > centre + optInFactorWide * band))))) ^ (1)) & ((int)(((lo > centre - optInFactorWide * band) & (up < centre + optInFactorWide * band))) - 1)))) ^ (2)) & ((int)(((lo > centre - optInFactorNormal * band) & (up < centre + optInFactorNormal * band))) - 1)))) ^ (3)) & ((int)(((lo > centre - optInFactorNarrow * band) & (up < centre + optInFactorNarrow * band))) - 1)))

#define IN(idx) in[(idx) - inBase]

/* Built with -DPROBE_RESEED=1 the carried means re-seed at every block, which
 * the comparison must report: the proof that it can fail. */
#ifndef PROBE_RESEED
#define PROBE_RESEED 0
#endif

/* ---- carried sub-calls ------------------------------------------------ */

typedef struct { int live; double periodTotal; } SmaCarry;

/* Outputs for input indices from..to. The first call's `from` is the start
 * index of the batch call it stands for; later calls continue at to+1. */
static void sma_blk( SmaCarry *s, const double *in, int inBase, int optInTimePeriod,
                     int from, int to, double *outReal )
{
   double periodTotal, tempReal;
   int i, trailingIdx, outIdx;

   trailingIdx = from - (optInTimePeriod - 1);
   if( !s->live || PROBE_RESEED )
   {
      periodTotal = 0.0;
      i = trailingIdx;
      if( optInTimePeriod > 1 )
      {
         while( i < from )
         {
            periodTotal += IN(i);
            i = i + 1;
         }
      }
      s->live = 1;
   }
   else
   {
      periodTotal = s->periodTotal;
      i = from;
   }
   outIdx = 0;
   while( i <= to )
   {
      periodTotal += IN(i);
      i = i + 1;
      tempReal = periodTotal;
      periodTotal -= IN(trailingIdx);
      trailingIdx = trailingIdx + 1;
      outReal[outIdx] = tempReal / (double)optInTimePeriod;
      outIdx = outIdx + 1;
   }
   s->periodTotal = periodTotal;
}

typedef struct
{
   int live, barsSinceReseed;
   double shift, periodTotal1, periodTotal2, peakTotal2;
} VarCarry;

/* from <= to always: the shipped loop is a do/while. */
static void var_blk( VarCarry *s, const double *in, int inBase, int optInTimePeriod,
                     int from, int to, double *outReal )
{
   double tempReal, periodTotal1, periodTotal2, meanValue1, shift, variance, invPeriod, peakTotal2;
   int i, j, outIdx, trailingIdx, nbInitialElementNeeded, barsSinceReseed, windowStart;

   nbInitialElementNeeded = optInTimePeriod - 1;
   invPeriod = 1.0 / (double)optInTimePeriod;
   trailingIdx = from - nbInitialElementNeeded;
   if( !s->live )
   {
      shift = IN(trailingIdx);
      periodTotal1 = 0.0;
      periodTotal2 = 0.0;
      for( j = trailingIdx; j < from; j += 1 )
      {
         tempReal = IN(j) - shift;
         periodTotal1 += tempReal;
         tempReal *= tempReal;
         periodTotal2 += tempReal;
      }
      barsSinceReseed = 32 * optInTimePeriod;
      peakTotal2 = periodTotal2;
      s->live = 1;
   }
   else
   {
      shift = s->shift;
      periodTotal1 = s->periodTotal1;
      periodTotal2 = s->periodTotal2;
      peakTotal2 = s->peakTotal2;
      barsSinceReseed = s->barsSinceReseed;
   }
   i = from;
   outIdx = 0;
   do
   {
      tempReal = IN(i) - shift;
      periodTotal1 += tempReal;
      tempReal *= tempReal;
      periodTotal2 += tempReal;
      peakTotal2 = (periodTotal2 > peakTotal2) ? periodTotal2 : peakTotal2;
      meanValue1 = periodTotal1 * invPeriod;
      variance = periodTotal2 * invPeriod - meanValue1 * meanValue1;
      tempReal = IN(trailingIdx) - shift;
      periodTotal1 -= tempReal;
      tempReal *= tempReal;
      periodTotal2 -= tempReal;
      trailingIdx += 1;
      barsSinceReseed -= 1;
      if( variance < 0.000001 * (peakTotal2 * invPeriod) || barsSinceReseed <= 0 )
      {
         barsSinceReseed = 32 * optInTimePeriod;
         windowStart = i - nbInitialElementNeeded;
         tempReal = 0.0;
         for( j = windowStart; j <= i; j += 1 )
         {
            tempReal += IN(j);
         }
         shift = tempReal * invPeriod;
         periodTotal1 = 0.0;
         periodTotal2 = 0.0;
         for( j = windowStart; j <= i; j += 1 )
         {
            tempReal = IN(j) - shift;
            periodTotal1 += tempReal;
            tempReal *= tempReal;
            periodTotal2 += tempReal;
         }
         meanValue1 = periodTotal1 * invPeriod;
         variance = periodTotal2 * invPeriod - meanValue1 * meanValue1;
         if( variance < 0.000001 * (periodTotal2 * invPeriod) )
         {
            shift = IN(i);
            periodTotal1 = 0.0;
            periodTotal2 = 0.0;
            for( j = windowStart; j <= i; j += 1 )
            {
               tempReal = IN(j) - shift;
               periodTotal1 += tempReal;
               tempReal *= tempReal;
               periodTotal2 += tempReal;
            }
            meanValue1 = periodTotal1 * invPeriod;
            variance = periodTotal2 * invPeriod - meanValue1 * meanValue1;
         }
         peakTotal2 = periodTotal2;
         if( variance < 0.000000000001 * (periodTotal2 * invPeriod) )
         {
            variance = 0.0;
         }
         tempReal = IN(windowStart) - shift;
         periodTotal1 -= tempReal;
         tempReal *= tempReal;
         periodTotal2 -= tempReal;
      }
      outReal[outIdx++] = variance;
      i += 1;
   } while( i <= to );
   s->shift = shift;
   s->periodTotal1 = periodTotal1;
   s->periodTotal2 = periodTotal2;
   s->peakTotal2 = peakTotal2;
   s->barsSinceReseed = barsSinceReseed;
}

typedef struct
{
   int live, barsSinceReseed;
   double SumXY, SumY, sumAbs, trailingValue;
} LrCarry;

static void lr_blk( LrCarry *s, const double *in, int inBase, int optInTimePeriod,
                    int from, int to, double *outReal )
{
   int outIdx, today, lookbackTotal, trailingIdx, i, j, windowStart, barsSinceReseed;
   double SumX, SumXY, SumY, SumXSqr, Divisor, m, b;
   double tempValue1, tempValue2, trailingValue, weightedTrailing, sumAbs;

   lookbackTotal = optInTimePeriod - 1;
   outIdx = 0;
   today = from;
   trailingIdx = from - lookbackTotal;
   SumX = (double)optInTimePeriod * (optInTimePeriod - 1) * 0.5;
   SumXSqr = (double)optInTimePeriod * (optInTimePeriod - 1) * (2 * optInTimePeriod - 1) / 6.0;
   Divisor = SumX * SumX - optInTimePeriod * SumXSqr;
   if( !s->live )
   {
      SumXY = 0;
      SumY = 0;
      sumAbs = 0;
      for( i = optInTimePeriod; i-- != 0;  )
      {
         tempValue1 = IN(today - i);
         SumY += tempValue1;
         SumXY += (double)i * tempValue1;
         sumAbs += fabs(tempValue1);
      }
      m = (optInTimePeriod * SumXY - SumX * SumY) / Divisor;
      b = (SumY - m * SumX) / (double)optInTimePeriod;
      barsSinceReseed = 32 * optInTimePeriod;
      trailingValue = IN(trailingIdx);
      trailingIdx += 1;
      outReal[outIdx++] = fma(m, (double)(optInTimePeriod - 1), b);
      today += 1;
      s->live = 1;
   }
   else
   {
      SumXY = s->SumXY;
      SumY = s->SumY;
      sumAbs = s->sumAbs;
      trailingValue = s->trailingValue;
      barsSinceReseed = s->barsSinceReseed;
   }
   while( today <= to )
   {
      weightedTrailing = (double)optInTimePeriod * trailingValue;
      SumXY = SumXY + SumY - weightedTrailing;
      SumY = SumY - trailingValue + IN(today);
      sumAbs = sumAbs - fabs(trailingValue) + fabs(IN(today));
      barsSinceReseed -= 1;
      if( barsSinceReseed <= 0 || fabs(weightedTrailing) > 100.0 * sumAbs )
      {
         barsSinceReseed = 32 * optInTimePeriod;
         windowStart = today - lookbackTotal;
         SumY = 0;
         SumXY = 0;
         sumAbs = 0;
         tempValue2 = (double)lookbackTotal;
         for( j = windowStart; j <= today; j += 1 )
         {
            tempValue1 = IN(j);
            SumY += tempValue1;
            SumXY += tempValue2 * tempValue1;
            sumAbs += fabs(tempValue1);
            tempValue2 -= 1.0;
         }
      }
      m = (optInTimePeriod * SumXY - SumX * SumY) / Divisor;
      b = (SumY - m * SumX) / (double)optInTimePeriod;
      trailingValue = IN(trailingIdx);
      trailingIdx += 1;
      outReal[outIdx++] = fma(m, (double)(optInTimePeriod - 1), b);
      today += 1;
   }
   s->SumXY = SumXY;
   s->SumY = SumY;
   s->sumAbs = sumAbs;
   s->trailingValue = trailingValue;
   s->barsSinceReseed = barsSinceReseed;
}

typedef struct { int live; double prevMA; } EmaCarry;

/* The shipped TA_EMA double body for a period above 1 (parts only). */
static void ema_blk( EmaCarry *s, const double *in, int inBase, int optInTimePeriod,
                     int from, int to, double *outReal )
{
   double emaBeta, optInK_1, tempReal, prevMA;
   int i, today, outIdx, lookbackTotal;

   emaBeta = (double)(optInTimePeriod - 1) / (double)(optInTimePeriod + 1);
   optInK_1 = 1.0 - emaBeta;
   if( emaBeta < 0.5 )
   {
      emaBeta = 1.0 - optInK_1;
   }
   if( !s->live )
   {
      lookbackTotal = TA_EMA_Lookback( optInTimePeriod );
      today = from - lookbackTotal;
      i = optInTimePeriod;
      tempReal = 0.0;
      while( i-- > 0 )
      {
         tempReal += IN(today++);
      }
      prevMA = tempReal / optInTimePeriod;
      while( today <= from )
      {
         prevMA = fma(emaBeta, prevMA, optInK_1 * IN(today++));
      }
      outReal[0] = prevMA;
      outIdx = 1;
      s->live = 1;
   }
   else
   {
      prevMA = s->prevMA;
      today = from;
      outIdx = 0;
   }
   while( today <= to )
   {
      prevMA = fma(emaBeta, prevMA, optInK_1 * IN(today++));
      outReal[outIdx++] = prevMA;
   }
   s->prevMA = prevMA;
}

/* ---- blk: the blocked body, hand-carried loops ----------------------- */

/* Pads hold bars [a-T, b] of a block [a, b], T = k-1: the tail the next
 * block's windows reach back into. In/out buffers must not overlap. */
static TA_RetCode sqzmom_blocked( int startIdx, int endIdx,
                                  const double inHigh[], const double inLow[], const double inClose[],
                                  int optInBBPeriod, double optInNbDev, int optInKCPeriod,
                                  double optInFactorWide, double optInFactorNormal, double optInFactorNarrow,
                                  int blockLen,
                                  int *outBegIdx, int *outNBElement,
                                  double outMomentum[], int outSqueeze[] )
{
   TA_RetCode retCode;
   int lookbackTotal, stageIdx, T, padLen, a, b, f, g, t, i, len, padBase, ob, on, first;
   double *mem, *devPad, *kcmPad, *trPad, *raw, *bandPad, *midPad, *sdPad;
   double dev, up, lo, centre, band;
   SmaCarry kcm = {0}, bandS = {0}, mid = {0};
   VarCarry var = {0};
   LrCarry lr = {0};

   if( optInFactorNormal > optInFactorWide || optInFactorNarrow > optInFactorNormal )
      return TA_BAD_PARAM;
   lookbackTotal = TA_SQZMOM_Lookback( optInBBPeriod, optInNbDev, optInKCPeriod,
                                       optInFactorWide, optInFactorNormal, optInFactorNarrow );
   if( lookbackTotal < 0 )
      return TA_BAD_PARAM;
   *outBegIdx = 0;
   *outNBElement = 0;
   if( lookbackTotal > endIdx )
      return TA_SUCCESS;
   if( startIdx < lookbackTotal )
      startIdx = lookbackTotal;
   if( startIdx > endIdx )
      return TA_SUCCESS;

   T = optInKCPeriod - 1;
   stageIdx = startIdx - T;
   padLen = T + blockLen;
   mem = malloc( (size_t)(3 * padLen + (padLen + T) + 3 * blockLen) * sizeof(double) );
   if( !mem )
      return TA_ALLOC_ERR;
   devPad = mem;
   kcmPad = devPad + padLen;
   trPad = kcmPad + padLen;
   raw = trPad + padLen;
   bandPad = raw + padLen + T;
   midPad = bandPad + blockLen;
   sdPad = midPad + blockLen;

   first = 1;
   a = startIdx;
   len = 0;
   while( a <= endIdx )
   {
      b = a + blockLen - 1;
      if( b > endIdx )
         b = endIdx;
      padBase = a - T;
      if( first )
         f = stageIdx;
      else
      {
         f = a;
         probeEdges++;
         memmove( devPad, devPad + len, (size_t)T * sizeof(double) );
         memmove( trPad, trPad + len, (size_t)T * sizeof(double) );
      }
      len = b - a + 1;

      /* The extrema restart on the grid the single-range call scans. */
      g = f - ((f - stageIdx) % optInKCPeriod);
      retCode = TA_MIDPRICE( g, b, inHigh, inLow, optInKCPeriod, &ob, &on, raw );
      if( retCode != TA_SUCCESS || ob != g || on != b - g + 1 )
      {
         free( mem );
         return retCode != TA_SUCCESS ? retCode : TA_INTERNAL_ERROR(1);
      }
      sma_blk( &kcm, inClose, 0, optInKCPeriod, f, b, kcmPad + (f - padBase) );
      for( t = f; t <= b; t++ )
         devPad[t - padBase] = inClose[t] - (raw[t - g] + kcmPad[t - padBase]) / 2.0;

      retCode = TA_TRANGE( f, b, inHigh, inLow, inClose, &ob, &on, trPad + (f - padBase) );
      if( retCode != TA_SUCCESS || ob != f || on != b - f + 1 )
      {
         free( mem );
         return retCode != TA_SUCCESS ? retCode : TA_INTERNAL_ERROR(2);
      }
      sma_blk( &bandS, trPad, padBase, optInKCPeriod, a, b, bandPad );
      sma_blk( &mid, inClose, 0, optInBBPeriod, a, b, midPad );
      var_blk( &var, inClose, 0, optInBBPeriod, a, b, sdPad );
      for( i = 0; i < len; i += 1 )
         sdPad[i] = sqrt(sdPad[i]);
      lr_blk( &lr, devPad, padBase, optInKCPeriod, a, b, outMomentum + (a - startIdx) );

      for( i = 0; i < len; i += 1 )
      {
         dev = sdPad[i] * optInNbDev;
         up = midPad[i] + dev;
         lo = midPad[i] - dev;
         centre = kcmPad[i + T];
         band = bandPad[i];
         outSqueeze[a - startIdx + i] = SQZ_LEVEL( lo, up, centre, band, optInFactorWide, optInFactorNormal, optInFactorNarrow );
      }
      first = 0;
      a = b + 1;
   }
   free( mem );
   *outBegIdx = startIdx;
   *outNBElement = endIdx - startIdx + 1;
   return TA_SUCCESS;
}

/* ---- xblk: blk with chosen sub-calculations as step blocks ------------ */

/* PROBE_XMASK (default 127, every one) picks which of blk's sub-calculations
 * run as PROBE_X_Block over a handle instead of blk's form; everything else
 * (pads, block grid, maps) is blk's, so mask 0 is blk. A handle is opened on
 * the first block at the bar that block's hand form starts on. */
enum { X_KCM = 1, X_BAND = 2, X_MID = 4, X_VAR = 8, X_LR = 16, X_MIDP = 32, X_TR = 64 };

static int xblkMask( void )
{
   static int mask = -1;
   if( mask < 0 )
   {
      const char *s = getenv( "PROBE_XMASK" );
      mask = s ? (int)strtol( s, NULL, 0 ) & 127 : 127;
   }
   return mask;
}

static TA_RetCode sqzmom_xblk( int startIdx, int endIdx,
                               const double inHigh[], const double inLow[], const double inClose[],
                               int optInBBPeriod, double optInNbDev, int optInKCPeriod,
                               double optInFactorWide, double optInFactorNormal, double optInFactorNarrow,
                               int blockLen,
                               int *outBegIdx, int *outNBElement,
                               double outMomentum[], int outSqueeze[] )
{
   TA_RetCode retCode = TA_SUCCESS;
   int lookbackTotal, stageIdx, T, k, bb, padLen, a, b, f, g, t, i, len, padBase, ob, on, first, mask;
   double *mem, *devPad, *kcmPad, *trPad, *raw, *bandPad, *midPad, *sdPad, *mom;
   double dev, up, lo, centre, band;
   SmaCarry kcm = {0}, bandS = {0}, mid = {0};
   VarCarry var = {0};
   LrCarry lr = {0};
   TA_SMA_Stream *hKcm = NULL, *hBand = NULL, *hMid = NULL;
   TA_VAR_Stream *hVar = NULL;
   TA_LINEARREG_Stream *hLr = NULL;
   TA_MIDPRICE_Stream *hMp = NULL;
   TA_TRANGE_Stream *hTr = NULL;

   if( optInFactorNormal > optInFactorWide || optInFactorNarrow > optInFactorNormal )
      return TA_BAD_PARAM;
   lookbackTotal = TA_SQZMOM_Lookback( optInBBPeriod, optInNbDev, optInKCPeriod,
                                       optInFactorWide, optInFactorNormal, optInFactorNarrow );
   if( lookbackTotal < 0 )
      return TA_BAD_PARAM;
   *outBegIdx = 0;
   *outNBElement = 0;
   if( lookbackTotal > endIdx )
      return TA_SUCCESS;
   if( startIdx < lookbackTotal )
      startIdx = lookbackTotal;
   if( startIdx > endIdx )
      return TA_SUCCESS;

   mask = xblkMask();
   k = optInKCPeriod;
   bb = optInBBPeriod;
   T = k - 1;
   stageIdx = startIdx - T;
   padLen = T + blockLen;
   mem = malloc( (size_t)(3 * padLen + (padLen + T) + 3 * blockLen) * sizeof(double) );
   if( !mem )
      return TA_ALLOC_ERR;
   devPad = mem;
   kcmPad = devPad + padLen;
   trPad = kcmPad + padLen;
   raw = trPad + padLen;
   bandPad = raw + padLen + T;
   midPad = bandPad + blockLen;
   sdPad = midPad + blockLen;

#define XBLK_OK( call ) \
   do { if( (retCode = (call)) != TA_SUCCESS ) goto fail; } while( 0 )

   first = 1;
   a = startIdx;
   len = 0;
   while( a <= endIdx )
   {
      b = a + blockLen - 1;
      if( b > endIdx )
         b = endIdx;
      padBase = a - T;
      if( first )
         f = stageIdx;
      else
      {
         f = a;
         probeEdges++;
         memmove( devPad, devPad + len, (size_t)T * sizeof(double) );
         memmove( trPad, trPad + len, (size_t)T * sizeof(double) );
      }
      len = b - a + 1;
      mom = outMomentum + (a - startIdx);

      if( mask & X_MIDP )
      {
         g = f;
         if( first )
         {
            XBLK_OK( TA_MIDPRICE_Open( &hMp, inHigh + f - T, inLow + f - T, k, k, raw ) );
            PROBE_MIDPRICE_Block( hMp, inHigh + f + 1, inLow + f + 1, b - f, raw + 1 );
         }
         else
            PROBE_MIDPRICE_Block( hMp, inHigh + f, inLow + f, b - f + 1, raw );
      }
      else
      {
         g = f - ((f - stageIdx) % k);
         retCode = TA_MIDPRICE( g, b, inHigh, inLow, k, &ob, &on, raw );
         if( retCode == TA_SUCCESS && (ob != g || on != b - g + 1) )
            retCode = TA_INTERNAL_ERROR(30);
         if( retCode != TA_SUCCESS )
            goto fail;
      }
      if( mask & X_KCM )
      {
         double *o = kcmPad + (f - padBase);
         if( first )
         {
            XBLK_OK( TA_SMA_Open( &hKcm, inClose + f - T, k, k, o ) );
            PROBE_SMA_Block( hKcm, inClose + f + 1, b - f, o + 1 );
         }
         else
         {
#if PROBE_RESEED
            /* Mutant: the Keltner centre handle re-opened at every block. */
            double z;
            TA_SMA_Close( hKcm );
            hKcm = NULL;
            XBLK_OK( TA_SMA_Open( &hKcm, inClose + (f - 1) - T, k, k, &z ) );
#endif
            PROBE_SMA_Block( hKcm, inClose + f, b - f + 1, o );
         }
      }
      else
         sma_blk( &kcm, inClose, 0, k, f, b, kcmPad + (f - padBase) );
      for( t = f; t <= b; t++ )
         devPad[t - padBase] = inClose[t] - (raw[t - g] + kcmPad[t - padBase]) / 2.0;

      if( mask & X_TR )
      {
         double *o = trPad + (f - padBase);
         if( first )
         {
            XBLK_OK( TA_TRANGE_Open( &hTr, inHigh + f - 1, inLow + f - 1, inClose + f - 1, 2, o ) );
            PROBE_TRANGE_Block( hTr, inHigh + f + 1, inLow + f + 1, inClose + f + 1, b - f, o + 1 );
         }
         else
            PROBE_TRANGE_Block( hTr, inHigh + f, inLow + f, inClose + f, b - f + 1, o );
      }
      else
      {
         retCode = TA_TRANGE( f, b, inHigh, inLow, inClose, &ob, &on, trPad + (f - padBase) );
         if( retCode == TA_SUCCESS && (ob != f || on != b - f + 1) )
            retCode = TA_INTERNAL_ERROR(31);
         if( retCode != TA_SUCCESS )
            goto fail;
      }
      if( mask & X_BAND )
      {
         if( first )
         {
            XBLK_OK( TA_SMA_Open( &hBand, trPad, k, k, bandPad ) );
            PROBE_SMA_Block( hBand, trPad + T + 1, len - 1, bandPad + 1 );
         }
         else
            PROBE_SMA_Block( hBand, trPad + T, len, bandPad );
      }
      else
         sma_blk( &bandS, trPad, padBase, k, a, b, bandPad );
      if( mask & X_MID )
      {
         if( first )
         {
            XBLK_OK( TA_SMA_Open( &hMid, inClose + a - (bb - 1), bb, bb, midPad ) );
            PROBE_SMA_Block( hMid, inClose + a + 1, len - 1, midPad + 1 );
         }
         else
            PROBE_SMA_Block( hMid, inClose + a, len, midPad );
      }
      else
         sma_blk( &mid, inClose, 0, bb, a, b, midPad );
      if( mask & X_VAR )
      {
         if( first )
         {
            XBLK_OK( TA_VAR_Open( &hVar, inClose + a - (bb - 1), bb, bb, 1.0, sdPad ) );
            PROBE_VAR_Block( hVar, inClose + a + 1, len - 1, sdPad + 1 );
         }
         else
            PROBE_VAR_Block( hVar, inClose + a, len, sdPad );
      }
      else
         var_blk( &var, inClose, 0, bb, a, b, sdPad );
      for( i = 0; i < len; i += 1 )
         sdPad[i] = sqrt(sdPad[i]);
      if( mask & X_LR )
      {
         if( first )
         {
            XBLK_OK( TA_LINEARREG_Open( &hLr, devPad, k, k, mom ) );
            PROBE_LINEARREG_Block( hLr, devPad + T + 1, len - 1, mom + 1 );
         }
         else
            PROBE_LINEARREG_Block( hLr, devPad + T, len, mom );
      }
      else
         lr_blk( &lr, devPad, padBase, k, a, b, mom );

      for( i = 0; i < len; i += 1 )
      {
         dev = sdPad[i] * optInNbDev;
         up = midPad[i] + dev;
         lo = midPad[i] - dev;
         centre = kcmPad[i + T];
         band = bandPad[i];
         outSqueeze[a - startIdx + i] = SQZ_LEVEL( lo, up, centre, band, optInFactorWide, optInFactorNormal, optInFactorNarrow );
      }
      first = 0;
      a = b + 1;
   }
fail:
#undef XBLK_OK
   TA_SMA_Close( hKcm );
   TA_SMA_Close( hBand );
   TA_SMA_Close( hMid );
   TA_VAR_Close( hVar );
   TA_LINEARREG_Close( hLr );
   TA_MIDPRICE_Close( hMp );
   TA_TRANGE_Close( hTr );
   free( mem );
   if( retCode != TA_SUCCESS )
      return retCode;
   *outBegIdx = startIdx;
   *outNBElement = endIdx - startIdx + 1;
   return TA_SUCCESS;
}

/* ---- five / one: the generated composition --------------------------- */

/* The generated argument checks and range clamp, verbatim. */
#define SQZ_VALIDATE() \
   if( (startIdx < 0) || (startIdx > TA_INDEX_MAX) ) \
      return TA_OUT_OF_RANGE_START_INDEX; \
   if( (endIdx < 0) || (endIdx > TA_INDEX_MAX) || (endIdx < startIdx) ) \
      return TA_OUT_OF_RANGE_END_INDEX; \
   if( (int)optInBBPeriod == TA_INTEGER_DEFAULT ) \
      optInBBPeriod = 20; \
   else if( (int)optInBBPeriod < 2 || (int)optInBBPeriod > 100000 ) \
      return TA_BAD_PARAM; \
   if( optInNbDev == TA_REAL_DEFAULT ) \
      optInNbDev = 2; \
   else if( !(optInNbDev >= 0e0 && optInNbDev <= TA_REAL_MAX) ) \
      return TA_BAD_PARAM; \
   if( (int)optInKCPeriod == TA_INTEGER_DEFAULT ) \
      optInKCPeriod = 20; \
   else if( (int)optInKCPeriod < 2 || (int)optInKCPeriod > 100000 ) \
      return TA_BAD_PARAM; \
   if( optInFactorWide == TA_REAL_DEFAULT ) \
      optInFactorWide = 2; \
   else if( !(optInFactorWide >= 0e0 && optInFactorWide <= TA_REAL_MAX) ) \
      return TA_BAD_PARAM; \
   if( optInFactorNormal == TA_REAL_DEFAULT ) \
      optInFactorNormal = 1.5; \
   else if( !(optInFactorNormal >= 0e0 && optInFactorNormal <= TA_REAL_MAX) ) \
      return TA_BAD_PARAM; \
   if( optInFactorNarrow == TA_REAL_DEFAULT ) \
      optInFactorNarrow = 1; \
   else if( !(optInFactorNarrow >= 0e0 && optInFactorNarrow <= TA_REAL_MAX) ) \
      return TA_BAD_PARAM; \
   if( !inHigh || !inLow || !inClose || !outBegIdx || !outNBElement || !outMomentum || !outSqueeze ) \
      return TA_BAD_PARAM; \
   if( (const void *)outMomentum == (const void *)outSqueeze ) \
      return TA_BAD_PARAM; \
   if( optInFactorNormal > optInFactorWide || optInFactorNarrow > optInFactorNormal ) \
   { \
      return TA_BAD_PARAM; \
   } \
   lookbackTotal = TA_SQZMOM_Lookback(optInBBPeriod,optInNbDev,optInKCPeriod,optInFactorWide,optInFactorNormal,optInFactorNarrow); \
   if( lookbackTotal > endIdx ) \
   { \
      *outBegIdx= 0; \
      *outNBElement= 0; \
      return TA_SUCCESS; \
   } \
   if( startIdx < lookbackTotal ) \
   { \
      startIdx = lookbackTotal; \
   } \
   if( startIdx > endIdx ) \
   { \
      *outBegIdx= 0; \
      *outNBElement= 0; \
      return TA_SUCCESS; \
   }

#define FIVE_FREE() do { free(tempDev); free(tempWork); free(tempKCM); free(tempMid); free(tempSD); } while( 0 )
#define FIVE_FAIL( rc ) do { FIVE_FREE(); *outBegIdx= 0; *outNBElement= 0; return (rc); } while( 0 )

static TA_RetCode sqzmom_five( int startIdx, int endIdx,
                               const double inHigh[], const double inLow[], const double inClose[],
                               int optInBBPeriod, double optInNbDev, int optInKCPeriod,
                               double optInFactorWide, double optInFactorNormal, double optInFactorNarrow,
                               int blockLen,
                               int *outBegIdx, int *outNBElement,
                               double outMomentum[], int outSqueeze[] )
{
   TA_RetCode retCode;
   int lookbackTotal, stageLookback, stageIdx, stageLen, i, n;
   int tempBegIdx, tempNbElement, kcmNb, sdNb, kcmShift;
   double *tempDev, *tempWork, *tempKCM, *tempMid, *tempSD;
   double dev, up, lo, centre, band;

   (void)blockLen;
   SQZ_VALIDATE()
   n = endIdx - startIdx + 1;
   stageLookback = TA_LINEARREG_Lookback(optInKCPeriod);
   stageIdx = startIdx - stageLookback;
   stageLen = n + stageLookback;
   tempDev = malloc(stageLen * sizeof(double));
   if( !tempDev )
   {
      *outBegIdx= 0;
      *outNBElement= 0;
      return TA_ALLOC_ERR;
   }
   tempWork = malloc(stageLen * sizeof(double));
   if( !tempWork )
   {
      free(tempDev);
      *outBegIdx= 0;
      *outNBElement= 0;
      return TA_ALLOC_ERR;
   }
   tempKCM = malloc(stageLen * sizeof(double));
   if( !tempKCM )
   {
      free(tempDev);
      free(tempWork);
      *outBegIdx= 0;
      *outNBElement= 0;
      return TA_ALLOC_ERR;
   }
   tempMid = malloc(n * sizeof(double));
   if( !tempMid )
   {
      free(tempDev);
      free(tempWork);
      free(tempKCM);
      *outBegIdx= 0;
      *outNBElement= 0;
      return TA_ALLOC_ERR;
   }
   tempSD = malloc(n * sizeof(double));
   if( !tempSD )
   {
      free(tempDev);
      free(tempWork);
      free(tempKCM);
      free(tempMid);
      *outBegIdx= 0;
      *outNBElement= 0;
      return TA_ALLOC_ERR;
   }
   retCode = TA_MIDPRICE(stageIdx,endIdx,inHigh,inLow,optInKCPeriod,outBegIdx,outNBElement,tempDev);
   if( retCode != TA_SUCCESS ) FIVE_FAIL( retCode );
   retCode = TA_SMA(stageIdx,endIdx,inClose,optInKCPeriod,&tempBegIdx,&kcmNb,tempKCM);
   if( retCode != TA_SUCCESS ) FIVE_FAIL( retCode );
   for( i = 0; i < stageLen; i += 1 )
   {
      tempDev[i] = inClose[*outBegIdx + i] - (tempDev[i] + tempKCM[i]) / 2.0;
   }
   retCode = TA_TRANGE(stageIdx,endIdx,inHigh,inLow,inClose,&tempBegIdx,&tempNbElement,tempWork);
   if( retCode != TA_SUCCESS ) FIVE_FAIL( retCode );
   retCode = TA_SMA(0,stageLen - 1,tempWork,optInKCPeriod,&tempBegIdx,&tempNbElement,tempWork);
   if( retCode != TA_SUCCESS ) FIVE_FAIL( retCode );
   retCode = TA_SMA(startIdx,endIdx,inClose,optInBBPeriod,&tempBegIdx,&tempNbElement,tempMid);
   if( retCode != TA_SUCCESS ) FIVE_FAIL( retCode );
   retCode = TA_STDDEV(startIdx,endIdx,inClose,optInBBPeriod,1.0,&tempBegIdx,&sdNb,tempSD);
   if( retCode != TA_SUCCESS ) FIVE_FAIL( retCode );
   kcmShift = kcmNb - sdNb;
   retCode = TA_LINEARREG(0,stageLen - 1,tempDev,optInKCPeriod,&tempBegIdx,&tempNbElement,outMomentum);
   if( retCode != TA_SUCCESS ) FIVE_FAIL( retCode );
   for( i = 0; i < n; i += 1 )
   {
      dev = tempSD[i] * optInNbDev;
      up = tempMid[i] + dev;
      lo = tempMid[i] - dev;
      centre = tempKCM[i + kcmShift];
      band = tempWork[i];
      outSqueeze[i] = SQZ_LEVEL( lo, up, centre, band, optInFactorWide, optInFactorNormal, optInFactorNarrow );
   }
   FIVE_FREE();
   *outBegIdx= startIdx;
   *outNBElement= n;
   return TA_SUCCESS;
}

#define ONE_FAIL( rc ) do { free(mem); *outBegIdx= 0; *outNBElement= 0; return (rc); } while( 0 )

static TA_RetCode sqzmom_one( int startIdx, int endIdx,
                              const double inHigh[], const double inLow[], const double inClose[],
                              int optInBBPeriod, double optInNbDev, int optInKCPeriod,
                              double optInFactorWide, double optInFactorNormal, double optInFactorNarrow,
                              int blockLen,
                              int *outBegIdx, int *outNBElement,
                              double outMomentum[], int outSqueeze[] )
{
   TA_RetCode retCode;
   int lookbackTotal, stageLookback, stageIdx, stageLen, i, n;
   int tempBegIdx, tempNbElement, kcmNb, sdNb, kcmShift;
   double *mem, *tempDev, *tempWork, *tempKCM, *tempMid, *tempSD;
   double dev, up, lo, centre, band;

   (void)blockLen;
   SQZ_VALIDATE()
   n = endIdx - startIdx + 1;
   stageLookback = TA_LINEARREG_Lookback(optInKCPeriod);
   stageIdx = startIdx - stageLookback;
   stageLen = n + stageLookback;
   mem = malloc((3 * (size_t)stageLen + 2 * (size_t)n) * sizeof(double));
   if( !mem )
   {
      *outBegIdx= 0;
      *outNBElement= 0;
      return TA_ALLOC_ERR;
   }
   tempDev = mem;
   tempWork = tempDev + stageLen;
   tempKCM = tempWork + stageLen;
   tempMid = tempKCM + stageLen;
   tempSD = tempMid + n;
   retCode = TA_MIDPRICE(stageIdx,endIdx,inHigh,inLow,optInKCPeriod,outBegIdx,outNBElement,tempDev);
   if( retCode != TA_SUCCESS ) ONE_FAIL( retCode );
   retCode = TA_SMA(stageIdx,endIdx,inClose,optInKCPeriod,&tempBegIdx,&kcmNb,tempKCM);
   if( retCode != TA_SUCCESS ) ONE_FAIL( retCode );
   for( i = 0; i < stageLen; i += 1 )
   {
      tempDev[i] = inClose[*outBegIdx + i] - (tempDev[i] + tempKCM[i]) / 2.0;
   }
   retCode = TA_TRANGE(stageIdx,endIdx,inHigh,inLow,inClose,&tempBegIdx,&tempNbElement,tempWork);
   if( retCode != TA_SUCCESS ) ONE_FAIL( retCode );
   retCode = TA_SMA(0,stageLen - 1,tempWork,optInKCPeriod,&tempBegIdx,&tempNbElement,tempWork);
   if( retCode != TA_SUCCESS ) ONE_FAIL( retCode );
   retCode = TA_SMA(startIdx,endIdx,inClose,optInBBPeriod,&tempBegIdx,&tempNbElement,tempMid);
   if( retCode != TA_SUCCESS ) ONE_FAIL( retCode );
   retCode = TA_STDDEV(startIdx,endIdx,inClose,optInBBPeriod,1.0,&tempBegIdx,&sdNb,tempSD);
   if( retCode != TA_SUCCESS ) ONE_FAIL( retCode );
   kcmShift = kcmNb - sdNb;
   retCode = TA_LINEARREG(0,stageLen - 1,tempDev,optInKCPeriod,&tempBegIdx,&tempNbElement,outMomentum);
   if( retCode != TA_SUCCESS ) ONE_FAIL( retCode );
   for( i = 0; i < n; i += 1 )
   {
      dev = tempSD[i] * optInNbDev;
      up = tempMid[i] + dev;
      lo = tempMid[i] - dev;
      centre = tempKCM[i + kcmShift];
      band = tempWork[i];
      outSqueeze[i] = SQZ_LEVEL( lo, up, centre, band, optInFactorWide, optInFactorNormal, optInFactorNarrow );
   }
   free(mem);
   *outBegIdx= startIdx;
   *outNBElement= n;
   return TA_SUCCESS;
}

/* ---- strm: the public composed stream -------------------------------- */

static TA_RetCode sqzmom_strm( int startIdx, int endIdx,
                               const double inHigh[], const double inLow[], const double inClose[],
                               int optInBBPeriod, double optInNbDev, int optInKCPeriod,
                               double optInFactorWide, double optInFactorNormal, double optInFactorNarrow,
                               int blockLen,
                               int *outBegIdx, int *outNBElement,
                               double outMomentum[], int outSqueeze[] )
{
   TA_RetCode retCode;
   TA_SQZMOM_Stream *h = NULL;
   int lookbackTotal, base, ob, on, t;

   (void)blockLen;
   SQZ_VALIDATE()
   /* History of exactly lookbackTotal+1 bars: its one output is bar startIdx. */
   base = startIdx - lookbackTotal;
   retCode = TA_SQZMOM_OpenAndFill( &h, inHigh + base, inLow + base, inClose + base, lookbackTotal + 1,
                                    optInBBPeriod, optInNbDev, optInKCPeriod,
                                    optInFactorWide, optInFactorNormal, optInFactorNarrow,
                                    &ob, &on, outMomentum, outSqueeze );
   if( retCode != TA_SUCCESS || ob != lookbackTotal || on != 1 )
   {
      TA_SQZMOM_Close( h );
      *outBegIdx = 0;
      *outNBElement = 0;
      return retCode != TA_SUCCESS ? retCode : TA_INTERNAL_ERROR(3);
   }
   for( t = startIdx + 1; t <= endIdx; t++ )
   {
      retCode = TA_SQZMOM_Update( h, inHigh[t], inLow[t], inClose[t],
                                  &outMomentum[t - startIdx], &outSqueeze[t - startIdx] );
      if( retCode != TA_SUCCESS )
      {
         TA_SQZMOM_Close( h );
         *outBegIdx = 0;
         *outNBElement = 0;
         return retCode;
      }
   }
   TA_SQZMOM_Close( h );
   *outBegIdx = startIdx;
   *outNBElement = endIdx - startIdx + 1;
   return TA_SUCCESS;
}

/* ---- sblk: blocked, carried by the sub-functions' stream handles ------ */

/* Each handle is opened so that the bar it last consumed is startIdx and its
 * rolling state began where the batch sub-call's does (TA_SQZMOM_OpenBody):
 *   MIDPRICE(k), SMA(k) close, TRANGE   start at stageIdx = startIdx-(k-1)
 *   SMA(k) over TR, LINEARREG(k) over DEV  first output at startIdx, their
 *                                          window [stageIdx, startIdx]
 *   SMA(b), STDDEV(b) close              start at startIdx
 * The head arrays are period-sized: the k MID/KCM/TR/DEV values the TR-SMA
 * and LINEARREG handles take as history. */
static TA_RetCode sqzmom_sblk( int startIdx, int endIdx,
                               const double inHigh[], const double inLow[], const double inClose[],
                               int optInBBPeriod, double optInNbDev, int optInKCPeriod,
                               double optInFactorWide, double optInFactorNormal, double optInFactorNarrow,
                               int blockLen,
                               int *outBegIdx, int *outNBElement,
                               double outMomentum[], int outSqueeze[] )
{
   TA_RetCode retCode;
   int lookbackTotal, T, k, bb, stageIdx, n, P, a, e, len, i, ob, on, bad;
   double *mem, *headMid, *headKcm, *headDev, *headTr;
   double *midPad, *kcmPad, *devPad, *trPad, *bandPad, *bbPad, *sdPad;
   double band0, mid0, sd0, dev, up, lo, centre, band;
   TA_MIDPRICE_Stream *hMid = NULL;
   TA_SMA_Stream *hKcm = NULL, *hBand = NULL, *hBB = NULL;
   TA_TRANGE_Stream *hTr = NULL;
   TA_STDDEV_Stream *hSd = NULL;
   TA_LINEARREG_Stream *hLr = NULL;

   SQZ_VALIDATE()
   k = optInKCPeriod;
   bb = optInBBPeriod;
   T = k - 1;
   stageIdx = startIdx - T;
   n = endIdx - startIdx + 1;
   P = blockLen;
   if( P > n - 1 )
      P = n - 1;
   if( P < 1 )
      P = 1;
   mem = malloc( (size_t)(4 * k + 7 * P) * sizeof(double) );
   if( !mem )
   {
      *outBegIdx = 0;
      *outNBElement = 0;
      return TA_ALLOC_ERR;
   }
   headMid = mem;
   headKcm = headMid + k;
   headDev = headKcm + k;
   headTr = headDev + k;
   midPad = headTr + k;
   kcmPad = midPad + P;
   devPad = kcmPad + P;
   trPad = devPad + P;
   bandPad = trPad + P;
   bbPad = bandPad + P;
   sdPad = bbPad + P;

#define SBLK_CHECK( cond, rc ) \
   do { if( !(cond) ) { retCode = (rc); goto fail; } } while( 0 )

   retCode = TA_MIDPRICE_OpenAndFill( &hMid, inHigh + stageIdx - T, inLow + stageIdx - T, 2 * T + 1, k, &ob, &on, headMid );
   SBLK_CHECK( retCode == TA_SUCCESS, retCode );
   SBLK_CHECK( on == k, TA_INTERNAL_ERROR(10) );
   retCode = TA_SMA_OpenAndFill( &hKcm, inClose + stageIdx - T, 2 * T + 1, k, &ob, &on, headKcm );
   SBLK_CHECK( retCode == TA_SUCCESS, retCode );
   SBLK_CHECK( on == k, TA_INTERNAL_ERROR(11) );
   for( i = 0; i < k; i += 1 )
      headDev[i] = inClose[stageIdx + i] - (headMid[i] + headKcm[i]) / 2.0;
   retCode = TA_TRANGE_OpenAndFill( &hTr, inHigh + stageIdx - 1, inLow + stageIdx - 1, inClose + stageIdx - 1, k + 1, &ob, &on, headTr );
   SBLK_CHECK( retCode == TA_SUCCESS, retCode );
   SBLK_CHECK( on == k, TA_INTERNAL_ERROR(12) );
   retCode = TA_SMA_Open( &hBand, headTr, k, k, &band0 );
   SBLK_CHECK( retCode == TA_SUCCESS, retCode );
   retCode = TA_SMA_Open( &hBB, inClose + startIdx - (bb - 1), bb, bb, &mid0 );
   SBLK_CHECK( retCode == TA_SUCCESS, retCode );
   retCode = TA_STDDEV_Open( &hSd, inClose + startIdx - (bb - 1), bb, bb, 1.0, &sd0 );
   SBLK_CHECK( retCode == TA_SUCCESS, retCode );
   retCode = TA_LINEARREG_Open( &hLr, headDev, k, k, &outMomentum[0] );
   SBLK_CHECK( retCode == TA_SUCCESS, retCode );
   dev = sd0 * optInNbDev;
   up = mid0 + dev;
   lo = mid0 - dev;
   centre = headKcm[T];
   band = band0;
   outSqueeze[0] = SQZ_LEVEL( lo, up, centre, band, optInFactorWide, optInFactorNormal, optInFactorNarrow );

   bad = 0;
   for( a = startIdx + 1; a <= endIdx; a += P )
   {
      const double *h, *l, *c;
      e = a + P - 1;
      if( e > endIdx )
         e = endIdx;
      len = e - a + 1;
      h = inHigh + a;
      l = inLow + a;
      c = inClose + a;
      probeEdges++;
#if PROBE_RESEED
      /* Mutant: the Keltner centre re-seeded at every block, a correct value
       * on a different rounding path. It must be one on the momentum path:
       * a last-bit change in a band mean almost never moves the ordinal. */
      TA_SMA_Close( hKcm );
      hKcm = NULL;
      retCode = TA_SMA_Open( &hKcm, inClose + (a - 1) - T, k, k, &band0 );
      SBLK_CHECK( retCode == TA_SUCCESS, retCode );
#endif
      for( i = 0; i < len; i += 1 )
         bad |= TA_MIDPRICE_Update( hMid, h[i], l[i], &midPad[i] ) != TA_SUCCESS;
      for( i = 0; i < len; i += 1 )
         bad |= TA_SMA_Update( hKcm, c[i], &kcmPad[i] ) != TA_SUCCESS;
      for( i = 0; i < len; i += 1 )
         devPad[i] = c[i] - (midPad[i] + kcmPad[i]) / 2.0;
      for( i = 0; i < len; i += 1 )
         bad |= TA_TRANGE_Update( hTr, h[i], l[i], c[i], &trPad[i] ) != TA_SUCCESS;
      for( i = 0; i < len; i += 1 )
         bad |= TA_SMA_Update( hBand, trPad[i], &bandPad[i] ) != TA_SUCCESS;
      for( i = 0; i < len; i += 1 )
         bad |= TA_SMA_Update( hBB, c[i], &bbPad[i] ) != TA_SUCCESS;
      for( i = 0; i < len; i += 1 )
         bad |= TA_STDDEV_Update( hSd, c[i], &sdPad[i] ) != TA_SUCCESS;
      for( i = 0; i < len; i += 1 )
         bad |= TA_LINEARREG_Update( hLr, devPad[i], &outMomentum[a - startIdx + i] ) != TA_SUCCESS;
      SBLK_CHECK( !bad, TA_BAD_PARAM );
      for( i = 0; i < len; i += 1 )
      {
         dev = sdPad[i] * optInNbDev;
         up = bbPad[i] + dev;
         lo = bbPad[i] - dev;
         centre = kcmPad[i];
         band = bandPad[i];
         outSqueeze[a - startIdx + i] = SQZ_LEVEL( lo, up, centre, band, optInFactorWide, optInFactorNormal, optInFactorNarrow );
      }
   }
   retCode = TA_SUCCESS;
fail:
#undef SBLK_CHECK
   TA_MIDPRICE_Close( hMid );
   TA_SMA_Close( hKcm );
   TA_TRANGE_Close( hTr );
   TA_SMA_Close( hBand );
   TA_SMA_Close( hBB );
   TA_STDDEV_Close( hSd );
   TA_LINEARREG_Close( hLr );
   free( mem );
   if( retCode != TA_SUCCESS )
   {
      *outBegIdx = 0;
      *outNBElement = 0;
      return retCode;
   }
   *outBegIdx = startIdx;
   *outNBElement = n;
   return TA_SUCCESS;
}

/* ---- iblk: sblk with each per-block Update loop one block call -------- */

/* sblk line for line, except that each `for(...) TA_X_Update(...)` is one
 * PROBE_X_Block: a loop over TA_X_StepImpl inside ta_X.c's own translation
 * unit (inl_x.c). viaVar takes STDDEV through PROBE_STDDEV_BlockSub. */
static TA_RetCode sqzmom_iblk_body( int startIdx, int endIdx,
                                    const double inHigh[], const double inLow[], const double inClose[],
                                    int optInBBPeriod, double optInNbDev, int optInKCPeriod,
                                    double optInFactorWide, double optInFactorNormal, double optInFactorNarrow,
                                    int blockLen,
                                    int *outBegIdx, int *outNBElement,
                                    double outMomentum[], int outSqueeze[], int viaVar )
{
   TA_RetCode retCode;
   int lookbackTotal, T, k, bb, stageIdx, n, P, a, e, len, i, ob, on;
   double *mem, *headMid, *headKcm, *headDev, *headTr;
   double *midPad, *kcmPad, *devPad, *trPad, *bandPad, *bbPad, *sdPad;
   double band0, mid0, sd0, dev, up, lo, centre, band;
   TA_MIDPRICE_Stream *hMid = NULL;
   TA_SMA_Stream *hKcm = NULL, *hBand = NULL, *hBB = NULL;
   TA_TRANGE_Stream *hTr = NULL;
   TA_STDDEV_Stream *hSd = NULL;
   TA_LINEARREG_Stream *hLr = NULL;

   SQZ_VALIDATE()
   k = optInKCPeriod;
   bb = optInBBPeriod;
   T = k - 1;
   stageIdx = startIdx - T;
   n = endIdx - startIdx + 1;
   P = blockLen;
   if( P > n - 1 )
      P = n - 1;
   if( P < 1 )
      P = 1;
   mem = malloc( (size_t)(4 * k + 7 * P) * sizeof(double) );
   if( !mem )
   {
      *outBegIdx = 0;
      *outNBElement = 0;
      return TA_ALLOC_ERR;
   }
   headMid = mem;
   headKcm = headMid + k;
   headDev = headKcm + k;
   headTr = headDev + k;
   midPad = headTr + k;
   kcmPad = midPad + P;
   devPad = kcmPad + P;
   trPad = devPad + P;
   bandPad = trPad + P;
   bbPad = bandPad + P;
   sdPad = bbPad + P;

#define IBLK_CHECK( cond, rc ) \
   do { if( !(cond) ) { retCode = (rc); goto fail; } } while( 0 )

   retCode = TA_MIDPRICE_OpenAndFill( &hMid, inHigh + stageIdx - T, inLow + stageIdx - T, 2 * T + 1, k, &ob, &on, headMid );
   IBLK_CHECK( retCode == TA_SUCCESS, retCode );
   IBLK_CHECK( on == k, TA_INTERNAL_ERROR(10) );
   retCode = TA_SMA_OpenAndFill( &hKcm, inClose + stageIdx - T, 2 * T + 1, k, &ob, &on, headKcm );
   IBLK_CHECK( retCode == TA_SUCCESS, retCode );
   IBLK_CHECK( on == k, TA_INTERNAL_ERROR(11) );
   for( i = 0; i < k; i += 1 )
      headDev[i] = inClose[stageIdx + i] - (headMid[i] + headKcm[i]) / 2.0;
   retCode = TA_TRANGE_OpenAndFill( &hTr, inHigh + stageIdx - 1, inLow + stageIdx - 1, inClose + stageIdx - 1, k + 1, &ob, &on, headTr );
   IBLK_CHECK( retCode == TA_SUCCESS, retCode );
   IBLK_CHECK( on == k, TA_INTERNAL_ERROR(12) );
   retCode = TA_SMA_Open( &hBand, headTr, k, k, &band0 );
   IBLK_CHECK( retCode == TA_SUCCESS, retCode );
   retCode = TA_SMA_Open( &hBB, inClose + startIdx - (bb - 1), bb, bb, &mid0 );
   IBLK_CHECK( retCode == TA_SUCCESS, retCode );
   retCode = TA_STDDEV_Open( &hSd, inClose + startIdx - (bb - 1), bb, bb, 1.0, &sd0 );
   IBLK_CHECK( retCode == TA_SUCCESS, retCode );
   retCode = TA_LINEARREG_Open( &hLr, headDev, k, k, &outMomentum[0] );
   IBLK_CHECK( retCode == TA_SUCCESS, retCode );
   dev = sd0 * optInNbDev;
   up = mid0 + dev;
   lo = mid0 - dev;
   centre = headKcm[T];
   band = band0;
   outSqueeze[0] = SQZ_LEVEL( lo, up, centre, band, optInFactorWide, optInFactorNormal, optInFactorNarrow );

   for( a = startIdx + 1; a <= endIdx; a += P )
   {
      const double *h, *l, *c;
      e = a + P - 1;
      if( e > endIdx )
         e = endIdx;
      len = e - a + 1;
      h = inHigh + a;
      l = inLow + a;
      c = inClose + a;
      probeEdges++;
#if PROBE_RESEED
      /* Mutant, as in sblk: the Keltner centre handle re-opened at every block. */
      TA_SMA_Close( hKcm );
      hKcm = NULL;
      retCode = TA_SMA_Open( &hKcm, inClose + (a - 1) - T, k, k, &band0 );
      IBLK_CHECK( retCode == TA_SUCCESS, retCode );
#endif
      PROBE_MIDPRICE_Block( hMid, h, l, len, midPad );
      PROBE_SMA_Block( hKcm, c, len, kcmPad );
      for( i = 0; i < len; i += 1 )
         devPad[i] = c[i] - (midPad[i] + kcmPad[i]) / 2.0;
      PROBE_TRANGE_Block( hTr, h, l, c, len, trPad );
      PROBE_SMA_Block( hBand, trPad, len, bandPad );
      PROBE_SMA_Block( hBB, c, len, bbPad );
      if( viaVar )
         PROBE_STDDEV_BlockSub( hSd, c, len, sdPad );
      else
      {
         retCode = PROBE_STDDEV_Block( hSd, c, len, sdPad );
         IBLK_CHECK( retCode == TA_SUCCESS, retCode );
      }
      PROBE_LINEARREG_Block( hLr, devPad, len, &outMomentum[a - startIdx] );
      for( i = 0; i < len; i += 1 )
      {
         dev = sdPad[i] * optInNbDev;
         up = bbPad[i] + dev;
         lo = bbPad[i] - dev;
         centre = kcmPad[i];
         band = bandPad[i];
         outSqueeze[a - startIdx + i] = SQZ_LEVEL( lo, up, centre, band, optInFactorWide, optInFactorNormal, optInFactorNarrow );
      }
   }
   retCode = TA_SUCCESS;
fail:
#undef IBLK_CHECK
   TA_MIDPRICE_Close( hMid );
   TA_SMA_Close( hKcm );
   TA_TRANGE_Close( hTr );
   TA_SMA_Close( hBand );
   TA_SMA_Close( hBB );
   TA_STDDEV_Close( hSd );
   TA_LINEARREG_Close( hLr );
   free( mem );
   if( retCode != TA_SUCCESS )
   {
      *outBegIdx = 0;
      *outNBElement = 0;
      return retCode;
   }
   *outBegIdx = startIdx;
   *outNBElement = n;
   return TA_SUCCESS;
}

static TA_RetCode sqzmom_iblk( int startIdx, int endIdx,
                               const double inHigh[], const double inLow[], const double inClose[],
                               int optInBBPeriod, double optInNbDev, int optInKCPeriod,
                               double optInFactorWide, double optInFactorNormal, double optInFactorNarrow,
                               int blockLen,
                               int *outBegIdx, int *outNBElement,
                               double outMomentum[], int outSqueeze[] )
{
   return sqzmom_iblk_body( startIdx, endIdx, inHigh, inLow, inClose, optInBBPeriod, optInNbDev, optInKCPeriod,
                            optInFactorWide, optInFactorNormal, optInFactorNarrow, blockLen,
                            outBegIdx, outNBElement, outMomentum, outSqueeze, 0 );
}

static TA_RetCode sqzmom_iblkv( int startIdx, int endIdx,
                                const double inHigh[], const double inLow[], const double inClose[],
                                int optInBBPeriod, double optInNbDev, int optInKCPeriod,
                                double optInFactorWide, double optInFactorNormal, double optInFactorNarrow,
                                int blockLen,
                                int *outBegIdx, int *outNBElement,
                                double outMomentum[], int outSqueeze[] )
{
   return sqzmom_iblk_body( startIdx, endIdx, inHigh, inLow, inClose, optInBBPeriod, optInNbDev, optInKCPeriod,
                            optInFactorWide, optInFactorNormal, optInFactorNarrow, blockLen,
                            outBegIdx, outNBElement, outMomentum, outSqueeze, 1 );
}

/* ---- hblk: blk's batch MIDPRICE and TRANGE, iblkv's step blocks -------- */

/* iblkv with the two handles whose batch tier is cheaper dropped: MIDPRICE is
 * the public batch call restarted on stageIdx's period grid per block, as in
 * blk (its head too), and TRANGE the public batch call on the block. */
static TA_RetCode sqzmom_hblk( int startIdx, int endIdx,
                               const double inHigh[], const double inLow[], const double inClose[],
                               int optInBBPeriod, double optInNbDev, int optInKCPeriod,
                               double optInFactorWide, double optInFactorNormal, double optInFactorNarrow,
                               int blockLen,
                               int *outBegIdx, int *outNBElement,
                               double outMomentum[], int outSqueeze[] )
{
   TA_RetCode retCode;
   int lookbackTotal, T, k, bb, stageIdx, n, P, a, e, g, len, i, ob, on;
   double *mem, *headMid, *headKcm, *headDev, *headTr, *raw;
   double *kcmPad, *devPad, *trPad, *bandPad, *bbPad, *sdPad;
   const double *midPad;
   double band0, mid0, sd0, dev, up, lo, centre, band;
   TA_SMA_Stream *hKcm = NULL, *hBand = NULL, *hBB = NULL;
   TA_STDDEV_Stream *hSd = NULL;
   TA_LINEARREG_Stream *hLr = NULL;

   SQZ_VALIDATE()
   k = optInKCPeriod;
   bb = optInBBPeriod;
   T = k - 1;
   stageIdx = startIdx - T;
   n = endIdx - startIdx + 1;
   P = blockLen;
   if( P > n - 1 )
      P = n - 1;
   if( P < 1 )
      P = 1;
   mem = malloc( (size_t)(4 * k + (P + T) + 6 * P) * sizeof(double) );
   if( !mem )
   {
      *outBegIdx = 0;
      *outNBElement = 0;
      return TA_ALLOC_ERR;
   }
   headMid = mem;
   headKcm = headMid + k;
   headDev = headKcm + k;
   headTr = headDev + k;
   raw = headTr + k;
   kcmPad = raw + P + T;
   devPad = kcmPad + P;
   trPad = devPad + P;
   bandPad = trPad + P;
   bbPad = bandPad + P;
   sdPad = bbPad + P;

/* Unlikely, or gcc's static guess makes the block loop cold and leaves the
 * dev and squeeze maps scalar. */
#define HBLK_CHECK( cond, rc ) \
   do { if( __builtin_expect( !(cond), 0 ) ) { retCode = (rc); goto fail; } } while( 0 )

   retCode = TA_MIDPRICE( stageIdx, startIdx, inHigh, inLow, k, &ob, &on, headMid );
   HBLK_CHECK( retCode == TA_SUCCESS, retCode );
   HBLK_CHECK( ob == stageIdx && on == k, TA_INTERNAL_ERROR(20) );
   retCode = TA_SMA_OpenAndFill( &hKcm, inClose + stageIdx - T, 2 * T + 1, k, &ob, &on, headKcm );
   HBLK_CHECK( retCode == TA_SUCCESS, retCode );
   HBLK_CHECK( on == k, TA_INTERNAL_ERROR(21) );
   for( i = 0; i < k; i += 1 )
      headDev[i] = inClose[stageIdx + i] - (headMid[i] + headKcm[i]) / 2.0;
   retCode = TA_TRANGE( stageIdx, startIdx, inHigh, inLow, inClose, &ob, &on, headTr );
   HBLK_CHECK( retCode == TA_SUCCESS, retCode );
   HBLK_CHECK( ob == stageIdx && on == k, TA_INTERNAL_ERROR(22) );
   retCode = TA_SMA_Open( &hBand, headTr, k, k, &band0 );
   HBLK_CHECK( retCode == TA_SUCCESS, retCode );
   retCode = TA_SMA_Open( &hBB, inClose + startIdx - (bb - 1), bb, bb, &mid0 );
   HBLK_CHECK( retCode == TA_SUCCESS, retCode );
   retCode = TA_STDDEV_Open( &hSd, inClose + startIdx - (bb - 1), bb, bb, 1.0, &sd0 );
   HBLK_CHECK( retCode == TA_SUCCESS, retCode );
   retCode = TA_LINEARREG_Open( &hLr, headDev, k, k, &outMomentum[0] );
   HBLK_CHECK( retCode == TA_SUCCESS, retCode );
   dev = sd0 * optInNbDev;
   up = mid0 + dev;
   lo = mid0 - dev;
   centre = headKcm[T];
   band = band0;
   outSqueeze[0] = SQZ_LEVEL( lo, up, centre, band, optInFactorWide, optInFactorNormal, optInFactorNarrow );

   for( a = startIdx + 1; a <= endIdx; a += P )
   {
      const double *c;
      e = a + P - 1;
      if( e > endIdx )
         e = endIdx;
      len = e - a + 1;
      c = inClose + a;
      probeEdges++;
#if PROBE_RESEED
      /* Mutant, as in sblk: the Keltner centre handle re-opened at every block. */
      TA_SMA_Close( hKcm );
      hKcm = NULL;
      retCode = TA_SMA_Open( &hKcm, inClose + (a - 1) - T, k, k, &band0 );
      HBLK_CHECK( retCode == TA_SUCCESS, retCode );
#endif
      g = a - ((a - stageIdx) % k);
      retCode = TA_MIDPRICE( g, e, inHigh, inLow, k, &ob, &on, raw );
      HBLK_CHECK( retCode == TA_SUCCESS, retCode );
      HBLK_CHECK( ob == g && on == e - g + 1, TA_INTERNAL_ERROR(23) );
      midPad = raw + (a - g);
      PROBE_SMA_Block( hKcm, c, len, kcmPad );
      for( i = 0; i < len; i += 1 )
         devPad[i] = c[i] - (midPad[i] + kcmPad[i]) / 2.0;
      retCode = TA_TRANGE( a, e, inHigh, inLow, inClose, &ob, &on, trPad );
      HBLK_CHECK( retCode == TA_SUCCESS, retCode );
      HBLK_CHECK( ob == a && on == len, TA_INTERNAL_ERROR(24) );
      PROBE_SMA_Block( hBand, trPad, len, bandPad );
      PROBE_SMA_Block( hBB, c, len, bbPad );
      PROBE_STDDEV_BlockSub( hSd, c, len, sdPad );
      PROBE_LINEARREG_Block( hLr, devPad, len, &outMomentum[a - startIdx] );
      for( i = 0; i < len; i += 1 )
      {
         dev = sdPad[i] * optInNbDev;
         up = bbPad[i] + dev;
         lo = bbPad[i] - dev;
         centre = kcmPad[i];
         band = bandPad[i];
         outSqueeze[a - startIdx + i] = SQZ_LEVEL( lo, up, centre, band, optInFactorWide, optInFactorNormal, optInFactorNarrow );
      }
   }
   retCode = TA_SUCCESS;
fail:
#undef HBLK_CHECK
   TA_SMA_Close( hKcm );
   TA_SMA_Close( hBand );
   TA_SMA_Close( hBB );
   TA_STDDEV_Close( hSd );
   TA_LINEARREG_Close( hLr );
   free( mem );
   if( retCode != TA_SUCCESS )
   {
      *outBegIdx = 0;
      *outNBElement = 0;
      return retCode;
   }
   *outBegIdx = startIdx;
   *outNBElement = n;
   return TA_SUCCESS;
}

/* ---- kblk: hblk over the mechanical kernels -------------------------- */

/* hblk line for line, with the five step blocks (SMA x3, VAR inside the
 * STDDEV map, LINEARREG) replaced by the non-restrict PROBE_X_Kern. */
static TA_RetCode sqzmom_kblk( int startIdx, int endIdx,
                               const double inHigh[], const double inLow[], const double inClose[],
                               int optInBBPeriod, double optInNbDev, int optInKCPeriod,
                               double optInFactorWide, double optInFactorNormal, double optInFactorNarrow,
                               int blockLen,
                               int *outBegIdx, int *outNBElement,
                               double outMomentum[], int outSqueeze[] )
{
   TA_RetCode retCode;
   int lookbackTotal, T, k, bb, stageIdx, n, P, a, e, g, len, i, ob, on;
   double *mem, *headMid, *headKcm, *headDev, *headTr, *raw;
   double *kcmPad, *devPad, *trPad, *bandPad, *bbPad, *sdPad;
   const double *midPad;
   double band0, mid0, sd0, dev, up, lo, centre, band;
   TA_SMA_Stream *hKcm = NULL, *hBand = NULL, *hBB = NULL;
   TA_STDDEV_Stream *hSd = NULL;
   TA_LINEARREG_Stream *hLr = NULL;

   SQZ_VALIDATE()
   k = optInKCPeriod;
   bb = optInBBPeriod;
   T = k - 1;
   stageIdx = startIdx - T;
   n = endIdx - startIdx + 1;
   P = blockLen;
   if( P > n - 1 )
      P = n - 1;
   if( P < 1 )
      P = 1;
   mem = malloc( (size_t)(4 * k + (P + T) + 6 * P) * sizeof(double) );
   if( !mem )
   {
      *outBegIdx = 0;
      *outNBElement = 0;
      return TA_ALLOC_ERR;
   }
   headMid = mem;
   headKcm = headMid + k;
   headDev = headKcm + k;
   headTr = headDev + k;
   raw = headTr + k;
   kcmPad = raw + P + T;
   devPad = kcmPad + P;
   trPad = devPad + P;
   bandPad = trPad + P;
   bbPad = bandPad + P;
   sdPad = bbPad + P;

/* As HBLK_CHECK. */
#define KBLK_CHECK( cond, rc ) \
   do { if( __builtin_expect( !(cond), 0 ) ) { retCode = (rc); goto fail; } } while( 0 )

   retCode = TA_MIDPRICE( stageIdx, startIdx, inHigh, inLow, k, &ob, &on, headMid );
   KBLK_CHECK( retCode == TA_SUCCESS, retCode );
   KBLK_CHECK( ob == stageIdx && on == k, TA_INTERNAL_ERROR(40) );
   retCode = TA_SMA_OpenAndFill( &hKcm, inClose + stageIdx - T, 2 * T + 1, k, &ob, &on, headKcm );
   KBLK_CHECK( retCode == TA_SUCCESS, retCode );
   KBLK_CHECK( on == k, TA_INTERNAL_ERROR(41) );
   for( i = 0; i < k; i += 1 )
      headDev[i] = inClose[stageIdx + i] - (headMid[i] + headKcm[i]) / 2.0;
   retCode = TA_TRANGE( stageIdx, startIdx, inHigh, inLow, inClose, &ob, &on, headTr );
   KBLK_CHECK( retCode == TA_SUCCESS, retCode );
   KBLK_CHECK( ob == stageIdx && on == k, TA_INTERNAL_ERROR(42) );
   retCode = TA_SMA_Open( &hBand, headTr, k, k, &band0 );
   KBLK_CHECK( retCode == TA_SUCCESS, retCode );
   retCode = TA_SMA_Open( &hBB, inClose + startIdx - (bb - 1), bb, bb, &mid0 );
   KBLK_CHECK( retCode == TA_SUCCESS, retCode );
   retCode = TA_STDDEV_Open( &hSd, inClose + startIdx - (bb - 1), bb, bb, 1.0, &sd0 );
   KBLK_CHECK( retCode == TA_SUCCESS, retCode );
   retCode = TA_LINEARREG_Open( &hLr, headDev, k, k, &outMomentum[0] );
   KBLK_CHECK( retCode == TA_SUCCESS, retCode );
   dev = sd0 * optInNbDev;
   up = mid0 + dev;
   lo = mid0 - dev;
   centre = headKcm[T];
   band = band0;
   outSqueeze[0] = SQZ_LEVEL( lo, up, centre, band, optInFactorWide, optInFactorNormal, optInFactorNarrow );

   for( a = startIdx + 1; a <= endIdx; a += P )
   {
      const double *c;
      e = a + P - 1;
      if( e > endIdx )
         e = endIdx;
      len = e - a + 1;
      c = inClose + a;
      probeEdges++;
#if PROBE_RESEED
      /* Mutant, as in sblk: the Keltner centre handle re-opened at every block. */
      TA_SMA_Close( hKcm );
      hKcm = NULL;
      retCode = TA_SMA_Open( &hKcm, inClose + (a - 1) - T, k, k, &band0 );
      KBLK_CHECK( retCode == TA_SUCCESS, retCode );
#endif
      g = a - ((a - stageIdx) % k);
      retCode = TA_MIDPRICE( g, e, inHigh, inLow, k, &ob, &on, raw );
      KBLK_CHECK( retCode == TA_SUCCESS, retCode );
      KBLK_CHECK( ob == g && on == e - g + 1, TA_INTERNAL_ERROR(43) );
      midPad = raw + (a - g);
      PROBE_SMA_Kern( hKcm, c, len, kcmPad );
      for( i = 0; i < len; i += 1 )
         devPad[i] = c[i] - (midPad[i] + kcmPad[i]) / 2.0;
      retCode = TA_TRANGE( a, e, inHigh, inLow, inClose, &ob, &on, trPad );
      KBLK_CHECK( retCode == TA_SUCCESS, retCode );
      KBLK_CHECK( ob == a && on == len, TA_INTERNAL_ERROR(44) );
      PROBE_SMA_Kern( hBand, trPad, len, bandPad );
      PROBE_SMA_Kern( hBB, c, len, bbPad );
      PROBE_STDDEV_BlockSubK( hSd, c, len, sdPad );
      PROBE_LINEARREG_Kern( hLr, devPad, len, &outMomentum[a - startIdx] );
      for( i = 0; i < len; i += 1 )
      {
         dev = sdPad[i] * optInNbDev;
         up = bbPad[i] + dev;
         lo = bbPad[i] - dev;
         centre = kcmPad[i];
         band = bandPad[i];
         outSqueeze[a - startIdx + i] = SQZ_LEVEL( lo, up, centre, band, optInFactorWide, optInFactorNormal, optInFactorNarrow );
      }
   }
   retCode = TA_SUCCESS;
fail:
#undef KBLK_CHECK
   TA_SMA_Close( hKcm );
   TA_SMA_Close( hBand );
   TA_SMA_Close( hBB );
   TA_STDDEV_Close( hSd );
   TA_LINEARREG_Close( hLr );
   free( mem );
   if( retCode != TA_SUCCESS )
   {
      *outBegIdx = 0;
      *outNBElement = 0;
      return retCode;
   }
   *outBegIdx = startIdx;
   *outNBElement = n;
   return TA_SUCCESS;
}

/* ---- ref: the shipped function in the shared signature ---------------- */

static TA_RetCode sqzmom_ref( int startIdx, int endIdx,
                              const double inHigh[], const double inLow[], const double inClose[],
                              int optInBBPeriod, double optInNbDev, int optInKCPeriod,
                              double optInFactorWide, double optInFactorNormal, double optInFactorNarrow,
                              int blockLen,
                              int *outBegIdx, int *outNBElement,
                              double outMomentum[], int outSqueeze[] )
{
   (void)blockLen;
   return TA_SQZMOM( startIdx, endIdx, inHigh, inLow, inClose, optInBBPeriod, optInNbDev, optInKCPeriod,
                     optInFactorWide, optInFactorNormal, optInFactorNarrow,
                     outBegIdx, outNBElement, outMomentum, outSqueeze );
}

typedef TA_RetCode (*SqzFn)( int, int, const double *, const double *, const double *,
                             int, double, int, double, double, double, int,
                             int *, int *, double *, int * );

typedef struct
{
   const char *name;
   SqzFn fn;
   int blocked;
   long long compared, outputs, edges;
   int fails;
} Variant;

/* Timing order; [0] is the oracle. */
static Variant variants[] = {
   { "ref",  sqzmom_ref,     0, 0, 0, 0, 0 },
   { "blk",  sqzmom_blocked, 1, 0, 0, 0, 0 },
   { "five", sqzmom_five,    0, 0, 0, 0, 0 },
   { "one",  sqzmom_one,     0, 0, 0, 0, 0 },
   { "strm", sqzmom_strm,    0, 0, 0, 0, 0 },
   { "sblk", sqzmom_sblk,    1, 0, 0, 0, 0 },
   { "iblk", sqzmom_iblk,    1, 0, 0, 0, 0 },
   { "iblkv", sqzmom_iblkv,  1, 0, 0, 0, 0 },
   { "hblk", sqzmom_hblk,    1, 0, 0, 0, 0 },
   { "xblk", sqzmom_xblk,    1, 0, 0, 0, 0 },
   { "kblk", sqzmom_kblk,    1, 0, 0, 0, 0 },
};
#define NVAR ((int)(sizeof(variants)/sizeof(variants[0])))
#define FAILCAP 20

/* ---- input series ----------------------------------------------------- */

static unsigned long long rngState;
static double rnd( void )
{
   rngState = rngState * 6364136223846793005ULL + 1442695040888963407ULL;
   return (double)(rngState >> 11) / 9007199254740992.0;
}
static double cents( double x ) { return floor( x * 100.0 + 0.5 ) / 100.0; }

enum { S_WALK, S_PLATEAU, S_SPIKE, S_ZEROS, S_COUNT };
static const char *shapeName[S_COUNT] = { "walk", "plateau", "spike", "zeros" };

/* PROBE_REVERT=r pulls the walk back toward 100 by r per bar. Without it the
 * multiplicative walk drifts down and cents rounding freezes it at 0.50 past
 * about 100,000 bars: a flat series, which is not what a long benchmark means. */
static double walkRevert;

static void makeSeries( int shape, unsigned long long seed, int n,
                        double *high, double *low, double *close )
{
   int i;
   double c = 100.0, o, h, l;
   rngState = seed;
   for( i = 0; i < n; i++ )
   {
      o = c;
      c = c * (1.0 + 0.02 * (rnd() - 0.5));
      c += (100.0 - c) * walkRevert;
      if( shape == S_PLATEAU && (i / 37) % 2 == 1 )
         c = o;
      if( shape == S_SPIKE && i % 53 == 17 )
         c = c * 1000.0;
      else if( shape == S_SPIKE && i % 53 == 18 )
         c = c / 1000.0;
      c = cents( c );
      h = cents( (o > c ? o : c) * (1.0 + 0.003 * rnd()) );
      l = cents( (o < c ? o : c) * (1.0 - 0.003 * rnd()) );
      if( shape == S_PLATEAU && (i / 37) % 2 == 1 )
      {
         h = c;
         l = c;
      }
      high[i] = h;
      low[i] = l;
      close[i] = c;
      if( shape == S_ZEROS )
      {
         /* Stretches of mixed-sign zeros between ordinary bars. */
         if( (i / 29) % 3 != 0 )
         {
            high[i] = (rnd() < 0.5) ? 0.0 : -0.0;
            low[i] = (rnd() < 0.5) ? 0.0 : -0.0;
            close[i] = (rnd() < 0.5) ? 0.0 : -0.0;
         }
         else
         {
            high[i] = h - 100.0 + 0.5;
            low[i] = l - 100.0 - 0.5;
            close[i] = c - 100.0;
         }
      }
   }
}

/* ---- comparison ------------------------------------------------------- */

typedef struct { int b, k; double nbdev, fw, fn, fa; } Params;

/* ref against every variant; B-independent variants only when firstB. */
static int compareAll( const double *high, const double *low, const double *close,
                       int s, int e, const Params *p, int B, int firstB, int bufLen,
                       double *refM, int *refS, double *vM, int *vS, const char *what )
{
   int rb = -7, rn = -7, vi, fails = 0;
   TA_RetCode r1, r2;

   r1 = TA_SQZMOM( s, e, high, low, close, p->b, p->nbdev, p->k, p->fw, p->fn, p->fa, &rb, &rn, refM, refS );
   for( vi = 1; vi < NVAR; vi++ )
   {
      Variant *v = &variants[vi];
      int vb = -7, vn = -7;
      long long e0;
      if( (!v->blocked && !firstB) || v->fails >= FAILCAP )
         continue;
      memset( vM, 0xA5, (size_t)bufLen * sizeof(double) );
      memset( vS, 0x5A, (size_t)bufLen * sizeof(int) );
      e0 = probeEdges;
      r2 = v->fn( s, e, high, low, close, p->b, p->nbdev, p->k, p->fw, p->fn, p->fa, B, &vb, &vn, vM, vS );
      v->compared++;
      v->edges += probeEdges - e0;
      if( r1 != r2 || rb != vb || rn != vn )
      {
         printf( "MISMATCH %s %s b=%d k=%d B=%d [%d,%d]: ret %d/%d beg %d/%d nb %d/%d\n",
                 v->name, what, p->b, p->k, B, s, e, (int)r1, (int)r2, rb, vb, rn, vn );
         v->fails++;
         fails++;
         continue;
      }
      if( r1 != TA_SUCCESS || rn == 0 )
         continue;
      v->outputs += rn;
      if( memcmp( refM, vM, (size_t)rn * sizeof(double) ) != 0 ||
          memcmp( refS, vS, (size_t)rn * sizeof(int) ) != 0 )
      {
         int i;
         for( i = 0; i < rn; i++ )
            if( memcmp( &refM[i], &vM[i], sizeof(double) ) != 0 || refS[i] != vS[i] )
               break;
         printf( "MISMATCH %s %s b=%d k=%d B=%d [%d,%d] at out %d (bar %d): mom %.17g/%.17g sqz %d/%d\n",
                 v->name, what, p->b, p->k, B, s, e, i, rb + i, refM[i], vM[i], refS[i], vS[i] );
         v->fails++;
         fails++;
      }
   }
   return fails;
}

static const Params paramSet[] = {
   { 20, 20, 2.0, 2.0, 1.5, 1.0 },
   {  2,  2, 2.0, 2.0, 1.5, 1.0 },
   {  5,  3, 2.0, 2.0, 1.5, 1.0 },
   {  3,  5, 1.0, 2.0, 2.0, 1.0 },
   { 14, 20, 2.5, 3.0, 1.5, 0.5 },
   { 30, 10, 2.0, 2.0, 1.5, 1.0 },
};
#define NPARAMS ((int)(sizeof(paramSet)/sizeof(paramSet[0])))

/* Every start and end on a short series, every block length that matters. */
static int runCheck( void )
{
   enum { N = 220 };
   static double high[N], low[N], close[N], refM[N], vM[N];
   static int refS[N], vS[N];
   int shape, pi, bi, s, e, fails = 0, nB;
   int Bs[16];

   for( shape = 0; shape < S_COUNT; shape++ )
   {
      makeSeries( shape, 42 + (unsigned)shape, N, high, low, close );
      for( pi = 0; pi < NPARAMS; pi++ )
      {
         const Params *p = &paramSet[pi];
         nB = 0;
         Bs[nB++] = 1; Bs[nB++] = 2; Bs[nB++] = 3; Bs[nB++] = 7;
         if( p->k > 2 ) Bs[nB++] = p->k - 1;
         Bs[nB++] = p->k; Bs[nB++] = p->k + 1;
         Bs[nB++] = 32 * p->k - 1; Bs[nB++] = 32 * p->k; Bs[nB++] = 32 * p->k + 1;
         Bs[nB++] = 64; Bs[nB++] = 1024;
         for( bi = 0; bi < nB; bi++ )
            for( s = 0; s < N; s++ )
               for( e = s; e < N; e++ )
                  fails += compareAll( high, low, close, s, e, p, Bs[bi], bi == 0, N,
                                       refM, refS, vM, vS, shapeName[shape] );
      }
   }
   return fails;
}

/* Long series: the 32*period countdowns fire many times, on and off block edges. */
static int runLong( void )
{
   enum { N = 100000 };
   double *high = malloc( N * sizeof(double) ), *low = malloc( N * sizeof(double) );
   double *close = malloc( N * sizeof(double) ), *refM = malloc( N * sizeof(double) );
   double *vM = malloc( N * sizeof(double) );
   int *refS = malloc( N * sizeof(int) ), *vS = malloc( N * sizeof(int) );
   static const int Bs[] = { 1, 7, 19, 20, 21, 639, 640, 641, 1000, 1024, 4096 };
   static const int starts[] = { 0, 1, 38, 39, 40, 777, 50000 };
   int shape, pi, bi, si, fails = 0;

   for( shape = 0; shape < S_COUNT; shape++ )
   {
      makeSeries( shape, 4242 + (unsigned)shape, N, high, low, close );
      for( pi = 0; pi < NPARAMS; pi++ )
         for( bi = 0; bi < (int)(sizeof(Bs)/sizeof(Bs[0])); bi++ )
            for( si = 0; si < (int)(sizeof(starts)/sizeof(starts[0])); si++ )
            {
               if( Bs[bi] == 1 && si > 1 )
                  continue;
               /* Bs[1] is the first length every start runs at. */
               fails += compareAll( high, low, close, starts[si], N - 1 - si, &paramSet[pi], Bs[bi], bi == 1, N,
                                    refM, refS, vM, vS, shapeName[shape] );
            }
   }
   free( high ); free( low ); free( close ); free( refM ); free( vM ); free( refS ); free( vS );
   return fails;
}

/* ---- timing ----------------------------------------------------------- */

static double nowNs( void )
{
   struct timespec ts;
   clock_gettime( CLOCK_MONOTONIC, &ts );
   return (double)ts.tv_sec * 1e9 + (double)ts.tv_nsec;
}
static long minflt( void )
{
   struct rusage ru;
   getrusage( RUSAGE_SELF, &ru );
   return ru.ru_minflt;
}
static int cmpD( const void *x, const void *y )
{
   double a = *(const double *)x, b = *(const double *)y;
   return (a > b) - (a < b);
}

/* PROBE_ONLY=iblk,ref times only those variants, in that order within each
 * rep (PROBE_ONLY=iblk alone: nothing else ever runs in the process). The
 * variants share one allocator, and glibc's mmap threshold moves with what
 * any of them frees. Fills order[] and returns its length. */
static int timeOrder( int *order )
{
   const char *only = getenv( "PROBE_ONLY" ), *p;
   int vi, cnt = 0;
   if( !only || !*only )
   {
      for( vi = 0; vi < NVAR; vi++ )
         order[cnt++] = vi;
      return cnt;
   }
   for( p = only; *p; )
   {
      size_t len = strcspn( p, "," );
      for( vi = 0; vi < NVAR; vi++ )
         if( strlen( variants[vi].name ) == len && strncmp( variants[vi].name, p, len ) == 0 )
            break;
      if( vi < NVAR && cnt < NVAR )
      {
         int dup = 0, q;
         for( q = 0; q < cnt; q++ )
            dup |= order[q] == vi;
         if( !dup )
            order[cnt++] = vi;
      }
      p += len;
      if( *p == ',' )
         p++;
   }
   return cnt;
}

/* PROBE_ROTATE=R: R walk series of the same length and distribution (seeds
 * 7..7+R-1), call j on series j % R, so no input is replayed often enough for
 * a branch predictor to learn it. Unset or 1: the one series, replayed. */
static int rotateCount( void )
{
   const char *s = getenv( "PROBE_ROTATE" );
   int r = s ? atoi( s ) : 1;
   return r < 1 ? 1 : r;
}

static int runTime( int n, int B, int reps, int period, int fresh )
{
   int R = rotateCount();
   double *high = malloc( (size_t)R * (size_t)n * sizeof(double) ), *low = malloc( (size_t)R * (size_t)n * sizeof(double) );
   double *close = malloc( (size_t)R * (size_t)n * sizeof(double) ), *fixM = malloc( (size_t)n * sizeof(double) );
   int *fixS = malloc( (size_t)n * sizeof(int) );
   double *tNs = malloc( (size_t)NVAR * (size_t)reps * sizeof(double) );
   int tCap = reps;
   long faults[16] = {0};
   Params p = paramSet[0];
   int r, vi, oi, ob, on, inner, j, used = 0, stable, nOrd, order[NVAR];
   long f0;
   long long call = 0;
   double t0, sink = 0.0, lo1, lo2, tol = 0.01, warmUntil;
   const char *tolEnv = getenv( "PROBE_TOL" );

   if( n < 2 || B < 1 || reps < 2 || period < 2 || !high || !low || !close )
   {
      printf( "time: bad arguments\n" );
      return 2;
   }
   nOrd = timeOrder( order );
   p.b = period;
   p.k = period;
   for( j = 0; j < R; j++ )
      makeSeries( S_WALK, 7 + (unsigned)j, n, high + (size_t)j * n, low + (size_t)j * n, close + (size_t)j * n );
   memset( fixM, 0, (size_t)n * sizeof(double) );
   memset( fixS, 0, (size_t)n * sizeof(int) );
   inner = 2000000 / n;
   if( inner < 1 )
      inner = 1;
   if( tolEnv && atof( tolEnv ) > 0.0 )
      tol = atof( tolEnv );
   /* Warm-up reps for PROBE_WARM_MS (default 300), then at least two timed
    * ones; stop as soon as every variant's two lowest times agree within tol,
    * or at `reps`. A rep is already `inner` calls, so two agreeing reps
    * reproduce the floor; the warm-up is by time because a process that has
    * not reached its speed level yet agrees with itself too. */
   warmUntil = nowNs() + 1e6 * (getenv( "PROBE_WARM_MS" ) ? atof( getenv( "PROBE_WARM_MS" ) ) : 300.0);
   for( r = -1; r < reps; r++ )
   {
      for( oi = 0; oi < nOrd; oi++ )
      {
         SqzFn fn;
         vi = order[oi];
         fn = variants[vi].fn;
         f0 = minflt();
         t0 = nowNs();
         for( j = 0; j < inner; j++ )
         {
            double *outM = fixM;
            int *outS = fixS;
            size_t so = (size_t)(call++ % R) * (size_t)n;
            if( fresh )
            {
               outM = malloc( (size_t)n * sizeof(double) );
               outS = malloc( (size_t)n * sizeof(int) );
            }
            on = 0;
            fn( 0, n - 1, high + so, low + so, close + so, p.b, p.nbdev, p.k, p.fw, p.fn, p.fa, B, &ob, &on, outM, outS );
            if( on > 0 )
               sink += outM[on - 1] + outS[on / 2];
            if( fresh )
            {
               free( outM );
               free( outS );
            }
         }
         if( r >= 0 )
         {
            tNs[vi * tCap + r] = (nowNs() - t0) / inner;
            faults[vi] += minflt() - f0;
         }
      }
      if( r < 0 )
      {
         if( nowNs() < warmUntil )
            r = -2;
         continue;
      }
      used = r + 1;
      if( used < 2 )
         continue;
      stable = 1;
      for( oi = 0; oi < nOrd; oi++ )
      {
         vi = order[oi];
         lo1 = lo2 = 1e300;
         for( j = 0; j < used; j++ )
         {
            double t = tNs[vi * tCap + j];
            if( t < lo1 ) { lo2 = lo1; lo1 = t; }
            else if( t < lo2 ) lo2 = t;
         }
         if( lo2 > lo1 * (1.0 + tol) )
            stable = 0;
      }
      if( stable )
         break;
   }
   printf( "time: points=%d blockLen=%d reps=%d/%d%s inner=%d period=%d fresh=%d tol=%g rotate=%d only=%s\n",
           n, B, used, reps, used == reps && reps > 2 ? " (cap)" : "", inner, period, fresh, tol, R,
           getenv( "PROBE_ONLY" ) ? getenv( "PROBE_ONLY" ) : "all" );
   reps = used;
   for( oi = 0; oi < nOrd; oi++ )
   {
      double *t;
      vi = order[oi];
      t = &tNs[vi * tCap];
      qsort( t, (size_t)reps, sizeof(double), cmpD );
      printf( "  %-4s min %10.0f  2nd %10.0f  med %10.0f  max %10.0f ns/call  min %7.3f ns/bar  %8.2f minflt/call\n",
              variants[vi].name, t[0], t[reps > 1 ? 1 : 0], t[reps / 2], t[reps - 1], t[0] / n,
              (double)faults[vi] / ((double)reps * inner) );
   }
   printf( "  sink %g\n", sink );
   free( high ); free( low ); free( close ); free( fixM ); free( fixS ); free( tNs );
   return 0;
}

/* ---- parts: each sub-calculation alone, hand/batch form vs step form ---- */

/* One context for every part. Series s of R lives at offset s*n (syn: the
 * DEV series SQZMOM feeds LINEARREG, synN = n-(k-1) bars, same offsets).
 * With `full` set, outputs land there (bar s0 at [0]) for the bit compare;
 * otherwise in a block-sized pad, as in the blocked variants. */
typedef struct
{
   int n, B, k, R, synN;
   long long calls;
   const double *high, *low, *close, *syn;
   double *pad, *raw, *full;
   double sink;
   int emaS0;
} PartCtx;

static size_t partSeries( PartCtx *c )
{
   return (size_t)(c->calls++ % c->R) * (size_t)c->n;
}

static double *partOut( PartCtx *c, int a, int s0 )
{
   return c->full ? c->full + (a - s0) : c->pad;
}

static int partEnd( const PartCtx *c, int a, int last )
{
   int e = a + c->B - 1;
   return e > last ? last : e;
}

static void part_sma_hand( void *v )
{
   PartCtx *c = v;
   const double *in = c->close + partSeries( c );
   SmaCarry s = {0};
   int a, e, s0 = c->k - 1;
   for( a = s0; a < c->n; a = e + 1 )
   {
      double *o = partOut( c, a, s0 );
      e = partEnd( c, a, c->n - 1 );
      sma_blk( &s, in, 0, c->k, a, e, o );
      c->sink += o[e - a];
   }
}

static void part_var_hand( void *v )
{
   PartCtx *c = v;
   const double *in = c->close + partSeries( c );
   VarCarry s = {0};
   int a, e, s0 = c->k - 1;
   for( a = s0; a < c->n; a = e + 1 )
   {
      double *o = partOut( c, a, s0 );
      e = partEnd( c, a, c->n - 1 );
      var_blk( &s, in, 0, c->k, a, e, o );
      c->sink += o[e - a];
   }
}

static void part_lr_hand( void *v )
{
   PartCtx *c = v;
   const double *in = c->syn + partSeries( c );
   LrCarry s = {0};
   int a, e, s0 = c->k - 1;
   for( a = s0; a < c->synN; a = e + 1 )
   {
      double *o = partOut( c, a, s0 );
      e = partEnd( c, a, c->synN - 1 );
      lr_blk( &s, in, 0, c->k, a, e, o );
      c->sink += o[e - a];
   }
}

/* As blk: each block restarts the batch call on the period grid the
 * single-range call from s0 scans, into a pad of B+k-1. */
static void part_mid_batch( void *v )
{
   PartCtx *c = v;
   size_t so = partSeries( c );
   const double *hi = c->high + so, *lo = c->low + so;
   int a, e, g, ob, on, s0 = c->k - 1;
   for( a = s0; a < c->n; a = e + 1 )
   {
      e = partEnd( c, a, c->n - 1 );
      g = a - ((a - s0) % c->k);
      if( TA_MIDPRICE( g, e, hi, lo, c->k, &ob, &on, c->raw ) != TA_SUCCESS || ob != g || on != e - g + 1 )
         abort();
      if( c->full )
         memcpy( c->full + (a - s0), c->raw + (a - g), (size_t)(e - a + 1) * sizeof(double) );
      c->sink += c->raw[e - g];
   }
}

static void part_mid_step( void *v )
{
   PartCtx *c = v;
   size_t so = partSeries( c );
   const double *hi = c->high + so, *lo = c->low + so;
   TA_MIDPRICE_Stream *h = NULL;
   int a, e, s0 = c->k - 1;
   double o0;
   if( TA_MIDPRICE_Open( &h, hi, lo, c->k, c->k, &o0 ) != TA_SUCCESS )
      abort();
   if( c->full )
      c->full[0] = o0;
   for( a = s0 + 1; a < c->n; a = e + 1 )
   {
      double *o = partOut( c, a, s0 );
      e = partEnd( c, a, c->n - 1 );
      PROBE_MIDPRICE_Block( h, hi + a, lo + a, e - a + 1, o );
      c->sink += o[e - a];
   }
   TA_MIDPRICE_Close( h );
}

static void part_tr_batch( void *v )
{
   PartCtx *c = v;
   size_t so = partSeries( c );
   const double *hi = c->high + so, *lo = c->low + so, *cl = c->close + so;
   int a, e, ob, on, s0 = 1;
   for( a = s0; a < c->n; a = e + 1 )
   {
      double *o = partOut( c, a, s0 );
      e = partEnd( c, a, c->n - 1 );
      if( TA_TRANGE( a, e, hi, lo, cl, &ob, &on, o ) != TA_SUCCESS || ob != a || on != e - a + 1 )
         abort();
      c->sink += o[e - a];
   }
}

static void part_tr_step( void *v )
{
   PartCtx *c = v;
   size_t so = partSeries( c );
   const double *hi = c->high + so, *lo = c->low + so, *cl = c->close + so;
   TA_TRANGE_Stream *h = NULL;
   int a, e, s0 = 1;
   double o0;
   if( TA_TRANGE_Open( &h, hi, lo, cl, 2, &o0 ) != TA_SUCCESS )
      abort();
   if( c->full )
      c->full[0] = o0;
   for( a = s0 + 1; a < c->n; a = e + 1 )
   {
      double *o = partOut( c, a, s0 );
      e = partEnd( c, a, c->n - 1 );
      PROBE_TRANGE_Block( h, hi + a, lo + a, cl + a, e - a + 1, o );
      c->sink += o[e - a];
   }
   TA_TRANGE_Close( h );
}

static void part_ema_hand( void *v )
{
   PartCtx *c = v;
   const double *in = c->close + partSeries( c );
   EmaCarry s = {0};
   int a, e, s0 = c->emaS0;
   for( a = s0; a < c->n; a = e + 1 )
   {
      double *o = partOut( c, a, s0 );
      e = partEnd( c, a, c->n - 1 );
      ema_blk( &s, in, 0, c->k, a, e, o );
      c->sink += o[e - a];
   }
}

/* A single-input handle opened on bars [0, s0] (OPEN fills o0, the output at
 * s0), then BLOCK over each later block of c->LEN bars of c->SRC. */
#define PART_HANDLE( fname, Stream, OPEN, BLOCK, CLOSE, SRC, LEN, S0 ) \
static void fname( void *v ) \
{ \
   PartCtx *c = v; \
   const double *in = c->SRC + partSeries( c ); \
   Stream *h = NULL; \
   int a, e, s0 = (S0); \
   double o0; \
   if( (OPEN) != TA_SUCCESS ) \
      abort(); \
   if( c->full ) \
      c->full[0] = o0; \
   for( a = s0 + 1; a < c->LEN; a = e + 1 ) \
   { \
      double *o = partOut( c, a, s0 ); \
      e = partEnd( c, a, c->LEN - 1 ); \
      BLOCK( h, in + a, e - a + 1, o ); \
      c->sink += o[e - a]; \
   } \
   CLOSE( h ); \
}

#define OPEN_SMA TA_SMA_Open( &h, in, s0 + 1, c->k, &o0 )
#define OPEN_VAR TA_VAR_Open( &h, in, s0 + 1, c->k, 1.0, &o0 )
#define OPEN_LR  TA_LINEARREG_Open( &h, in, s0 + 1, c->k, &o0 )
#define OPEN_EMA TA_EMA_Open( &h, in, s0 + 1, c->k, &o0 )
PART_HANDLE( part_sma_step,  TA_SMA_Stream, OPEN_SMA, PROBE_SMA_Block, TA_SMA_Close, close, n, c->k - 1 )
PART_HANDLE( part_sma_kern,  TA_SMA_Stream, OPEN_SMA, PROBE_SMA_Kern, TA_SMA_Close, close, n, c->k - 1 )
PART_HANDLE( part_sma_kernR, TA_SMA_Stream, OPEN_SMA, PROBE_SMA_KernR, TA_SMA_Close, close, n, c->k - 1 )
PART_HANDLE( part_var_step,  TA_VAR_Stream, OPEN_VAR, PROBE_VAR_Block, TA_VAR_Close, close, n, c->k - 1 )
PART_HANDLE( part_var_kern,  TA_VAR_Stream, OPEN_VAR, PROBE_VAR_Kern, TA_VAR_Close, close, n, c->k - 1 )
PART_HANDLE( part_var_kernR, TA_VAR_Stream, OPEN_VAR, PROBE_VAR_KernR, TA_VAR_Close, close, n, c->k - 1 )
PART_HANDLE( part_lr_step,   TA_LINEARREG_Stream, OPEN_LR, PROBE_LINEARREG_Block, TA_LINEARREG_Close, syn, synN, c->k - 1 )
PART_HANDLE( part_lr_kern,   TA_LINEARREG_Stream, OPEN_LR, PROBE_LINEARREG_Kern, TA_LINEARREG_Close, syn, synN, c->k - 1 )
PART_HANDLE( part_lr_kernR,  TA_LINEARREG_Stream, OPEN_LR, PROBE_LINEARREG_KernR, TA_LINEARREG_Close, syn, synN, c->k - 1 )
PART_HANDLE( part_ema_step,  TA_EMA_Stream, OPEN_EMA, PROBE_EMA_Block, TA_EMA_Close, close, n, c->emaS0 )
PART_HANDLE( part_ema_kern,  TA_EMA_Stream, OPEN_EMA, PROBE_EMA_Kern, TA_EMA_Close, close, n, c->emaS0 )
PART_HANDLE( part_ema_kernR, TA_EMA_Stream, OPEN_EMA, PROBE_EMA_KernR, TA_EMA_Close, close, n, c->emaS0 )
#undef PART_HANDLE

/* Forms per part: hand (or batch), step, kern, kernR; NULL where absent. */
enum { NFORM = 4 };
static const char *formName[NFORM] = { "hand", "step", "kern", "kernR" };
enum { S0_K1, S0_ONE, S0_EMA };
typedef void (*PartFn)( void * );
typedef struct { const char *name; PartFn fn[NFORM]; int s0Kind, onSyn; } PartPair;

static const PartPair partPairs[] = {
   { "SMA(k) close",     { part_sma_hand, part_sma_step, part_sma_kern, part_sma_kernR }, S0_K1, 0 },
   { "VAR(b) close",     { part_var_hand, part_var_step, part_var_kern, part_var_kernR }, S0_K1, 0 },
   { "LINEARREG(k) dev", { part_lr_hand, part_lr_step, part_lr_kern, part_lr_kernR }, S0_K1, 1 },
   { "EMA(k) close",     { part_ema_hand, part_ema_step, part_ema_kern, part_ema_kernR }, S0_EMA, 0 },
   { "MIDPRICE(k)",      { part_mid_batch, part_mid_step, NULL, NULL }, S0_K1, 0 },
   { "TRANGE",           { part_tr_batch, part_tr_step, NULL, NULL }, S0_ONE, 0 },
};
#define NPART ((int)(sizeof(partPairs)/sizeof(partPairs[0])))

/* The input shape each N reaches: the walk drifts down in log price, and
 * cents rounding turns a low price into flat runs. MIDPRICE's stream rescans
 * its window whenever the cached extreme leaves it; this replays that rule
 * (both extremes, rescan scan length included) without timing it. */
static void seriesStats( const double *high, const double *low, const double *close, int n, int k )
{
   static const int cut[] = { 0, 2000, 20000, 100000, 1000000, 10000000 };
   int ci, t, hiIdx = -1, loIdx = -1, i, trailing, today;
   double hiV = 0.0, loV = 0.0;
   for( ci = 0; ci + 1 < (int)(sizeof(cut)/sizeof(cut[0])) && cut[ci] < n; ci++ )
   {
      int from = cut[ci], to = cut[ci + 1] < n ? cut[ci + 1] : n;
      long long flat = 0, rep = 0, rescans = 0, scanned = 0, bars = 0;
      for( t = from; t < to; t++ )
      {
         flat += high[t] == low[t];
         rep += t > 0 && close[t] == close[t - 1];
         if( t < k - 1 )
            continue;
         today = t;
         trailing = t - (k - 1);
         bars++;
         if( hiIdx < trailing )
         {
            rescans++;
            hiIdx = trailing;
            hiV = high[hiIdx];
            for( i = hiIdx + 1; i <= today; i++ )
            {
               scanned++;
               if( high[i] > hiV ) { hiIdx = i; hiV = high[i]; }
            }
         }
         else if( high[today] >= hiV ) { hiIdx = today; hiV = high[today]; }
         if( loIdx < trailing )
         {
            rescans++;
            loIdx = trailing;
            loV = low[loIdx];
            for( i = loIdx + 1; i <= today; i++ )
            {
               scanned++;
               if( low[i] < loV ) { loIdx = i; loV = low[i]; }
            }
         }
         else if( low[today] <= loV ) { loIdx = today; loV = low[today]; }
      }
      printf( "  series [%d,%d): close at end %.6g, high==low %.1f%%, close repeats %.1f%%, "
              "MIDPRICE step rescans %.3f/bar scanning %.2f elems/bar\n",
              from, to, close[to - 1], 100.0 * flat / (to - from), 100.0 * rep / (to - from),
              bars ? (double)rescans / bars : 0.0, bars ? (double)scanned / bars : 0.0 );
   }
}

static int partS0( const PartCtx *c, int pi )
{
   switch( partPairs[pi].s0Kind )
   {
   case S0_ONE: return 1;
   case S0_EMA: return c->emaS0;
   default:     return c->k - 1;
   }
}

/* 1: same bits, 2: differ in signed zeros only, 0: differ. */
static int bitsCmp( const double *x, const double *y, int cnt )
{
   int j, r = memcmp( x, y, (size_t)cnt * sizeof(double) ) == 0 ? 1 : 2;
   for( j = 0; r == 2 && j < cnt; j++ )
      if( memcmp( &x[j], &y[j], sizeof(double) ) != 0 && !(x[j] == 0.0 && y[j] == 0.0) )
         r = 0;
   return r;
}

static const char *bitsName( int b )
{
   return b == 1 ? "same" : b == 2 ? "DIFFER (signed zeros only)" : "DIFFER";
}

static int runParts( int n, int B, int reps, int period )
{
   int R = rotateCount(), k = period, synN = n - (period - 1), emaS0 = TA_EMA_Lookback( period );
   size_t tot = (size_t)R * (size_t)n;
   double *high = malloc( tot * sizeof(double) ), *low = malloc( tot * sizeof(double) );
   double *close = malloc( tot * sizeof(double) ), *syn = malloc( tot * sizeof(double) );
   double *mid = malloc( (size_t)n * sizeof(double) ), *sma = malloc( (size_t)n * sizeof(double) );
   double *pad = malloc( (size_t)(B + k) * sizeof(double) ), *raw = malloc( (size_t)(B + k) * sizeof(double) );
   double *fullA = malloc( (size_t)n * sizeof(double) ), *fullB = malloc( (size_t)n * sizeof(double) );
   double *tNs = malloc( (size_t)(NFORM * NPART) * (size_t)reps * sizeof(double) );
   PartCtx ctx[NFORM * NPART];
   int s, pi, fi, ji, r, j, inner, used = 0, stable, ob, m1, m2, bits[NPART][NFORM];
   double t0, lo1, lo2, tol = 0.01, warmUntil, sink = 0.0;
   const char *tolEnv = getenv( "PROBE_TOL" );

   if( n < 4 * period || n <= emaS0 + 1 || B < 1 || reps < 2 || period < 2 || !high || !low || !close || !syn || !tNs )
   {
      printf( "parts: bad arguments (points >= 4*period and > EMA lookback + 1)\n" );
      return 2;
   }
   for( s = 0; s < R; s++ )
   {
      size_t so = (size_t)s * n;
      makeSeries( S_WALK, 7 + (unsigned)s, n, high + so, low + so, close + so );
      if( TA_MIDPRICE( 0, n - 1, high + so, low + so, k, &ob, &m1, mid ) != TA_SUCCESS ||
          TA_SMA( 0, n - 1, close + so, k, &ob, &m2, sma ) != TA_SUCCESS || m1 != synN || m2 != synN )
         return 2;
      for( j = 0; j < synN; j++ )
         syn[so + j] = close[so + j + k - 1] - (mid[j] + sma[j]) / 2.0;
   }
   if( tolEnv && atof( tolEnv ) > 0.0 )
      tol = atof( tolEnv );
   printf( "parts: points=%d blockLen=%d period=%d rotate=%d (series 0 below)\n", n, B, period, R );
   seriesStats( high, low, close, n, k );

   for( ji = 0; ji < NFORM * NPART; ji++ )
   {
      PartCtx z = { n, B, k, R, synN, 0, high, low, close, syn, pad, raw, NULL, 0.0, emaS0 };
      ctx[ji] = z;
   }

   /* ema_blk against the public batch call, once, on series 0. */
   {
      PartCtx a = ctx[0];
      int eb = -1, en = -1;
      a.R = 1;
      memset( fullA, 0xA5, (size_t)n * sizeof(double) );
      memset( fullB, 0x5A, (size_t)n * sizeof(double) );
      a.full = fullA;
      part_ema_hand( &a );
      if( TA_EMA( 0, n - 1, close, k, &eb, &en, fullB ) != TA_SUCCESS || eb != emaS0 || en != n - emaS0 )
         printf( "  ema hand vs TA_EMA: DIFFER (TA_EMA range %d+%d, hand %d+%d)\n", eb, en, emaS0, n - emaS0 );
      else
         printf( "  ema hand vs TA_EMA: bits=%s\n", bitsName( bitsCmp( fullA, fullB, en ) ) );
   }

   /* Every form against the part's hand form, on series 0. */
   for( pi = 0; pi < NPART; pi++ )
   {
      int cnt = (partPairs[pi].onSyn ? synN : n) - partS0( &ctx[0], pi );
      PartCtx a = ctx[NFORM * pi];
      a.R = 1;
      memset( fullA, 0xA5, (size_t)n * sizeof(double) );
      a.full = fullA;
      partPairs[pi].fn[0]( &a );
      bits[pi][0] = 1;
      for( fi = 1; fi < NFORM; fi++ )
      {
         PartCtx b = ctx[NFORM * pi + fi];
         if( !partPairs[pi].fn[fi] )
            continue;
         b.R = 1;
         memset( fullB, 0x5A, (size_t)n * sizeof(double) );
         b.full = fullB;
         partPairs[pi].fn[fi]( &b );
         bits[pi][fi] = bitsCmp( fullA, fullB, cnt );
      }
   }

   inner = 2000000 / n;
   if( inner < 1 )
      inner = 1;
   warmUntil = nowNs() + 1e6 * (getenv( "PROBE_WARM_MS" ) ? atof( getenv( "PROBE_WARM_MS" ) ) : 300.0);
   for( r = -1; r < reps; r++ )
   {
      for( ji = 0; ji < NFORM * NPART; ji++ )
      {
         PartFn fn = partPairs[ji / NFORM].fn[ji % NFORM];
         if( !fn )
            continue;
         t0 = nowNs();
         for( j = 0; j < inner; j++ )
            fn( &ctx[ji] );
         if( r >= 0 )
            tNs[ji * reps + r] = (nowNs() - t0) / inner;
      }
      if( r < 0 )
      {
         if( nowNs() < warmUntil )
            r = -2;
         continue;
      }
      used = r + 1;
      if( used < 2 )
         continue;
      stable = 1;
      for( ji = 0; ji < NFORM * NPART; ji++ )
      {
         if( !partPairs[ji / NFORM].fn[ji % NFORM] )
            continue;
         lo1 = lo2 = 1e300;
         for( j = 0; j < used; j++ )
         {
            double t = tNs[ji * reps + j];
            if( t < lo1 ) { lo2 = lo1; lo1 = t; }
            else if( t < lo2 ) lo2 = t;
         }
         if( lo2 > lo1 * (1.0 + tol) )
            stable = 0;
      }
      if( stable )
         break;
   }
   printf( "  reps=%d/%d%s inner=%d tol=%g\n", used, reps, used == reps && reps > 2 ? " (cap)" : "", inner, tol );
   for( pi = 0; pi < NPART; pi++ )
   {
      double best[NFORM];
      int bars = (partPairs[pi].onSyn ? synN : n) - partS0( &ctx[0], pi );
      for( fi = 0; fi < NFORM; fi++ )
      {
         double *t = &tNs[(NFORM * pi + fi) * reps];
         if( !partPairs[pi].fn[fi] )
            continue;
         qsort( t, (size_t)used, sizeof(double), cmpD );
         best[fi] = t[0] / bars;
         sink += ctx[NFORM * pi + fi].sink;
      }
      if( !partPairs[pi].fn[2] )
         printf( "  %-17s hand/batch %7.3f ns/bar  step %7.3f ns/bar  step-hand %+7.3f  bits=%s\n",
                 partPairs[pi].name, best[0], best[1], best[1] - best[0], bitsName( bits[pi][1] ) );
      else
      {
         printf( "  %-17s %s %7.3f", partPairs[pi].name, formName[0], best[0] );
         for( fi = 1; fi < NFORM; fi++ )
            printf( "  %s %7.3f bits=%s", formName[fi], best[fi], bitsName( bits[pi][fi] ) );
         printf( "  ns/bar\n" );
      }
   }
   printf( "  sink %g\n", sink );
   free( high ); free( low ); free( close ); free( syn ); free( mid ); free( sma );
   free( pad ); free( raw ); free( fullA ); free( fullB ); free( tNs );
   return 0;
}

int main( int argc, char **argv )
{
   int fails, vi, vacuous = 0;
   if( TA_Initialize() != TA_SUCCESS )
      return 2;
   if( getenv( "PROBE_REVERT" ) )
      walkRevert = atof( getenv( "PROBE_REVERT" ) );
#ifdef __APPLE__
   /* No affinity on Apple Silicon: the QoS class is what keeps the thread off the E-cores. */
   pthread_set_qos_class_self_np( QOS_CLASS_USER_INITIATED, 0 );
#endif
   if( argc >= 2 && strcmp( argv[1], "check" ) == 0 )
      fails = runCheck();
   else if( argc >= 2 && strcmp( argv[1], "long" ) == 0 )
      fails = runLong();
   else if( argc >= 3 && strcmp( argv[1], "spin" ) == 0 )
   {
      /* Busy for that many seconds: brings a sleeping core up to frequency. */
      double until = nowNs() + 1e9 * atof( argv[2] );
      volatile double x = 0.0;
      while( nowNs() < until )
         x += 1.0;
      return 0;
   }
   else if( argc >= 5 && strcmp( argv[1], "time" ) == 0 )
      return runTime( atoi( argv[2] ), atoi( argv[3] ), atoi( argv[4] ),
                      argc >= 6 ? atoi( argv[5] ) : 20, argc >= 7 ? atoi( argv[6] ) : 0 );
   else if( argc >= 5 && strcmp( argv[1], "parts" ) == 0 )
      return runParts( atoi( argv[2] ), atoi( argv[3] ), atoi( argv[4] ), argc >= 6 ? atoi( argv[5] ) : 20 );
   else
   {
      printf( "usage: probe check | probe long | probe spin <secs> | probe time <points> <blockLen> <reps> [period] [fresh]\n"
              "       | probe parts <points> <blockLen> <reps> [period]\n"
              "env (time, parts): PROBE_ONLY=a,b (time: those variants, in that order) PROBE_ROTATE=R PROBE_WARM_MS PROBE_TOL\n" );
      return 2;
   }
   for( vi = 1; vi < NVAR; vi++ )
   {
      Variant *v = &variants[vi];
      printf( "%s %-4s: %lld calls compared, %lld outputs, %lld block edges crossed, %d mismatch(es)%s\n",
              argv[1], v->name, v->compared, v->outputs, v->edges, v->fails,
              v->fails >= FAILCAP ? " (stopped at cap)" : "" );
      if( v->outputs == 0 || (v->blocked && v->edges == 0) )
         vacuous = 1;
   }
   if( vacuous )
   {
      printf( "VACUOUS: a variant compared no output, or a blocked one crossed no block edge\n" );
      return 3;
   }
   printf( "%s: %s\n", argv[1], fails ? "FAIL" : "PASS" );
   return fails ? 1 : 0;
}
