/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  KL       Kevin Lin (@kevinlincg)
 *  MF       Mario Fortier
 *  CC       Claude Code (AI assistant)
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  092826 KL,CC  First version (#465).
 */

int emv_lookback(int optInTimePeriod, double optInVolumeDivisor)
{
   (void)optInVolumeDivisor;
   return 1 + sma_lookback(optInTimePeriod);
}

TA_RetCode emv(int startIdx, int endIdx,
   const double inHigh[],
   const double inLow[],
   const double inVolume[],
   int optInTimePeriod,
   double optInVolumeDivisor,
   int *outBegIdx, int *outNBElement,
   double outReal[])
{
   double periodTotal, prevMid, mid, range, boxRatio, raw, tempReal;
   int lookbackTotal, outIdx, i;

   /* The one-bar values are kept in a ring rather than recomputed at the
    * trailing index: outReal may alias an input, and those bars may already
    * hold outputs.
    */
   CIRCBUF_PROLOG(rawBuffer,double,50);

   lookbackTotal = emv_lookback( optInTimePeriod, optInVolumeDivisor );

   if( startIdx < lookbackTotal )
      startIdx = lookbackTotal;

   if( startIdx > endIdx )
   {
      *outBegIdx = 0;
      *outNBElement = 0;
      return TA_SUCCESS;
   }

   CIRCBUF_INIT( rawBuffer, double, optInTimePeriod );

   /* Keep the operand order and sma.c's add / snapshot / subtract order: the
    * composite gate rebuilds this from MEDPRICE, MOM, SUB, DIV and SMA and
    * compares bitwise.
    *
    * The zero guards are exact != 0.0 tests, never TA_IS_ZERO: the output
    * scale follows the volume and the divisor, so no absolute band fits. A
    * nonzero range with a box ratio that underflows to 0 takes the guard too.
    */
   i = startIdx - lookbackTotal;
   prevMid = (inHigh[i]+inLow[i])/2.0;
   i = i + 1;
   periodTotal = 0.0;
   while( i < startIdx )
   {
      mid   = (inHigh[i]+inLow[i])/2.0;
      range = inHigh[i]-inLow[i];
      raw   = 0.0;
      if( range != 0.0 )
      {
         boxRatio = (inVolume[i]/optInVolumeDivisor)/range;
         if( boxRatio != 0.0 )
            raw = (mid-prevMid)/boxRatio;
      }
      prevMid = mid;
      i = i + 1;

      rawBuffer[rawBuffer_Idx] = raw;
      periodTotal += raw;
      CIRCBUF_NEXT(rawBuffer);
   }

   outIdx = 0;
   while( i <= endIdx )
   {
      mid   = (inHigh[i]+inLow[i])/2.0;
      range = inHigh[i]-inLow[i];
      raw   = 0.0;
      if( range != 0.0 )
      {
         boxRatio = (inVolume[i]/optInVolumeDivisor)/range;
         if( boxRatio != 0.0 )
            raw = (mid-prevMid)/boxRatio;
      }
      prevMid = mid;
      i = i + 1;

      rawBuffer[rawBuffer_Idx] = raw;
      periodTotal += raw;
      tempReal = periodTotal;
      CIRCBUF_NEXT(rawBuffer);
      periodTotal -= rawBuffer[rawBuffer_Idx];

      outReal[outIdx] = tempReal / (double)optInTimePeriod;
      outIdx = outIdx + 1;
   }

   CIRCBUF_DESTROY(rawBuffer);

   *outBegIdx    = startIdx;
   *outNBElement = outIdx;

   return TA_SUCCESS;
}
