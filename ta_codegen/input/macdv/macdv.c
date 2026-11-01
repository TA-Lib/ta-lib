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
 *  103126 MF,CC  Creation (#570).
 */

int macdv_lookback(int optInFastPeriod, int optInSlowPeriod, int optInSignalPeriod)
{
   int tempInteger, lookbackLine, lookbackATR;

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

   /* The line needs the slow EMA and the Average True Range on the same bar,
    * so its first bar is the later of their two lookbacks. Each term is
    * exactly the lookback of the function it comes from and is never restated
    * here, which is what makes MACDV inherit TA_FUNC_UNST_EMA and
    * TA_FUNC_UNST_ATR from its callees.
    */
   lookbackLine = ema_lookback( optInSlowPeriod );
   lookbackATR  = atr_lookback( optInSlowPeriod );
   if( lookbackATR > lookbackLine )
      lookbackLine = lookbackATR;

   return lookbackLine + ema_lookback( optInSignalPeriod );
}

TA_RetCode macdv(int startIdx, int endIdx,
   const double inHigh[],
   const double inLow[],
   const double inClose[],
   int optInFastPeriod,
   int optInSlowPeriod,
   int optInSignalPeriod,
   int *outBegIdx, int *outNBElement,
   double outMACDV[],
   double outMACDVSignal[],
   double outMACDVHist[])
{
   double prevFast, prevSlow, prevATR, prevSignal, lineValue, tempReal;
   double slowK, fastK, signalK;
   double slowBeta, fastBeta, signalBeta;
   double wAlpha, wBeta, periodTotal;
   double val2, val3, greatest, spread, bandScale, periodScale;
   double tempCY, tempLT, tempHT;
   int i, today, outIdx, tempInteger;
   int lookbackTotal, lookbackSignal, lookbackSlow, lookbackFast, lookbackATR;
   int lineIdx;

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

   /* ema.c's k and beta for the three EMA, macd.c's spelling. */
   slowBeta = ((double)(optInSlowPeriod - 1)) / ((double)(optInSlowPeriod + 1));
   slowK = 1.0 - slowBeta;
   if( slowBeta < 0.5 ) slowBeta = 1.0 - slowK;

   fastBeta = ((double)(optInFastPeriod - 1)) / ((double)(optInFastPeriod + 1));
   fastK = 1.0 - fastBeta;
   if( fastBeta < 0.5 ) fastBeta = 1.0 - fastK;

   /* A signal period of 1 disables signal-line smoothing: the signal IS the
    * MACD-V line and the histogram is exactly zero. The recursion below, at a
    * k of 1.0 and a beta of 0.0, does not keep the sign of a -0.0 line value;
    * hence the explicit arm at each step.
    */
   signalBeta = ((double)(optInSignalPeriod - 1)) / ((double)(optInSignalPeriod + 1));
   signalK = 1.0 - signalBeta;
   if( signalBeta < 0.5 ) signalBeta = 1.0 - signalK;

   /* atr.c's Wilder coefficients: wAlpha derived FROM wBeta, never the
    * reverse, so that the pair is the one TA_ATR runs.
    */
   wBeta  = (double)(optInSlowPeriod - 1) / (double)optInSlowPeriod;
   wAlpha = 1.0 - wBeta;

   /* An EMA of a constant can settle short of it by a number of ulps that
    * grows in proportion to the period.
    */
   periodScale = (double)(optInSlowPeriod + 1);

   lookbackSignal = ema_lookback( optInSignalPeriod );
   lookbackSlow   = ema_lookback( optInSlowPeriod );
   lookbackFast   = ema_lookback( optInFastPeriod );
   lookbackATR    = atr_lookback( optInSlowPeriod );
   lookbackTotal  = macdv_lookback( optInFastPeriod, optInSlowPeriod, optInSignalPeriod );

   /* Move up the start index if there is not
    * enough initial data.
    */
   if( startIdx < lookbackTotal )
      startIdx = lookbackTotal;

   /* Make sure there is still something to evaluate. */
   if( startIdx > endIdx )
   {
      *outBegIdx = 0;
      *outNBElement = 0;
      return TA_SUCCESS;
   }

   /* Everything is computed in a single lockstep pass: each bar advances the
    * fast EMA, the slow EMA and the Average True Range, the scaled quotient
    * is the MACD-V line, and each line value is immediately fed into the
    * signal EMA. No temporary buffers.
    *
    * The arithmetic order below is the bit-exactness contract with TA_EMA,
    * TA_MACD and TA_ATR (do not reorder or fuse operations):
    *  - EMA recursion: k*x + beta*prev, with ema.c's k and beta.
    *  - True Range: start from high-low, then compare/replace with the two
    *    previous-close distances, in that order; Wilder smoothing in ONE
    *    statement.
    *  - Each recursion is seeded with the sum of its first 'period' inputs,
    *    accumulated from 0.0 in input order, divided by the period. The
    *    signal EMA is seeded the same way from the first 'signal period'
    *    line values.
    *  - The line is the spread divided by the Average True Range, then
    *    scaled: what a caller dividing TA_MACD by TA_ATR computes.
    *
    * In-place (an output == an input) is supported: outputs at [outIdx] are
    * written only after every input at or before bar startIdx+outIdx was
    * read, and the previous close is re-read from the input one bar back,
    * which no output store has reached.
    */

   /* Each leg is seeded at its OWN lookback and warmed up to lineIdx, the
    * first MACD-V line bar, so each is the shipped function entered at that
    * bar and nothing here over-warms the shorter one: a caller who wants a
    * leg converged sets that function's unstable period, exactly as when
    * calling it directly.
    */
   lineIdx = startIdx - lookbackSignal;

   /* Slow EMA. */
   today = lineIdx - lookbackSlow;
   tempReal = 0.0;
   i = optInSlowPeriod;
   while( i-- > 0 )
      tempReal += inClose[today++];
   prevSlow = tempReal / optInSlowPeriod;
   while( today <= lineIdx )
   {
      tempReal = inClose[today++];
      prevSlow = slowK * tempReal + slowBeta * prevSlow;
   }

   /* Fast EMA. */
   today = lineIdx - lookbackFast;
   tempReal = 0.0;
   i = optInFastPeriod;
   while( i-- > 0 )
      tempReal += inClose[today++];
   prevFast = tempReal / optInFastPeriod;
   while( today <= lineIdx )
   {
      tempReal = inClose[today++];
      prevFast = fastK * tempReal + fastBeta * prevFast;
   }

   /* Average True Range: the first True Range consumes a previous close. */
   today = lineIdx - lookbackATR + 1;
   periodTotal = 0.0;
   i = optInSlowPeriod;
   while( i-- > 0 )
   {
      tempLT = inLow[today];
      tempHT = inHigh[today];
      tempCY = inClose[today-1];
      greatest = tempHT - tempLT; /* val1 */

      val2 = fabs( tempCY - tempHT );
      if( val2 > greatest )
         greatest = val2;

      val3 = fabs( tempCY - tempLT );
      if( val3 > greatest )
         greatest = val3;

      periodTotal += greatest;
      today++;
   }
   prevATR = periodTotal / optInSlowPeriod;
   while( today <= lineIdx )
   {
      tempLT = inLow[today];
      tempHT = inHigh[today];
      tempCY = inClose[today-1];
      greatest = tempHT - tempLT; /* val1 */

      val2 = fabs( tempCY - tempHT );
      if( val2 > greatest )
         greatest = val2;

      val3 = fabs( tempCY - tempLT );
      if( val3 > greatest )
         greatest = val3;

      prevATR = wAlpha * greatest + wBeta * prevATR;
      today++;
   }

   /* Without price movement the line answers its neutral point, 0.0 (#570).
    * In a flat run the Wilder average only decays, to a subnormal floor,
    * while the spread stalls at the EMAs' rounding residue, so an exact test
    * lets their quotient grow until it overflows. Both are therefore tested
    * against the EMAs' own magnitude times the residue's period factor: that
    * also bounds |line| for any finite input, which a scale taken from the
    * close cannot, since a close may lie outside its bar and be zero.
    */
   spread = prevFast - prevSlow;
   bandScale = fabs( prevFast );
   if( fabs( prevSlow ) > bandScale )
      bandScale = fabs( prevSlow );
   bandScale = bandScale * periodScale;
   lineValue = 0.0;
   if( !TA_IS_ZERO_SCALED(spread, bandScale) && !TA_IS_ZERO_SCALED(prevATR, bandScale)
      && prevATR >= 2.2250738585072014e-308 )
   lineValue = spread / prevATR * 100.0;

   /* Seed the signal EMA with a simple average of the first 'signal period'
    * line values, accumulated as they are produced.
    */
   prevSignal = 0.0;
   prevSignal += lineValue;
   i = optInSignalPeriod-1;
   while( i-- > 0 )
   {
      tempLT = inLow[today];
      tempHT = inHigh[today];
      tempCY = inClose[today-1];
      tempReal = inClose[today];
      greatest = tempHT - tempLT; /* val1 */

      val2 = fabs( tempCY - tempHT );
      if( val2 > greatest )
         greatest = val2;

      val3 = fabs( tempCY - tempLT );
      if( val3 > greatest )
         greatest = val3;

      prevATR = wAlpha * greatest + wBeta * prevATR;
      prevFast = fastK * tempReal + fastBeta * prevFast;
      prevSlow = slowK * tempReal + slowBeta * prevSlow;
      spread = prevFast - prevSlow;
      bandScale = fabs( prevFast );
      if( fabs( prevSlow ) > bandScale )
         bandScale = fabs( prevSlow );
      bandScale = bandScale * periodScale;
      lineValue = 0.0;
      if( !TA_IS_ZERO_SCALED(spread, bandScale) && !TA_IS_ZERO_SCALED(prevATR, bandScale)
         && prevATR >= 2.2250738585072014e-308 )
      lineValue = spread / prevATR * 100.0;
      prevSignal += lineValue;
      today++;
   }
   prevSignal = prevSignal / optInSignalPeriod;

   /* Advance everything in lockstep through the unstable period of the
    * signal EMA, up to the first output bar.
    */
   while( today <= startIdx )
   {
      tempLT = inLow[today];
      tempHT = inHigh[today];
      tempCY = inClose[today-1];
      tempReal = inClose[today];
      greatest = tempHT - tempLT; /* val1 */

      val2 = fabs( tempCY - tempHT );
      if( val2 > greatest )
         greatest = val2;

      val3 = fabs( tempCY - tempLT );
      if( val3 > greatest )
         greatest = val3;

      prevATR = wAlpha * greatest + wBeta * prevATR;
      prevFast = fastK * tempReal + fastBeta * prevFast;
      prevSlow = slowK * tempReal + slowBeta * prevSlow;
      spread = prevFast - prevSlow;
      bandScale = fabs( prevFast );
      if( fabs( prevSlow ) > bandScale )
         bandScale = fabs( prevSlow );
      bandScale = bandScale * periodScale;
      lineValue = 0.0;
      if( !TA_IS_ZERO_SCALED(spread, bandScale) && !TA_IS_ZERO_SCALED(prevATR, bandScale)
         && prevATR >= 2.2250738585072014e-308 )
      lineValue = spread / prevATR * 100.0;
      if( optInSignalPeriod == 1 )
         prevSignal = lineValue;
      else
         prevSignal = signalK * lineValue + signalBeta * prevSignal;
      today++;
   }

   /* Stable zone: keep advancing in lockstep and write the three outputs. */
   outMACDV[0] = lineValue;
   outMACDVSignal[0] = prevSignal;
   outMACDVHist[0] = lineValue - prevSignal;
   outIdx = 1;
   while( today <= endIdx )
   {
      tempLT = inLow[today];
      tempHT = inHigh[today];
      tempCY = inClose[today-1];
      tempReal = inClose[today];
      greatest = tempHT - tempLT; /* val1 */

      val2 = fabs( tempCY - tempHT );
      if( val2 > greatest )
         greatest = val2;

      val3 = fabs( tempCY - tempLT );
      if( val3 > greatest )
         greatest = val3;

      prevATR = wAlpha * greatest + wBeta * prevATR;
      prevFast = fastK * tempReal + fastBeta * prevFast;
      prevSlow = slowK * tempReal + slowBeta * prevSlow;
      spread = prevFast - prevSlow;
      bandScale = fabs( prevFast );
      if( fabs( prevSlow ) > bandScale )
         bandScale = fabs( prevSlow );
      bandScale = bandScale * periodScale;
      lineValue = 0.0;
      if( !TA_IS_ZERO_SCALED(spread, bandScale) && !TA_IS_ZERO_SCALED(prevATR, bandScale)
         && prevATR >= 2.2250738585072014e-308 )
      lineValue = spread / prevATR * 100.0;
      if( optInSignalPeriod == 1 )
         prevSignal = lineValue;
      else
         prevSignal = signalK * lineValue + signalBeta * prevSignal;
      outMACDV[outIdx] = lineValue;
      outMACDVSignal[outIdx] = prevSignal;
      outMACDVHist[outIdx] = lineValue - prevSignal;
      outIdx++;
      today++;
   }

   /* All done! Indicate the output limits and return success. */
   *outBegIdx     = startIdx;
   *outNBElement  = outIdx;

   return TA_SUCCESS;
}
