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
 *  112400 MF     Template creation.
 *  052603 MF     Adapt code to compile with .NET Managed C++
 *  071126 MF,CC  Inline the fixed-26/12 MACD lockstep pass (was a
 *                delegation to macd(...,0,0,...)); bit-exact, streamable.
 *
 */

int macdfix_lookback(int optInSignalPeriod)
{
   /* The lookback is driven by the signal line output.
    *
    * (must also account for the initial data consume
    *  by the fix 26 period EMA).
    */
   return ema_lookback( 26 )
   + ema_lookback( optInSignalPeriod );
}

TA_RetCode macdfix(int startIdx, int endIdx,
   const double inReal[],
   int optInSignalPeriod,
   int *outBegIdx, int *outNBElement,
   double outMACD[],
   double outMACDSignal[],
   double outMACDHist[])
{
   double prevFast, prevSlow, prevSignal, macdValue, tempReal;
   double slowK, fastK, signalK;
   int i, today, outIdx;
   int lookbackTotal, lookbackSignal;

   /* MACDFIX is the fixed 26/12 MACD: the fast/slow periods and their
    * smoothing factors are hardcoded (the general MACD selects these
    * exact values when its fast/slow period arguments are 0). Only the
    * signal period is caller-provided.
    *    Fix 12 -> fastK = 0.15
    *    Fix 26 -> slowK = 0.075
    */
   int optInFastPeriod = 12;
   int optInSlowPeriod = 26;
   fastK = 0.15;
   slowK = 0.075;

   signalK = 2.0 / ((double)(optInSignalPeriod + 1));
   lookbackSignal = ema_lookback( optInSignalPeriod );

   /* Move up the start index if there is not
    * enough initial data.
    */
   lookbackTotal =  lookbackSignal;
   lookbackTotal += ema_lookback( 26 ); /* fixed slow period */

   if( startIdx < lookbackTotal )
      startIdx = lookbackTotal;

   /* Make sure there is still something to evaluate. */
   if( startIdx > endIdx )
   {
      *outBegIdx = 0;
      *outNBElement = 0;
      return TA_SUCCESS;
   }

   /* Everything is computed in a single lockstep pass: each bar
    * advances the fast and slow EMA (two independent recursions),
    * their difference is the MACD line, and each MACD-line value
    * is immediately fed into the signal EMA. No temporary buffers.
    *
    * The arithmetic order below is the bit-exactness contract
    * (do not reorder or fuse operations):
    *  - EMA recursion: ((x-prev)*k)+prev.
    *  - Default compatibility: each EMA is seeded with the sum of
    *    its first 'period' inputs, accumulated from 0.0 in input
    *    order, divided by the period. The fast and slow seed
    *    windows end on the same bar. The signal EMA is seeded the
    *    same way from the first 'signal period' MACD-line values.
    *  - Metastock compatibility: the fast and slow EMA are seeded
    *    from inReal[0], the signal EMA from the first MACD-line
    *    value.
    * Output alignment is identical for all compatibility modes;
    * only the seed values differ.
    *
    * In-place (an output == inReal) is supported: outputs at
    * [outIdx] are written only after inReal[startIdx+outIdx] was
    * read.
    */
   if( TA_GetCompatibility() == TA_COMPATIBILITY_DEFAULT )
   {
      /* Seed each price EMA with a simple average of its first
       * 'period' price bars. The fast window is the tail of the
       * slow window: consume the leading slow-only bars first,
       * then accumulate both over the shared bars.
       */
      today = startIdx-lookbackTotal;
      tempReal = 0.0;
      i = optInSlowPeriod - optInFastPeriod;
      while( i-- > 0 )
         tempReal += inReal[today++];

      prevFast = 0.0;
      i = optInFastPeriod;
      while( i-- > 0 )
      {
         prevFast += inReal[today];
         tempReal += inReal[today++];
      }
      prevSlow = tempReal / optInSlowPeriod;
      prevFast = prevFast / optInFastPeriod;

      /* Advance both EMA through their unstable period, up to the
       * first MACD-line bar.
       */
      while( today <= startIdx-lookbackSignal )
      {
         tempReal = inReal[today++];
         prevFast = ((tempReal-prevFast)*fastK) + prevFast;
         prevSlow = ((tempReal-prevSlow)*slowK) + prevSlow;
      }
      macdValue = prevFast - prevSlow;

      /* Seed the signal EMA with a simple average of the first
       * 'signal period' MACD-line values, accumulated as they are
       * produced.
       */
      prevSignal = 0.0;
      prevSignal += macdValue;
      i = optInSignalPeriod-1;
      while( i-- > 0 )
      {
         tempReal = inReal[today++];
         prevFast = ((tempReal-prevFast)*fastK) + prevFast;
         prevSlow = ((tempReal-prevSlow)*slowK) + prevSlow;
         macdValue = prevFast - prevSlow;
         prevSignal += macdValue;
      }
      prevSignal = prevSignal / optInSignalPeriod;
   }
   else
   {
      /* Metastock/Tradestation: seed the fast and slow EMA with
       * inReal[0], advance them in lockstep up to the first
       * MACD-line bar, then seed the signal EMA with the first
       * MACD-line value.
       */
      prevFast = inReal[0];
      prevSlow = inReal[0];
      today = 1;
      while( today <= startIdx-lookbackSignal )
      {
         tempReal = inReal[today++];
         prevFast = ((tempReal-prevFast)*fastK) + prevFast;
         prevSlow = ((tempReal-prevSlow)*slowK) + prevSlow;
      }
      macdValue = prevFast - prevSlow;
      prevSignal = macdValue;
   }

   /* Advance everything in lockstep through the unstable period
    * of the signal EMA, up to the first output bar.
    */
   while( today <= startIdx )
   {
      tempReal = inReal[today++];
      prevFast = ((tempReal-prevFast)*fastK) + prevFast;
      prevSlow = ((tempReal-prevSlow)*slowK) + prevSlow;
      macdValue = prevFast - prevSlow;
      prevSignal = ((macdValue-prevSignal)*signalK) + prevSignal;
   }

   /* Stable zone: keep advancing in lockstep and write the three
    * outputs.
    */
   outMACD[0] = macdValue;
   outMACDSignal[0] = prevSignal;
   outMACDHist[0] = macdValue - prevSignal;
   outIdx = 1;
   while( today <= endIdx )
   {
      tempReal = inReal[today++];
      prevFast = ((tempReal-prevFast)*fastK) + prevFast;
      prevSlow = ((tempReal-prevSlow)*slowK) + prevSlow;
      macdValue = prevFast - prevSlow;
      prevSignal = ((macdValue-prevSignal)*signalK) + prevSignal;
      outMACD[outIdx] = macdValue;
      outMACDSignal[outIdx] = prevSignal;
      outMACDHist[outIdx] = macdValue - prevSignal;
      outIdx++;
   }

   /* All done! Indicate the output limits and return success. */
   *outBegIdx     = startIdx;
   *outNBElement  = outIdx;

   return TA_SUCCESS;
}
