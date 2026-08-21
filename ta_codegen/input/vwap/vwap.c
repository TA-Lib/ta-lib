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
 *  082126 MF,CC  First version (issue #237).
 */

int vwap_lookback(void)
{
   /* Cumulative from the first bar of the requested range, so the very
    * first bar already has a complete answer and nothing is consumed
    * before it. Same shape as ta_AD.c and ta_OBV.c.
    */
   return 0;
}

TA_RetCode vwap(int startIdx, int endIdx,
   const double inHigh[],
   const double inLow[],
   const double inClose[],
   const double inVolume[],
   int *outBegIdx, int *outNBElement,
   double outReal[])
{
   double sumPV, sumV, typPrice, volume, tempReal, vwap;
   int outIdx, i;

   /* Volume Weighted Average Price: the average typical price paid per
    * unit of volume, accumulated from the first bar of the range.
    *
    *    VWAP = sum( TYPPRICE * Volume ) / sum( Volume )
    *
    * Every charting package anchors this to a trading session and resets
    * the two sums at each session boundary. TA-Lib takes no timestamp on
    * any function, so the anchor is the caller's choice of range: pass one
    * session's slice of bars to get that session's VWAP. This is how AD
    * and OBV, the other two cumulative volume functions, are already used
    * across session boundaries (issue #237).
    */
   sumPV = 0.0;
   sumV  = 0.0;
   vwap  = 0.0;

   outIdx = 0;
   for( i=startIdx; i <= endIdx; i++ )
   {
      /* The typical price is written exactly as in ta_TYPPRICE.c so that the
       * two agree bit for bit and this stays a true composite of it.
       */
      typPrice = ( inHigh[i] + inLow[i] + inClose[i] ) / 3.0;
      volume   = inVolume[i];

      /* The product is kept in its own statement so no compiler may contract
       * it into an FMA. Contracting here would make the C output disagree
       * with the Rust, Java and C# backends under the cross-language bitwise
       * gate. Same reason as in ta_codegen/input/vwma/vwma.c.
       */
      tempReal = typPrice * volume;
      sumPV += tempReal;
      sumV  += volume;

      /* Bars that traded nothing carry no weight, so a zero-volume bar in
       * the middle of a series leaves both sums untouched and repeats the
       * previous value on its own -- no arm needed for that.
       *
       * The arm below is for the one case the ratio cannot express: a
       * leading run of bars before any volume has traded, where there are
       * no weights at all and the weighted mean is undefined. The last
       * value computed is carried forward instead, which is 0.0 until the
       * first bar with volume. Volume is non-negative, so once the divisor
       * leaves zero it never returns and this arm cannot fire again.
       *
       * A successful call therefore never emits NaN or Inf (issue #112),
       * which is the divergence from pandas-ta-classic and from
       * trading-signals: the first emits NaN there, the second no bar at
       * all. Testing sumV rather than the bar's own volume also keeps a
       * negative divisor -- which no non-negative volume series can
       * produce -- out of a price-scale output, as ta_CMF.c does.
       */
      if( sumV > 0.0 )
         vwap = sumPV / sumV;

      outReal[outIdx++] = vwap;
   }

   *outBegIdx    = startIdx;
   *outNBElement = outIdx;

   return TA_SUCCESS;
}
