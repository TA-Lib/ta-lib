/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  AA       Andrew Atkinson
 *
 * Change history:
 *
 *  MMDDYY BY   Description
 *  -------------------------------------------------------------------
 *  112400 MF   Template creation.
 *  052603 MF   Adapt code to compile with .NET Managed C++
 *  020605 AA   Fix #1117666 Lookback bug.
 *
 */

int ppo_lookback(int optInFastPeriod, int optInSlowPeriod, TA_MAType optInMAType)
{
   /* Lookback is driven by the slowest MA. */
   return ma_lookback( max(optInSlowPeriod,optInFastPeriod), optInMAType );
}

TA_RetCode ppo(int startIdx, int endIdx, const double inReal[], int optInFastPeriod, int optInSlowPeriod, TA_MAType optInMAType, int *outBegIdx, int *outNBElement, double outReal[])
{
   double *tempBuffer;
   TA_RetCode retCode;
   double tempReal;
   int tempInteger;
   int outBegIdx1, outNbElement1;
   int outBegIdx2, outNbElement2;
   int i, j;

   /* Allocate an intermediate buffer. */
   tempBuffer = malloc((endIdx-startIdx+1) * sizeof(double));
   if( !tempBuffer )
      return TA_ALLOC_ERR;

   /* Make sure slow is really slower than
    * the fast period! if not, swap...
    */
   if( optInSlowPeriod < optInFastPeriod )
   {
      /* swap */
      tempInteger     = optInSlowPeriod;
      optInSlowPeriod = optInFastPeriod;
      optInFastPeriod = tempInteger;
   }

   /* Calculate the fast MA into the tempBuffer. */
   retCode = ma( startIdx, endIdx,
      inReal,
      optInFastPeriod,
      optInMAType,
      &outBegIdx2, &outNbElement2,
      tempBuffer );

   if( retCode == TA_SUCCESS )
   {
      /* Calculate the slow MA into the output. */
      retCode = ma( startIdx, endIdx,
         inReal,
         optInSlowPeriod,
         optInMAType,
         &outBegIdx1, &outNbElement1,
         outReal );

      if( retCode == TA_SUCCESS )
      {
         /* The slow MA begins at or after the fast MA, so the offset is
          * valid whenever the slow MA produced output. Guard it so the empty
          * case leaves the difference loop untouched. */
         if( outNbElement1 > 0 )
         {
            tempInteger = outBegIdx1 - outBegIdx2;
            /* Calculate ((fast MA)-(slow MA))/(slow MA) in the output. */
            for( i=0,j=tempInteger; i < outNbElement1; i++, j++ )
            {
               tempReal = outReal[i];
               if( !TA_IS_ZERO(tempReal) )
                  outReal[i] = ((tempBuffer[j]-tempReal)/tempReal)*100.0;
               else
                  outReal[i] = 0.0;
            }
         }

         *outBegIdx    = outBegIdx1;
         *outNBElement = outNbElement1;
      }
   }

   free(tempBuffer);

   return retCode;
}
