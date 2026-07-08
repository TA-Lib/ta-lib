/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  AM       Adrian Michel
 *  MIF      Mirek Fontan (mira@fontan.cz)
 *  CF       Christo Fogelberg
 *
 * Change history:
 *
 *  MMDDYY BY    Description
 *  -------------------------------------------------------------------
 *  010802 MF    Template creation.
 *  052603 MF    Adapt code to compile with .NET Managed C++
 *  082303 MF    Fix #792298. Remove rounding. Bug reported by AM.
 *  062704 MF    Fix #965557. Div by zero bug reported by MIF.
 *  122204 MF,CF Fix #1090231. Issues when period is 1.
 */

int plus_di_lookback(int optInTimePeriod)
{
   if( optInTimePeriod > 1 )
      return optInTimePeriod + TA_GetUnstablePeriod(TA_FUNC_UNST_PLUS_DI);
   else
      return 1;
}

TA_RetCode plus_di(int startIdx, int endIdx, const double inHigh[], const double inLow[], const double inClose[], int optInTimePeriod, int *outBegIdx, int *outNBElement, double outReal[])
{
   int today, lookbackTotal, outIdx;
   double prevHigh, prevLow, prevClose;
   double prevPlusDM, prevTR;
   double tempReal, tempReal2, diffP, diffM;

   int i;

   /*
    * The DM1 (one period) is base on the largest part of
    * today's range that is outside of yesterdays range.
    *
    * The following 7 cases explain how the +DM and -DM are
    * calculated on one period:
    *
    * Case 1:                       Case 2:
    *    C|                        A|
    *     |                         | C|
    *     | +DM1 = (C-A)           B|  | +DM1 = 0
    *     | -DM1 = 0                   | -DM1 = (B-D)
    * A|  |                           D|
    *  | D|
    * B|
    *
    * Case 3:                       Case 4:
    *    C|                           C|
    *     |                        A|  |
    *     | +DM1 = (C-A)            |  | +DM1 = 0
    *     | -DM1 = 0               B|  | -DM1 = (B-D)
    * A|  |                            |
    *  |  |                           D|
    * B|  |
    *    D|
    *
    * Case 5:                      Case 6:
    * A|                           A| C|
    *  | C| +DM1 = 0                |  |  +DM1 = 0
    *  |  | -DM1 = 0                |  |  -DM1 = 0
    *  | D|                         |  |
    * B|                           B| D|
    *
    *
    * Case 7:
    *
    *    C|
    * A|  |
    *  |  | +DM1=0
    * B|  | -DM1=0
    *    D|
    *
    * In case 3 and 4, the rule is that the smallest delta between
    * (C-A) and (B-D) determine which of +DM or -DM is zero.
    *
    * In case 7, (C-A) and (B-D) are equal, so both +DM and -DM are
    * zero.
    *
    * The rules remain the same when A=B and C=D (when the highs
    * equal the lows).
    *
    * When calculating the DM over a period > 1, the one-period DM
    * for the desired period are initialy sum. In other word,
    * for a -DM14, sum the -DM1 for the first 14 days (that's
    * 13 values because there is no DM for the first day!)
    * Subsequent DM are calculated using the Wilder's
    * smoothing approach:
    *
    *                                    Previous +DM14
    *  Today's +DM14 = Previous +DM14 -  -------------- + Today's +DM1
    *                                         14
    *
    * Calculation of a +DI14 is as follow:
    *
    *               +DM14
    *     +DI14 =  --------
    *                TR14
    *
    * Calculation of the TR14 is:
    *
    *                                   Previous TR14
    *    Today's TR14 = Previous TR14 - -------------- + Today's TR1
    *                                         14
    *
    *    The first TR14 is the summation of the first 14 TR1. See the
    *    TA_TRANGE function on how to calculate the true range.
    *
    * Reference:
    *    New Concepts In Technical Trading Systems, J. Welles Wilder Jr
    */

   /* Original implementation from Wilder's book was doing some integer
    * rounding in its calculations.
    *
    * This was understandable in the context that at the time the book
    * was written, most user were doing the calculation by hand.
    *
    * For a computer, rounding is unnecessary (and even problematic when inputs
    * are close to 1).
    *
    * TA-Lib does not do the rounding. Still, if you want to reproduce Wilder's examples,
    * you can comment out the following #undef/#define and rebuild the library.
    */
   if( optInTimePeriod > 1 )
      lookbackTotal = optInTimePeriod + TA_GetUnstablePeriod(TA_FUNC_UNST_PLUS_DI);
   else
      lookbackTotal = 1;

   /* Adjust startIdx to account for the lookback period. */
   if( startIdx < lookbackTotal )
      startIdx = lookbackTotal;

   /* Make sure there is still something to evaluate. */
   if( startIdx > endIdx )
   {
      *outBegIdx = 0;
      *outNBElement = 0;
      return TA_SUCCESS;
   }

   /* Indicate where the next output should be put
    * in the outReal.
    */
   outIdx = 0;

   /* Trap the case where no smoothing is needed. */
   if( optInTimePeriod <= 1 )
   {
      /* No smoothing needed. Just do the following:
       * for each price bar.
       *          +DM1
       *   +DI1 = ----
       *           TR1
       */
      *outBegIdx = startIdx;
      today = startIdx-1;
      prevHigh  = inHigh[today];
      prevLow   = inLow[today];
      prevClose = inClose[today];
      while( today < endIdx )
      {
         today++;
         tempReal = inHigh[today];
         diffP    = tempReal-prevHigh; /* Plus Delta */
         prevHigh = tempReal;
         tempReal = inLow[today];
         diffM    = prevLow-tempReal;   /* Minus Delta */
         prevLow  = tempReal;
         if( (diffP > 0) && (diffP > diffM) )
         {
            /* Case 1 and 3: +DM=diffP,-DM=0 */
            tempReal = ta_true_range(prevHigh, prevLow, prevClose);
            if( TA_IS_ZERO(tempReal) )
               outReal[outIdx++] = (double)0.0;
            else
               outReal[outIdx++] = diffP/tempReal;
         }
         else
            outReal[outIdx++] = (double)0.0;
         prevClose = inClose[today];
      }

      *outNBElement = outIdx;
      return TA_SUCCESS;
   }

   /* Process the initial DM and TR */
   *outBegIdx = today = startIdx;

   prevPlusDM  = 0.0;
   prevTR      = 0.0;
   today       = startIdx - lookbackTotal;
   prevHigh    = inHigh[today];
   prevLow     = inLow[today];
   prevClose   = inClose[today];
   i           = optInTimePeriod-1;
   while( i-- > 0 )
   {
      today++;
      tempReal = inHigh[today];
      diffP    = tempReal-prevHigh; /* Plus Delta */
      prevHigh = tempReal;

      tempReal = inLow[today];
      diffM    = prevLow-tempReal;   /* Minus Delta */
      prevLow  = tempReal;
      if( (diffP > 0) && (diffP > diffM) )
      {
         /* Case 1 and 3: +DM=diffP,-DM=0 */
         prevPlusDM += diffP;
      }

      tempReal = ta_true_range(prevHigh, prevLow, prevClose);
      prevTR += tempReal;
      prevClose = inClose[today];
   }

   /* Process subsequent DI */

   /* Skip the unstable period. Note that this loop must be executed
    * at least ONCE to calculate the first DI.
    */
   i = TA_GetUnstablePeriod(TA_FUNC_UNST_PLUS_DI) + 1;
   while( i-- != 0 )
   {
      /* Calculate the prevPlusDM */
      today++;
      tempReal = inHigh[today];
      diffP    = tempReal-prevHigh; /* Plus Delta */
      prevHigh = tempReal;
      tempReal = inLow[today];
      diffM    = prevLow-tempReal;   /* Minus Delta */
      prevLow  = tempReal;
      if( (diffP > 0) && (diffP > diffM) )
      {
         /* Case 1 and 3: +DM=diffP,-DM=0 */
         prevPlusDM = prevPlusDM - (prevPlusDM/optInTimePeriod) + diffP;
      }
      else
      {
         /* Case 2,4,5 and 7 */
         prevPlusDM = prevPlusDM - (prevPlusDM/optInTimePeriod);
      }

      /* Calculate the prevTR */
      tempReal = ta_true_range(prevHigh, prevLow, prevClose);
      prevTR = prevTR - (prevTR/optInTimePeriod) + tempReal;
      prevClose = inClose[today];
   }

   /* Now start to write the output in
    * the caller provided outReal.
    */

   if( !TA_IS_ZERO(prevTR) )
      outReal[0] = ta_round_pos(100.0*(prevPlusDM/prevTR));
   else
      outReal[0] = 0.0;
   outIdx = 1;

   while( today < endIdx )
   {
      /* Calculate the prevPlusDM */
      today++;
      tempReal = inHigh[today];
      diffP    = tempReal-prevHigh; /* Plus Delta */
      prevHigh = tempReal;
      tempReal = inLow[today];
      diffM    = prevLow-tempReal;   /* Minus Delta */
      prevLow  = tempReal;
      if( (diffP > 0) && (diffP > diffM) )
      {
         /* Case 1 and 3: +DM=diffP,-DM=0 */
         prevPlusDM = prevPlusDM - (prevPlusDM/optInTimePeriod) + diffP;
      }
      else
      {
         /* Case 2,4,5 and 7 */
         prevPlusDM = prevPlusDM - (prevPlusDM/optInTimePeriod);
      }

      /* Calculate the prevTR */
      tempReal = ta_true_range(prevHigh, prevLow, prevClose);
      prevTR = prevTR - (prevTR/optInTimePeriod) + tempReal;
      prevClose = inClose[today];

      /* Calculate the DI. The value is rounded (see Wilder book). */
      if( !TA_IS_ZERO(prevTR) )
         outReal[outIdx++] = ta_round_pos(100.0*(prevPlusDM/prevTR));
      else
         outReal[outIdx++] = 0.0;

   }

   *outNBElement = outIdx;

   return TA_SUCCESS;
}
