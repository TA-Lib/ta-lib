/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  EKO      echo999@ifrance.com
 *  CC       Claude Code (AI assistant)
 *
 * Change history:
 *
 *  MMDDYY BY    Description
 *  -------------------------------------------------------------------
 *  010802 MF    Template creation.
 *  051103 EKO   Found bug and fix related to outFastD.
 *  052603 MF    Adapt code to compile with .NET Managed C++
 *  071026 MF,CC Fix #107. Guard the Fast-K division with TA_IS_ZERO, not an
 *               exact `diff != 0.0`, so a machine-flat window yields 0 instead
 *               of dividing a sub-epsilon residue into [0,100] noise (STOCHRSI).
 *
 */

int stochf_lookback(int optInFastK_Period, int optInFastD_Period, TA_MAType optInFastD_MAType)
{
   int retValue;

   /* Account for the initial data needed for Fast-K. */
   retValue = (optInFastK_Period - 1);

   /* Add the smoothing being done for Fast-D */
   retValue += ma_lookback( optInFastD_Period, optInFastD_MAType );

   return retValue;
}

TA_RetCode stochf(int startIdx, int endIdx,
   const double inHigh[],
   const double inLow[],
   const double inClose[],
   int optInFastK_Period,
   int optInFastD_Period,
   TA_MAType optInFastD_MAType,
   int *outBegIdx, int *outNBElement,
   double outFastK[],
   double outFastD[])
{
   TA_RetCode retCode;
   double lowest, highest, tmp, diff;
   double *tempBuffer;
   int outIdx, lowestIdx, highestIdx;
   int lookbackTotal, lookbackK, lookbackFastD;
   int trailingIdx, today, i;

   int bufferIsAllocated;

   /* With stochastic, there is a total of 4 different lines that
    * are defined: FASTK, FASTD, SLOWK and SLOWD.
    *
    * The D is the signal line usually drawn over its
    * corresponding K function.
    *
    *                    (Today's Close - LowestLow)
    *  FASTK(Kperiod) =  --------------------------- * 100
    *                     (HighestHigh - LowestLow)
    *
    *  FASTD(FastDperiod, MA type) = MA Smoothed FASTK over FastDperiod
    *
    *  SLOWK(SlowKperiod, MA type) = MA Smoothed FASTK over SlowKperiod
    *
    *  SLOWD(SlowDperiod, MA Type) = MA Smoothed SLOWK over SlowDperiod
    *
    * The HighestHigh and LowestLow are the extreme values among the
    * last 'Kperiod'.
    *
    * SLOWK and FASTD are equivalent when using the same period.
    *
    * The following shows how these four lines are made available in TA-LIB:
    *
    *  TA_STOCH  : Returns the SLOWK and SLOWD
    *  TA_STOCHF : Returns the FASTK and FASTD
    *
    * The TA_STOCH function correspond to the more widely implemented version
    * found in many software/charting package. The TA_STOCHF is more rarely
    * used because its higher volatility cause often whipsaws.
    */

   /* Identify the lookback needed. */
   lookbackK      = optInFastK_Period-1;
   lookbackFastD = ma_lookback( optInFastD_Period, optInFastD_MAType );
   lookbackTotal  = lookbackK + lookbackFastD;

   /* Move up the start index if there is not
    * enough initial data.
    */
   if( startIdx < lookbackTotal )
      startIdx = lookbackTotal;

   /* Make sure there is still something to evaluate. */
   if( startIdx > endIdx )
   {
      /* Succeed... but no data in the output. */
      *outBegIdx = 0;
      *outNBElement = 0;
      return TA_SUCCESS;
   }

   /* Do the K calculation:
    *
    *    Kt = 100 x ((Ct-Lt)/(Ht-Lt))
    *
    * Kt is today stochastic
    * Ct is today closing price.
    * Lt is the lowest price of the last K Period (including today)
    * Ht is the highest price of the last K Period (including today)
    */

   /* Proceed with the calculation for the requested range.
    * Note that this algorithm allows the input and
    * output to be the same buffer.
    */
   outIdx = 0;

   /* Calculate just enough K for ending up with the caller
    * requested range. (The range of k must consider all
    * the lookback involve with the smoothing).
    */
   trailingIdx = startIdx-lookbackTotal;
   today       = trailingIdx+lookbackK;
   lowestIdx   = highestIdx = -1;
   diff = highest = lowest  = 0.0;

   /* Allocate a temporary buffer large enough to
    * store the K.
    *
    * If the output is the same as the input, great
    * we just save ourself one memory allocation.
    */
   bufferIsAllocated = 0;

   if( (outFastK == inHigh) ||
      (outFastK == inLow)  ||
      (outFastK == inClose) )
   {
      tempBuffer = outFastK;
   }
   else if( (outFastD == inHigh) ||
      (outFastD == inLow)  ||
      (outFastD == inClose) )
   {
      tempBuffer = outFastD;
   }
   else
   {
      bufferIsAllocated = 1;
      tempBuffer = malloc((endIdx-today+1) * sizeof(double));
   }

   /* Do the K calculation */
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
         diff = (highest - lowest)/100.0;
      }
      else if( tmp <= lowest )
      {
         lowestIdx = today;
         lowest = tmp;
         diff = (highest - lowest)/100.0;
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
         diff = (highest - lowest)/100.0;
      }
      else if( tmp >= highest )
      {
         highestIdx = today;
         highest = tmp;
         diff = (highest - lowest)/100.0;
      }

      /* Calculate stochastic. Guard with TA_IS_ZERO, not an exact `diff != 0.0`:
       * a machine-flat window leaves a sub-epsilon residue that an exact check
       * would divide into [0,100] noise (issue #107 / STOCHRSI). */
      if( !TA_IS_ZERO(diff) )
         tempBuffer[outIdx++] = (inClose[today]-lowest)/diff;
      else
         tempBuffer[outIdx++] = 0.0;

      trailingIdx++;
      today++;
   }

   /* Fast-K calculation completed. This K calculation is returned
    * to the caller. It is smoothed to become Fast-D.
    */
   retCode = ma( 0, outIdx-1,
      tempBuffer, optInFastD_Period,
      optInFastD_MAType,
      outBegIdx, outNBElement, outFastD );

   if( (retCode != TA_SUCCESS ) || ((int)*outNBElement) == 0 )
   {
      if (bufferIsAllocated) { free(tempBuffer); }
         /* Something wrong happen? No further data? */
      *outBegIdx = 0;
      *outNBElement = 0;
      return retCode;
   }

   /* Copy tempBuffer into the caller buffer.
    * (Calculation could not be done directly in the
    *  caller buffer because more input data then the
    *  requested range was needed for doing %D).
    */
   /* memmove, not memcpy: tempBuffer aliases outFastK when the caller buffer is
    * reused as scratch, so source and destination overlap (issue #94). */
   memmove(outFastK, &tempBuffer[lookbackFastD], ((int)*outNBElement) * sizeof(double));

   /* Don't need K anymore, free it if it was allocated here. */
   if (bufferIsAllocated) { free(tempBuffer); }

      if( retCode != TA_SUCCESS )
   {
      /* Something wrong happen while processing %D? */
      *outBegIdx = 0;
      *outNBElement = 0;
      return retCode;
   }

   /* Note: Keep the outBegIdx relative to the
    *       caller input before returning.
    */
   *outBegIdx = startIdx;

   return TA_SUCCESS;
}
