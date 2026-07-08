/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *
 *
 * Change history:
 *
 *  MMDDYY BY   Description
 *  -------------------------------------------------------------------
 *  010802 MF   Template creation.
 *  052603 MF   Adapt code to compile with .NET Managed C++
 *
 */

int minus_dm_lookback(int optInTimePeriod)
{
   if( optInTimePeriod > 1 )
      return optInTimePeriod + TA_GetUnstablePeriod(TA_FUNC_UNST_MINUS_DM) - 1;
   else
      return 1;
}

TA_RetCode minus_dm(int startIdx, int endIdx,
   const double inHigh[],
   const double inLow[],
   int optInTimePeriod,
   int *outBegIdx, int *outNBElement,
   double outReal[])
{
   int today, lookbackTotal, outIdx;
   double prevHigh, prevLow, tempReal;
   double prevMinusDM;
   double diffP, diffM;
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
    *  |  | +DM=0
    * B|  | -DM=0
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
    *                                    Previous -DM14
    *  Today's -DM14 = Previous -DM14 -  -------------- + Today's -DM1
    *                                         14
    *
    * Reference:
    *    New Concepts In Technical Trading Systems, J. Welles Wilder Jr
    */

   if( optInTimePeriod > 1 )
      lookbackTotal = optInTimePeriod + TA_GetUnstablePeriod(TA_FUNC_UNST_MINUS_DM) - 1;
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
      /* No smoothing needed. Just do a simple DM1
       * for each price bar.
       */
      *outBegIdx = startIdx;
      today = startIdx-1;
      prevHigh = inHigh[today];
      prevLow  = inLow[today];
      while( today < endIdx )
      {
         today++;
         tempReal = inHigh[today];
         diffP    = tempReal-prevHigh; /* Plus Delta */
         prevHigh = tempReal;
         tempReal = inLow[today];
         diffM    = prevLow-tempReal;   /* Minus Delta */
         prevLow  = tempReal;
         if( (diffM > 0) && (diffP < diffM) )
         {
            /* Case 2 and 4: +DM=0,-DM=diffM */
            outReal[outIdx++] = diffM;
         }
         else
            outReal[outIdx++] = 0;
      }

      *outNBElement = outIdx;
      return TA_SUCCESS;
   }

   /* Process the initial DM */
   *outBegIdx = startIdx;

   prevMinusDM = 0.0;
   today       = startIdx - lookbackTotal;
   prevHigh    = inHigh[today];
   prevLow     = inLow[today];
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

      if( (diffM > 0) && (diffP < diffM) )
      {
         /* Case 2 and 4: +DM=0,-DM=diffM */
         prevMinusDM += diffM;
      }
   }

   /* Process subsequent DM */

   /* Skip the unstable period. */
   i = TA_GetUnstablePeriod(TA_FUNC_UNST_MINUS_DM);
   while( i-- != 0 )
   {
      today++;
      tempReal = inHigh[today];
      diffP    = tempReal-prevHigh; /* Plus Delta */
      prevHigh = tempReal;
      tempReal = inLow[today];
      diffM    = prevLow-tempReal;   /* Minus Delta */
      prevLow  = tempReal;
      if( (diffM > 0) && (diffP < diffM) )
      {
         /* Case 2 and 4: +DM=0,-DM=diffM */
         prevMinusDM = prevMinusDM - (prevMinusDM/optInTimePeriod) + diffM;
      }
      else
      {
         /* Case 1,3,5 and 7 */
         prevMinusDM = prevMinusDM - (prevMinusDM/optInTimePeriod);
      }
   }

   /* Now start to write the output in
    * the caller provided outReal.
    */
   outReal[0] = prevMinusDM;
   outIdx = 1;

   while( today < endIdx )
   {
      today++;
      tempReal = inHigh[today];
      diffP    = tempReal-prevHigh; /* Plus Delta */
      prevHigh = tempReal;
      tempReal = inLow[today];
      diffM    = prevLow-tempReal;  /* Minus Delta */
      prevLow  = tempReal;

      if( (diffM > 0) && (diffP < diffM) )
      {
         /* Case 2 and 4: +DM=0,-DM=diffM */
         prevMinusDM = prevMinusDM - (prevMinusDM/optInTimePeriod) + diffM;
      }
      else
      {
         /* Case 1,3,5 and 7 */
         prevMinusDM = prevMinusDM - (prevMinusDM/optInTimePeriod);
      }

      outReal[outIdx++] = prevMinusDM;
   }

   *outNBElement = outIdx;

   return TA_SUCCESS;
}
