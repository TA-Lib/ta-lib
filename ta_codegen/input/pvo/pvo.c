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
 *  071626 MF,CC  Initial version (#119).
 *  092726 MF,CC  0 on a slow window of zero bars for the windowed MA types (#454).
 */

int pvo_lookback(int optInFastPeriod, int optInSlowPeriod, TA_MAType optInMAType)
{
   /* Lookback is driven by the slowest MA. */
   return ma_lookback( max(optInSlowPeriod,optInFastPeriod), optInMAType );
}

TA_RetCode pvo(int startIdx, int endIdx,
   const double inVolume[],
   int optInFastPeriod,
   int optInSlowPeriod,
   TA_MAType optInMAType,
   int *outBegIdx, int *outNBElement,
   double outReal[])
{
   double *tempBuffer;
   TA_RetCode retCode;
   double tempReal;
   int tempInteger;
   int fastBeg, fastNb;
   int offset;
   int slowLookback;
   int windowed;
   int zeroRun;
   int i;

   /* Nothing to produce: the range ends before the lookback. Return before
    * touching anything.
    *
    * Without this the fast MA below runs first, and its lookback is SMALLER
    * than pvo's own — so it reads the whole range and computes a result the
    * empty slow MA then discards. Observably identical (the slow MA's own early
    * return already yields 0,0 here), but it is the difference between "a range
    * that ends before the lookback reads nothing" being true of this function and
    * being false: with a caller-supplied inVolume that stops short of endIdx, that
    * discarded work is an out-of-bounds read. Pinned by the zero-length no-I/O
    * probe over every guarded core.
    */
   if( ma_lookback( max(optInSlowPeriod,optInFastPeriod), optInMAType ) > endIdx )
   {
      *outBegIdx = 0;
      *outNBElement = 0;
      return TA_SUCCESS;
   }

   /* Allocate an intermediate buffer. */
   tempBuffer = malloc((endIdx-startIdx+1) * sizeof(double));
   if( !tempBuffer )
   {
      *outBegIdx = 0;
      *outNBElement = 0;
      return TA_ALLOC_ERR;
   }

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
      inVolume,
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
      inVolume,
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

   /* A windowed slow MA (SMA, WMA, TRIMA, HMA) over bars that are all exactly
    * zero is exactly zero, but its running sums leave residue there that
    * TA_IS_ZERO does not catch, and residue over residue is noise where 0 is
    * documented. zeroRun counts the trailing zero bars, held at slowLookback once
    * the window is dead. The recursive MA types really are nonzero on such a
    * window, so they keep the plain loop.
    */
   slowLookback = ma_lookback( optInSlowPeriod, optInMAType );
   windowed = ( optInMAType == TA_MAType_SMA || optInMAType == TA_MAType_WMA ||
      optInMAType == TA_MAType_TRIMA || optInMAType == TA_MAType_HMA ) ? 1 : 0;
   zeroRun = 0;
   for( i = *outBegIdx - slowLookback; i < *outBegIdx; i++ )
      zeroRun = fabs(inVolume[i]) <= 0.0 ? zeroRun + 1 : 0;

   if( windowed != 0 )
   {
      for( i=0; i < (int)*outNBElement; i++ )
      {
         zeroRun = fabs(inVolume[*outBegIdx + i]) <= 0.0 ? zeroRun + 1 : 0;
         tempReal = outReal[i];
         if( zeroRun > slowLookback )
         {
            zeroRun = slowLookback;
            outReal[i] = 0.0;
         }
         else if( !TA_IS_ZERO(tempReal) )
            outReal[i] = ((tempBuffer[i+offset]-tempReal)/tempReal)*100.0;
         else
            outReal[i] = 0.0;
      }
   }
   else
   {
      /* Calculate ((fast MA)-(slow MA))/(slow MA) in the output. */
      for( i=0; i < (int)*outNBElement; i++ )
      {
         tempReal = outReal[i];
         if( !TA_IS_ZERO(tempReal) )
            outReal[i] = ((tempBuffer[i+offset]-tempReal)/tempReal)*100.0;
         else
            outReal[i] = 0.0;
      }
   }

   free( tempBuffer );

   return TA_SUCCESS;
}
