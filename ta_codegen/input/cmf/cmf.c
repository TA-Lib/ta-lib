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
 *  072126 MF,CC  First version (issue #134).
 */

int cmf_lookback(int optInTimePeriod)
{
   return optInTimePeriod-1;
}

TA_RetCode cmf(int startIdx, int endIdx,
   const double inHigh[],
   const double inLow[],
   const double inClose[],
   const double inVolume[],
   int optInTimePeriod,
   int *outBegIdx, int *outNBElement,
   double outReal[])
{
   double sumMFV, sumVol, high, low, close, tmp, mfv;
   int lookbackTotal, outIdx, i, today;

   /* Both the per-bar money flow volume and the volume that produced it are
    * carried in the circular buffer. Keeping the volume here rather than
    * re-reading inVolume[] at the trailing index is what makes outReal safe to
    * alias any input: once a bar has been consumed it is never read again.
    */
   typedef struct { double flow; double volume; } MoneyFlowVolume;
   CIRCBUF_PROLOG_CLASS( mfv, MoneyFlowVolume, 50 ); /* Id, Type, Static Size */

   *outBegIdx = 0;
   *outNBElement = 0;

   /* Identify the minimum number of price bar needed
    * to calculate at least one output.
    */
   lookbackTotal = optInTimePeriod-1;

   /* Move up the start index if there is not
    * enough initial data.
    */
   if( startIdx < lookbackTotal )
      startIdx = lookbackTotal;

   /* Make sure there is still something to evaluate. */
   if( startIdx > endIdx )
      return TA_SUCCESS;

   CIRCBUF_INIT_CLASS( mfv, MoneyFlowVolume, optInTimePeriod );

   outIdx = 0;

   /* Accumulate the money flow volume and the volume over the first
    * complete window, filling the circular buffer as we go.
    *
    * The per-bar multiplier is written exactly as in ta_AD.c so that the
    * Chaikin money flow volume has one definition in the library.
    */
   today = startIdx-lookbackTotal;
   sumMFV = 0.0;
   sumVol = 0.0;
   for( i=optInTimePeriod; i > 0; i-- )
   {
      high  = inHigh[today];
      low   = inLow[today];
      close = inClose[today];
      tmp   = high-low;

      if( tmp > 0.0 )
         mfv = (((close-low)-(high-close))/tmp)*inVolume[today];
      else
         mfv = 0.0;

      mfv_flow[mfv_Idx]   = mfv;
      mfv_volume[mfv_Idx] = inVolume[today];
      sumMFV += mfv;
      sumVol += inVolume[today];
      today++;

      CIRCBUF_NEXT(mfv);
   }

   /* The first full window is complete: emit its output for startIdx here,
    * then slide the window over the remaining bars below.
    *
    * A window whose volume is entirely zero has no money flow to distribute;
    * report 0.0 rather than propagating a division by zero (issue #112).
    */
   if( sumVol > 0.0 )
      outReal[outIdx++] = sumMFV/sumVol;
   else
      outReal[outIdx++] = 0.0;

   /* Now continue processing the remaining bars. */
   while( today <= endIdx )
   {
      sumMFV -= mfv_flow[mfv_Idx];
      sumVol -= mfv_volume[mfv_Idx];

      high  = inHigh[today];
      low   = inLow[today];
      close = inClose[today];
      tmp   = high-low;

      if( tmp > 0.0 )
         mfv = (((close-low)-(high-close))/tmp)*inVolume[today];
      else
         mfv = 0.0;

      mfv_flow[mfv_Idx]   = mfv;
      mfv_volume[mfv_Idx] = inVolume[today];
      sumMFV += mfv;
      sumVol += inVolume[today];
      today++;

      if( sumVol > 0.0 )
         outReal[outIdx++] = sumMFV/sumVol;
      else
         outReal[outIdx++] = 0.0;

      CIRCBUF_NEXT(mfv);
   }

   CIRCBUF_DESTROY(mfv);

   *outBegIdx = startIdx;
   *outNBElement = outIdx;

   return TA_SUCCESS;
}
