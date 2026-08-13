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

   /**
    * Number of leading input bars {@link Core#CDLRICKSHAWMAN} consumes before
    * it can produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int CDLRICKSHAWMAN_Lookback( )
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
   RetCode CDLRICKSHAWMAN_Internal( int startIdx,
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
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      /* Identify the minimum number of price bar needed
       * to calculate at least one output.
       */
      lookbackTotal = CDLRICKSHAWMAN_Lookback();
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
   RetCode CDLRICKSHAWMAN_Internal( int startIdx,
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
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      lookbackTotal = CDLRICKSHAWMAN_Lookback();
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
   /**
    * Single-candle doji with two long shadows whose body sits near the midpoint
    * of the high-low range. It is a neutral indecision signal, not a
    * directional (bullish/bearish) reversal. A hit marks market
    * indecision/uncertainty; neutral, neither bullish nor bearish.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#CDLRICKSHAWMAN_Lookback} is a
    * <b>success with no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inOpen Open price of each bar.
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param inClose Close price of each bar.
    * @param outInteger +100 when the pattern is present, 0 otherwise. Never
    *        -100; the code notes the positive value does NOT imply bullish, it signals
    *        uncertainty. Must hold at least {@code endIdx - startIdx + 1} values.
    * @return The range written: {@code begIdx} is the first bar with a value,
    *        {@code count} how many were written.
    * @throws IndexOutOfBoundsException if {@code startIdx} or {@code endIdx} is
    *        negative or above {@link Core#MAX_INDEX}, or {@code endIdx < startIdx}.
    * @throws IllegalArgumentException if an optional parameter is outside its
    *        documented range, or two outputs share one array.
    * @throws NullPointerException if any input or output array is null.
    *
    * @see Core#CDLLONGLEGGEDDOJI
    * @see Core#CDLDOJI
    * @see Core#CDLHIGHWAVE
    */
   public OutRange CDLRICKSHAWMAN( int startIdx,
                                   int endIdx,
                                   double inOpen[],
                                   double inHigh[],
                                   double inLow[],
                                   double inClose[],
                                   int outInteger[] )
   {
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = CDLRICKSHAWMAN_Internal(startIdx, endIdx, inOpen, inHigh, inLow, inClose, outBegIdx, outNBElement, outInteger);
      if( retCode != RetCode.Success ) {
         throw failure("CDLRICKSHAWMAN", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Single-candle doji with two long shadows whose body sits near the midpoint
    * of the high-low range. It is a neutral indecision signal, not a
    * directional (bullish/bearish) reversal. A hit marks market
    * indecision/uncertainty; neutral, neither bullish nor bearish.
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#CDLRICKSHAWMAN_Lookback} is a
    * <b>success with no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inOpen Open price of each bar.
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param inClose Close price of each bar.
    * @param outInteger +100 when the pattern is present, 0 otherwise. Never
    *        -100; the code notes the positive value does NOT imply bullish, it signals
    *        uncertainty. Must hold at least {@code endIdx - startIdx + 1} values.
    * @return The range written: {@code begIdx} is the first bar with a value,
    *        {@code count} how many were written.
    * @throws IndexOutOfBoundsException if {@code startIdx} or {@code endIdx} is
    *        negative or above {@link Core#MAX_INDEX}, or {@code endIdx < startIdx}.
    * @throws IllegalArgumentException if an optional parameter is outside its
    *        documented range, or two outputs share one array.
    * @throws NullPointerException if any input or output array is null.
    *
    * @see Core#CDLLONGLEGGEDDOJI
    * @see Core#CDLDOJI
    * @see Core#CDLHIGHWAVE
    */
   public OutRange CDLRICKSHAWMAN( int startIdx,
                                   int endIdx,
                                   float inOpen[],
                                   float inHigh[],
                                   float inLow[],
                                   float inClose[],
                                   int outInteger[] )
   {
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = CDLRICKSHAWMAN_Internal(startIdx, endIdx, inOpen, inHigh, inLow, inClose, outBegIdx, outNBElement, outInteger);
      if( retCode != RetCode.Success ) {
         throw failure("CDLRICKSHAWMAN", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live CDLRICKSHAWMAN stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#CDLRICKSHAWMAN} over the same series.
    * Open with {@link Core#CDLRICKSHAWMAN_Open}; there is no close — the handle is
    * ordinary heap state, unreferenced handles are simply garbage-collected.
    * <p>Concurrency: a handle is single-writer — {@code update}, {@code peek},
    * {@code value} and {@code copy} must not race with an {@code update} on
    * the same handle. With no concurrent {@code update}, {@code peek}/
    * {@code value}/{@code copy} never write the handle and may be called
    * concurrently after safe publication. Independent handles (including
    * {@code copy()} results) are fully independent.
    * <p>Not serializable by design: to checkpoint, retain the history and
    * re-open — the result is bit-identical by contract.
    */
   public static final class CDLRICKSHAWMAN_Stream {
      Core core;
      double BodyDojiPeriodTotal;
      double ShadowLongPeriodTotal;
      double NearPeriodTotal;
      int ringPos_BodyDojiTrailingIdx;
      int ringCap_BodyDojiTrailingIdx;
      double[] ring_BodyDojiTrailingIdx_inOpen;
      double[] ring_BodyDojiTrailingIdx_inHigh;
      double[] ring_BodyDojiTrailingIdx_inLow;
      double[] ring_BodyDojiTrailingIdx_inClose;
      int ringPos_NearTrailingIdx;
      int ringCap_NearTrailingIdx;
      double[] ring_NearTrailingIdx_inOpen;
      double[] ring_NearTrailingIdx_inHigh;
      double[] ring_NearTrailingIdx_inLow;
      double[] ring_NearTrailingIdx_inClose;
      int ringPos_ShadowLongTrailingIdx;
      int ringCap_ShadowLongTrailingIdx;
      double[] ring_ShadowLongTrailingIdx_inOpen;
      double[] ring_ShadowLongTrailingIdx_inHigh;
      double[] ring_ShadowLongTrailingIdx_inLow;
      double[] ring_ShadowLongTrailingIdx_inClose;
      int cs_BodyDoji_rangeType;
      int cs_BodyDoji_avgPeriod;
      double cs_BodyDoji_factor;
      int cs_Near_rangeType;
      int cs_Near_avgPeriod;
      double cs_Near_factor;
      int cs_ShadowLong_rangeType;
      int cs_ShadowLong_avgPeriod;
      double cs_ShadowLong_factor;
      int cur_outInteger;
      OutRange fillRange = OutRange.EMPTY;

      CDLRICKSHAWMAN_Stream( Core core ) { this.core = core; }

      /**
       * The range filled by {@link Core#CDLRICKSHAWMAN_OpenAndFill}, or
       * {@link OutRange#EMPTY} when this handle came from a plain
       * {@code open} (which fills nothing). Never {@code null}; a
       * successful {@code openAndFill} always writes at least one value,
       * so {@link OutRange#isEmpty()} tells the two apart.
       */
      public OutRange fillRange() { return fillRange; }

      CDLRICKSHAWMAN_Stream( CDLRICKSHAWMAN_Stream other ) {
         this.core = other.core;
         this.BodyDojiPeriodTotal = other.BodyDojiPeriodTotal;
         this.ShadowLongPeriodTotal = other.ShadowLongPeriodTotal;
         this.NearPeriodTotal = other.NearPeriodTotal;
         this.ringPos_BodyDojiTrailingIdx = other.ringPos_BodyDojiTrailingIdx;
         this.ringCap_BodyDojiTrailingIdx = other.ringCap_BodyDojiTrailingIdx;
         this.ring_BodyDojiTrailingIdx_inOpen = other.ring_BodyDojiTrailingIdx_inOpen.clone();
         this.ring_BodyDojiTrailingIdx_inHigh = other.ring_BodyDojiTrailingIdx_inHigh.clone();
         this.ring_BodyDojiTrailingIdx_inLow = other.ring_BodyDojiTrailingIdx_inLow.clone();
         this.ring_BodyDojiTrailingIdx_inClose = other.ring_BodyDojiTrailingIdx_inClose.clone();
         this.ringPos_NearTrailingIdx = other.ringPos_NearTrailingIdx;
         this.ringCap_NearTrailingIdx = other.ringCap_NearTrailingIdx;
         this.ring_NearTrailingIdx_inOpen = other.ring_NearTrailingIdx_inOpen.clone();
         this.ring_NearTrailingIdx_inHigh = other.ring_NearTrailingIdx_inHigh.clone();
         this.ring_NearTrailingIdx_inLow = other.ring_NearTrailingIdx_inLow.clone();
         this.ring_NearTrailingIdx_inClose = other.ring_NearTrailingIdx_inClose.clone();
         this.ringPos_ShadowLongTrailingIdx = other.ringPos_ShadowLongTrailingIdx;
         this.ringCap_ShadowLongTrailingIdx = other.ringCap_ShadowLongTrailingIdx;
         this.ring_ShadowLongTrailingIdx_inOpen = other.ring_ShadowLongTrailingIdx_inOpen.clone();
         this.ring_ShadowLongTrailingIdx_inHigh = other.ring_ShadowLongTrailingIdx_inHigh.clone();
         this.ring_ShadowLongTrailingIdx_inLow = other.ring_ShadowLongTrailingIdx_inLow.clone();
         this.ring_ShadowLongTrailingIdx_inClose = other.ring_ShadowLongTrailingIdx_inClose.clone();
         this.cs_BodyDoji_rangeType = other.cs_BodyDoji_rangeType;
         this.cs_BodyDoji_avgPeriod = other.cs_BodyDoji_avgPeriod;
         this.cs_BodyDoji_factor = other.cs_BodyDoji_factor;
         this.cs_Near_rangeType = other.cs_Near_rangeType;
         this.cs_Near_avgPeriod = other.cs_Near_avgPeriod;
         this.cs_Near_factor = other.cs_Near_factor;
         this.cs_ShadowLong_rangeType = other.cs_ShadowLong_rangeType;
         this.cs_ShadowLong_avgPeriod = other.cs_ShadowLong_avgPeriod;
         this.cs_ShadowLong_factor = other.cs_ShadowLong_factor;
         this.cur_outInteger = other.cur_outInteger;
         this.fillRange = other.fillRange;
      }

      void copyFrom( CDLRICKSHAWMAN_Stream other ) {
         this.core = other.core;
         this.BodyDojiPeriodTotal = other.BodyDojiPeriodTotal;
         this.ShadowLongPeriodTotal = other.ShadowLongPeriodTotal;
         this.NearPeriodTotal = other.NearPeriodTotal;
         this.ringPos_BodyDojiTrailingIdx = other.ringPos_BodyDojiTrailingIdx;
         this.ringCap_BodyDojiTrailingIdx = other.ringCap_BodyDojiTrailingIdx;
         if( this.ring_BodyDojiTrailingIdx_inOpen != null && this.ring_BodyDojiTrailingIdx_inOpen.length == other.ring_BodyDojiTrailingIdx_inOpen.length ) {
            System.arraycopy( other.ring_BodyDojiTrailingIdx_inOpen, 0, this.ring_BodyDojiTrailingIdx_inOpen, 0, other.ring_BodyDojiTrailingIdx_inOpen.length );
         } else {
            this.ring_BodyDojiTrailingIdx_inOpen = other.ring_BodyDojiTrailingIdx_inOpen.clone();
         }
         if( this.ring_BodyDojiTrailingIdx_inHigh != null && this.ring_BodyDojiTrailingIdx_inHigh.length == other.ring_BodyDojiTrailingIdx_inHigh.length ) {
            System.arraycopy( other.ring_BodyDojiTrailingIdx_inHigh, 0, this.ring_BodyDojiTrailingIdx_inHigh, 0, other.ring_BodyDojiTrailingIdx_inHigh.length );
         } else {
            this.ring_BodyDojiTrailingIdx_inHigh = other.ring_BodyDojiTrailingIdx_inHigh.clone();
         }
         if( this.ring_BodyDojiTrailingIdx_inLow != null && this.ring_BodyDojiTrailingIdx_inLow.length == other.ring_BodyDojiTrailingIdx_inLow.length ) {
            System.arraycopy( other.ring_BodyDojiTrailingIdx_inLow, 0, this.ring_BodyDojiTrailingIdx_inLow, 0, other.ring_BodyDojiTrailingIdx_inLow.length );
         } else {
            this.ring_BodyDojiTrailingIdx_inLow = other.ring_BodyDojiTrailingIdx_inLow.clone();
         }
         if( this.ring_BodyDojiTrailingIdx_inClose != null && this.ring_BodyDojiTrailingIdx_inClose.length == other.ring_BodyDojiTrailingIdx_inClose.length ) {
            System.arraycopy( other.ring_BodyDojiTrailingIdx_inClose, 0, this.ring_BodyDojiTrailingIdx_inClose, 0, other.ring_BodyDojiTrailingIdx_inClose.length );
         } else {
            this.ring_BodyDojiTrailingIdx_inClose = other.ring_BodyDojiTrailingIdx_inClose.clone();
         }
         this.ringPos_NearTrailingIdx = other.ringPos_NearTrailingIdx;
         this.ringCap_NearTrailingIdx = other.ringCap_NearTrailingIdx;
         if( this.ring_NearTrailingIdx_inOpen != null && this.ring_NearTrailingIdx_inOpen.length == other.ring_NearTrailingIdx_inOpen.length ) {
            System.arraycopy( other.ring_NearTrailingIdx_inOpen, 0, this.ring_NearTrailingIdx_inOpen, 0, other.ring_NearTrailingIdx_inOpen.length );
         } else {
            this.ring_NearTrailingIdx_inOpen = other.ring_NearTrailingIdx_inOpen.clone();
         }
         if( this.ring_NearTrailingIdx_inHigh != null && this.ring_NearTrailingIdx_inHigh.length == other.ring_NearTrailingIdx_inHigh.length ) {
            System.arraycopy( other.ring_NearTrailingIdx_inHigh, 0, this.ring_NearTrailingIdx_inHigh, 0, other.ring_NearTrailingIdx_inHigh.length );
         } else {
            this.ring_NearTrailingIdx_inHigh = other.ring_NearTrailingIdx_inHigh.clone();
         }
         if( this.ring_NearTrailingIdx_inLow != null && this.ring_NearTrailingIdx_inLow.length == other.ring_NearTrailingIdx_inLow.length ) {
            System.arraycopy( other.ring_NearTrailingIdx_inLow, 0, this.ring_NearTrailingIdx_inLow, 0, other.ring_NearTrailingIdx_inLow.length );
         } else {
            this.ring_NearTrailingIdx_inLow = other.ring_NearTrailingIdx_inLow.clone();
         }
         if( this.ring_NearTrailingIdx_inClose != null && this.ring_NearTrailingIdx_inClose.length == other.ring_NearTrailingIdx_inClose.length ) {
            System.arraycopy( other.ring_NearTrailingIdx_inClose, 0, this.ring_NearTrailingIdx_inClose, 0, other.ring_NearTrailingIdx_inClose.length );
         } else {
            this.ring_NearTrailingIdx_inClose = other.ring_NearTrailingIdx_inClose.clone();
         }
         this.ringPos_ShadowLongTrailingIdx = other.ringPos_ShadowLongTrailingIdx;
         this.ringCap_ShadowLongTrailingIdx = other.ringCap_ShadowLongTrailingIdx;
         if( this.ring_ShadowLongTrailingIdx_inOpen != null && this.ring_ShadowLongTrailingIdx_inOpen.length == other.ring_ShadowLongTrailingIdx_inOpen.length ) {
            System.arraycopy( other.ring_ShadowLongTrailingIdx_inOpen, 0, this.ring_ShadowLongTrailingIdx_inOpen, 0, other.ring_ShadowLongTrailingIdx_inOpen.length );
         } else {
            this.ring_ShadowLongTrailingIdx_inOpen = other.ring_ShadowLongTrailingIdx_inOpen.clone();
         }
         if( this.ring_ShadowLongTrailingIdx_inHigh != null && this.ring_ShadowLongTrailingIdx_inHigh.length == other.ring_ShadowLongTrailingIdx_inHigh.length ) {
            System.arraycopy( other.ring_ShadowLongTrailingIdx_inHigh, 0, this.ring_ShadowLongTrailingIdx_inHigh, 0, other.ring_ShadowLongTrailingIdx_inHigh.length );
         } else {
            this.ring_ShadowLongTrailingIdx_inHigh = other.ring_ShadowLongTrailingIdx_inHigh.clone();
         }
         if( this.ring_ShadowLongTrailingIdx_inLow != null && this.ring_ShadowLongTrailingIdx_inLow.length == other.ring_ShadowLongTrailingIdx_inLow.length ) {
            System.arraycopy( other.ring_ShadowLongTrailingIdx_inLow, 0, this.ring_ShadowLongTrailingIdx_inLow, 0, other.ring_ShadowLongTrailingIdx_inLow.length );
         } else {
            this.ring_ShadowLongTrailingIdx_inLow = other.ring_ShadowLongTrailingIdx_inLow.clone();
         }
         if( this.ring_ShadowLongTrailingIdx_inClose != null && this.ring_ShadowLongTrailingIdx_inClose.length == other.ring_ShadowLongTrailingIdx_inClose.length ) {
            System.arraycopy( other.ring_ShadowLongTrailingIdx_inClose, 0, this.ring_ShadowLongTrailingIdx_inClose, 0, other.ring_ShadowLongTrailingIdx_inClose.length );
         } else {
            this.ring_ShadowLongTrailingIdx_inClose = other.ring_ShadowLongTrailingIdx_inClose.clone();
         }
         this.cs_BodyDoji_rangeType = other.cs_BodyDoji_rangeType;
         this.cs_BodyDoji_avgPeriod = other.cs_BodyDoji_avgPeriod;
         this.cs_BodyDoji_factor = other.cs_BodyDoji_factor;
         this.cs_Near_rangeType = other.cs_Near_rangeType;
         this.cs_Near_avgPeriod = other.cs_Near_avgPeriod;
         this.cs_Near_factor = other.cs_Near_factor;
         this.cs_ShadowLong_rangeType = other.cs_ShadowLong_rangeType;
         this.cs_ShadowLong_avgPeriod = other.cs_ShadowLong_avgPeriod;
         this.cs_ShadowLong_factor = other.cs_ShadowLong_factor;
         this.cur_outInteger = other.cur_outInteger;
         this.fillRange = other.fillRange;
      }

      /** {@code peek}'s reusable scratch — one per thread, see {@code copyFrom}. */
      private static final ThreadLocal<CDLRICKSHAWMAN_Stream> PEEK_SCRATCH = new ThreadLocal<>();

      /**
       * Commit one closed bar; always produces the new current value.
       * Never throws after a successful open; never allocates handle state.
       */
      public int update( double inOpen, double inHigh, double inLow, double inClose ) {
         core.CDLRICKSHAWMAN_StreamStep(this, inOpen, inHigh, inLow, inClose);
         return this.cur_outInteger;
      }

      /**
       * Evaluate a forming bar without committing — bit-identical to what the
       * next {@code update} with the same bar would return (it is the same
       * generated code, run on a copy). Never writes this handle, so peeks may
       * run concurrently with each other. It runs on a scratch handle held per thread and
       * reused, so the copy allocates nothing after the first peek of this
       * indicator on this thread. That scratch is retained for the life of
       * the thread.
       */
      public int peek( double inOpen, double inHigh, double inLow, double inClose ) {
         CDLRICKSHAWMAN_Stream scratch = PEEK_SCRATCH.get();
         if( scratch == null ) {
            scratch = new CDLRICKSHAWMAN_Stream(this);
            PEEK_SCRATCH.set(scratch);
         } else {
            scratch.copyFrom(this);
         }
         core.CDLRICKSHAWMAN_StreamStep(scratch, inOpen, inHigh, inLow, inClose);
         return scratch.cur_outInteger;
      }

      /**
       * The value at the most recently committed bar — the last history bar
       * right after open, then whatever the latest {@code update} returned.
       * A pure field read; {@code peek} does not change it.
       */
      public int value() {
         return this.cur_outInteger;
      }

      /**
       * An independent deep copy of this stream: both evolve separately from
       * here on (the Java rendering of the Rust handle's {@code Clone}).
       */
      public CDLRICKSHAWMAN_Stream copy() {
         return new CDLRICKSHAWMAN_Stream(this);
      }
   }
   void CDLRICKSHAWMAN_StreamStep( CDLRICKSHAWMAN_Stream sp, double inOpen, double inHigh, double inLow, double inClose )
   {
      int BodyDoji_rangeType = sp.cs_BodyDoji_rangeType;
      int BodyDoji_avgPeriod = sp.cs_BodyDoji_avgPeriod;
      double BodyDoji_factor = sp.cs_BodyDoji_factor;
      int Near_rangeType = sp.cs_Near_rangeType;
      int Near_avgPeriod = sp.cs_Near_avgPeriod;
      double Near_factor = sp.cs_Near_factor;
      int ShadowLong_rangeType = sp.cs_ShadowLong_rangeType;
      int ShadowLong_avgPeriod = sp.cs_ShadowLong_avgPeriod;
      double ShadowLong_factor = sp.cs_ShadowLong_factor;
      if( sp.ringCap_BodyDojiTrailingIdx == 0 ) {
         sp.ring_BodyDojiTrailingIdx_inOpen[0] = inOpen;
         sp.ring_BodyDojiTrailingIdx_inHigh[0] = inHigh;
         sp.ring_BodyDojiTrailingIdx_inLow[0] = inLow;
         sp.ring_BodyDojiTrailingIdx_inClose[0] = inClose;
      }
      if( sp.ringCap_NearTrailingIdx == 0 ) {
         sp.ring_NearTrailingIdx_inOpen[0] = inOpen;
         sp.ring_NearTrailingIdx_inHigh[0] = inHigh;
         sp.ring_NearTrailingIdx_inLow[0] = inLow;
         sp.ring_NearTrailingIdx_inClose[0] = inClose;
      }
      if( sp.ringCap_ShadowLongTrailingIdx == 0 ) {
         sp.ring_ShadowLongTrailingIdx_inOpen[0] = inOpen;
         sp.ring_ShadowLongTrailingIdx_inHigh[0] = inHigh;
         sp.ring_ShadowLongTrailingIdx_inLow[0] = inLow;
         sp.ring_ShadowLongTrailingIdx_inClose[0] = inClose;
      }
      if( Math.abs(inClose - inOpen) <= ((BodyDoji_factor * (((BodyDoji_avgPeriod != 0) ? (sp.BodyDojiPeriodTotal / BodyDoji_avgPeriod) : ((BodyDoji_rangeType == 0) ? (Math.abs(inClose - inOpen)) : ((BodyDoji_rangeType == 1) ? (inHigh - inLow) : ((BodyDoji_rangeType == 2) ? ((inHigh - inLow) - Math.abs(inClose - inOpen)) : 0.0)))) / ((BodyDoji_rangeType == 2) ? 2.0 : 1.0)))) && /* doji */
          (((inClose >= inOpen) ? inOpen : inClose) - inLow) > ((ShadowLong_factor * (((ShadowLong_avgPeriod != 0) ? (sp.ShadowLongPeriodTotal / ShadowLong_avgPeriod) : ((ShadowLong_rangeType == 0) ? (Math.abs(inClose - inOpen)) : ((ShadowLong_rangeType == 1) ? (inHigh - inLow) : ((ShadowLong_rangeType == 2) ? ((inHigh - inLow) - Math.abs(inClose - inOpen)) : 0.0)))) / ((ShadowLong_rangeType == 2) ? 2.0 : 1.0)))) && /* long shadow */
          (inHigh - ((inClose >= inOpen) ? inClose : inOpen)) > ((ShadowLong_factor * (((ShadowLong_avgPeriod != 0) ? (sp.ShadowLongPeriodTotal / ShadowLong_avgPeriod) : ((ShadowLong_rangeType == 0) ? (Math.abs(inClose - inOpen)) : ((ShadowLong_rangeType == 1) ? (inHigh - inLow) : ((ShadowLong_rangeType == 2) ? ((inHigh - inLow) - Math.abs(inClose - inOpen)) : 0.0)))) / ((ShadowLong_rangeType == 2) ? 2.0 : 1.0)))) && /* long shadow */
          Math.min(inOpen, inClose) <= inLow + (inHigh - inLow) / 2 + ((Near_factor * (((Near_avgPeriod != 0) ? (sp.NearPeriodTotal / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs(inClose - inOpen)) : ((Near_rangeType == 1) ? (inHigh - inLow) : ((Near_rangeType == 2) ? ((inHigh - inLow) - Math.abs(inClose - inOpen)) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) && Math.max(inOpen, inClose) >= inLow + (inHigh - inLow) / 2 - ((Near_factor * (((Near_avgPeriod != 0) ? (sp.NearPeriodTotal / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs(inClose - inOpen)) : ((Near_rangeType == 1) ? (inHigh - inLow) : ((Near_rangeType == 2) ? ((inHigh - inLow) - Math.abs(inClose - inOpen)) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) ) /* body near midpoint */
      {
         sp.cur_outInteger = 100;
      } else {
         sp.cur_outInteger = 0;
      }
      /* add the current range and subtract the first range: this is done after the pattern recognition
       * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
       */
      sp.BodyDojiPeriodTotal += ((BodyDoji_rangeType == 0) ? (Math.abs(inClose - inOpen)) : ((BodyDoji_rangeType == 1) ? (inHigh - inLow) : ((BodyDoji_rangeType == 2) ? ((inHigh - inLow) - Math.abs(inClose - inOpen)) : 0.0))) - ((BodyDoji_rangeType == 0) ? (Math.abs(sp.ring_BodyDojiTrailingIdx_inClose[sp.ringPos_BodyDojiTrailingIdx] - sp.ring_BodyDojiTrailingIdx_inOpen[sp.ringPos_BodyDojiTrailingIdx])) : ((BodyDoji_rangeType == 1) ? (sp.ring_BodyDojiTrailingIdx_inHigh[sp.ringPos_BodyDojiTrailingIdx] - sp.ring_BodyDojiTrailingIdx_inLow[sp.ringPos_BodyDojiTrailingIdx]) : ((BodyDoji_rangeType == 2) ? ((sp.ring_BodyDojiTrailingIdx_inHigh[sp.ringPos_BodyDojiTrailingIdx] - sp.ring_BodyDojiTrailingIdx_inLow[sp.ringPos_BodyDojiTrailingIdx]) - Math.abs(sp.ring_BodyDojiTrailingIdx_inClose[sp.ringPos_BodyDojiTrailingIdx] - sp.ring_BodyDojiTrailingIdx_inOpen[sp.ringPos_BodyDojiTrailingIdx])) : 0.0)));
      sp.ShadowLongPeriodTotal += ((ShadowLong_rangeType == 0) ? (Math.abs(inClose - inOpen)) : ((ShadowLong_rangeType == 1) ? (inHigh - inLow) : ((ShadowLong_rangeType == 2) ? ((inHigh - inLow) - Math.abs(inClose - inOpen)) : 0.0))) - ((ShadowLong_rangeType == 0) ? (Math.abs(sp.ring_ShadowLongTrailingIdx_inClose[sp.ringPos_ShadowLongTrailingIdx] - sp.ring_ShadowLongTrailingIdx_inOpen[sp.ringPos_ShadowLongTrailingIdx])) : ((ShadowLong_rangeType == 1) ? (sp.ring_ShadowLongTrailingIdx_inHigh[sp.ringPos_ShadowLongTrailingIdx] - sp.ring_ShadowLongTrailingIdx_inLow[sp.ringPos_ShadowLongTrailingIdx]) : ((ShadowLong_rangeType == 2) ? ((sp.ring_ShadowLongTrailingIdx_inHigh[sp.ringPos_ShadowLongTrailingIdx] - sp.ring_ShadowLongTrailingIdx_inLow[sp.ringPos_ShadowLongTrailingIdx]) - Math.abs(sp.ring_ShadowLongTrailingIdx_inClose[sp.ringPos_ShadowLongTrailingIdx] - sp.ring_ShadowLongTrailingIdx_inOpen[sp.ringPos_ShadowLongTrailingIdx])) : 0.0)));
      sp.NearPeriodTotal += ((Near_rangeType == 0) ? (Math.abs(inClose - inOpen)) : ((Near_rangeType == 1) ? (inHigh - inLow) : ((Near_rangeType == 2) ? ((inHigh - inLow) - Math.abs(inClose - inOpen)) : 0.0))) - ((Near_rangeType == 0) ? (Math.abs(sp.ring_NearTrailingIdx_inClose[sp.ringPos_NearTrailingIdx] - sp.ring_NearTrailingIdx_inOpen[sp.ringPos_NearTrailingIdx])) : ((Near_rangeType == 1) ? (sp.ring_NearTrailingIdx_inHigh[sp.ringPos_NearTrailingIdx] - sp.ring_NearTrailingIdx_inLow[sp.ringPos_NearTrailingIdx]) : ((Near_rangeType == 2) ? ((sp.ring_NearTrailingIdx_inHigh[sp.ringPos_NearTrailingIdx] - sp.ring_NearTrailingIdx_inLow[sp.ringPos_NearTrailingIdx]) - Math.abs(sp.ring_NearTrailingIdx_inClose[sp.ringPos_NearTrailingIdx] - sp.ring_NearTrailingIdx_inOpen[sp.ringPos_NearTrailingIdx])) : 0.0)));
      sp.ring_BodyDojiTrailingIdx_inOpen[sp.ringPos_BodyDojiTrailingIdx] = inOpen;
      sp.ring_BodyDojiTrailingIdx_inHigh[sp.ringPos_BodyDojiTrailingIdx] = inHigh;
      sp.ring_BodyDojiTrailingIdx_inLow[sp.ringPos_BodyDojiTrailingIdx] = inLow;
      sp.ring_BodyDojiTrailingIdx_inClose[sp.ringPos_BodyDojiTrailingIdx] = inClose;
      sp.ringPos_BodyDojiTrailingIdx = sp.ringPos_BodyDojiTrailingIdx + 1;
      if( sp.ringPos_BodyDojiTrailingIdx >= sp.ringCap_BodyDojiTrailingIdx ) {
         sp.ringPos_BodyDojiTrailingIdx = 0;
      }
      sp.ring_NearTrailingIdx_inOpen[sp.ringPos_NearTrailingIdx] = inOpen;
      sp.ring_NearTrailingIdx_inHigh[sp.ringPos_NearTrailingIdx] = inHigh;
      sp.ring_NearTrailingIdx_inLow[sp.ringPos_NearTrailingIdx] = inLow;
      sp.ring_NearTrailingIdx_inClose[sp.ringPos_NearTrailingIdx] = inClose;
      sp.ringPos_NearTrailingIdx = sp.ringPos_NearTrailingIdx + 1;
      if( sp.ringPos_NearTrailingIdx >= sp.ringCap_NearTrailingIdx ) {
         sp.ringPos_NearTrailingIdx = 0;
      }
      sp.ring_ShadowLongTrailingIdx_inOpen[sp.ringPos_ShadowLongTrailingIdx] = inOpen;
      sp.ring_ShadowLongTrailingIdx_inHigh[sp.ringPos_ShadowLongTrailingIdx] = inHigh;
      sp.ring_ShadowLongTrailingIdx_inLow[sp.ringPos_ShadowLongTrailingIdx] = inLow;
      sp.ring_ShadowLongTrailingIdx_inClose[sp.ringPos_ShadowLongTrailingIdx] = inClose;
      sp.ringPos_ShadowLongTrailingIdx = sp.ringPos_ShadowLongTrailingIdx + 1;
      if( sp.ringPos_ShadowLongTrailingIdx >= sp.ringCap_ShadowLongTrailingIdx ) {
         sp.ringPos_ShadowLongTrailingIdx = 0;
      }
   }
   private RetCode CDLRICKSHAWMAN_OpenCore( CDLRICKSHAWMAN_Stream sp, double inOpen[], double inHigh[], double inLow[], double inClose[], int startIdx, MInteger outBegIdx, MInteger outNBElement, int outInteger[], int outStride )
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
      int historyLen = inOpen.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 || inHigh.length != inOpen.length || inLow.length != inOpen.length || inClose.length != inOpen.length ) {
         return RetCode.BadParam;
      }
      if( historyLen > MAX_INDEX + 1 ) {
         return RetCode.OutOfRangeEndIndex;
      }
      int BodyDoji_rangeType = this.candleSettings[CandleSettingType.BodyDoji.ordinal()].rangeType.ordinal();
      int BodyDoji_avgPeriod = this.candleSettings[CandleSettingType.BodyDoji.ordinal()].avgPeriod;
      double BodyDoji_factor = this.candleSettings[CandleSettingType.BodyDoji.ordinal()].factor;
      int Near_rangeType = this.candleSettings[CandleSettingType.Near.ordinal()].rangeType.ordinal();
      int Near_avgPeriod = this.candleSettings[CandleSettingType.Near.ordinal()].avgPeriod;
      double Near_factor = this.candleSettings[CandleSettingType.Near.ordinal()].factor;
      int ShadowLong_rangeType = this.candleSettings[CandleSettingType.ShadowLong.ordinal()].rangeType.ordinal();
      int ShadowLong_avgPeriod = this.candleSettings[CandleSettingType.ShadowLong.ordinal()].avgPeriod;
      double ShadowLong_factor = this.candleSettings[CandleSettingType.ShadowLong.ordinal()].factor;
      /* Identify the minimum number of price bar needed
       * to calculate at least one output.
       */
      lookbackTotal = CDLRICKSHAWMAN_Lookback();
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
         return RetCode.OutOfRangeEndIndex ;
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
            outInteger[outIdx++ * outStride] = 100;
         } else {
            outInteger[outIdx++ * outStride] = 0;
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
      /* Capture the live batch state into the handle. */
      int cap_BodyDojiTrailingIdx = i - BodyDojiTrailingIdx;
      if( cap_BodyDojiTrailingIdx < 0 || cap_BodyDojiTrailingIdx > historyLen ) {
         return RetCode.InternalError;
      }
      int allocN_BodyDojiTrailingIdx = (cap_BodyDojiTrailingIdx > 0)? cap_BodyDojiTrailingIdx : 1;
      double[] capRing_BodyDojiTrailingIdx_inOpen = new double[allocN_BodyDojiTrailingIdx];
      System.arraycopy(inOpen, historyLen - cap_BodyDojiTrailingIdx, capRing_BodyDojiTrailingIdx_inOpen, 0, cap_BodyDojiTrailingIdx);
      double[] capRing_BodyDojiTrailingIdx_inHigh = new double[allocN_BodyDojiTrailingIdx];
      System.arraycopy(inHigh, historyLen - cap_BodyDojiTrailingIdx, capRing_BodyDojiTrailingIdx_inHigh, 0, cap_BodyDojiTrailingIdx);
      double[] capRing_BodyDojiTrailingIdx_inLow = new double[allocN_BodyDojiTrailingIdx];
      System.arraycopy(inLow, historyLen - cap_BodyDojiTrailingIdx, capRing_BodyDojiTrailingIdx_inLow, 0, cap_BodyDojiTrailingIdx);
      double[] capRing_BodyDojiTrailingIdx_inClose = new double[allocN_BodyDojiTrailingIdx];
      System.arraycopy(inClose, historyLen - cap_BodyDojiTrailingIdx, capRing_BodyDojiTrailingIdx_inClose, 0, cap_BodyDojiTrailingIdx);
      int cap_NearTrailingIdx = i - NearTrailingIdx;
      if( cap_NearTrailingIdx < 0 || cap_NearTrailingIdx > historyLen ) {
         return RetCode.InternalError;
      }
      int allocN_NearTrailingIdx = (cap_NearTrailingIdx > 0)? cap_NearTrailingIdx : 1;
      double[] capRing_NearTrailingIdx_inOpen = new double[allocN_NearTrailingIdx];
      System.arraycopy(inOpen, historyLen - cap_NearTrailingIdx, capRing_NearTrailingIdx_inOpen, 0, cap_NearTrailingIdx);
      double[] capRing_NearTrailingIdx_inHigh = new double[allocN_NearTrailingIdx];
      System.arraycopy(inHigh, historyLen - cap_NearTrailingIdx, capRing_NearTrailingIdx_inHigh, 0, cap_NearTrailingIdx);
      double[] capRing_NearTrailingIdx_inLow = new double[allocN_NearTrailingIdx];
      System.arraycopy(inLow, historyLen - cap_NearTrailingIdx, capRing_NearTrailingIdx_inLow, 0, cap_NearTrailingIdx);
      double[] capRing_NearTrailingIdx_inClose = new double[allocN_NearTrailingIdx];
      System.arraycopy(inClose, historyLen - cap_NearTrailingIdx, capRing_NearTrailingIdx_inClose, 0, cap_NearTrailingIdx);
      int cap_ShadowLongTrailingIdx = i - ShadowLongTrailingIdx;
      if( cap_ShadowLongTrailingIdx < 0 || cap_ShadowLongTrailingIdx > historyLen ) {
         return RetCode.InternalError;
      }
      int allocN_ShadowLongTrailingIdx = (cap_ShadowLongTrailingIdx > 0)? cap_ShadowLongTrailingIdx : 1;
      double[] capRing_ShadowLongTrailingIdx_inOpen = new double[allocN_ShadowLongTrailingIdx];
      System.arraycopy(inOpen, historyLen - cap_ShadowLongTrailingIdx, capRing_ShadowLongTrailingIdx_inOpen, 0, cap_ShadowLongTrailingIdx);
      double[] capRing_ShadowLongTrailingIdx_inHigh = new double[allocN_ShadowLongTrailingIdx];
      System.arraycopy(inHigh, historyLen - cap_ShadowLongTrailingIdx, capRing_ShadowLongTrailingIdx_inHigh, 0, cap_ShadowLongTrailingIdx);
      double[] capRing_ShadowLongTrailingIdx_inLow = new double[allocN_ShadowLongTrailingIdx];
      System.arraycopy(inLow, historyLen - cap_ShadowLongTrailingIdx, capRing_ShadowLongTrailingIdx_inLow, 0, cap_ShadowLongTrailingIdx);
      double[] capRing_ShadowLongTrailingIdx_inClose = new double[allocN_ShadowLongTrailingIdx];
      System.arraycopy(inClose, historyLen - cap_ShadowLongTrailingIdx, capRing_ShadowLongTrailingIdx_inClose, 0, cap_ShadowLongTrailingIdx);
      sp.BodyDojiPeriodTotal = BodyDojiPeriodTotal;
      sp.ShadowLongPeriodTotal = ShadowLongPeriodTotal;
      sp.NearPeriodTotal = NearPeriodTotal;
      sp.ringPos_BodyDojiTrailingIdx = 0;
      sp.ringCap_BodyDojiTrailingIdx = cap_BodyDojiTrailingIdx;
      sp.ring_BodyDojiTrailingIdx_inOpen = capRing_BodyDojiTrailingIdx_inOpen;
      sp.ring_BodyDojiTrailingIdx_inHigh = capRing_BodyDojiTrailingIdx_inHigh;
      sp.ring_BodyDojiTrailingIdx_inLow = capRing_BodyDojiTrailingIdx_inLow;
      sp.ring_BodyDojiTrailingIdx_inClose = capRing_BodyDojiTrailingIdx_inClose;
      sp.ringPos_NearTrailingIdx = 0;
      sp.ringCap_NearTrailingIdx = cap_NearTrailingIdx;
      sp.ring_NearTrailingIdx_inOpen = capRing_NearTrailingIdx_inOpen;
      sp.ring_NearTrailingIdx_inHigh = capRing_NearTrailingIdx_inHigh;
      sp.ring_NearTrailingIdx_inLow = capRing_NearTrailingIdx_inLow;
      sp.ring_NearTrailingIdx_inClose = capRing_NearTrailingIdx_inClose;
      sp.ringPos_ShadowLongTrailingIdx = 0;
      sp.ringCap_ShadowLongTrailingIdx = cap_ShadowLongTrailingIdx;
      sp.ring_ShadowLongTrailingIdx_inOpen = capRing_ShadowLongTrailingIdx_inOpen;
      sp.ring_ShadowLongTrailingIdx_inHigh = capRing_ShadowLongTrailingIdx_inHigh;
      sp.ring_ShadowLongTrailingIdx_inLow = capRing_ShadowLongTrailingIdx_inLow;
      sp.ring_ShadowLongTrailingIdx_inClose = capRing_ShadowLongTrailingIdx_inClose;
      sp.cs_BodyDoji_rangeType = BodyDoji_rangeType;
      sp.cs_BodyDoji_avgPeriod = BodyDoji_avgPeriod;
      sp.cs_BodyDoji_factor = BodyDoji_factor;
      sp.cs_Near_rangeType = Near_rangeType;
      sp.cs_Near_avgPeriod = Near_avgPeriod;
      sp.cs_Near_factor = Near_factor;
      sp.cs_ShadowLong_rangeType = ShadowLong_rangeType;
      sp.cs_ShadowLong_avgPeriod = ShadowLong_avgPeriod;
      sp.cs_ShadowLong_factor = ShadowLong_factor;
      sp.cur_outInteger = outInteger[(outNBElement.value - 1) * outStride];
      return RetCode.Success;
   }
   private RetCode CDLRICKSHAWMAN_OpenBody( CDLRICKSHAWMAN_Stream sp, double inOpen[], double inHigh[], double inLow[], double inClose[], int startIdx )
   {
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      int[] sink_outInteger = new int[1];
      return CDLRICKSHAWMAN_OpenCore( sp, inOpen, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, sink_outInteger, 0 );
   }
   private RetCode CDLRICKSHAWMAN_OpenAndFillBody( CDLRICKSHAWMAN_Stream sp, double inOpen[], double inHigh[], double inLow[], double inClose[], MInteger outBegIdx, MInteger outNBElement, int outInteger[] )
   {
      if( (Object)outInteger == (Object)inOpen || (Object)outInteger == (Object)inHigh || (Object)outInteger == (Object)inLow || (Object)outInteger == (Object)inClose ) {
         return RetCode.BadParam;
      }
      return CDLRICKSHAWMAN_OpenCore( sp, inOpen, inHigh, inLow, inClose, 0, outBegIdx, outNBElement, outInteger, 1 );
   }
   private RetCode CDLRICKSHAWMAN_OpenAndFillInternalBody( CDLRICKSHAWMAN_Stream sp, double inOpen[], double inHigh[], double inLow[], double inClose[], int startIdx, MInteger outBegIdx, MInteger outNBElement, int outInteger[] )
   {
      return CDLRICKSHAWMAN_OpenCore(sp, inOpen, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, outInteger, 1);
   }
   /* CDLRICKSHAWMAN_OpenAndFill anchored at startIdx — the composed-open fusion seam. */
   CDLRICKSHAWMAN_Stream CDLRICKSHAWMAN_OpenAndFillInternal( double inOpen[], double inHigh[], double inLow[], double inClose[], int startIdx, MInteger outBegIdx, MInteger outNBElement, int outInteger[] )
   {
      CDLRICKSHAWMAN_Stream sp = new CDLRICKSHAWMAN_Stream(this);
      RetCode retCode = CDLRICKSHAWMAN_OpenAndFillInternalBody(sp, inOpen, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, outInteger);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("CDLRICKSHAWMAN openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("CDLRICKSHAWMAN openAndFill: internal error");
      }
      throw new IllegalArgumentException("CDLRICKSHAWMAN openAndFill: " + retCode);
   }
   /* Internal startIdx-anchored open behind CDLRICKSHAWMAN_Open (composition seam). */
   CDLRICKSHAWMAN_Stream CDLRICKSHAWMAN_OpenInternal( double inOpen[], double inHigh[], double inLow[], double inClose[], int startIdx )
   {
      CDLRICKSHAWMAN_Stream sp = new CDLRICKSHAWMAN_Stream(this);
      RetCode retCode = CDLRICKSHAWMAN_OpenBody(sp, inOpen, inHigh, inLow, inClose, startIdx);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("CDLRICKSHAWMAN open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("CDLRICKSHAWMAN open: internal error");
      }
      throw new IllegalArgumentException("CDLRICKSHAWMAN open: " + retCode);
   }
   /**
    * Open a live CDLRICKSHAWMAN stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#CDLRICKSHAWMAN} at that bar.
    * <p>The history must hold at least {@code CDLRICKSHAWMAN_Lookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API).
    */
   public CDLRICKSHAWMAN_Stream CDLRICKSHAWMAN_Open( double inOpen[], double inHigh[], double inLow[], double inClose[] )
   {
      return CDLRICKSHAWMAN_OpenInternal(inOpen, inHigh, inLow, inClose, 0);
   }
   /**
    * {@link Core#CDLRICKSHAWMAN_Open} that also fills the output array(s) bit-identically
    * to {@link Core#CDLRICKSHAWMAN} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values.
    * <p>The range written is on the returned handle:
    * {@link CDLRICKSHAWMAN_Stream#fillRange()}.
    */
   public CDLRICKSHAWMAN_Stream CDLRICKSHAWMAN_OpenAndFill( double inOpen[], double inHigh[], double inLow[], double inClose[], int outInteger[] )
   {
      CDLRICKSHAWMAN_Stream sp = new CDLRICKSHAWMAN_Stream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = CDLRICKSHAWMAN_OpenAndFillBody(sp, inOpen, inHigh, inLow, inClose, outBegIdx, outNBElement, outInteger);
      sp.fillRange = new OutRange(outBegIdx.value, outNBElement.value);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("CDLRICKSHAWMAN openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("CDLRICKSHAWMAN openAndFill: internal error");
      }
      throw new IllegalArgumentException("CDLRICKSHAWMAN openAndFill: " + retCode);
   }
