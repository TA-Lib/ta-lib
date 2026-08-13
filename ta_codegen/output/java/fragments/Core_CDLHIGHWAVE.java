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
 *  072404 AC   Creation
 */

   /**
    * Number of leading input bars {@link Core#CDLHIGHWAVE} consumes before it
    * can produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int CDLHIGHWAVE_Lookback( )
   {
      int BodyShort_rangeType = this.candleSettings[CandleSettingType.BodyShort.ordinal()].rangeType.ordinal();
      int BodyShort_avgPeriod = this.candleSettings[CandleSettingType.BodyShort.ordinal()].avgPeriod;
      double BodyShort_factor = this.candleSettings[CandleSettingType.BodyShort.ordinal()].factor;
      int ShadowVeryLong_rangeType = this.candleSettings[CandleSettingType.ShadowVeryLong.ordinal()].rangeType.ordinal();
      int ShadowVeryLong_avgPeriod = this.candleSettings[CandleSettingType.ShadowVeryLong.ordinal()].avgPeriod;
      double ShadowVeryLong_factor = this.candleSettings[CandleSettingType.ShadowVeryLong.ordinal()].factor;
      return Math.max(BodyShort_avgPeriod, ShadowVeryLong_avgPeriod) ;

   }
   RetCode CDLHIGHWAVE_Internal( int startIdx,
                                 int endIdx,
                                 double inOpen[],
                                 double inHigh[],
                                 double inLow[],
                                 double inClose[],
                                 MInteger outBegIdx,
                                 MInteger outNBElement,
                                 int outInteger[] )
   {
      double BodyPeriodTotal = 0;
      double ShadowPeriodTotal = 0;
      int i = 0;
      int outIdx = 0;
      int BodyTrailingIdx = 0;
      int ShadowTrailingIdx = 0;
      int lookbackTotal = 0;
      int BodyShort_rangeType = this.candleSettings[CandleSettingType.BodyShort.ordinal()].rangeType.ordinal();
      int BodyShort_avgPeriod = this.candleSettings[CandleSettingType.BodyShort.ordinal()].avgPeriod;
      double BodyShort_factor = this.candleSettings[CandleSettingType.BodyShort.ordinal()].factor;
      int ShadowVeryLong_rangeType = this.candleSettings[CandleSettingType.ShadowVeryLong.ordinal()].rangeType.ordinal();
      int ShadowVeryLong_avgPeriod = this.candleSettings[CandleSettingType.ShadowVeryLong.ordinal()].avgPeriod;
      double ShadowVeryLong_factor = this.candleSettings[CandleSettingType.ShadowVeryLong.ordinal()].factor;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      /* Identify the minimum number of price bar needed
       * to calculate at least one output.
       */
      lookbackTotal = CDLHIGHWAVE_Lookback();
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
      BodyPeriodTotal = 0;
      BodyTrailingIdx = startIdx - BodyShort_avgPeriod;
      ShadowPeriodTotal = 0;
      ShadowTrailingIdx = startIdx - ShadowVeryLong_avgPeriod;
      i = BodyTrailingIdx;
      while( i < startIdx ) {
         BodyPeriodTotal += ((BodyShort_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyShort_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((BodyShort_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)));
         i += 1;
      }
      i = ShadowTrailingIdx;
      while( i < startIdx ) {
         ShadowPeriodTotal += ((ShadowVeryLong_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowVeryLong_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((ShadowVeryLong_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)));
         i += 1;
      }
      /* Proceed with the calculation for the requested range.
       * Must have:
       * - short real body
       * - very long upper and lower shadow
       * The meaning of "short" and "very long" is specified with TA_SetCandleSettings
       * outInteger is positive (1 to 100) when white or negative (-1 to -100) when black;
       * it does not mean bullish or bearish
       */
      outIdx = 0;
      do {
         if( Math.abs(inClose[i] - inOpen[i]) < ((BodyShort_factor * (((BodyShort_avgPeriod != 0) ? (BodyPeriodTotal / BodyShort_avgPeriod) : ((BodyShort_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyShort_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((BodyShort_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)))) / ((BodyShort_rangeType == 2) ? 2.0 : 1.0)))) && (inHigh[i] - ((inClose[i] >= inOpen[i]) ? inClose[i] : inOpen[i])) > ((ShadowVeryLong_factor * (((ShadowVeryLong_avgPeriod != 0) ? (ShadowPeriodTotal / ShadowVeryLong_avgPeriod) : ((ShadowVeryLong_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowVeryLong_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((ShadowVeryLong_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)))) / ((ShadowVeryLong_rangeType == 2) ? 2.0 : 1.0)))) && (((inClose[i] >= inOpen[i]) ? inOpen[i] : inClose[i]) - inLow[i]) > ((ShadowVeryLong_factor * (((ShadowVeryLong_avgPeriod != 0) ? (ShadowPeriodTotal / ShadowVeryLong_avgPeriod) : ((ShadowVeryLong_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowVeryLong_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((ShadowVeryLong_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)))) / ((ShadowVeryLong_rangeType == 2) ? 2.0 : 1.0)))) ) {
            outInteger[outIdx++] = ((inClose[i] >= inOpen[i]) ? 1 : 0 - 1) * 100;
         } else {
            outInteger[outIdx++] = 0;
         }
         /* add the current range and subtract the first range: this is done after the pattern recognition
          * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
          */
         BodyPeriodTotal += ((BodyShort_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyShort_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((BodyShort_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0))) - ((BodyShort_rangeType == 0) ? (Math.abs(inClose[BodyTrailingIdx] - inOpen[BodyTrailingIdx])) : ((BodyShort_rangeType == 1) ? (inHigh[BodyTrailingIdx] - inLow[BodyTrailingIdx]) : ((BodyShort_rangeType == 2) ? ((inHigh[BodyTrailingIdx] - inLow[BodyTrailingIdx]) - Math.abs(inClose[BodyTrailingIdx] - inOpen[BodyTrailingIdx])) : 0.0)));
         ShadowPeriodTotal += ((ShadowVeryLong_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowVeryLong_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((ShadowVeryLong_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0))) - ((ShadowVeryLong_rangeType == 0) ? (Math.abs(inClose[ShadowTrailingIdx] - inOpen[ShadowTrailingIdx])) : ((ShadowVeryLong_rangeType == 1) ? (inHigh[ShadowTrailingIdx] - inLow[ShadowTrailingIdx]) : ((ShadowVeryLong_rangeType == 2) ? ((inHigh[ShadowTrailingIdx] - inLow[ShadowTrailingIdx]) - Math.abs(inClose[ShadowTrailingIdx] - inOpen[ShadowTrailingIdx])) : 0.0)));
         i += 1;
         BodyTrailingIdx += 1;
         ShadowTrailingIdx += 1;
      } while( i <= endIdx );
      /* All done. Indicate the output limits and return. */
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   RetCode CDLHIGHWAVE_Internal( int startIdx,
                                 int endIdx,
                                 float inOpen[],
                                 float inHigh[],
                                 float inLow[],
                                 float inClose[],
                                 MInteger outBegIdx,
                                 MInteger outNBElement,
                                 int outInteger[] )
   {
      double BodyPeriodTotal = 0;
      double ShadowPeriodTotal = 0;
      int i = 0;
      int outIdx = 0;
      int BodyTrailingIdx = 0;
      int ShadowTrailingIdx = 0;
      int lookbackTotal = 0;
      int BodyShort_rangeType = this.candleSettings[CandleSettingType.BodyShort.ordinal()].rangeType.ordinal();
      int BodyShort_avgPeriod = this.candleSettings[CandleSettingType.BodyShort.ordinal()].avgPeriod;
      double BodyShort_factor = this.candleSettings[CandleSettingType.BodyShort.ordinal()].factor;
      int ShadowVeryLong_rangeType = this.candleSettings[CandleSettingType.ShadowVeryLong.ordinal()].rangeType.ordinal();
      int ShadowVeryLong_avgPeriod = this.candleSettings[CandleSettingType.ShadowVeryLong.ordinal()].avgPeriod;
      double ShadowVeryLong_factor = this.candleSettings[CandleSettingType.ShadowVeryLong.ordinal()].factor;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      lookbackTotal = CDLHIGHWAVE_Lookback();
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      BodyPeriodTotal = 0;
      BodyTrailingIdx = startIdx - BodyShort_avgPeriod;
      ShadowPeriodTotal = 0;
      ShadowTrailingIdx = startIdx - ShadowVeryLong_avgPeriod;
      i = BodyTrailingIdx;
      while( i < startIdx ) {
         BodyPeriodTotal += ((BodyShort_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((BodyShort_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((BodyShort_rangeType == 2) ? (((double)inHigh[i] - (double)inLow[i]) - Math.abs((double)inClose[i] - (double)inOpen[i])) : 0.0)));
         i += 1;
      }
      i = ShadowTrailingIdx;
      while( i < startIdx ) {
         ShadowPeriodTotal += ((ShadowVeryLong_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((ShadowVeryLong_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((ShadowVeryLong_rangeType == 2) ? (((double)inHigh[i] - (double)inLow[i]) - Math.abs((double)inClose[i] - (double)inOpen[i])) : 0.0)));
         i += 1;
      }
      outIdx = 0;
      do {
         if( Math.abs((double)inClose[i] - (double)inOpen[i]) < ((BodyShort_factor * (((BodyShort_avgPeriod != 0) ? (BodyPeriodTotal / BodyShort_avgPeriod) : ((BodyShort_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((BodyShort_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((BodyShort_rangeType == 2) ? (((double)inHigh[i] - (double)inLow[i]) - Math.abs((double)inClose[i] - (double)inOpen[i])) : 0.0)))) / ((BodyShort_rangeType == 2) ? 2.0 : 1.0)))) && ((double)inHigh[i] - (((double)inClose[i] >= (double)inOpen[i]) ? (double)inClose[i] : (double)inOpen[i])) > ((ShadowVeryLong_factor * (((ShadowVeryLong_avgPeriod != 0) ? (ShadowPeriodTotal / ShadowVeryLong_avgPeriod) : ((ShadowVeryLong_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((ShadowVeryLong_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((ShadowVeryLong_rangeType == 2) ? (((double)inHigh[i] - (double)inLow[i]) - Math.abs((double)inClose[i] - (double)inOpen[i])) : 0.0)))) / ((ShadowVeryLong_rangeType == 2) ? 2.0 : 1.0)))) && ((((double)inClose[i] >= (double)inOpen[i]) ? (double)inOpen[i] : (double)inClose[i]) - (double)inLow[i]) > ((ShadowVeryLong_factor * (((ShadowVeryLong_avgPeriod != 0) ? (ShadowPeriodTotal / ShadowVeryLong_avgPeriod) : ((ShadowVeryLong_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((ShadowVeryLong_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((ShadowVeryLong_rangeType == 2) ? (((double)inHigh[i] - (double)inLow[i]) - Math.abs((double)inClose[i] - (double)inOpen[i])) : 0.0)))) / ((ShadowVeryLong_rangeType == 2) ? 2.0 : 1.0)))) ) {
            outInteger[outIdx++] = (((double)inClose[i] >= (double)inOpen[i]) ? 1 : 0 - 1) * 100;
         } else {
            outInteger[outIdx++] = 0;
         }
         BodyPeriodTotal += ((BodyShort_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((BodyShort_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((BodyShort_rangeType == 2) ? (((double)inHigh[i] - (double)inLow[i]) - Math.abs((double)inClose[i] - (double)inOpen[i])) : 0.0))) - ((BodyShort_rangeType == 0) ? (Math.abs((double)inClose[BodyTrailingIdx] - (double)inOpen[BodyTrailingIdx])) : ((BodyShort_rangeType == 1) ? ((double)inHigh[BodyTrailingIdx] - (double)inLow[BodyTrailingIdx]) : ((BodyShort_rangeType == 2) ? (((double)inHigh[BodyTrailingIdx] - (double)inLow[BodyTrailingIdx]) - Math.abs((double)inClose[BodyTrailingIdx] - (double)inOpen[BodyTrailingIdx])) : 0.0)));
         ShadowPeriodTotal += ((ShadowVeryLong_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((ShadowVeryLong_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((ShadowVeryLong_rangeType == 2) ? (((double)inHigh[i] - (double)inLow[i]) - Math.abs((double)inClose[i] - (double)inOpen[i])) : 0.0))) - ((ShadowVeryLong_rangeType == 0) ? (Math.abs((double)inClose[ShadowTrailingIdx] - (double)inOpen[ShadowTrailingIdx])) : ((ShadowVeryLong_rangeType == 1) ? ((double)inHigh[ShadowTrailingIdx] - (double)inLow[ShadowTrailingIdx]) : ((ShadowVeryLong_rangeType == 2) ? (((double)inHigh[ShadowTrailingIdx] - (double)inLow[ShadowTrailingIdx]) - Math.abs((double)inClose[ShadowTrailingIdx] - (double)inOpen[ShadowTrailingIdx])) : 0.0)));
         i += 1;
         BodyTrailingIdx += 1;
         ShadowTrailingIdx += 1;
      } while( i <= endIdx );
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   /**
    * Single-candle pattern: a short real body with both a very long upper and a
    * very long lower shadow. Signals market indecision; the output sign reports
    * only candle color, not a bullish/bearish direction. A hit marks indecision
    * (long-legged candle); not directional - sign encodes only the candle's
    * color.
    * <p><b>Formula</b>
    * <pre>{@code
    * One candle at index i. Hit when all hold: (1) short real body: real body < the BodyShort average; (2) very long upper shadow: upper shadow > the ShadowVeryLong average; (3) very long lower shadow: lower shadow > the ShadowVeryLong average. No color, gap, or trend condition.
    * }</pre>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#CDLHIGHWAVE_Lookback} is a <b>success
    * with no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inOpen Open price of each bar.
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param inClose Close price of each bar.
    * @param outInteger On a hit, +100 when the candle is white (close &gt;=
    *        open) or -100 when black (close &lt; open); 0 otherwise. Sign denotes
    *        color, NOT bull/bear. Must hold at least {@code endIdx - startIdx + 1}
    *        values.
    * @return The range written: {@code begIdx} is the first bar with a value,
    *        {@code count} how many were written.
    * @throws IndexOutOfBoundsException if {@code startIdx} or {@code endIdx} is
    *        negative or above {@link Core#MAX_INDEX}, or {@code endIdx < startIdx}.
    * @throws IllegalArgumentException if an optional parameter is outside its
    *        documented range, or two outputs share one array.
    * @throws NullPointerException if any input or output array is null.
    *
    * @see Core#CDLLONGLEGGEDDOJI
    * @see Core#CDLSPINNINGTOP
    * @see Core#CDLRICKSHAWMAN
    * @see Core#CDLDOJI
    */
   public OutRange CDLHIGHWAVE( int startIdx,
                                int endIdx,
                                double inOpen[],
                                double inHigh[],
                                double inLow[],
                                double inClose[],
                                int outInteger[] )
   {
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = CDLHIGHWAVE_Internal(startIdx, endIdx, inOpen, inHigh, inLow, inClose, outBegIdx, outNBElement, outInteger);
      if( retCode != RetCode.Success ) {
         throw failure("CDLHIGHWAVE", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Single-candle pattern: a short real body with both a very long upper and a
    * very long lower shadow. Signals market indecision; the output sign reports
    * only candle color, not a bullish/bearish direction. A hit marks indecision
    * (long-legged candle); not directional - sign encodes only the candle's
    * color.
    * <p><b>Formula</b>
    * <pre>{@code
    * One candle at index i. Hit when all hold: (1) short real body: real body < the BodyShort average; (2) very long upper shadow: upper shadow > the ShadowVeryLong average; (3) very long lower shadow: lower shadow > the ShadowVeryLong average. No color, gap, or trend condition.
    * }</pre>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#CDLHIGHWAVE_Lookback} is a <b>success
    * with no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inOpen Open price of each bar.
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param inClose Close price of each bar.
    * @param outInteger On a hit, +100 when the candle is white (close &gt;=
    *        open) or -100 when black (close &lt; open); 0 otherwise. Sign denotes
    *        color, NOT bull/bear. Must hold at least {@code endIdx - startIdx + 1}
    *        values.
    * @return The range written: {@code begIdx} is the first bar with a value,
    *        {@code count} how many were written.
    * @throws IndexOutOfBoundsException if {@code startIdx} or {@code endIdx} is
    *        negative or above {@link Core#MAX_INDEX}, or {@code endIdx < startIdx}.
    * @throws IllegalArgumentException if an optional parameter is outside its
    *        documented range, or two outputs share one array.
    * @throws NullPointerException if any input or output array is null.
    *
    * @see Core#CDLLONGLEGGEDDOJI
    * @see Core#CDLSPINNINGTOP
    * @see Core#CDLRICKSHAWMAN
    * @see Core#CDLDOJI
    */
   public OutRange CDLHIGHWAVE( int startIdx,
                                int endIdx,
                                float inOpen[],
                                float inHigh[],
                                float inLow[],
                                float inClose[],
                                int outInteger[] )
   {
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = CDLHIGHWAVE_Internal(startIdx, endIdx, inOpen, inHigh, inLow, inClose, outBegIdx, outNBElement, outInteger);
      if( retCode != RetCode.Success ) {
         throw failure("CDLHIGHWAVE", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live CDLHIGHWAVE stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#CDLHIGHWAVE} over the same series.
    * Open with {@link Core#CDLHIGHWAVE_Open}; there is no close — the handle is
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
   public static final class CDLHIGHWAVE_Stream {
      Core core;
      double BodyPeriodTotal;
      double ShadowPeriodTotal;
      int ringPos_BodyTrailingIdx;
      int ringCap_BodyTrailingIdx;
      double[] ring_BodyTrailingIdx_inOpen;
      double[] ring_BodyTrailingIdx_inHigh;
      double[] ring_BodyTrailingIdx_inLow;
      double[] ring_BodyTrailingIdx_inClose;
      int ringPos_ShadowTrailingIdx;
      int ringCap_ShadowTrailingIdx;
      double[] ring_ShadowTrailingIdx_inOpen;
      double[] ring_ShadowTrailingIdx_inHigh;
      double[] ring_ShadowTrailingIdx_inLow;
      double[] ring_ShadowTrailingIdx_inClose;
      int cs_BodyShort_rangeType;
      int cs_BodyShort_avgPeriod;
      double cs_BodyShort_factor;
      int cs_ShadowVeryLong_rangeType;
      int cs_ShadowVeryLong_avgPeriod;
      double cs_ShadowVeryLong_factor;
      int cur_outInteger;
      OutRange fillRange = OutRange.EMPTY;

      CDLHIGHWAVE_Stream( Core core ) { this.core = core; }

      /**
       * The range filled by {@link Core#CDLHIGHWAVE_OpenAndFill}, or
       * {@link OutRange#EMPTY} when this handle came from a plain
       * {@code open} (which fills nothing). Never {@code null}; a
       * successful {@code openAndFill} always writes at least one value,
       * so {@link OutRange#isEmpty()} tells the two apart.
       */
      public OutRange fillRange() { return fillRange; }

      CDLHIGHWAVE_Stream( CDLHIGHWAVE_Stream other ) {
         this.core = other.core;
         this.BodyPeriodTotal = other.BodyPeriodTotal;
         this.ShadowPeriodTotal = other.ShadowPeriodTotal;
         this.ringPos_BodyTrailingIdx = other.ringPos_BodyTrailingIdx;
         this.ringCap_BodyTrailingIdx = other.ringCap_BodyTrailingIdx;
         this.ring_BodyTrailingIdx_inOpen = other.ring_BodyTrailingIdx_inOpen.clone();
         this.ring_BodyTrailingIdx_inHigh = other.ring_BodyTrailingIdx_inHigh.clone();
         this.ring_BodyTrailingIdx_inLow = other.ring_BodyTrailingIdx_inLow.clone();
         this.ring_BodyTrailingIdx_inClose = other.ring_BodyTrailingIdx_inClose.clone();
         this.ringPos_ShadowTrailingIdx = other.ringPos_ShadowTrailingIdx;
         this.ringCap_ShadowTrailingIdx = other.ringCap_ShadowTrailingIdx;
         this.ring_ShadowTrailingIdx_inOpen = other.ring_ShadowTrailingIdx_inOpen.clone();
         this.ring_ShadowTrailingIdx_inHigh = other.ring_ShadowTrailingIdx_inHigh.clone();
         this.ring_ShadowTrailingIdx_inLow = other.ring_ShadowTrailingIdx_inLow.clone();
         this.ring_ShadowTrailingIdx_inClose = other.ring_ShadowTrailingIdx_inClose.clone();
         this.cs_BodyShort_rangeType = other.cs_BodyShort_rangeType;
         this.cs_BodyShort_avgPeriod = other.cs_BodyShort_avgPeriod;
         this.cs_BodyShort_factor = other.cs_BodyShort_factor;
         this.cs_ShadowVeryLong_rangeType = other.cs_ShadowVeryLong_rangeType;
         this.cs_ShadowVeryLong_avgPeriod = other.cs_ShadowVeryLong_avgPeriod;
         this.cs_ShadowVeryLong_factor = other.cs_ShadowVeryLong_factor;
         this.cur_outInteger = other.cur_outInteger;
         this.fillRange = other.fillRange;
      }

      void copyFrom( CDLHIGHWAVE_Stream other ) {
         this.core = other.core;
         this.BodyPeriodTotal = other.BodyPeriodTotal;
         this.ShadowPeriodTotal = other.ShadowPeriodTotal;
         this.ringPos_BodyTrailingIdx = other.ringPos_BodyTrailingIdx;
         this.ringCap_BodyTrailingIdx = other.ringCap_BodyTrailingIdx;
         if( this.ring_BodyTrailingIdx_inOpen != null && this.ring_BodyTrailingIdx_inOpen.length == other.ring_BodyTrailingIdx_inOpen.length ) {
            System.arraycopy( other.ring_BodyTrailingIdx_inOpen, 0, this.ring_BodyTrailingIdx_inOpen, 0, other.ring_BodyTrailingIdx_inOpen.length );
         } else {
            this.ring_BodyTrailingIdx_inOpen = other.ring_BodyTrailingIdx_inOpen.clone();
         }
         if( this.ring_BodyTrailingIdx_inHigh != null && this.ring_BodyTrailingIdx_inHigh.length == other.ring_BodyTrailingIdx_inHigh.length ) {
            System.arraycopy( other.ring_BodyTrailingIdx_inHigh, 0, this.ring_BodyTrailingIdx_inHigh, 0, other.ring_BodyTrailingIdx_inHigh.length );
         } else {
            this.ring_BodyTrailingIdx_inHigh = other.ring_BodyTrailingIdx_inHigh.clone();
         }
         if( this.ring_BodyTrailingIdx_inLow != null && this.ring_BodyTrailingIdx_inLow.length == other.ring_BodyTrailingIdx_inLow.length ) {
            System.arraycopy( other.ring_BodyTrailingIdx_inLow, 0, this.ring_BodyTrailingIdx_inLow, 0, other.ring_BodyTrailingIdx_inLow.length );
         } else {
            this.ring_BodyTrailingIdx_inLow = other.ring_BodyTrailingIdx_inLow.clone();
         }
         if( this.ring_BodyTrailingIdx_inClose != null && this.ring_BodyTrailingIdx_inClose.length == other.ring_BodyTrailingIdx_inClose.length ) {
            System.arraycopy( other.ring_BodyTrailingIdx_inClose, 0, this.ring_BodyTrailingIdx_inClose, 0, other.ring_BodyTrailingIdx_inClose.length );
         } else {
            this.ring_BodyTrailingIdx_inClose = other.ring_BodyTrailingIdx_inClose.clone();
         }
         this.ringPos_ShadowTrailingIdx = other.ringPos_ShadowTrailingIdx;
         this.ringCap_ShadowTrailingIdx = other.ringCap_ShadowTrailingIdx;
         if( this.ring_ShadowTrailingIdx_inOpen != null && this.ring_ShadowTrailingIdx_inOpen.length == other.ring_ShadowTrailingIdx_inOpen.length ) {
            System.arraycopy( other.ring_ShadowTrailingIdx_inOpen, 0, this.ring_ShadowTrailingIdx_inOpen, 0, other.ring_ShadowTrailingIdx_inOpen.length );
         } else {
            this.ring_ShadowTrailingIdx_inOpen = other.ring_ShadowTrailingIdx_inOpen.clone();
         }
         if( this.ring_ShadowTrailingIdx_inHigh != null && this.ring_ShadowTrailingIdx_inHigh.length == other.ring_ShadowTrailingIdx_inHigh.length ) {
            System.arraycopy( other.ring_ShadowTrailingIdx_inHigh, 0, this.ring_ShadowTrailingIdx_inHigh, 0, other.ring_ShadowTrailingIdx_inHigh.length );
         } else {
            this.ring_ShadowTrailingIdx_inHigh = other.ring_ShadowTrailingIdx_inHigh.clone();
         }
         if( this.ring_ShadowTrailingIdx_inLow != null && this.ring_ShadowTrailingIdx_inLow.length == other.ring_ShadowTrailingIdx_inLow.length ) {
            System.arraycopy( other.ring_ShadowTrailingIdx_inLow, 0, this.ring_ShadowTrailingIdx_inLow, 0, other.ring_ShadowTrailingIdx_inLow.length );
         } else {
            this.ring_ShadowTrailingIdx_inLow = other.ring_ShadowTrailingIdx_inLow.clone();
         }
         if( this.ring_ShadowTrailingIdx_inClose != null && this.ring_ShadowTrailingIdx_inClose.length == other.ring_ShadowTrailingIdx_inClose.length ) {
            System.arraycopy( other.ring_ShadowTrailingIdx_inClose, 0, this.ring_ShadowTrailingIdx_inClose, 0, other.ring_ShadowTrailingIdx_inClose.length );
         } else {
            this.ring_ShadowTrailingIdx_inClose = other.ring_ShadowTrailingIdx_inClose.clone();
         }
         this.cs_BodyShort_rangeType = other.cs_BodyShort_rangeType;
         this.cs_BodyShort_avgPeriod = other.cs_BodyShort_avgPeriod;
         this.cs_BodyShort_factor = other.cs_BodyShort_factor;
         this.cs_ShadowVeryLong_rangeType = other.cs_ShadowVeryLong_rangeType;
         this.cs_ShadowVeryLong_avgPeriod = other.cs_ShadowVeryLong_avgPeriod;
         this.cs_ShadowVeryLong_factor = other.cs_ShadowVeryLong_factor;
         this.cur_outInteger = other.cur_outInteger;
         this.fillRange = other.fillRange;
      }

      /** {@code peek}'s reusable scratch — one per thread, see {@code copyFrom}. */
      private static final ThreadLocal<CDLHIGHWAVE_Stream> PEEK_SCRATCH = new ThreadLocal<>();

      /**
       * Commit one closed bar; always produces the new current value.
       * Never throws after a successful open; never allocates handle state.
       */
      public int update( double inOpen, double inHigh, double inLow, double inClose ) {
         core.CDLHIGHWAVE_StreamStep(this, inOpen, inHigh, inLow, inClose);
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
         CDLHIGHWAVE_Stream scratch = PEEK_SCRATCH.get();
         if( scratch == null ) {
            scratch = new CDLHIGHWAVE_Stream(this);
            PEEK_SCRATCH.set(scratch);
         } else {
            scratch.copyFrom(this);
         }
         core.CDLHIGHWAVE_StreamStep(scratch, inOpen, inHigh, inLow, inClose);
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
      public CDLHIGHWAVE_Stream copy() {
         return new CDLHIGHWAVE_Stream(this);
      }
   }
   void CDLHIGHWAVE_StreamStep( CDLHIGHWAVE_Stream sp, double inOpen, double inHigh, double inLow, double inClose )
   {
      int BodyShort_rangeType = sp.cs_BodyShort_rangeType;
      int BodyShort_avgPeriod = sp.cs_BodyShort_avgPeriod;
      double BodyShort_factor = sp.cs_BodyShort_factor;
      int ShadowVeryLong_rangeType = sp.cs_ShadowVeryLong_rangeType;
      int ShadowVeryLong_avgPeriod = sp.cs_ShadowVeryLong_avgPeriod;
      double ShadowVeryLong_factor = sp.cs_ShadowVeryLong_factor;
      if( sp.ringCap_BodyTrailingIdx == 0 ) {
         sp.ring_BodyTrailingIdx_inOpen[0] = inOpen;
         sp.ring_BodyTrailingIdx_inHigh[0] = inHigh;
         sp.ring_BodyTrailingIdx_inLow[0] = inLow;
         sp.ring_BodyTrailingIdx_inClose[0] = inClose;
      }
      if( sp.ringCap_ShadowTrailingIdx == 0 ) {
         sp.ring_ShadowTrailingIdx_inOpen[0] = inOpen;
         sp.ring_ShadowTrailingIdx_inHigh[0] = inHigh;
         sp.ring_ShadowTrailingIdx_inLow[0] = inLow;
         sp.ring_ShadowTrailingIdx_inClose[0] = inClose;
      }
      if( Math.abs(inClose - inOpen) < ((BodyShort_factor * (((BodyShort_avgPeriod != 0) ? (sp.BodyPeriodTotal / BodyShort_avgPeriod) : ((BodyShort_rangeType == 0) ? (Math.abs(inClose - inOpen)) : ((BodyShort_rangeType == 1) ? (inHigh - inLow) : ((BodyShort_rangeType == 2) ? ((inHigh - inLow) - Math.abs(inClose - inOpen)) : 0.0)))) / ((BodyShort_rangeType == 2) ? 2.0 : 1.0)))) && (inHigh - ((inClose >= inOpen) ? inClose : inOpen)) > ((ShadowVeryLong_factor * (((ShadowVeryLong_avgPeriod != 0) ? (sp.ShadowPeriodTotal / ShadowVeryLong_avgPeriod) : ((ShadowVeryLong_rangeType == 0) ? (Math.abs(inClose - inOpen)) : ((ShadowVeryLong_rangeType == 1) ? (inHigh - inLow) : ((ShadowVeryLong_rangeType == 2) ? ((inHigh - inLow) - Math.abs(inClose - inOpen)) : 0.0)))) / ((ShadowVeryLong_rangeType == 2) ? 2.0 : 1.0)))) && (((inClose >= inOpen) ? inOpen : inClose) - inLow) > ((ShadowVeryLong_factor * (((ShadowVeryLong_avgPeriod != 0) ? (sp.ShadowPeriodTotal / ShadowVeryLong_avgPeriod) : ((ShadowVeryLong_rangeType == 0) ? (Math.abs(inClose - inOpen)) : ((ShadowVeryLong_rangeType == 1) ? (inHigh - inLow) : ((ShadowVeryLong_rangeType == 2) ? ((inHigh - inLow) - Math.abs(inClose - inOpen)) : 0.0)))) / ((ShadowVeryLong_rangeType == 2) ? 2.0 : 1.0)))) ) {
         sp.cur_outInteger = ((inClose >= inOpen) ? 1 : 0 - 1) * 100;
      } else {
         sp.cur_outInteger = 0;
      }
      /* add the current range and subtract the first range: this is done after the pattern recognition
       * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
       */
      sp.BodyPeriodTotal += ((BodyShort_rangeType == 0) ? (Math.abs(inClose - inOpen)) : ((BodyShort_rangeType == 1) ? (inHigh - inLow) : ((BodyShort_rangeType == 2) ? ((inHigh - inLow) - Math.abs(inClose - inOpen)) : 0.0))) - ((BodyShort_rangeType == 0) ? (Math.abs(sp.ring_BodyTrailingIdx_inClose[sp.ringPos_BodyTrailingIdx] - sp.ring_BodyTrailingIdx_inOpen[sp.ringPos_BodyTrailingIdx])) : ((BodyShort_rangeType == 1) ? (sp.ring_BodyTrailingIdx_inHigh[sp.ringPos_BodyTrailingIdx] - sp.ring_BodyTrailingIdx_inLow[sp.ringPos_BodyTrailingIdx]) : ((BodyShort_rangeType == 2) ? ((sp.ring_BodyTrailingIdx_inHigh[sp.ringPos_BodyTrailingIdx] - sp.ring_BodyTrailingIdx_inLow[sp.ringPos_BodyTrailingIdx]) - Math.abs(sp.ring_BodyTrailingIdx_inClose[sp.ringPos_BodyTrailingIdx] - sp.ring_BodyTrailingIdx_inOpen[sp.ringPos_BodyTrailingIdx])) : 0.0)));
      sp.ShadowPeriodTotal += ((ShadowVeryLong_rangeType == 0) ? (Math.abs(inClose - inOpen)) : ((ShadowVeryLong_rangeType == 1) ? (inHigh - inLow) : ((ShadowVeryLong_rangeType == 2) ? ((inHigh - inLow) - Math.abs(inClose - inOpen)) : 0.0))) - ((ShadowVeryLong_rangeType == 0) ? (Math.abs(sp.ring_ShadowTrailingIdx_inClose[sp.ringPos_ShadowTrailingIdx] - sp.ring_ShadowTrailingIdx_inOpen[sp.ringPos_ShadowTrailingIdx])) : ((ShadowVeryLong_rangeType == 1) ? (sp.ring_ShadowTrailingIdx_inHigh[sp.ringPos_ShadowTrailingIdx] - sp.ring_ShadowTrailingIdx_inLow[sp.ringPos_ShadowTrailingIdx]) : ((ShadowVeryLong_rangeType == 2) ? ((sp.ring_ShadowTrailingIdx_inHigh[sp.ringPos_ShadowTrailingIdx] - sp.ring_ShadowTrailingIdx_inLow[sp.ringPos_ShadowTrailingIdx]) - Math.abs(sp.ring_ShadowTrailingIdx_inClose[sp.ringPos_ShadowTrailingIdx] - sp.ring_ShadowTrailingIdx_inOpen[sp.ringPos_ShadowTrailingIdx])) : 0.0)));
      sp.ring_BodyTrailingIdx_inOpen[sp.ringPos_BodyTrailingIdx] = inOpen;
      sp.ring_BodyTrailingIdx_inHigh[sp.ringPos_BodyTrailingIdx] = inHigh;
      sp.ring_BodyTrailingIdx_inLow[sp.ringPos_BodyTrailingIdx] = inLow;
      sp.ring_BodyTrailingIdx_inClose[sp.ringPos_BodyTrailingIdx] = inClose;
      sp.ringPos_BodyTrailingIdx = sp.ringPos_BodyTrailingIdx + 1;
      if( sp.ringPos_BodyTrailingIdx >= sp.ringCap_BodyTrailingIdx ) {
         sp.ringPos_BodyTrailingIdx = 0;
      }
      sp.ring_ShadowTrailingIdx_inOpen[sp.ringPos_ShadowTrailingIdx] = inOpen;
      sp.ring_ShadowTrailingIdx_inHigh[sp.ringPos_ShadowTrailingIdx] = inHigh;
      sp.ring_ShadowTrailingIdx_inLow[sp.ringPos_ShadowTrailingIdx] = inLow;
      sp.ring_ShadowTrailingIdx_inClose[sp.ringPos_ShadowTrailingIdx] = inClose;
      sp.ringPos_ShadowTrailingIdx = sp.ringPos_ShadowTrailingIdx + 1;
      if( sp.ringPos_ShadowTrailingIdx >= sp.ringCap_ShadowTrailingIdx ) {
         sp.ringPos_ShadowTrailingIdx = 0;
      }
   }
   private RetCode CDLHIGHWAVE_OpenCore( CDLHIGHWAVE_Stream sp, double inOpen[], double inHigh[], double inLow[], double inClose[], int startIdx, MInteger outBegIdx, MInteger outNBElement, int outInteger[], int outStride )
   {
      double BodyPeriodTotal = 0;
      double ShadowPeriodTotal = 0;
      int i = 0;
      int outIdx = 0;
      int BodyTrailingIdx = 0;
      int ShadowTrailingIdx = 0;
      int lookbackTotal = 0;
      int historyLen = inOpen.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 || inHigh.length != inOpen.length || inLow.length != inOpen.length || inClose.length != inOpen.length ) {
         return RetCode.BadParam;
      }
      if( historyLen > MAX_INDEX + 1 ) {
         return RetCode.OutOfRangeEndIndex;
      }
      int BodyShort_rangeType = this.candleSettings[CandleSettingType.BodyShort.ordinal()].rangeType.ordinal();
      int BodyShort_avgPeriod = this.candleSettings[CandleSettingType.BodyShort.ordinal()].avgPeriod;
      double BodyShort_factor = this.candleSettings[CandleSettingType.BodyShort.ordinal()].factor;
      int ShadowVeryLong_rangeType = this.candleSettings[CandleSettingType.ShadowVeryLong.ordinal()].rangeType.ordinal();
      int ShadowVeryLong_avgPeriod = this.candleSettings[CandleSettingType.ShadowVeryLong.ordinal()].avgPeriod;
      double ShadowVeryLong_factor = this.candleSettings[CandleSettingType.ShadowVeryLong.ordinal()].factor;
      /* Identify the minimum number of price bar needed
       * to calculate at least one output.
       */
      lookbackTotal = CDLHIGHWAVE_Lookback();
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
      BodyPeriodTotal = 0;
      BodyTrailingIdx = startIdx - BodyShort_avgPeriod;
      ShadowPeriodTotal = 0;
      ShadowTrailingIdx = startIdx - ShadowVeryLong_avgPeriod;
      i = BodyTrailingIdx;
      while( i < startIdx ) {
         BodyPeriodTotal += ((BodyShort_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyShort_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((BodyShort_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)));
         i += 1;
      }
      i = ShadowTrailingIdx;
      while( i < startIdx ) {
         ShadowPeriodTotal += ((ShadowVeryLong_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowVeryLong_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((ShadowVeryLong_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)));
         i += 1;
      }
      /* Proceed with the calculation for the requested range.
       * Must have:
       * - short real body
       * - very long upper and lower shadow
       * The meaning of "short" and "very long" is specified with TA_SetCandleSettings
       * outInteger is positive (1 to 100) when white or negative (-1 to -100) when black;
       * it does not mean bullish or bearish
       */
      outIdx = 0;
      do {
         if( Math.abs(inClose[i] - inOpen[i]) < ((BodyShort_factor * (((BodyShort_avgPeriod != 0) ? (BodyPeriodTotal / BodyShort_avgPeriod) : ((BodyShort_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyShort_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((BodyShort_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)))) / ((BodyShort_rangeType == 2) ? 2.0 : 1.0)))) && (inHigh[i] - ((inClose[i] >= inOpen[i]) ? inClose[i] : inOpen[i])) > ((ShadowVeryLong_factor * (((ShadowVeryLong_avgPeriod != 0) ? (ShadowPeriodTotal / ShadowVeryLong_avgPeriod) : ((ShadowVeryLong_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowVeryLong_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((ShadowVeryLong_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)))) / ((ShadowVeryLong_rangeType == 2) ? 2.0 : 1.0)))) && (((inClose[i] >= inOpen[i]) ? inOpen[i] : inClose[i]) - inLow[i]) > ((ShadowVeryLong_factor * (((ShadowVeryLong_avgPeriod != 0) ? (ShadowPeriodTotal / ShadowVeryLong_avgPeriod) : ((ShadowVeryLong_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowVeryLong_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((ShadowVeryLong_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)))) / ((ShadowVeryLong_rangeType == 2) ? 2.0 : 1.0)))) ) {
            outInteger[outIdx++ * outStride] = ((inClose[i] >= inOpen[i]) ? 1 : 0 - 1) * 100;
         } else {
            outInteger[outIdx++ * outStride] = 0;
         }
         /* add the current range and subtract the first range: this is done after the pattern recognition
          * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
          */
         BodyPeriodTotal += ((BodyShort_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyShort_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((BodyShort_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0))) - ((BodyShort_rangeType == 0) ? (Math.abs(inClose[BodyTrailingIdx] - inOpen[BodyTrailingIdx])) : ((BodyShort_rangeType == 1) ? (inHigh[BodyTrailingIdx] - inLow[BodyTrailingIdx]) : ((BodyShort_rangeType == 2) ? ((inHigh[BodyTrailingIdx] - inLow[BodyTrailingIdx]) - Math.abs(inClose[BodyTrailingIdx] - inOpen[BodyTrailingIdx])) : 0.0)));
         ShadowPeriodTotal += ((ShadowVeryLong_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowVeryLong_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((ShadowVeryLong_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0))) - ((ShadowVeryLong_rangeType == 0) ? (Math.abs(inClose[ShadowTrailingIdx] - inOpen[ShadowTrailingIdx])) : ((ShadowVeryLong_rangeType == 1) ? (inHigh[ShadowTrailingIdx] - inLow[ShadowTrailingIdx]) : ((ShadowVeryLong_rangeType == 2) ? ((inHigh[ShadowTrailingIdx] - inLow[ShadowTrailingIdx]) - Math.abs(inClose[ShadowTrailingIdx] - inOpen[ShadowTrailingIdx])) : 0.0)));
         i += 1;
         BodyTrailingIdx += 1;
         ShadowTrailingIdx += 1;
      } while( i <= endIdx );
      /* All done. Indicate the output limits and return. */
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      /* Capture the live batch state into the handle. */
      int cap_BodyTrailingIdx = i - BodyTrailingIdx;
      if( cap_BodyTrailingIdx < 0 || cap_BodyTrailingIdx > historyLen ) {
         return RetCode.InternalError;
      }
      int allocN_BodyTrailingIdx = (cap_BodyTrailingIdx > 0)? cap_BodyTrailingIdx : 1;
      double[] capRing_BodyTrailingIdx_inOpen = new double[allocN_BodyTrailingIdx];
      System.arraycopy(inOpen, historyLen - cap_BodyTrailingIdx, capRing_BodyTrailingIdx_inOpen, 0, cap_BodyTrailingIdx);
      double[] capRing_BodyTrailingIdx_inHigh = new double[allocN_BodyTrailingIdx];
      System.arraycopy(inHigh, historyLen - cap_BodyTrailingIdx, capRing_BodyTrailingIdx_inHigh, 0, cap_BodyTrailingIdx);
      double[] capRing_BodyTrailingIdx_inLow = new double[allocN_BodyTrailingIdx];
      System.arraycopy(inLow, historyLen - cap_BodyTrailingIdx, capRing_BodyTrailingIdx_inLow, 0, cap_BodyTrailingIdx);
      double[] capRing_BodyTrailingIdx_inClose = new double[allocN_BodyTrailingIdx];
      System.arraycopy(inClose, historyLen - cap_BodyTrailingIdx, capRing_BodyTrailingIdx_inClose, 0, cap_BodyTrailingIdx);
      int cap_ShadowTrailingIdx = i - ShadowTrailingIdx;
      if( cap_ShadowTrailingIdx < 0 || cap_ShadowTrailingIdx > historyLen ) {
         return RetCode.InternalError;
      }
      int allocN_ShadowTrailingIdx = (cap_ShadowTrailingIdx > 0)? cap_ShadowTrailingIdx : 1;
      double[] capRing_ShadowTrailingIdx_inOpen = new double[allocN_ShadowTrailingIdx];
      System.arraycopy(inOpen, historyLen - cap_ShadowTrailingIdx, capRing_ShadowTrailingIdx_inOpen, 0, cap_ShadowTrailingIdx);
      double[] capRing_ShadowTrailingIdx_inHigh = new double[allocN_ShadowTrailingIdx];
      System.arraycopy(inHigh, historyLen - cap_ShadowTrailingIdx, capRing_ShadowTrailingIdx_inHigh, 0, cap_ShadowTrailingIdx);
      double[] capRing_ShadowTrailingIdx_inLow = new double[allocN_ShadowTrailingIdx];
      System.arraycopy(inLow, historyLen - cap_ShadowTrailingIdx, capRing_ShadowTrailingIdx_inLow, 0, cap_ShadowTrailingIdx);
      double[] capRing_ShadowTrailingIdx_inClose = new double[allocN_ShadowTrailingIdx];
      System.arraycopy(inClose, historyLen - cap_ShadowTrailingIdx, capRing_ShadowTrailingIdx_inClose, 0, cap_ShadowTrailingIdx);
      sp.BodyPeriodTotal = BodyPeriodTotal;
      sp.ShadowPeriodTotal = ShadowPeriodTotal;
      sp.ringPos_BodyTrailingIdx = 0;
      sp.ringCap_BodyTrailingIdx = cap_BodyTrailingIdx;
      sp.ring_BodyTrailingIdx_inOpen = capRing_BodyTrailingIdx_inOpen;
      sp.ring_BodyTrailingIdx_inHigh = capRing_BodyTrailingIdx_inHigh;
      sp.ring_BodyTrailingIdx_inLow = capRing_BodyTrailingIdx_inLow;
      sp.ring_BodyTrailingIdx_inClose = capRing_BodyTrailingIdx_inClose;
      sp.ringPos_ShadowTrailingIdx = 0;
      sp.ringCap_ShadowTrailingIdx = cap_ShadowTrailingIdx;
      sp.ring_ShadowTrailingIdx_inOpen = capRing_ShadowTrailingIdx_inOpen;
      sp.ring_ShadowTrailingIdx_inHigh = capRing_ShadowTrailingIdx_inHigh;
      sp.ring_ShadowTrailingIdx_inLow = capRing_ShadowTrailingIdx_inLow;
      sp.ring_ShadowTrailingIdx_inClose = capRing_ShadowTrailingIdx_inClose;
      sp.cs_BodyShort_rangeType = BodyShort_rangeType;
      sp.cs_BodyShort_avgPeriod = BodyShort_avgPeriod;
      sp.cs_BodyShort_factor = BodyShort_factor;
      sp.cs_ShadowVeryLong_rangeType = ShadowVeryLong_rangeType;
      sp.cs_ShadowVeryLong_avgPeriod = ShadowVeryLong_avgPeriod;
      sp.cs_ShadowVeryLong_factor = ShadowVeryLong_factor;
      sp.cur_outInteger = outInteger[(outNBElement.value - 1) * outStride];
      return RetCode.Success;
   }
   private RetCode CDLHIGHWAVE_OpenBody( CDLHIGHWAVE_Stream sp, double inOpen[], double inHigh[], double inLow[], double inClose[], int startIdx )
   {
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      int[] sink_outInteger = new int[1];
      return CDLHIGHWAVE_OpenCore( sp, inOpen, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, sink_outInteger, 0 );
   }
   private RetCode CDLHIGHWAVE_OpenAndFillBody( CDLHIGHWAVE_Stream sp, double inOpen[], double inHigh[], double inLow[], double inClose[], MInteger outBegIdx, MInteger outNBElement, int outInteger[] )
   {
      if( (Object)outInteger == (Object)inOpen || (Object)outInteger == (Object)inHigh || (Object)outInteger == (Object)inLow || (Object)outInteger == (Object)inClose ) {
         return RetCode.BadParam;
      }
      return CDLHIGHWAVE_OpenCore( sp, inOpen, inHigh, inLow, inClose, 0, outBegIdx, outNBElement, outInteger, 1 );
   }
   private RetCode CDLHIGHWAVE_OpenAndFillInternalBody( CDLHIGHWAVE_Stream sp, double inOpen[], double inHigh[], double inLow[], double inClose[], int startIdx, MInteger outBegIdx, MInteger outNBElement, int outInteger[] )
   {
      return CDLHIGHWAVE_OpenCore(sp, inOpen, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, outInteger, 1);
   }
   /* CDLHIGHWAVE_OpenAndFill anchored at startIdx — the composed-open fusion seam. */
   CDLHIGHWAVE_Stream CDLHIGHWAVE_OpenAndFillInternal( double inOpen[], double inHigh[], double inLow[], double inClose[], int startIdx, MInteger outBegIdx, MInteger outNBElement, int outInteger[] )
   {
      CDLHIGHWAVE_Stream sp = new CDLHIGHWAVE_Stream(this);
      RetCode retCode = CDLHIGHWAVE_OpenAndFillInternalBody(sp, inOpen, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, outInteger);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("CDLHIGHWAVE openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("CDLHIGHWAVE openAndFill: internal error");
      }
      throw new IllegalArgumentException("CDLHIGHWAVE openAndFill: " + retCode);
   }
   /* Internal startIdx-anchored open behind CDLHIGHWAVE_Open (composition seam). */
   CDLHIGHWAVE_Stream CDLHIGHWAVE_OpenInternal( double inOpen[], double inHigh[], double inLow[], double inClose[], int startIdx )
   {
      CDLHIGHWAVE_Stream sp = new CDLHIGHWAVE_Stream(this);
      RetCode retCode = CDLHIGHWAVE_OpenBody(sp, inOpen, inHigh, inLow, inClose, startIdx);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("CDLHIGHWAVE open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("CDLHIGHWAVE open: internal error");
      }
      throw new IllegalArgumentException("CDLHIGHWAVE open: " + retCode);
   }
   /**
    * Open a live CDLHIGHWAVE stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#CDLHIGHWAVE} at that bar.
    * <p>The history must hold at least {@code CDLHIGHWAVE_Lookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API).
    */
   public CDLHIGHWAVE_Stream CDLHIGHWAVE_Open( double inOpen[], double inHigh[], double inLow[], double inClose[] )
   {
      return CDLHIGHWAVE_OpenInternal(inOpen, inHigh, inLow, inClose, 0);
   }
   /**
    * {@link Core#CDLHIGHWAVE_Open} that also fills the output array(s) bit-identically
    * to {@link Core#CDLHIGHWAVE} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values.
    * <p>The range written is on the returned handle:
    * {@link CDLHIGHWAVE_Stream#fillRange()}.
    */
   public CDLHIGHWAVE_Stream CDLHIGHWAVE_OpenAndFill( double inOpen[], double inHigh[], double inLow[], double inClose[], int outInteger[] )
   {
      CDLHIGHWAVE_Stream sp = new CDLHIGHWAVE_Stream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = CDLHIGHWAVE_OpenAndFillBody(sp, inOpen, inHigh, inLow, inClose, outBegIdx, outNBElement, outInteger);
      sp.fillRange = new OutRange(outBegIdx.value, outNBElement.value);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("CDLHIGHWAVE openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("CDLHIGHWAVE openAndFill: internal error");
      }
      throw new IllegalArgumentException("CDLHIGHWAVE openAndFill: " + retCode);
   }
