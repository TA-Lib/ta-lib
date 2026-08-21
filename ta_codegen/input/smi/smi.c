/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  CC       Claude Code (AI assistant)
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  082026 MF,CC  Initial version (#238).
 *
 */

int smi_lookback(int optInTimePeriod, int optInFastPeriod, int optInSlowPeriod, int optInSignalPeriod)
{
   /* One high/low window, then the three EMA warm-ups the pipeline stacks on
    * top of it: slow smooths the raw momentum, fast smooths that, and signal
    * smooths the finished SMI line. Every term is exactly the lookback of the
    * function it comes from, so none of them is restated here -- which is also
    * what makes SMI inherit TA_FUNC_UNST_EMA from its callee.
    */
   return (optInTimePeriod - 1)
   + ema_lookback( optInSlowPeriod )
   + ema_lookback( optInFastPeriod )
   + ema_lookback( optInSignalPeriod );
}

TA_RetCode smi(int startIdx, int endIdx,
   const double inHigh[],
   const double inLow[],
   const double inClose[],
   int optInTimePeriod,
   int optInFastPeriod,
   int optInSlowPeriod,
   int optInSignalPeriod,
   int *outBegIdx, int *outNBElement,
   double outSMI[],
   double outSMISignal[])
{
   double kSlow, kFast, kSignal;
   double highest, lowest, tmp;
   double emaSlowNum, emaSlowDen, emaFastNum, emaFastDen;
   double sumSlowNum, sumSlowDen, sumFastNum, sumFastDen, sumSignal;
   double num, den, halfDen, smiValue, prevSignal;
   int lookbackTotal, lookbackSlow, lookbackFast;
   int today, trailingIdx, highestIdx, lowestIdx, i, outIdx;
   int nBar, nFast, nSignal;

   lookbackTotal = smi_lookback( optInTimePeriod, optInFastPeriod, optInSlowPeriod, optInSignalPeriod );

   if( startIdx < lookbackTotal )
      startIdx = lookbackTotal;

   if( startIdx > endIdx )
   {
      *outBegIdx = 0;
      *outNBElement = 0;
      return TA_SUCCESS;
   }

   *outBegIdx = startIdx;

   /* Blau's pipeline in one pass. A composed form is not available: the
    * streaming producer model carries exactly one intermediate series
    * (streaming.rs:2076) and SMI has two, the numerator and the denominator,
    * which must be smoothed separately before they are divided.
    *
    * Each of the three stages seeds the way ema.c does -- a simple average of
    * that stage's first 'period' inputs -- so the result is bit-identical to
    * TA_MAX + TA_MIN followed by four TA_EMA calls. The stage boundaries below
    * are the callee LOOKBACKS, not (period-1), so that a warm
    * TA_SetUnstablePeriod(TA_FUNC_UNST_EMA) folds in: each stage then seeds
    * from the values its predecessor would have published, exactly as the
    * composed form does. The seed sums accumulate from 0.0 in production
    * order; do not reorder or fuse them (0.0+x is not x for x=-0.0).
    *
    * TA_GetCompatibility() is deliberately NOT consulted, for the reason
    * spelled out in efi.c: ema.c's TA_COMPATIBILITY_METASTOCK seeding arm is
    * preserved for the functions that already shipped with it and dropped from
    * new ones, and it is not reachable at all from the Rust, Java and C# APIs.
    * The seeding choice itself is measured in docs/studies/ema-seeding/README.md.
    */
   kSlow   = 2.0 / ((double)(optInSlowPeriod + 1));
   kFast   = 2.0 / ((double)(optInFastPeriod + 1));
   kSignal = 2.0 / ((double)(optInSignalPeriod + 1));

   lookbackSlow = ema_lookback( optInSlowPeriod );
   lookbackFast = ema_lookback( optInFastPeriod );

   emaSlowNum = 0.0;
   emaSlowDen = 0.0;
   emaFastNum = 0.0;
   emaFastDen = 0.0;
   prevSignal = 0.0;
   smiValue   = 0.0;
   sumSlowNum = 0.0;
   sumSlowDen = 0.0;
   sumFastNum = 0.0;
   sumFastDen = 0.0;
   sumSignal  = 0.0;

   highest    = 0.0;
   lowest     = 0.0;
   highestIdx = -1;
   lowestIdx  = -1;

   /* The first bar carrying a full high/low window. */
   trailingIdx = startIdx - lookbackTotal;
   today       = trailingIdx + (optInTimePeriod - 1);
   nBar        = 0;

   /* Warm-up. Runs through startIdx inclusive: the last pass here is the one
    * that completes the signal seed, so it produces the first output pair.
    */
   while( today <= startIdx )
   {
      /* Set the lowest low */
      tmp = inLow[today];
      if( lowestIdx < trailingIdx )
      {
         lowestIdx = trailingIdx;
         lowest = inLow[lowestIdx];
         i = lowestIdx;
         while( ++i<=today )
         {
            tmp = inLow[i];
            if( tmp < lowest )
            {
               lowestIdx = i;
               lowest = tmp;
            }
         }
      }
      else if( tmp <= lowest )
      {
         lowestIdx = today;
         lowest = tmp;
      }

      /* Set the highest high */
      tmp = inHigh[today];
      if( highestIdx < trailingIdx )
      {
         highestIdx = trailingIdx;
         highest = inHigh[highestIdx];
         i = highestIdx;
         while( ++i<=today )
         {
            tmp = inHigh[i];
            if( tmp > highest )
            {
               highestIdx = i;
               highest = tmp;
            }
         }
      }
      else if( tmp >= highest )
      {
         highestIdx = today;
         highest = tmp;
      }

      den = highest - lowest;
      num = inClose[today] - ((highest + lowest) * 0.5);

      /* Stage 1: the slow EMA, over the raw momentum. */
      if( nBar < optInSlowPeriod )
      {
         sumSlowNum = sumSlowNum + num;
         sumSlowDen = sumSlowDen + den;
         if( nBar == optInSlowPeriod - 1 )
         {
            emaSlowNum = sumSlowNum / optInSlowPeriod;
            emaSlowDen = sumSlowDen / optInSlowPeriod;
         }
      }
      else
      {
         emaSlowNum = ((num - emaSlowNum) * kSlow) + emaSlowNum;
         emaSlowDen = ((den - emaSlowDen) * kSlow) + emaSlowDen;
      }

      /* Stage 2: the fast EMA, over what stage 1 publishes.
       *
       * The stage counters are compared BEFORE they are subtracted, never
       * after. Writing this as `nFast = nBar - lookbackSlow; if( nFast >= 0 )`
       * is correct in C, where the counters are signed, and broken everywhere
       * else: the Rust backend renders them as `usize`, so the subtraction
       * underflows for the first lookbackSlow bars -- a panic in a debug build
       * and a wrap in release. It would also be invisible to the cross-language
       * gate, which runs release servers at unstable period 0, where the branch
       * the wrap wrongly takes happens to be a no-op because both accumulators
       * are still 0.0. */
      if( nBar >= lookbackSlow )
      {
         nFast = nBar - lookbackSlow;
         if( nFast < optInFastPeriod )
         {
            sumFastNum = sumFastNum + emaSlowNum;
            sumFastDen = sumFastDen + emaSlowDen;
            if( nFast == optInFastPeriod - 1 )
            {
               emaFastNum = sumFastNum / optInFastPeriod;
               emaFastDen = sumFastDen / optInFastPeriod;
            }
         }
         else
         {
            emaFastNum = ((emaSlowNum - emaFastNum) * kFast) + emaFastNum;
            emaFastDen = ((emaSlowDen - emaFastDen) * kFast) + emaFastDen;
         }
      }

      /* Stage 3: the SMI line, then the signal EMA over it. */
      if( nBar >= lookbackSlow + lookbackFast )
      {
         nSignal = (nBar - lookbackSlow) - lookbackFast;
         halfDen = 0.5 * emaFastDen;
         if( !TA_IS_ZERO(halfDen) )
            smiValue = (100.0 * emaFastNum) / halfDen;
         else
            smiValue = 0.0;

         if( nSignal < optInSignalPeriod )
         {
            sumSignal = sumSignal + smiValue;
            if( nSignal == optInSignalPeriod - 1 )
               prevSignal = sumSignal / optInSignalPeriod;
         }
         else
            prevSignal = ((smiValue - prevSignal) * kSignal) + prevSignal;
      }

      nBar = nBar + 1;
      trailingIdx = trailingIdx + 1;
      today = today + 1;
   }

   outSMI[0] = smiValue;
   outSMISignal[0] = prevSignal;
   outIdx = 1;

   /* Stable zone. Every stage is a pure recursion from here on. */
   while( today <= endIdx )
   {
      /* Set the lowest low */
      tmp = inLow[today];
      if( lowestIdx < trailingIdx )
      {
         lowestIdx = trailingIdx;
         lowest = inLow[lowestIdx];
         i = lowestIdx;
         while( ++i<=today )
         {
            tmp = inLow[i];
            if( tmp < lowest )
            {
               lowestIdx = i;
               lowest = tmp;
            }
         }
      }
      else if( tmp <= lowest )
      {
         lowestIdx = today;
         lowest = tmp;
      }

      /* Set the highest high */
      tmp = inHigh[today];
      if( highestIdx < trailingIdx )
      {
         highestIdx = trailingIdx;
         highest = inHigh[highestIdx];
         i = highestIdx;
         while( ++i<=today )
         {
            tmp = inHigh[i];
            if( tmp > highest )
            {
               highestIdx = i;
               highest = tmp;
            }
         }
      }
      else if( tmp >= highest )
      {
         highestIdx = today;
         highest = tmp;
      }

      den = highest - lowest;
      num = inClose[today] - ((highest + lowest) * 0.5);

      emaSlowNum = ((num - emaSlowNum) * kSlow) + emaSlowNum;
      emaSlowDen = ((den - emaSlowDen) * kSlow) + emaSlowDen;
      emaFastNum = ((emaSlowNum - emaFastNum) * kFast) + emaFastNum;
      emaFastDen = ((emaSlowDen - emaFastDen) * kFast) + emaFastDen;

      /* Guard with TA_IS_ZERO, not an exact `halfDen != 0.0`: a machine-flat
       * window leaves a sub-epsilon residue that an exact check would divide
       * into noise (issue #107 / STOCHRSI). A window whose bars are all
       * H == L makes num zero too, so this is 0/0, and the neutral 0.0 is the
       * CCI (#7) and IMI (#112) convention.
       */
      halfDen = 0.5 * emaFastDen;
      if( !TA_IS_ZERO(halfDen) )
         smiValue = (100.0 * emaFastNum) / halfDen;
      else
         smiValue = 0.0;

      prevSignal = ((smiValue - prevSignal) * kSignal) + prevSignal;

      outSMI[outIdx] = smiValue;
      outSMISignal[outIdx] = prevSignal;
      outIdx = outIdx + 1;
      trailingIdx = trailingIdx + 1;
      today = today + 1;
   }

   *outNBElement = outIdx;

   return TA_SUCCESS;
}
