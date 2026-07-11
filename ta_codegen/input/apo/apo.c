/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  AA       Andrew Atkinson
 *  CC       Claude Code (AI assistant)
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  112400 MF     Template creation.
 *  052603 MF     Adapt code to compile with .NET Managed C++
 *  062804 MF     Resolve div by zero bug on limit case.
 *  020605 AA     Fix #1117666 Lookback & out-of-bound bug.
 *  071126 MF,CC  Rewrite the combine into flat error-guards and a single-cursor
 *                offset index (offset = fastNb - *outNBElement). Bit-identical,
 *                streamable, and index-safe.
 */

int apo_lookback(int optInFastPeriod, int optInSlowPeriod, TA_MAType optInMAType)
{
   /* The slow MA is the key factor determining the lookback period. */
   return ma_lookback( max(optInSlowPeriod,optInFastPeriod), optInMAType );
}

TA_RetCode apo(int startIdx, int endIdx,
   const double inReal[],
   int optInFastPeriod,
   int optInSlowPeriod,
   TA_MAType optInMAType,
   int *outBegIdx, int *outNBElement,
   double outReal[])
{
   double *tempBuffer;
   TA_RetCode retCode;
   int tempInteger;
   int fastBeg, fastNb;
   int offset;
   int i;

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
      &fastBeg, &fastNb,
      tempBuffer );
   if( retCode != TA_SUCCESS )
   {
      free( tempBuffer );
      return retCode;
   }

   /* Calculate the slow MA into the output. */
   retCode = ma( startIdx, endIdx,
      inReal,
      optInSlowPeriod,
      optInMAType,
      outBegIdx, outNBElement,
      outReal );
   if( retCode != TA_SUCCESS )
   {
      free( tempBuffer );
      return retCode;
   }

   /* fastNb - *outNBElement == slowBeg - fastBeg (the fast MA has at least as
    * many outputs), so tempBuffer[i+offset] is the fast MA at the same bar as
    * outReal[i], with a non-negative index. An empty slow MA skips the loop.
    */
   offset = fastNb - *outNBElement;

   /* Calculate (fast MA)-(slow MA) in the output. */
   for( i=0; i < (int)*outNBElement; i++ )
      outReal[i] = tempBuffer[i+offset] - outReal[i];

   free( tempBuffer );

   return TA_SUCCESS;
}
