/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  JV       Jesus Viver <324122@cienz.unizar.es>
 *
 * Change history:
 *
 *  MMDDYY BY   Description
 *  -------------------------------------------------------------------
 *  112400 MF   Template creation.
 *  100502 JV   Speed optimization of the algorithm
 *  052603 MF   Adapt code to compile with .NET Managed C++
 *  090404 MF   Fix #978056. Trap sqrt with negative zero values.
 */

int stddev_lookback(int optInTimePeriod, double optInNbDev)
{
   /* Lookback is driven by the variance. */
   return var_lookback( optInTimePeriod, optInNbDev );
}

TA_RetCode stddev(int startIdx, int endIdx, const double inReal[], int optInTimePeriod, double optInNbDev, int *outBegIdx, int *outNBElement, double outReal[])
{
   int i;
   TA_RetCode retCode;
   double tempReal;

   /* Calculate the variance. */
   retCode = var( startIdx, endIdx,
      inReal, optInTimePeriod, 1.0,
      outBegIdx, outNBElement, outReal );

   if( retCode != TA_SUCCESS )
      return retCode;

   /* Calculate the square root of each variance, this
    * is the standard deviation.
    *
    * Multiply also by the ratio specified.
    */
   if( optInNbDev != 1.0 )
   {
      for( i=0; i < (int)*outNBElement; i++ )
      {
         tempReal = outReal[i];
         if( !TA_IS_ZERO_OR_NEG(tempReal) )
            outReal[i] = sqrt(tempReal) * optInNbDev;
         else
            outReal[i] = (double)0.0;
      }
   }
   else
   {
      for( i=0; i < (int)*outNBElement; i++ )
      {
         tempReal = outReal[i];
         if( !TA_IS_ZERO_OR_NEG(tempReal) )
            outReal[i] = sqrt(tempReal);
         else
            outReal[i] = (double)0.0;
      }
   }

   return TA_SUCCESS;
}

void stddev_using_precalc_ma( const double inReal[],
   const double inMovAvg[],
   int inMovAvgBegIdx,
   int inMovAvgNbElement,
   int timePeriod,
   double output[] )
{
   /* The inMovAvg is the moving average of the inReal.
    *
    * inMovAvgBegIdx is relative to inReal, in other word
    * this is the 'outBegIdx' who was returned when doing the
    * MA on the inReal.
    *
    * inMovAvgNbElement is the number of element who was returned
    * when doing the MA on the inReal.
    *
    * Note: This function is not used by TA_STDDEV, since TA_STDDEV
    *       is optimized considering it uses always a simple moving
    *       average. Still the function is put here because it is
    *       closely related.
    */
   double tempReal, periodTotal2, meanValue2;
   int outIdx;

   /* Start/end index for sumation. */
   int startSum, endSum;

   startSum = 1+inMovAvgBegIdx-timePeriod;
   endSum = inMovAvgBegIdx;

   periodTotal2 = 0;

   for( outIdx = startSum; outIdx < endSum; outIdx++)
   {
      tempReal = inReal[outIdx];
      tempReal *= tempReal;
      periodTotal2 += tempReal;
   }

   for( outIdx=0; outIdx < inMovAvgNbElement; outIdx++, startSum++, endSum++ )
   {
      tempReal = inReal[endSum];
      tempReal *= tempReal;
      periodTotal2 += tempReal;
      meanValue2 = periodTotal2/timePeriod;

      tempReal = inReal[startSum];
      tempReal *= tempReal;
      periodTotal2 -= tempReal;

      tempReal = inMovAvg[outIdx];
      tempReal *= tempReal;
      meanValue2 -= tempReal;

      if( !TA_IS_ZERO_OR_NEG(meanValue2) )
         output[outIdx] = sqrt(meanValue2);
      else
         output[outIdx] = (double)0.0;
   }
}
