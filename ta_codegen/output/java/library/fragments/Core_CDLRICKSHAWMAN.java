/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  AC       Angelo Ciceri
 *
 *
 * Change history:
 *
 *  MMDDYY BY   Description
 *  -------------------------------------------------------------------
 *  011505 AC   Creation
 */

   public int cdlRickshawManLookback( )
   {
      int BodyDoji_rangeType = this.candleSettings[CandleSettingType.BodyDoji.ordinal()].rangeType.ordinal();
      int BodyDoji_avgPeriod = this.candleSettings[CandleSettingType.BodyDoji.ordinal()].avgPeriod;
      double BodyDoji_factor = this.candleSettings[CandleSettingType.BodyDoji.ordinal()].factor;
      int Near_rangeType = this.candleSettings[CandleSettingType.Near.ordinal()].rangeType.ordinal();
      int Near_avgPeriod = this.candleSettings[CandleSettingType.Near.ordinal()].avgPeriod;
      double Near_factor = this.candleSettings[CandleSettingType.Near.ordinal()].factor;
      int ShadowLong_rangeType = this.candleSettings[CandleSettingType.ShadowLong.ordinal()].rangeType.ordinal();
      int ShadowLong_avgPeriod = this.candleSettings[CandleSettingType.ShadowLong.ordinal()].avgPeriod;
      double ShadowLong_factor = this.candleSettings[CandleSettingType.ShadowLong.ordinal()].factor;
      return Math.max(Math.max(BodyDoji_avgPeriod, ShadowLong_avgPeriod), Near_avgPeriod) ;

   }
   public RetCode cdlRickshawMan( int startIdx,
                                  int endIdx,
                                  double inOpen[],
                                  double inHigh[],
                                  double inLow[],
                                  double inClose[],
                                  MInteger outBegIdx,
                                  MInteger outNBElement,
                                  int outInteger[] )
   {
      double BodyDojiPeriodTotal = 0;
      double ShadowLongPeriodTotal = 0;
      double NearPeriodTotal = 0;
      int i = 0;
      int outIdx = 0;
      int BodyDojiTrailingIdx = 0;
      int ShadowLongTrailingIdx = 0;
      int NearTrailingIdx = 0;
      int lookbackTotal = 0;
      int BodyDoji_rangeType = this.candleSettings[CandleSettingType.BodyDoji.ordinal()].rangeType.ordinal();
      int BodyDoji_avgPeriod = this.candleSettings[CandleSettingType.BodyDoji.ordinal()].avgPeriod;
      double BodyDoji_factor = this.candleSettings[CandleSettingType.BodyDoji.ordinal()].factor;
      int Near_rangeType = this.candleSettings[CandleSettingType.Near.ordinal()].rangeType.ordinal();
      int Near_avgPeriod = this.candleSettings[CandleSettingType.Near.ordinal()].avgPeriod;
      double Near_factor = this.candleSettings[CandleSettingType.Near.ordinal()].factor;
      int ShadowLong_rangeType = this.candleSettings[CandleSettingType.ShadowLong.ordinal()].rangeType.ordinal();
      int ShadowLong_avgPeriod = this.candleSettings[CandleSettingType.ShadowLong.ordinal()].avgPeriod;
      double ShadowLong_factor = this.candleSettings[CandleSettingType.ShadowLong.ordinal()].factor;
      if( startIdx < 0 ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      /* Identify the minimum number of price bar needed
       * to calculate at least one output.
       */
      lookbackTotal = cdlRickshawManLookback();
      /* Move up the start index if there is not
       * enough initial data.
       */
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      /* Do the calculation using tight loops. */
      /* Add-up the initial period, except for the last value. */
      BodyDojiPeriodTotal = 0;
      BodyDojiTrailingIdx = startIdx - BodyDoji_avgPeriod;
      ShadowLongPeriodTotal = 0;
      ShadowLongTrailingIdx = startIdx - ShadowLong_avgPeriod;
      NearPeriodTotal = 0;
      NearTrailingIdx = startIdx - Near_avgPeriod;
      i = BodyDojiTrailingIdx;
      while( i < startIdx ) {
         BodyDojiPeriodTotal += ((BodyDoji_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyDoji_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((BodyDoji_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)));
         i += 1;
      }
      i = ShadowLongTrailingIdx;
      while( i < startIdx ) {
         ShadowLongPeriodTotal += ((ShadowLong_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowLong_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((ShadowLong_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)));
         i += 1;
      }
      i = NearTrailingIdx;
      while( i < startIdx ) {
         NearPeriodTotal += ((Near_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((Near_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((Near_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)));
         i += 1;
      }
      /* Proceed with the calculation for the requested range.
       *
       * Must have:
       * - doji body
       * - two long shadows
       * - body near the midpoint of the high-low range
       * The meaning of "doji" and "near" is specified with TA_SetCandleSettings
       * outInteger is always positive (1 to 100) but this does not mean it is bullish: rickshaw man shows uncertainty
       */
      outIdx = 0;
      do {
         if( Math.abs(inClose[i] - inOpen[i]) <= ((BodyDoji_factor * (((BodyDoji_avgPeriod != 0) ? (BodyDojiPeriodTotal / BodyDoji_avgPeriod) : ((BodyDoji_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyDoji_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((BodyDoji_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)))) / ((BodyDoji_rangeType == 2) ? 2.0 : 1.0)))) && /* doji */
             (((inClose[i] >= inOpen[i]) ? inOpen[i] : inClose[i]) - inLow[i]) > ((ShadowLong_factor * (((ShadowLong_avgPeriod != 0) ? (ShadowLongPeriodTotal / ShadowLong_avgPeriod) : ((ShadowLong_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowLong_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((ShadowLong_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)))) / ((ShadowLong_rangeType == 2) ? 2.0 : 1.0)))) && /* long shadow */
             (inHigh[i] - ((inClose[i] >= inOpen[i]) ? inClose[i] : inOpen[i])) > ((ShadowLong_factor * (((ShadowLong_avgPeriod != 0) ? (ShadowLongPeriodTotal / ShadowLong_avgPeriod) : ((ShadowLong_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowLong_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((ShadowLong_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)))) / ((ShadowLong_rangeType == 2) ? 2.0 : 1.0)))) && /* long shadow */
             Math.min(inOpen[i], inClose[i]) <= inLow[i] + (inHigh[i] - inLow[i]) / 2 + ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((Near_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((Near_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) && Math.max(inOpen[i], inClose[i]) >= inLow[i] + (inHigh[i] - inLow[i]) / 2 - ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((Near_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((Near_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) ) /* body near midpoint */
         {
            outInteger[outIdx++] = 100;
         } else {
            outInteger[outIdx++] = 0;
         }
         /* add the current range and subtract the first range: this is done after the pattern recognition
          * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
          */
         BodyDojiPeriodTotal += ((BodyDoji_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyDoji_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((BodyDoji_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0))) - ((BodyDoji_rangeType == 0) ? (Math.abs(inClose[BodyDojiTrailingIdx] - inOpen[BodyDojiTrailingIdx])) : ((BodyDoji_rangeType == 1) ? (inHigh[BodyDojiTrailingIdx] - inLow[BodyDojiTrailingIdx]) : ((BodyDoji_rangeType == 2) ? ((inHigh[BodyDojiTrailingIdx] - inLow[BodyDojiTrailingIdx]) - Math.abs(inClose[BodyDojiTrailingIdx] - inOpen[BodyDojiTrailingIdx])) : 0.0)));
         ShadowLongPeriodTotal += ((ShadowLong_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowLong_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((ShadowLong_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0))) - ((ShadowLong_rangeType == 0) ? (Math.abs(inClose[ShadowLongTrailingIdx] - inOpen[ShadowLongTrailingIdx])) : ((ShadowLong_rangeType == 1) ? (inHigh[ShadowLongTrailingIdx] - inLow[ShadowLongTrailingIdx]) : ((ShadowLong_rangeType == 2) ? ((inHigh[ShadowLongTrailingIdx] - inLow[ShadowLongTrailingIdx]) - Math.abs(inClose[ShadowLongTrailingIdx] - inOpen[ShadowLongTrailingIdx])) : 0.0)));
         NearPeriodTotal += ((Near_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((Near_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((Near_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0))) - ((Near_rangeType == 0) ? (Math.abs(inClose[NearTrailingIdx] - inOpen[NearTrailingIdx])) : ((Near_rangeType == 1) ? (inHigh[NearTrailingIdx] - inLow[NearTrailingIdx]) : ((Near_rangeType == 2) ? ((inHigh[NearTrailingIdx] - inLow[NearTrailingIdx]) - Math.abs(inClose[NearTrailingIdx] - inOpen[NearTrailingIdx])) : 0.0)));
         i += 1;
         BodyDojiTrailingIdx += 1;
         ShadowLongTrailingIdx += 1;
         NearTrailingIdx += 1;
      } while( i <= endIdx );
      /* All done. Indicate the output limits and return. */
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   public RetCode cdlRickshawManUnguarded( int startIdx,
                                           int endIdx,
                                           double inOpen[],
                                           double inHigh[],
                                           double inLow[],
                                           double inClose[],
                                           MInteger outBegIdx,
                                           MInteger outNBElement,
                                           int outInteger[] )
   {
      double BodyDojiPeriodTotal = 0;
      double ShadowLongPeriodTotal = 0;
      double NearPeriodTotal = 0;
      int i = 0;
      int outIdx = 0;
      int BodyDojiTrailingIdx = 0;
      int ShadowLongTrailingIdx = 0;
      int NearTrailingIdx = 0;
      int lookbackTotal = 0;
      int BodyDoji_rangeType = this.candleSettings[CandleSettingType.BodyDoji.ordinal()].rangeType.ordinal();
      int BodyDoji_avgPeriod = this.candleSettings[CandleSettingType.BodyDoji.ordinal()].avgPeriod;
      double BodyDoji_factor = this.candleSettings[CandleSettingType.BodyDoji.ordinal()].factor;
      int Near_rangeType = this.candleSettings[CandleSettingType.Near.ordinal()].rangeType.ordinal();
      int Near_avgPeriod = this.candleSettings[CandleSettingType.Near.ordinal()].avgPeriod;
      double Near_factor = this.candleSettings[CandleSettingType.Near.ordinal()].factor;
      int ShadowLong_rangeType = this.candleSettings[CandleSettingType.ShadowLong.ordinal()].rangeType.ordinal();
      int ShadowLong_avgPeriod = this.candleSettings[CandleSettingType.ShadowLong.ordinal()].avgPeriod;
      double ShadowLong_factor = this.candleSettings[CandleSettingType.ShadowLong.ordinal()].factor;
      lookbackTotal = cdlRickshawManLookback();
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      BodyDojiPeriodTotal = 0;
      BodyDojiTrailingIdx = startIdx - BodyDoji_avgPeriod;
      ShadowLongPeriodTotal = 0;
      ShadowLongTrailingIdx = startIdx - ShadowLong_avgPeriod;
      NearPeriodTotal = 0;
      NearTrailingIdx = startIdx - Near_avgPeriod;
      i = BodyDojiTrailingIdx;
      while( i < startIdx ) {
         BodyDojiPeriodTotal += ((BodyDoji_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyDoji_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((BodyDoji_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)));
         i += 1;
      }
      i = ShadowLongTrailingIdx;
      while( i < startIdx ) {
         ShadowLongPeriodTotal += ((ShadowLong_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowLong_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((ShadowLong_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)));
         i += 1;
      }
      i = NearTrailingIdx;
      while( i < startIdx ) {
         NearPeriodTotal += ((Near_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((Near_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((Near_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)));
         i += 1;
      }
      outIdx = 0;
      do {
         if( Math.abs(inClose[i] - inOpen[i]) <= ((BodyDoji_factor * (((BodyDoji_avgPeriod != 0) ? (BodyDojiPeriodTotal / BodyDoji_avgPeriod) : ((BodyDoji_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyDoji_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((BodyDoji_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)))) / ((BodyDoji_rangeType == 2) ? 2.0 : 1.0)))) && (((inClose[i] >= inOpen[i]) ? inOpen[i] : inClose[i]) - inLow[i]) > ((ShadowLong_factor * (((ShadowLong_avgPeriod != 0) ? (ShadowLongPeriodTotal / ShadowLong_avgPeriod) : ((ShadowLong_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowLong_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((ShadowLong_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)))) / ((ShadowLong_rangeType == 2) ? 2.0 : 1.0)))) && (inHigh[i] - ((inClose[i] >= inOpen[i]) ? inClose[i] : inOpen[i])) > ((ShadowLong_factor * (((ShadowLong_avgPeriod != 0) ? (ShadowLongPeriodTotal / ShadowLong_avgPeriod) : ((ShadowLong_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowLong_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((ShadowLong_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)))) / ((ShadowLong_rangeType == 2) ? 2.0 : 1.0)))) && (Math.min(inOpen[i], inClose[i]) <= inLow[i] + (inHigh[i] - inLow[i]) / 2 + ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((Near_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((Near_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) && Math.max(inOpen[i], inClose[i]) >= inLow[i] + (inHigh[i] - inLow[i]) / 2 - ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((Near_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((Near_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0))))) ) {
            outInteger[outIdx++] = 100;
         } else {
            outInteger[outIdx++] = 0;
         }
         BodyDojiPeriodTotal += ((BodyDoji_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyDoji_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((BodyDoji_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0))) - ((BodyDoji_rangeType == 0) ? (Math.abs(inClose[BodyDojiTrailingIdx] - inOpen[BodyDojiTrailingIdx])) : ((BodyDoji_rangeType == 1) ? (inHigh[BodyDojiTrailingIdx] - inLow[BodyDojiTrailingIdx]) : ((BodyDoji_rangeType == 2) ? ((inHigh[BodyDojiTrailingIdx] - inLow[BodyDojiTrailingIdx]) - Math.abs(inClose[BodyDojiTrailingIdx] - inOpen[BodyDojiTrailingIdx])) : 0.0)));
         ShadowLongPeriodTotal += ((ShadowLong_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowLong_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((ShadowLong_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0))) - ((ShadowLong_rangeType == 0) ? (Math.abs(inClose[ShadowLongTrailingIdx] - inOpen[ShadowLongTrailingIdx])) : ((ShadowLong_rangeType == 1) ? (inHigh[ShadowLongTrailingIdx] - inLow[ShadowLongTrailingIdx]) : ((ShadowLong_rangeType == 2) ? ((inHigh[ShadowLongTrailingIdx] - inLow[ShadowLongTrailingIdx]) - Math.abs(inClose[ShadowLongTrailingIdx] - inOpen[ShadowLongTrailingIdx])) : 0.0)));
         NearPeriodTotal += ((Near_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((Near_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((Near_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0))) - ((Near_rangeType == 0) ? (Math.abs(inClose[NearTrailingIdx] - inOpen[NearTrailingIdx])) : ((Near_rangeType == 1) ? (inHigh[NearTrailingIdx] - inLow[NearTrailingIdx]) : ((Near_rangeType == 2) ? ((inHigh[NearTrailingIdx] - inLow[NearTrailingIdx]) - Math.abs(inClose[NearTrailingIdx] - inOpen[NearTrailingIdx])) : 0.0)));
         i += 1;
         BodyDojiTrailingIdx += 1;
         ShadowLongTrailingIdx += 1;
         NearTrailingIdx += 1;
      } while( i <= endIdx );
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   public RetCode cdlRickshawMan( int startIdx,
                                  int endIdx,
                                  float inOpen[],
                                  float inHigh[],
                                  float inLow[],
                                  float inClose[],
                                  MInteger outBegIdx,
                                  MInteger outNBElement,
                                  int outInteger[] )
   {
      double BodyDojiPeriodTotal = 0;
      double ShadowLongPeriodTotal = 0;
      double NearPeriodTotal = 0;
      int i = 0;
      int outIdx = 0;
      int BodyDojiTrailingIdx = 0;
      int ShadowLongTrailingIdx = 0;
      int NearTrailingIdx = 0;
      int lookbackTotal = 0;
      int BodyDoji_rangeType = this.candleSettings[CandleSettingType.BodyDoji.ordinal()].rangeType.ordinal();
      int BodyDoji_avgPeriod = this.candleSettings[CandleSettingType.BodyDoji.ordinal()].avgPeriod;
      double BodyDoji_factor = this.candleSettings[CandleSettingType.BodyDoji.ordinal()].factor;
      int Near_rangeType = this.candleSettings[CandleSettingType.Near.ordinal()].rangeType.ordinal();
      int Near_avgPeriod = this.candleSettings[CandleSettingType.Near.ordinal()].avgPeriod;
      double Near_factor = this.candleSettings[CandleSettingType.Near.ordinal()].factor;
      int ShadowLong_rangeType = this.candleSettings[CandleSettingType.ShadowLong.ordinal()].rangeType.ordinal();
      int ShadowLong_avgPeriod = this.candleSettings[CandleSettingType.ShadowLong.ordinal()].avgPeriod;
      double ShadowLong_factor = this.candleSettings[CandleSettingType.ShadowLong.ordinal()].factor;
      if( startIdx < 0 ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      lookbackTotal = cdlRickshawManLookback();
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      BodyDojiPeriodTotal = 0;
      BodyDojiTrailingIdx = startIdx - BodyDoji_avgPeriod;
      ShadowLongPeriodTotal = 0;
      ShadowLongTrailingIdx = startIdx - ShadowLong_avgPeriod;
      NearPeriodTotal = 0;
      NearTrailingIdx = startIdx - Near_avgPeriod;
      i = BodyDojiTrailingIdx;
      while( i < startIdx ) {
         BodyDojiPeriodTotal += ((BodyDoji_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((BodyDoji_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((BodyDoji_rangeType == 2) ? (((double)inHigh[i] - (double)inLow[i]) - Math.abs((double)inClose[i] - (double)inOpen[i])) : 0.0)));
         i += 1;
      }
      i = ShadowLongTrailingIdx;
      while( i < startIdx ) {
         ShadowLongPeriodTotal += ((ShadowLong_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((ShadowLong_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((ShadowLong_rangeType == 2) ? (((double)inHigh[i] - (double)inLow[i]) - Math.abs((double)inClose[i] - (double)inOpen[i])) : 0.0)));
         i += 1;
      }
      i = NearTrailingIdx;
      while( i < startIdx ) {
         NearPeriodTotal += ((Near_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((Near_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((Near_rangeType == 2) ? (((double)inHigh[i] - (double)inLow[i]) - Math.abs((double)inClose[i] - (double)inOpen[i])) : 0.0)));
         i += 1;
      }
      outIdx = 0;
      do {
         if( Math.abs((double)inClose[i] - (double)inOpen[i]) <= ((BodyDoji_factor * (((BodyDoji_avgPeriod != 0) ? (BodyDojiPeriodTotal / BodyDoji_avgPeriod) : ((BodyDoji_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((BodyDoji_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((BodyDoji_rangeType == 2) ? (((double)inHigh[i] - (double)inLow[i]) - Math.abs((double)inClose[i] - (double)inOpen[i])) : 0.0)))) / ((BodyDoji_rangeType == 2) ? 2.0 : 1.0)))) && ((((double)inClose[i] >= (double)inOpen[i]) ? (double)inOpen[i] : (double)inClose[i]) - (double)inLow[i]) > ((ShadowLong_factor * (((ShadowLong_avgPeriod != 0) ? (ShadowLongPeriodTotal / ShadowLong_avgPeriod) : ((ShadowLong_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((ShadowLong_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((ShadowLong_rangeType == 2) ? (((double)inHigh[i] - (double)inLow[i]) - Math.abs((double)inClose[i] - (double)inOpen[i])) : 0.0)))) / ((ShadowLong_rangeType == 2) ? 2.0 : 1.0)))) && ((double)inHigh[i] - (((double)inClose[i] >= (double)inOpen[i]) ? (double)inClose[i] : (double)inOpen[i])) > ((ShadowLong_factor * (((ShadowLong_avgPeriod != 0) ? (ShadowLongPeriodTotal / ShadowLong_avgPeriod) : ((ShadowLong_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((ShadowLong_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((ShadowLong_rangeType == 2) ? (((double)inHigh[i] - (double)inLow[i]) - Math.abs((double)inClose[i] - (double)inOpen[i])) : 0.0)))) / ((ShadowLong_rangeType == 2) ? 2.0 : 1.0)))) && (Math.min((double)inOpen[i], (double)inClose[i]) <= (double)inLow[i] + ((double)inHigh[i] - (double)inLow[i]) / 2 + ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((Near_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((Near_rangeType == 2) ? (((double)inHigh[i] - (double)inLow[i]) - Math.abs((double)inClose[i] - (double)inOpen[i])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) && Math.max((double)inOpen[i], (double)inClose[i]) >= (double)inLow[i] + ((double)inHigh[i] - (double)inLow[i]) / 2 - ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((Near_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((Near_rangeType == 2) ? (((double)inHigh[i] - (double)inLow[i]) - Math.abs((double)inClose[i] - (double)inOpen[i])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0))))) ) {
            outInteger[outIdx++] = 100;
         } else {
            outInteger[outIdx++] = 0;
         }
         BodyDojiPeriodTotal += ((BodyDoji_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((BodyDoji_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((BodyDoji_rangeType == 2) ? (((double)inHigh[i] - (double)inLow[i]) - Math.abs((double)inClose[i] - (double)inOpen[i])) : 0.0))) - ((BodyDoji_rangeType == 0) ? (Math.abs((double)inClose[BodyDojiTrailingIdx] - (double)inOpen[BodyDojiTrailingIdx])) : ((BodyDoji_rangeType == 1) ? ((double)inHigh[BodyDojiTrailingIdx] - (double)inLow[BodyDojiTrailingIdx]) : ((BodyDoji_rangeType == 2) ? (((double)inHigh[BodyDojiTrailingIdx] - (double)inLow[BodyDojiTrailingIdx]) - Math.abs((double)inClose[BodyDojiTrailingIdx] - (double)inOpen[BodyDojiTrailingIdx])) : 0.0)));
         ShadowLongPeriodTotal += ((ShadowLong_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((ShadowLong_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((ShadowLong_rangeType == 2) ? (((double)inHigh[i] - (double)inLow[i]) - Math.abs((double)inClose[i] - (double)inOpen[i])) : 0.0))) - ((ShadowLong_rangeType == 0) ? (Math.abs((double)inClose[ShadowLongTrailingIdx] - (double)inOpen[ShadowLongTrailingIdx])) : ((ShadowLong_rangeType == 1) ? ((double)inHigh[ShadowLongTrailingIdx] - (double)inLow[ShadowLongTrailingIdx]) : ((ShadowLong_rangeType == 2) ? (((double)inHigh[ShadowLongTrailingIdx] - (double)inLow[ShadowLongTrailingIdx]) - Math.abs((double)inClose[ShadowLongTrailingIdx] - (double)inOpen[ShadowLongTrailingIdx])) : 0.0)));
         NearPeriodTotal += ((Near_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((Near_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((Near_rangeType == 2) ? (((double)inHigh[i] - (double)inLow[i]) - Math.abs((double)inClose[i] - (double)inOpen[i])) : 0.0))) - ((Near_rangeType == 0) ? (Math.abs((double)inClose[NearTrailingIdx] - (double)inOpen[NearTrailingIdx])) : ((Near_rangeType == 1) ? ((double)inHigh[NearTrailingIdx] - (double)inLow[NearTrailingIdx]) : ((Near_rangeType == 2) ? (((double)inHigh[NearTrailingIdx] - (double)inLow[NearTrailingIdx]) - Math.abs((double)inClose[NearTrailingIdx] - (double)inOpen[NearTrailingIdx])) : 0.0)));
         i += 1;
         BodyDojiTrailingIdx += 1;
         ShadowLongTrailingIdx += 1;
         NearTrailingIdx += 1;
      } while( i <= endIdx );
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   public RetCode cdlRickshawManUnguarded( int startIdx,
                                           int endIdx,
                                           float inOpen[],
                                           float inHigh[],
                                           float inLow[],
                                           float inClose[],
                                           MInteger outBegIdx,
                                           MInteger outNBElement,
                                           int outInteger[] )
   {
      double BodyDojiPeriodTotal = 0;
      double ShadowLongPeriodTotal = 0;
      double NearPeriodTotal = 0;
      int i = 0;
      int outIdx = 0;
      int BodyDojiTrailingIdx = 0;
      int ShadowLongTrailingIdx = 0;
      int NearTrailingIdx = 0;
      int lookbackTotal = 0;
      int BodyDoji_rangeType = this.candleSettings[CandleSettingType.BodyDoji.ordinal()].rangeType.ordinal();
      int BodyDoji_avgPeriod = this.candleSettings[CandleSettingType.BodyDoji.ordinal()].avgPeriod;
      double BodyDoji_factor = this.candleSettings[CandleSettingType.BodyDoji.ordinal()].factor;
      int Near_rangeType = this.candleSettings[CandleSettingType.Near.ordinal()].rangeType.ordinal();
      int Near_avgPeriod = this.candleSettings[CandleSettingType.Near.ordinal()].avgPeriod;
      double Near_factor = this.candleSettings[CandleSettingType.Near.ordinal()].factor;
      int ShadowLong_rangeType = this.candleSettings[CandleSettingType.ShadowLong.ordinal()].rangeType.ordinal();
      int ShadowLong_avgPeriod = this.candleSettings[CandleSettingType.ShadowLong.ordinal()].avgPeriod;
      double ShadowLong_factor = this.candleSettings[CandleSettingType.ShadowLong.ordinal()].factor;
      lookbackTotal = cdlRickshawManLookback();
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      BodyDojiPeriodTotal = 0;
      BodyDojiTrailingIdx = startIdx - BodyDoji_avgPeriod;
      ShadowLongPeriodTotal = 0;
      ShadowLongTrailingIdx = startIdx - ShadowLong_avgPeriod;
      NearPeriodTotal = 0;
      NearTrailingIdx = startIdx - Near_avgPeriod;
      i = BodyDojiTrailingIdx;
      while( i < startIdx ) {
         BodyDojiPeriodTotal += ((BodyDoji_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((BodyDoji_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((BodyDoji_rangeType == 2) ? (((double)inHigh[i] - (double)inLow[i]) - Math.abs((double)inClose[i] - (double)inOpen[i])) : 0.0)));
         i += 1;
      }
      i = ShadowLongTrailingIdx;
      while( i < startIdx ) {
         ShadowLongPeriodTotal += ((ShadowLong_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((ShadowLong_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((ShadowLong_rangeType == 2) ? (((double)inHigh[i] - (double)inLow[i]) - Math.abs((double)inClose[i] - (double)inOpen[i])) : 0.0)));
         i += 1;
      }
      i = NearTrailingIdx;
      while( i < startIdx ) {
         NearPeriodTotal += ((Near_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((Near_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((Near_rangeType == 2) ? (((double)inHigh[i] - (double)inLow[i]) - Math.abs((double)inClose[i] - (double)inOpen[i])) : 0.0)));
         i += 1;
      }
      outIdx = 0;
      do {
         if( Math.abs((double)inClose[i] - (double)inOpen[i]) <= ((BodyDoji_factor * (((BodyDoji_avgPeriod != 0) ? (BodyDojiPeriodTotal / BodyDoji_avgPeriod) : ((BodyDoji_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((BodyDoji_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((BodyDoji_rangeType == 2) ? (((double)inHigh[i] - (double)inLow[i]) - Math.abs((double)inClose[i] - (double)inOpen[i])) : 0.0)))) / ((BodyDoji_rangeType == 2) ? 2.0 : 1.0)))) && ((((double)inClose[i] >= (double)inOpen[i]) ? (double)inOpen[i] : (double)inClose[i]) - (double)inLow[i]) > ((ShadowLong_factor * (((ShadowLong_avgPeriod != 0) ? (ShadowLongPeriodTotal / ShadowLong_avgPeriod) : ((ShadowLong_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((ShadowLong_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((ShadowLong_rangeType == 2) ? (((double)inHigh[i] - (double)inLow[i]) - Math.abs((double)inClose[i] - (double)inOpen[i])) : 0.0)))) / ((ShadowLong_rangeType == 2) ? 2.0 : 1.0)))) && ((double)inHigh[i] - (((double)inClose[i] >= (double)inOpen[i]) ? (double)inClose[i] : (double)inOpen[i])) > ((ShadowLong_factor * (((ShadowLong_avgPeriod != 0) ? (ShadowLongPeriodTotal / ShadowLong_avgPeriod) : ((ShadowLong_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((ShadowLong_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((ShadowLong_rangeType == 2) ? (((double)inHigh[i] - (double)inLow[i]) - Math.abs((double)inClose[i] - (double)inOpen[i])) : 0.0)))) / ((ShadowLong_rangeType == 2) ? 2.0 : 1.0)))) && (Math.min((double)inOpen[i], (double)inClose[i]) <= (double)inLow[i] + ((double)inHigh[i] - (double)inLow[i]) / 2 + ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((Near_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((Near_rangeType == 2) ? (((double)inHigh[i] - (double)inLow[i]) - Math.abs((double)inClose[i] - (double)inOpen[i])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) && Math.max((double)inOpen[i], (double)inClose[i]) >= (double)inLow[i] + ((double)inHigh[i] - (double)inLow[i]) / 2 - ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((Near_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((Near_rangeType == 2) ? (((double)inHigh[i] - (double)inLow[i]) - Math.abs((double)inClose[i] - (double)inOpen[i])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0))))) ) {
            outInteger[outIdx++] = 100;
         } else {
            outInteger[outIdx++] = 0;
         }
         BodyDojiPeriodTotal += ((BodyDoji_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((BodyDoji_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((BodyDoji_rangeType == 2) ? (((double)inHigh[i] - (double)inLow[i]) - Math.abs((double)inClose[i] - (double)inOpen[i])) : 0.0))) - ((BodyDoji_rangeType == 0) ? (Math.abs((double)inClose[BodyDojiTrailingIdx] - (double)inOpen[BodyDojiTrailingIdx])) : ((BodyDoji_rangeType == 1) ? ((double)inHigh[BodyDojiTrailingIdx] - (double)inLow[BodyDojiTrailingIdx]) : ((BodyDoji_rangeType == 2) ? (((double)inHigh[BodyDojiTrailingIdx] - (double)inLow[BodyDojiTrailingIdx]) - Math.abs((double)inClose[BodyDojiTrailingIdx] - (double)inOpen[BodyDojiTrailingIdx])) : 0.0)));
         ShadowLongPeriodTotal += ((ShadowLong_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((ShadowLong_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((ShadowLong_rangeType == 2) ? (((double)inHigh[i] - (double)inLow[i]) - Math.abs((double)inClose[i] - (double)inOpen[i])) : 0.0))) - ((ShadowLong_rangeType == 0) ? (Math.abs((double)inClose[ShadowLongTrailingIdx] - (double)inOpen[ShadowLongTrailingIdx])) : ((ShadowLong_rangeType == 1) ? ((double)inHigh[ShadowLongTrailingIdx] - (double)inLow[ShadowLongTrailingIdx]) : ((ShadowLong_rangeType == 2) ? (((double)inHigh[ShadowLongTrailingIdx] - (double)inLow[ShadowLongTrailingIdx]) - Math.abs((double)inClose[ShadowLongTrailingIdx] - (double)inOpen[ShadowLongTrailingIdx])) : 0.0)));
         NearPeriodTotal += ((Near_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((Near_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((Near_rangeType == 2) ? (((double)inHigh[i] - (double)inLow[i]) - Math.abs((double)inClose[i] - (double)inOpen[i])) : 0.0))) - ((Near_rangeType == 0) ? (Math.abs((double)inClose[NearTrailingIdx] - (double)inOpen[NearTrailingIdx])) : ((Near_rangeType == 1) ? ((double)inHigh[NearTrailingIdx] - (double)inLow[NearTrailingIdx]) : ((Near_rangeType == 2) ? (((double)inHigh[NearTrailingIdx] - (double)inLow[NearTrailingIdx]) - Math.abs((double)inClose[NearTrailingIdx] - (double)inOpen[NearTrailingIdx])) : 0.0)));
         i += 1;
         BodyDojiTrailingIdx += 1;
         ShadowLongTrailingIdx += 1;
         NearTrailingIdx += 1;
      } while( i <= endIdx );
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
