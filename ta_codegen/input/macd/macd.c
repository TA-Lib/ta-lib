/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  JPP      JP Pienaar (j.pienaar@mci.co.za)
 *  CC       Claude Code (AI assistant)
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  112400 MF     Template creation.
 *  052603 MF     Adapt code to compile with .NET Managed C++
 *  080403 JPP    Fix #767653 for logic when swapping periods.
 *  070526 MF,CC  Speed optimization: compute the two price EMA, the
 *                signal line and the histogram in a single lockstep
 *                pass (bit-exact, no temporary buffers).
 *  080926 MF,CC  Explicit no-smoothing signal at a signal period of 1.
 *
 */

int macd_lookback(int optInFastPeriod, int optInSlowPeriod, int optInSignalPeriod)
{
   int tempInteger;

   /* The lookback is driven by the signal line output.
    *
    * (must also account for the initial data consume
    *  by the slow period).
    */

   /* Make sure slow is really slower than
    * the fast period! if not, swap...
    */
   if( optInSlowPeriod < optInFastPeriod )
   {
      /* swap */
      tempInteger       = optInSlowPeriod;
      optInSlowPeriod = optInFastPeriod;
      optInFastPeriod = tempInteger;
   }

   return ema_lookback( optInSlowPeriod )
   + ema_lookback( optInSignalPeriod );
}

TA_RetCode macd(int startIdx, int endIdx,
   const double inReal[],
   int optInFastPeriod,
   int optInSlowPeriod,
   int optInSignalPeriod,
   int *outBegIdx, int *outNBElement,
   double outMACD[],
   double outMACDSignal[],
   double outMACDHist[])
{
   double prevFast, prevSlow, prevSignal, macdValue, tempReal;
   double slowK, fastK, signalK;
   double slowBeta, fastBeta, signalBeta;
   int i, today, outIdx, tempInteger;
   int lookbackTotal, lookbackSignal;

   /* Make sure slow is really slower than
    * the fast period! if not, swap...
    */
   if( optInSlowPeriod < optInFastPeriod )
   {
      /* swap */
      tempInteger       = optInSlowPeriod;
      optInSlowPeriod = optInFastPeriod;
      optInFastPeriod = tempInteger;
   }

   /* The fixed 26/12 MACD: k of 0.075 and 0.15, as near as a pair summing
    * to exactly 1.0 comes.
    */
   if( optInSlowPeriod == 0 )
   {
      /* Fix 26 */
      optInSlowPeriod = 26;
      slowBeta = 1.0 - 0.075;
   }
   else
      slowBeta = ((double)(optInSlowPeriod - 1)) / ((double)(optInSlowPeriod + 1));
   slowK = 1.0 - slowBeta;
   slowBeta = 1.0 - slowK;

   if( optInFastPeriod == 0 )
   {
      /* Fix 12 */
      optInFastPeriod = 12;
      fastBeta = 1.0 - 0.15;
   }
   else
      fastBeta = ((double)(optInFastPeriod - 1)) / ((double)(optInFastPeriod + 1));
   fastK = 1.0 - fastBeta;
   fastBeta = 1.0 - fastK;

   /* A signal period of 1 disables signal-line smoothing: the signal IS the
    * MACD line and the histogram is exactly zero. The recursion
    * below, at a k of 1.0 and a beta of 0.0, does not keep the sign of a
    * -0.0 line value; hence the explicit arm at each step.
    */
   signalBeta = ((double)(optInSignalPeriod - 1)) / ((double)(optInSignalPeriod + 1));
   signalK = 1.0 - signalBeta;
   signalBeta = 1.0 - signalK;
   lookbackSignal = ema_lookback( optInSignalPeriod );

   /* Move up the start index if there is not
    * enough initial data.
    */
   lookbackTotal =  lookbackSignal;
   lookbackTotal += ema_lookback( optInSlowPeriod );

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
    *  - EMA recursion: k*x + beta*prev, with ema.c's k and beta.
    *  - Each EMA is seeded with the sum of its first 'period'
    *    inputs, accumulated from 0.0 in input order, divided by
    *    the period. The fast and slow seed windows end on the
    *    same bar. The signal EMA is seeded the same way from the
    *    first 'signal period' MACD-line values.
    *
    * In-place (an output == inReal) is supported: outputs at
    * [outIdx] are written only after inReal[startIdx+outIdx] was
    * read.
    */

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
      prevFast = fastK * tempReal + fastBeta * prevFast;
      prevSlow = slowK * tempReal + slowBeta * prevSlow;
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
      prevFast = fastK * tempReal + fastBeta * prevFast;
      prevSlow = slowK * tempReal + slowBeta * prevSlow;
      macdValue = prevFast - prevSlow;
      prevSignal += macdValue;
   }
   prevSignal = prevSignal / optInSignalPeriod;

   /* Advance everything in lockstep through the unstable period
    * of the signal EMA, up to the first output bar.
    */
   while( today <= startIdx )
   {
      tempReal = inReal[today++];
      prevFast = fastK * tempReal + fastBeta * prevFast;
      prevSlow = slowK * tempReal + slowBeta * prevSlow;
      macdValue = prevFast - prevSlow;
      if( optInSignalPeriod == 1 )
         prevSignal = macdValue;
      else
         prevSignal = signalK * macdValue + signalBeta * prevSignal;
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
      prevFast = fastK * tempReal + fastBeta * prevFast;
      prevSlow = slowK * tempReal + slowBeta * prevSlow;
      macdValue = prevFast - prevSlow;
      if( optInSignalPeriod == 1 )
         prevSignal = macdValue;
      else
         prevSignal = signalK * macdValue + signalBeta * prevSignal;
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
