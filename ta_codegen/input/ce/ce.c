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
 *  103126 MF,CC  Creation (#568).
 */

int ce_lookback(int optInTimePeriod, double optInMultiplier)
{
   (void)optInMultiplier;

   /* The ATR term is never restated here, which is what makes CE inherit
    * TA_FUNC_UNST_ATR. It is at least the extreme window's own lookback.
    */
   return atr_lookback( optInTimePeriod );
}

TA_RetCode ce(int startIdx, int endIdx,
   const double inHigh[],
   const double inLow[],
   const double inClose[],
   int optInTimePeriod,
   double optInMultiplier,
   int *outBegIdx, int *outNBElement,
   double outRealLong[],
   double outRealShort[])
{
   CIRCBUF_PROLOG(hhBuf,double,30);
   CIRCBUF_PROLOG(llBuf,double,30);
   int i, ip, today, outIdx, lookbackTotal, lastP;

   double prevATR, periodTotal, wAlpha, wBeta;
   double val2, val3, greatest;
   double tempCY, tempLT, tempHT, tempReal;
   double hhPre, llPre, sufHi, sufLo;
   double highest, lowest, band;

   *outBegIdx = 0;
   *outNBElement = 0;

   lookbackTotal = ce_lookback( optInTimePeriod, optInMultiplier );

   if( startIdx < lookbackTotal )
      startIdx = lookbackTotal;

   /* Make sure there is still something to evaluate. */
   if( startIdx > endIdx )
      return TA_SUCCESS;

   /* Two rolling extrema, van Herk / Gil-Werman, one buffer each: the window
    * ending in slot j is the current block's prefix extremum joined with the
    * previous block's suffix extremum from slot j+1. A slot holds the raw value
    * while its block is filling and is turned into the suffix extremum, in
    * place, when the block completes; slot j+1 is always still the previous
    * block's when slot j is filled. The high and low sides share an index.
    *
    * The extrema are exact, so both outputs must stay bit-identical to
    * TA_MAX(high) - x*TA_ATR and its mirror, whatever the block phase; only
    * the sign of a zero tied between +0.0 and -0.0 is free.
    */
   CIRCBUF_INIT(hhBuf,double,optInTimePeriod);
   CIRCBUF_INIT(llBuf,double,optInTimePeriod);
   lastP = optInTimePeriod - 1;

   /* The Average True Range is carried inline rather than taken from a call,
    * so that the extreme window and the average advance together one bar at
    * a time and the body streams.
    *
    * The arithmetic order below is the bit-exactness contract with TA_ATR (do
    * not reorder): True Range from high-low, then the two previous-close
    * distances in that order; the seed summed from 0.0 over the first 'period'
    * True Ranges and divided once; the same two Wilder coefficients, wBeta
    * rounded and wAlpha derived from it, in one fused statement.
    */
   wBeta  = (double)(optInTimePeriod - 1) / (double)optInTimePeriod;
   wAlpha = 1.0 - wBeta;

   today = startIdx - lookbackTotal + 1;

   periodTotal = 0.0;
   i = optInTimePeriod;
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
   prevATR = periodTotal / optInTimePeriod;

   /* Skip the bars the lookback adds for the unstable period. Taking the count
    * from the lookback rather than naming the setting keeps the two from
    * disagreeing.
    */
   i = lookbackTotal - optInTimePeriod;
   while( i != 0 )
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
      i--;
   }

   /* prevATR is now the Average True Range of bar today-1, the first output
    * bar. Its extreme window is one whole block, taken straight from the
    * input.
    */
   highest = inHigh[today-1];
   lowest  = inLow[today-1];
   hhBuf[lastP] = highest;
   llBuf[lastP] = lowest;
   ip = lastP;
   while( ip > 0 )
   {
      ip--;
      tempHT = inHigh[today-1-lastP+ip];
      tempLT = inLow[today-1-lastP+ip];
      if( tempHT > highest )
         highest = tempHT;
      if( tempLT < lowest )
         lowest = tempLT;
      hhBuf[ip] = highest;
      llBuf[ip] = lowest;
   }
   hhPre = highest;
   llPre = lowest;

   /* The multiple of the ATR is formed on its own, never fused into the
    * offset, so that each line is TA_MAX - x*TA_ATR (or its mirror) as a
    * caller composing the three functions would compute it.
    */
   band = optInMultiplier * prevATR;
   outRealLong[0]  = highest - band;
   outRealShort[0] = lowest + band;
   outIdx = 1;

   while( today <= endIdx )
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

      /* Keep this shape: the prefix extreme selected in a local, the
       * block-start override after it, one store. A store made only when the
       * bar sets a new extreme, or the override ahead of the select, compiles
       * to a data-dependent branch in the stream step, where the prefix lives
       * in the handle.
       */
      highest = hhPre;
      lowest  = llPre;
      if( tempHT > highest )
         highest = tempHT;
      if( tempLT < lowest )
         lowest = tempLT;
      if( hhBuf_Idx == 0 )
      {
         highest = tempHT;
         lowest  = tempLT;
      }
      hhPre = highest;
      llPre = lowest;
      if( hhBuf_Idx < lastP )
      {
         tempReal = hhBuf[hhBuf_Idx + 1];
         if( tempReal > highest )
            highest = tempReal;
         tempReal = llBuf[hhBuf_Idx + 1];
         if( tempReal < lowest )
            lowest = tempReal;
      }
      hhBuf[hhBuf_Idx] = tempHT;
      llBuf[hhBuf_Idx] = tempLT;
      CIRCBUF_NEXT(hhBuf);

      band = optInMultiplier * prevATR;
      outRealLong[outIdx]  = highest - band;
      outRealShort[outIdx] = lowest + band;

      /* A completed block becomes its suffix extrema. Nothing this bar reads
       * them, so the pass stays below the output stores, where the peek
       * frame never runs it.
       */
      if( hhBuf_Idx == 0 )
      {
         sufHi = hhBuf[lastP];
         sufLo = llBuf[lastP];
         ip = lastP;
         while( ip > 1 )
         {
            ip--;
            tempReal = hhBuf[ip];
            if( tempReal > sufHi )
               sufHi = tempReal;
            hhBuf[ip] = sufHi;
            tempReal = llBuf[ip];
            if( tempReal < sufLo )
               sufLo = tempReal;
            llBuf[ip] = sufLo;
         }
      }
      outIdx++;
      today++;
   }

   CIRCBUF_DESTROY(hhBuf);
   CIRCBUF_DESTROY(llBuf);

   *outBegIdx    = startIdx;
   *outNBElement = outIdx;

   return TA_SUCCESS;
}
