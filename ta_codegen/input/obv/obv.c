/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  AC       Angelo Ciceri
 *
 *
 * Change history:
 *
 *  MMDDYY BY   Description
 *  -------------------------------------------------------------------
 *  010802 MF   Template creation.
 *  052603 MF   Adapt code to compile with .NET Managed C++
 *  110206 AC   Change volume and open interest to double
 *
 */

int obv_lookback(void)
{
   /* This function have no lookback needed. */
   return 0;
}

TA_RetCode obv(int startIdx, int endIdx,
   const double inReal[],
   const double inVolume[],
   int *outBegIdx, int *outNBElement,
   double outReal[])
{
   int i;
   int outIdx;
   double prevReal, tempReal, prevOBV;

   prevOBV  = inVolume[startIdx];
   prevReal = inReal[startIdx];
   outIdx = 0;

   for(i=startIdx; i <= endIdx; i++ )
   {
      tempReal = inReal[i];
      if( tempReal > prevReal )
         prevOBV += inVolume[i];
      else if( tempReal < prevReal )
         prevOBV -= inVolume[i];

      outReal[outIdx++] = prevOBV;
      prevReal = tempReal;
   }

   *outBegIdx = startIdx;
   *outNBElement = outIdx;

   return TA_SUCCESS;
}
